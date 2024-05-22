#ifndef TANGO_TESTS_CATCH2_UTILS_TEST_SERVER_H
#define TANGO_TESTS_CATCH2_UTILS_TEST_SERVER_H

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace TangoTest
{

class Logger
{
  public:
    virtual void log(const std::string &) = 0;

    virtual ~Logger() { }
};

/* RAII class for a TestServer process
 */
class TestServer
{
  public:
    constexpr static const int k_num_port_tries = 5;
    constexpr static const char *k_ready_string = "Ready to accept request";
    constexpr static const char *k_port_in_use_string = "INITIALIZE_TransportError";
    constexpr static std::chrono::milliseconds k_default_timeout{5000};

    TestServer() = default;

    TestServer(const TestServer &) = delete;
    TestServer &operator=(TestServer &) = delete;

    // TODO: Run with TangoDB?

    /** Starts a TestServer instance with a single device of the specified class.
     *
     *  The ctor only finishes after the ready string ("Ready to accept request")
     *  has been output.
     *
     *  The first time the `start` is called a free port is randomly chosen.  On
     *  subsequent calls the same port is always used, is that port is
     *  unavailable `start` will fail.
     *
     *  @param instance_name -- name of the TestServer instance
     *  @param extra_args -- additional cli arguments to pass to the TestServer
     *  @param extra_env -- additional environment variables to set for the TestServer
     *  @param timeout -- how long to wait for the ready string
     *
     *  Throws a std::runtime_exception in case the server fails to start, or
     *  takes too long to output the ready string, or we fail to find a free port
     *  after a `k_num_port_tries` attempts.
     *
     *  Expects: `!this->is_running()` and `instance_name != ""`
     */
    void start(const std::string &instance_name,
               const std::vector<std::string> &extra_args,
               const std::vector<std::string> &extra_env,
               std::chrono::milliseconds timeout = k_default_timeout);

    /** Stop the TestServer instance if it has been started.
     *
     *  If the instance has a non-zero exit status, then diagnostics will be
     *  output with the Catch2 WARN macro.
     *
     *  After the server has been stopped, it can be started again using the
     *  same port by calling `start`.
     */
    void stop(std::chrono::milliseconds timeout = k_default_timeout);

    ~TestServer();

    /** Returns `true` if the server has been start()'d and not yet stop()'d.
     */
    bool is_running()
    {
        return m_handle != nullptr;
    }

    /** Return the port that the server is connected to.
     */
    int get_port() const
    {
        return m_port;
    }

    const std::string &get_redirect_file() const
    {
        return m_redirect_file;
    }

    struct Handle;

    // We store the next port here so that it can be controlled from a test (for
    // internal testing).
    static int s_next_port;
    static std::unique_ptr<Logger> s_logger;

  private:
    Handle *m_handle = nullptr;
    int m_port = -1;
    std::string m_redirect_file;
};

} // namespace TangoTest

#endif
