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
    sigset_t sigs_to_catch;

    //
    // Catch main signals
    //

    sigemptyset(&sigs_to_catch);

    sigaddset(&sigs_to_catch, SIGINT);
    sigaddset(&sigs_to_catch, SIGTERM);
    sigaddset(&sigs_to_catch, SIGHUP);
    sigaddset(&sigs_to_catch, SIGQUIT);

    //
    // Init thread id and pid (for linux !!)
    //

    my_thread = pthread_self();
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
#ifndef _TG_WINDOWS_
        int ret = sigwait(&sigs_to_catch, &signo);
        // sigwait() under linux might return an errno number without initialising the
        // signo variable. Do a ckeck here to avoid problems!!!
        if(ret != 0)
        {
            TANGO_LOG_DEBUG << "Signal thread awaken on error " << ret << std::endl;
            continue;
        }

        TANGO_LOG_DEBUG << "Signal thread awaken for signal " << sig_name[signo] << std::endl;

        if(signo == SIGHUP)
        {
            continue;
        }
#else
        WaitForSingleObject(ds->win_ev, INFINITE);
        signo = ds->win_signo;

        TANGO_LOG_DEBUG << "Signal thread awaken for signal " << sig_name[signo] << std::endl;
#endif

        //
        // Respond to possible command from the DServerSignal object.  Either:
        // - stop this thread
        // - add a new signal to catch in the mask (Linux/macOS only)
        //

        {
            omni_mutex_lock sy(*ds);
            if(signo == SIGINT)
            {
                // We check for a stop request first in case multiple SIGINT
                // signals have been merged by the kernel
                if(ds->sig_th_should_stop)
                {
                    TANGO_LOG_DEBUG << "ThSig stop requested by DSignalServer singleton" << std::endl;
                    break;
                }

#ifndef _TG_WINDOWS_
                bool job_done = false;

                if(ds->sig_to_install)
                {
                    ds->sig_to_install = false;
                    sigaddset(&sigs_to_catch, ds->inst_sig);
                    TANGO_LOG_DEBUG << "signal " << ds->inst_sig << " installed" << std::endl;
                    job_done = true;
                }

                //
                // Remove a signal from the catched one
                //

                if(ds->sig_to_remove)
                {
                    ds->sig_to_remove = false;
                    sigdelset(&sigs_to_catch, ds->rem_sig);
                    TANGO_LOG_DEBUG << "signal " << ds->rem_sig << " removed" << std::endl;
                    job_done = true;
                }

                if(job_done)
                {
                    ds->signal();
                    continue;
                }
#endif
            }
        }

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
