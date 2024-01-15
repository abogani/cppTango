#ifndef TANGO_TESTS_BDD_UTILS_PLATFORM_UNIX_INTERFACE_H
#define TANGO_TESTS_BDD_UTILS_PLATFORM_UNIX_INTERFACE_H

#include <memory>
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace TangoTest::platform::unix
{

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
    ~FileWatcher();

    // Return a file descriptor that can be `select()`'d on.
    //
    // The file descriptor becomes readable whenever a "write event" occurs.
    //
    // Spurious wake-ups are possible, that is the file descriptor could become
    // readable even when no write event has occurred.
    int get_file_descriptor();

    // Read and discard a single `write()` event.
    void pop_event();

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

} // namespace TangoTest::platform::unix

#endif
