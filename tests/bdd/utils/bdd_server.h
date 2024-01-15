#ifndef TANGO_TESTS_BDD_UTILS_BDD_SERVER_H
#define TANGO_TESTS_BDD_UTILS_BDD_SERVER_H

#include <vector>
#include <string>
#include <chrono>

namespace TangoTest
{

/* RAII class for a BddServer process
 */
class BddServer
{
  public:
    constexpr static const int k_num_port_tries = 5;
    constexpr static const char *k_ready_string = "Ready to accept request";
    constexpr static const char *k_port_in_use_string = "INITIALIZE_TransportError";
    constexpr static std::chrono::milliseconds k_timeout{5000};

    BddServer() = default;

    BddServer(const BddServer &) = delete;
    BddServer &operator=(BddServer &) = delete;

    // TODO: Run with TangoDB?

    /** Starts a BddServer instance with a single device of the specified class.
     *
     *  The ctor only finishes after the ready string ("Ready to accept request")
     *  has been output.
     *
     *  A free port is randomly chosen.
     *
     *  @param extra_args Arguments to
     *
     *  Throws a std::runtime_exception in case the server fails to start, or
     *  takes too long to output the ready string, or we fail to find a free port
     *  after a `num_port_tries` attempts.
     */
    void start(const std::string &instance_name, std::vector<const char *> extra_args);

    /** Stop the BddServer instance if it has been started.
     *
     *  If the instance has a non-zero exit status, then diagnostics will be
     *  output with the Catch2 WARN macro.
     */
    void stop();

    ~BddServer();

    /** Return the port that the server is connected to.
     */
    int get_port() const
    {
        return m_port;
    }

    struct Handle;

  private:
    Handle *m_handle = nullptr;
    int m_port = -1;
    std::string m_redirect_file;
};

} // namespace TangoTest

#endif
