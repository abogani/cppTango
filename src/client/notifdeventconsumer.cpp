////////////////////////////////////////////////////////////////////////////////
///
///  file     notifdeventconsumer.cpp
///
///        C++ classes for implementing the event consumer
///        singleton class when used with notifd
///
///        author(s) : E.Taurel
///
///        original : 16 August 2011
///
// Copyright (C) :      2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
///
///
///
////////////////////////////////////////////////////////////////////////////////

#include <tango/client/eventconsumer.h>
#include <tango/client/Database.h>
#include <tango/client/event.h>
#include <tango/server/auto_tango_monitor.h>

#include <cstdio>

#ifdef _TG_WINDOWS_
  #include <process.h>
#else
  #include <unistd.h>
#endif

using namespace CORBA;

namespace Tango
{

void NotifdEventConsumer::disconnect_structured_push_consumer()
{
    TANGO_LOG_DEBUG << "calling Tango::NotifdEventConsumer::disconnect_structured_push_consumer() \n";
}

void NotifdEventConsumer::offer_change(TANGO_UNUSED(const CosNotification::EventTypeSeq &added),
                                       TANGO_UNUSED(const CosNotification::EventTypeSeq &deled))
{
    TANGO_LOG_DEBUG << "calling Tango::NotifdEventConsumer::subscription_change() \n";
}

/************************************************************************/
/*                                                                           */
/*             NotifdEventConsumer class                                     */
/*            -------------------                                            */
/*                                                                           */
/************************************************************************/

NotifdEventConsumer::NotifdEventConsumer(ApiUtil *ptr) :
    EventConsumer(ptr),
    omni_thread((void *) ptr)
{
    TANGO_LOG_DEBUG << "calling Tango::NotifdEventConsumer::NotifdEventConsumer() \n";
    orb_ = ptr->get_orb();

    start_undetached();
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventConsumer::run_undetached()
//
// description :     Activate the CORBA POA for the server embedded in client
//                  when it receives events.
//                  Run the CORBA orb.
//
//-----------------------------------------------------------------------------

void *NotifdEventConsumer::run_undetached(void *arg)
{
    //
    // activate POA and go into endless loop waiting for events to be pushed
    // this method should run as a separate thread so as not to block the client
    //

    ApiUtil *api_util_ptr = (ApiUtil *) arg;
    if(!api_util_ptr->in_server())
    {
        CORBA::Object_var obj = orb_->resolve_initial_references("RootPOA");
        PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);
        PortableServer::POAManager_var pman = poa->the_POAManager();
        pman->activate();

        orb_->run();

        orb_->destroy();
    }

    return (void *) nullptr;
}

void NotifdEventConsumer::query_event_system(TANGO_UNUSED(std::ostream &os))
{
    TANGO_ASSERT(0);
}

void NotifdEventConsumer::get_subscribed_event_ids(TANGO_UNUSED(DeviceProxy *dev), TANGO_UNUSED(std::vector<int> &ids))
{
    TANGO_ASSERT(0);
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventConsumer::cleanup_EventChannel_map()
//
// description :     Method to destroy the DeviceProxy objects
//                    stored in the EventChannel map
//
//-----------------------------------------------------------------------------

void NotifdEventConsumer::cleanup_EventChannel_map()
{
    std::map<std::string, EventCallBackStruct>::iterator epos;
    EvChanIte evt_it;
    for(epos = event_callback_map.begin(); epos != event_callback_map.end(); ++epos)
    {
        EventCallBackStruct &evt_cb = epos->second;
        evt_it = channel_map.find(evt_cb.channel_name);
        EventChannelStruct &evt_ch = evt_it->second;
        if(evt_ch.channel_type == NOTIFD)
        {
            try
            {
                CosNotifyFilter::Filter_var f = evt_ch.structuredProxyPushSupplier->get_filter(evt_cb.filter_id);
                evt_ch.structuredProxyPushSupplier->remove_filter(evt_cb.filter_id);

                f->destroy();
            }
            catch(...)
            {
                std::cerr << "Could not remove filter from notification daemon for " << evt_cb.channel_name
                          << std::endl;
            }
        }
    }

    for(evt_it = channel_map.begin(); evt_it != channel_map.end(); ++evt_it)
    {
        EventChannelStruct &evt_ch = evt_it->second;
        if(evt_ch.adm_device_proxy != nullptr)
        {
            AutoTangoMonitor _mon(evt_ch.channel_monitor);

            if(evt_ch.channel_type == NOTIFD)
            {
                try
                {
                    // Destroy the filter created in the
                    // notification service for the heartbeat event

                    CosNotifyFilter::Filter_var f =
                        evt_ch.structuredProxyPushSupplier->get_filter(evt_ch.heartbeat_filter_id);
                    evt_ch.structuredProxyPushSupplier->remove_filter(evt_ch.heartbeat_filter_id);
                    f->destroy();

                    // disconnect the pushsupplier to stop receiving events

                    // TANGO_LOG << "EventConsumer::cleanup_EventChannel_map(): Disconnect push supplier!" << std::endl;

                    evt_ch.structuredProxyPushSupplier->disconnect_structured_push_supplier();
                }
                catch(...)
                {
                    std::cerr << "Could not remove heartbeat filter from notification daemon for "
                              << evt_ch.full_adm_name << std::endl;
                }

                evt_ch.adm_device_proxy = nullptr;
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// method :         EventConsumer::connect_event_system()
//
// description :    Connect to the real event (archive, change, periodic,...)
//
// argument : in :    - device_name : The device fqdn (lower case)
//                  - att_name : The attribute name
//                  - event_name : The event name
//                  - filters : The event filters given by the user
//                  - evt_it : Iterator pointing to the event channel entry
//                             in channel_map map
//                  - new_event_callback : Structure used for the event callback
//                                         entry in the event_callback_map
//                  - dd : The data returned by the DS admin device
//                         xxxSubscriptionChange command
//                  - valid_end : Valid endpoint in case the DS has retruned several
//                                possible endpoints
//
//-----------------------------------------------------------------------------

void NotifdEventConsumer::connect_event_system(const std::string &device_name,
                                               const std::string &att_name,
                                               const std::string &event_name,
                                               const std::vector<std::string> &filters,
                                               const EvChanIte &evt_it,
                                               EventCallBackStruct &new_event_callback,
                                               TANGO_UNUSED(DeviceData &dd),
                                               TANGO_UNUSED(size_t valid_end))
{
    //
    // Build a filter using the CORBA Notify constraint Language
    // (use attribute name in lowercase letters)
    //

    CosNotifyFilter::FilterFactory_var ffp;
    CosNotifyFilter::Filter_var filter = CosNotifyFilter::Filter::_nil();
    CosNotifyFilter::FilterID filter_id;

    try
    {
        EventChannelStruct &evt_ch = evt_it->second;
        {
            AutoTangoMonitor _mon(evt_ch.channel_monitor);
            ffp = evt_ch.eventChannel->default_filter_factory();
            filter = ffp->create_filter("EXTENDED_TCL");
        }
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
    // The device name received by this method is the FQDN, remove the
    // protocol prefix
    //

    const std::string &tmp_dev_name(device_name);
    std::string::size_type pos;
    pos = tmp_dev_name.find("://");
    pos = pos + 3;
    pos = tmp_dev_name.find('/', pos);
    ++pos;
    std::string d_name = tmp_dev_name.substr(pos, tmp_dev_name.size() - pos);

    char constraint_expr[512];

    std::snprintf(constraint_expr,
                  sizeof(constraint_expr),
                  R"($domain_name == '%s/%s' and $event_name == '%s')",
                  d_name.c_str(),
                  att_name.c_str(),
                  event_name.c_str());

    if(filters.size() != 0)
    {
        ::strcat(&(constraint_expr[strlen(constraint_expr)]), " and ((");
        for(unsigned int i = 0; i < filters.size(); i++)
        {
            ::strcat(&(constraint_expr[strlen(constraint_expr)]), filters[i].c_str());

            if(i != filters.size() - 1)
            {
                ::strcat(&(constraint_expr[strlen(constraint_expr)]), " and ");
            }
        }
        ::strcat(&(constraint_expr[strlen(constraint_expr)]), ") or $forced_event > 0.5)");
    }

    CosNotification::EventTypeSeq evs;
    CosNotifyFilter::ConstraintExpSeq exp;
    exp.length(1);
    exp[0].event_types = evs;
    exp[0].constraint_expr = Tango::string_dup(constraint_expr);
    bool error_occurred = false;

    try
    {
        CosNotifyFilter::ConstraintInfoSeq_var dummy = filter->add_constraints(exp);
        filter_id = evt_it->second.structuredProxyPushSupplier->add_filter(filter);

        new_event_callback.filter_id = filter_id;
        new_event_callback.filter_constraint = constraint_expr;
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
    // If error, destroy filter. Else, set the filter_ok flag to true
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
    else
    {
        new_event_callback.filter_ok = true;
    }
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventConsumer::connect_event_channel()
//
// description :    Connect to the event channel
//                  This means connect to the heartbeat event
//
// Args in : - channel name : The event channel name (DS admin name)
//           - db : Database object
//           - reconnect: Flag set to true in case this method is called for
//                        event reconnection purpose
//           - dd : The DS admin device command returned data
//                  (ZmqEventSubscriptionChange command)
//
//-----------------------------------------------------------------------------

void NotifdEventConsumer::connect_event_channel(const std::string &channel_name,
                                                Database *db,
                                                bool reconnect,
                                                TANGO_UNUSED(DeviceData &dd))
{
    CORBA::Any_var received;
    const DevVarLongStringArray *dev_import_list;

    //
    // Get a reference to an EventChannel for this device server from the
    // TANGO database or from the DS admin device (for device in a DS
    // started with the -file option)
    //

    bool channel_exported;
    std::string channel_ior;
    std::string hostname;

    if(db != nullptr)
    {
        //
        // Remove extra info from channel name (protocol,  dbase=xxx)
        //

        std::string local_channel_name(channel_name);
        std::string::size_type pos;
        if((pos = local_channel_name.find('#')) != std::string::npos)
        {
            local_channel_name.erase(pos);
        }
        if((pos = local_channel_name.find("://")) != std::string::npos)
        {
            pos = pos + 3;
            if((pos = local_channel_name.find('/', pos)) != std::string::npos)
            {
                local_channel_name.erase(0, pos + 1);
            }
        }

        //
        // Import channel event
        //

        try
        {
            received = db->import_event(local_channel_name);
        }
        catch(...)
        {
            TangoSys_OMemStream o;

            o << channel_name;
            o << " has no event channel defined in the database\n";
            o << "Maybe the server is not running or is not linked with Tango release 4.x (or above)... " << std::ends;
            TANGO_THROW_EXCEPTION(API_NotificationServiceFailed, o.str());
        }

        received.inout() >>= dev_import_list;
        channel_ior = std::string((dev_import_list->svalue)[1]);
        channel_exported = static_cast<bool>(dev_import_list->lvalue[0]);

        // get the hostname where the notifyd should be running
        hostname = std::string(dev_import_list->svalue[3]);
    }
    else
    {
        DeviceProxy adm(channel_name);

        try
        {
            DeviceData ddd;
            ddd = adm.command_inout("QueryEventChannelIOR");
            ddd >> channel_ior;
            channel_exported = true;

            // get the hostname where the notifyd should be running
            DeviceInfo info = adm.info();
            hostname = info.server_host;
        }
        catch(...)
        {
            TangoSys_OMemStream o;

            o << channel_name;
            o << " has no event channel\n";
            o << "Maybe the server is not running or is not linked with Tango release 4.x (or above)... " << std::ends;
            TANGO_THROW_EXCEPTION(API_NotificationServiceFailed, o.str());
        }
    }

    if(channel_exported)
    {
        try
        {
            CORBA::Object *event_channel_obj;
            event_channel_obj = orb_->string_to_object(channel_ior.c_str());

            if(event_channel_obj->_non_existent())
            {
                event_channel_obj = CORBA::Object::_nil();
            }

            eventChannel = CosNotifyChannelAdmin::EventChannel::_nil();
            eventChannel = CosNotifyChannelAdmin::EventChannel::_narrow(event_channel_obj);

            if(CORBA::is_nil(eventChannel))
            {
                channel_exported = false;
            }
        }
        catch(...)
        {
            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to narrow EventChannel from notification daemon (hint: make sure the "
                "notifd process is running on this host)");
        }
    }
    else
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_EventChannelNotExported,
            "Failed to narrow EventChannel (hint: make sure a notifd process is running on the server host)");
    }

    //
    // Obtain a Consumer Admin
    //

    //
    // We'll use the channel's default Consumer admin
    //

    try
    {
        consumerAdmin = eventChannel->default_consumer_admin();

        if(CORBA::is_nil(consumerAdmin))
        {
            // cerr << "Could not get CosNotifyChannelAdmin::ConsumerAdmin" << std::endl;
            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to get default Consumer admin from notification daemon (hint: make sure "
                "the notifd process is running on this host)");
        }
    }
    catch(...)
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_NotificationServiceFailed,
            "Failed to get default Consumer admin from notification daemon (hint: make sure the "
            "notifd process is running on this host)");
    }

    //
    // Obtain a Proxy Supplier
    //

    //
    // We are using the "Push" model and Structured data
    //

    try
    {
        proxySupplier =
            consumerAdmin->obtain_notification_push_supplier(CosNotifyChannelAdmin::STRUCTURED_EVENT, proxyId);
        if(CORBA::is_nil(proxySupplier))
        {
            // cerr << "Could not get CosNotifyChannelAdmin::ProxySupplier" << std::endl;
            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to obtain a push supplier from notification daemon (hint: make sure the "
                "notifd process is running on this host)");
        }

        structuredProxyPushSupplier = CosNotifyChannelAdmin::StructuredProxyPushSupplier::_narrow(proxySupplier);
        if(CORBA::is_nil(structuredProxyPushSupplier))
        {
            // cerr << "Tango::NotifdEventConsumer::NotifdEventConsumer() could not get
            // CosNotifyChannelAdmin::StructuredProxyPushSupplier" << std::endl;
            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to narrow the push supplier from notification daemon (hint: make sure "
                "the notifd process is running on this host)");
        }

        //
        // Set a large timeout on this CORBA object
        // This is necessary in case of maany threads doing subscribe/unsubscribe as fast as they can
        //

        omniORB::setClientCallTimeout(structuredProxyPushSupplier, 20000);
    }
    catch(const CosNotifyChannelAdmin::AdminLimitExceeded &)
    {
        TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept,
                                       API_NotificationServiceFailed,
                                       "Failed to get PushSupplier from notification daemon due to AdminLimitExceeded "
                                       "(hint: make sure the notifd process is running on this host)");
    }

    //
    // Connect to the Proxy Consumer
    //

    try
    {
        structuredProxyPushSupplier->connect_structured_push_consumer(_this());
    }
    catch(const CosEventChannelAdmin::AlreadyConnected &)
    {
        std::cerr << "Tango::NotifdEventConsumer::NotifdEventConsumer() caught AlreadyConnected exception" << std::endl;
    }

    auto evt_it = channel_map.end();
    if(reconnect)
    {
        evt_it = channel_map.find(channel_name);
        EventChannelStruct &evt_ch = evt_it->second;
        evt_ch.eventChannel = eventChannel;
        evt_ch.structuredProxyPushSupplier = structuredProxyPushSupplier;
        evt_ch.last_heartbeat = Tango::get_current_system_datetime();
        evt_ch.heartbeat_skipped = false;
        evt_ch.notifyd_host = hostname;
        evt_ch.event_system_failed = false;
        evt_ch.has_notifd_closed_the_connection = 0;
    }
    else
    {
        EventChannelStruct new_event_channel_struct;
        new_event_channel_struct.eventChannel = eventChannel;
        new_event_channel_struct.structuredProxyPushSupplier = structuredProxyPushSupplier;
        new_event_channel_struct.last_heartbeat = Tango::get_current_system_datetime();
        new_event_channel_struct.heartbeat_skipped = false;
        new_event_channel_struct.adm_device_proxy = nullptr;
        // create a channel monitor
        new_event_channel_struct.channel_monitor = std::make_shared<TangoMonitor>(channel_name.c_str());
        // set the timeout for the channel monitor to 1000ms not to block the event consumer for to long.
        new_event_channel_struct.channel_monitor->timeout(1000);
        set_channel_type(new_event_channel_struct);

        channel_map[channel_name] = new_event_channel_struct;
        evt_it = channel_map.find(channel_name);
        EventChannelStruct &evt_ch = evt_it->second;
        evt_ch.notifyd_host = hostname;
        evt_ch.event_system_failed = false;
        evt_ch.has_notifd_closed_the_connection = 0;
    }

    //
    // add a filter for heartbeat events
    //

    char constraint_expr[256];
    std::snprintf(constraint_expr, sizeof(constraint_expr), "$event_name == \'heartbeat\'");
    CosNotifyFilter::FilterFactory_var ffp;
    CosNotifyFilter::Filter_var filter = CosNotifyFilter::Filter::_nil();
    try
    {
        ffp = evt_it->second.eventChannel->default_filter_factory();
        filter = ffp->create_filter("EXTENDED_TCL");
    }
    catch(...)
    {
        // cerr << "Caught exception obtaining filter object" << std::endl;
        TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept,
                                       API_NotificationServiceFailed,
                                       "Caught exception while creating heartbeat filter (check filter)");
    }

    //
    // Construct a simple constraint expression; add it to fadmin
    //

    CosNotification::EventTypeSeq evs;
    CosNotifyFilter::ConstraintExpSeq exp;
    exp.length(1);
    exp[0].event_types = evs;
    exp[0].constraint_expr = Tango::string_dup(constraint_expr);
    bool error_occurred = false;
    try
    {
        CosNotifyFilter::ConstraintInfoSeq_var dummy = filter->add_constraints(exp);
        evt_it->second.heartbeat_filter_id = evt_it->second.structuredProxyPushSupplier->add_filter(filter);
    }
    catch(...)
    {
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
                                       "Caught exception while adding constraint for heartbeat (check filter)");
    }
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventConsumer::push_structured_event()
//
// description :     Method run when an event is received
//
// argument : in :    event : The event itself...
//
//-----------------------------------------------------------------------------

void NotifdEventConsumer::push_structured_event(const CosNotification::StructuredEvent &event)
{
    std::string domain_name(event.header.fixed_header.event_type.domain_name);
    std::string event_type(event.header.fixed_header.event_type.type_name);
    std::string event_name(event.header.fixed_header.event_name);

    //    TANGO_LOG << "Received event: domain_name = " << domain_name << ", event_type = " << event_type << ",
    //    event_name = " << event_name << std::endl;

    bool svr_send_tg_host = false;

    if(event_name == "heartbeat")
    {
        std::string fq_dev_name = domain_name;
        if(event_type.find("tango://") != std::string::npos)
        {
            if(event_type.find('#') == std::string::npos)
            {
                fq_dev_name.insert(0, event_type);
            }
            else
            {
                fq_dev_name.insert(0, event_type, 0, event_type.size() - 1);
                fq_dev_name = fq_dev_name + MODIFIER_DBASE_NO;
            }
            svr_send_tg_host = true;
        }
        else
        {
            fq_dev_name.insert(0, env_var_fqdn_prefix[0]);
        }

        // only reading from the maps
        map_modification_lock.readerIn();

        std::map<std::string, EventChannelStruct>::iterator ipos;
        ipos = channel_map.find(fq_dev_name);

        //
        // Search for entry within the channel_map map using
        // 1 - The fully qualified device name
        // 2 - The fully qualified device name but with the Tango database host name specifed without fqdn
        // (in case of)
        // 3 - The device name (for old servers)
        //

        if(ipos == channel_map.end())
        {
            std::string::size_type pos, end;
            if((pos = event_type.find('.')) != std::string::npos)
            {
                end = event_type.find(':', pos);
                fq_dev_name.erase(pos, end - pos);

                ipos = channel_map.find(fq_dev_name);

                if(ipos == channel_map.end())
                {
                    ipos = channel_map.find(domain_name);
                }
            }
            else
            {
                ipos = channel_map.find(domain_name);
            }
        }

        //
        // Special case for Tango system with multiple db server
        //
        // The event carry info for only one of the multiple db server
        // The client also has to know the list of db servers (via the TANGO_HOST)
        // Find the event db server in the client list of db server and if found,
        // replace in the fqdn the event db server with the first one in the client
        // list. The first db server is the one used to create the entry in the map
        //

        if(ipos == channel_map.end() && svr_send_tg_host)
        {
            std::string svc_tango_host = event_type.substr(8, event_type.size() - 9);
            unsigned int i = 0;
            for(i = 1; i < env_var_fqdn_prefix.size(); i++)
            {
                if(env_var_fqdn_prefix[i].find(svc_tango_host) != std::string::npos)
                {
                    fq_dev_name = domain_name;
                    fq_dev_name.insert(0, env_var_fqdn_prefix[0]);
                    break;
                }
            }
            if(i != env_var_fqdn_prefix.size())
            {
                ipos = channel_map.find(fq_dev_name);
            }
        }

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
                std::cerr << "Tango::NotifdEventConsumer::push_structured_event() timeout on channel monitor of "
                          << ipos->first << std::endl;
            }
        }

        map_modification_lock.readerOut();
    }
    else
    {
        std::string fq_dev_name = domain_name;
        if(event_type.find("tango://") != std::string::npos)
        {
            if(event_type.find('#') == std::string::npos)
            {
                fq_dev_name.insert(0, event_type);
            }
            else
            {
                fq_dev_name.insert(0, event_type, 0, event_type.size() - 1);
                fq_dev_name = fq_dev_name + MODIFIER_DBASE_NO;
            }
            svr_send_tg_host = true;
        }
        else
        {
            fq_dev_name.insert(0, env_var_fqdn_prefix[0]);
        }

        map_modification_lock.readerIn();
        bool map_lock = true;

        //
        // Search for entry within the event_callback map using
        // 1 - The fully qualified attribute event name
        // 2 - The fully qualified attribute event name but with the Tango database host name specifed without fqdn
        // (in case of)
        // 3 - In case of Tango system with multi db server, replace the db server defined by
        // the event with the first one in the client list (like for the heartbeat event)
        //

        std::string attr_event_name = fq_dev_name + "." + event_name;
        std::map<std::string, EventCallBackStruct>::iterator ipos;

        ipos = event_callback_map.find(attr_event_name);
        if(ipos == event_callback_map.end())
        {
            std::string::size_type pos, end;
            if((pos = event_type.find('.')) != std::string::npos)
            {
                end = event_type.find(':', pos);
                attr_event_name.erase(pos, end - pos);

                ipos = event_callback_map.find(attr_event_name);

                if(ipos == event_callback_map.end() && svr_send_tg_host)
                {
                    std::string svc_tango_host = event_type.substr(8, event_type.size() - 9);
                    unsigned int i = 0;
                    for(i = 1; i < env_var_fqdn_prefix.size(); i++)
                    {
                        if(env_var_fqdn_prefix[i].find(svc_tango_host) != std::string::npos)
                        {
                            fq_dev_name = domain_name;
                            fq_dev_name.insert(0, env_var_fqdn_prefix[0]);
                            attr_event_name = fq_dev_name + "." + event_name;
                            break;
                        }
                    }

                    if(i != env_var_fqdn_prefix.size())
                    {
                        ipos = event_callback_map.find(attr_event_name);
                    }
                }
            }
        }

        if(ipos != event_callback_map.end())
        {
            EventCallBackStruct &evt_cb = ipos->second;
            try
            {
                AutoTangoMonitor _mon(evt_cb.callback_monitor);

                AttributeValue *attr_value = nullptr;
                AttributeValue_3 *attr_value_3 = nullptr;
                AttributeValue_4 *attr_value_4 = nullptr;
                AttributeConfig_2 *attr_conf_2 = nullptr;
                AttributeConfig_3 *attr_conf_3 = nullptr;
                AttributeInfoEx *attr_info_ex = nullptr;
                AttDataReady *att_ready = nullptr;
                DevErrorList errors;
                bool ev_attr_conf = false;
                bool ev_attr_ready = false;
                const DevErrorList *err_ptr = nullptr;

                //
                // Check if the event transmit error
                //

                DeviceAttribute *dev_attr = nullptr;
                CORBA::TypeCode_var ty = event.remainder_of_body.type();
                if(ty->kind() == tk_struct)
                {
                    CORBA::String_var st_name;
                    st_name = ty->name();
                    const char *tmp_ptr = st_name.in();
                    long vers;
                    if(::strcmp(tmp_ptr, "AttributeValue_4") == 0)
                    {
                        dev_attr = new(DeviceAttribute);
                        event.remainder_of_body >>= attr_value_4;
                        vers = 4;
                        attr_to_device(attr_value_4, dev_attr);
                    }
                    else if(::strcmp(tmp_ptr, "AttributeValue_3") == 0)
                    {
                        dev_attr = new(DeviceAttribute);
                        event.remainder_of_body >>= attr_value_3;
                        vers = 3;
                        attr_to_device(attr_value, attr_value_3, vers, dev_attr);
                    }
                    else if(::strcmp(tmp_ptr, "AttributeValue") == 0)
                    {
                        dev_attr = new(DeviceAttribute);
                        event.remainder_of_body >>= attr_value;
                        vers = 2;
                        attr_to_device(attr_value, attr_value_3, vers, dev_attr);
                    }
                    else if(::strcmp(tmp_ptr, "AttributeConfig_2") == 0)
                    {
                        event.remainder_of_body >>= attr_conf_2;
                        vers = 2;
                        attr_info_ex = new AttributeInfoEx();
                        *attr_info_ex = attr_conf_2;
                        ev_attr_conf = true;
                    }
                    else if(::strcmp(tmp_ptr, "AttributeConfig_3") == 0)
                    {
                        event.remainder_of_body >>= attr_conf_3;
                        vers = 3;
                        attr_info_ex = new AttributeInfoEx();
                        *attr_info_ex = attr_conf_3;
                        ev_attr_conf = true;
                    }
                    else if(::strcmp(tmp_ptr, "AttDataReady") == 0)
                    {
                        event.remainder_of_body >>= att_ready;
                        ev_attr_conf = false;
                        ev_attr_ready = true;
                    }
                    else
                    {
                        errors.length(1);

                        errors[0].severity = Tango::ERR;
                        errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
                        errors[0].reason = Tango::string_dup(API_IncompatibleAttrDataType);
                        errors[0].desc =
                            Tango::string_dup("Unknown structure used to pass attribute value (Need compilation ?)");
                        dev_attr = nullptr;
                    }
                }
                else
                {
                    event.remainder_of_body >>= err_ptr;
                    errors = *err_ptr;

                    //
                    // We need to find which type of event we have received
                    //

                    std::string::size_type pos = attr_event_name.find('.');
                    std::string att_type = attr_event_name.substr(pos + 1);
                    if(att_type == CONF_TYPE_EVENT)
                    {
                        ev_attr_conf = true;
                    }
                    else if(att_type == DATA_READY_TYPE_EVENT)
                    {
                        ev_attr_ready = true;
                    }
                }

                //
                // Fire the user callback
                //

                std::vector<EventSubscribeStruct>::iterator esspos;

                unsigned int cb_nb = ipos->second.callback_list.size();
                unsigned int cb_ctr = 0;

                for(esspos = evt_cb.callback_list.begin(); esspos != evt_cb.callback_list.end(); ++esspos)
                {
                    cb_ctr++;
                    if(esspos->id > 0)
                    {
                        CallBack *callback;
                        callback = esspos->callback;
                        EventQueue *ev_queue;
                        ev_queue = esspos->ev_queue;

                        if(cb_ctr == cb_nb)
                        {
                            map_lock = false;
                            map_modification_lock.readerOut();
                        }

                        if((!ev_attr_conf) && (!ev_attr_ready))
                        {
                            EventData *event_data;
                            if(cb_ctr != cb_nb)
                            {
                                DeviceAttribute *dev_attr_copy = nullptr;
                                if(dev_attr != nullptr)
                                {
                                    dev_attr_copy = new DeviceAttribute();
                                    dev_attr_copy->deep_copy(*dev_attr);
                                }

                                event_data =
                                    new EventData(esspos->device, fq_dev_name, event_name, dev_attr_copy, errors);
                            }
                            else
                            {
                                event_data = new EventData(esspos->device, fq_dev_name, event_name, dev_attr, errors);
                            }

                            // if a callback method was specified, call it!
                            if(callback != nullptr)
                            {
                                try
                                {
                                    callback->push_event(event_data);
                                }
                                catch(...)
                                {
                                    std::cerr << "Tango::NotifdEventConsumer::push_structured_event() exception in "
                                                 "callback method of "
                                              << ipos->first << std::endl;
                                }

                                delete event_data;
                            }

                            // no calback method, the event has to be instered
                            // into the event queue
                            else
                            {
                                ev_queue->insert_event(event_data);
                            }
                        }
                        else if(!ev_attr_ready)
                        {
                            AttrConfEventData *event_data;

                            if(cb_ctr != cb_nb)
                            {
                                AttributeInfoEx *attr_info_copy = new AttributeInfoEx();
                                *attr_info_copy = *attr_info_ex;
                                event_data = new AttrConfEventData(
                                    esspos->device, fq_dev_name, event_name, attr_info_copy, errors);
                            }
                            else
                            {
                                event_data = new AttrConfEventData(
                                    esspos->device, fq_dev_name, event_name, attr_info_ex, errors);
                            }

                            // if callback methods were specified, call them!
                            if(callback != nullptr)
                            {
                                try
                                {
                                    callback->push_event(event_data);
                                }
                                catch(...)
                                {
                                    std::cerr << "Tango::NotifdEventConsumer::push_structured_event() exception in "
                                                 "callback method of "
                                              << ipos->first << std::endl;
                                }

                                delete event_data;
                            }

                            // no calback method, the event has to be instered
                            // into the event queue
                            else
                            {
                                ev_queue->insert_event(event_data);
                            }
                        }
                        else
                        {
                            DataReadyEventData *event_data =
                                new DataReadyEventData(esspos->device, att_ready, event_name, errors);
                            // if a callback method was specified, call it!
                            if(callback != nullptr)
                            {
                                try
                                {
                                    callback->push_event(event_data);
                                }
                                catch(...)
                                {
                                    std::cerr << "Tango::NotifdEventConsumer::push_structured_event() exception in "
                                                 "callback method of "
                                              << ipos->first << std::endl;
                                }
                                delete event_data;
                            }

                            // no calback method, the event has to be instered
                            // into the event queue
                            else
                            {
                                ev_queue->insert_event(event_data);
                            }
                        }
                    }
                    else // id < 0
                    {
                        if(cb_ctr == cb_nb)
                        {
                            map_lock = false;
                            map_modification_lock.readerOut();
                        }

                        if((!ev_attr_conf) && (!ev_attr_ready))
                        {
                            delete dev_attr;
                        }
                        else if(!ev_attr_ready)
                        {
                            delete attr_info_ex;
                        }
                    }
                } // End of for
            }
            catch(...)
            {
                // free the map lock if not already done
                if(map_lock)
                {
                    map_modification_lock.readerOut();
                }

                std::cerr << "Tango::NotifdEventConsumer::push_structured_event() timeout on callback monitor of "
                          << ipos->first << std::endl;
            }
        }
        else
        {
            // even if nothing was found in the map, free the lock
            map_modification_lock.readerOut();
        }
    }
}

ReceivedFromAdmin
    NotifdEventConsumer::initialize_received_from_admin(TANGO_UNUSED(const Tango::DevVarLongStringArray *dvlsa),
                                                        const std::string &local_callback_key,
                                                        const std::string &adm_name,
                                                        bool device_from_env_var)
{
    ReceivedFromAdmin result;
    result.event_name = local_callback_key;

    std::string full_adm_name(adm_name);
    if(device_from_env_var)
    {
        full_adm_name.insert(0, env_var_fqdn_prefix[0]);
    }

    result.channel_name = full_adm_name;

    TANGO_LOG_DEBUG << "received_from_admin.event_name = " << result.event_name << std::endl;
    TANGO_LOG_DEBUG << "received_from_admin.channel_name = " << result.channel_name << std::endl;
    return result;
}

} // namespace Tango
