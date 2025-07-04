//+=============================================================================
//
// file :               utils_shut.cpp
//
// description :        C++ source for some methods of the Util class related
//                        to server shutdown
//
// project :            TANGO
//
// author(s) :             E.Taurel
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
//-=============================================================================

#include <tango/client/eventconsumer.h>
#include <tango/server/utils.h>
#include <tango/server/dserver.h>
#include <tango/server/eventsupplier.h>
#include <tango/client/Database.h>

namespace Tango
{
//+----------------------------------------------------------------------------
//
// method :         Util::shutdown_ds()
//
// description :     This method sends command to the polling thread for
//            all cmd/attr with polling configuration stored in db.
//            This is done in separate thread in order to equally
//            spread all the polled objects polling time on the
//            smallest polling period.
//
//-----------------------------------------------------------------------------

void Util::shutdown_ds()
{
    //
    // Stopping a device server means :
    //        - Mark the server as shutting down
    //        - Send kill command to the polling thread
    //        - Join with this polling thread
    //        - Unregister server in database
    //        - Delete devices (except the admin one)
    //        - Stop the KeepAliveThread and the EventConsumer Thread when
    //          they have been started to receive events
    //        - Force writing file database in case of
    //        - Shutdown the ORB
    //

    set_svr_shutting_down(true);

    //
    // send the exit command to all the polling threads in the pool
    //

    stop_all_polling_threads();
    stop_heartbeat_thread();
    clr_heartbeat_th_ptr();

    //
    // Unregister the server in the database
    //

    try
    {
        unregister_server();
    }
    catch(...)
    {
    }

    //
    // Delete the devices (except the admin one)
    //

    get_dserver_device()->delete_devices();

    //
    //     Stop the KeepAliveThread and the EventConsumer thread when
    //  they have been started to receive events.
    //

    ApiUtil *au = ApiUtil::instance();
    au->shutdown_event_consumers();

    //
    // Disconnect the server from the notifd, when it was connected
    //

    NotifdEventSupplier *ev = get_notifd_event_supplier();
    if(ev != nullptr)
    {
        ev->disconnect_from_notifd();
    }

    //
    // Delete ZmqEventSupplier
    //

    ZmqEventSupplier *zev = get_zmq_event_supplier();
    delete zev;

    //
    // Close access to file database when used
    //

    if(_FileDb)
    {
        Database *db_ptr = get_database();
        db_ptr->write_filedatabase();
        delete db_ptr;
        TANGO_LOG_DEBUG << "Database object deleted" << std::endl;
    }

    //
    // If the server uses its own event loop, do not call it any more
    //

    if(is_server_event_loop_set())
    {
        set_shutdown_server(true);
    }

    //
    // Shutdown the ORB
    //

    TANGO_LOG_DEBUG << "Going to shutdown ORB" << std::endl;
    CORBA::ORB_var loc_orb = get_orb();
    loc_orb->shutdown(true);

    TANGO_LOG_DEBUG << "ORB shutdown" << std::endl;
}

} // namespace Tango
