#ifndef TANGO_TESTS_CATCH2_UTILS_PLATFORM_PLATFORM_H
#define TANGO_TESTS_CATCH2_UTILS_PLATFORM_PLATFORM_H

#include "utils/test_server.h"

#include <vector>
#include <string>
#include <string_view>
#include <chrono>

namespace TangoTest::platform
{

constexpr static const char *k_test_server_binary_path = TANGO_TEST_CATCH2_SERVER_BINARY_PATH;
constexpr static const char *k_output_directory_path = TANGO_TEST_CATCH2_OUTPUT_DIRECTORY_PATH;
constexpr static const std::string_view k_resource_path = TANGO_TEST_CATCH2_RESOURCE_PATH;

/** Return the platform specific default environment table
 */
std::vector<std::string> default_env();

/** Called when the test run starts to do any setup required by the platform
 */
void init();

struct StartServerResult
{
    enum class Kind
    {
        Started, // The server started and outputted the ready_string,
                 // this->handle will be active
        Timeout, // The server timed out waiting for the ready_string,
                 // this->handle will be active
        Exited   // The server exited before outputting the ready_string,
                 // this->exit_status will be active
    };

    Kind kind;

    union
    {
        TestServer::Handle *handle;
        ExitStatus exit_status;
    };
};

// The following two functions are to be defined by each platform implementation

/**
 * @brief Start a TestServer process
 *
 * All memory passed to this function is copied to the child process as
 * required.
 *
 * @param args - arguments to pass to the TestServer binary.
 *              These strings will end up as the `argv` array for the process.
 * @param env - environment to provide for the child process.
 *              Each entry must be of the form "KEY=VALUE" or "KEY=".
 * @param redirect_filename - file to redirect all output to
 * @param ready_string - string to wait for to know when ready
 * @param timeout - how long until we give up waiting for `ready_string`
 * @return - the outcome of starting the server. See TangoTest::platform::StartServerResult.
 */
StartServerResult start_server(const std::vector<std::string> &args,
                               const std::vector<std::string> &env,
                               const std::string &redirect_filename,
                               const std::string &ready_string,
                               std::chrono::milliseconds timeout);

struct StopServerResult
{
    enum class Kind
    {
        ExitedEarly, // The server had already exited when `stop_server()` was called,
                     // exit_status is set
        Exiting,     // The server has been signalled to stop, exit_status is
                     // undefined
    };

    Kind kind;
    ExitStatus exit_status;
};

/**
 * @brief Signal to a server to stop a server
 *
 * @param handle - a server returned by
 *                 TangoTest::platform::start_server
 * @param timeout - how long until we give up stopping the server
 * @return - the outcome of stopping the server. See
 *           TangoTest::platform::StopServerResult
 */
StopServerResult stop_server(TestServer::Handle *handle);

struct WaitForStopResult
{
    enum class Kind
    {
        Timeout, // Timed out waiting for the sever to exit, exit_status
                 // undefined
        Exited,  // The server stopped with exit_status
    };

    Kind kind;
    ExitStatus exit_status;
};

WaitForStopResult wait_for_stop(TestServer::Handle *handle, std::chrono::milliseconds timeout);

} // namespace TangoTest::platform

#endif
