#include "utils/platform/unix/interface.h"
#include "utils/platform/platform.h"

#include <csignal>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <poll.h>
#include <unistd.h>

namespace TangoTest::platform
{

std::vector<std::string> default_env()
{
    return {};
}

namespace unix
{
struct FileWatcher::Impl
{
    explicit Impl(const char *filename)
    {
        fd = inotify_init();
        if(fd == -1)
        {
            throw_strerror("inotify_init()");
        }

        if(inotify_add_watch(fd, filename, IN_MODIFY) == -1)
        {
            throw_strerror("inotify_add_watch(\"", filename, "\")");
        }
    }

    ~Impl()
    {
        ::close(fd);
    }

    void pop_event()
    {
        struct inotify_event ev;
        while(true)
        {
            if(read(fd, &ev, sizeof(ev)) == -1)
            {
                if(errno == EINTR)
                {
                    continue;
                }

                throw_strerror("read()");
            }

            break;
        }
    }

    int fd = -1;
};

FileWatcher::FileWatcher(const char *filename) :
    m_pimpl{std::make_unique<FileWatcher::Impl>(filename)}
{
}

FileWatcher::~FileWatcher() { }

void FileWatcher::start_watching() { }

void FileWatcher::stop_watching() { }

int FileWatcher::get_file_descriptor()
{
    return m_pimpl->fd;
}

void FileWatcher::cleanup_in_child()
{
    m_pimpl.reset(nullptr);
}

void FileWatcher::pop_event()
{
    m_pimpl->pop_event();
}

void kill_self_on_parent_death(pid_t ppid)
{
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    // Our parent might have died after the call to fork(), but before the call
    // to prctl()
    if(getppid() != ppid)
    {
        exit(0);
    }
}

int wait_for_fd_or_signal(int fd, struct timespec *timeout, sigset_t *sigmask)
{
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN;

    return ppoll(&fds, 1, timeout, sigmask);
}

} // namespace unix
} // namespace TangoTest::platform
