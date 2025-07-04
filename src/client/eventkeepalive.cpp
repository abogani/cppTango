//=====================================================================================================================
//
// file :                event.cpp
//
// description :        C++ classes for implementing the event system KeepAliveThread. This class cheks that the
//                        heartbeat event are well received. It also manages the stateless subscription because this
//                        is this class which will regularely re-try to subsribe to event in case it is needed.
//                        Finally, it is also this class which generates the re-connection in case a device server
//                        sending events is stopped and re-started
//
// author(s) :             A.Gotz (goetz@esrf.fr)
//
// original :             7 April 2003
//
// Copyright (C) :      2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
//====================================================================================================================

#include <tango/client/eventconsumer.h>
#include <tango/server/auto_tango_monitor.h>
#include <tango/client/event.h>

#include <tango/common/pointer_with_lock.h>

#include <tango/internal/utils.h>

#include <cstdio>

#ifdef _TG_WINDOWS_
  #include <process.h>
#else
  #include <unistd.h>
#endif

using namespace CORBA;

namespace Tango
{

/************************************************************************/
/*                                                                           */
/*             EventConsumerKeepAlive class                                 */
/*            ----------------------------                                */
/*                                                                           */
/************************************************************************/

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::reconnect_to_channel()
//
// description :
//        Method to reconnect the process to an event channel in case of reconnection to a notifd
//
// argument :
//        in :
//            - ipos : An iterator to the EventChannel structure to reconnect to in the Event Channel map
//            - event_consumer : Pointer to the EventConsumer singleton
//
// return :
//         This method returns true if the reconnection succeeds. Otherwise, returns false
//
//--------------------------------------------------------------------------------------------------------------------

bool EventConsumerKeepAliveThread::reconnect_to_channel(const EvChanIte &ipos,
                                                        PointerWithLock<EventConsumer> &event_consumer)
{
    bool ret = true;
    EvCbIte epos;

    TANGO_LOG_DEBUG << "Entering KeepAliveThread::reconnect()" << std::endl;

    for(epos = event_consumer->event_callback_map.begin(); epos != event_consumer->event_callback_map.end(); ++epos)
    {
        if(epos->second.channel_name == ipos->first)
        {
            bool need_reconnect = false;
            std::vector<EventSubscribeStruct>::iterator esspos;
            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                if(esspos->callback != nullptr || esspos->ev_queue != nullptr)
                {
                    need_reconnect = true;
                    break;
                }
            }

            if(need_reconnect)
            {
                try
                {
                    DeviceData dummy;
                    std::string adm_name = ipos->second.full_adm_name;
                    event_consumer->connect_event_channel(
                        adm_name, epos->second.get_device_proxy().get_device_db(), true, dummy);

                    ipos->second.adm_device_proxy = std::make_shared<DeviceProxy>(ipos->second.full_adm_name);
                    TANGO_LOG_DEBUG << "Reconnected to event channel" << std::endl;
                }
                catch(...)
                {
                    ret = false;
                }

                break;
            }
        }
    }

    return ret;
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::reconnect_to_zmq_channel()
//
// description :
//        Method to reconnect the process to a ZMQ event channel in case of reconnection
//
// argument :
//        in :
//            - ipos : An iterator to the EventChannel structure to reconnect to in the Event Channel map
//            - event_consumer : Pointer to the EventConsumer singleton
//            - dd :
//
// return :
//         This method returns true if the reconnection succeeds. Otherwise, returns false
//
//---------------------------------------------------------------------------------------------------------------------

bool EventConsumerKeepAliveThread::reconnect_to_zmq_channel(const EvChanIte &ipos,
                                                            PointerWithLock<EventConsumer> &event_consumer,
                                                            DeviceData &dd)
{
    TANGO_LOG_DEBUG << "Entering KeepAliveThread::reconnect_to_zmq_channel()" << std::endl;

    for(auto epos = event_consumer->event_callback_map.begin(); epos != event_consumer->event_callback_map.end();
        ++epos)
    {
        if(epos->second.channel_name == ipos->first)
        {
            bool need_reconnect = false;
            std::vector<EventSubscribeStruct>::iterator esspos;
            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                if(esspos->callback != nullptr || esspos->ev_queue != nullptr)
                {
                    need_reconnect = true;
                    break;
                }
            }

            if(need_reconnect)
            {
                try
                {
                    // Admin name might have changed while the event system was down.
                    std::string old_adm_name = ipos->second.full_adm_name;
                    std::string new_adm_name = old_adm_name;
                    try
                    {
                        new_adm_name = epos->second.get_device_proxy().adm_name();
                    }
                    catch(const DevFailed &)
                    {
                        // Here we silently ignore the issue but most likely
                        // the ZmqEventSubscriptionChange command will fail,
                        // or we will be unable to create admin DeviceProxy.
                    }

                    ipos->second.adm_device_proxy = std::make_shared<DeviceProxy>(new_adm_name);

                    DeviceData subscriber_in, subscriber_out;
                    std::vector<std::string> subscriber_info;
                    subscriber_info.push_back(epos->second.get_device_proxy().dev_name());
                    subscriber_info.push_back(epos->second.obj_name);
                    subscriber_info.emplace_back("subscribe");
                    subscriber_info.push_back(epos->second.event_name);
                    subscriber_info.emplace_back("0");

                    subscriber_in << subscriber_info;

                    subscriber_out =
                        ipos->second.adm_device_proxy->command_inout("ZmqEventSubscriptionChange", subscriber_in);

                    // Calculate new event channel name.
                    // This must be done using the initialize_received_from_admin
                    // function in order to support older (< 9.3) Tango versions.
                    const DevVarLongStringArray *event_sub_change_result;
                    subscriber_out >> event_sub_change_result;
                    std::string local_callback_key; // We are not interested in event name, pass dummy value.
                    auto event_and_channel_name = event_consumer->initialize_received_from_admin(
                        event_sub_change_result,
                        local_callback_key,
                        new_adm_name,
                        epos->second.get_device_proxy().get_from_env_var());

                    ipos->second.full_adm_name = event_and_channel_name.channel_name;

                    //
                    // Forget exception which could happen during massive restart of device server process running on
                    // the same host
                    //
                    try
                    {
                        event_consumer->disconnect_event_channel(
                            old_adm_name, ipos->second.endpoint, epos->second.endpoint);
                    }
                    catch(Tango::DevFailed &)
                    {
                    }
                    // old_adm_name is correct here as the renaming happens at the end of
                    // EventConsumerKeepAliveThread::run_undetached()
                    event_consumer->connect_event_channel(
                        old_adm_name, epos->second.get_device_proxy().get_device_db(), true, subscriber_out);

                    dd = subscriber_out;

                    TANGO_LOG_DEBUG << "Reconnected to zmq event channel" << std::endl;
                }
                catch(...)
                {
                    TANGO_LOG_DEBUG << "Failed to reconnect to zmq event channel" << std::endl;

                    return false;
                }

                break;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::reconnect_to_event()
//
// description :
//        Method to reconnect each event associated to a specific event channel to the just reconnected event channel
//
// argument :
//        in :
//            - ipos : An iterator to the EventChannel structure in the Event Channel map
//            - event_consumer : Pointer to the EventConsumer singleton
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::reconnect_to_event(const EvChanIte &ipos,
                                                      PointerWithLock<EventConsumer> &event_consumer)
{
    EvCbIte epos;

    TANGO_LOG_DEBUG << "Entering KeepAliveThread::reconnect_to_event()" << std::endl;

    for(epos = event_consumer->event_callback_map.begin(); epos != event_consumer->event_callback_map.end(); ++epos)
    {
        if(epos->second.channel_name == ipos->first)
        {
            bool need_reconnect = false;
            std::vector<EventSubscribeStruct>::iterator esspos;
            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                if(esspos->callback != nullptr || esspos->ev_queue != nullptr)
                {
                    need_reconnect = true;
                    break;
                }
            }

            if(need_reconnect)
            {
                try
                {
                    epos->second.callback_monitor->get_monitor();

                    try
                    {
                        re_subscribe_event(epos, ipos);
                        epos->second.filter_ok = true;
                        TANGO_LOG_DEBUG << "Reconnected to event" << std::endl;
                    }
                    catch(...)
                    {
                        epos->second.filter_ok = false;
                    }

                    epos->second.callback_monitor->rel_monitor();
                }
                catch(...)
                {
                    ApiUtil *au = ApiUtil::instance();
                    std::stringstream ss;

                    ss << "EventConsumerKeepAliveThread::reconnect_to_event() cannot get callback monitor for "
                       << epos->first;
                    au->print_error_message(ss.str().c_str());
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::re_subscribe_event()
//
// description :
//        Method to reconnect a specific event to an event channel just reconnected
//
// argument :
//        in :
//            - epos : An iterator to the EventCallback structure in the Event Callback map
//            - ipos : Pointer to the EventChannel structure in the Event Channel map
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::re_subscribe_event(const EvCbIte &epos, const EvChanIte &ipos)
{
    //
    // Build a filter using the CORBA Notify constraint Language (use attribute name in lowercase letters)
    //

    CosNotifyFilter::FilterFactory_var ffp;
    CosNotifyFilter::Filter_var filter = CosNotifyFilter::Filter::_nil();
    CosNotifyFilter::FilterID filter_id;

    try
    {
        ffp = ipos->second.eventChannel->default_filter_factory();
        filter = ffp->create_filter("EXTENDED_TCL");
    }
    catch(CORBA::COMM_FAILURE &)
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_NotificationServiceFailed,
            "Caught CORBA::COMM_FAILURE exception while creating event filter (check filter)");
    }
    catch(...)
    {
        TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept,
                                       API_NotificationServiceFailed,
                                       "Caught exception while creating event filter (check filter)");
    }

    //
    // Construct a simple constraint expression; add it to fadmin
    //

    std::string constraint_expr = epos->second.filter_constraint;

    CosNotification::EventTypeSeq evs;
    CosNotifyFilter::ConstraintExpSeq exp;
    exp.length(1);
    exp[0].event_types = evs;
    exp[0].constraint_expr = Tango::string_dup(constraint_expr.c_str());
    bool error_occurred = false;
    try
    {
        CosNotifyFilter::ConstraintInfoSeq_var dummy = filter->add_constraints(exp);

        filter_id = ipos->second.structuredProxyPushSupplier->add_filter(filter);

        epos->second.filter_id = filter_id;
    }
    catch(CosNotifyFilter::InvalidConstraint &)
    {
        // cerr << "Exception thrown : Invalid constraint given "
        //      << (const char *)constraint_expr << std::endl;
        error_occurred = true;
    }
    catch(...)
    {
        // cerr << "Exception thrown while adding constraint "
        //      << (const char *)constraint_expr << std::endl;
        error_occurred = true;
    }

    //
    // If error, destroy filter
    //

    if(error_occurred)
    {
        try
        {
            filter->destroy();
        }
        catch(...)
        {
        }

        filter = CosNotifyFilter::Filter::_nil();
        TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept,
                                       API_NotificationServiceFailed,
                                       "Caught exception while creating event filter (check filter)");
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::reconnect_to_zmq_event()
//
// description :
//        Method to reconnect each event associated to a specific event channel to the just reconnected event channel
//
// argument :
//        in :
//            - ipos : An iterator to the EventChannel structure in the Event Channel map
//            - event_consumer : Pointer to the EventConsumer singleton
//            - dd :
//
//-------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::reconnect_to_zmq_event(const EvChanIte &ipos,
                                                          PointerWithLock<EventConsumer> &event_consumer,
                                                          DeviceData &dd)
{
    EvCbIte epos;
    bool disconnect_called = false;

    TANGO_LOG_DEBUG << "Entering KeepAliveThread::reconnect_to_zmq_event()" << std::endl;

    for(epos = event_consumer->event_callback_map.begin(); epos != event_consumer->event_callback_map.end(); ++epos)
    {
        // Here ipos->first still points to the old channel name (before reconnection).
        if(epos->second.channel_name == ipos->first)
        {
            // Admin name might have changed while the event system was down.
            epos->second.channel_name = ipos->second.full_adm_name;
            epos->second.received_from_admin.channel_name = ipos->second.full_adm_name;

            bool need_reconnect = false;
            std::vector<EventSubscribeStruct>::iterator esspos;
            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                if(esspos->callback != nullptr || esspos->ev_queue != nullptr)
                {
                    need_reconnect = true;
                    break;
                }
            }

            if(need_reconnect)
            {
                try
                {
                    epos->second.callback_monitor->get_monitor();

                    try
                    {
                        EventCallBackStruct ecbs;
                        std::vector<std::string> vs;

                        vs.emplace_back("reconnect");

                        std::string d_name = epos->second.get_device_proxy().dev_name();
                        std::string &fqen = epos->second.fully_qualified_event_name;
                        std::string::size_type pos = fqen.find('/');
                        pos = pos + 2;
                        pos = fqen.find('/', pos);
                        std::string prefix = fqen.substr(0, pos + 1);
                        d_name.insert(0, prefix);

                        if(!disconnect_called)
                        {
                            event_consumer->disconnect_event(epos->second.fully_qualified_event_name,
                                                             epos->second.endpoint);
                            disconnect_called = true;
                        }
                        event_consumer->connect_event_system(d_name,
                                                             epos->second.obj_name,
                                                             epos->second.event_name,
                                                             vs,
                                                             ipos,
                                                             epos->second,
                                                             dd,
                                                             ipos->second.valid_endpoint);

                        const DevVarLongStringArray *dvlsa;
                        dd >> dvlsa;
                        epos->second.endpoint = dvlsa->svalue[(ipos->second.valid_endpoint << 1) + 1].in();

                        TANGO_LOG_DEBUG << "Reconnected to ZMQ event" << std::endl;
                    }
                    catch(...)
                    {
                        epos->second.filter_ok = false;
                    }

                    epos->second.callback_monitor->rel_monitor();
                }
                catch(...)
                {
                    ApiUtil *au = ApiUtil::instance();
                    std::stringstream ss;

                    ss << "EventConsumerKeepAliveThread::reconnect_to_zmq_event() cannot get callback monitor for "
                       << epos->first;
                    au->print_error_message(ss.str().c_str());
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::run_undetached
//
// description :
//        The main code of the KeepAliveThread
//
//-------------------------------------------------------------------------------------------------------------------

void *EventConsumerKeepAliveThread::run_undetached(TANGO_UNUSED(void *arg))
{
    int time_to_sleep;
    time_t now;

    bool exit_th = false;

    while(!exit_th)
    {
        time_to_sleep = EVENT_HEARTBEAT_PERIOD;

        //
        // Go to sleep until next heartbeat. Wait on a monitor. This allows another thread to wake-up this thread
        // before the end of the EVENT_HEARTBEAT_PERIOD time which is 10 seconds. Only one command can now be send to
        // the thread. It is a stop command
        //

        {
            omni_mutex_lock sync(shared_cmd);
            if(!shared_cmd.cmd_pending)
            {
                unsigned long s, n;

                unsigned long nb_sec, nb_nanos;
                nb_sec = time_to_sleep;
                nb_nanos = 0;

                omni_thread::get_time(&s, &n, nb_sec, nb_nanos);
                shared_cmd.cond.timedwait(s, n);
            }
            if(shared_cmd.cmd_pending)
            {
                exit_th = true;
                return (void *) nullptr;
            }
        }

        //
        // Re-subscribe
        //

        TANGO_LOG_DEBUG << "KeepAliveThread at work" << std::endl;

        auto event_consumer = ApiUtil::instance()->get_zmq_event_consumer();
        auto notifd_event_consumer = ApiUtil::instance()->get_notifd_event_consumer();

        now = Tango::get_current_system_datetime();
        if(!event_consumer->event_not_connected.empty())
        {
            DelayEvent de(event_consumer);
            event_consumer->map_modification_lock.writerIn();

            //
            // Check the list of not yet connected events and try to subscribe
            //

            not_conected_event(event_consumer, now, notifd_event_consumer);

            event_consumer->map_modification_lock.writerOut();
        }

        //
        // Check for all other event reconnections
        //

        std::vector<EvChanIte> renamed_channels{};

        {
            // lock the maps only for reading
            ReaderLock r(event_consumer->map_modification_lock);

            EvChanIte ipos;
            EvCbIte epos;

            renamed_channels.reserve(event_consumer->channel_map.size());

            for(ipos = event_consumer->channel_map.begin(); ipos != event_consumer->channel_map.end(); ++ipos)
            {
                try
                {
                    // lock the event channel
                    ipos->second.channel_monitor->get_monitor();

                    //
                    // Check if it is necessary for client to confirm its subscription. Note that starting with
                    // Tango 8.1 (and for ZMQ), there is a new command in the admin device which allows a better
                    // (optimized) confirmation algorithm
                    //

                    if((now - ipos->second.last_subscribed) > EVENT_RESUBSCRIBE_PERIOD / 3)
                    {
                        confirm_subscription(event_consumer, ipos);
                    }

                    //
                    // Check if a heartbeat have been skipped. If a heartbeat is missing, there are four possibilities :
                    // 1 - The notifd is dead (or the crate is rebooting or has already reboot)
                    // 2 - The server is dead
                    // 3 - The network was down;
                    // 4 - The server has been restarted on another host.
                    //

                    bool heartbeat_skipped;
                    heartbeat_skipped = ((now - ipos->second.last_heartbeat) >= EVENT_HEARTBEAT_PERIOD);

                    if(heartbeat_skipped || ipos->second.heartbeat_skipped || ipos->second.event_system_failed)
                    {
                        ipos->second.heartbeat_skipped = true;
                        main_reconnect(event_consumer, notifd_event_consumer, epos, ipos);
                        if(ipos->first != ipos->second.full_adm_name)
                        {
                            // Channel name has changed after reconnection.
                            // Store the iterator and update the map later.
                            renamed_channels.push_back(ipos);
                        }
                    }
                    else
                    {
                        // When the heartbeat has worked, mark the connection to the notifd as OK
                        if(ipos->second.channel_type == NOTIFD)
                        {
                            ipos->second.has_notifd_closed_the_connection = 0;
                        }
                    }

                    // release channel monitor
                    ipos->second.channel_monitor->rel_monitor();
                }
                catch(...)
                {
                    ApiUtil *au = ApiUtil::instance();
                    std::stringstream ss;

                    ss << "EventConsumerKeepAliveThread::run_undetached() timeout on callback monitor of "
                       << epos->first;
                    au->print_error_message(ss.str().c_str());
                }
            }
        }

        {
            // Move entries for renamed channels. This is done outside of the reconnection loop to avoid
            // reconnecting to the same channel twice in case it would be inserted past ipos iterator.

            WriterLock writer_lock(event_consumer->map_modification_lock);
            for(const auto &channel : renamed_channels)
            {
                for(auto &device : event_consumer->device_channel_map)
                {
                    if(device.second == channel->first)
                    {
                        device.second = channel->second.full_adm_name;
                    }
                }

                AutoTangoMonitor monitor_lock(channel->second.channel_monitor);
                event_consumer->channel_map.emplace(channel->second.full_adm_name, channel->second);
                event_consumer->channel_map.erase(channel);
            }
        }
    }

    //
    // If we arrive here, this means that we have received the exit thread command.
    //

    return (void *) nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::not_connected_event
//
// description :
//        Try to connect not yet connected event(s). Try first with ZMQ event consumer.
//        If ZMQ is not used (API_CommandNotFound exception), use notifd.
//
// argument :
//        in :
//            - event_consumer : The ZMQ event consumer object
//            - now : The date
//            - notifd_event_consumer : The notifd event consumer object
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::not_conected_event(PointerWithLock<EventConsumer> &event_consumer,
                                                      time_t now,
                                                      PointerWithLock<EventConsumer> &)
{
    if(!event_consumer->event_not_connected.empty())
    {
        std::vector<EventNotConnected>::iterator vpos;
        for(vpos = event_consumer->event_not_connected.begin(); vpos != event_consumer->event_not_connected.end();
            /*vpos++*/)
        {
            bool inc_vpos = true;

            //
            // check wether it is necessary to try to subscribe again!
            //

            if((now - vpos->last_heartbeat) >= (EVENT_HEARTBEAT_PERIOD - 1))
            {
                try
                {
                    // try to subscribe
                    event_consumer->connect_event(vpos->device,
                                                  vpos->attribute,
                                                  vpos->event_type,
                                                  vpos->callback,
                                                  vpos->ev_queue,
                                                  vpos->filters,
                                                  vpos->event_name,
                                                  vpos->event_id);

                    //
                    // delete element from vector when subscribe worked
                    //

                    vpos = event_consumer->event_not_connected.erase(vpos);
                    inc_vpos = false;
                }

                catch(Tango::DevFailed &e)
                {
                    std::string reason(e.errors[0].reason.in());
                    if(reason == API_CommandNotFound)
                    {
                        try
                        {
                            auto notifd_consumer = ApiUtil::instance()->create_notifd_event_consumer();
                            notifd_consumer->connect_event(vpos->device,
                                                           vpos->attribute,
                                                           vpos->event_type,
                                                           vpos->callback,
                                                           vpos->ev_queue,
                                                           vpos->filters,
                                                           vpos->event_name,
                                                           vpos->event_id);

                            //
                            // delete element from vector when subscribe worked
                            //

                            vpos = event_consumer->event_not_connected.erase(vpos);
                            inc_vpos = false;
                        }
                        catch(Tango::DevFailed &e)
                        {
                            stateless_subscription_failed(vpos, e, now);
                        }
                    }
                    else
                    {
                        stateless_subscription_failed(vpos, e, now);
                    }
                }
                catch(...)
                {
                    //
                    // subscribe has not worked, try again in the next hearbeat period
                    //

                    vpos->last_heartbeat = now;

                    ApiUtil *au = ApiUtil::instance();
                    au->print_error_message("During the event subscription an exception was sent which is not a "
                                            "Tango::DevFailed exception!");
                }
            }
            if(inc_vpos)
            {
                ++vpos;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::fwd_not_connected_event
//
// description :
//        Try to connect not yet connected event(s). This method is called only in case of forwarded attribute with
//      root attribute inside the same process than the fwd attribute!
//
// argument :
//        in :
//            - event_consumer : The ZMQ event consumer object
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::fwd_not_conected_event(PointerWithLock<EventConsumer> &event_consumer)
{
    //
    // lock the maps only for reading
    //

    event_consumer->map_modification_lock.writerIn();

    if(!event_consumer->event_not_connected.empty())
    {
        const time_t now = Tango::get_current_system_datetime();
        std::vector<EventNotConnected>::iterator vpos;
        for(vpos = event_consumer->event_not_connected.begin(); vpos != event_consumer->event_not_connected.end();
            /*vpos++*/)
        {
            bool inc_vpos = true;

            //
            // check wether it is necessary to try to subscribe again!
            //

            try
            {
                // try to subscribe

                event_consumer->connect_event(vpos->device,
                                              vpos->attribute,
                                              vpos->event_type,
                                              vpos->callback,
                                              vpos->ev_queue,
                                              vpos->filters,
                                              vpos->event_name,
                                              vpos->event_id);

                //
                // delete element from vector when subscribe worked
                //

                vpos = event_consumer->event_not_connected.erase(vpos);
                inc_vpos = false;
            }
            catch(Tango::DevFailed &e)
            {
                stateless_subscription_failed(vpos, e, now);
            }
            catch(...)
            {
                //
                // subscribe has not worked, try again in the next hearbeat period
                //

                vpos->last_heartbeat = now;

                ApiUtil *au = ApiUtil::instance();
                au->print_error_message(
                    "During the event subscription an exception was sent which is not a Tango::DevFailed exception!");
            }

            if(inc_vpos)
            {
                ++vpos;
            }
        }
    }

    event_consumer->map_modification_lock.writerOut();
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::confirm_subscription
//
// description :
//        Confirm event subscription for all events coming from a specified event channel (Device server)
//
// argument :
//        in :
//            - event_consumer : The ZMQ event consumer object
//            - ipos : Iterator on the EventChannel map
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::confirm_subscription(PointerWithLock<EventConsumer> &event_consumer,
                                                        const std::map<std::string, EventChannelStruct>::iterator &ipos)
{
    std::vector<std::string> cmd_params;
    std::vector<std::map<std::string, EventCallBackStruct>::difference_type> vd;
    std::map<std::string, EventCallBackStruct>::iterator epos;

    for(epos = event_consumer->event_callback_map.begin(); epos != event_consumer->event_callback_map.end(); ++epos)
    {
        if(epos->second.channel_name == ipos->first)
        {
            try
            {
                //
                // lock the callback
                //

                epos->second.callback_monitor->get_monitor();

                if(ipos->second.channel_type == ZMQ)
                {
                    cmd_params.push_back(epos->second.get_device_proxy().dev_name());
                    cmd_params.push_back(epos->second.obj_name);
                    cmd_params.push_back(epos->second.event_name);

                    vd.push_back(distance(event_consumer->event_callback_map.begin(), epos));
                }
                else
                {
                    DeviceData subscriber_in;
                    std::vector<std::string> subscriber_info;
                    subscriber_info.push_back(epos->second.get_device_proxy().dev_name());
                    subscriber_info.push_back(epos->second.obj_name);
                    subscriber_info.emplace_back("subscribe");
                    subscriber_info.push_back(epos->second.event_name);
                    subscriber_in << subscriber_info;

                    ipos->second.adm_device_proxy->command_inout("EventSubscriptionChange", subscriber_in);

                    const time_t ti = Tango::get_current_system_datetime();
                    ipos->second.last_subscribed = ti;
                    epos->second.last_subscribed = ti;
                }
                epos->second.callback_monitor->rel_monitor();
            }
            catch(...)
            {
                epos->second.callback_monitor->rel_monitor();
            }
        }
    }

    if(ipos->second.channel_type == ZMQ && !cmd_params.empty())
    {
        try
        {
            DeviceData sub_cmd_in;
            sub_cmd_in << cmd_params;

            ipos->second.adm_device_proxy->command_inout("EventConfirmSubscription", sub_cmd_in);

            const time_t ti = Tango::get_current_system_datetime();
            ipos->second.last_subscribed = ti;
            for(unsigned int loop = 0; loop < vd.size(); ++loop)
            {
                epos = event_consumer->event_callback_map.begin();
                advance(epos, vd[loop]);

                epos->second.callback_monitor->get_monitor();
                epos->second.last_subscribed = ti;
                epos->second.callback_monitor->rel_monitor();
            }
        }
        catch(Tango::DevFailed &e)
        {
            std::string reason(e.errors[0].reason.in());
            if(reason == API_CommandNotFound)
            {
                //
                // We are connected to a Tango 8 server which do not implement the EventConfirmSubscription command
                // Send confirmation the old way
                //

                const time_t ti = Tango::get_current_system_datetime();
                ipos->second.last_subscribed = ti;

                for(unsigned int loop = 0; loop < vd.size(); ++loop)
                {
                    DeviceData subscriber_in;
                    std::vector<std::string> subscriber_info;
                    subscriber_info.push_back(cmd_params[(loop * 3)]);
                    subscriber_info.push_back(cmd_params[(loop * 3) + 1]);
                    subscriber_info.emplace_back("subscribe");
                    subscriber_info.push_back(cmd_params[(loop * 3) + 2]);
                    subscriber_info.emplace_back("0");
                    subscriber_in << subscriber_info;

                    try
                    {
                        ipos->second.adm_device_proxy->command_inout("ZmqEventSubscriptionChange", subscriber_in);
                    }
                    catch(...)
                    {
                    }

                    epos = event_consumer->event_callback_map.begin();
                    advance(epos, vd[loop]);

                    epos->second.callback_monitor->get_monitor();
                    epos->second.last_subscribed = ti;
                    epos->second.callback_monitor->rel_monitor();
                }
            }
        }
        catch(...)
        {
        }

        cmd_params.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::main_reconnect
//
// description :
//        Main method executed to send error to the user callback or to reconnect the event
//
// argument :
//        in :
//            - event_consumer : The ZMQ event consumer object
//            - notifd_event_consumer : The notifd event consumer object
//            - epos : Iterator on the EventCallback map
//            - ipos : Iterator on the EventChannel map
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::main_reconnect(PointerWithLock<EventConsumer> &event_consumer,
                                                  PointerWithLock<EventConsumer> &notifd_event_consumer,
                                                  std::map<std::string, EventCallBackStruct>::iterator &epos,
                                                  const std::map<std::string, EventChannelStruct>::iterator &ipos)
{
    //
    // First, try to reconnect
    //

    if(ipos->second.channel_type == NOTIFD)
    {
        //
        // Check notifd by trying to read an attribute of the event channel
        //

        try
        {
            //
            // Check if the device server is now running on a different host. In this case we have to reconnect to
            // another notification daemon.
            //
            DeviceInfo info;
            try
            {
                info = ipos->second.adm_device_proxy->info();
            }
            catch(Tango::DevFailed &)
            {
                // in case of failure, just stay connected to the actual notifd
                info.server_host = ipos->second.notifyd_host;
            }

            if(ipos->second.notifyd_host != info.server_host)
            {
                ipos->second.event_system_failed = true;
            }
            else
            {
                CosNotifyChannelAdmin::EventChannelFactory_var ecf = ipos->second.eventChannel->MyFactory();
                if(ipos->second.full_adm_name.find(MODIFIER_DBASE_NO) != std::string::npos)
                {
                    ipos->second.event_system_failed = true;
                }
            }
        }
        catch(...)
        {
            ipos->second.event_system_failed = true;
            TANGO_LOG_DEBUG << "Notifd is dead !!!" << std::endl;
        }

        //
        // if the connection to the notify daemon is marked as ok, the device server is working fine but
        // the heartbeat is still not coming back since three periods:
        // The notify deamon might have closed the connection, try to reconnect!
        //

        if(!ipos->second.event_system_failed && ipos->second.has_notifd_closed_the_connection >= 3)
        {
            ipos->second.event_system_failed = true;
        }

        //
        // Re-build connection to the event channel. This is a two steps process. First, reconnect to the new event
        // channel, then reconnect callbacks to this new event channel
        //

        if(ipos->second.event_system_failed)
        {
            bool notifd_reco = reconnect_to_channel(ipos, notifd_event_consumer);
            ipos->second.event_system_failed = !notifd_reco;

            if(!ipos->second.event_system_failed)
            {
                reconnect_to_event(ipos, notifd_event_consumer);
            }
        }
    }
    else
    {
        DeviceData dd;
        bool zmq_reco = reconnect_to_zmq_channel(ipos, event_consumer, dd);
        ipos->second.event_system_failed = !zmq_reco;

        if(!ipos->second.event_system_failed)
        {
            reconnect_to_zmq_event(ipos, event_consumer, dd);
        }
    }

    Tango::DevErrorList errors(1);

    errors.length(1);
    errors[0].severity = Tango::ERR;
    errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
    errors[0].reason = Tango::string_dup(API_EventTimeout);
    errors[0].desc =
        Tango::string_dup("Event channel is not responding anymore, maybe the server or event system is down");
    DeviceAttribute *dev_attr = nullptr;
    AttributeInfoEx *dev_attr_conf = nullptr;
    DevicePipe *dev_pipe = nullptr;

    for(epos = event_consumer->event_callback_map.begin(); epos != event_consumer->event_callback_map.end(); ++epos)
    {
        // Here ipos->first still points to the old channel name (before reconnection),
        // but epos->second.channel_name might have been updated (reconnect_to_zmq_event).
        // We must compare to ipos->second.full_adm_name.
        if(epos->second.channel_name == ipos->second.full_adm_name)
        {
            bool need_reconnect = false;
            std::vector<EventSubscribeStruct>::iterator esspos;
            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                if(esspos->callback != nullptr || esspos->ev_queue != nullptr)
                {
                    need_reconnect = true;
                    break;
                }
            }

            try
            {
                epos->second.callback_monitor->get_monitor();

                if(need_reconnect)
                {
                    if((ipos->second.channel_type == NOTIFD) && (!epos->second.filter_ok))
                    {
                        try
                        {
                            re_subscribe_event(epos, ipos);
                            epos->second.filter_ok = true;
                        }
                        catch(...)
                        {
                        }
                    }
                }

                std::string domain_name;
                std::string event_name;

                std::string::size_type pos = epos->first.rfind('.');
                if(pos == std::string::npos)
                {
                    domain_name = "domain_name";
                    event_name = "event_name";
                }
                else
                {
                    domain_name = epos->second.get_client_attribute_name();
                    event_name = epos->first.substr(pos + 1);

                    event_name = detail::remove_idl_prefix(event_name);
                }

                for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
                {
                    CallBack *callback = esspos->callback;
                    EventQueue *ev_queue = esspos->ev_queue;

                    //
                    // Push an event with error set
                    //

                    if(event_name == CONF_TYPE_EVENT)
                    {
                        auto *event_data =
                            new FwdAttrConfEventData(esspos->device, domain_name, event_name, dev_attr_conf, errors);

                        safe_execute_callback_or_store_data(callback,
                                                            event_data,
                                                            "EventConsumerKeepAliveThread::run_undetached()",
                                                            epos->first,
                                                            ev_queue);
                    }
                    else if(event_name == DATA_READY_TYPE_EVENT)
                    {
                        DataReadyEventData *event_data =
                            new DataReadyEventData(esspos->device, nullptr, event_name, errors);

                        safe_execute_callback_or_store_data(callback,
                                                            event_data,
                                                            "EventConsumerKeepAliveThread::run_undetached()",
                                                            epos->first,
                                                            ev_queue);
                    }
                    else if(event_name == EventName[INTERFACE_CHANGE_EVENT])
                    {
                        auto *event_data = new DevIntrChangeEventData(esspos->device,
                                                                      event_name,
                                                                      domain_name,
                                                                      (CommandInfoList *) nullptr,
                                                                      (AttributeInfoListEx *) nullptr,
                                                                      false,
                                                                      errors);
                        safe_execute_callback_or_store_data(callback,
                                                            event_data,
                                                            "EventConsumerKeepAliveThread::run_undetached()",
                                                            epos->first,
                                                            ev_queue);
                    }
                    else if(event_name == EventName[PIPE_EVENT])
                    {
                        PipeEventData *event_data =
                            new PipeEventData(esspos->device, domain_name, event_name, dev_pipe, errors);

                        safe_execute_callback_or_store_data(callback,
                                                            event_data,
                                                            "EventConsumerKeepAliveThread::run_undetached()",
                                                            epos->first,
                                                            ev_queue);
                    }
                    else
                    {
                        FwdEventData *event_data =
                            new FwdEventData(esspos->device, domain_name, event_name, dev_attr, errors);

                        safe_execute_callback_or_store_data(callback,
                                                            event_data,
                                                            "EventConsumerKeepAliveThread::run_undetached()",
                                                            epos->first,
                                                            ev_queue);
                    }
                }

                if(!ipos->second.event_system_failed)
                {
                    re_subscribe_after_reconnect(event_consumer, notifd_event_consumer, epos, ipos, domain_name);
                }
                // release callback monitor
                epos->second.callback_monitor->rel_monitor();
            }
            catch(...)
            {
                ApiUtil *au = ApiUtil::instance();
                std::stringstream ss;

                ss << "EventConsumerKeepAliveThread::run_undetached() timeout on callback monitor of " << epos->first;
                au->print_error_message(ss.str().c_str());
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::re_subscribe_after_reconnect
//
// description :
//        Re subscribe to the event after a successfull reconnection to the event channel (device server)
//
// argument :
//        in :
//            - event_consumer : The ZMQ event consumer object
//            - notifd_event_consumer : The notifd event consumer object
//            - epos : Iterator on the EventCallback map
//            - ipos : Iterator on the EventChannel map
//            - domain_name :
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::re_subscribe_after_reconnect(
    PointerWithLock<EventConsumer> &event_consumer,
    PointerWithLock<EventConsumer> &notifd_event_consumer,
    const std::map<std::string, EventCallBackStruct>::iterator &epos,
    const std::map<std::string, EventChannelStruct>::iterator &ipos,
    const std::string &domain_name)
{
    DeviceData subscriber_in;
    std::vector<std::string> subscriber_info;
    auto &device = epos->second.get_device_proxy();
    subscriber_info.push_back(device.dev_name());
    subscriber_info.push_back(epos->second.obj_name);
    subscriber_info.emplace_back("subscribe");
    subscriber_info.push_back(epos->second.event_name);
    if(ipos->second.channel_type == ZMQ)
    {
        subscriber_info.emplace_back("0");
    }
    subscriber_in << subscriber_info;

    bool ds_failed = false;

    try
    {
        if(ipos->second.channel_type == ZMQ)
        {
            ipos->second.adm_device_proxy->command_inout("ZmqEventSubscriptionChange", subscriber_in);
        }
        else
        {
            ipos->second.adm_device_proxy->command_inout("EventSubscriptionChange", subscriber_in);
        }

        ipos->second.heartbeat_skipped = false;
        ipos->second.last_subscribed = Tango::get_current_system_datetime();
    }
    catch(...)
    {
        ds_failed = true;
    }

    if(!ds_failed)
    {
        //
        // Push an event with the value just read from the re-connected server
        // NOT NEEDED for the Data Ready event
        //

        std::vector<EventSubscribeStruct>::iterator esspos;

        std::string ev_name = detail::remove_idl_prefix(epos->second.event_name);

        if((ev_name == "change") || (ev_name == "alarm") || (ev_name == "archive") || (ev_name == "user_event"))
        {
            //
            // For attribute data event
            //

            DeviceAttribute *da = nullptr;
            DevErrorList err;
            err.length(0);

            bool old_transp = device.get_transparency_reconnection();
            device.set_transparency_reconnection(true);

            try
            {
                da = new DeviceAttribute();
                *da = device.read_attribute(epos->second.obj_name.c_str());

                if(da->has_failed())
                {
                    err = da->get_err_stack();
                }
                //
                // The reconnection worked fine. The heartbeat should come back now, when the notifd has not closed the
                // connection. Increase the counter to detect when the heartbeat is not coming back.
                //

                if(ipos->second.channel_type == NOTIFD)
                {
                    ipos->second.has_notifd_closed_the_connection++;
                }
            }
            catch(DevFailed &e)
            {
                err = e.errors;
            }
            device.set_transparency_reconnection(old_transp);

            // if callback methods were specified, call them!
            unsigned int cb_nb = epos->second.callback_list.size();
            unsigned int cb_ctr = 0;
            DeviceAttribute *da_copy = nullptr;

            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                cb_ctr++;
                FwdEventData *event_data;

                if(cb_ctr != cb_nb)
                {
                    da_copy = new DeviceAttribute();
                    da_copy->deep_copy(*da);

                    event_data = new FwdEventData(esspos->device, domain_name, ev_name, da_copy, err);
                }
                else
                {
                    event_data = new FwdEventData(esspos->device, domain_name, ev_name, da, err);
                }

                CallBack *callback = esspos->callback;
                EventQueue *ev_queue = esspos->ev_queue;

                safe_execute_callback_or_store_data(
                    callback, event_data, "EventConsumerKeepAliveThread::run_undetached()", epos->first, ev_queue);
            }
        }

        else if(epos->second.event_name.find(CONF_TYPE_EVENT) != std::string::npos)
        {
            //
            // For attribute configuration event
            //

            AttributeInfoEx *aie = nullptr;
            DevErrorList err;
            err.length(0);
            std::string prefix;
            if(ipos->second.channel_type == NOTIFD)
            {
                prefix = notifd_event_consumer->env_var_fqdn_prefix[0];
            }
            else
            {
                if(!device.get_from_env_var())
                {
                    prefix = "tango://";
                    if(!device.is_dbase_used())
                    {
                        prefix = prefix + device.get_dev_host() + ':' + device.get_dev_port() + '/';
                    }
                    else
                    {
                        prefix = prefix + device.get_db_host() + ':' + device.get_db_port() + '/';
                    }
                }
                else
                {
                    prefix = event_consumer->env_var_fqdn_prefix[0];
                }
            }

            std::string dom_name = prefix + device.dev_name();
            if(!device.is_dbase_used())
            {
                dom_name = dom_name + MODIFIER_DBASE_NO;
            }
            dom_name = dom_name + "/" + epos->second.obj_name;

            bool old_transp = device.get_transparency_reconnection();
            device.set_transparency_reconnection(true);

            try
            {
                aie = new AttributeInfoEx();
                *aie = device.get_attribute_config(epos->second.obj_name);

                //
                // The reconnection worked fine. The heartbeat should come back now, when the notifd has not closed the
                // connection. Increase the counter to detect when the heartbeat is not coming back.
                //

                if(ipos->second.channel_type == NOTIFD)
                {
                    ipos->second.has_notifd_closed_the_connection++;
                }
            }
            catch(DevFailed &e)
            {
                err = e.errors;
            }
            device.set_transparency_reconnection(old_transp);

            unsigned int cb_nb = epos->second.callback_list.size();
            unsigned int cb_ctr = 0;
            AttributeInfoEx *aie_copy = nullptr;

            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                cb_ctr++;
                FwdAttrConfEventData *event_data;
                std::string ev_name = detail::remove_idl_prefix(epos->second.event_name);

                if(cb_ctr != cb_nb)
                {
                    aie_copy = new AttributeInfoEx;
                    *aie_copy = *aie;
                    event_data = new FwdAttrConfEventData(esspos->device, dom_name, ev_name, aie_copy, err);
                }
                else
                {
                    event_data = new FwdAttrConfEventData(esspos->device, dom_name, ev_name, aie, err);
                }

                CallBack *callback = esspos->callback;
                EventQueue *ev_queue = esspos->ev_queue;

                safe_execute_callback_or_store_data(
                    callback, event_data, "EventConsumerKeepAliveThread::run_undetached()", epos->first, ev_queue);
            }
        }
        else if(epos->second.event_name == EventName[INTERFACE_CHANGE_EVENT])
        {
            //
            // For device interface change event
            //

            AttributeInfoListEx *aie = nullptr;
            CommandInfoList *cil = nullptr;
            DevErrorList err;
            err.length(0);
            std::string prefix = event_consumer->env_var_fqdn_prefix[0];
            std::string dom_name = prefix + device.dev_name();

            bool old_transp = device.get_transparency_reconnection();
            device.set_transparency_reconnection(true);

            try
            {
                aie = device.attribute_list_query_ex();
                cil = device.command_list_query();
            }
            catch(DevFailed &e)
            {
                delete aie;
                aie = nullptr;
                delete cil;
                cil = nullptr;

                err = e.errors;
            }
            device.set_transparency_reconnection(old_transp);

            unsigned int cb_nb = epos->second.callback_list.size();
            unsigned int cb_ctr = 0;
            CommandInfoList *cil_copy = nullptr;
            AttributeInfoListEx *aie_copy = nullptr;

            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                cb_ctr++;
                DevIntrChangeEventData *event_data;
                std::string ev_name(epos->second.event_name);

                if(cb_ctr != cb_nb)
                {
                    aie_copy = new AttributeInfoListEx;
                    *aie_copy = *aie;
                    cil_copy = new CommandInfoList;
                    *cil_copy = *cil;
                    event_data =
                        new DevIntrChangeEventData(esspos->device, ev_name, dom_name, cil_copy, aie_copy, true, err);
                }
                else
                {
                    event_data = new DevIntrChangeEventData(esspos->device, ev_name, dom_name, cil, aie, true, err);
                }

                CallBack *callback = esspos->callback;
                EventQueue *ev_queue = esspos->ev_queue;

                safe_execute_callback_or_store_data(
                    callback, event_data, "EventConsumerKeepAliveThread::run_undetached()", epos->first, ev_queue);

                if(callback != nullptr)
                {
                    if(cb_ctr != cb_nb)
                    {
                        delete aie_copy;
                        delete cil_copy;
                    }
                    else
                    {
                        delete aie;
                        delete cil;
                    }
                }
            }
        }
        else if(epos->second.event_name == EventName[PIPE_EVENT])
        {
            //
            // For pipe event
            //

            DevicePipe *dp = nullptr;
            DevErrorList err;
            err.length(0);

            bool old_transp = device.get_transparency_reconnection();
            device.set_transparency_reconnection(true);

            try
            {
                dp = new DevicePipe();
                *dp = device.read_pipe(epos->second.obj_name);
            }
            catch(DevFailed &e)
            {
                err = e.errors;
            }
            device.set_transparency_reconnection(old_transp);

            unsigned int cb_nb = epos->second.callback_list.size();
            unsigned int cb_ctr = 0;

            DevicePipe *dp_copy = nullptr;

            for(esspos = epos->second.callback_list.begin(); esspos != epos->second.callback_list.end(); ++esspos)
            {
                cb_ctr++;
                PipeEventData *event_data;

                if(cb_ctr != cb_nb)
                {
                    dp_copy = new DevicePipe();
                    *dp_copy = *dp;

                    event_data = new PipeEventData(esspos->device, domain_name, epos->second.event_name, dp_copy, err);
                }
                else
                {
                    event_data = new PipeEventData(esspos->device, domain_name, epos->second.event_name, dp, err);
                }

                CallBack *callback = esspos->callback;
                EventQueue *ev_queue = esspos->ev_queue;

                safe_execute_callback_or_store_data(
                    callback, event_data, "EventConsumerKeepAliveThread::run_undetached()", epos->first, ev_queue);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        EventConsumerKeepAliveThread::stateless_subscription_failed
//
// description :
//
// argument :
//        in :
//            - vpos : An iterator in the vector of non-connected event for the concerned event
//            - e : The received exception
//            - now : When the exception was received
//
//--------------------------------------------------------------------------------------------------------------------

void EventConsumerKeepAliveThread::stateless_subscription_failed(const std::vector<EventNotConnected>::iterator &vpos,
                                                                 const DevFailed &e,
                                                                 const time_t &now)
{
    //
    // Subscribe has not worked, try again in the next hearbeat period
    //

    vpos->last_heartbeat = now;

    //
    // The event can still not be connected. Send the return error message as event to the client application.
    // Push an event with the error message!
    //

    auto event_name = detail::remove_idl_prefix(vpos->event_name);

    DevErrorList err;
    err.length(0);
    std::string domain_name = vpos->prefix + vpos->device->dev_name();
    if(event_name != EventName[INTERFACE_CHANGE_EVENT])
    {
        domain_name = domain_name + "/" + vpos->attribute;
    }
    err = e.errors;

    //
    // For attribute data event
    //

    if((event_name == "change") || (event_name == "alarm") || (event_name == "archive") || (event_name == "periodic") ||
       (event_name == "user_event"))
    {
        DeviceAttribute *da = nullptr;
        FwdEventData *event_data = new FwdEventData(vpos->device, domain_name, event_name, da, err);

        // if a callback method was specified, call it!

        safe_execute_callback_or_store_data(vpos->callback,
                                            event_data,
                                            "EventConsumerKeepAliveThread::stateless_subscription_failed()",
                                            domain_name,
                                            vpos->ev_queue);
    }

    //
    // For attribute configuration event
    //

    else if(event_name == CONF_TYPE_EVENT)
    {
        AttributeInfoEx *aie = nullptr;
        AttrConfEventData *event_data = new AttrConfEventData(vpos->device, domain_name, event_name, aie, err);

        safe_execute_callback_or_store_data(vpos->callback,
                                            event_data,
                                            "EventConsumerKeepAliveThread::stateless_subscription_failed()",
                                            domain_name,
                                            vpos->ev_queue);
    }
    else if(event_name == DATA_READY_TYPE_EVENT)
    {
        DataReadyEventData *event_data = new DataReadyEventData(vpos->device, nullptr, event_name, err);

        safe_execute_callback_or_store_data(vpos->callback,
                                            event_data,
                                            "EventConsumerKeepAliveThread::stateless_subscription_failed()",
                                            domain_name,
                                            vpos->ev_queue);
    }
    else if(event_name == EventName[INTERFACE_CHANGE_EVENT])
    {
        auto *event_data = new DevIntrChangeEventData(vpos->device,
                                                      event_name,
                                                      domain_name,
                                                      (CommandInfoList *) nullptr,
                                                      (AttributeInfoListEx *) nullptr,
                                                      false,
                                                      err);

        safe_execute_callback_or_store_data(vpos->callback,
                                            event_data,
                                            "EventConsumerKeepAliveThread::stateless_subscription_failed()",
                                            domain_name,
                                            vpos->ev_queue);
    }
    else if(event_name == EventName[PIPE_EVENT])
    {
        PipeEventData *event_data =
            new PipeEventData(vpos->device, domain_name, event_name, (DevicePipe *) nullptr, err);

        safe_execute_callback_or_store_data(vpos->callback,
                                            event_data,
                                            "EventConsumerKeepAliveThread::stateless_subscription_failed()",
                                            domain_name,
                                            vpos->ev_queue);
    }
}

} // namespace Tango
