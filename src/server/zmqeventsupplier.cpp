//+==================================================================================================================
//
//  zmqeventsupplier.cpp : C++ classes for implementing the event server and client singleton classes -ZmqEventSupplier
//                           This class is used to send events from the server to the client(s) when zmq is used to
//                           transport the events
//
//  author(s) :           E.Taurel (taurel@esrf.fr)
//
//    original :               August 2011
//
// Copyright (C) :      2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or
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
//+==================================================================================================================

#include <tango/internal/net.h>
#include <tango/internal/utils.h>
#include <tango/server/eventsupplier.h>
#include <tango/server/pipe.h>
#include <tango/server/device.h>
#include <tango/server/dserver.h>
#include <tango/server/utils.h>
#include <tango/client/apiexcept.h>

#include <omniORB4/internal/giopStream.h>
#include <tango/internal/perf_mon.h>

#include <iterator>
#include <future>

#ifdef _TG_WINDOWS_
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
#endif /* _TG_WINDOWS_ */

using namespace CORBA;

namespace Tango
{

// Environment variables for ZMQ publish ports - Event and Heartbeat
static const char *TangoEventPortEnvVar = "TANGO_ZMQ_EVENT_PORT";
static const char *TangoHeartbeatPortEnvVar = "TANGO_ZMQ_HEARTBEAT_PORT";
// check and use environment variables for zmq ports
static void get_zmq_port_from_envvar(const char *, std::string &);

ZmqEventSupplier *ZmqEventSupplier::_instance = nullptr;

/************************************************************************/
/*                                                                           */
/*             ZmqEventSupplier class                                         */
/*            ----------------                                            */
/*                                                                           */
/************************************************************************/

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//      void get_zmq_port_from_envvar()
//
// description :
//      Get the ZeroMQ publisher port from an Environment Variable if defined
//      appends port to endpoint if env var is set, or '*' where zmq will allocate
//      an ephemeral port.
//
//------------------------------------------------------------------------------------------------------------------

void get_zmq_port_from_envvar(const char *zmq_port_env_var, std::string &endpoint)
{
    // We expect a valid endpoint without a port specification - so ends in ':'
    TANGO_ASSERT(!endpoint.empty());
    TANGO_ASSERT(endpoint.back() == ':');
    // default to ephemeral port request for zmq, so that this is always the drop
    // through incase of parsing failures
    std::string zmq_port("*");
    std::string zmq_port_var;
    ApiUtil *au = ApiUtil::instance();
    // get the environment variable
    if(au->get_env_var(zmq_port_env_var, zmq_port_var) == 0)
    {
        // We have an env var - set the port
        zmq_port = zmq_port_var;
    }
    endpoint += zmq_port;
}

ZmqEventSupplier::ZmqEventSupplier(Util *tg) :
    EventSupplier(tg),
    zmq_context(1)

{
    _instance = this;

    //
    // Create zmq release number
    //

    int zmq_major, zmq_minor, zmq_patch;
    zmq_version(&zmq_major, &zmq_minor, &zmq_patch);
    zmq_release = (zmq_major * 100) + (zmq_minor * 10) + zmq_patch;

    //
    // Create the Publisher socket for heartbeat event and bind it
    // If the user has specified one IP address on the command line, re-use it in the endpoint
    // But if the address was specified as a name (supported by omniORB), convert this name to IP address
    // but uses the name in the endpoint given to client
    //

    heartbeat_pub_sock = new zmq::socket_t(zmq_context, ZMQ_PUB);

    int reconnect_ivl = -1;

    heartbeat_pub_sock->set(zmq::sockopt::linger, DEFAULT_LINGER);
    try
    {
        heartbeat_pub_sock->set(zmq::sockopt::reconnect_ivl, reconnect_ivl);
    }
    catch(zmq::error_t &)
    {
        reconnect_ivl = 30000;
        heartbeat_pub_sock->set(zmq::sockopt::reconnect_ivl, reconnect_ivl);
    }

    heartbeat_endpoint = "tcp://";
    alt_ip.clear();
    alternate_e_endpoint.clear();
    alternate_h_endpoint.clear();

    std::string &specified_addr = tg->get_specified_ip();

    bool specified_name = false;
    if(!specified_addr.empty())
    {
        specified_name = !detail::is_ip_address(specified_addr);
    }

    std::string &h_name = tg->get_host_name();
    std::string canon_name;
    std::string specified_ip;

    if(specified_name)
    {
        auto results = detail::resolve_hostname_address(specified_addr);
        specified_ip = results.front();
    }
    else
    {
        specified_ip = specified_addr;
    }

    std::string::size_type pos = h_name.find('.');
    if(pos != std::string::npos)
    {
        canon_name = h_name.substr(0, pos);
    }

    if(!specified_addr.empty() && (specified_addr != "localhost"))
    {
        heartbeat_endpoint = heartbeat_endpoint + specified_ip + ':';
        ip_specified = true;
        if(specified_name)
        {
            name_specified = true;
        }
        user_ip = specified_ip;
    }
    else
    {
        heartbeat_endpoint = heartbeat_endpoint + "*:";
        ip_specified = false;
    }

    // add on the port specification
    get_zmq_port_from_envvar(TangoHeartbeatPortEnvVar, heartbeat_endpoint);

    //
    // Bind the heartbeat publisher socket to the port
    //
    try
    {
        tango_bind(heartbeat_pub_sock, heartbeat_endpoint);
    }
    catch(Tango::DevFailed &ex)
    {
        delete heartbeat_pub_sock;
        heartbeat_pub_sock = nullptr;
        TANGO_RETHROW_DETAILED_EXCEPTION(EventSystemExcept, ex, API_ZmqInitFailed, "Failed to bind heartbeat socket");
    }
    catch(...)
    {
        delete heartbeat_pub_sock;
        heartbeat_pub_sock = nullptr;
        throw;
    }

    auto port_str = detail::get_port_from_endpoint(heartbeat_endpoint);

    //
    // If needed, replace * by host IP address in endpoint string
    //

    if(specified_ip.empty())
    {
        ApiUtil *au = ApiUtil::instance();
        std::vector<std::string> adrs;

        au->get_ip_from_if(adrs);

        std::string::size_type pos = heartbeat_endpoint.find('*');
        if(adrs.size() > 1)
        {
            bool first_set = false;

            for(unsigned int i = 0; i < adrs.size(); ++i)
            {
                std::string::size_type start;
                if((start = adrs[i].find("127.")) == 0)
                {
                    continue;
                }

                if(!first_set)
                {
                    heartbeat_endpoint.replace(pos, 1, adrs[i]);
                    host_ip = adrs[i];
                    first_set = true;
                }
                else
                {
                    alternate_h_endpoint.push_back(detail::qualify_host_address(adrs[i], port_str));
                    alt_ip.push_back(adrs[i]);
                }
            }
        }
        else
        {
            heartbeat_endpoint.replace(pos, 1, adrs[0]);
            host_ip = adrs[0];
        }
    }
    else if(specified_name)
    {
        std::string::size_type start = heartbeat_endpoint.find("//");
        start = start + 2;
        std::string::size_type stop = heartbeat_endpoint.rfind(':');
        heartbeat_endpoint.replace(start, stop - start, specified_addr);
    }

    if(tg->get_endpoint_publish_specified())
    {
        alternate_h_endpoint.push_back(detail::qualify_host_address(tg->get_endpoint_publish(), port_str));
    }

    //
    // Find out the host endianness and create the zmq message used to pass it
    //

    host_endian = test_endian();

    endian_mess.rebuild(1);
    memcpy(endian_mess.data(), &host_endian, 1);

    endian_mess_2.copy(endian_mess);

    endian_mess_heartbeat.rebuild(1);
    memcpy(endian_mess_heartbeat.data(), &host_endian, 1);

    endian_mess_heartbeat_2.copy(endian_mess_heartbeat);

    //
    // Init heartbeat call info
    // Leave the OID and method name un-initialized. Marshall the structure into CORBA CDR
    //

    heartbeat_call.version = ZMQ_EVENT_PROT_VERSION;
    heartbeat_call.call_is_except = false;

    heartbeat_call >>= heartbeat_call_cdr;

    //
    // Create some ZMQ messages from the already created memory buffer in CDR
    //

    heartbeat_call_mess.rebuild(heartbeat_call_cdr.bufSize());
    memcpy(heartbeat_call_mess.data(), heartbeat_call_cdr.bufPtr(), heartbeat_call_cdr.bufSize());

    heartbeat_call_mess_2.copy(heartbeat_call_mess);

    //
    // Build heartbeat name
    // This is something like
    //   tango://host:port/dserver/exec_name/inst_name.heartbeat when using DB
    //   tango://host:port/dserver/exec_name/inst_name#dbase=no.heartbeat when using file as database
    heartbeat_event_name = fqdn_prefix;
    heartbeat_event_name = heartbeat_event_name + "dserver/";
    heartbeat_event_name += tg->get_ds_name();
    if(tg->use_file_db() || !tg->use_db())
    {
        bool db_ds = false;
        const std::vector<DeviceClass *> *cl_ptr = tg->get_class_list();
        for(size_t loop = 0; loop < cl_ptr->size(); loop++)
        {
            if((*cl_ptr)[loop]->get_name() == DATABASE_CLASS)
            {
                db_ds = true;
                break;
            }
        }

        if(!db_ds)
        {
            heartbeat_event_name = heartbeat_event_name + MODIFIER_DBASE_NO;
        }
    }

    heartbeat_event_name = heartbeat_event_name + ".heartbeat";
    std::transform(heartbeat_event_name.begin(), heartbeat_event_name.end(), heartbeat_event_name.begin(), ::tolower);
}

ZmqEventSupplier *ZmqEventSupplier::create(Util *tg)
{
    TANGO_LOG_DEBUG << "calling Tango::ZmqEventSupplier::create()" << std::endl;

    //
    // Does the ZmqEventSupplier singleton exist already ? if so simply return it
    //

    if(_instance != nullptr)
    {
        return _instance;
    }

    //
    // ZmqEventSupplier singleton does not exist, create it
    //

    ZmqEventSupplier *_event_supplier = new ZmqEventSupplier(tg);

    return _event_supplier;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::~ZmqEventSupplier()
//
// description :
//        ZmqEventSupplier destructor.
//      This dtor delete the zmq socket. The ZMQ context will be deleted (then calling zmq_term) when the object is
//      destroyed
//
//-------------------------------------------------------------------------------------------------------------------

ZmqEventSupplier::~ZmqEventSupplier()
{
    //
    // Delete zmq sockets
    //

    delete heartbeat_pub_sock;
    delete event_pub_sock;

    if(!event_mcast.empty())
    {
        std::map<std::string, McastSocketPub>::iterator ite, ite_stop;
        ite = event_mcast.begin();
        ite_stop = event_mcast.end();

        for(ite = event_mcast.begin(); ite != ite_stop; ++ite)
        {
            delete ite->second.pub_socket;
        }
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::tango_bind()
//
// description :
//        Choose a free port to bind ZMQ socket. Ask ZMQ to give us a port number
//
// argument :
//        in :
//        - sock : The ZMQ socket
//      - endpoint : The beginning of the ZMQ endpoint
//
//-------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::tango_bind(zmq::socket_t *sock, std::string &endpoint)
{
    std::string base_endpoint(endpoint);

    try
    {
        sock->bind(base_endpoint);

        std::string str = sock->get(zmq::sockopt::last_endpoint);

        // if this was a request for an ephemeral port then we need to look
        // up which port was allocated
        if(endpoint.back() == '*')
        {
            endpoint.pop_back();
            std::string::size_type pos = str.rfind(':');
            std::string port_str = str.substr(pos + 1);
            endpoint = endpoint + port_str;
        }
    }
    catch(const zmq::error_t &ex)
    {
        std::stringstream ss;
        ss << "Cannot bind to ZMQ endpoint \"" << base_endpoint << "\": " << ex.what();
        TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept, API_ZmqInitFailed, ss.str());
    }
    catch(...)
    {
        std::stringstream ss;
        ss << "Cannot bind to ZMQ endpoint \"" << base_endpoint << "\"";
        TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept, API_ZmqInitFailed, ss.str());
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::test_endian()
//
// description :
//        Get the host endianness
//
// return:
//         This method returns the host endianness
//             0 -> Big endian
//          1 -> Little endian
//
//-------------------------------------------------------------------------------------------------------------------

unsigned char ZmqEventSupplier::test_endian()
{
    int test_var = 1;
    unsigned char *cptr = (unsigned char *) &test_var;
    return static_cast<unsigned char>(!(cptr[0] == 0));
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::create_event_socket()
//
// description :
//        Create and bind the publisher socket used to publish the real events
//
//------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::create_event_socket()
{
    if(event_pub_sock == nullptr)
    {
        omni_mutex_lock oml(event_mutex);
        //
        // Create the Publisher socket for real events and bind it
        // If the user has specified one IP address on the command line, re-use it in the endpoint
        //

        event_pub_sock = new zmq::socket_t(zmq_context, ZMQ_PUB);
        int reconnect_ivl = -1;
        event_pub_sock->set(zmq::sockopt::linger, DEFAULT_LINGER);

        try
        {
            event_pub_sock->set(zmq::sockopt::reconnect_ivl, reconnect_ivl);
        }
        catch(zmq::error_t &)
        {
            reconnect_ivl = 30000;
            event_pub_sock->set(zmq::sockopt::reconnect_ivl, reconnect_ivl);
        }

        event_endpoint = "tcp://";

        if(ip_specified)
        {
            event_endpoint = event_endpoint + user_ip + ':';
        }
        else
        {
            event_endpoint = event_endpoint + "*:";
        }

        //
        // Set a publisher HWM
        //

        Tango::Util *tg = Tango::Util::instance();
        DServer *admin_dev = tg->get_dserver_device();

        int hwm = tg->get_user_pub_hwm();
        if(hwm == -1)
        {
            hwm = admin_dev->zmq_pub_event_hwm;
        }

        event_pub_sock->set(zmq::sockopt::sndhwm, hwm);

        // add on the port specification
        get_zmq_port_from_envvar(TangoEventPortEnvVar, event_endpoint);

        //
        // Bind the event publisher socket to the port
        //

        try
        {
            tango_bind(event_pub_sock, event_endpoint);
        }
        catch(Tango::DevFailed &ex)
        {
            delete event_pub_sock;
            event_pub_sock = nullptr;
            TANGO_RETHROW_DETAILED_EXCEPTION(EventSystemExcept, ex, API_ZmqInitFailed, "Failed to bind event socket");
        }
        catch(...)
        {
            delete event_pub_sock;
            event_pub_sock = nullptr;
            throw;
        }

        //
        // If needed, replace * by host IP address in endpoint string
        //
        auto port_str = detail::get_port_from_endpoint(event_endpoint);

        if(!ip_specified)
        {
            event_endpoint.replace(6, 1, host_ip);
            if(!alt_ip.empty())
            {
                for(size_t loop = 0; loop < alt_ip.size(); loop++)
                {
                    alternate_e_endpoint.push_back(detail::qualify_host_address(alt_ip[loop], port_str));
                }
            }
        }
        else if(name_specified)
        {
            std::string::size_type start = event_endpoint.find("//");
            start = start + 2;
            std::string::size_type stop = event_endpoint.rfind(':');
            std::string &specified_addr = tg->get_specified_ip();
            event_endpoint.replace(start, stop - start, specified_addr);
        }

        if(tg->get_endpoint_publish_specified())
        {
            alternate_e_endpoint.push_back(detail::qualify_host_address(tg->get_endpoint_publish(), port_str));
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::create_mcast_event_socket()
//
// description :
//        Create and bind the publisher socket used to publish the real events when multicast transport is required
//
// argument :
//        in :
//            - mcast_data : The multicast addr and port (mcast_adr:port)
//          - ev_name : The event name (dev_name/attr_name.event_type)
//          - rate: The user defined PGM rate (O if undefined)
//          - local_call: True if the caller is on the same host
//
//-------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::create_mcast_event_socket(const std::string &mcast_data,
                                                 const std::string &ev_name,
                                                 int rate,
                                                 bool local_call)
{
    std::map<std::string, McastSocketPub>::iterator ite;

    //
    // If the event is already in the mcast event map, check if it is already used by local clients
    //

    if((ite = event_mcast.find(ev_name)) != event_mcast.end())
    {
        if(local_call)
        {
            if(!ite->second.local_client)
            {
                create_event_socket();

                ite->second.local_client = true;
            }
        }
        else
        {
            if(ite->second.local_client && ite->second.pub_socket == nullptr)
            {
                create_mcast_socket(mcast_data, rate, ite->second);
            }
        }
        ite->second.double_send = true;
    }
    else
    {
        //
        // New mcast event
        //

        McastSocketPub ms;
        ms.double_send = true;

        if(local_call)
        {
            create_event_socket();

            ms.pub_socket = nullptr;
            ms.local_client = true;
        }
        else
        {
            create_mcast_socket(mcast_data, rate, ms);

            ms.local_client = false;
        }

        //
        // Insert element in map
        //

        if(!event_mcast.insert(make_pair(ev_name, ms)).second)
        {
            TangoSys_OMemStream o;
            o << "Can't insert multicast transport parameter for event ";
            o << ev_name << " in EventSupplier instance" << std::ends;

            TANGO_THROW_EXCEPTION(API_InternalError, o.str());
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::create_mcast_socket()
//
// description :
//        Create and bind the publisher socket used to publish the real events when multicast transport is required
//
// argument :
//        in :
//            - mcast_data : The multicast addr and port (mcast_adr:port)
//          - rate: The user defined PGM rate (O if undefined)
//          - ms: Reference to the structure to be stored in the macst map
//
//-------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::create_mcast_socket(const std::string &mcast_data, int rate, McastSocketPub &ms)
{
    //
    // Create the Publisher socket for real events and bind it
    // If the user has specified one IP address on the command line, re-use it in the endpoint
    //

    ms.pub_socket = new zmq::socket_t(zmq_context, ZMQ_PUB);

    ms.endpoint = MCAST_PROT;
    if(ip_specified)
    {
        ms.endpoint = ms.endpoint + user_ip + ';';
    }
    else
    {
        ApiUtil *au = ApiUtil::instance();
        std::vector<std::string> adrs;

        au->get_ip_from_if(adrs);

        for(unsigned int i = 0; i < adrs.size(); ++i)
        {
            if(adrs[i].find("127.") == 0)
            {
                continue;
            }
            ms.endpoint = ms.endpoint + adrs[i] + ';';
            break;
        }
    }
    ms.endpoint = ms.endpoint + mcast_data;

    ms.pub_socket->set(zmq::sockopt::linger, DEFAULT_LINGER);

    //
    // Change multicast hops
    //

    Tango::Util *tg = Tango::Util::instance();
    DServer *admin_dev = tg->get_dserver_device();

    ms.pub_socket->set(zmq::sockopt::multicast_hops, static_cast<int>(admin_dev->mcast_hops));

    //
    // Change PGM rate to default value (80 Mbits/sec) or to user defined value
    //

    ms.pub_socket->set(zmq::sockopt::rate, rate);

    //
    // Bind the publisher socket to the specified port
    //

    try
    {
        ms.pub_socket->bind(ms.endpoint);
    }
    catch(zmq::error_t &e)
    {
        TangoSys_OMemStream o;
        o << "Can't bind ZMQ socket with endpoint ";
        o << ms.endpoint;
        o << "\nZmq error: " << e.what() << std::ends;

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }

    //
    // The connection string returned to client does not need the host IP at all
    //

    ms.endpoint = MCAST_PROT + mcast_data;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::is_event_mcast()
//
// description :
//        This method checks if the event is already defined in the map of multicast event.
//
// argument :
//        in :
//            - ev_name : The event name (device/attr.event_type)
//
// returns :
//         This method returns true if the event is in the map and false otherwise
//
//--------------------------------------------------------------------------------------------------------------------

bool ZmqEventSupplier::is_event_mcast(const std::string &ev_name)
{
    bool ret = false;

    if(event_mcast.find(ev_name) != event_mcast.end())
    {
        ret = true;
    }

    return ret;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::get_mcast_event_endpoint()
//
// description :
//        This method returns the multicast socket endpoint for the event passed as parameter
//
// argument :
//        in :
//            - event_name : The event name (device/attr.event_type)
//
// return :
//         This method returns a reference to the enpoint string
//
//--------------------------------------------------------------------------------------------------------------------

std::string &ZmqEventSupplier::get_mcast_event_endpoint(const std::string &ev_name)
{
    return event_mcast.find(ev_name)->second.endpoint;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::init_event_cptr()
//
// description :
//        Method to initialize event counter for a specific event
//
// argument :
//        in :
//            - event_name : The event name (device/attr.event_type)
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::init_event_cptr(const std::string &event_name)
{
    std::map<std::string, unsigned int>::iterator pos;

    pos = event_cptr.find(event_name);
    if(pos == event_cptr.end())
    {
        if(!event_cptr.insert(make_pair(event_name, 1)).second)
        {
            TangoSys_OMemStream o;
            o << "Can't insert event counter for event ";
            o << event_name << " in EventSupplier instance" << std::ends;

            TANGO_THROW_EXCEPTION(API_InternalError, o.str());
        }
    }
}

// Performance monitoring
namespace
{
struct PerfMonSample
{
    std::int64_t micros_since_last_event{k_invalid_duration};
    std::int64_t push_event_micros{0};

    void json_dump(std::ostream &os)
    {
        os << "{\"micros_since_last_event\":";
        if(micros_since_last_event != k_invalid_duration)
        {
            os << micros_since_last_event;
        }
        else
        {
            os << "null";
        }
        os << ",\"push_event_micros\":" << push_event_micros;
        os << "}";
    }
};

DoubleBuffer<PerfMonSample> g_perf_mon;

// Must hold g_perf_mon.lock to access
PerfClock::time_point g_last_event_timestamp;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::query_event_system()
//
// description :
//  Report information about the event supplier as a JSON object.
//
// argument :
//        in :
//          - os : Output stream to write the JSON object to
//
//--------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::query_event_system(std::ostream &os)
{
    // We don't need to lock access to `event_cptr` because the only time the
    // map is modified is during the ZmqEventSubscriptionChange() command
    // where we are holding the `DServer` lock, which we are holding now.  We
    // might miss an increment of the value from a `push_event` if it is called
    // from some other thread, but that doesn't really matter.
    os << R"({"event_counters":{)";
    bool first = true;
    for(const auto &pair : event_cptr)
    {
        if(!first)
        {
            os << ",";
        }
        os << "\"" << pair.first << "\":" << pair.second;
        first = false;
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

void ZmqEventSupplier::enable_perf_mon(Tango::DevBoolean enabled)
{
    g_perf_mon.enable(enabled);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::push_heartbeat_event()
//
// description :
//        Method to push the hearbeat event
//
//-------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::push_heartbeat_event()
{
    time_t delta_time;
    time_t now_time;

    //
    // Heartbeat - check wether a heartbeat event has been sent recently. If not then send it.
    // A heartbeat contains no data, it is used by the consumer to know that the supplier is still alive.
    //

    Tango::Util *tg = Tango::Util::instance();
    DServer *adm_dev = tg->get_dserver_device();
    now_time = Tango::get_current_system_datetime();
    delta_time = now_time - adm_dev->last_heartbeat_zmq;
    TANGO_LOG_DEBUG << "ZmqEventSupplier::push_heartbeat_event(): delta time since last heartbeat " << delta_time
                    << std::endl;

    //
    // We here compare delta_time to 8 and not to 10.
    // This is necessary because, sometimes the polling thread is some milli second in advance.
    // The computation here is done in seconds. So, if the polling thread is in advance, delta_time computed in
    // seconds will be 9 even if in reality it is 9,9
    //

    if(delta_time >= 8)
    {
        int nb_event = 1;

        TANGO_LOG_DEBUG << "ZmqEventSupplier::push_heartbeat_event(): detected heartbeat event for "
                        << heartbeat_event_name << std::endl;
        TANGO_LOG_DEBUG << "ZmqEventSupplier::push_heartbeat_event(): delta _time " << delta_time << std::endl;

        if(double_send_heartbeat)
        {
            nb_event = 2;
            double_send_heartbeat = false;
        }

        TANGO_LOG_DEBUG << "ZmqEventSupplier::push_heartbeat_event(): nb_event = " << nb_event << std::endl;

        while(nb_event != 0)
        {
            //
            // Create zmq message
            //

            zmq::message_t name_mess(heartbeat_event_name.size());
            memcpy(name_mess.data(), (void *) heartbeat_event_name.data(), heartbeat_event_name.size());

            bool endian_mess_sent = false;
            bool call_mess_sent = false;

            try
            {
                //
                // For debug and logging purposes
                //

                if(nb_event == 1)
                {
                    if(omniORB::trace(20) != 0)
                    {
                        omniORB::logger log;
                        log << "ZMQ: Pushing some data" << '\n';
                    }
                    if(omniORB::trace(30) != 0)
                    {
                        {
                            omniORB::logger log;
                            log << "ZMQ: Event name" << '\n';
                        }
                        omni::giopStream::dumpbuf((unsigned char *) name_mess.data(), name_mess.size());

                        {
                            omniORB::logger log;
                            log << "ZMQ: Endianness" << '\n';
                        }
                        omni::giopStream::dumpbuf((unsigned char *) endian_mess_heartbeat.data(),
                                                  endian_mess_heartbeat.size());

                        {
                            omniORB::logger log;
                            log << "ZMQ: Call info" << '\n';
                        }
                        omni::giopStream::dumpbuf((unsigned char *) heartbeat_call_mess.data(),
                                                  heartbeat_call_mess.size());
                    }
                }

                //
                // Push the event
                //

                adm_dev->last_heartbeat_zmq = now_time;

                heartbeat_pub_sock->send(name_mess, zmq::send_flags::sndmore);
                heartbeat_pub_sock->send(endian_mess_heartbeat, zmq::send_flags::sndmore);
                endian_mess_sent = true;
                heartbeat_pub_sock->send(heartbeat_call_mess, zmq::send_flags::none);
                call_mess_sent = true;

                //
                // Push a dummy message in the event socket. This is required by the ZMQ layer. In case of reconnection,
                // if nothing is sent on the socket between two reconnections, some memory will be "leaked". ZMQ is
                // deleting that memory at process end (zmq context delete) OR when something is sent to the socket.
                // Therefore, send a dummy message to the socket to force ZMQ "garbage collection" because we cannot
                // wait for process termination!
                //

                std::string dummy_message("I like beer");
                zmq::message_t dummy_mess(dummy_message.size());
                memcpy(dummy_mess.data(), (void *) dummy_message.data(), dummy_message.size());

                {
                    omni_mutex_lock guard(push_mutex);
                    event_pub_sock->send(dummy_mess, zmq::send_flags::none);
                }

                //
                // For reference counting on zmq messages which do not have a local scope
                //

                endian_mess_heartbeat.copy(endian_mess_heartbeat_2);
                heartbeat_call_mess.copy(heartbeat_call_mess_2);

                nb_event--;
            }
            catch(...)
            {
                TANGO_LOG_DEBUG << "ZmqEventSupplier::push_heartbeat_event() failed !\n";
                if(endian_mess_sent)
                {
                    endian_mess_heartbeat.copy(endian_mess_heartbeat_2);
                }
                if(call_mess_sent)
                {
                    heartbeat_call_mess.copy(heartbeat_call_mess_2);
                }

                TangoSys_OMemStream o;
                o << "Can't push ZMQ heartbeat event for event ";
                o << heartbeat_event_name;
                if(zmq_errno() != 0)
                {
                    o << "\nZmq error: " << zmq_strerror(zmq_errno()) << std::ends;
                }
                else
                {
                    o << std::ends;
                }

                TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
            }
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::push_event()
//
// description :
//        Method to send the event to the event channel
//
// argument :
//        in :
//            - device_impl : The device
//            - event_type : The event type (change, periodic....)
//            - filterable_names :
//            - filterable_data :
//            - attr_value : The attribute value
//            - obj_name : The attribute/pipe name
//            - except : The exception thrown during the last attribute reading. nullptr if no exception
//
//-------------------------------------------------------------------------------------------------------------------

// Small callback used by ZMQ when using the no-copy API to signal that the message has been sent.
// We now use a std::future (hidden by its associated std::promise) to notify the thread sending the event that
// the associated data has been fully sent and is no longer used by zeromq (part of the Tango zero copy strategy).
//
// For information, note that this method is also called when ZMQ connection is closed. In such a case,
// the calling thread is the thread doing the zmq::send() call and it's not the ZMQ thread. However this particular
// behavior has no impact on the current implementation (i.e. the one making use of the std::future).
//
// The hint argument is a pointer to std::promise<void>. Callback takes ownership of it and deletes it after setting a
// value. Setting the value of the promise (i.e., of the underlying std::future) will wakeup any thread waiting on this
// future. Contrary to what could happen with a condition variable, the std::future garantees that the thread calling
// std::future::wait will never be deadlocked.
static void tg_unlock(TANGO_UNUSED(void *data), void *hint)
{
    std::promise<void> *p = static_cast<std::promise<void> *>(hint);
    p->set_value();
    delete p;
}

void ZmqEventSupplier::push_event(DeviceImpl *device_impl,
                                  std::string event_type,
                                  TANGO_UNUSED(const std::vector<std::string> &filterable_names),
                                  TANGO_UNUSED(const std::vector<double> &filterable_data),
                                  TANGO_UNUSED(const std::vector<std::string> &filterable_names_lg),
                                  TANGO_UNUSED(const std::vector<long> &filterable_data_lg),
                                  const struct SuppliedEventData &ev_value,
                                  const std::string &obj_name,
                                  DevFailed *except,
                                  bool inc_cptr)
{
    PerfMonSample sample;
    SamplePusher<PerfMonSample> pusher{false, sample, *g_perf_mon.front, g_perf_mon.lock};
    TimeBlockMicros perf_mon;
    if(g_perf_mon.lock.try_lock())
    {
        if(g_perf_mon.enabled)
        {
            perf_mon = TimeBlockMicros{true, &sample.push_event_micros};
            pusher.enabled = true;

            if(g_last_event_timestamp != PerfClock::time_point{})
            {
                sample.micros_since_last_event = duration_micros(g_last_event_timestamp, perf_mon.start);
            }
            g_last_event_timestamp = perf_mon.start;
        }

        g_perf_mon.lock.unlock();
    }

    if(device_impl == nullptr)
    {
        return;
    }

    TANGO_LOG_DEBUG << "ZmqEventSupplier::push_event(): called for attribute/pipe " << obj_name << std::endl;

    //
    // Get the mutex to synchronize the sending of events
    // This method may be called by several threads in case they are several
    // user threads doing dev.push_xxxx_event() on several devices.
    // On top of that, zmq socket can be used by several threads
    // only if they are memory barriers between their use in these different
    // threads. The mutex used here is also a memory barrier
    //

    omni_thread *th_id = omni_thread::self();
    if(th_id == nullptr)
    {
        th_id = omni_thread::create_dummy();
    }

    omni_mutex_lock guard(push_mutex);

    calling_th = th_id->id();

    //
    // Create full event name
    // Don't forget case where we have notifd client (thus with a fqdn_prefix modified)
    //

    std::string local_event_type = detail::remove_idl_prefix(event_type);

    bool intr_change = false;
    if(local_event_type == EventName[INTERFACE_CHANGE_EVENT])
    {
        intr_change = true;
    }

    bool pipe_event = false;
    if(local_event_type == EventName[PIPE_EVENT])
    {
        pipe_event = true;
    }

    std::string loc_obj_name(obj_name);
    std::transform(loc_obj_name.begin(), loc_obj_name.end(), loc_obj_name.begin(), ::tolower);

    event_name = create_full_event_name(device_impl, event_type, loc_obj_name, intr_change);
    ctr_event_name = create_full_event_name(device_impl, local_event_type, loc_obj_name, intr_change);

    //
    // Create zmq messages
    // Use memcpy here. Don't use message with no-copy option because
    // it does not give any performance improvement in this case (too small amount of data)
    //

    zmq::message_t name_mess(event_name.size());
    memcpy(name_mess.data(), event_name.data(), event_name.size());

    //
    // Get event cptr and create the event call zmq message
    //

    std::map<std::string, unsigned int>::iterator ev_cptr_ite;
    unsigned int ev_ctr = 0;

    ev_cptr_ite = event_cptr.find(ctr_event_name);
    if(ev_cptr_ite != event_cptr.end())
    {
        ev_ctr = ev_cptr_ite->second;
    }
    else
    {
        bool print = false;
        if(!intr_change && !pipe_event)
        {
            Attribute &att = device_impl->get_device_attr()->get_attr_by_name(obj_name.c_str());

            if(local_event_type == "data_ready")
            {
                if(att.event_data_ready_subscription != 0)
                {
                    print = true;
                }
            }
            else if(local_event_type == "attr_conf")
            {
                if(att.event_attr_conf_subscription != 0 || att.event_attr_conf5_subscription != 0)
                {
                    print = true;
                }
            }
            else if(local_event_type == "user_event")
            {
                if(att.event_user3_subscription != 0 || att.event_user4_subscription != 0 ||
                   att.event_user5_subscription != 0)
                {
                    print = true;
                }
            }
            else if(local_event_type == "change")
            {
                if(att.event_change3_subscription != 0 || att.event_change4_subscription != 0 ||
                   att.event_change5_subscription != 0)
                {
                    print = true;
                }
            }
            else if(local_event_type == "periodic")
            {
                if(att.event_periodic3_subscription != 0 || att.event_periodic4_subscription != 0 ||
                   att.event_periodic5_subscription != 0)
                {
                    print = true;
                }
            }
            else if(local_event_type == "archive")
            {
                if(att.event_archive3_subscription != 0 || att.event_archive4_subscription != 0 ||
                   att.event_archive5_subscription != 0)
                {
                    print = true;
                }
            }
            else if(local_event_type == "alarm")
            {
                if(att.event_alarm6_subscription != 0)
                {
                    print = true;
                }
            }
        }
        else if(pipe_event)
        {
            Pipe &pi =
                device_impl->get_device_class()->get_pipe_by_name(obj_name.c_str(), device_impl->get_name_lower());
            if(pi.event_subscription != 0)
            {
                print = true;
            }
        }
        else
        {
            if(device_impl->get_event_intr_change_subscription() != 0)
            {
                print = true;
            }
        }

        if(print)
        {
            TANGO_LOG_DEBUG << "-----> Can't find event counter for event " << event_name << " in map!!!!!!!!!!"
                            << std::endl;
        }
    }

    ZmqCallInfo event_call;
    event_call.version = ZMQ_EVENT_PROT_VERSION;
    event_call.call_is_except = except != nullptr;
    event_call.ctr = ev_ctr;

    cdrMemoryStream event_call_cdr;
    event_call >>= event_call_cdr;

    zmq::message_t event_call_mess(event_call_cdr.bufSize());
    memcpy(event_call_mess.data(), event_call_cdr.bufPtr(), event_call_cdr.bufSize());

    bool large_data = false;
    size_t mess_size;
    void *mess_ptr;
    zmq::message_t data_mess;
    //
    // We will need this std::future object only for sending a large data.
    // In other cases it has invalid state and shouldn't be used.
    //
    std::future<void> large_message_future;

    if(ev_value.zmq_mess != nullptr)
    {
        //
        // It's a forwarded attribute, therefore, use the already marshalled message
        //

        mess_ptr = ev_value.zmq_mess->data();
        mess_size = ev_value.zmq_mess->size();

        data_mess.move(*(ev_value.zmq_mess));
    }
    else
    {
        //
        // Marshall the event data
        //

        CORBA::ULong padding = 0XDEC0DEC0UL;
        data_call_cdr.rewindPtrs();

        padding >>= data_call_cdr;
        padding >>= data_call_cdr;

        if(except == nullptr)
        {
            if(ev_value.attr_val != nullptr)
            {
                *(ev_value.attr_val) >>= data_call_cdr;
            }
            else if(ev_value.attr_val_3 != nullptr)
            {
                *(ev_value.attr_val_3) >>= data_call_cdr;
            }
            else if(ev_value.attr_val_4 != nullptr)
            {
                //
                // Get number of data exchanged by this event. If this value is greater than a threshold, set a flag
                // In such a case, we will use ZMQ no-copy message call
                //

                *(ev_value.attr_val_4) >>= data_call_cdr;

                mess_ptr = data_call_cdr.bufPtr();
                mess_ptr = (char *) mess_ptr + (sizeof(CORBA::Long) << 1);

                int nb_data;
                int data_discr = ((int *) mess_ptr)[0];

                if(data_discr == ATT_ENCODED)
                {
                    const DevVarEncodedArray &dvea = ev_value.attr_val_4->value.encoded_att_value();
                    nb_data = dvea.length();
                    if(nb_data > LARGE_DATA_THRESHOLD_ENCODED)
                    {
                        large_data = true;
                    }
                }
                else if(data_discr == ATT_NO_DATA)
                {
                    nb_data = 0;
                }
                else
                {
                    nb_data = ((int *) mess_ptr)[1];
                    if(nb_data >= LARGE_DATA_THRESHOLD)
                    {
                        large_data = true;
                    }
                }
            }
            else if(ev_value.attr_val_5 != nullptr)
            {
                //
                // Get number of data exchanged by this event. If this value is greater than a threshold, set a flag
                // In such a case, we will use ZMQ no-copy message call
                //

                *(ev_value.attr_val_5) >>= data_call_cdr;

                mess_ptr = data_call_cdr.bufPtr();
                mess_ptr = (char *) mess_ptr + (sizeof(CORBA::Long) << 1);

                int nb_data;
                int data_discr = ((int *) mess_ptr)[0];

                if(data_discr == ATT_ENCODED)
                {
                    const DevVarEncodedArray &dvea = ev_value.attr_val_5->value.encoded_att_value();
                    nb_data = dvea.length();
                    if(nb_data > LARGE_DATA_THRESHOLD_ENCODED)
                    {
                        large_data = true;
                    }
                }
                else if(data_discr == ATT_NO_DATA)
                {
                    nb_data = 0;
                }
                else
                {
                    nb_data = ((int *) mess_ptr)[1];
                    if(nb_data >= LARGE_DATA_THRESHOLD)
                    {
                        large_data = true;
                    }
                }
            }
            else if(ev_value.attr_conf_2 != nullptr)
            {
                *(ev_value.attr_conf_2) >>= data_call_cdr;
            }
            else if(ev_value.attr_conf_3 != nullptr)
            {
                *(ev_value.attr_conf_3) >>= data_call_cdr;
            }
            else if(ev_value.attr_conf_5 != nullptr)
            {
                *(ev_value.attr_conf_5) >>= data_call_cdr;
            }
            else if(ev_value.attr_dat_ready != nullptr)
            {
                *(ev_value.attr_dat_ready) >>= data_call_cdr;
            }
            else if(ev_value.pipe_val != nullptr)
            {
                size_t nb_data = get_blob_data_nb(ev_value.pipe_val->data_blob.blob_data);
                if(nb_data >= LARGE_DATA_THRESHOLD)
                {
                    large_data = true;
                }

                *(ev_value.pipe_val) >>= data_call_cdr;
            }
            else
            {
                *(ev_value.dev_intr_change) >>= data_call_cdr;
            }
        }
        else
        {
            except->errors >>= data_call_cdr;
        }

        if(!pipe_event)
        {
            mess_size = data_call_cdr.bufSize() - sizeof(CORBA::Long);
            mess_ptr = (char *) data_call_cdr.bufPtr() + sizeof(CORBA::Long);
        }
        else
        {
            mess_size = data_call_cdr.bufSize();
            mess_ptr = (char *) data_call_cdr.bufPtr();
        }

        //
        // For event with small amount of data, use memcpy to initialize the zmq message. For large amount of data, use
        // zmq message with no-copy option
        //

        if(large_data)
        {
            std::promise<void> *large_message_promise = new std::promise<void>();
            large_message_future = large_message_promise->get_future();
            //
            // We pass ownership of large_message_promise to tg_unlock function.
            //
            data_mess.rebuild(mess_ptr, mess_size, tg_unlock, (void *) large_message_promise);
        }
        else
        {
            data_mess.rebuild(mess_size);
            memcpy(data_mess.data(), mess_ptr, mess_size);
        }
    }

    //
    // Send the data
    //

    bool endian_mess_sent = false;

    try
    {
        //
        // For debug and logging purposes
        //

        if(omniORB::trace(20) != 0)
        {
            omniORB::logger log;
            log << "ZMQ: Pushing some data" << '\n';
        }
        if(omniORB::trace(30) != 0)
        {
            {
                omniORB::logger log;
                log << "ZMQ: Event name" << '\n';
            }
            omni::giopStream::dumpbuf((unsigned char *) name_mess.data(), name_mess.size());

            {
                omniORB::logger log;
                log << "ZMQ: Endianness" << '\n';
            }
            omni::giopStream::dumpbuf((unsigned char *) endian_mess.data(), endian_mess.size());

            {
                omniORB::logger log;
                log << "ZMQ: Call info" << '\n';
            }
            omni::giopStream::dumpbuf((unsigned char *) event_call_mess.data(), event_call_mess.size());

            {
                omniORB::logger log;
                log << "ZMQ: Event data" << '\n';
            }
            omni::giopStream::dumpbuf((unsigned char *) data_mess.data(), data_mess.size());
        }

        //
        // Get publisher socket (multicast case)
        //

        int send_nb = 1;
        zmq::socket_t *pub;
        pub = event_pub_sock;

        zmq::message_t *name_mess_ptr = &name_mess;
        zmq::message_t *endian_mess_ptr = &endian_mess;
        zmq::message_t *event_call_mess_ptr = &event_call_mess;
        zmq::message_t *data_mess_ptr = &data_mess;

        std::map<std::string, McastSocketPub>::iterator mcast_ite;
        auto mcast_ite_end = event_mcast.end();

        int local_double_send = double_send;
        bool mcast_event = false;

        if(!event_mcast.empty())
        {
            if((mcast_ite = event_mcast.find(event_name)) != mcast_ite_end)
            {
                if(!mcast_ite->second.local_client)
                {
                    pub = mcast_ite->second.pub_socket;
                }
                else
                {
                    if(mcast_ite->second.pub_socket != nullptr)
                    {
                        send_nb = 2;
                        pub = mcast_ite->second.pub_socket;
                    }
                }
                if(mcast_ite->second.double_send)
                {
                    local_double_send++;
                }
                mcast_ite->second.double_send = false;
                mcast_event = true;
            }
        }

        if(local_double_send > 0)
        {
            send_nb = 2;
            if(!mcast_event)
            {
                double_send--;
            }
        }

        //
        // If we have a multicast socket with also a local client we are obliged to send two times the messages.
        // ZMQ does not support local client with PGM socket
        //

        zmq::message_t name_mess_cpy;
        zmq::message_t event_call_mess_cpy;
        zmq::message_t data_mess_cpy;

        if(send_nb == 2)
        {
            name_mess_cpy.copy(name_mess);
            event_call_mess_cpy.copy(event_call_mess);
            data_mess_cpy.copy(data_mess);
        }

        while(send_nb > 0)
        {
            //
            // Push the event
            //

            auto ret = pub->send(*name_mess_ptr, zmq::send_flags::sndmore);
            if(!ret)
            {
                std::cerr << "Name message returned false, assertion!!!!" << std::endl;
                TANGO_ASSERT(false);
            }

            ret = pub->send(*endian_mess_ptr, zmq::send_flags::sndmore);
            if(!ret)
            {
                std::cerr << "Endian message returned false, assertion!!!!" << std::endl;
                TANGO_ASSERT(false);
            }
            endian_mess_sent = true;

            ret = pub->send(*event_call_mess_ptr, zmq::send_flags::sndmore);
            if(!ret)
            {
                std::cerr << "Call message returned false, assertion!!!!" << std::endl;
                TANGO_ASSERT(false);
            }

            ret = pub->send(*data_mess_ptr, zmq::send_flags::none);
            if(!ret)
            {
                std::cerr << "Data message returned false, assertion!!!!" << std::endl;
                TANGO_ASSERT(false);
            }

            send_nb--;
            if(send_nb == 1)
            {
                if((!event_mcast.empty()) && (mcast_ite != mcast_ite_end))
                {
                    //
                    // Case of multicast socket with a local client. Send the event also on the local socket
                    //

                    if(mcast_ite->second.local_client)
                    {
                        zmq::socket_t *old_pub = pub;
                        pub = event_pub_sock;

                        name_mess.copy(name_mess_cpy);
                        endian_mess.copy(endian_mess_2);
                        event_call_mess.copy(event_call_mess_cpy);
                        data_mess.copy(data_mess_cpy);

                        pub->send(*name_mess_ptr, zmq::send_flags::sndmore);
                        pub->send(*endian_mess_ptr, zmq::send_flags::sndmore);
                        endian_mess_sent = true;
                        pub->send(*event_call_mess_ptr, zmq::send_flags::sndmore);
                        pub->send(*data_mess_ptr, zmq::send_flags::none);

                        pub = old_pub;

                        name_mess_ptr = &name_mess_cpy;
                        endian_mess.copy(endian_mess_2);
                        event_call_mess_ptr = &event_call_mess_cpy;
                        data_mess_ptr = &data_mess_cpy;
                    }
                    else
                    {
                        name_mess_ptr = &name_mess_cpy;
                        endian_mess.copy(endian_mess_2);
                        event_call_mess_ptr = &event_call_mess_cpy;
                        data_mess_ptr = &data_mess_cpy;
                    }
                }

                name_mess_ptr = &name_mess_cpy;
                endian_mess.copy(endian_mess_2);
                event_call_mess_ptr = &event_call_mess_cpy;
                data_mess_ptr = &data_mess_cpy;
            }
        }

        //
        // Increment event counter if required
        //

        if(ev_cptr_ite != event_cptr.end() && inc_cptr)
        {
            ev_cptr_ite->second++;
        }

        //
        // For reference counting on zmq messages which do not have a local scope
        //

        endian_mess.copy(endian_mess_2);

        //
        // wait for data to be acyually sent (ZMQ no copy mode)
        //
        if(large_data)
        {
            large_message_future.wait();
        }
    }
    catch(...)
    {
        TANGO_LOG_DEBUG << "ZmqEventSupplier::push_event() failed !!!!!!!!!!!\n";
        if(endian_mess_sent)
        {
            endian_mess.copy(endian_mess_2);
        }

        TangoSys_OMemStream o;
        o << "Can't push ZMQ event for event ";
        o << event_name;
        if(zmq_errno() != 0)
        {
            o << "\nZmq error: " << zmq_strerror(zmq_errno()) << std::ends;
        }
        else
        {
            o << std::ends;
        }

        TANGO_THROW_EXCEPTION(API_ZmqFailed, o.str());
    }
}

std::string ZmqEventSupplier::create_full_event_name(DeviceImpl *device_impl,
                                                     const std::string &event_type,
                                                     const std::string &obj_name_lower,
                                                     bool intr_change)
{
    std::string full_event_name = fqdn_prefix;

    int size = full_event_name.size();
    if(full_event_name[size - 1] == '#')
    {
        full_event_name.erase(size - 1);
    }

    full_event_name = full_event_name + device_impl->get_name_lower();
    if(!intr_change)
    {
        full_event_name = full_event_name + '/' + obj_name_lower;
    }
    if(Util::instance()->use_file_db() || !Util::instance()->use_db())
    {
        full_event_name = full_event_name + MODIFIER_DBASE_NO;
    }
    full_event_name = full_event_name + '.';
    full_event_name = full_event_name + event_type;

    return full_event_name;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::update_connected_client
//
// description :
//
//-------------------------------------------------------------------------------------------------------------------

bool ZmqEventSupplier::update_connected_client(client_addr *cl)
{
    bool ret = false;

    //
    // Immediately return if client identification not possible. (Very old client....)
    //

    if(cl == nullptr)
    {
        return ret;
    }

    //
    // First try to find the client in list
    //

    auto pos =
        find_if(con_client.begin(), con_client.end(), [&](ConnectedClient &cc) -> bool { return (cc.clnt == *cl); });

    //
    // Update date if client in list. Otherwise add client to list
    //

    auto now = std::chrono::steady_clock::now();

    if(pos != con_client.end())
    {
        pos->date = now;
    }
    else
    {
        ConnectedClient new_cc;
        new_cc.clnt = *cl;
        new_cc.date = now;

        con_client.push_back(new_cc);
        ret = true;
    }

    //
    // Remove presumly dead client
    //

    con_client.remove_if([&](ConnectedClient &cc) { return (now - cc.date) > std::chrono::seconds(500); });

    return ret;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::push_event_loop()
//
// description :
//        Method to send the event in a loop due to possible different client release (event compatibility)
//
// argument :
//        in :
//            - device_impl : The device
//            - event_type : The event type (change, periodic....)
//            - filterable_names :
//            - filterable_data :
//            - attr_value : The attribute value
//            - att : The attribute object reference
//            - except : The exception thrown during the last attribute reading. nullptr if no exception
//
//-------------------------------------------------------------------------------------------------------------------

void ZmqEventSupplier::push_event_loop(DeviceImpl *device_impl,
                                       EventType event_type,
                                       TANGO_UNUSED(const std::vector<std::string> &filterable_names),
                                       TANGO_UNUSED(const std::vector<double> &filterable_data),
                                       TANGO_UNUSED(const std::vector<std::string> &filterable_names_lg),
                                       TANGO_UNUSED(const std::vector<long> &filterable_data_lg),
                                       const struct SuppliedEventData &attr_value,
                                       Attribute &att,
                                       DevFailed *except)
{
    TANGO_LOG_DEBUG << "ZmqEventSupplier::push_event_loop(): called for attribute " << att.get_name() << std::endl;

    std::vector<int> &client_libs = att.get_client_lib(event_type);
    std::vector<int>::iterator ite;
    std::string ev_name = EventName[event_type];
    bool inc_ctr = true;

    for(ite = client_libs.begin(); ite != client_libs.end(); ++ite)
    {
        bool need_free = false;
        bool name_changed = false;

        struct SuppliedEventData sent_value;
        ::memset(&sent_value, 0, sizeof(sent_value));

        // We only need the IDL5 compatibility prefix for event types that
        // existed before IDL5 was released, i.e. not for ALARM_EVENT.

        if(*ite >= 5 && event_type != ALARM_EVENT)
        {
            ev_name = detail::add_idl_prefix(ev_name);
            name_changed = true;
        }

        if(except == nullptr)
        {
            switch(*ite)
            {
            case 6:
            case 5:
            {
                convert_att_event_to_5(attr_value, sent_value, need_free, att);
            }
            break;

            case 4:
            {
                convert_att_event_to_4(attr_value, sent_value, need_free, att);
            }
            break;

            default:
            {
                convert_att_event_to_3(attr_value, sent_value, need_free, att);
            }
            break;
            }
        }

        push_event(device_impl,
                   ev_name,
                   filterable_names,
                   filterable_data,
                   filterable_names_lg,
                   filterable_data_lg,
                   sent_value,
                   att.get_name_lower(),
                   except,
                   inc_ctr);

        inc_ctr = false;
        if(need_free)
        {
            if(sent_value.attr_val_5 != nullptr)
            {
                delete sent_value.attr_val_5;
            }
            else if(sent_value.attr_val_4 != nullptr)
            {
                delete sent_value.attr_val_4;
            }
            else if(sent_value.attr_val_3 != nullptr)
            {
                delete sent_value.attr_val_3;
            }
            else
            {
                delete sent_value.attr_val;
            }
        }
        if(name_changed)
        {
            ev_name = EventName[event_type];
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::get_blob_data_elt()
//
// description :
//        Get how many data are transferred in the blob given as parameter. For instance, for a blob transporting
//        one array of 1000 double, the returned value will be 1000
//
// argument :
//        in :
//            - dvpdea : The sequence of data element in the blob
//
// return :
//        Size (in number of data) of the data blob
//
//-------------------------------------------------------------------------------------------------------------------

size_t ZmqEventSupplier::get_blob_data_nb(DevVarPipeDataEltArray &dvpdea)
{
    size_t ret = 0;

    size_t nb_blob = dvpdea.length();
    for(size_t loop = 0; loop < nb_blob; loop++)
    {
        ret = ret + get_data_elt_data_nb(dvpdea[loop]);
    }

    return ret;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        ZmqEventSupplier::get_data_elt_data_nb()
//
// description :
//        Get how many data are transferred in the data element given as parameter. For instance, for a data element
//        transporting one array of 250 floats, the returned value will be 250
//
// argument :
//        in :
//            - dvde : The data element
//
// return :
//        Size (in number of data) of the data element
//
//-------------------------------------------------------------------------------------------------------------------

size_t ZmqEventSupplier::get_data_elt_data_nb(DevPipeDataElt &dvde)
{
    size_t ret = 0;

    if(dvde.inner_blob.length() != 0)
    {
        ret = get_blob_data_nb(dvde.inner_blob);
    }
    else
    {
        switch(dvde.value._d())
        {
        case ATT_BOOL:
        {
            const DevVarBooleanArray &dvba = dvde.value.bool_att_value();
            ret = dvba.length();
        }
        break;

        case ATT_SHORT:
        {
            const DevVarShortArray &dvsa = dvde.value.short_att_value();
            ret = dvsa.length();
        }
        break;

        case ATT_LONG:
        {
            const DevVarLongArray &dvla = dvde.value.long_att_value();
            ret = dvla.length();
        }
        break;

        case ATT_LONG64:
        {
            const DevVarLong64Array &dvlo64 = dvde.value.long64_att_value();
            ret = dvlo64.length();
        }
        break;

        case ATT_FLOAT:
        {
            const DevVarFloatArray &dvfa = dvde.value.float_att_value();
            ret = dvfa.length();
        }
        break;

        case ATT_DOUBLE:
        {
            const DevVarDoubleArray &dvda = dvde.value.double_att_value();
            ret = dvda.length();
        }
        break;

        case ATT_UCHAR:
        {
            const DevVarCharArray &dvda = dvde.value.uchar_att_value();
            ret = dvda.length();
        }
        break;

        case ATT_USHORT:
        {
            const DevVarUShortArray &dvda = dvde.value.ushort_att_value();
            ret = dvda.length();
        }
        break;

        case ATT_ULONG:
        {
            const DevVarULongArray &dvda = dvde.value.ulong_att_value();
            ret = dvda.length();
        }
        break;

        case ATT_ULONG64:
        {
            const DevVarULong64Array &dvda = dvde.value.ulong64_att_value();
            ret = dvda.length();
        }
        break;

        case ATT_STRING:
        {
            const DevVarStringArray &dvda = dvde.value.string_att_value();
            ret = dvda.length();
        }
        break;

        case ATT_STATE:
        {
            const DevVarStateArray &dvda = dvde.value.state_att_value();
            ret = dvda.length();
        }
        break;

        case ATT_ENCODED:
        {
            const DevVarEncodedArray &dvda = dvde.value.encoded_att_value();
            ret = dvda[0].encoded_data.length();
        }
        break;

        case DEVICE_STATE:
            [[fallthrough]];
        case ATT_NO_DATA:
            TANGO_THROW_EXCEPTION(API_PipeWrongArg,
                                  "Unsupported data type in data element! (ATT_NO_DATA, DEVICE_STATE)");
            break;

        default:
            TANGO_ASSERT_ON_DEFAULT(dvde.value._d());
        }
    }

    return ret;
}

} // namespace Tango
