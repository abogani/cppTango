#include "utils/platform/unix/interface.h"
#include "utils/platform/platform.h"

#include <functional>
#include <thread>
#include <array>
#include <cstring>
#include <csignal>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <iostream>

namespace TangoTest::platform
{

std::vector<std::string> default_env()
{
    return {"PATH="};
}

namespace unix
{

struct FileWatcher::Impl
{
    explicit Impl(const char *filename)
    {
        // Create a kqueue that allows us to watch file events.
        if((kernel_queue = kqueue()) < 0)
        {
            throw_strerror("Could not open a kernel queue.");
        }

        // Open a file that we want to watch via the kqueue events.
        const auto fd{open(filename, O_EVTONLY)};
        if(fd <= 0)
        {
            throw_strerror("Could not open file \"", filename, "\".");
        }
        else
        {
            event_fd = fd;
        }

        // Tell the kqueue event set what we want to monitor.
        wanted_events = NOTE_WRITE;
        EV_SET64(&(events_to_monitor[0]), event_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, wanted_events, 0, 0, 0, 0);

        if(::pipe(watched_fds) != 0)
        {
            throw_strerror("Could not open a pipe for communication with the "
                           "testing framework.");
        }
    }

    void start_watching()
    {
        stop_thread = false;
        event_thread = std::thread(&FileWatcher::Impl::thread_run, this);
    }

    void thread_run()
    {
        struct timespec timeout
        {
        };

        while(!stop_thread)
        {
            // Always reset the timeout to 0.1s.
            timeout.tv_sec = 0;
            timeout.tv_nsec = 100'000'000;

            // Wait until a kqueue event that we want happens. This call
            // will time out after timeout seconds.
            const int event_count{kevent64(kernel_queue,
                                           &(events_to_monitor[0]),
                                           number_of_watched_files,
                                           &(event_data[0]),
                                           number_of_watched_files,
                                           KEVENT_FLAG_NONE,
                                           &timeout)};
            if((event_count < 0) || (event_data[0].flags == EV_ERROR))
            {
                // An error occurred.
                throw_strerror("An error occurred (event count=", event_count, ").");
                break;
            }

            if(event_count)
            {
                ::write(watched_fds[1], "\0", 1);
            }
        };
    }

    void stop_watching_thread()
    {
        stop_thread = true;
        if(event_thread.joinable())
        {
            event_thread.join();
        }
    }

    ~Impl()
    {
        stop_watching_thread();
    }

    void stop_watching()
    {
        stop_watching_thread();
        ::close(kernel_queue);

        const int fd{event_fd};
        event_fd = -1;
        ::close(fd);

        const int fds[2]{watched_fds[0], watched_fds[1]};
        watched_fds[0] = watched_fds[1] = -1;
        ::close(fds[1]);
        ::close(fds[0]);
    }

    const std::size_t number_of_watched_files{1};
    int event_fd{-1};
    int watched_fds[2]{-1, -1};
    int kernel_queue{-1};
    bool stop_thread{true};
    std::thread event_thread;
    std::array<struct kevent64_s, 1> events_to_monitor{};
    std::array<struct kevent64_s, 1> event_data{};

    struct timespec timeout
    {
        1, 0
    };

    unsigned int wanted_events;
};

FileWatcher::FileWatcher(const char *filename) :
    m_pimpl{std::make_unique<FileWatcher::Impl>(filename)}
{
}

FileWatcher::~FileWatcher() { }

int FileWatcher::get_file_descriptor()
{
    return m_pimpl->watched_fds[0];
}

void FileWatcher::start_watching()
{
    m_pimpl->start_watching();
}

void FileWatcher::stop_watching()
{
    m_pimpl->stop_watching();
}

void FileWatcher::cleanup_in_child()
{
    m_pimpl.reset(nullptr);
}

void FileWatcher::pop_event()
{
    char foo;
    ::read(m_pimpl->watched_fds[0], &foo, 1);
}

// There is no equivalent of prctl(PR_SET_PDEATHSIG). It is not worth the
// effort to implement something that mimics it. The tests works without it.
void kill_self_on_parent_death([[maybe_unused]] pid_t ppid) { }

} // namespace unix
} // namespace TangoTest::platform
