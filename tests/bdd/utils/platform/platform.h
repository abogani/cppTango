#ifndef TANGO_TESTS_BDD_PLATFORM_IMPL_COMMON_H
#define TANGO_TESTS_BDD_PLATFORM_IMPL_COMMON_H

#include "utils/bdd_server.h"

#include <vector>
#include <string>
#include <chrono>

namespace TangoTest::platform
{

constexpr static const char *k_bdd_server_binary_path = TANGO_TEST_BDD_SERVER_BINARY_PATH;
constexpr static const char *k_output_directory_path = TANGO_TEST_BDD_OUTPUT_DIRECTORY_PATH;

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
        BddServer::Handle *handle;
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
StopServerResult stop_server(BddServer::Handle *handle, std::chrono::milliseconds timeout);

} // namespace TangoTest::platform

#endif
