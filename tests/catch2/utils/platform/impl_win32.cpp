#include "utils/platform/platform.h"

namespace TangoTest::platform
{

void init() { }

StartServerResult start_server([[maybe_unused]] const std::vector<std::string> &args,
                               [[maybe_unused]] const std::vector<std::string> &env,
                               [[maybe_unused]] const std::string &redirect_filename,
                               [[maybe_unused]] const std::string &ready_string,
                               [[maybe_unused]] std::chrono::milliseconds timeout)
{
    return StartServerResult{};
}

StopServerResult stop_server([[maybe_unused]] TestServer::Handle *handle)
{
    return StopServerResult{};
}

WaitForStopResult wait_for_stop([[maybe_unused]] TestServer::Handle *handle,
                                [[maybe_unused]] std::chrono::milliseconds timeout)
{
    return WaitForStopResult{};
}
} // namespace TangoTest::platform
