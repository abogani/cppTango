#include "utils/platform/platform.h"

#include "utils/platform/unix/interface.h"
#include "utils/platform/ready_string_finder.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
  #include <signal.h>
#endif

#include <fstream>
#include <thread>
#include <iostream>

#include <tango/common/utils/assert.h>

// Common platform implementation for the Unix-like platforms we support, i.e.
// Linux and macOS.

extern char **environ;

namespace TangoTest::platform
{

namespace
{

struct timespec duration_to_timespec(std::chrono::nanoseconds d)
{
    using namespace std::chrono;
    auto secs = duration_cast<seconds>(d);
    d -= secs;

    return {static_cast<decltype(timespec{}.tv_sec)>(secs.count()),
            static_cast<decltype(timespec{}.tv_nsec)>(d.count())};
}

struct BlockSigChild
{
    BlockSigChild()
    {
        sigset_t blockset;
        sigemptyset(&blockset);
        sigaddset(&blockset, SIGCHLD);
        if(sigprocmask(SIG_BLOCK, &blockset, &origset) == -1)
        {
            unix::throw_strerror("sigprocmask()");
        }
    }

    ~BlockSigChild()
    {
        sigprocmask(SIG_BLOCK, &origset, nullptr);
    }

    sigset_t origset;
};

struct RedirectFile
{
    RedirectFile(const char *path)
    {
        fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if(fd == -1)
        {
            unix::throw_strerror("open(\"", path, "\")");
        }
    }

    void close()
    {
        ::close(fd);
        fd = -1;
    }

    ~RedirectFile()
    {
        close();
    }

    int fd = -1;
};

void handle_child(int)
{
    // Do nothing, we want to handle the server exiting synchronously
}

ExitStatus convert_wait_status(int status)
{
    using Kind = ExitStatus::Kind;

    ExitStatus result;

    if(WIFEXITED(status))
    {
        result.kind = Kind::Normal;
        result.code = WEXITSTATUS(status);
    }
    else if(WIFSIGNALED(status))
    {
        result.kind = Kind::Aborted;
        result.signal = WTERMSIG(status);
    }
    else
    {
        // This should never happen, but just in case we fill something in here.
        result.kind = Kind::AbortedNoSignal;
    }

    return result;
}

} // namespace

void init()
{
    // We are handling the reaping of our children in start_server() and
    // stop_server() so we want to disable the kernel's automatic reaping when
    // SIGCHLD is set to ignore.
    struct sigaction childaction;
    childaction.sa_handler = &handle_child;
    sigemptyset(&childaction.sa_mask);
    childaction.sa_flags = 0;
    if(sigaction(SIGCHLD, &childaction, nullptr) == -1)
    {
        unix::throw_strerror("sigaction()");
    }
}

StartServerResult start_server(const std::vector<std::string> &args,
                               const std::vector<std::string> &env,
                               const std::string &redirect_filename,
                               const std::string &ready_string,
                               std::chrono::milliseconds timeout)
{
    using Kind = StartServerResult::Kind;

    StartServerResult result;

    // In order to be able to handle SIGCHLD as part of our wait_for_fd_or_signal() loop, we
    // need to:
    //  1. Block the signal and then unblock during the wait_for_fd_or_signal() call
    //  2. Install a do-nothing signal handler, so that the kernel will actually
    //  interrupt the wait_for_fd_or_signal() call with the SIGCHLD.
    //
    //  We restore the block mask but not the action so that if the server dies
    //  during the test we can waitpid in the call to stop_server at the end of
    //  the test to get the exit status.
    BlockSigChild block;

    // We setup the file watch now, before the fork(), so that we can be sure we
    // do not miss any write events.  We have to create the file first, so that
    // we can add the watch for it.

    RedirectFile redirect{redirect_filename.c_str()};
    unix::FileWatcher watcher{redirect_filename.c_str()};

    pid_t ppid = getpid();
    pid_t pid = fork();

    switch(pid)
    {
    case -1:
    {
        unix::throw_strerror("fork()");
    }
    case 0:
    {
        watcher.cleanup_in_child();

        if(dup2(redirect.fd, 1) == -1)
        {
            perror("dup2()");
            exit(1);
        }

        if(dup2(redirect.fd, 2) == -1)
        {
            perror("dup2()");
            exit(1);
        }

        unix::kill_self_on_parent_death(ppid);

        auto make_cstr_buffer = [](const std::vector<std::string> &items)
        {
            std::vector<const char *> result;

            result.reserve(items.size() + 1);

            for(const auto &entry : items)
            {
                result.emplace_back(entry.c_str());
            }

            result.emplace_back(nullptr);

            return result;
        };

        std::vector<const char *> env_buffer = make_cstr_buffer(env);
        std::vector<const char *> arg_buffer = make_cstr_buffer(args);

        environ = const_cast<char **>(env_buffer.data());

        if(execv(k_test_server_binary_path, const_cast<char *const *>(arg_buffer.data())) == -1)
        {
            perror("execv()");
            exit(1);
        }

        // unreachable
        result.kind = Kind::Exited;
        return result;
    }
    default:
    {
        using std::chrono::steady_clock;
        redirect.close();

        sigset_t emptyset;
        sigemptyset(&emptyset);

        ReadyStringFinder finder{redirect_filename};

        // Begin watching the device server's log file.
        // This is a no-op on Linux, only needed on macOS.
        watcher.start_watching();

        int watch_fd = watcher.get_file_descriptor();

        auto end = steady_clock::now() + timeout;
        while(true)
        {
            struct timespec remaining_timeout = duration_to_timespec(end - steady_clock::now());
            int ready = unix::wait_for_fd_or_signal(watch_fd, &remaining_timeout, &emptyset);

            if(ready == -1)
            {
                if(errno != EINTR)
                {
                    unix::throw_strerror("pselect()");
                }

                int status;
                int ret = waitpid(pid, &status, WNOHANG);
                if(ret == -1)
                {
                    unix::throw_strerror("waitpid()");
                }

                if(ret != 0)
                {
                    result.kind = Kind::Exited;
                    result.exit_status = convert_wait_status(status);
                    return result;
                }
            }
            else if(ready == 0)
            {
                result.kind = Kind::Timeout;
                result.handle = reinterpret_cast<TestServer::Handle *>(pid);
                return result;
            }
            else
            {
                watcher.pop_event();

                if(finder.check_for_ready_string(ready_string))
                {
                    result.kind = Kind::Started;
                    result.handle = reinterpret_cast<TestServer::Handle *>(pid);
                    return result;
                }
            }
        }

        // Tell the watcher on macOS to stop the watcher thread
        // and close all fds. On Linux this is a no-op.
        watcher.stop_watching();
    }
    }
}

static void kill(pid_t pid, int signo)
{
    if(::kill(pid, signo))
    {
        perror("kill()");
    }
}

std::vector<int> relevant_sendable_signals()
{
    return {SIGINT, SIGTERM};
}

void send_signal(TestServer::Handle *handle, int signo)
{
    pid_t child = static_cast<pid_t>(reinterpret_cast<ssize_t>(handle));
    kill(child, signo);
}

StopServerResult stop_server(TestServer::Handle *handle)
{
    using std::chrono::steady_clock;
    using Kind = StopServerResult::Kind;

    StopServerResult result;

    pid_t child = static_cast<pid_t>(reinterpret_cast<ssize_t>(handle));

    // Has the server already exited?
    int status = 0;
    pid_t pid = waitpid(child, &status, WNOHANG);

    if(pid != 0)
    {
        result.kind = Kind::ExitedEarly;
        result.exit_status = convert_wait_status(status);
        return result;
    }

    kill(child, SIGTERM);

    result.kind = Kind::Exiting;
    return result;
}

WaitForStopResult wait_for_stop(TestServer::Handle *handle, std::chrono::milliseconds timeout)
{
    using std::chrono::steady_clock;
    using Kind = WaitForStopResult::Kind;

    WaitForStopResult result;

    pid_t child = static_cast<pid_t>(reinterpret_cast<ssize_t>(handle));

    auto end = steady_clock::now() + timeout;
    while(steady_clock::now() < end)
    {
        int status = 0;
        pid_t pid = waitpid(child, &status, WNOHANG);
        if(pid != 0)
        {
            result.kind = Kind::Exited;
            result.exit_status = convert_wait_status(status);
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    };

    result.kind = Kind::Timeout;
    return result;
}

} // namespace TangoTest::platform
