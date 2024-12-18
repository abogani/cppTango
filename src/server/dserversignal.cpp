//+==================================================================================================================
//
// file :               DServerSignal.cpp
//
// description :        C++ source for the DServer class and its commands. The class is derived from Device.
//                        It represents the CORBA servant object which will be accessed from the network.
//                        All commands which can be executed on a DServer object are implemented in this file.
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
//-===================================================================================================================

#include <tango/tango.h>
#include <new>
#include <tango/server/dserversignal.h>

#ifndef _TG_WINDOWS_
  #include <cerrno>
#endif

namespace Tango
{

std::unique_ptr<DServerSignal> DServerSignal::_instance = nullptr;
DevSigAction DServerSignal::reg_sig[_NSIG];
std::string DServerSignal::sig_name[_NSIG];
#ifdef _TG_WINDOWS_
HANDLE DServerSignal::win_ev = nullptr;
int DServerSignal::win_signo = 0;
#endif

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::Instance()
//
// description :
//        Instance method for DServerSignal object. This class is a singleton and this method creates the object the
//        first time it is called or simply returns a pointer to the already created object for all the other calls.
//
//-------------------------------------------------------------------------------------------------------------------

DServerSignal *DServerSignal::instance()
{
    if(_instance == nullptr)
    {
        try
        {
            _instance.reset(new DServerSignal());
        }
        catch(std::bad_alloc &)
        {
            throw;
        }
    }
    return _instance.get();
}

void DServerSignal::cleanup_singleton()
{
    _instance.reset(nullptr);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::DServerSignal()
//
// description :
//        constructor for DServerSignal object. As this class is a singleton, this method is protected
//
//-----------------------------------------------------------------------------------------------------------------

DServerSignal::DServerSignal() :
    TangoMonitor("signal")
{
    TANGO_LOG_DEBUG << "Entering DServerSignal constructor" << std::endl;

    //
    // Set array of signal name
    //

#ifdef _TG_WINDOWS_
    sig_name[SIGINT] = "SIGINT";
    sig_name[SIGILL] = "SIGILL";
    sig_name[SIGFPE] = "SIGFPE";
    sig_name[SIGSEGV] = "SIGSEGV";
    sig_name[SIGTERM] = "SIGTERM";
    sig_name[SIGBREAK] = "SIGBREAK";
    sig_name[SIGABRT] = "SIGABRT";
#else
    sig_name[SIGHUP] = "SIGHUP";
    sig_name[SIGINT] = "SIGINT";
    sig_name[SIGQUIT] = "SIGQUIT";
    sig_name[SIGILL] = "SIGILL";
    sig_name[SIGTRAP] = "SIGTRAP";
    sig_name[SIGABRT] = "SIGABRT";
    sig_name[SIGFPE] = "SIGFPE";
    sig_name[SIGKILL] = "SIGKILL";
    sig_name[SIGBUS] = "SIGBUS";
    sig_name[SIGSEGV] = "SIGSEGV";
    sig_name[SIGPIPE] = "SIGPIPE";
    sig_name[SIGALRM] = "SIGALRM";
    sig_name[SIGTERM] = "SIGTERM";
    sig_name[SIGUSR1] = "SIGUSR1";
    sig_name[SIGUSR2] = "SIGUSR2";
    sig_name[SIGCHLD] = "SIGCHLD";
    sig_name[SIGVTALRM] = "SIGVTALRM";
    sig_name[SIGPROF] = "SIGPROF";
    sig_name[SIGIO] = "SIGIO";
    sig_name[SIGWINCH] = "SIGWINCH";
    sig_name[SIGSTOP] = "SIGSTOP";
    sig_name[SIGTSTP] = "SIGTSTP";
    sig_name[SIGCONT] = "SIGCONT";
    sig_name[SIGTTIN] = "SIGTTIN";
    sig_name[SIGTTOU] = "SIGTTOU";
    sig_name[SIGURG] = "SIGURG";
    sig_name[SIGSYS] = "SIGSYS";
  #ifdef linux
    sig_name[SIGXCPU] = "SIGXCPU";
    sig_name[SIGXFSZ] = "SIGXFSZ";
    sig_name[SIGCLD] = "SIGCLD";
    sig_name[SIGPWR] = "SIGPWR";
  #else
    #ifdef __APPLE__
    sig_name[SIGEMT] = "SIGEMT";
    sig_name[SIGINFO] = "SIGINFO";
    #else
      #ifdef __freebsd__
    sig_name[SIGXCPU] = "SIGXCPU";
    sig_name[SIGXFSZ] = "SIGXFSZ";
      #endif /* __freebsd__ */
    #endif   /* __APPLE__ */
  #endif     /* linux */
#endif       /* _TG_WINDOWS_ */

    TangoSys_OMemStream o;
    for(long i = 0; i < _NSIG; i++)
    {
        if(sig_name[i].size() == 0)
        {
            o << i << std::ends;
            sig_name[i] = o.str();
            o.seekp(0);
            o.clear();
        }
    }

    sig_to_install = false;
    sig_to_remove = false;

#ifndef _TG_WINDOWS_
    //
    // With Solaris/Linux, the SIGINT and SIGQUIT default actions are set to SIG_IGN for all processes started in the
    // background (POSIX requirement). Signal SIGINT is used by Tango in its signal management, reset the default action
    // to default. SIGTERM must also be reset, or a SIGTERM handler created in a thread stops the server from exiting.
    //

    struct sigaction sa;

    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);

    for(int signum : {SIGINT, SIGQUIT, SIGTERM})
    {
        if(sigaction(signum, &sa, nullptr) == -1)
        {
            std::cerr << "DServerSignal::DServerSignal --> Can't reset to default action for " << sig_name[signum]
                      << ". Process might fail to terminate when " << sig_name[signum] << " is recieved." << std::endl;
        }
    }

    //
    // Block signals in thread other than the thread dedicated to signal
    //

    sigset_t sigs_to_block;
    sigemptyset(&sigs_to_block);

    sigfillset(&sigs_to_block);

    sigdelset(&sigs_to_block, SIGABRT);
    sigdelset(&sigs_to_block, SIGKILL);
    sigdelset(&sigs_to_block, SIGILL);
    sigdelset(&sigs_to_block, SIGTRAP);
    sigdelset(&sigs_to_block, SIGIOT);
    sigdelset(&sigs_to_block, SIGFPE);
    sigdelset(&sigs_to_block, SIGBUS);
    sigdelset(&sigs_to_block, SIGSEGV);
    sigdelset(&sigs_to_block, SIGSYS);
    sigdelset(&sigs_to_block, SIGPIPE);
    sigdelset(&sigs_to_block, SIGSTOP);

    sigdelset(&sigs_to_block, SIGTSTP);
    sigdelset(&sigs_to_block, SIGUSR1);
    sigdelset(&sigs_to_block, SIGUSR2);
    sigprocmask(SIG_BLOCK, &sigs_to_block, nullptr);
#else  /* _TG_WINDOWS_ */
    win_ev = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    register_handler(SIGINT);
    register_handler(SIGTERM);
    register_handler(SIGABRT);
    if(Util::_service == false)
    {
        register_handler(SIGBREAK);
    }
#endif /* _TG_WINDOWS_ */

    //
    // Start the thread dedicated to signal
    //

    sig_th = new ThSig(this);
    sig_th->start();

    TANGO_LOG_DEBUG << "leaving DServerSignal constructor" << std::endl;
}

DServerSignal::~DServerSignal()
{
    // Request that ThSig stops just in case we have got here because some other
    // thread is shutting the server down, in which case the ThSig might be
    // waiting forever for a signal.
    sig_th_should_stop = true;
#ifndef _TG_WINDOWS_
    pthread_kill(sig_th->my_thread, SIGINT);
#else
    win_signo = SIGINT;
    SetEvent(win_ev);
#endif
    try
    {
        sig_th->join(nullptr);
    }
    catch(...)
    {
        // We are in the middle of shutting down and we don't care if we cannot
        // join the thread, so we just continue here.
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::register_class_signal()
//
// description :
//        method to register a signal handler at the class level
//
// argument :
//         in :
//            - signo : Signal number
//            - cl_ptr : Pointer to device class object
//
//-----------------------------------------------------------------------------

#if defined _TG_WINDOWS_
void DServerSignal::register_class_signal(long signo, DeviceClass *cl_ptr)
#else
void DServerSignal::register_class_signal(long signo, bool handler, DeviceClass *cl_ptr)
#endif
{
    //
    // Check signal validity
    //

    if((signo < 1) || (signo >= _NSIG))
    {
        TangoSys_OMemStream o;
        o << "Signal number " << signo << " out of range" << std::ends;
        TANGO_THROW_EXCEPTION(API_SignalOutOfRange, o.str());
    }

#ifndef _TG_WINDOWS_
    if((auto_signal(signo)) && (handler))
    {
        TangoSys_OMemStream o;
        o << "Signal " << sig_name[signo] << "is not authorized using your own handler" << std::ends;
        TANGO_THROW_EXCEPTION(API_SignalOutOfRange, o.str());
    }
#endif

    //
    // If nothing is registered for this signal, install the OS signal handler
    //

    if(!auto_signal(signo))
    {
        if((reg_sig[signo].registered_devices.empty()) && (reg_sig[signo].registered_classes.empty()))
        {
#ifdef _TG_WINDOWS_
            register_handler(signo);
#else
            register_handler(signo, handler);
#endif
        }
    }

    //
    // Check if class is already registered for this signal. If it is already done, leave method.
    // Otherwise, record class pointer
    //

    auto f = find_class(signo, cl_ptr);

    if(f == reg_sig[signo].registered_classes.end())
    {
        reg_sig[signo].registered_classes.push_back(cl_ptr);
#ifndef _TG_WINDOWS_
        reg_sig[signo].own_handler = handler;
#endif
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::find_class
//
// description :
//        Method to check if a class is already registered for a signal. If it is true, this method returns in which
//        element of the vector the class is registered
//
// argument :
//         in :
//            - signo : Signal number
//            - cl_ptr : Pointer to device class object
//
//-------------------------------------------------------------------------------------------------------------------

std::vector<DeviceClass *>::iterator DServerSignal::find_class(long signo, DeviceClass *cl_ptr)
{
    return std::find(reg_sig[signo].registered_classes.begin(), reg_sig[signo].registered_classes.end(), cl_ptr);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::register_dev_signal()
//
// description :
//        method to register a signal handler at the device level
//
// argument :
//         in :
//            - signo : Signal number
//            - dev_ptr : Pointer to device object
//
//------------------------------------------------------------------------------------------------------------------

#ifdef _TG_WINDOWS_
void DServerSignal::register_dev_signal(long signo, DeviceImpl *dev_ptr)
#else
void DServerSignal::register_dev_signal(long signo, bool handler, DeviceImpl *dev_ptr)
#endif
{
    //
    // Check signal validity
    //

    if((signo < 1) || (signo >= _NSIG))
    {
        TangoSys_OMemStream o;
        o << "Signal number " << signo << " out of range" << std::ends;
        TANGO_THROW_EXCEPTION(API_SignalOutOfRange, o.str());
    }

#ifndef _TG_WINDOWS_
    if((auto_signal(signo)) && (handler))
    {
        TangoSys_OMemStream o;
        o << "Signal " << sig_name[signo] << "is not authorized using your own handler" << std::ends;
        TANGO_THROW_EXCEPTION(API_SignalOutOfRange, o.str());
    }
#endif

    //
    // If nothing is registered for this signal, install the OS signal handler
    //

    if(!auto_signal(signo))
    {
        if((reg_sig[signo].registered_devices.empty()) && (reg_sig[signo].registered_classes.empty()))
        {
#ifdef _TG_WINDOWS_
            register_handler(signo);
#else
            register_handler(signo, handler);
#endif
        }
    }

    //
    // Check if devices is already registered for this signal. If it is already done, leave method.
    // Otherwise, record class pointer
    //

    auto f = find_device(signo, dev_ptr);

    if(f == reg_sig[signo].registered_devices.end())
    {
        reg_sig[signo].registered_devices.push_back(dev_ptr);
#ifndef _TG_WINDOWS_
        reg_sig[signo].own_handler = handler;
#endif
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::find_device
//
// description :
//        Method to check if a device is already registered for a signal. If it is true, this method returns  in which
//        element of the vector the device is registered
//
// argument :
//         in :
//             - signo : Signal number
//             - dev_ptr : Pointer to device object
//
//--------------------------------------------------------------------------------------------------------------------

std::vector<DeviceImpl *>::iterator DServerSignal::find_device(long signo, DeviceImpl *dev_ptr)
{
    return std::find(reg_sig[signo].registered_devices.begin(), reg_sig[signo].registered_devices.end(), dev_ptr);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::unregister_class_signal()
//
// description :
//        Method to unregister a signal handler at the class level
//
// argument :
//         in :
//            - signo : Signal number
//            - cl_ptr : Pointer to device class object
//
//------------------------------------------------------------------------------------------------------------------

void DServerSignal::unregister_class_signal(long signo, DeviceClass *cl_ptr)
{
    //
    // Check signal validity
    //

    if((signo < 1) || (signo >= _NSIG))
    {
        TangoSys_OMemStream o;
        o << "Signal number " << signo << " out of range" << std::ends;
        TANGO_THROW_EXCEPTION(API_SignalOutOfRange, o.str());
    }

    //
    // Check if class is already registered for this signal. If it is already done, leave method.
    // Otherwise, record class pointer
    //

    auto f = find_class(signo, cl_ptr);

    if(f == reg_sig[signo].registered_classes.end())
    {
        return;
    }
    else
    {
        reg_sig[signo].registered_classes.erase(f);
    }

    //
    // If nothing is registered for this signal, unregister the OS signal handler and (eventually) the event handler
    //

    if(!auto_signal(signo))
    {
        if((reg_sig[signo].registered_classes.empty()) && (reg_sig[signo].registered_devices.empty()))
        {
            unregister_handler(signo);
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::unregister_dev_signal()
//
// description :
//        Method to unregister a signal handler at the class level
//
// argument :
//         in :
//            - signo : Signal number
//            - dev_ptr : Pointer to device object
//
//------------------------------------------------------------------------------------------------------------------

void DServerSignal::unregister_dev_signal(long signo, DeviceImpl *dev_ptr)
{
    //
    // Check signal validity
    //

    if((signo < 1) || (signo >= _NSIG))
    {
        TangoSys_OMemStream o;
        o << "Signal number " << signo << " out of range" << std::ends;
        TANGO_THROW_EXCEPTION(API_SignalOutOfRange, o.str());
    }

    //
    // Check if device is already registered for this signal. If yes, remove it.
    // Otherwise, leave method
    //

    auto f = find_device(signo, dev_ptr);

    if(f == reg_sig[signo].registered_devices.end())
    {
        return;
    }
    else
    {
        reg_sig[signo].registered_devices.erase(f);
    }

    //
    // If nothing is registered for this signal, unregister the OS signal handler and eventually the event handler
    //

    if(!auto_signal(signo))
    {
        if((reg_sig[signo].registered_classes.empty()) && (reg_sig[signo].registered_devices.empty()))
        {
            unregister_handler(signo);
        }
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::unregister_dev_signal()
//
// description :
//        Method to unregister a signal handler at the device level for all signals
//
// argument :
//         in :
//            - dev_ptr : Pointer to device object
//
//------------------------------------------------------------------------------------------------------------------

void DServerSignal::unregister_dev_signal(DeviceImpl *dev_ptr)
{
    for(long i = 1; i < _NSIG; i++)
    {
        unregister_dev_signal(i, dev_ptr);
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::unregister_class_signal()
//
// description :
//        Method to unregister a signal handler at the class level for all signals
//
// argument :
//        in :
//            - cl_ptr : Pointer to device class object
//
//-------------------------------------------------------------------------------------------------------------------

void DServerSignal::unregister_class_signal(DeviceClass *cl_ptr)
{
    for(long i = 1; i < _NSIG; i++)
    {
        unregister_class_signal(i, cl_ptr);
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::register_handler()
//
// description :
//        Method to register in the OS the main signal handler for a given signal
//
// argument :
//         in :
//            - signo : Signal number
//
//------------------------------------------------------------------------------------------------------------------

#ifdef _TG_WINDOWS_
void DServerSignal::register_handler(long signo)
#else
void DServerSignal::register_handler(long signo, bool handler)
#endif
{
    TANGO_LOG_DEBUG << "Installing OS signal handler for signal " << sig_name[signo] << std::endl;

#ifdef _TG_WINDOWS_
    if(::signal(signo, DServerSignal::main_sig_handler) == SIG_ERR)
    {
        TangoSys_OMemStream o;
        o << "Can't install signal " << signo << ". OS error = " << errno << std::ends;
        TANGO_THROW_EXCEPTION(API_CantInstallSignal, o.str());
    }
#else
    if(handler)
    {
        sigset_t sigs_to_unblock;
        sigemptyset(&sigs_to_unblock);
        sigaddset(&sigs_to_unblock, signo);

        if(pthread_sigmask(SIG_UNBLOCK, &sigs_to_unblock, nullptr) != 0)
        {
            TangoSys_OMemStream o;
            o << "Can't install signal " << signo << ". OS error = " << errno << std::ends;
            TANGO_THROW_EXCEPTION(API_CantInstallSignal, o.str());
        }

        struct sigaction sa;

        sa.sa_flags = SA_RESTART;
        sa.sa_handler = DServerSignal::main_sig_handler;
        sigemptyset(&sa.sa_mask);

        if(sigaction((int) signo, &sa, nullptr) == -1)
        {
            TangoSys_OMemStream o;
            o << "Can't install signal " << signo << ". OS error = " << errno << std::ends;
            TANGO_THROW_EXCEPTION(API_CantInstallSignal, o.str());
        }
    }
    else
    {
        omni_mutex_lock sy(*this);

        while(sig_to_install)
        {
            wait();
        }
        sig_to_install = true;
        inst_sig = signo;

        pthread_kill(sig_th->my_thread, SIGINT);
    }

#endif
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::unregister_handler()
//
// description :
//        Method to unregister from the OS the main signal handler for a given signal
//
// argument :
//         in :
//            - signo : Signal number
//
//-----------------------------------------------------------------------------------------------------------------

void DServerSignal::unregister_handler(long signo)
{
    TANGO_LOG_DEBUG << "Removing OS signal handler for signal " << sig_name[signo] << std::endl;

#ifdef _TG_WINDOWS_
    if(::signal(signo, SIG_DFL) == SIG_ERR)
    {
        TangoSys_OMemStream o;
        o << "Can't install signal " << signo << ". OS error = " << errno << std::ends;
        TANGO_THROW_EXCEPTION(API_CantInstallSignal, o.str());
    }
#else
    if(reg_sig[signo].own_handler)
    {
        struct sigaction sa;

        sa.sa_flags = 0;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);

        if(sigaction((int) signo, &sa, nullptr) == -1)
        {
            TangoSys_OMemStream o;
            o << "Can't install signal " << signo << ". OS error = " << errno << std::ends;
            TANGO_THROW_EXCEPTION(API_CantInstallSignal, o.str());
        }
    }
    else
    {
        omni_mutex_lock sy(*this);

        while(sig_to_remove)
        {
            wait();
        }
        sig_to_remove = true;
        rem_sig = signo;

        pthread_kill(sig_th->my_thread, SIGINT);
    }

#endif
}

#ifndef _TG_WINDOWS_
pid_t DServerSignal::get_sig_thread_pid()
{
    omni_mutex_lock syn(*this);

    if(sig_th->my_pid == 0)
    {
        wait(1000);
    }
    return sig_th->my_pid;
}
#endif
//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        DServerSignal::main_sig_handler()
//
// description :
//        This is a dummy signal handler used only with solaris which needs one for signal with a default ignore
//        action to work correctly with the sigwait() call.
//
// argument :
//         in :
//            - signo : Signal number
//
//-------------------------------------------------------------------------------------------------------------------

#ifndef _TG_WINDOWS_
void DServerSignal::main_sig_handler(int signo)
{
    TANGO_LOG_DEBUG << "In main sig_handler !!!!" << std::endl;
    deliver_to_registered_handlers(signo);
}
#else
void DServerSignal::main_sig_handler(int signo)
{
    TANGO_LOG_DEBUG << "In main sig_handler !!!!" << std::endl;

    win_signo = signo;
    SetEvent(win_ev);
}
#endif

void DServerSignal::deliver_to_registered_handlers(int signo)
{
    //
    // First, execute all the handlers installed at the class level,
    // then those at the device level
    //
    DevSigAction *act_ptr = &(DServerSignal::reg_sig[signo]);
    for(auto *class_ : act_ptr->registered_classes)
    {
        class_->signal_handler((long) signo);
    }
    for(auto *device : act_ptr->registered_devices)
    {
        device->signal_handler((long) signo);
    }
}

} // namespace Tango
