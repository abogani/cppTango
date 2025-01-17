//+=============================================================================
//
// file :               thsig.cpp
//
// description :        C++ source for the DServer class and its commands.
//            The class is derived from Device. It represents the
//            CORBA servant object which will be accessed from the
//            network. All commands which can be executed on a
//            DServer object are implemented in this file.
//
// project :            TANGO
//
// author(s) :          A.Gotz + E.Taurel
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

#include <tango/tango.h>
#include <tango/server/dserversignal.h>

namespace Tango
{

void *DServerSignal::ThSig::run_undetached(TANGO_UNUSED(void *ptr))
{
    is_tango_library_thread = true;

#ifndef _TG_WINDOWS_
    //
    // Block all signals on this thread.
    // This avoids running into a potential deadlock where a signal is delivered
    // to this thread while signal_queue.get() is temporarily holding the
    // underlying mutex. This mutext in turn is also locked by the signal handler when
    // putting the signo into the queue via signal_queue.put(int)
    //
    sigset_t sigs_to_block;
    sigfillset(&sigs_to_block);
    pthread_sigmask(SIG_BLOCK, &sigs_to_block, nullptr);

    //
    // Init pid (for linux !!)
    //
    {
        omni_mutex_lock syn(*ds);
        my_pid = getpid();
        ds->signal();
    }
#endif

    int signo = 0;

    //
    // The infinite loop
    //

    while(true)
    {
        signo = ds->signal_queue.get();
        if(signo == STOP_SIGNAL_THREAD)
        {
            TANGO_LOG_DEBUG << "ThSig stop requested by DSignalServer singleton" << std::endl;
            break;
        }

        TANGO_LOG_DEBUG << "Signal thread awaken for signal " << sig_name[signo] << std::endl;

#ifndef _TG_WINDOWS_
        if(signo == SIGHUP)
        {
            continue;
        }
#endif

        DServerSignal::deliver_to_registered_handlers(signo);

        //
        // For the automatically installed signal, unregister servers from database,
        // destroy the ORB and exit
        //

        if(auto_signal(signo))
        {
            Tango::Util *tg = Tango::Util::instance();
            if(tg != nullptr && !tg->is_svr_shutting_down())
            {
                try
                {
                    tg->shutdown_ds();
                }
                catch(...)
                {
#ifndef _TG_WINDOWS_
                    raise(SIGKILL);
#endif
                }
            }
        }
    }

    return nullptr;
}

} // namespace Tango
