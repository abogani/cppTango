#ifndef TANGO_TESTS_CATCH2_UTILS_PLATFORM_UNIX_INTERFACE_H
#define TANGO_TESTS_CATCH2_UTILS_PLATFORM_UNIX_INTERFACE_H

#include <memory>
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace TangoTest::platform::unix
{

// TODO: Reimplement the entire macos implementation using the kqueue rather
// than sharing this unix file with the linux implementation

template <typename... Args>
[[noreturn]] void throw_strerror(Args... args)
{
    std::stringstream ss;
    (ss << ... << args);
    ss << ": " << std::strerror(errno);
    throw std::runtime_error(ss.str());
}

// To be implemented by Linux and macOS implementations

// Watches a file for `write()` events and notifies a user by sending events which can
// be read from a file descriptor.
class FileWatcher
{
  public:
    // Construct the FileWatcher monitoring the file at `filename` for `write()`
    // events.
    //
    // Requires that `filename` exists.
    explicit FileWatcher(const char *filename);
    FileWatcher(const FileWatcher &) = delete;
    FileWatcher &operator=(FileWatcher &) = delete;
    virtual ~FileWatcher();

    // Return a file descriptor that can be `select()`'d on.
    //
    // The file descriptor becomes readable whenever a "write event" occurs.
    //
    // Spurious wake-ups are possible, that is the file descriptor could become
    // readable even when no write event has occurred.
    int get_file_descriptor();

    // Read and discard a single `write()` event.
    void pop_event();

    // Start the file watcher thread on macOS.
    // On Linux do nothing.
    void start_watching();

    // Stop the file watcher thread on macOS and close all fds.
    // On Linux do nothing.
    void stop_watching();

    // Cleanup any resources created by the `FileWatcher` which survive a
    // `fork()`.
    //
    // This is intended to allow a user to constructor a `FileWatcher` before
    // calling `fork()` then cleanup any resources in the child process.
    //
    // This is required so that the user will not miss a `write()` event.
    void cleanup_in_child();

  private:
    struct Impl;
    std::unique_ptr<Impl> m_pimpl;
};

// Arranges for this process to die if this process's parent is not `ppid`,
// i.e. because they parent has died and this process has been re-parented.
void kill_self_on_parent_death(pid_t ppid);

// Wait for fd to be ready for reading or signal to be received
// Returns:
//  - > 0 if fd is ready to be read
//  - 0 if timed out
//  - -1 if error, check errno
int wait_for_fd_or_signal(int fd, struct timespec *timeout, sigset_t *sigmask);

} // namespace TangoTest::platform::unix

#endif
