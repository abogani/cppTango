//=============================================================================
//
// file :               DServerSignal.h
//
// description :        Include for the DServer class. This class implements
//                      all the commands which are available for device
//            of the DServer class. There is one device of the
//            DServer class for each device server process
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
//=============================================================================

#ifndef _DSERVERSIGNAL_H
#define _DSERVERSIGNAL_H

#include <condition_variable>
#include <deque>
#include <tango/tango.h>
#include <signal.h>

#ifdef _TG_WINDOWS_
struct sigaction
{
    void (*sa_handler)(int);
};
#endif

namespace Tango
{

//=============================================================================
//
//            The DServerSignal class
//
// description :    Class to implement all data members and commands for
//            signal management in a TANGO device server.
//
//=============================================================================

#if (defined(_TG_WINDOWS_) || (defined __APPLE__) || (defined __freebsd__))
  #define _NSIG NSIG
#endif

typedef struct
{
    std::vector<DeviceClass *> registered_classes;
    std::vector<DeviceImpl *> registered_devices;
} DevSigAction;

template <typename T>
class SynchronisedQueue
{
    std::condition_variable cv;
    std::mutex mutex;
    std::deque<T> values;

  public:
    void put(T value)
    {
        {
            std::lock_guard<std::mutex> lock{mutex};
            values.push_back(value);
        }
        cv.notify_one();
    }

    T get()
    {
        std::unique_lock<std::mutex> lock{mutex};
        cv.wait(lock, [this] { return !values.empty(); });
        auto value = values.front();
        values.pop_front();
        lock.unlock();
        return value;
    }
};

class DServerSignal : public TangoMonitor
{
  public:
    static DServerSignal *instance();
    static void cleanup_singleton();
    static void initialise_signal_names();

    ~DServerSignal();

    void initialise();

#ifndef _TG_WINDOWS_
    void register_class_signal(long, bool, DeviceClass *);
    void register_dev_signal(long, bool, DeviceImpl *);

    void register_handler(long, bool);
    pid_t get_sig_thread_pid();
#else
    void register_class_signal(long, DeviceClass *);
    void register_dev_signal(long, DeviceImpl *);

    void register_handler(long);
#endif

    void unregister_class_signal(long, DeviceClass *);
    void unregister_class_signal(DeviceClass *);
    void unregister_dev_signal(long, DeviceImpl *);
    void unregister_dev_signal(DeviceImpl *);

    void unregister_handler(long);

    class ThSig : public omni_thread
    {
        DServerSignal *ds;

      public:
        ThSig(DServerSignal *d) :
            ds(d),
            my_pid(0)

        {
        }

        ~ThSig() override { }

        TangoSys_Pid my_pid;
        void *run_undetached(void *) override;

        void start()
        {
            start_undetached();
        }
    };
    friend class ThSig;
    ThSig *sig_th;

  protected:
    DServerSignal();
    static DevSigAction reg_sig[_NSIG];
    static void deliver_to_registered_handlers(int signo);

    bool sig_th_should_stop = false;

  private:
    static std::unique_ptr<DServerSignal> _instance;
    std::vector<DeviceImpl *>::iterator find_device(long, DeviceImpl *);
    std::vector<DeviceImpl *>::iterator find_delayed_device(long, DeviceImpl *);

    std::vector<DeviceClass *>::iterator find_class(long, DeviceClass *);
    std::vector<DeviceClass *>::iterator find_delayed_class(long, DeviceClass *);

    static bool auto_signal(long s)
    {
#ifdef _TG_WINDOWS_
        return (s == SIGINT) || (s == SIGTERM) || (s == SIGABRT) || (s == SIGBREAK);
#else
        return (s == SIGINT) || (s == SIGTERM) || (s == SIGQUIT) || (s == SIGHUP);
#endif
    }

    static std::string sig_name[_NSIG];
    struct sigaction enqueueing_sa;
    struct sigaction direct_sa;
    struct sigaction default_sa;

    void handle_on_signal_thread(int signo);
    void handle_directly(int signo);
    void handle_with_default(int signo);

    SynchronisedQueue<int> signal_queue;
};

} // End of namespace Tango

#endif
