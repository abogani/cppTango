//=====================================================================================================================
//
// file :                zmqeventconsumer.cpp
//
// description :         C++ classes for implementing the event consumer singleton class when used with zmq
//
// author(s) :             E.Taurel
//
// original :             16 August 2011
//
// Copyright (C) :      2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU
// Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//
//====================================================================================================================

#include <tango/internal/net.h>
#include <tango/internal/utils.h>
#include <tango/client/eventconsumer.h>
#include <tango/client/event.h>
#include <tango/server/auto_tango_monitor.h>

#include <cstdio>

#include <omniORB4/internal/giopStream.h>
#include <tango/internal/perf_mon.h>

#ifdef _TG_WINDOWS_
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <process.h>
#else
  #include <unistd.h>
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <fcntl.h>
  #include <arpa/inet.h>
  #include <netinet/in.h> // Needed for systems that do not include this file from arpa/inet.h
#endif

#include <tango/common/pointer_with_lock.h>

using namespace CORBA;

namespace
{
//--------------------------------------------------------------------------------------------------------------------
//
// auxiliary method to repost about malformed event data
//
//--------------------------------------------------------------------------------------------------------------------

void fill_deverror_for_malformed_event_data(const std::string &msg, Tango::DevErrorList &errors)
{
    std::ostringstream o;
    o << "Received malformed data for event " << msg << std::ends;

    errors.length(1);
    errors[0].reason = Tango::string_dup(Tango::API_WrongEventData);
    errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
    errors[0].desc = Tango::string_dup(o.str().c_str());
    errors[0].severity = Tango::ERR;
}
} // namespace

namespace Tango
{

// Performance Monitoring
namespace
{
struct PerfMonSample
{
    static constexpr const size_t k_attr_name_size = 31;
    std::int64_t micros_since_last_event{k_invalid_duration};
    std::int64_t sleep_micros{0};
    std::int64_t process_micros{0};
    std::int64_t first_callback_latency_micros{k_invalid_duration};
    std::uint32_t callback_count{0};
    std::uint32_t wake_count{0};
    char attr_name[k_attr_name_size + 1]{};
    bool discarded{false};

    void json_dump(std::ostream &os)
    {
        os << R"({"attr_name":")" << attr_name << "\"";
        os << R"(,"micros_since_last_event":)";
        if(micros_since_last_event != k_invalid_duration)
        {
            os << micros_since_last_event;
        }
        else
        {
            os << "null";
        }
        os << R"(,"sleep_micros":)" << sleep_micros;
        os << R"(,"process_micros":)" << process_micros;
        os << R"(,"first_callback_latency_micros":)";
        if(first_callback_latency_micros != k_invalid_duration)
        {
            os << first_callback_latency_micros;
        }
        else
        {
            os << "null";
        }
        os << R"(,"callback_count":)" << callback_count;
        os << R"(,"wake_count":)" << wake_count;
        os << R"(,"discarded":)" << std::boolalpha << discarded;
        os << "}";
    }
};

DoubleBuffer<PerfMonSample> g_perf_mon;

// Used to pass the sample into `push_zmq_event` from `run_undetached`
PerfMonSample *g_current_perf_mon_sample = nullptr;
} // namespace

/************************************************************************/
/*                                                                           */
/*             ZmqEventConsumer class                                         */
/*            ----------------                                            */
/*                                                                           */
/************************************************************************/

ZmqEventConsumer::ZmqEventConsumer(ApiUtil *ptr) :
    EventConsumer(ptr),
    omni_thread((void *) ptr),
    zmq_context(1)

{
    TANGO_LOG_DEBUG << "calling Tango::ZmqEventConsumer::ZmqEventConsumer() \n";

    //
    // Initialize the var references
    //

    av = new AttributeValue();
    av3 = new AttributeValue_3();
    ac2 = new AttributeConfig_2();
    ac3 = new AttributeConfig_3();
    ac5 = new AttributeConfig_5();
    adr = new AttDataReady();
    dic = new DevIntrChange();
    del = new DevErrorList();

    start_undetached();
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::run_undetached()
//
// description :
//        Main method for the ZMQ event system reciving thread
//
//-------------------------------------------------------------------------------------------------------------------

void *ZmqEventConsumer::run_undetached(TANGO_UNUSED(void *arg))
{
    //
    // Store thread ID
    //

    thread_id = self()->id();

    //
    // Create the subscriber socket used to receive heartbeats coming from different DS. This socket subscribe to
    // everything because dedicated publishers are used to send the heartbeat events. This socket will be connected
    // to all needed publishers
    //

    heartbeat_sub_sock = new zmq::socket_t(zmq_context, ZMQ_SUB);
    heartbeat_sub_sock->set(zmq::sockopt::linger, DEFAULT_LINGER);

    int reconnect_ivl = -1;
    try
    {
        heartbeat_sub_sock->set(zmq::sockopt::reconnect_ivl, reconnect_ivl);
    }
    catch(zmq::error_t &)
    {
        reconnect_ivl = 30000;
        heartbeat_sub_sock->set(zmq::sockopt::reconnect_ivl, reconnect_ivl);
    }
    heartbeat_sub_sock->set(zmq::sockopt::sndhwm, SUB_SEND_HWM);

    //
    // Create the subscriber socket used to receive events coming from different DS. This socket subscribe to everything
    // because dedicated publishers are used to send the heartbeat events. This socket will be connected to all needed
    // publishers
    //

    event_sub_sock = new zmq::socket_t(zmq_context, ZMQ_SUB);
    event_sub_sock->set(zmq::sockopt::linger, DEFAULT_LINGER);
    event_sub_sock->set(zmq::sockopt::reconnect_ivl, reconnect_ivl);
    event_sub_sock->set(zmq::sockopt::sndhwm, SUB_SEND_HWM);

    //
    // Create the control socket (REQ/REP pattern) and binds it
    //

    control_sock = new zmq::socket_t(zmq_context, ZMQ_REP);
    control_sock->set(zmq::sockopt::linger, DEFAULT_LINGER);
    control_sock->bind(CTRL_SOCK_ENDPOINT);

    set_ctrl_sock_bound();

    //
    // Initialize poll set
    //

    zmq::pollitem_t *items = new zmq::pollitem_t[MAX_SOCKET_SUB];
    int nb_poll_item = 3;

    items[0].socket = control_sock->handle();
    items[1].socket = heartbeat_sub_sock->handle();
    items[2].socket = event_sub_sock->handle();

    for(int loop = 0; loop < nb_poll_item; loop++)
    {
        items[loop].fd = 0;
        items[loop].events = ZMQ_POLLIN;
        items[loop].revents = 0;
    }

    //
    // Enter the infinite loop
    //

    PerfMonSample perf_mon_sample;
    bool do_sample_next_event = false;
    PerfClock::time_point last_event_sampled_timestamp = {};
    while(true)
    {
        zmq::message_t received_event_name, received_endian;
        zmq::message_t received_call, received_event_data;
        zmq::message_t received_ctrl;

        //
        // Init messages used by multicast event
        //

        zmq::message_t mcast_received_event_name;
        zmq::message_t mcast_received_endian;
        zmq::message_t mcast_received_call;
        zmq::message_t mcast_received_event_data;

        // For each performance sample, we want to record how long we were
        // sleeping for, before we received that event, taking into account the
        // fact that we might be woken up for ZMQ messages which are not events,
        // i.e. heartbeat and control messages.
        //
        // This means that, in general, a single event corresponds to multiple trips
        // around the loop.  We take `do_sample_next_event` being true at the start of
        // the loop to mean that we are in the middle of producing a performance sample
        // and that the last time through the loop we got some non-event ZMQ
        // message.
        //
        // If `!do_sample_next_event` at the start of the loop, then we are not in
        // the middle of producing a performance sample, so we check with the
        // global variables if performance sampling is enabled.
        if(!do_sample_next_event && g_perf_mon.lock.try_lock())
        {
            do_sample_next_event = g_perf_mon.enabled;
            if(!g_perf_mon.enabled)
            {
                last_event_sampled_timestamp = {};
            }
            g_perf_mon.lock.unlock();
        }

        //
        // Wait for message. The try/catch is usefull when the process is running under gdb control
        //

        try
        {
            TimeBlockMicros time_block(do_sample_next_event, &perf_mon_sample.sleep_micros);
            zmq::poll(items, nb_poll_item);
            // TANGO_LOG << "Awaken !!!!!!!!" << std::endl;
        }
        catch(zmq::error_t &e)
        {
            if(e.num() == EINTR)
            {
                continue;
            }
        }

        if(do_sample_next_event)
        {
            perf_mon_sample.wake_count++; // We don't care about spurious wakes here
        }

        //
        // Something received by the heartbeat socket ?
        //

        if((items[1].revents & ZMQ_POLLIN) != 0)
        {
            // TANGO_LOG << "For the heartbeat socket" << std::endl;
            try
            {
                auto res = heartbeat_sub_sock->recv(received_event_name, zmq::recv_flags::dontwait);
                if(!res)
                {
                    print_error_message(
                        "First Zmq recv call on heartbeat socket returned false! De-synchronized event system?");
                    items[1].revents = 0;
                    continue;
                }

                res = heartbeat_sub_sock->recv(received_endian, zmq::recv_flags::dontwait);
                if(!res)
                {
                    print_error_message(
                        "Second Zmq recv call on heartbeat socket returned false! De-synchronized event system?");
                    items[1].revents = 0;
                    continue;
                }

                res = heartbeat_sub_sock->recv(received_call, zmq::recv_flags::dontwait);
                if(!res)
                {
                    print_error_message(
                        "Third Zmq recv call on heartbeat socket returned false! De-synchronized event system?");
                    items[1].revents = 0;
                    continue;
                }

                process_heartbeat(received_event_name, received_endian, received_call);
            }
            catch(zmq::error_t &e)
            {
                print_error_message("Zmq exception while receiving heartbeat data!");
                std::cerr << "Error number: " << e.num() << ", error message: " << e.what() << std::endl;
                items[1].revents = 0;
                continue;
            }

            items[1].revents = 0;
        }

        //
        // Something received by the event socket (TCP transport)?
        //

        if((items[2].revents & ZMQ_POLLIN) != 0)
        {
            // We reset `do_sample_next_event` here so that at the start of the
            // loop, we will check if we are sampling again and start producing
            // a new performance sample if we are.
            // The pusher will always reset the `perf_mon_sample` in its
            // dtor, so we can start a new sample, even if we ended up not
            // pushing this one.
            bool do_sample_this_event = do_sample_next_event;
            do_sample_next_event = false;
            SamplePusher<PerfMonSample> pusher{
                do_sample_this_event, perf_mon_sample, *g_perf_mon.front, g_perf_mon.lock};
            TimeBlockMicros time_block{do_sample_this_event, &perf_mon_sample.process_micros};

            if(do_sample_this_event)
            {
                if(last_event_sampled_timestamp != PerfClock::time_point{})
                {
                    perf_mon_sample.micros_since_last_event =
                        duration_micros(last_event_sampled_timestamp, time_block.start);
                }
                last_event_sampled_timestamp = time_block.start;
            }

            try
            {
                auto res = event_sub_sock->recv(received_event_name, zmq::recv_flags::dontwait);
                if(!res)
                {
                    print_error_message(
                        "First Zmq recv call on event socket returned false! De-synchronized event system?");
                    items[2].revents = 0;
                    continue;
                }

                res = event_sub_sock->recv(received_endian, zmq::recv_flags::dontwait);
                if(!res)
                {
                    print_error_message(
                        "Second Zmq recv call on event socket returned false! De-synchronized event system?");
                    items[2].revents = 0;
                    continue;
                }

                res = event_sub_sock->recv(received_call, zmq::recv_flags::dontwait);
                if(!res)
                {
                    print_error_message(
                        "Third Zmq recv call on event socket returned false! De-synchronized event system?");
                    items[2].revents = 0;
                    continue;
                }

                res = event_sub_sock->recv(received_event_data, zmq::recv_flags::dontwait);
                if(!res)
                {
                    print_error_message(
                        "Forth Zmq recv call on event socket returned false! De-synchronized event system?");
                    items[2].revents = 0;
                    continue;
                }

                g_current_perf_mon_sample = do_sample_this_event ? &perf_mon_sample : nullptr;
                process_event(received_event_name, received_endian, received_call, received_event_data);
                g_current_perf_mon_sample = nullptr;
            }
            catch(zmq::error_t &e)
            {
                print_error_message("Zmq exception while receiving event data!");
                std::cerr << "Error number: " << e.num() << ", error message: " << e.what() << std::endl;
                items[2].revents = 0;
                continue;
            }

            items[2].revents = 0;
        }

        //
        // Something received by the control socket?
        //

        if((items[0].revents & ZMQ_POLLIN) != 0)
        {
            // TANGO_LOG << "For the control socket" << std::endl;
            auto result = control_sock->recv(received_ctrl);
            TANGO_ASSERT(result);

            std::string ret_str;
            bool ret = false;

            try
            {
                ret = process_ctrl(received_ctrl, items, nb_poll_item);
                ret_str = "OK";
            }
            catch(zmq::error_t &e)
            {
                ret_str = e.what();
            }
            catch(Tango::DevFailed &e)
            {
                ret_str = e.errors[0].desc;
            }

            zmq::message_t reply(ret_str.size());
            ::memcpy(reply.data(), ret_str.data(), ret_str.size());
            result = control_sock->send(reply, zmq::send_flags::none);
            TANGO_ASSERT(result);

            if(ret)
            {
                delete heartbeat_sub_sock;
                heartbeat_sub_sock = nullptr;
                delete control_sock;
                control_sock = nullptr;
                delete[] items;
                items = nullptr;

                break;
            }

            items[0].revents = 0;
        }

        //
        // Something received by the event socket (mcast transport)?

        for(int loop = 3; loop < nb_poll_item; loop++)
        {
            if((items[loop].revents & ZMQ_POLLIN) != 0)
            {
                mcast_received_event_name.rebuild();
                mcast_received_endian.rebuild();
                mcast_received_call.rebuild();
                mcast_received_event_data.rebuild();

                zmq::socket_ref s(zmq::from_handle, items[loop].socket);

                auto result = s.recv(mcast_received_event_name, zmq::recv_flags::none);
                TANGO_ASSERT(result);
                result = s.recv(mcast_received_endian, zmq::recv_flags::none);
                TANGO_ASSERT(result);
                result = s.recv(mcast_received_call, zmq::recv_flags::none);
                TANGO_ASSERT(result);
                result = s.recv(mcast_received_event_data, zmq::recv_flags::none);
                TANGO_ASSERT(result);

                process_event(
                    mcast_received_event_name, mcast_received_endian, mcast_received_call, mcast_received_event_data);

                items[loop].revents = 0;
            }
        }
    }

    return (void *) nullptr;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::process_heartbeat()
//
// description :
//        Process execution when a message has been received by the heartbeat socket
//
// argument :
//        in :
//            - received_event_name : The full event name
//            - received_endian : The sender endianess
//            - received_call : The call informations (oid - method name...)
//
//---------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::process_heartbeat(zmq::message_t &received_event_name,
                                         zmq::message_t &received_endian,
                                         zmq::message_t &received_call)
{
    //
    // For debug and logging purposes
    //

    if(omniORB::trace(20) != 0)
    {
        omniORB::logger log;
        log << "ZMQ: A heartbeat message has been received" << '\n';
    }
    if(omniORB::trace(30) != 0)
    {
        {
            omniORB::logger log;
            log << "ZMQ: Event name" << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) received_event_name.data(), received_event_name.size());

        {
            omniORB::logger log;
            log << "ZMQ: Endianness" << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) received_endian.data(), received_endian.size());

        {
            omniORB::logger log;
            log << "ZMQ: Call info" << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) received_call.data(), received_call.size());
    }

    //
    // Extract data from messages
    //

    unsigned char endian = ((char *) received_endian.data())[0];
    std::string event_name((char *) received_event_name.data(), received_event_name.size());

    cdrMemoryStream call_info((char *) received_call.data(), received_call.size());
    call_info.setByteSwapFlag(endian != 0u);

    ZmqCallInfo_var c_info_var = new ZmqCallInfo;
    try
    {
        (ZmqCallInfo &) c_info_var <<= call_info;
    }
    catch(...)
    {
        std::string st("Received a malformed heartbeat event: ");
        st = st + event_name;
        print_error_message(st.c_str());
        unsigned char *tmp = (unsigned char *) received_call.data();
        for(unsigned int loop = 0; loop < received_call.size(); loop++)
        {
            std::cerr << "Heartbeat event data[" << loop << "] = " << std::hex << (int) tmp[loop] << std::dec
                      << std::endl;
        }
        return;
    }

    //
    // Call the heartbeat method
    //

    push_heartbeat_event(event_name);
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::process_event()
//
// description :
//        Process execution when a message has been received by the event socket
//
// argument :
//        in :
//            - received_event_name : The full event name
//            - received_endian : The sender endianess
//            - received_call : The call informations (oid - method name...)
//            - event_data : The event data !
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::process_event(zmq::message_t &received_event_name,
                                     zmq::message_t &received_endian,
                                     zmq::message_t &received_call,
                                     zmq::message_t &event_data)
{
    // TANGO_LOG << "event name message adr = " << (void *)(&received_event_name) << " - size = " <<
    // received_event_name.size() << " - ptr = " << (void *)(received_event_name.data()) << std::endl; TANGO_LOG <<
    // "endian message adr = " << (void *)(&received_endian) << " - size = " << received_endian.size() << " - ptr = " <<
    // (void *)(received_endian.data()) << std::endl; TANGO_LOG << "call info message adr = " << (void
    // *)(&received_call) << " - size = " << received_call.size() << " - ptr = " << (void *)(received_call.data()) <<
    // std::endl; TANGO_LOG << "event data message adr = " << (void *)(&event_data) << " - size = " << event_data.size()
    // << " - ptr = " << (void *)(event_data.data()) << std::endl;

    //
    // For debug and logging purposes
    //

    if(omniORB::trace(20) != 0)
    {
        omniORB::logger log;
        log << "ZMQ: A event message has been received" << '\n';
    }
    if(omniORB::trace(30) != 0)
    {
        {
            omniORB::logger log;
            log << "ZMQ: Event name" << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) received_event_name.data(), received_event_name.size());

        {
            omniORB::logger log;
            log << "ZMQ: Endianness" << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) received_endian.data(), received_endian.size());

        {
            omniORB::logger log;
            log << "ZMQ: Call info" << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) received_call.data(), received_call.size());

        {
            omniORB::logger log;
            log << "ZMQ: Event data" << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) event_data.data(), event_data.size());
    }

    //
    // Extract data from messages
    //

    const ZmqCallInfo *receiv_call;

    unsigned char endian = ((char *) received_endian.data())[0];
    std::string event_name((char *) received_event_name.data(), received_event_name.size());

    cdrMemoryStream call_info((char *) received_call.data(), received_call.size());
    call_info.setByteSwapFlag(endian != 0u);

    ZmqCallInfo_var c_info_var = new ZmqCallInfo;
    try
    {
        (ZmqCallInfo &) c_info_var <<= call_info;
    }
    catch(...)
    {
        std::string st("Received a malformed event call info data for event ");
        st = st + event_name;
        print_error_message(st.c_str());
        unsigned char *tmp = (unsigned char *) received_call.data();
        for(unsigned int loop = 0; loop < received_call.size(); loop++)
        {
            std::cerr << "Event data[" << loop << "] = " << std::hex << (int) tmp[loop] << std::dec << std::endl;
        }
        return;
    }
    receiv_call = &c_info_var.in();

    //
    // Call the event method
    //

    push_zmq_event(event_name, endian, event_data, receiv_call->call_is_except, receiv_call->ctr);
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::process_ctrl()
//
// description :
//        Process task when something has been received by the control socket
//
// argument :
//        in :
//            - received_ctrl :  The received data
//           - poll_list :      The ZMQ poll ed item list
//           - poll_nb :        The item number in previous list
//
// return :
//         This method returns true if the calling thread has to exit (because a ZMQ_END command has been received).
//        Otherwise, it returns false
//
//--------------------------------------------------------------------------------------------------------------------

bool ZmqEventConsumer::process_ctrl(zmq::message_t &received_ctrl, zmq::pollitem_t *poll_list, int &poll_nb)
{
    bool ret = false;

    //
    // For debug and logging purposes
    //

    if(omniORB::trace(20) != 0)
    {
        omniORB::logger log;
        log << "ZMQ: A control message has been received" << '\n';
    }
    if(omniORB::trace(30) != 0)
    {
        {
            omniORB::logger log;
            log << "ZMQ: Control data " << '\n';
        }
        omni::giopStream::dumpbuf((unsigned char *) received_ctrl.data(), received_ctrl.size());
    }

    //
    // Extract cmd code from messages
    //

    const char *tmp_ptr = (const char *) received_ctrl.data();
    char cmd_code = tmp_ptr[0];

    //
    // Process each command
    //

    switch(cmd_code)
    {
    case ZMQ_END:
    {
        ret = true;
    }
    break;

    case ZMQ_CONNECT_HEARTBEAT:
    {
        //
        // First extract the endpoint and the event name from received buffer
        //

        char force_connect = tmp_ptr[1];
        const char *endpoint = &(tmp_ptr[2]);
        int start = ::strlen(endpoint) + 3;
        const char *event_name = &(tmp_ptr[start]);

        //
        // Connect the heartbeat socket to the new publisher if not already done
        //

        bool connect_heart = false;

        if(!connected_heartbeat.empty())
        {
            if(force_connect == 1)
            {
                connect_heart = true;
            }
            else
            {
                std::vector<std::string>::iterator pos;
                pos = find(connected_heartbeat.begin(), connected_heartbeat.end(), endpoint);
                if(pos == connected_heartbeat.end())
                {
                    connect_heart = true;
                }
            }
        }
        else
        {
            connect_heart = true;
        }

        if(connect_heart)
        {
            heartbeat_sub_sock->connect(endpoint);
            if(force_connect == 0)
            {
                connected_heartbeat.emplace_back(endpoint);
            }
        }

        //
        // Subscribe to the new heartbeat event
        //

        heartbeat_sub_sock->set(zmq::sockopt::subscribe, event_name);

        //
        // Most of the time, we have only one TANGO_HOST to take into account and we dont need to execute following
        // code. But there are some control system where several TANGO_HOST are defined
        //

        if(env_var_fqdn_prefix.size() > 1)
        {
            std::string base_name(event_name);
            multi_tango_host(heartbeat_sub_sock, SUBSCRIBE, base_name);
        }
    }
    break;

    case ZMQ_DISCONNECT_HEARTBEAT:
    {
        //
        // Get event name and endpoint name
        //

        const char *event_name = &(tmp_ptr[1]);
        const char *endpoint = &(tmp_ptr[1 + ::strlen(event_name) + 1]);
        const char *endpoint_event = &(tmp_ptr[1 + ::strlen(event_name) + ::strlen(endpoint) + 2]);

        //
        // Unsubscribe this event from the heartbeat socket
        //

        heartbeat_sub_sock->set(zmq::sockopt::unsubscribe, event_name);

        //
        // Most of the time, we have only one TANGO_HOST to take into account and we don need to execute following code.
        // But there are some control system where several TANGO_HOST are defined
        //

        if(env_var_fqdn_prefix.size() > 1)
        {
            std::string base_name(event_name);
            multi_tango_host(heartbeat_sub_sock, UNSUBSCRIBE, base_name);
        }

        //
        // Remove the endpoint in the vector of already connected heartbeat and disconnect the socket to this endpoint
        //

        std::vector<std::string>::iterator pos;
        std::string endpoint_str(endpoint);
        pos = find(connected_heartbeat.begin(), connected_heartbeat.end(), endpoint_str);
        if(pos != connected_heartbeat.end())
        {
            connected_heartbeat.erase(pos);
        }

        disconnect_socket(*heartbeat_sub_sock, endpoint);

        //
        // Remove the event endpoint from the already connected event and disconnect the event socket
        //

        pos = find(connected_pub.begin(), connected_pub.end(), std::string(endpoint_event));
        if(pos != connected_pub.end())
        {
            connected_pub.erase(pos);
            disconnect_socket(*event_sub_sock, endpoint_event);
        }
    }
    break;

    case ZMQ_CONNECT_EVENT:
    {
        //
        // First extract the endpoint and the event name from received buffer
        //

        char force_connect = tmp_ptr[1];
        const char *endpoint = &(tmp_ptr[2]);
        int start = ::strlen(endpoint) + 3;
        const char *event_name = &(tmp_ptr[start]);
        start = start + ::strlen(event_name) + 1;
        Tango::DevLong sub_hwm;
        ::memcpy(&sub_hwm, &(tmp_ptr[start]), sizeof(Tango::DevLong));

        //
        // Connect the socket to the publisher
        //

        bool connect_pub = false;

        if(!connected_pub.empty())
        {
            if(force_connect == 1)
            {
                connect_pub = true;
            }
            else
            {
                std::vector<std::string>::iterator pos;
                pos = find(connected_pub.begin(), connected_pub.end(), endpoint);
                if(pos == connected_pub.end())
                {
                    connect_pub = true;
                }
            }
        }
        else
        {
            connect_pub = true;
        }

        if(connect_pub)
        {
            set_socket_hwm((int) sub_hwm);

            event_sub_sock->connect(endpoint);
            if(force_connect == 0)
            {
                connected_pub.emplace_back(endpoint);
            }
        }

        //
        // Subscribe to the new event
        //

        event_sub_sock->set(zmq::sockopt::subscribe, event_name);

        //
        // Most of the time, we have only one TANGO_HOST to take into account and we don't need to execute following
        // code. But there are some control system where several TANGO_HOST are defined!
        //

        if(env_var_fqdn_prefix.size() > 1)
        {
            std::string base_name(event_name);
            multi_tango_host(event_sub_sock, SUBSCRIBE, base_name);
        }
    }
    break;

    case ZMQ_DISCONNECT_EVENT:
    {
        //
        // Get event name
        //

        const char *event_name = &(tmp_ptr[1]);
        std::string ev_name(event_name);

        //
        // Check if it is a multicast event
        //

        bool mcast = false;

        std::map<std::string, zmq::socket_t *>::iterator pos;
        if(!event_mcast.empty())
        {
            pos = event_mcast.find(ev_name);
            if(pos != event_mcast.end())
            {
                mcast = true;
            }
        }

        //
        // Unsubscribe this event from the socket
        //

        if(!mcast)
        {
            event_sub_sock->set(zmq::sockopt::unsubscribe, event_name);

            //
            // Most of the time, we have only one TANGO_HOST to take into account and we don need to execute following
            // code. But there are some control system where several TANGO_HOST are defined
            //

            if(env_var_fqdn_prefix.size() > 1)
            {
                std::string base_name(event_name);
                multi_tango_host(event_sub_sock, UNSUBSCRIBE, base_name);
            }
        }
        else
        {
            delete pos->second;
            event_mcast.erase(pos);
            old_poll_nb--;
        }
    }
    break;

    case ZMQ_CONNECT_MCAST_EVENT:
    {
        //
        // First extract the endpoint and the event name from received buffer
        //

        const char *endpoint = &(tmp_ptr[2]);
        int start = ::strlen(endpoint) + 3;
        const char *event_name = &(tmp_ptr[start]);
        start = start + ::strlen(event_name) + 1;
        Tango::DevLong sub_hwm, rate, ivl;
        ::memcpy(&sub_hwm, &(tmp_ptr[start]), sizeof(Tango::DevLong));
        start = start + sizeof(Tango::DevLong);
        ::memcpy(&rate, &(tmp_ptr[start]), sizeof(Tango::DevLong));
        start = start + sizeof(Tango::DevLong);
        ::memcpy(&ivl, &(tmp_ptr[start]), sizeof(Tango::DevLong));

        //
        // Connect the socket to the publisher
        //

        bool created_sub = false;
        std::string ev_name(event_name);
        std::map<std::string, zmq::socket_t *>::iterator pos;

        if(!event_mcast.empty())
        {
            pos = event_mcast.find(ev_name);
            if(pos != event_mcast.end())
            {
                created_sub = true;
            }
        }

        if(!created_sub)
        {
            //
            // Check that we are not at the socket high limit
            //

            if(poll_nb == MAX_SOCKET_SUB)
            {
                TANGO_THROW_EXCEPTION(API_InternalError, "Array to store sockets for zmq poll() call is already full");
            }

            //
            // Create the socket
            //

            zmq::socket_t *tmp_sock = new zmq::socket_t(zmq_context, ZMQ_SUB);

            //
            // Set socket rate, ivl linger and hwm
            //

            tmp_sock->set(zmq::sockopt::rate, static_cast<int>(rate));
            tmp_sock->set(zmq::sockopt::recovery_ivl, static_cast<int>(ivl));
            tmp_sock->set(zmq::sockopt::linger, DEFAULT_LINGER);
            tmp_sock->set(zmq::sockopt::rcvhwm, static_cast<int>(sub_hwm));

            //
            // Connect the socket
            //

            tmp_sock->connect(endpoint);

            //
            // Subscribe to the new event
            //

            tmp_sock->set(zmq::sockopt::subscribe, event_name);

            //
            // Store socket in map
            //

            if(!event_mcast.insert(make_pair(ev_name, tmp_sock)).second)
            {
                delete tmp_sock;
                print_error_message("Error while inserting pair<event name,mcast socket> in map!");

                TANGO_THROW_EXCEPTION(API_InternalError,
                                      "Error while inserting pair<event name,multicast socket> in map");
            }

            //
            // Update poll item list
            //

            poll_list[old_poll_nb].socket = tmp_sock->handle();
            poll_list[old_poll_nb].fd = 0;
            poll_list[old_poll_nb].events = ZMQ_POLLIN;
            poll_list[old_poll_nb].revents = 0;

            old_poll_nb++;
        }
    }
    break;

    case ZMQ_DELAY_EVENT:
    {
        // If poll_nb == 1, then we are already in a situation where events are being delayed
        // and we are currently only taking care of messages received on the control socket
        // No need to update old_poll_nb in this case because it is already correct
        // otherwise this would lead to issues like https://github.com/tango-controls/cppTango/issues/686
        // where events would no longer be received if someone subscribes or unsubscribes to events in
        // an event callback and when the callback is executed during a subscribe_event call
        if(poll_nb != 1)
        {
            old_poll_nb = poll_nb;
            poll_nb = 1;
        }
        nb_current_delay_event_requests++;
    }
    break;

    case ZMQ_RELEASE_EVENT:
    {
        if(nb_current_delay_event_requests >= 1)
        {
            nb_current_delay_event_requests--;
        }
        if(nb_current_delay_event_requests == 0)
        {
            // Stop delaying events only if there is no other ZMQ_DELAY_EVENT command requested
            poll_nb = old_poll_nb;
        }
    }
    break;

    default:
        print_error_message("ZMQ main thread: Received an unknown command code from control socket!");
        break;
    }

    return ret;
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::multi_tango_host()
//
// description :
//        Method to execute a ZMQ socket command (actually only SUBSCRIBE or UNSUBSCRIBE) when several TANGO_HOST is
//        used in a control system
//
// argument :
//        in :
//            - sock : The ZMQ socket
//             - cmd : The command to be done on socket
//            - event_name: Event name
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::multi_tango_host(zmq::socket_t *sock, SocketCmd cmd, const std::string &event_name)
{
    size_t pos = event_name.find('/', 8);
    std::string base_tango_host = event_name.substr(0, pos + 1);
    std::string ev_name = event_name.substr(pos + 1);
    for(unsigned int loop = 0; loop < env_var_fqdn_prefix.size(); loop++)
    {
        if(env_var_fqdn_prefix[loop] == base_tango_host)
        {
            continue;
        }
        else
        {
            std::string new_tango_host = env_var_fqdn_prefix[loop] + ev_name;
            const char *tmp_ev_name = new_tango_host.c_str();
            if(cmd == SUBSCRIBE)
            {
                sock->set(zmq::sockopt::subscribe, tmp_ev_name);
            }
            else
            {
                sock->set(zmq::sockopt::unsubscribe, tmp_ev_name);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::query_event_system()
//
// description :
//  Report information about the event consumer as a JSON object.
//
// argument :
//        in :
//          - os : Output stream to write the JSON object to
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::query_event_system(std::ostream &os)
{
    {
        ReaderLock l(map_modification_lock);
        os << "{\"event_callbacks\":{";
        {
            bool first = true;
            for(const auto &[name, obj] : event_callback_map)
            {
                if(!first)
                {
                    os << ",";
                }
                os << "\"" << name << "\":{";
                os << R"("channel_name":")" << obj.channel_name << "\"";
                os << R"(,"callback_count":)" << obj.callback_list.size();
                os << R"(,"server_counter":)" << obj.ctr;
                os << R"(,"event_count":)" << obj.event_count;
                os << R"(,"missed_event_count":)" << obj.missed_event_count;
                os << R"(,"discarded_event_count":)" << obj.discarded_event_count;
                os << R"(,"last_resubscribed":)";
                if(obj.last_subscribed == 0)
                {
                    os << "null";
                }
                else
                {
                    os << "\"" << std::put_time(std::gmtime(&obj.last_subscribed), "%Y-%m-%dT%H:%M:%S") << "\"";
                }
                os << "}";
                first = false;
            }
        }
        os << R"(},"not_connected":[)";
        {
            bool first = true;
            for(const auto &obj : event_not_connected)
            {
                if(!first)
                {
                    os << ",";
                }
                os << "{";
                os << R"("device":")";
                if(obj.device->is_dbase_used())
                {
                    os << obj.device->dev_name();
                }
                else
                {
                    os << "tango://" << obj.device->get_dev_host() << ":" << obj.device->get_dev_port() << "/"
                       << obj.device->dev_name() << "#dbase=no";
                }
                os << R"(","attribute":")" << obj.attribute << "\"";
                os << R"(,"event_type":")" << obj.event_name << "\"";
                os << R"(,"last_heartbeat":)";
                if(obj.last_heartbeat == 0)
                {
                    os << "null";
                }
                else
                {
                    os << "\"" << std::put_time(std::gmtime(&obj.last_heartbeat), "%Y-%m-%dT%H:%M:%S") << "\"";
                }
                os << R"(,"tango_host":)";
                if(obj.prefix != TangoHostNotSet)
                {
                    os << "\"" << obj.prefix << "\"";
                }
                else
                {
                    os << "null";
                }

                os << "}";

                first = false;
            }
        }
        os << R"(],"event_channels":{)";
        {
            bool first = true;
            for(const auto &[name, obj] : channel_map)
            {
                if(!first)
                {
                    os << ",";
                }
                os << "\"" << name << "\":{";
                os << R"("endpoint":")" << obj.endpoint << "\"";
                os << "}";
                first = false;
            }
        }
    }
    os << R"(},"perf":)";
    g_perf_mon.json_dump(os);
    os << "}";
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::enable_perf_mon()
//
// description :
//  Enable or disable collection of performance counters for supplier
//
// argument :
//        in :
//          - enabled : If true, enable sampling otherwise disable sampling
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::enable_perf_mon(Tango::DevBoolean enabled)
{
    g_perf_mon.enable(enabled);
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::cleanup_EventChannel_map()
//
// description :
//        Method to destroy the DeviceProxy objects stored in the EventChannel map.
//      It also destroys some allocated objects (to make valgrind happy)
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::cleanup_EventChannel_map()
{
    EvChanIte evt_it;

    for(evt_it = channel_map.begin(); evt_it != channel_map.end(); ++evt_it)
    {
        EventChannelStruct &evt_ch = evt_it->second;
        if((evt_ch.channel_type == ZMQ) && (evt_ch.adm_device_proxy != nullptr))
        {
            AutoTangoMonitor _mon(evt_ch.channel_monitor);

            //
            // Release the connection to the device server administration device
            //

            evt_ch.adm_device_proxy = nullptr;
        }
        evt_ch.channel_monitor = nullptr;
    }

    //
    // Delete a Tango monitor in Callback structs
    //

    EvCbIte cb_it;

    for(cb_it = event_callback_map.begin(); cb_it != event_callback_map.end(); ++cb_it)
    {
        EventCallBackStruct &evt_cb = cb_it->second;
        delete evt_cb.callback_monitor;
        evt_cb.callback_monitor = nullptr;
    }

    //
    // Create and connect the REQ socket used to send message to the ZMQ main thread
    //

    zmq::message_t reply;

    try
    {
        zmq::socket_t sender(zmq_context, ZMQ_REQ);
        sender.connect(CTRL_SOCK_ENDPOINT);

        //
        // Build message sent to ZMQ main thread. In this case, this is only a command code
        //

        char buffer[10];
        int length = 0;

        buffer[length] = ZMQ_END;
        length++;

        //
        // Send command to main ZMQ thread
        //

        zmq::message_t send_data(length);
        ::memcpy(send_data.data(), buffer, length);
        auto result = sender.send(send_data, zmq::send_flags::none);
        TANGO_ASSERT(result);

        result = sender.recv(reply);
        TANGO_ASSERT(result);
    }
    catch(zmq::error_t &)
    {
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::connect_event_channel()
//
// description :
//        Connect to the event channel. This means connect to the heartbeat event
//
// argument :
//        in :
//            - channel name : The event channel name (DS admin name)
//            - db : Database object
//            - reconnect: Flag set to true in case this method is called for event reconnection purpose
//            - dd : The DS admin device command returned data (ZmqEventSubscriptionChange command)
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::connect_event_channel(const std::string &channel_name,
                                             TANGO_UNUSED(Database *db),
                                             bool reconnect,
                                             DeviceData &dd)
{
    // Channel name may change during reconnection. In this case the caller is responsible
    // for updating full_adm_name field inside channel_map before calling this function.
    auto event_channel_info = channel_map.end();
    std::string event_channel_name = channel_name;
    if(reconnect)
    {
        event_channel_info = channel_map.find(channel_name);
        TANGO_ASSERT(event_channel_info != channel_map.end());
        event_channel_name = event_channel_info->second.full_adm_name;
    }

    //
    // Extract server command result
    //

    const DevVarLongStringArray *ev_svr_data;
    dd >> ev_svr_data;

    //
    // Do we have this tango host info in the vector of possible TANGO_HOST. If not get them
    //

    std::string prefix = event_channel_name.substr(0, event_channel_name.find('/', 8) + 1);
    bool found = false;
    for(const auto &elem : env_var_fqdn_prefix)
    {
        if(elem == prefix)
        {
            found = true;
            break;
        }
    }

    if(!found && db != nullptr)
    {
        auto vs = detail::get_databases_from_control_system(db);
        update_alias_map(db, vs);
        detail::append_fqdn_host_prefixes_from_db(vs, env_var_fqdn_prefix);
    }

    //
    // If the server has returned several possible ZMQ endpoints (because several NIC boards on server host), check
    // which one is correct
    //
    size_t nb_endpoints = ev_svr_data->svalue.length();

    Tango::DevLong server_tango_lib_version = ev_svr_data->lvalue[0];
    if(server_tango_lib_version >= 930)
    {
        // ZmqEventSubscriptionChange returns the ZMQ message filters used for the attribute
        // and for the heartbeat events at the end of the string array part of the returned DevVarLongStringArray
        nb_endpoints -= 2;
    }

    nb_endpoints = nb_endpoints >> 1;
    size_t valid_endpoint = 0;

    if(nb_endpoints != 1)
    {
        for(valid_endpoint = 0; valid_endpoint < nb_endpoints; valid_endpoint++)
        {
            std::string endpoint(ev_svr_data->svalue[valid_endpoint << 1]);

            TANGO_LOG_DEBUG << "Trying alternate endpoint: " << endpoint << std::endl;

            if(check_zmq_endpoint(endpoint))
            {
                TANGO_LOG_DEBUG << "Plain IPv4 address and OK: " << endpoint << std::endl;
                break;
            }

            if(!detail::is_ip_address(endpoint))
            {
                TANGO_LOG_DEBUG << "Maybe this (" << endpoint << ") is a hostname?" << std::endl;

                std::string hostname, port;
                detail::split_endpoint(endpoint, hostname, port);

                std::string resolved;

                try
                {
                    auto results = detail::resolve_hostname_address(hostname);

                    resolved = results.front();
                }
                catch(Tango::DevFailed &)
                {
                    TANGO_LOG_DEBUG << "Could not resolve hostname: " << hostname << std::endl;
                    continue;
                }

                TANGO_LOG_DEBUG << "Trying again with resolved IPv4 address: " << resolved << std::endl;

                if(check_zmq_endpoint(detail::qualify_host_address(resolved, port)))
                {
                    TANGO_LOG_DEBUG << "Resolvable hostname which can be reached as well." << std::endl;
                    break;
                }

                TANGO_LOG_DEBUG << "Can't connect to hostname" << hostname << " which resolved to " << resolved
                                << std::endl;
                TANGO_LOG_DEBUG << "Removing " << endpoint << " from the list of possible alternate endpoints."
                                << std::endl;
            }
        }

        if(valid_endpoint == nb_endpoints)
        {
            std::stringstream o;

            o << "Failed to create connection to event channel!\n";
            o << "Impossible to create a network connection to any of the event endpoints returned by server";

            TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
        }
    }

    //
    // Create and connect the REQ socket used to send message to the ZMQ main thread
    //

    zmq::message_t reply;
    try
    {
        zmq::socket_t sender(zmq_context, ZMQ_REQ);

        //
        // In case this thread runs before the main ZMQ thread, it is possible to call connect before the main ZMQ
        // thread has binded its socket. In such a case, error code is set to ECONNREFUSED. If this happens, give the
        // main ZMQ thread a chance to run and retry the connect call I have tried with a yield call but it still failed
        // in some cases (when running the DS with a file as database  for instance). Replace the yield with a 10 mS
        // sleep !!!
        //

        try
        {
            sender.connect(CTRL_SOCK_ENDPOINT);
        }
        catch(zmq::error_t &e)
        {
            if(e.num() == ECONNREFUSED)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                sender.connect(CTRL_SOCK_ENDPOINT);
            }
            else
            {
                throw;
            }
        }

        //
        // Build message sent to ZMQ main thread
        // In this case, this is the command code, the publisher endpoint and the event name
        //

        char buffer[1024];
        int length = 0;

        buffer[length] = ZMQ_CONNECT_HEARTBEAT;
        length++;

        buffer[length] = 0;
        length++;

        ::strcpy(&(buffer[length]), ev_svr_data->svalue[valid_endpoint << 1].in());
        length = length + ::strlen(ev_svr_data->svalue[valid_endpoint << 1].in()) + 1;

        std::string sub(event_channel_name);
        sub = sub + '.' + HEARTBEAT_EVENT_NAME;

        ::strcpy(&(buffer[length]), sub.c_str());
        length = length + sub.size() + 1;

        //
        // Send command to main ZMQ thread
        //

        zmq::message_t send_data(length);
        ::memcpy(send_data.data(), buffer, length);
        auto result = sender.send(send_data, zmq::send_flags::none);
        TANGO_ASSERT(result);

        result = sender.recv(reply);
        TANGO_ASSERT(result);
    }
    catch(zmq::error_t &e)
    {
        std::stringstream o;

        o << "Failed to create connection to event channel!\n";
        o << "Error while communicating with the ZMQ main thread\n";
        o << "ZMQ error code = " << e.num() << "\n";
        o << "ZMQ message: " << e.what() << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }

    //
    // Any error during ZMQ main thread socket operations?
    //

    if(reply.size() != 2)
    {
        char err_mess[512];
        ::memcpy(err_mess, reply.data(), reply.size());
        err_mess[reply.size()] = '\0';

        std::stringstream o;

        o << "Failed to create connection to event channel!\n";
        o << "Error while trying to connect or subscribe the heartbeat ZMQ socket to the new publisher\n";
        o << "ZMQ message: " << err_mess << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }

    //
    // Init (or create) EventChannelStruct
    //

    if(reconnect)
    {
        EventChannelStruct &evt_ch = event_channel_info->second;
        evt_ch.channel_monitor->set_name(event_channel_name.c_str());
        evt_ch.last_heartbeat = Tango::get_current_system_datetime();
        evt_ch.heartbeat_skipped = false;
        evt_ch.event_system_failed = false;
        evt_ch.endpoint = ev_svr_data->svalue[valid_endpoint << 1].in();
        evt_ch.valid_endpoint = valid_endpoint;

        // We may need to update key in channel_map entry but to avoid iterator
        // invalidation we will do this later in EventConsumerKeepAliveThread's
        // main loop.
    }
    else
    {
        EventChannelStruct new_event_channel_struct;

        new_event_channel_struct.last_heartbeat = Tango::get_current_system_datetime();
        new_event_channel_struct.heartbeat_skipped = false;
        new_event_channel_struct.adm_device_proxy = nullptr;
        // create a channel monitor
        new_event_channel_struct.channel_monitor = std::make_shared<TangoMonitor>(event_channel_name.c_str());
        // set the timeout for the channel monitor to 1000ms not to block the event consumer for to long.
        new_event_channel_struct.channel_monitor->timeout(1000);

        new_event_channel_struct.event_system_failed = false;
        set_channel_type(new_event_channel_struct);
        new_event_channel_struct.endpoint = ev_svr_data->svalue[valid_endpoint << 1].in();
        new_event_channel_struct.valid_endpoint = valid_endpoint;

        channel_map[channel_name] = new_event_channel_struct;
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::disconnect_event_channel()
//
// description :
//        Disconnect to the event channel. This means that the process should not receive the heartbeat event for this
//        channel. It will be filtered out by ZMQ
//
// argument :
//        in :
//            - channel name : The event channel name (DS admin name)
//            - endpoint : The ZMQ endpoint for the heartbeat publisher socket
//            - endpoint_event : The ZMQ endpoint for the event publisher socket
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::disconnect_event_channel(const std::string &channel_name,
                                                const std::string &endpoint,
                                                const std::string &endpoint_event)
{
    std::string unsub(channel_name);
    unsub = unsub + '.' + HEARTBEAT_EVENT_NAME;

    //
    // Create and connect the REQ socket used to send message to the ZMQ main thread
    //

    zmq::message_t reply;

    try
    {
        zmq::socket_t sender(zmq_context, ZMQ_REQ);
        sender.connect(CTRL_SOCK_ENDPOINT);

        //
        // Build message sent to ZMQ main thread
        // In this case, this is the command code, the publisher endpoint and the event name
        //

        char buffer[1024];
        int length = 0;

        buffer[length] = ZMQ_DISCONNECT_HEARTBEAT;
        length++;

        ::strcpy(&(buffer[length]), unsub.c_str());
        length = length + unsub.size() + 1;

        ::strcpy(&(buffer[length]), endpoint.c_str());
        length = length + endpoint.size() + 1;

        ::strcpy(&(buffer[length]), endpoint_event.c_str());
        length = length + endpoint_event.size() + 1;

        //
        // Send command to main ZMQ thread
        //

        zmq::message_t send_data(length);
        ::memcpy(send_data.data(), buffer, length);
        auto result = sender.send(send_data, zmq::send_flags::none);
        TANGO_ASSERT(result);

        result = sender.recv(reply);
        TANGO_ASSERT(result);
    }
    catch(zmq::error_t &e)
    {
        TangoSys_OMemStream o;

        o << "Failed to disconnect from the event channel!\n";
        o << "Error while communicating with the ZMQ main thread\n";
        o << "ZMQ message: " << e.what() << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }

    //
    // In case of error returned by the main ZMQ thread
    //

    if(reply.size() != 2)
    {
        char err_mess[512];
        ::memcpy(err_mess, reply.data(), reply.size());
        err_mess[reply.size()] = '\0';

        TangoSys_OMemStream o;

        o << "Failed to disconnect from event channel!\n";
        o << "Error while trying to unsubscribe the heartbeat ZMQ socket from the channel heartbeat publisher\n";
        o << "ZMQ message: " << err_mess << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::disconnect_event()
//
// description :
//        Disconnect to the event. This means that the process should not receive the event any more
//        It will be filtered out by ZMQ
//
// argument :
//        in :
//            - event_name : The event name
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::disconnect_event(const std::string &event_name, const std::string &endpoint)
{
    //
    // Create and connect the REQ socket used to send message to the ZMQ main thread
    //

    zmq::message_t reply;

    try
    {
        zmq::socket_t sender(zmq_context, ZMQ_REQ);
        sender.connect(CTRL_SOCK_ENDPOINT);

        //
        // Build message sent to ZMQ main thread
        // In this case, this is the command code, the publisher endpoint and the event name
        //

        char buffer[1024];
        int length = 0;

        buffer[length] = ZMQ_DISCONNECT_EVENT;
        length++;

        ::strcpy(&(buffer[length]), event_name.c_str());
        length = length + event_name.size() + 1;

        ::strcpy(&(buffer[length]), endpoint.c_str());
        length = length + endpoint.size() + 1;

        //
        // Send command to main ZMQ thread
        //

        zmq::message_t send_data(length);
        ::memcpy(send_data.data(), buffer, length);
        auto result = sender.send(send_data, zmq::send_flags::none);
        TANGO_ASSERT(result);

        result = sender.recv(reply);
        TANGO_ASSERT(result);
    }
    catch(zmq::error_t &e)
    {
        TangoSys_OMemStream o;

        o << "Failed to disconnect from event!\n";
        o << "Error while communicating with the ZMQ main thread\n";
        o << "ZMQ message: " << e.what() << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }

    //
    // In case of error returned by the main ZMQ thread
    //

    if(reply.size() != 2)
    {
        char err_mess[512];
        ::memcpy(err_mess, reply.data(), reply.size());
        err_mess[reply.size()] = '\0';

        TangoSys_OMemStream o;

        o << "Failed to disconnect from event!\n";
        o << "Error while trying to unsubscribe the heartbeat ZMQ socket from the channel heartbeat publisher\n";
        o << "ZMQ message: " << err_mess << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::connect_event_system()
//
// description :
//        Connect to the real event (change, archive,...)
//
// argument :
//        in :
//            - device_name : The device fqdn (lower case)
//            - obj_name : The attribute/pipe name
//            - event_name : The event name
//            - filters : The event filters given by the user
//            - evt_it : Iterator pointing to the event channel entry in channel_map map
//            - new_event_callback : Structure used for the event callback entry in the event_callback_map
//            - dd : The data returned by the DS admin device xxxSubscriptionChange command
//          - valid_end : The valid endpoint in the list of endpoint returned by ZMQEventSubscriptionChange command
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::connect_event_system(TANGO_UNUSED(const std::string &device_name),
                                            TANGO_UNUSED(const std::string &obj_name),
                                            TANGO_UNUSED(const std::string &event_name),
                                            TANGO_UNUSED(const std::vector<std::string> &filters),
                                            TANGO_UNUSED(const EvChanIte &eve_it),
                                            EventCallBackStruct &new_event_callback,
                                            DeviceData &dd,
                                            size_t valid_end)
{
    //
    // Extract server command result
    //

    const DevVarLongStringArray *ev_svr_data;
    dd >> ev_svr_data;

    //
    // Create and connect the REQ socket used to send message to the ZMQ main thread
    //

    zmq::message_t reply;
    try
    {
        zmq::socket_t sender(zmq_context, ZMQ_REQ);
        sender.connect(CTRL_SOCK_ENDPOINT);

        //
        // If the transport is multicast, add main IP interface address in endpoint
        //

        bool mcast_transport = false;
        ApiUtil *au = ApiUtil::instance();

        std::string endpoint(ev_svr_data->svalue[(valid_end << 1) + 1].in());
        if(endpoint.find(MCAST_PROT) != std::string::npos)
        {
            mcast_transport = true;

            std::vector<std::string> adrs;
            au->get_ip_from_if(adrs);

            for(unsigned int i = 0; i < adrs.size(); ++i)
            {
                if(adrs[i].find("127.") == 0)
                {
                    continue;
                }
                adrs[i] = adrs[i] + ';';
                std::string::size_type pos = endpoint.find('/');
                pos = pos + 2;
                endpoint.insert(pos, adrs[i]);
                break;
            }
        }

        //
        // Build message sent to ZMQ main thread
        // In this case, this is the command code, the publisher endpoint, the event name and the sub hwm
        //

        char buffer[1024];
        int length = 0;

        if(mcast_transport)
        {
            buffer[length] = ZMQ_CONNECT_MCAST_EVENT;
        }
        else
        {
            buffer[length] = ZMQ_CONNECT_EVENT;
        }
        length++;

        buffer[length] = 0;
        length++;

        ::strcpy(&(buffer[length]), endpoint.c_str());
        length = length + endpoint.size() + 1;

        ::strcpy(&(buffer[length]), new_event_callback.received_from_admin.event_name.c_str());
        length = length + new_event_callback.received_from_admin.event_name.size() + 1;

        DevLong user_hwm = au->get_user_sub_hwm();
        if(user_hwm != -1)
        {
            ::memcpy(&(buffer[length]), &(user_hwm), sizeof(Tango::DevLong));
        }
        else
        {
            ::memcpy(&(buffer[length]), &(ev_svr_data->lvalue[2]), sizeof(Tango::DevLong));
        }
        length = length + sizeof(Tango::DevLong);

        //
        // In case of multicasting, add rate and ivl parameters
        //

        if(mcast_transport)
        {
            ::memcpy(&(buffer[length]), &(ev_svr_data->lvalue[3]), sizeof(Tango::DevLong));
            length = length + sizeof(Tango::DevLong);

            ::memcpy(&(buffer[length]), &(ev_svr_data->lvalue[4]), sizeof(Tango::DevLong));
            length = length + sizeof(Tango::DevLong);
        }

        //
        // Send command to main ZMQ thread
        //

        zmq::message_t send_data(length);
        ::memcpy(send_data.data(), buffer, length);
        auto result = sender.send(send_data, zmq::send_flags::none);
        TANGO_ASSERT(result);

        result = sender.recv(reply);
        TANGO_ASSERT(result);
    }
    catch(zmq::error_t &e)
    {
        std::stringstream o;

        o << "Failed to create connection to event!\n";
        o << "Error while communicating with the ZMQ main thread\n";
        o << "ZMQ message: " << e.what() << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }

    //
    // Any error during ZMQ main thread socket operations?
    //

    if(reply.size() != 2)
    {
        char err_mess[512];
        ::memcpy(err_mess, reply.data(), reply.size());
        err_mess[reply.size()] = '\0';

        std::stringstream o;

        o << "Failed to create connection to event!\n";
        o << "Error while trying to connect or subscribe the event ZMQ socket to the new publisher\n";
        o << "ZMQ message: " << err_mess << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::push_heartbeat_event()
//
// description :
//        Method called when the heartbeat event is received. This method retrieve the channel entry in the channel_map
//        and update the last heartbeat date.
//
// argument :
//        in :
//            - ev_name : The fully qualifed event name
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::push_heartbeat_event(std::string &ev_name)
{
    //
    // Remove ".heartbeat" at the end of event name
    //

    std::string::size_type pos = ev_name.find(".heartbeat");
    if(pos == std::string::npos)
    {
        return;
    }
    ev_name.erase(pos);

    //
    // Only reading from the maps
    //

    map_modification_lock.readerIn();

    std::map<std::string, EventChannelStruct>::iterator ipos;
    ipos = channel_map.find(ev_name);

    if(ipos != channel_map.end())
    {
        EventChannelStruct &evt_ch = ipos->second;
        try
        {
            AutoTangoMonitor _mon(evt_ch.channel_monitor);
            evt_ch.last_heartbeat = Tango::get_current_system_datetime();
        }
        catch(...)
        {
            std::string st("Tango::ZmqEventConsumer::push_heartbeat_event() timeout on channel monitor of ");
            st = st + ipos->first;
            print_error_message(st.c_str());
        }
    }
    else
    {
        unsigned int loop = 0;
        if(env_var_fqdn_prefix.size() > 1)
        {
            size_t pos = ev_name.find('/', 8);
            std::string base_tango_host = ev_name.substr(0, pos + 1);
            std::string canon_ev_name = ev_name.substr(pos + 1);
            for(loop = 0; loop < env_var_fqdn_prefix.size(); loop++)
            {
                if(env_var_fqdn_prefix[loop] == base_tango_host)
                {
                    continue;
                }
                else
                {
                    std::string new_tango_host = env_var_fqdn_prefix[loop] + canon_ev_name;
                    ipos = channel_map.find(new_tango_host);

                    if(ipos != channel_map.end())
                    {
                        EventChannelStruct &evt_ch = ipos->second;
                        try
                        {
                            AutoTangoMonitor _mon(evt_ch.channel_monitor);
                            evt_ch.last_heartbeat = Tango::get_current_system_datetime();
                        }
                        catch(...)
                        {
                            std::string st(
                                "Tango::ZmqEventConsumer::push_heartbeat_event() timeout on channel monitor of ");
                            st = st + ipos->first;
                            print_error_message(st.c_str());
                        }
                        break;
                    }
                }
            }
        }
        if(loop == env_var_fqdn_prefix.size())
        {
            std::string st("No entry in channel map for heartbeat ");
            st = st + ev_name;
            print_error_message(st.c_str());
        }
    }

    map_modification_lock.readerOut();
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::push_zmq_event()
//
// description :
//        Method called when the event is received. This method retrieve the channel entry in the channel_map
//        and update the last heartbeat date.
//
// argument :
//        in :
//            - ev_name : The fully qualifed event name
//            - endian : The sender host endianess
//            - event_data : The event data still in a ZMQ message
//            - error : Flag set to true if the event data is an error stack
//            - ctr : Event counter as received from server
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::push_zmq_event(
    std::string &ev_name, unsigned char endian, zmq::message_t &event_data, bool error, const DevULong &ds_ctr)
{
    map_modification_lock.readerIn();
    bool map_lock = true;
    //    TANGO_LOG << "Lib: Received event for " << ev_name << std::endl;

    // debug info
    //    for (const auto &elem : event_callback_map)
    //    {
    //        printf("Key in event_callback_map = %s\n", elem.first.c_str());
    //    }
    //    for (const auto &elem : channel_map)
    //    {
    //        printf("Key in channel_map = %s\n", elem.first.c_str());
    //    }

    //    TANGO_LOG << "ds_ctr" << ds_ctr << std::endl;
    //
    // Search for entry within the event_callback map using the event name received in the event
    //

    std::map<std::string, EventCallBackStruct>::iterator ipos;
    size_t loop;
    bool no_db_dev = false;

    size_t pos = ev_name.find('/', 8);
    std::string canon_ev_name = ev_name.substr(pos + 1);

    if(ev_name.find(MODIFIER_DBASE_NO) != std::string::npos)
    {
        no_db_dev = true;
    }

    for(loop = 0; loop < env_var_fqdn_prefix.size() + 1; loop++)
    {
        //
        // Test different fully qualified event name depending on different TANGO_HOST defined for the control system
        //

        std::string new_tango_host;

        if(loop == 0 || no_db_dev)
        {
            new_tango_host = ev_name;
        }
        else
        {
            new_tango_host = env_var_fqdn_prefix[loop - 1] + canon_ev_name;
        }

        ipos = event_callback_map.find(new_tango_host);

        if(ipos != event_callback_map.end())
        {
            const AttributeValue *attr_value = nullptr;
            const AttributeValue_3 *attr_value_3 = nullptr;
            const ZmqAttributeValue_4 *z_attr_value_4 = nullptr;
            const ZmqAttributeValue_5 *z_attr_value_5 = nullptr;
            const AttributeConfig_2 *attr_conf_2 = nullptr;
            const AttributeConfig_3 *attr_conf_3 = nullptr;
            const AttributeConfig_5 *attr_conf_5 = nullptr;
            AttDataReady *att_ready = nullptr;
            DevIntrChange *dev_intr_change = nullptr;
            const DevErrorList *err_ptr;
            DevErrorList errors;
            AttributeInfoEx *attr_info_ex = nullptr;

            bool ev_attr_conf = false;
            bool ev_attr_ready = false;
            bool ev_dev_intr = false;
            bool pipe_event = false;

            EventCallBackStruct &evt_cb = ipos->second;
            //            TANGO_LOG << "evt_cb.ctr" << evt_cb.ctr << std::endl;

            //
            // Miss some events?
            // Due to LIBZMQ Bug 283, the first event after a process startup is sent two times
            // with the same ctr value. Do not call the user callback for the second times.
            //

            bool err_missed_event = false;
            if(ds_ctr > 1 && evt_cb.ctr == 0)
            {
                evt_cb.ctr = ds_ctr - 1;
            }

            // This can be negative after reconnection if server was restarted.
            DevLong missed_event = (ds_ctr >= evt_cb.ctr) ? static_cast<DevLong>(ds_ctr - evt_cb.ctr)
                                                          : -static_cast<DevLong>(evt_cb.ctr - ds_ctr);

            if(missed_event >= 2)
            {
                err_missed_event = true;
                evt_cb.discarded_event = false;
            }
            else if(missed_event == 0)
            {
                if(!evt_cb.discarded_event)
                {
                    evt_cb.discarded_event = true;
                    evt_cb.discarded_event_count++;
                    if(g_current_perf_mon_sample != nullptr)
                    {
                        g_current_perf_mon_sample->discarded = true;
                    }
                    map_modification_lock.readerOut();
                    return;
                }
                else
                {
                    evt_cb.discarded_event = false;
                }
            }
            else
            {
                evt_cb.discarded_event = false;
            }

            evt_cb.ctr = ds_ctr;
            evt_cb.event_count++;

            if(err_missed_event)
            {
                evt_cb.missed_event_count++;
            }

            //
            // Get which type of event data has been received (from the event type)
            //
            auto event_name = detail::get_event_name(ev_name);

            //
            // If the client TANGO_HOST is one alias, replace in the event name the host name by the alias
            //

            std::string full_att_name = evt_cb.get_client_attribute_name();
            pos = full_att_name.rfind('/');
            std::string att_name = full_att_name.substr(pos + 1);

            UserDataEventType data_type;

            if(event_name.find(CONF_TYPE_EVENT) != std::string::npos)
            {
                data_type = ATT_CONF;
            }
            else if(event_name == DATA_READY_TYPE_EVENT)
            {
                data_type = ATT_READY;
            }
            else if(event_name == EventName[INTERFACE_CHANGE_EVENT])
            {
                data_type = DEV_INTR;
            }
            else if(event_name == EventName[PIPE_EVENT])
            {
                data_type = PIPE;
            }
            else
            {
                data_type = ATT_VALUE;
            }

            if(g_current_perf_mon_sample != nullptr)
            {
                strncpy(g_current_perf_mon_sample->attr_name, att_name.c_str(), PerfMonSample::k_attr_name_size);
                g_current_perf_mon_sample->attr_name[PerfMonSample::k_attr_name_size] = '\0';
            }

            //
            // Unmarshal the event data
            //

            long vers = 0;
            DeviceAttribute *dev_attr = nullptr;
            DevicePipe *dev_pipe = nullptr;
            bool no_unmarshalling = false;

            if(evt_cb.fwd_att && data_type != ATT_CONF && !error)
            {
                no_unmarshalling = true;
            }
            else
            {
                //
                // For 64 bits data (double, long64 and ulong64), omniORB unmarshalling
                // methods required that the 64 bits data are aligned on a 8 bytes memory address.
                // ZMQ returned memory which is sometimes aligned on a 8 bytes boundary but
                // not always (seems to depend on the host architecture)
                // The attribute data transfert starts with the union discriminator
                // (4 bytes), the elt nb (4 bytes) and the element themselves.
                // This means 8 bytes before the real data.
                // There is a trick here.
                // The buffer is always transferred with an extra 4 bytes added at the beginning
                // If the alignememnt is not correct (buffer aligned on a 8 bytes boundary
                // and 64 bits data type), shift the whole buffer by 4 bytes erasing the
                // additional 4 bytes sent.
                //
                // Note: The buffer is not correctly aligned if it is returned on a
                // 8 bytes boundary because we have the 4 extra bytes + 8 bytes for
                // union discriminator + elt nb. This means 64 bits data not on a
                // 8 bytes boundary
                //

                char *data_ptr = (char *) event_data.data();
                size_t data_size = event_data.size();

                bool shift_zmq420 = false;
                int shift_mem = reinterpret_cast<std::uintptr_t>(data_ptr) & 0x3;
                if(shift_mem != 0)
                {
                    char *src = data_ptr + 4;

                    size_t size_to_move = data_size - 4;
                    if(data_type == PIPE)
                    {
                        src = src + 4;
                        size_to_move = size_to_move - 4;
                    }

                    char *dest = src - shift_mem;
                    if((reinterpret_cast<std::uintptr_t>(dest) & 0x7) == 4)
                    {
                        dest = dest - 4;
                    }
                    memmove((void *) dest, (void *) src, size_to_move);
                    shift_zmq420 = true;

                    data_ptr = dest;
                }

                bool data64 = false;
                if(data_type == PIPE)
                {
                    data64 = true;
                }
                else if(data_type == ATT_VALUE && !error)
                {
                    int disc = shift_zmq420 ? ((int *) data_ptr)[0] : ((int *) data_ptr)[1];
                    if(endian == 0)
                    {
                        char first_byte = disc & 0xFF;
                        char second_byte = (disc & 0xFF00) >> 8;
                        char third_byte = (disc & 0xFF0000) >> 16;
                        char forth_byte = (disc & 0xFF000000) >> 24;
                        disc = 0;
                        disc = forth_byte + (third_byte << 8) + (second_byte << 16) + (first_byte << 24);
                    }
                    if(disc == ATT_DOUBLE || disc == ATT_LONG64 || disc == ATT_ULONG64)
                    {
                        data64 = true;
                    }
                }

                bool buffer_aligned64 = false;
                if(data64)
                {
                    if((reinterpret_cast<std::uintptr_t>(data_ptr) & 0x7) == 0)
                    {
                        buffer_aligned64 = true;
                    }
                }

                //
                // Shift buffer if required
                //

                if(data_type == PIPE && data64 && !buffer_aligned64)
                {
                    if(omniORB::trace(30) != 0)
                    {
                        omniORB::logger log;
                        log << "ZMQ: Pipe event -> Shifting received buffer to be aligned on a 8 bytes boundary"
                            << '\n';
                    }
                    char *src = data_ptr + 8;
                    char *dest = data_ptr + 4;
                    memmove((void *) dest, (void *) src, data_size - 8);

                    data_ptr = data_ptr + 4;
                    data_size = data_size - 4;
                }
                else if(data_type != PIPE && data64 && buffer_aligned64 && !shift_zmq420)
                {
                    if(omniORB::trace(30) != 0)
                    {
                        omniORB::logger log;
                        log << "ZMQ: Classical event -> Shifting received buffer to be aligned on a 8 bytes boundary"
                            << '\n';
                    }
                    char *src = data_ptr + 4;
                    char *dest = data_ptr;
                    memmove((void *) dest, (void *) src, data_size - 4);

                    data_size = data_size - 4;
                }
                else
                {
                    if(data_type == PIPE)
                    {
                        if(!shift_zmq420)
                        {
                            data_ptr = data_ptr + (sizeof(CORBA::Long) << 1);
                        }
                        data_size = data_size - (sizeof(CORBA::Long) << 1);
                    }
                    else
                    {
                        if(!shift_zmq420)
                        {
                            data_ptr = data_ptr + sizeof(CORBA::Long);
                        }
                        data_size = data_size - sizeof(CORBA::Long);
                    }
                }

                TangoCdrMemoryStream event_data_cdr(data_ptr, data_size);
                event_data_cdr.setByteSwapFlag(endian != 0u);

                //
                // Unmarshall the data
                //

                if(error)
                {
                    switch(data_type)
                    {
                    case ATT_CONF:
                        ev_attr_conf = true;
                        break;

                    case ATT_READY:
                        ev_attr_ready = true;
                        break;

                    case DEV_INTR:
                        ev_dev_intr = true;
                        break;

                    case PIPE:
                        pipe_event = true;
                        break;

                    case ATT_VALUE:
                        // do nothing
                        break;

                    default:
                        TANGO_ASSERT_ON_DEFAULT(data_type);
                    }

                    try
                    {
                        (DevErrorList &) del <<= event_data_cdr;
                        err_ptr = &del.in();
                        errors = *err_ptr;
                    }
                    catch(...)
                    {
                        fill_deverror_for_malformed_event_data(ev_name, errors);
                    }
                }
                else
                {
                    switch(data_type)
                    {
                    case ATT_CONF:
                        if(evt_cb.device_idl > 4)
                        {
                            //
                            // Event if the device sending the event is IDL 5/6
                            //

                            try
                            {
                                ev_attr_conf = true;
                                (AttributeConfig_5 &) ac5 <<= event_data_cdr;
                                attr_conf_5 = &ac5.in();
                                vers = evt_cb.device_idl;
                                attr_info_ex = new AttributeInfoEx();
                                *attr_info_ex = const_cast<AttributeConfig_5 *>(attr_conf_5);
                            }
                            catch(...)
                            {
                                fill_deverror_for_malformed_event_data(ev_name, errors);
                            }
                        }
                        else if(evt_cb.device_idl > 2)
                        {
                            try
                            {
                                ev_attr_conf = true;
                                (AttributeConfig_3 &) ac3 <<= event_data_cdr;
                                attr_conf_3 = &ac3.in();
                                vers = 3;
                                attr_info_ex = new AttributeInfoEx();
                                *attr_info_ex = const_cast<AttributeConfig_3 *>(attr_conf_3);
                            }
                            catch(...)
                            {
                                fill_deverror_for_malformed_event_data(ev_name, errors);
                            }
                        }
                        else if(evt_cb.device_idl == 2)
                        {
                            ev_attr_conf = true;
                            (AttributeConfig_2 &) ac2 <<= event_data_cdr;
                            attr_conf_2 = &ac2.in();
                            vers = 2;
                            attr_info_ex = new AttributeInfoEx();
                            *attr_info_ex = const_cast<AttributeConfig_2 *>(attr_conf_2);
                        }
                        break;

                    case ATT_READY:
                        try
                        {
                            ev_attr_ready = true;
                            (AttDataReady &) adr <<= event_data_cdr;
                            att_ready = &adr.inout();
                            att_ready->name = full_att_name.c_str();
                        }
                        catch(...)
                        {
                            fill_deverror_for_malformed_event_data(ev_name, errors);
                        }
                        break;

                    case DEV_INTR:
                        try
                        {
                            ev_dev_intr = true;
                            (DevIntrChange &) dic <<= event_data_cdr;
                            dev_intr_change = &dic.inout();
                        }
                        catch(...)
                        {
                            fill_deverror_for_malformed_event_data(ev_name, errors);
                        }
                        break;

                    case ATT_VALUE:
                        if(evt_cb.device_idl >= 5)
                        {
                            event_data_cdr.set_un_marshal_type(TangoCdrMemoryStream::UN_ATT);
                            try
                            {
                                vers = evt_cb.device_idl;
                                zav5.operator<<=(event_data_cdr);
                                z_attr_value_5 = &zav5;
                                dev_attr = new(DeviceAttribute);
                                attr_to_device(z_attr_value_5, dev_attr);

                                //
                                // Update name in DeviceAttribute in case it is not coherent with name received in first
                                // ZMQ message part. This happens in case of forwarded attribute but also in case of DS
                                // started with file as database
                                //

                                std::string::size_type pos = att_name.find(MODIFIER_DBASE_NO);
                                std::string a_name;
                                if(pos != std::string::npos)
                                {
                                    a_name = att_name.substr(0, pos);
                                }
                                else
                                {
                                    a_name = att_name;
                                }
                                if(a_name != dev_attr->get_name())
                                {
                                    dev_attr->set_name(a_name);
                                }
                            }
                            catch(...)
                            {
                                fill_deverror_for_malformed_event_data(ev_name, errors);
                            }
                        }
                        else if(evt_cb.device_idl == 4)
                        {
                            event_data_cdr.set_un_marshal_type(TangoCdrMemoryStream::UN_ATT);
                            try
                            {
                                vers = 4;
                                zav4.operator<<=(event_data_cdr);
                                z_attr_value_4 = &zav4;
                                dev_attr = new(DeviceAttribute);
                                attr_to_device(z_attr_value_4, dev_attr);

                                //
                                // Update name in DeviceAttribute in case it is not coherent with name received in first
                                // ZMQ message part. This happens in case of forwarded attribute but also in case of DS
                                // started with file as database
                                //

                                std::string::size_type pos = att_name.find(MODIFIER_DBASE_NO);
                                std::string a_name;
                                if(pos != std::string::npos)
                                {
                                    a_name = att_name.substr(0, pos);
                                }
                                else
                                {
                                    a_name = att_name;
                                }
                                if(a_name != dev_attr->get_name())
                                {
                                    dev_attr->set_name(a_name);
                                }
                            }
                            catch(...)
                            {
                                fill_deverror_for_malformed_event_data(ev_name, errors);
                            }
                        }
                        else if(evt_cb.device_idl == 3)
                        {
                            event_data_cdr.set_un_marshal_type(TangoCdrMemoryStream::UN_ATT);
                            try
                            {
                                vers = 3;
                                (AttributeValue_3 &) av3 <<= event_data_cdr;
                                attr_value_3 = &av3.in();
                                dev_attr = new(DeviceAttribute);
                                attr_to_device(attr_value, attr_value_3, vers, dev_attr);
                            }
                            catch(...)
                            {
                                fill_deverror_for_malformed_event_data(
                                    " (AttributeValue_3 -> Device_3Impl....) " + ev_name, errors);
                            }
                        }
                        else if(evt_cb.device_idl < 3)
                        {
                            try
                            {
                                vers = 2;
                                (AttributeValue &) av <<= event_data_cdr;
                                attr_value = &av.in();
                                dev_attr = new(DeviceAttribute);
                                attr_to_device(attr_value, attr_value_3, vers, dev_attr);
                            }
                            catch(...)
                            {
                                fill_deverror_for_malformed_event_data(
                                    " (AttributeValue -> Device_2Impl....) " + ev_name, errors);
                            }
                        }
                        break;

                    case PIPE:
                        event_data_cdr.set_un_marshal_type(TangoCdrMemoryStream::UN_PIPE);
                        try
                        {
                            pipe_event = true;
                            zdpd.operator<<=(event_data_cdr);

                            std::string pipe_name = zdpd.name.in();
                            std::string root_blob_name = zdpd.data_blob.name.in();

                            dev_pipe = new DevicePipe(pipe_name, root_blob_name);
                            dev_pipe->set_time(zdpd.time);

                            CORBA::ULong max, len;
                            max = zdpd.data_blob.blob_data.maximum();
                            len = zdpd.data_blob.blob_data.length();
                            DevPipeDataElt *buf = zdpd.data_blob.blob_data.get_buffer((CORBA::Boolean) true);
                            auto *dvpdea = new DevVarPipeDataEltArray(max, len, buf, true);

                            dev_pipe->get_root_blob().set_extract_data(dvpdea);
                            dev_pipe->get_root_blob().set_extract_delete(true);
                        }
                        catch(...)
                        {
                            fill_deverror_for_malformed_event_data(ev_name, errors);
                        }
                        break;
                    }
                }
            }

            FwdEventData *missed_event_data = nullptr;
            FwdAttrConfEventData *missed_conf_event_data = nullptr;
            DataReadyEventData *missed_ready_event_data = nullptr;
            DevIntrChangeEventData *missed_dev_intr_event_data = nullptr;
            PipeEventData *missed_dev_pipe_data = nullptr;

            try
            {
                AutoTangoMonitor _mon(evt_cb.callback_monitor);

                //
                // In case we have missed some event, prepare structure to send to callback to inform user of this bad
                // behavior
                //

                if(err_missed_event)
                {
                    DevErrorList missed_errors;
                    missed_errors.length(1);
                    missed_errors[0].reason = Tango::string_dup(API_MissedEvents);
                    missed_errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
                    missed_errors[0].desc = "Missed some events! Zmq queue has reached HWM?";
                    missed_errors[0].severity = ERR;

                    // We prepare event data structures in this case beforehand.
                    // Later when we pass this data to user callbacks, we must
                    // set device proxy to the one corresponding to each callback.
                    DeviceProxy *const device = nullptr;

                    if((!ev_attr_conf) && (!ev_attr_ready) && (!ev_dev_intr) && (!pipe_event))
                    {
                        missed_event_data = new FwdEventData(device, full_att_name, event_name, nullptr, missed_errors);
                    }
                    else if(!ev_attr_ready && !ev_dev_intr && !pipe_event)
                    {
                        missed_conf_event_data =
                            new FwdAttrConfEventData(device, full_att_name, event_name, nullptr, missed_errors);
                    }
                    else if(!ev_dev_intr && !pipe_event)
                    {
                        missed_ready_event_data = new DataReadyEventData(device, nullptr, event_name, missed_errors);
                    }
                    else if(!ev_dev_intr)
                    {
                        missed_dev_pipe_data =
                            new PipeEventData(device, full_att_name, event_name, nullptr, missed_errors);
                    }
                    else
                    {
                        missed_dev_intr_event_data = new DevIntrChangeEventData(device,
                                                                                event_name,
                                                                                full_att_name,
                                                                                (CommandInfoList *) nullptr,
                                                                                (AttributeInfoListEx *) nullptr,
                                                                                false,
                                                                                missed_errors);
                    }
                }

                //
                // Fire the user callback
                //

                std::vector<EventSubscribeStruct>::iterator esspos;

                unsigned int cb_nb = ipos->second.callback_list.size();
                unsigned int cb_ctr = 0;

                bool first_callback = true;
                if(g_current_perf_mon_sample != nullptr)
                {
                    g_current_perf_mon_sample->callback_count = evt_cb.callback_list.size();
                }
                for(esspos = evt_cb.callback_list.begin(); esspos != evt_cb.callback_list.end(); ++esspos)
                {
                    if(missed_event_data != nullptr)
                    {
                        missed_event_data->device = esspos->device;
                    }
                    if(missed_conf_event_data != nullptr)
                    {
                        missed_conf_event_data->device = esspos->device;
                    }
                    if(missed_ready_event_data != nullptr)
                    {
                        missed_ready_event_data->device = esspos->device;
                    }
                    if(missed_dev_pipe_data != nullptr)
                    {
                        missed_dev_pipe_data->device = esspos->device;
                    }
                    if(missed_dev_intr_event_data != nullptr)
                    {
                        missed_dev_intr_event_data->device = esspos->device;
                    }

                    cb_ctr++;
                    if(esspos->id > 0)
                    {
                        CallBack *callback;
                        callback = esspos->callback;
                        EventQueue *ev_queue;
                        ev_queue = esspos->ev_queue;

                        if((!ev_attr_conf) && (!ev_attr_ready) && (!ev_dev_intr) && (!pipe_event))
                        {
                            FwdEventData *event_dat = newFwdEventData(event_data,
                                                                      esspos->device,
                                                                      errors,
                                                                      event_name,
                                                                      full_att_name,
                                                                      vers,
                                                                      dev_attr,
                                                                      no_unmarshalling,
                                                                      cb_nb,
                                                                      cb_ctr,
                                                                      callback);

                            if(g_current_perf_mon_sample != nullptr && first_callback &&
                               event_dat->attr_value != nullptr)
                            {
                                TimeVal reception_date = event_dat->reception_date;
                                TimeVal send_date = event_dat->attr_value->get_date();

                                g_current_perf_mon_sample->first_callback_latency_micros =
                                    (reception_date.tv_sec - send_date.tv_sec) * 1000000 +
                                    (reception_date.tv_usec - send_date.tv_usec);

                                first_callback = false;
                            }

                            safe_execute_callback_or_store_data(callback,
                                                                event_dat,
                                                                err_missed_event,
                                                                missed_event_data,
                                                                "Tango::ZmqEventConsumer::push_zmq_event()",
                                                                ipos->first,
                                                                ev_queue);

                            if(callback == nullptr && vers >= 4 && cb_ctr == cb_nb)
                            {
                                delete dev_attr;
                            }
                        }
                        else if(!ev_attr_ready && !ev_dev_intr && !pipe_event)
                        {
                            FwdAttrConfEventData *event_data_;

                            if(cb_ctr != cb_nb)
                            {
                                AttributeInfoEx *attr_info_copy = new AttributeInfoEx();
                                *attr_info_copy = *attr_info_ex;
                                event_data_ = new FwdAttrConfEventData(
                                    esspos->device, full_att_name, event_name, attr_info_copy, errors);
                                if(attr_conf_5 != nullptr)
                                {
                                    event_data_->set_fwd_attr_conf(attr_conf_5);
                                }
                            }
                            else
                            {
                                event_data_ = new FwdAttrConfEventData(
                                    esspos->device, full_att_name, event_name, attr_info_ex, errors);
                                if(attr_conf_5 != nullptr)
                                {
                                    event_data_->set_fwd_attr_conf(attr_conf_5);
                                }
                            }

                            safe_execute_callback_or_store_data(callback,
                                                                event_data_,
                                                                err_missed_event,
                                                                missed_conf_event_data,
                                                                "Tango::ZmqEventConsumer::push_zmq_event()",
                                                                ipos->first,
                                                                ev_queue);
                        }
                        else if(!ev_attr_ready && !pipe_event)
                        {
                            auto *event_data_ = new DevIntrChangeEventData(esspos->device,
                                                                           event_name,
                                                                           full_att_name,
                                                                           &dev_intr_change->cmds,
                                                                           &dev_intr_change->atts,
                                                                           dev_intr_change->dev_started,
                                                                           errors);
                            safe_execute_callback_or_store_data(callback,
                                                                event_data_,
                                                                err_missed_event,
                                                                missed_dev_intr_event_data,
                                                                "Tango::ZmqEventConsumer::push_zmq_event()",
                                                                ipos->first,
                                                                ev_queue);
                        }
                        else if(!ev_attr_ready)
                        {
                            PipeEventData *event_data_;

                            if(cb_ctr != cb_nb)
                            {
                                DevicePipe *dev_pipe_copy = new DevicePipe();
                                *dev_pipe_copy = *dev_pipe;
                                event_data_ =
                                    new PipeEventData(esspos->device, full_att_name, event_name, dev_pipe_copy, errors);
                            }
                            else
                            {
                                event_data_ =
                                    new PipeEventData(esspos->device, full_att_name, event_name, dev_pipe, errors);
                            }

                            safe_execute_callback_or_store_data(callback,
                                                                event_data_,
                                                                err_missed_event,
                                                                missed_dev_pipe_data,
                                                                "Tango::ZmqEventConsumer::push_zmq_event()",
                                                                ipos->first,
                                                                ev_queue);
                        }
                        else
                        {
                            DataReadyEventData *event_data_ =
                                new DataReadyEventData(esspos->device, att_ready, event_name, errors);

                            safe_execute_callback_or_store_data(callback,
                                                                event_data_,
                                                                err_missed_event,
                                                                missed_ready_event_data,
                                                                "Tango::ZmqEventConsumer::push_zmq_event()",
                                                                ipos->first,
                                                                ev_queue);
                        }
                    }

                } // End of for

                map_lock = false;
                map_modification_lock.readerOut();

                delete missed_event_data;
                delete missed_conf_event_data;
                delete missed_ready_event_data;
                delete missed_dev_intr_event_data;
                delete missed_dev_pipe_data;

                break;
            }
            catch(DevFailed &e)
            {
                delete missed_event_data;
                delete missed_conf_event_data;
                delete missed_ready_event_data;
                delete missed_dev_intr_event_data;
                delete missed_dev_pipe_data;

                // free the map lock if not already done
                if(map_lock)
                {
                    map_modification_lock.readerOut();
                }

                std::string reason = e.errors[0].reason.in();
                if(reason == API_CommandTimedOut)
                {
                    std::string st("Tango::ZmqEventConsumer::push_zmq_event() timeout on callback monitor of ");
                    st = st + ipos->first;
                    print_error_message(st.c_str());
                }

                break;
            }
            catch(...)
            {
                delete missed_event_data;
                delete missed_conf_event_data;
                delete missed_ready_event_data;
                delete missed_dev_intr_event_data;

                // free the map lock if not already done
                if(map_lock)
                {
                    map_modification_lock.readerOut();
                }

                std::string st("Tango::ZmqEventConsumer::push_zmq_event(): - ");
                st = st + ipos->first;
                st = st + " - Unknown exception (Not a DevFailed) while calling Callback ";
                print_error_message(st.c_str());

                break;
            }
        }
    }

    //
    // In case of error
    //

    if(loop == env_var_fqdn_prefix.size() + 1)
    {
        std::string st("Event ");
        st = st + ev_name;
        st = st + " not found in event callback map !!!";
        print_error_message(st.c_str());
        // even if nothing was found in the map, free the lock
        map_modification_lock.readerOut();
    }
}

FwdEventData *ZmqEventConsumer::newFwdEventData(zmq::message_t &event_data,
                                                DeviceProxy *device,
                                                DevErrorList &errors,
                                                std::string &event_name,
                                                std::string &full_att_name,
                                                long vers,
                                                const DeviceAttribute *dev_attr,
                                                bool no_unmarshalling,
                                                unsigned int cb_nb,
                                                unsigned int cb_ctr,
                                                const CallBack *callback) const
{
    //
    // In case we have several callbacks on the same event or if the event has to be stored in a queue, copy
    // the event data (Event data are in the ZMQ message)
    //

    std::string actual_full_att_name;
    actual_full_att_name = full_att_name;

    if(cb_ctr != cb_nb)
    {
        DeviceAttribute *dev_attr_copy = nullptr;
        if(dev_attr != nullptr || (callback == nullptr && vers >= 4))
        {
            dev_attr_copy = new DeviceAttribute();
            if(!no_unmarshalling)
            {
                dev_attr_copy->deep_copy(*dev_attr);
            }
        }

        if(!no_unmarshalling)
        {
            return new FwdEventData(device, actual_full_att_name, event_name, dev_attr_copy, errors);
        }
        else
        {
            return new FwdEventData(device, actual_full_att_name, event_name, dev_attr_copy, errors, &event_data);
        }
    }
    else
    {
        if(no_unmarshalling)
        {
            DeviceAttribute *dummy = new DeviceAttribute();
            return new FwdEventData(device, actual_full_att_name, event_name, dummy, errors, &event_data);
        }
        else
        {
            if(callback == nullptr && vers >= 4)
            {
                DeviceAttribute *dev_attr_copy = nullptr;
                if(dev_attr != nullptr)
                {
                    dev_attr_copy = new DeviceAttribute();
                    dev_attr_copy->deep_copy(*dev_attr);
                }

                return new FwdEventData(device, actual_full_att_name, event_name, dev_attr_copy, errors);
            }
            else
            {
                return new FwdEventData(
                    device, actual_full_att_name, event_name, const_cast<DeviceAttribute *>(dev_attr), errors);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::zmq_specific()
//
// description :
//        Do some ZMQ specific tasks like checking release compatibility or lower case the admin device name
//        which is used in the heartbeat event name.
//
//        Even if the current Tango  version does not work with zmq 3.x we need to keep these checks as the
//        other tango version across the network might still be using an old zmq version.
//
// argument :
//        in :
//            - dd : The result of the event subscription command
//            - adm_name : The admin device name used in the heartbeat event
//            - device : The device proxy pointer (for error message)
//            - obj_name : The attribute/pipe name (for error message)
//            - ev_type : Event type
//            - ev_name : Event name
//        out:
//            - mod_ev_name : Boolean set to true if the event name is modified
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::zmq_specific(DeviceData &dd,
                                    std::string &adm_name,
                                    DeviceProxy *device,
                                    const std::string &obj_name)
{
    const DevVarLongStringArray *ev_svr_data;
    dd >> ev_svr_data;

    //
    // For event coming from server still using Tango 8.0.x, do not lowercase the adm_name
    //

    if(ev_svr_data->lvalue[0] >= 810)
    {
        std::transform(adm_name.begin(), adm_name.end(), adm_name.begin(), ::tolower);
    }

    //
    // If the event is configured to use multicast, check ZMQ release
    //

    std::string endpoint(ev_svr_data->svalue[1].in());
    int ds_zmq_release = 0;

    if(ev_svr_data->lvalue.length() >= 6)
    {
        ds_zmq_release = (ev_svr_data->lvalue[5]);
    }

    int zmq_major, zmq_minor, zmq_patch;
    zmq_version(&zmq_major, &zmq_minor, &zmq_patch);

    //
    // Check for ZMQ compatible release. Impossible to check if server does not send which ZMQ release it is using.
    //

    if(ds_zmq_release == 310)
    {
        if(zmq_major != 3 || zmq_minor != 1 || zmq_patch != 0)
        {
            TANGO_THROW_EXCEPTION(API_UnsupportedFeature,
                                  "Incompatibility between ZMQ releases between client and server!");
        }
    }

    if(zmq_major == 3 && zmq_minor == 1 && zmq_patch == 0)
    {
        if(ds_zmq_release != 0 && ds_zmq_release != 310)
        {
            TANGO_THROW_EXCEPTION(API_UnsupportedFeature,
                                  "Incompatibility between ZMQ releases between client and server!");
        }
    }

    //
    // Check if multicasting is available (requires zmq 3.2.x)
    //

    if(endpoint.find(MCAST_PROT) != std::string::npos)
    {
        if(zmq_major == 3 && zmq_minor < 2)
        {
            TangoSys_OMemStream o;
            o << "The process is using zmq release ";
            o << zmq_major << "." << zmq_minor << "." << zmq_patch;
            o << "\nThe event on attribute or pipe " << obj_name << " for device " << device->dev_name();
            o << " is configured to use multicasting";
            o << "\nMulticast event(s) not available with this ZMQ release" << std::ends;

            TANGO_THROW_EXCEPTION(API_UnsupportedFeature, o.str());
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::check_zmq_endpoint()
//
// description :
//        Check if the endpoint returned by the ZMQEventSubscriptionChange DS admin device command are valid on the
//      client side.
//
// argument :
//        in :
//            - endpoint : The returned endpoint (contain
//
// return :
//      A boolean set to true if it is possible to establish a connection with this endpoint. Otherwise, returns false
//
//--------------------------------------------------------------------------------------------------------------------

bool ZmqEventConsumer::check_zmq_endpoint(const std::string &endpoint)
{
    //
    // Isolate IP address in endpoint
    //

    std::string ip, port_str;
    detail::split_endpoint(endpoint, ip, port_str);
    int port = atoi(port_str.c_str());

    //
    // Open a socket
    //

    struct sockaddr_in address;
    int result, len;

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0)
    {
        return false;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);
    len = sizeof(address);

#ifdef _TG_WINDOWS_
    //
    // Put socket in non-blocking mode
    //

    u_long iMode = 1;
    ioctlsocket(sockfd, FIONBIO, &iMode);

    //
    // Try to connect
    //

    result = ::connect(sockfd, (struct sockaddr *) &address, len);

    if(result == SOCKET_ERROR)
    {
        int err_code = WSAGetLastError();
        if(err_code == WSAEWOULDBLOCK)
        {
            struct timeval tv;
            fd_set myset;
            int res;

            tv.tv_sec = 0;
            tv.tv_usec = 100000;

            FD_ZERO(&myset);
            FD_SET(sockfd, &myset);

            //
            // Because socket is in non-blocking mode, call select to get connection status
            //

            res = select(sockfd + 1, nullptr, &myset, nullptr, &tv);

            if(res == 0)
            {
                closesocket(sockfd);
                return false;
            }
            else if(res < 0)
            {
                closesocket(sockfd);
                return false;
            }
            else if(res > 0)
            {
                socklen_t lon = sizeof(int);
                int valopt = 0;

                if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) (&valopt), &lon) < 0)
                {
                    closesocket(sockfd);
                    return false;
                }

                if(valopt)
                {
                    closesocket(sockfd);
                    return false;
                }
            }
        }
        else
        {
            closesocket(sockfd);
            return false;
        }
    }

    //
    // Connection is a success, return true
    //

    closesocket(sockfd);
#else
    long arg;
    //
    // Put socket in non-blocking mode
    //

    if((arg = fcntl(sockfd, F_GETFL, nullptr)) < 0)
    {
        close(sockfd);
        return false;
    }

    arg |= O_NONBLOCK;
    if(fcntl(sockfd, F_SETFL, arg) < 0)
    {
        close(sockfd);
        return false;
    }

    //
    // Try to connect
    //

    result = ::connect(sockfd, (struct sockaddr *) &address, len);

    if(result < 0)
    {
        if(errno == EINPROGRESS)
        {
            struct timeval tv;
            fd_set myset;
            int res;

            tv.tv_sec = 0;
            tv.tv_usec = 100000;

            FD_ZERO(&myset);
            FD_SET(sockfd, &myset);

            //
            // Because socket is in non-blocking mode, call select to get connection status
            //

            res = select(sockfd + 1, nullptr, &myset, nullptr, &tv);

            if(res == 0)
            {
                close(sockfd);
                return false;
            }
            else if(res < 0 && errno != EINTR)
            {
                close(sockfd);
                return false;
            }
            else if(res > 0)
            {
                socklen_t lon = sizeof(int);
                int valopt = 0;

                if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *) (&valopt), &lon) < 0)
                {
                    close(sockfd);
                    return false;
                }

                if(valopt != 0)
                {
                    close(sockfd);
                    return false;
                }
            }
        }
        else
        {
            close(sockfd);
            return false;
        }
    }

    //
    // Connection is a success, return true
    //

    close(sockfd);
#endif
    return true;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventConsumer::get_subscribed_event_ids()
//
// description :
//        Get event id for the DeviceProxy given as parameter
//
// argument :
//        in :
//            - _dev : The DeviceProxy object
//      out :
//          - _ids : Vector of event id for the given DeviceProxy
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventConsumer::get_subscribed_event_ids(DeviceProxy *_dev, std::vector<int> &_ids)
{
    if(!_ids.empty())
    {
        _ids.clear();
    }

    //
    // Lock the maps only for reading
    //

    ReaderLock r(map_modification_lock);

    //
    // Search with the callback_list map
    //

    for(const auto &entry : event_callback_map)
    {
        for(const auto &subscription : entry.second.callback_list)
        {
            if(subscription.device == _dev)
            {
                _ids.push_back(subscription.id);
            }
        }
    }

    //
    // Search as well in the not connected event(s) vector
    //

    std::vector<EventNotConnected>::iterator ite;
    for(ite = event_not_connected.begin(); ite != event_not_connected.end(); ++ite)
    {
        if(ite->device == _dev)
        {
            _ids.push_back(ite->event_id);
        }
    }
}

/**
 *
 * Set the ZMQ Receive Buffer High Water Mark only if it changes
 * This is to reduce the impact of a bug present in ZMQ 4.2.0 and ZMQ 4.2.1
 * which leads to a bad lwm and hwm calculation when ZMQ_RCVHWM is set after
 * the bind of the socket. See cppTango#444 for more details
 *
 * @param hwm: new ZMQ receive buffer high water mark
 */
void ZmqEventConsumer::set_socket_hwm(int hwm)
{
    if(hwm != event_sub_sock->get(zmq::sockopt::rcvhwm))
    {
        event_sub_sock->set(zmq::sockopt::rcvhwm, hwm);
    }
}

ReceivedFromAdmin ZmqEventConsumer::initialize_received_from_admin(const Tango::DevVarLongStringArray *dvlsa,
                                                                   const std::string &local_callback_key,
                                                                   const std::string &adm_name,
                                                                   bool device_from_env_var)
{
    ReceivedFromAdmin result;
    if(dvlsa->lvalue.length() == 0)
    {
        TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept,
                                       API_NotSupported,
                                       "Server did not send its tango lib version. The server is possibly too old. The "
                                       "event system is not initialized!");
    }

    long server_tango_lib_ver = dvlsa->lvalue[0];

    // event name is used for zmq topics filtering
    // channel name is used for heartbeat events
    if(server_tango_lib_ver >= 930)
    {
        result.event_name = (dvlsa->svalue[dvlsa->svalue.length() - 2]);
        result.channel_name = (dvlsa->svalue[dvlsa->svalue.length() - 1]);

        if(adm_name.find(MODIFIER_DBASE_NO) != std::string::npos &&
           result.channel_name.find(MODIFIER_DBASE_NO) == std::string::npos)
        {
            result.channel_name += MODIFIER_DBASE_NO;
        }
    }
    else
    {
        result.event_name = local_callback_key;

        if(server_tango_lib_ver >= 810)
        {
            std::string adm_name_lower(adm_name);
            if(device_from_env_var)
            {
                adm_name_lower.insert(0, env_var_fqdn_prefix[0]);
            }
            std::transform(adm_name_lower.begin(), adm_name_lower.end(), adm_name_lower.begin(), ::tolower);
            result.channel_name = adm_name_lower;
        }
        else
        {
            // For event coming from server still using Tango 8.0.x or below, do not lowercase
            // the adm_name in the channel name
            result.channel_name = adm_name;
        }
    }

    if(result.event_name.empty())
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_NotSupported,
            "Server did not send the event name. The server is possibly too old. The event system is not initialized!");
    }

    TANGO_LOG_DEBUG << "received_from_admin.event_name = " << result.event_name << std::endl;
    if(result.channel_name.empty())
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_NotSupported,
            "Server did not send the channel name. The server is possibly too old. The event "
            "system is not initialized!");
    }
    TANGO_LOG_DEBUG << "received_from_admin.channel_name = " << result.channel_name << std::endl;
    return result;
}

void ZmqEventConsumer::disconnect_socket(zmq::socket_t &socket, const char *endpoint)
{
    try
    {
        socket.disconnect(endpoint);
    }
    catch(const zmq::error_t &e)
    {
        // Silently ignore ENOENT as it indicates that endpoint is already disconnected.
        if(e.num() != ENOENT)
        {
            throw;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqAttrValUnion::operator<<=()
//
// description :
//        Write our own unmarshalling method. The omniORB one allocate memory and copy data. We already have memory
//        allocated in the ZMQ message. No need to allocate once more and to copy data. We are doing this only for
//        attribute data. For the remaining, keep using omniORB stuff
//
// argument :
//        in :
//            - n :
//
//-------------------------------------------------------------------------------------------------------------------

void ZmqAttrValUnion::operator<<=(TangoCdrMemoryStream &_n)
{
    char *data_ptr;
    if(_n.get_un_marshal_type() == TangoCdrMemoryStream::UN_ATT)
    {
        data_ptr = (char *) _n.bufPtr();
    }
    else
    {
        data_ptr = (char *) _n.get_mkr_in_buf();
    }

    //
    // Get union discriminator from cdr and if data type is string or device_state let omniORB do its stuff.
    // Don't forget to rewind memory ptr before returning to omniORB
    //

    AttributeDataType _pd__d = ATT_BOOL;
    _pd__d <<= _n;

    if(_pd__d == ATT_STRING || _pd__d == DEVICE_STATE)
    {
        if(_n.get_un_marshal_type() == TangoCdrMemoryStream::UN_ATT)
        {
            _n.rewindPtrs();
        }
        else
        {
            _n.rewind_in(4);
        }
        AttrValUnion::operator<<=(_n);
    }
    else
    {
        //
        // Get data length from cdr
        //

        CORBA::ULong length = 0ul;
        if(_pd__d != ATT_NO_DATA)
        {
            length <<= _n;
            if(length == 0)
            {
                return;
            }
        }

        //
        // Get att data depending on type
        //

        switch(_pd__d)
        {
        case ATT_SHORT:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_2);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevShort, DevVarShortArray>(data_ptr, length, _n);
        }
        break;

        case ATT_DOUBLE:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_8);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevDouble, DevVarDoubleArray>(data_ptr, length, _n);
        }
        break;

        case ATT_FLOAT:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_4);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevFloat, DevVarFloatArray>(data_ptr, length, _n);
        }
        break;

        case ATT_USHORT:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_2);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevUShort, DevVarUShortArray>(data_ptr, length, _n);
        }
        break;

        case ATT_BOOL:
        {
            init_seq<DevBoolean, DevVarBooleanArray>(data_ptr, length, _n);
        }
        break;

        case ATT_LONG:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_4);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevLong, DevVarLongArray>(data_ptr, length, _n);
        }
        break;

        case ATT_LONG64:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_8);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevLong64, DevVarLong64Array>(data_ptr, length, _n);
        }
        break;

        case ATT_ULONG:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_4);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevULong, DevVarULongArray>(data_ptr, length, _n);
        }
        break;

        case ATT_ULONG64:
        {
            omni::ptr_arith_t in = (omni::ptr_arith_t) _n.get_mkr_in_buf();
            omni::ptr_arith_t p1 = _n.align_to(in, omni::ALIGN_8);
            _n.set_mkr_in_buf((void *) p1);
            init_seq<DevULong64, DevVarULong64Array>(data_ptr, length, _n);
        }
        break;

        case ATT_UCHAR:
        {
            init_seq<DevUChar, DevVarUCharArray>(data_ptr, length, _n);
        }
        break;

        case ATT_STATE:
        {
            init_seq<DevState, DevVarStateArray>(data_ptr, length, _n);
        }
        break;

            //
            // We have special cases for DevEncoded (a structure) and ATT_NO_DATA
            //

        case ATT_ENCODED:
        {
            DevVarEncodedArray dummy_seq;
            encoded_att_value(dummy_seq);

            DevVarEncodedArray &dvea = encoded_att_value();
            dvea.length(length);

            for(CORBA::ULong i = 0; i < length; i++)
            {
                dvea[i].encoded_format = _n.unmarshalString(0);
                CORBA::ULong seq_length;
                seq_length <<= _n;
                _CORBA_Octet *ptr = (_CORBA_Octet *) (data_ptr + _n.currentInputPtr());
                dvea[i].encoded_data.replace(seq_length, seq_length, ptr, false);
                _n.tango_get_octet_array((seq_length * sizeof(_CORBA_Octet)));
            }
        }
        break;

        case ATT_NO_DATA:
        {
            DevBoolean bo;
            bo = _n.unmarshalBoolean();

            union_no_data(bo);
        }
        break;

        default:
            TANGO_ASSERT_ON_DEFAULT(_pd__d);
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqAttributeValue_4::operator<<=()
//
// description :
//
// argument :
//        in :
//
//--------------------------------------------------------------------------------------------------------------------

void Tango::ZmqAttributeValue_4::operator<<=(TangoCdrMemoryStream &_n)
{
    zvalue <<= _n;
    quality <<= _n;
    data_format <<= _n;
    time <<= _n;
    name = _n.unmarshalString(0);
    r_dim <<= _n;
    w_dim <<= _n;
    err_list <<= _n;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqAttributeValue_5::operator<<=()
//
// description :
//
// argument :
//        in :
//
//--------------------------------------------------------------------------------------------------------------------

void Tango::ZmqAttributeValue_5::operator<<=(TangoCdrMemoryStream &_n)
{
    zvalue <<= _n;
    quality <<= _n;
    data_format <<= _n;
    data_type <<= _n;
    time <<= _n;
    name = _n.unmarshalString(0);
    r_dim <<= _n;
    w_dim <<= _n;
    err_list <<= _n;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqDevPipeData::operator<<=()
//
// description :
//
// argument :
//        in :
//
//--------------------------------------------------------------------------------------------------------------------

void Tango::ZmqDevPipeData::operator<<=(TangoCdrMemoryStream &_n)
{
    name = _n.unmarshalString(0);
    time <<= _n;
    (ZmqDevPipeBlob &) data_blob <<= _n;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqDevPipeBlob::operator<<=()
//
// description :
//
// argument :
//        in :
//
//--------------------------------------------------------------------------------------------------------------------

#if defined(__clang__)
// UBSAN: downcast of address 0x55daeb87ca40 which does not point to an object of type
// 'Tango::ZmqDevVarPipeDataEltArray'
//        0x55daeb87ca40: note: object is of type 'Tango::DevVarPipeDataEltArray'
// TODO: All these downcasts to dummy Zmq... types are UB and eventually should be fixed.
[[clang::no_sanitize("vptr")]]
#elif defined(__GNUC__)
__attribute__((no_sanitize("vptr")))
#endif
void Tango::ZmqDevPipeBlob::operator<<=(TangoCdrMemoryStream &_n)
{
    name = _n.unmarshalString(0);
    (ZmqDevVarPipeDataEltArray &) blob_data <<= _n;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqDevVarPipeDataEltArray::operator<<=()
//
// description :
//
// argument :
//        in :
//
//--------------------------------------------------------------------------------------------------------------------

void Tango::ZmqDevVarPipeDataEltArray::operator<<=(TangoCdrMemoryStream &_n)
{
    CORBA::ULong _l;
    _l <<= _n;
    if(!_n.checkInputOverrun(1, _l))
    {
        _CORBA_marshal_sequence_range_check_error(_n);
        // never reach here
    }
    length(_l);
    for(CORBA::ULong _i = 0; _i < _l; _i++)
    {
        DevPipeDataElt &dpde = pd_buf[_i];
        ZmqDevPipeDataElt &z_dpde = static_cast<ZmqDevPipeDataElt &>(dpde);
        z_dpde <<= _n;
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqDevPipeDataElt::operator<<=()
//
// description :
//
// argument :
//        in :
//
//--------------------------------------------------------------------------------------------------------------------

#if defined(__clang__)
// Same problem as in: void Tango::ZmqDevPipeBlob::operator<<= (TangoCdrMemoryStream &_n)
[[clang::no_sanitize("vptr")]]
#elif defined(__GNUC__)
__attribute__((no_sanitize("vptr")))
#endif
void Tango::ZmqDevPipeDataElt::operator<<=(TangoCdrMemoryStream &_n)
{
    name = _n.unmarshalString(0);
    (ZmqAttrValUnion &) value <<= _n;
    (ZmqDevVarPipeDataEltArray &) inner_blob <<= _n;
    inner_blob_name = _n.unmarshalString(0);
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        DelayEvent::DelayEvent
//
// description :
//        A class to ask the ZMQ main thread to stop receiving external event. This is necessary to prevent a possible
//      deadlock which could happen if an event is received while a user is calling subscribe or unsubscribe event
//
// argument :
//        in :
//            - ec : Event consumer pointer
//
//--------------------------------------------------------------------------------------------------------------------

DelayEvent::DelayEvent(const PointerWithLock<EventConsumer> &ec)
{
    std::string str;
    ec->get_subscription_command_name(str);

    //
    // Do something only for ZMQ event system
    //

    if(str[0] == 'Z')
    {
        auto eve_con = ApiUtil::instance()->get_zmq_event_consumer_derived(ec.operator->());

        //
        // Do something only for ZMQ event system
        //

        zmq::message_t reply;

        try
        {
            zmq::socket_t sender(eve_con->zmq_context, ZMQ_REQ);

            //
            // In case this thread runs before the main ZMQ thread, it is possible to call connect before the main ZMQ
            // thread has binded its socket. In such a case, error code is set to ECONNREFUSED. If this happens, give
            // the main ZMQ thread a chance to run and retry the connect call. I have tried with a yield call but it
            // still failed in some cases (when running the DS with a file as database  for instance). Replace the yield
            // with a 15 mS sleep !!!
            //
            // Since ZMQ 4, itś possible to connect to the remote socket event if it is not yet bound but the remote
            // socket will hang in the its recev call!!!!
            // We still need the sleep call but not in the exception case
            //

            try
            {
                sender.connect(CTRL_SOCK_ENDPOINT);

                if(!eve_con->is_ctrl_sock_bound())
                {
#ifndef _TG_WINDOWS_
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
#else
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
#endif
                }
            }
            catch(zmq::error_t &e)
            {
                if(e.num() == ECONNREFUSED)
                {
#ifndef _TG_WINDOWS_
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
#else
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
#endif
                    sender.connect(CTRL_SOCK_ENDPOINT);
                }
                else
                {
                    throw;
                }
            }

            //
            // Build message sent to ZMQ main thread. In this case, this is only a command code
            //

            char buffer[10];
            int length = 0;

            buffer[length] = ZMQ_DELAY_EVENT;
            length++;

            eve_con->subscription_monitor.get_monitor();

            //
            // Send command to main ZMQ thread
            //

            zmq::message_t send_data(length);
            ::memcpy(send_data.data(), buffer, length);
            auto result = sender.send(send_data, zmq::send_flags::none);
            TANGO_ASSERT(result);

            result = sender.recv(reply);
            TANGO_ASSERT(result);
        }
        catch(zmq::error_t &e)
        {
            eve_con->subscription_monitor.rel_monitor();

            TangoSys_OMemStream o;

            o << "Failed to delay event!\n";
            o << "Error while communicating with the ZMQ main thread\n";
            o << "ZMQ message: " << e.what() << std::ends;

            TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
        }

        //
        // In case of error returned by the main ZMQ thread
        //

        if(reply.size() != 2)
        {
            eve_con->subscription_monitor.rel_monitor();

            char err_mess[512];
            ::memcpy(err_mess, reply.data(), reply.size());
            err_mess[reply.size()] = '\0';

            TangoSys_OMemStream o;

            o << "Failed to delay events!\n";
            o << "Error while asking the ZMQ thread to delay events\n";
            o << "ZMQ message: " << err_mess << std::ends;

            TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
        }
    }
}

DelayEvent::DelayEvent(EventConsumer *ec) :
    DelayEvent(ApiUtil::instance()->get_locked_event_consumer(ec))
{
}

DelayEvent::~DelayEvent()
{
    try
    {
        release();
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
    }
    catch(std::exception &e)
    {
        std::cerr << "DelayEvent::~DelayEvent(): std::exception thrown: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "DelayEvent::~DelayEvent(): Unknown exception thrown " << std::endl;
    }
}

void DelayEvent::release()
{
    auto ec = ApiUtil::instance()->get_zmq_event_consumer();

    if(ec == nullptr)
    {
        return;
    }

    auto eve_con = ApiUtil::instance()->get_zmq_event_consumer_derived(ec.operator->());

    zmq::message_t reply;

    try
    {
        zmq::socket_t sender(eve_con->zmq_context, ZMQ_REQ);
        sender.connect(CTRL_SOCK_ENDPOINT);

        //
        // Build message sent to ZMQ main thread. In this case, this is only a command code
        //

        char buffer[10];
        int length = 0;

        buffer[length] = ZMQ_RELEASE_EVENT;
        length++;

        //
        // Send command to main ZMQ thread
        //

        zmq::message_t send_data(length);
        ::memcpy(send_data.data(), buffer, length);
        auto result = sender.send(send_data, zmq::send_flags::none);
        TANGO_ASSERT(result);

        result = sender.recv(reply);
        TANGO_ASSERT(result);

        eve_con->subscription_monitor.rel_monitor();
    }
    catch(zmq::error_t &e)
    {
        eve_con->subscription_monitor.rel_monitor();

        TangoSys_OMemStream o;

        o << "Failed to delay event!\n";
        o << "Error while communicating with the ZMQ main thread\n";
        o << "ZMQ message: " << e.what() << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }

    //
    // In case of error returned by the main ZMQ thread
    //

    if(reply.size() != 2)
    {
        char err_mess[512];
        ::memcpy(err_mess, reply.data(), reply.size());
        err_mess[reply.size()] = '\0';

        TangoSys_OMemStream o;

        o << "Failed to release event!\n";
        o << "Error while trying to ask the ZMQ thread to release events\n";
        o << "ZMQ message: " << err_mess << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }
}

} // namespace Tango
