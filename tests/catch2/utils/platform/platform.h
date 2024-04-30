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
        int exit_status;
    };
};

// The following two functions are to be defined by each platform implementation

/** Start the server with the given args redirecting output to the file
 * specified by redirect_filename.
 *
 * Wait for the server to output ready_string before returning.
 */
StartServerResult start_server(const std::vector<const char *> &args,
                               const std::vector<const char *> &env,
                               const std::string &redirect_filename,
                               const std::string &ready_string,
                               std::chrono::milliseconds timeout);

struct StopServerResult
{
    enum class Kind
    {
        Timeout,     // Timed out waiting for the sever to exit, exit_status
                     // undefined
        ExitedEarly, // The server had already exited when `stop()` was called,
                     // exit_status is set
        Exited,      // The server stopped with exit_status
    };

    Kind kind;
    int exit_status;
};

/** Stop the server specified by handle.
 *
 */
StopServerResult stop_server(TestServer::Handle *handle, std::chrono::milliseconds timeout);

} // namespace TangoTest::platform

#endif
