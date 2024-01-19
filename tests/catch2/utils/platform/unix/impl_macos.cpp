#include "utils/platform/unix/interface.h"

namespace TangoTest::platform::unix
{

// TODO: Implement this with FSEvent and `pipe()`
struct FileWatcher::Impl
{
};

FileWatcher::FileWatcher([[maybe_unused]] const char *filename) :
    m_pimpl{std::make_unique<FileWatcher::Impl>()}
{
}

FileWatcher::~FileWatcher() { }

int FileWatcher::get_file_descriptor()
{
    return -1;
}

void FileWatcher::cleanup_in_child() { }

void FileWatcher::pop_event() { }

// TODO: Implementing this on macOS might be tricky as there is no equivalent of
// prctl(PR_SET_PDEATHSIG).  Perhaps we can `fork()` again to introduce a
// supervisor process which monitors things.  If this is to hard, it is
// acceptable to not implement this.
void kill_self_on_parent_death([[maybe_unused]] pid_t ppid) { }

} // namespace TangoTest::platform::unix
