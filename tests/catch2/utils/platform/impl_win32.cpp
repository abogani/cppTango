#include "platform/impl_common.h"

namespace TangoTest::platform::impl
{
void init() { }

StartServerResult start_server([[maybe_unused]] const std::vector<const char *> &args,
                               [[maybe_unused]] const std::string &redirect_filename,
                               [[maybe_unused]] const std::string &ready_string,
                               [[maybe_unused]] std::chrono::milliseconds timeout)
{
    return StartServerResult{};
}

StopServerResult stop_server([[maybe_unused]] TestServer::Handle *handle,
                             [[maybe_unused]] std::chrono::milliseconds timeout)
{
    return StopServerResult{};
}
} // namespace TangoTest::platform::impl
