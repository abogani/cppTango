#include <condition_variable>
#include <csignal>
#include <thread>

#ifdef _TG_WINDOWS_
  #include <windows.h>

static inline void sleep(DWORD seconds)
{
    Sleep(seconds * 1000);
}
#else
  #include <sys/wait.h>
#endif

#include <tango/tango.h>

// Define a simple device server
#ifndef COMPAT
class StepperMotor : public TANGO_BASE_CLASS
{
#else
class StepperMotor : public Tango::Device_5Impl
{
#endif
  public:
    StepperMotor(Tango::DeviceClass *, std::string &);
    StepperMotor(Tango::DeviceClass *, const char *);
    StepperMotor(Tango::DeviceClass *, const char *, const char *);
    StepperMotor(Tango::DeviceClass *, const char *, const char *, Tango::DevState, const char *);

    // using Device_5Impl::Device_5Impl;
    void init_device() override { }
};

#ifndef COMPAT
StepperMotor::StepperMotor(Tango::DeviceClass *cl, std::string &s) :
    TANGO_BASE_CLASS(cl, s.c_str())
{
    init_device();
}

StepperMotor::StepperMotor(Tango::DeviceClass *cl, const char *s) :
    TANGO_BASE_CLASS(cl, s)
{
    init_device();
}

StepperMotor::StepperMotor(Tango::DeviceClass *cl, const char *s, const char *d) :
    TANGO_BASE_CLASS(cl, s, d)
{
    init_device();
}

StepperMotor::StepperMotor(
    Tango::DeviceClass *cl, const char *s, const char *d, Tango::DevState state, const char *status) :
    TANGO_BASE_CLASS(cl, s, d, state, status)
{
    init_device();
}
#else
StepperMotor::StepperMotor(Tango::DeviceClass *cl, string &s) :
    Tango::Device_3Impl(cl, s.c_str())
{
    init_device();
}

StepperMotor::StepperMotor(Tango::DeviceClass *cl, const char *s) :
    Tango::Device_3Impl(cl, s)
{
    init_device();
}

StepperMotor::StepperMotor(Tango::DeviceClass *cl, const char *s, const char *d) :
    Tango::Device_3Impl(cl, s, d)
{
    init_device();
}

StepperMotor::StepperMotor(
    Tango::DeviceClass *cl, const char *s, const char *d, Tango::DevState state, const char *status) :
    Tango::Device_3Impl(cl, s, d, state, status)
{
    init_device();
}
#endif

class StepperMotorClass : public Tango::DeviceClass
{
  public:
    static StepperMotorClass *init(const char *s)
    {
        if(!_instance)
        {
            std::string str(s);
            _instance = new StepperMotorClass(str);
        }
        return _instance;
    }

    static StepperMotorClass *instance()
    {
        return _instance;
    }

    ~StepperMotorClass()
    {
        _instance = NULL;
    }

  protected:
    StepperMotorClass(std::string &s) :
        Tango::DeviceClass(s)
    {
    }

    static StepperMotorClass *_instance;

    void command_factory() { }

    void attribute_factory(std::vector<Tango::Attr *> &) { }

  public:
    void device_factory(const Tango::DevVarStringArray *devlist_ptr)
    {
        for(_CORBA_ULong i = 0; i < devlist_ptr->length(); i++)
        {
            device_list.push_back(new StepperMotor(this, (*devlist_ptr)[i]));
            if(Tango::Util::_UseDb)
            {
                export_device(device_list.back());
            }
            else
            {
                export_device(device_list.back(), ((*devlist_ptr)[i]));
            }
        }
    }
};

StepperMotorClass *StepperMotorClass::_instance = NULL;

void Tango::DServer::class_factory()
{
    add_class(StepperMotorClass::init("StepperMotor"));
}

// Install the specified signal handler in the device server
void install_signal_handler(int handlers)
{
#ifndef _TG_WINDOWS_
    struct sigaction sig;
    sig.sa_flags = SA_RESTART;
    sig.sa_handler = [](int signal) { std::cout << "Signal received: " << signal << '\n'; };

    auto _install_for_signal = [&](int signal, const std::string &signal_name)
    {
        if(sigaction(signal, &sig, nullptr) == -1)
        {
            perror("sigaction");
            return;
        }
        std::cout << "Installed " << signal_name << " signal handler\n";
    };

    switch(handlers)
    {
    case SIGINT:
        _install_for_signal(SIGINT, "SIGINT");
        break;
    case SIGTERM:
        _install_for_signal(SIGTERM, "SIGTERM");
        break;
    case -1:
        _install_for_signal(SIGINT, "SIGINT");
        _install_for_signal(SIGTERM, "SIGTERM");
        break;
    default:
        std::cout << "NOT installing any signal handlers" << std::endl;
        break;
    }
#endif
}

struct condition
{
    bool stopped{false};
    std::condition_variable cv;
    std::mutex mutex;
};

// Start thread if do_start_thread and wait for condition to be stopped, otherwise, do nothing
std::thread start_thread(condition &c, bool do_start_thread)
{
    if(do_start_thread)
    {
        return std::thread(
            [&]()
            {
                std::cout << "Started background thread\n";
                std::unique_lock<std::mutex> lock{c.mutex};
                c.cv.wait(lock, [&] { return c.stopped; });
                lock.unlock();
                std::cout << "Exiting background thread\n";
            });
    }
    return std::thread{};
}

// Create the device server; if do_start_thread or signal handlers specified, then
// do this before initialising the device server.
void create_device_server(int argc, char *argv[], bool do_start_thread, int handlers)
{
    struct condition c;
    auto thread = start_thread(c, do_start_thread);
    sleep(1); // Wait for thread to start.
    install_signal_handler(handlers);

    try
    {
        Tango::Util *tg = Tango::Util::init(argc, argv);
        tg->server_init();
        std::cout << "Device server initialised" << std::endl;

        std::cout << "Ready to accept request" << std::endl;
        tg->server_run();
        tg->server_cleanup();
        std::cout << "Server running" << std::endl;
    }
    catch(std::bad_alloc &)
    {
        std::cout << "Can't allocate memory to store device object !!!" << std::endl;
        std::cout << "Exiting" << std::endl;
    }
    catch(Tango::DevFailed &e)
    {
        Tango::Except::print_exception(e);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);

        std::cout << "Received a CORBA_Exception" << std::endl;
        std::cout << "Exiting" << std::endl;
    }

    if(thread.joinable())
    {
        {
            std::cout << "Stopping thread..." << std::endl;
            std::lock_guard<std::mutex> guard(c.mutex);
            c.stopped = true;
        }
        c.cv.notify_one();
        thread.join();
    }
}

// Fork the device server and send it SIGTERM.
void run_test(int argc, char *argv[], bool do_start_thread, int handlers)
{
#ifndef _TG_WINDOWS_
    int pid = fork();
    if(pid < 0)
    {
        perror("fork");
        exit(1);
    }
    else if(!pid)
    {
        create_device_server(argc, argv, do_start_thread, handlers);
    }
    else
    {
        sleep(3); // Wait for device server to initialise
        std::cout << "PARENT pid=" << getpid() << " CHILD pid=" << pid << std::endl;

        std::cout << "PARENT sending SIGTERM to " << pid << "..." << std::endl;
        kill(pid, SIGTERM);
        sleep(1); // Wait for child to process the signal

        int status = 0;
        std::cout << "Waiting for " << pid << "...\n";
        pid_t wait_result = waitpid(pid, &status, WNOHANG);

        std::cout << "wait()=" << wait_result << "\n  WIFEXITED=" << WIFEXITED(status)
                  << "\n  WEXITSTATUS=" << WEXITSTATUS(status) << "\n  WIFSIGNALED=" << WIFSIGNALED(status);
        if(WIFSIGNALED(status))
        {
            std::cout << "\n  WTERMSIG=" << WTERMSIG(status) << "\n  WCOREDUMP=" << WCOREDUMP(status);
        }
        std::cout << "\n  WIFSTOPPED=" << WIFSTOPPED(status) << "\n  WSTOPSIG=" << WSTOPSIG(status)
                  << "\n  WIFCONTINUED=" << WIFCONTINUED(status) << std::endl;

        // Device server should already be terminated, therefore kill() should return -1
        // and not zero.
        std::cout << "PARENT sending SIGINT to " << pid << "..." << std::endl;
        int result = kill(pid, SIGINT);
        std::cout << "kill(" << pid << ", SIGINT) == " << result << std::endl;
        if(result != -1)
        {
            int dev_server_stopped = 0;
            assert(dev_server_stopped);
        }
    }
#endif
}

int main()
{
    // This test only concerns the signal handling, therefore we don't need to
    // use the database.
    char *args[5] = {
        (char *) "SignalTest", (char *) "test", (char *) "-nodb", (char *) "-ORBendPoint", (char *) "giop:tcp::11000"};
    run_test(5, args, true, SIGTERM);

    return 0;
}
