////////////////////////////////////////////////////////////////////////////////
//
//  file     notifdeventsupplier.cpp
//
//        C++ class for implementing the event server
//        singleton class - NotifdEventSupplier.
//        This class is used to send events from the server
//        to the notification service.
//
//      author(s) : E.Taurel (taurel@esrf.fr)
//
//        original : August 2011
//
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
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

#include <tango/server/eventsupplier.h>
#include <tango/server/dserver.h>
#include <tango/server/device.h>
#include <tango/client/Database.h>

#include <tango/internal/utils.h>

#include <COS/CosNotification.hh>
#include <COS/CosNotifyChannelAdmin.hh>
#include <COS/CosNotifyComm.hh>
#include <cstdio>

#ifdef _TG_WINDOWS_
  #include <process.h>
#else
  #include <unistd.h>
#endif

using namespace CORBA;

namespace Tango
{

NotifdEventSupplier *NotifdEventSupplier::_instance = nullptr;

/************************************************************************/
/*                                                                           */
/*             NotifdEventSupplier class                                     */
/*            -------------------                                            */
/*                                                                           */
/************************************************************************/

NotifdEventSupplier::NotifdEventSupplier(
    CORBA::ORB_var _orb,
    CosNotifyChannelAdmin::SupplierAdmin_var _supplierAdmin,
    CosNotifyChannelAdmin::ProxyID _proxId,
    CosNotifyChannelAdmin::ProxyConsumer_var _proxyConsumer,
    CosNotifyChannelAdmin::StructuredProxyPushConsumer_var _structuredProxyPushConsumer,
    CosNotifyChannelAdmin::EventChannelFactory_var _eventChannelFactory,
    CosNotifyChannelAdmin::EventChannel_var _eventChannel,
    std::string &event_ior,
    Util *tg) :
    EventSupplier(tg)
{
    orb_ = _orb;
    supplierAdmin = _supplierAdmin;
    proxyId = _proxId;
    proxyConsumer = _proxyConsumer;
    structuredProxyPushConsumer = _structuredProxyPushConsumer;
    eventChannelFactory = _eventChannelFactory;
    eventChannel = _eventChannel;
    event_channel_ior = event_ior;

    _instance = this;
}

NotifdEventSupplier *NotifdEventSupplier::create(CORBA::ORB_var _orb, std::string server_name, Util *tg)
{
    TANGO_LOG_DEBUG << "calling Tango::NotifdEventSupplier::create() \n";

    //
    // does the NotifdEventSupplier singleton exist already ? if so simply return it
    //

    if(_instance != nullptr)
    {
        return _instance;
    }

    //
    // Connect process to the notifd service
    //

    NotifService ns;
    connect_to_notifd(ns, _orb, server_name, tg);

    //
    // NotifdEventSupplier singleton does not exist, create it
    //

    NotifdEventSupplier *_event_supplier = new NotifdEventSupplier(
        _orb, ns.SupAdm, ns.pID, ns.ProCon, ns.StrProPush, ns.EveChaFac, ns.EveCha, ns.ec_ior, tg);

    return _event_supplier;
}

void NotifdEventSupplier::connect()
{
    //
    // Connect to the Proxy Consumer
    //
    try
    {
        structuredProxyPushConsumer->connect_structured_push_supplier(_this());
    }
    catch(const CosEventChannelAdmin::AlreadyConnected &)
    {
        std::cerr << "Tango::NotifdEventSupplier::connect() caught AlreadyConnected exception" << std::endl;
    }
}

void NotifdEventSupplier::disconnect_structured_push_supplier()
{
    TANGO_LOG_DEBUG << "calling Tango::NotifdEventSupplier::disconnect_structured_push_supplier() \n";
}

void NotifdEventSupplier::subscription_change(TANGO_UNUSED(const CosNotification::EventTypeSeq &added),
                                              TANGO_UNUSED(const CosNotification::EventTypeSeq &deled))
{
    TANGO_LOG_DEBUG << "calling Tango::NotifdEventSupplier::subscription_change() \n";
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventSupplier::connect_to_notifd()
//
// description :     Method to connect the process to the notifd
//
// argument : in :    ns : Ref. to a struct with notifd connection parameters
//
//-----------------------------------------------------------------------------

void NotifdEventSupplier::connect_to_notifd(NotifService &ns,
                                            const CORBA::ORB_var &_orb,
                                            const std::string &server_name,
                                            Util *tg)
{
    CosNotifyChannelAdmin::EventChannelFactory_var _eventChannelFactory;
    CosNotifyChannelAdmin::EventChannel_var _eventChannel;

    //
    // Get a reference to the Notification Service EventChannelFactory from
    // the TANGO database or from the server itself in case of server
    // started with the -file option
    //

    std::string &host_name = tg->get_host_name();

    std::string factory_ior;
    std::string factory_name;
    factory_name = "notifd/factory/" + host_name;
    CORBA::Any_var received;
    std::string d_name = "DServer/";
    d_name = d_name + server_name;
    const DevVarLongStringArray *dev_import_list;

    Database *db = tg->get_database();

    //
    // For compatibility reason, search in database first with a fqdn name
    // If nothing is found in DB and if the provided name is a FQDN, redo the
    // search with a canonical name. In case the DbCache, is used, this algo
    // is implemented in the stored procedure
    //

    if(!tg->use_file_db())
    {
        try
        {
            if(tg->get_db_cache())
            {
                dev_import_list = tg->get_db_cache()->import_notifd_event();
            }
            else
            {
                try
                {
                    received = db->import_event(factory_name);
                }
                catch(Tango::DevFailed &e)
                {
                    std::string reason(e.errors[0].reason.in());
                    if(reason == DB_DeviceNotDefined)
                    {
                        std::string::size_type pos = factory_name.find('.');
                        if(pos != std::string::npos)
                        {
                            std::string factory_name_canon = factory_name.substr(0, pos);
                            received = db->import_event(factory_name_canon);
                        }
                        else
                        {
                            throw;
                        }
                    }
                    else
                    {
                        throw;
                    }
                }
            }
        }
        catch(...)
        {
            //
            // Impossible to connect to notifd. In case there is already an entry in the db
            // for this server event channel, clear it.
            //

            if(tg->is_svr_starting())
            {
                try
                {
                    db->unexport_event(d_name);
                }
                catch(...)
                {
                }

                //
                // There is a TANGO_LOG and a cerr here to have the message displayed on the console
                // AND sent to the logging system (TANGO_LOG is redirected to the logging when
                // compiled with -D_TANGO_LIB)
                //

                TANGO_LOG_DEBUG << "Failed to import EventChannelFactory " << factory_name << " from the Tango database"
                                << std::endl;
            }

            TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept,
                                           API_NotificationServiceFailed,
                                           "Failed to import the EventChannelFactory from the Tango database");
        }

        if(!tg->get_db_cache())
        {
            received.inout() >>= dev_import_list;
        }
        factory_ior = std::string((dev_import_list->svalue)[1]);
    }
    else
    {
        Tango::DbDatum na;

        std::string cl_name("notifd");
        try
        {
            na = db->get_device_name(server_name, cl_name);
        }
        catch(Tango::DevFailed &)
        {
            if(tg->is_svr_starting())
            {
                TANGO_LOG_DEBUG << "Failed to import EventChannelFactory from the Device Server property file"
                                << std::endl;
                TANGO_LOG_DEBUG << "Notifd event will not be generated" << std::endl;
            }

            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to import the EventChannelFactory from the Device Server property file");
        }

        factory_ior = na.value_string[0];
    }

    try
    {
        CORBA::Object *event_factory_obj;
        event_factory_obj = _orb->string_to_object(factory_ior.c_str());

//        omniORB::setClientConnectTimeout(NARROW_CLNT_TIMEOUT);
#ifndef _TG_WINDOWS_
        if(event_factory_obj->_non_existent())
        {
            event_factory_obj = CORBA::Object::_nil();
        }
#else
        if(!tg->use_file_db())
        {
            if((dev_import_list->lvalue)[0] == 0)
            {
                TANGO_THROW_EXCEPTION("aaa", "bbb");
            }
        }
#endif /* _TG_WINDOWS_ */

        //
        // Narrow the CORBA_Object reference to an EventChannelFactory
        // reference so we can invoke its methods
        //

        _eventChannelFactory = CosNotifyChannelAdmin::EventChannelFactory::_narrow(event_factory_obj);
        //        omniORB::setClientConnectTimeout(0);

        //
        // Make sure the CORBA object was really an EventChannelFactory
        //

        if(CORBA::is_nil(_eventChannelFactory))
        {
            std::cerr << factory_name << " is not an EventChannelFactory " << std::endl;
            TANGO_THROW_DETAILED_EXCEPTION(EventSystemExcept,
                                           API_NotificationServiceFailed,
                                           "Failed to import the EventChannelFactory from the Tango database");
        }
    }
    catch(...)
    {
        //        omniORB::setClientConnectTimeout(0);

        //
        // Impossible to connect to notifd. In case there is already an entry in the db
        // for this server event channel, clear it.
        //

        if(!tg->use_file_db() && (tg->is_svr_starting()))
        {
            try
            {
                db->unexport_event(d_name);
            }
            catch(...)
            {
            }
        }

        //
        // There is a TANGO_LOG and a cerr here to have the message displayed on the console
        // AND sent to the logging system (TANGO_LOG is redirected to the logging when
        // compiled with -D_TANGO_LIB)
        // Print these messages only during DS startup sequence
        //

        if(tg->is_svr_starting())
        {
            TANGO_LOG_DEBUG << "Failed to narrow the EventChannelFactory - Notifd events will not be generated (hint: "
                               "start the notifd daemon on this host)"
                            << std::endl;
        }

        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_NotificationServiceFailed,
            "Failed to narrow the EventChannelFactory, make sure the notifd process is running on this host");
    }

    //
    // Get a reference to an EventChannel for this device server from the
    // TANGO database
    //

    int channel_exported = -1;
    std::string channel_ior, channel_host;

    if(!tg->use_file_db())
    {
        try
        {
            if(tg->get_db_cache())
            {
                dev_import_list = tg->get_db_cache()->DbServerCache::import_adm_event();
            }
            else
            {
                received = db->import_event(d_name);
            }
        }
        catch(...)
        {
            //
            // There is a TANGO_LOG and a cerr here to have the message displayed on the console
            // AND sent to the logging system (TANGO_LOG is redirected to the logging when
            // compiled with -D_TANGO_LIB)
            //

            std::cerr << d_name << " has no event channel defined in the database - creating it " << std::endl;
            TANGO_LOG << d_name << " has no event channel defined in the database - creating it " << std::endl;

            channel_exported = 0;
        }

        if(channel_exported != 0)
        {
            if(!tg->get_db_cache())
            {
                received.inout() >>= dev_import_list;
            }
            channel_ior = std::string((dev_import_list->svalue)[1]);
            channel_exported = dev_import_list->lvalue[0];

            //
            // check if the channel is exported on this host, if not assume it
            // is an old channel and we need to recreate it on the local host
            //

            channel_host = std::string((dev_import_list->svalue)[3]);
            if(channel_host != host_name)
            {
                channel_exported = 0;
            }
        }
    }
    else
    {
        try
        {
            Tango::DbDatum na;

            std::string cl_name(NOTIFD_CHANNEL);
            na = db->get_device_name(server_name, cl_name);
            channel_ior = na.value_string[0];
            channel_exported = 1;
        }
        catch(Tango::DevFailed &)
        {
            channel_exported = 0;
        }
    }

    if(channel_exported != 0)
    {
        CORBA::Object *event_channel_obj;
        event_channel_obj = _orb->string_to_object(channel_ior.c_str());

        try
        {
            if(event_channel_obj->_non_existent())
            {
                event_channel_obj = CORBA::Object::_nil();
            }

            _eventChannel = CosNotifyChannelAdmin::EventChannel::_nil();
            _eventChannel = CosNotifyChannelAdmin::EventChannel::_narrow(event_channel_obj);

            if(CORBA::is_nil(_eventChannel))
            {
                channel_exported = 0;
            }
        }
        catch(...)
        {
            TANGO_LOG_DEBUG << "caught exception while trying to test event_channel object\n";
            channel_exported = 0;
        }
    }

    //
    // The device server event channel does not exist, let's create a new one
    //

    if(channel_exported == 0)
    {
        CosNotification::QoSProperties initialQoS;
        CosNotification::AdminProperties initialAdmin;
        CosNotifyChannelAdmin::ChannelID channelId;

        try
        {
            _eventChannel = _eventChannelFactory->create_channel(initialQoS, initialAdmin, channelId);
            TANGO_LOG_DEBUG << "Tango::NotifdEventSupplier::create() channel for server " << d_name << " created\n";
            char *_ior = _orb->object_to_string(_eventChannel);
            std::string ior_string(_ior);

            if(!tg->use_file_db())
            {
                auto *eve_export_list = new Tango::DevVarStringArray;
                eve_export_list->length(5);
                (*eve_export_list)[0] = Tango::string_dup(d_name.c_str());
                (*eve_export_list)[1] = Tango::string_dup(ior_string.c_str());
                (*eve_export_list)[2] = Tango::string_dup(host_name.c_str());
                std::ostringstream ostream;
                ostream << getpid() << std::ends;
                (*eve_export_list)[3] = Tango::string_dup(ostream.str().c_str());
                (*eve_export_list)[4] = Tango::string_dup("1");

                bool retry = true;
                int ctr = 0;
                int db_to = db->get_timeout_millis();

                db->set_timeout_millis(db_to * 2);

                while((retry) && (ctr < 4))
                {
                    try
                    {
                        db->export_event(eve_export_list);
                        retry = false;
                    }
                    catch(Tango::CommunicationFailed &)
                    {
                        ctr++;
                    }
                }

                db->set_timeout_millis(db_to);

                TANGO_LOG_DEBUG << "successfully  exported event channel to Tango database !\n";
            }
            else
            {
                //
                // In case of DS started with -file option, store the
                // event channel within the event supplier object and in the file
                //

                ns.ec_ior = ior_string;

                try
                {
                    db->write_event_channel_ior_filedatabase(ior_string);
                }
                catch(Tango::DevFailed &)
                {
                }
            }
            Tango::string_free(_ior);
        }
        catch(const CosNotification::UnsupportedQoS &)
        {
            std::cerr << "Failed to create event channel - events will not be generated (hint: start the notifd daemon "
                         "on this host)"
                      << std::endl;
            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to create a new EventChannel, make sure the notifd process is running on this host");
        }
        catch(const CosNotification::UnsupportedAdmin &)
        {
            std::cerr << "Failed to create event channel - events will not be generated (hint: start the notifd daemon "
                         "on this host)"
                      << std::endl;
            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to create a new EventChannel, make sure the notifd process is running on this host");
        }
    }
    else
    {
        TANGO_LOG_DEBUG << "Tango::NotifdEventSupplier::create(): _narrow worked, use this event channel\n";
        if(tg->use_file_db())
        {
            ns.ec_ior = channel_ior;
        }
    }

    //
    // Obtain a Supplier Admin
    //

    //
    // We'll use the channel's default Supplier admin
    //

    CosNotifyChannelAdmin::SupplierAdmin_var _supplierAdmin = _eventChannel->default_supplier_admin();
    if(CORBA::is_nil(_supplierAdmin))
    {
        std::cerr << "Could not get CosNotifyChannelAdmin::SupplierAdmin" << std::endl;
        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_NotificationServiceFailed,
            "Failed to get the default supplier admin from the notification daemon (hint: make "
            "sure the notifd process is running on this host)");
    }

    //
    // If necessary, clean up remaining proxies left by previous run of the DS
    // which did a core dump (or similar)
    //

    if(tg->is_svr_starting())
    {
        CosNotifyChannelAdmin::ProxyIDSeq_var proxies;

        proxies = _supplierAdmin->push_consumers();
        if(proxies->length() >= 1)
        {
            for(unsigned int loop = 0; loop < proxies->length(); loop++)
            {
                CosNotifyChannelAdmin::ProxyConsumer_var _tmp_proxyConsumer;
                _tmp_proxyConsumer = _supplierAdmin->get_proxy_consumer(proxies[loop]);

                CosNotifyChannelAdmin::StructuredProxyPushConsumer_var _tmp_structuredProxyPushConsumer;
                _tmp_structuredProxyPushConsumer =
                    CosNotifyChannelAdmin::StructuredProxyPushConsumer::_narrow(_tmp_proxyConsumer);
                if(CORBA::is_nil(_tmp_structuredProxyPushConsumer))
                {
                    continue;
                }

                try
                {
                    _tmp_structuredProxyPushConsumer->disconnect_structured_push_consumer();
                }
                catch(CORBA::Exception &)
                {
                }
            }
        }
    }

    //
    // Obtain a Proxy Consumer
    //

    //
    // We are using the "Push" model and Structured data
    //

    CosNotifyChannelAdmin::ProxyID _proxyId;
    CosNotifyChannelAdmin::ProxyConsumer_var _proxyConsumer;
    try
    {
        _proxyConsumer =
            _supplierAdmin->obtain_notification_push_consumer(CosNotifyChannelAdmin::STRUCTURED_EVENT, _proxyId);
        if(CORBA::is_nil(_proxyConsumer))
        {
            std::cerr << "Could not get CosNotifyChannelAdmin::ProxyConsumer" << std::endl;
            TANGO_THROW_DETAILED_EXCEPTION(
                EventSystemExcept,
                API_NotificationServiceFailed,
                "Failed to obtain a Notification push consumer, make sure the notifd process is running on this host");
        }
    }
    catch(const CosNotifyChannelAdmin::AdminLimitExceeded &)
    {
        std::cerr << "Failed to get push consumer from notification daemon - events will not be generated (hint: start "
                     "the notifd daemon on this host)"
                  << std::endl;
        TANGO_THROW_DETAILED_EXCEPTION(
            EventSystemExcept,
            API_NotificationServiceFailed,
            "Failed to get push consumer from notification daemon (hint: make sure the notifd "
            "process is running on this host)");
    }

    CosNotifyChannelAdmin::StructuredProxyPushConsumer_var _structuredProxyPushConsumer =
        CosNotifyChannelAdmin::StructuredProxyPushConsumer::_narrow(_proxyConsumer);
    if(CORBA::is_nil(_structuredProxyPushConsumer))
    {
        std::cerr
            << "Tango::NotifdEventSupplier::create() could not get CosNotifyChannelAdmin::StructuredProxyPushConsumer"
            << std::endl;
    }

    //
    // Init returned value
    //

    ns.SupAdm = _supplierAdmin;
    ns.pID = _proxyId;
    ns.ProCon = _proxyConsumer;
    ns.StrProPush = _structuredProxyPushConsumer;
    ns.EveChaFac = _eventChannelFactory;
    ns.EveCha = _eventChannel;
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventSupplier::push_heartbeat_event()
//
// description :     Method to send the hearbeat event
//
// argument : in :
//
//-----------------------------------------------------------------------------

void NotifdEventSupplier::push_heartbeat_event()
{
    CosNotification::StructuredEvent struct_event;
    std::string domain_name;
    time_t delta_time;
    time_t now_time;
    static int heartbeat_counter = 0;

    //
    // Heartbeat - check wether a heartbeat event has been sent recently
    // if not then send it. A heartbeat contains no data, it is used by the
    // consumer to know that the supplier is still alive.
    //

    Tango::Util *tg = Tango::Util::instance();
    DServer *adm_dev = tg->get_dserver_device();
    now_time = Tango::get_current_system_datetime();
    delta_time = now_time - adm_dev->last_heartbeat;
    TANGO_LOG_DEBUG << "NotifdEventSupplier::push_heartbeat_event(): delta time since last heartbeat " << delta_time
                    << std::endl;

    //
    // We here compare delta_time to 8 and not to 10.
    // This is necessary because, sometimes the polling thread is some
    // milli second in advance. The computation here is done in seconds
    // So, if the polling thread is in advance, delta_time computed in
    // seconds will be 9 even if in reality it is 9,9
    //

    if(delta_time >= 8)
    {
        domain_name = "dserver/" + adm_dev->get_full_name();

        struct_event.header.fixed_header.event_type.domain_name = Tango::string_dup(domain_name.c_str());
        struct_event.header.fixed_header.event_type.type_name = Tango::string_dup(fqdn_prefix.c_str());
        struct_event.header.variable_header.length(0);

        TANGO_LOG_DEBUG << "NotifdEventSupplier::push_heartbeat_event(): detected heartbeat event for " << domain_name
                        << std::endl;
        TANGO_LOG_DEBUG << "NotifdEventSupplier::push_heartbeat_event(): delta _time " << delta_time << std::endl;
        struct_event.header.fixed_header.event_name = Tango::string_dup("heartbeat");
        struct_event.filterable_data.length(1);
        struct_event.filterable_data[0].name = Tango::string_dup("heartbeat_counter");
        struct_event.filterable_data[0].value <<= (CORBA::Long) heartbeat_counter++;
        adm_dev->last_heartbeat = now_time;

        struct_event.remainder_of_body <<= (CORBA::Long) adm_dev->last_heartbeat;

        //
        // Push the event
        //

        bool fail = false;
        try
        {
            structuredProxyPushConsumer->push_structured_event(struct_event);
        }
        catch(const CosEventComm::Disconnected &)
        {
            TANGO_LOG_DEBUG << "NotifdEventSupplier::push_heartbeat_event() event channel disconnected !\n";
            fail = true;
        }
        catch(const CORBA::TRANSIENT &)
        {
            TANGO_LOG_DEBUG << "NotifdEventSupplier::push_heartbeat_event() caught a CORBA::TRANSIENT ! " << std::endl;
            fail = true;
        }
        catch(const CORBA::COMM_FAILURE &)
        {
            TANGO_LOG_DEBUG << "NotifdEventSupplier::push_heartbeat_event() caught a CORBA::COMM_FAILURE ! "
                            << std::endl;
            fail = true;
        }
        catch(const CORBA::SystemException &)
        {
            TANGO_LOG_DEBUG << "NotifdEventSupplier::push_heartbeat_event() caught a CORBA::SystemException ! "
                            << std::endl;
            fail = true;
        }

        //
        // If it was not possible to communicate with notifd,
        // try a reconnection
        //

        if(fail)
        {
            try
            {
                reconnect_notifd();
            }
            catch(...)
            {
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventSupplier::reconnect_notifd()
//
// description :     Method to reconnect to the notifd
//
// argument : in :
//
//-----------------------------------------------------------------------------

void NotifdEventSupplier::reconnect_notifd()
{
    //
    // Check notifd but trying to read an attribute of the event channel
    // If it works, we immediately return
    //

    try
    {
        CosNotifyChannelAdmin::EventChannelFactory_var ecf = eventChannel->MyFactory();
        return;
    }
    catch(...)
    {
        TANGO_LOG_DEBUG << "Notifd dead !!!!!!" << std::endl;
    }

    //
    // Reconnect process to notifd after forcing
    // process to re-read the file database
    // in case it is used
    //

    try
    {
        NotifService ns;
        Tango::Util *tg = Tango::Util::instance();
        Database *db = tg->get_database();

        if(tg->use_file_db())
        {
            db->reread_filedatabase();
        }

        connect_to_notifd(ns, orb_, tg->get_ds_name(), tg);

        supplierAdmin = ns.SupAdm;
        proxyId = ns.pID;
        proxyConsumer = ns.ProCon;
        structuredProxyPushConsumer = ns.StrProPush;
        eventChannelFactory = ns.EveChaFac;
        eventChannel = ns.EveCha;
        event_channel_ior = ns.ec_ior;
    }
    catch(...)
    {
        TANGO_LOG_DEBUG << "Can't reconnect.............." << std::endl;
    }

    connect();
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventSupplier::disconnect_from_notifd()
//
// description :     Method to disconnect the DS from the notifd event channel
//
//-----------------------------------------------------------------------------

void NotifdEventSupplier::disconnect_from_notifd()
{
    try
    {
        structuredProxyPushConsumer->disconnect_structured_push_consumer();
    }
    catch(CORBA::Exception &)
    {
    }
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventSupplier::push_event()
//
// description :     Method to send the event to the event channel
//
// argument : in :    device_impl : The device
//                    event_type : The event type (change, periodic....)
//                    filterable_names :
//                    filterable_data :
//                    attr_value : The attribute value
//                    except : The exception thrown during the last
//                        attribute reading. nullptr if no exception
//
//-----------------------------------------------------------------------------

void NotifdEventSupplier::push_event(DeviceImpl *device_impl,
                                     std::string event_type,
                                     const std::vector<std::string> &filterable_names,
                                     const std::vector<double> &filterable_data,
                                     const std::vector<std::string> &filterable_names_lg,
                                     const std::vector<long> &filterable_data_lg,
                                     const struct SuppliedEventData &attr_value,
                                     const std::string &attr_name,
                                     DevFailed *except,
                                     TANGO_UNUSED(bool inc_cptr))
{
    CosNotification::StructuredEvent struct_event;
    std::string domain_name;

    TANGO_LOG_DEBUG << "NotifdEventSupplier::push_event(): called for attribute " << attr_name << std::endl;

    //
    // If we are called for IDL 5 (AttributeValue_5 or AttributeConfig_5), simply return. IDL 5 is only for ZMQ
    //

    if(attr_value.attr_conf_5 != nullptr || attr_value.attr_val_5 != nullptr)
    {
        return;
    }

    // get the mutex to synchronize the sending of events
    omni_mutex_lock oml(push_mutex);

    std::string loc_attr_name = attr_name;
    std::transform(loc_attr_name.begin(), loc_attr_name.end(), loc_attr_name.begin(), ::tolower);
    domain_name = device_impl->get_name_lower() + "/" + loc_attr_name;

    event_type = detail::remove_idl_prefix(event_type);

    struct_event.header.fixed_header.event_type.domain_name = Tango::string_dup(domain_name.c_str());
    struct_event.header.fixed_header.event_type.type_name = Tango::string_dup(fqdn_prefix.c_str());

    struct_event.header.variable_header.length(0);

    unsigned long nb_filter = filterable_names.size();
    unsigned long nb_filter_lg = filterable_names_lg.size();

    struct_event.filterable_data.length(nb_filter + nb_filter_lg);

    if(nb_filter != 0)
    {
        if(nb_filter == filterable_data.size())
        {
            for(unsigned long i = 0; i < nb_filter; i++)
            {
                struct_event.filterable_data[i].name = Tango::string_dup(filterable_names[i].c_str());
                struct_event.filterable_data[i].value <<= (CORBA::Double) filterable_data[i];
            }
        }
    }

    if(nb_filter_lg != 0)
    {
        if(nb_filter_lg == filterable_data_lg.size())
        {
            for(unsigned long i = 0; i < nb_filter_lg; i++)
            {
                struct_event.filterable_data[i + nb_filter].name = Tango::string_dup(filterable_names_lg[i].c_str());
                struct_event.filterable_data[i + nb_filter].value <<= (CORBA::Long) filterable_data_lg[i];
            }
        }
    }

    if(except == nullptr)
    {
        if(attr_value.attr_val != nullptr)
        {
            struct_event.remainder_of_body <<= (*attr_value.attr_val);
        }
        else if(attr_value.attr_val_3 != nullptr)
        {
            struct_event.remainder_of_body <<= (*attr_value.attr_val_3);
        }
        else if(attr_value.attr_val_4 != nullptr)
        {
            struct_event.remainder_of_body <<= (*attr_value.attr_val_4);

            //
            // Insertion in the event structure Any also copy the mutex ptr.
            // When this event structure will be deleted (at the end of this method),
            // the struct inserted into the Any will also be deleted.
            // This will unlock the mutex....
            // Clean the mutex ptr in the structure inserted in the Any to prevent this
            // bad behavior
            //

            const Tango::AttributeValue_4 *tmp_ptr;
            struct_event.remainder_of_body >>= tmp_ptr;
            const_cast<Tango::AttributeValue_4 *>(tmp_ptr)->mut_ptr = nullptr;
        }
        else if(attr_value.attr_val_5 != nullptr)
        {
            std::string str("Can't send event! Client is too old (Tango 7 or less).\n");
            str = str + ("Please, re-compile your client with at least Tango 8");

            std::cerr << str << std::endl;
            TANGO_THROW_EXCEPTION(API_NotSupported, str);
        }
        else if(attr_value.attr_conf_2 != nullptr)
        {
            struct_event.remainder_of_body <<= (*attr_value.attr_conf_2);
        }
        else if(attr_value.attr_conf_3 != nullptr)
        {
            struct_event.remainder_of_body <<= (*attr_value.attr_conf_3);
        }
        else
        {
            struct_event.remainder_of_body <<= (*attr_value.attr_dat_ready);
        }
    }
    else
    {
        struct_event.remainder_of_body <<= except->errors;
    }
    struct_event.header.fixed_header.event_name = Tango::string_dup(event_type.c_str());

    TANGO_LOG_DEBUG << "EventSupplier::push_event(): push event " << event_type << " for "
                    << device_impl->get_name() + "/" + attr_name << std::endl;

    //
    // Push the event
    //

    bool fail = false;
    try
    {
        structuredProxyPushConsumer->push_structured_event(struct_event);
    }
    catch(const CosEventComm::Disconnected &)
    {
        TANGO_LOG_DEBUG << "EventSupplier::push_event() event channel disconnected !\n";
        fail = true;
    }
    catch(const CORBA::TRANSIENT &)
    {
        TANGO_LOG_DEBUG << "EventSupplier::push_event() caught a CORBA::TRANSIENT ! " << std::endl;
        fail = true;
    }
    catch(const CORBA::COMM_FAILURE &)
    {
        TANGO_LOG_DEBUG << "EventSupplier::push_event() caught a CORBA::COMM_FAILURE ! " << std::endl;
        fail = true;
    }
    catch(const CORBA::SystemException &)
    {
        TANGO_LOG_DEBUG << "EventSupplier::push_event() caught a CORBA::SystemException ! " << std::endl;
        fail = true;
    }
    catch(...)
    {
        fail = true;
    }

    //
    // If it was not possible to communicate with notifd, try a reconnection
    //

    if(fail)
    {
        try
        {
            reconnect_notifd();
        }
        catch(...)
        {
        }
    }
}

//+----------------------------------------------------------------------------
//
// method :         NotifdEventSupplier::file_db_svr()
//
// description :     In case of DS using file as database, set a marker in the
//                  fqdn_prefix
//
//-----------------------------------------------------------------------------

void NotifdEventSupplier::file_db_svr()
{
    fqdn_prefix = fqdn_prefix + '#';
}

} // namespace Tango
