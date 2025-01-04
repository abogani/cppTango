#ifndef TANGO_TESTS_CATCH2_UTILS_TEST_SERVER_H
#define TANGO_TESTS_CATCH2_UTILS_TEST_SERVER_H

#include <chrono>
#include <memory>
#include <string>
#include <optional>
#include <vector>

namespace TangoTest
{

class Logger
{
  public:
    virtual void log(const std::string &) = 0;

    virtual ~Logger() { }
};

struct ExitStatus
{
    enum class Kind
    {
        Normal,          // The TestServer exited normally, code is active.
        Aborted,         // The TestServer was Aborted, signal is active.
        AbortedNoSignal, // The TestServer was Aborted, neither code nor signal
                         // are defined.
    };

    Kind kind;

    union
    {
        int code;   // exit code of the TestServer
        int signal; // signal used to abort the TestServer
    };

    bool is_success()
    {
        return kind == Kind::Normal && code == 0;
    }
};

std::ostream &operator<<(std::ostream &os, const ExitStatus &status);

/* RAII class for a TestServer process
 */
class TestServer
{
  public:
    constexpr static const int k_num_port_tries = 5;
    constexpr static const char *k_ready_string = "Ready to accept request";
    constexpr static const char *k_port_in_use_string = "INITIALIZE_TransportError";
    constexpr static const char *k_start_bg_thread = "TANGO_TEST_SERVER_START_BG_THREAD";
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
     *
     *  Does nothing if `!is_running()`.
     *
     *  @param timeout -- how long to wait for the server to exit
     */
    void stop(std::chrono::milliseconds timeout = k_default_timeout);

    /**
     * The list of signals that can be sent from one process to another
     * that are of relevance to the Tango server signal handling logic.
     */
    static std::vector<int> relevant_sendable_signals();

    /** Send a signal to the server process.
     *
     *  @param signo -- the signal number
     */
    void send_signal(int signo);

    /** Wait until the server has exited.
     *
     *  Returns the exit status of the server.
     *
     *  Raises an exception if the timeout is exceed.
     *
     *  @param timeout -- how long to wait for the server to exit
     *
     *  Expects: `is_running()`
     */
    ExitStatus wait_for_exit(std::chrono::milliseconds timeout = k_default_timeout);

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

    // Set if the test has called wait_for_exit() and it didn't timeout.
    std::optional<ExitStatus> m_exit_status = std::nullopt;
};

} // namespace TangoTest

#endif
