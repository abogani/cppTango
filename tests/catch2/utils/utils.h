#ifndef TANGO_TESTS_CATCH2_UTILS_UTILS_H
#define TANGO_TESTS_CATCH2_UTILS_UTILS_H

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "utils/auto_device_class.h"
#include "utils/test_server.h"
#include "utils/callback_mock.h"
#include "utils/stringmakers.h"
#include "utils/matchers.h"
#include "utils/generators.h"
#include <tango/internal/base_classes.h>
#include <tango/internal/stl_corba_helpers.h>

#include <tango/tango.h>

#include <memory>
#include <type_traits>

namespace TangoTest
{

std::string make_nodb_fqtrl(int port, std::string_view device_name, std::string_view attr_name = "");

const char *get_current_log_file_path();

/* @brief Return a disc location where a FileDatabase can be created
 *
 */
std::string get_next_file_database_location();

// TODO: Multiple devices per server
struct ServerDescriptor
{
    // Name of the device server instance
    std::string instance_name;
    // IDL version to instantiate the template with
    std::string class_name;
    // File database contents
    std::optional<int> idlversion{std::nullopt};
    // Name of the device class to instantiate if idlversion!=nullopt
    // or name of the class template to use
    std::optional<std::string> extra_filedb_contents{std::nullopt};
    // Additional environment entries of the form "key1=value1"
    // For some gcc (for at least 14.2.1) complains with
    // -Wmissing-field-initializers if we don't add the "{}" here.
    std::vector<std::string> extra_env{}; // NOLINT(readability-redundant-member-init)
};

struct ContextDescriptor
{
    std::vector<ServerDescriptor> servers;
};

// TODO: Multiple devices and/or multiple device servers
// TODO: Maybe we want a builder API for this
class Context
{
  public:
    /**
     * @brief Create a Tango Test Context with a single device server in nodb mode
     *
     * @param instance_name Name of the device server instance
     * @param class_name Name of the device class to instantiate
     * @param env environment entries of the form "key1=value1", "key2=value2"
     */
    Context(const std::string &instance_name, const std::string &class_name, std::vector<std::string> env = {});

    /**
     * @brief Create a Tango Test Context with a single device server in nodb mode
     *
     * @param instance_name Name of the device server instance
     * @param tmpl_name Name of the template device class to instantiate
     * @param idlversion IDL version of the device class to instantiate
     * @param env environment entries of the form "key1=value1", "key2=value2"
     */
    Context(const std::string &instance_name,
            const std::string &tmpl_name,
            int idlversion,
            std::vector<std::string> env = {});
    /**
     * @brief Create a Tango Test Context with a single device server in filedb mode
     *
     * TODO: Do not require users to pass the filedb_contents
     *
     * @param instance_name Name of the device server instance
     * @param tmpl_name Name of the template device class to instantiate
     * @param idlversion IDL version of the device class to instantiate
     * @param extra_filedb_contents Contents of the filedb to use
     * @param env environment entries of the form "key1=value1", "key2=value2"
     */
    Context(const std::string &instance_name,
            const std::string &tmpl_name,
            int idlversion,
            const std::string &extra_filedb_contents,
            std::vector<std::string> env = {});

    /**
     * @brief Create TangoTest servers as specified by the descriptor
     *
     * @param desc Description of device servers to start
     */
    explicit Context(const ContextDescriptor &desc);

    Context(const Context &) = delete;
    Context &operator=(Context &) = delete;

    ~Context();

    // TODO: These should also take the device_name, however, we should also
    // allow the device name to be specified in the ServerDescriptor if we do
    // this.

    /** Return the fully qualified tango resource locator for a device or
     *  attribute.
     *
     *  @param instance server instance to provide the device for
     *  @param attr_name name of the attribute to be included in the FQTRL. If
     *  empty a FQTRL for the device will be returned
     *
     *  Expects: the instance to have been specified at construction
     */
    std::string get_fqtrl(std::string_view instance, std::string_view attr_name = "");

    /** Return a device proxy to the device in the only server
     *
     *  Expects: there was only one server specified at construction
     */
    std::unique_ptr<Tango::DeviceProxy> get_proxy();

    /** Return a device proxy to the device a server
     *
     *  @param instance server instance to provide the device for
     *
     *  Expects: the instance to have been specified at construction
     */
    std::unique_ptr<Tango::DeviceProxy> get_proxy(std::string_view instance);

    /** Return an admin device proxy to the device in the only server
     *
     *  Expects: there was only one server specified at construction
     */
    std::unique_ptr<Tango::DeviceProxy> get_admin_proxy();

    /** Return an admin device proxy to the device a server
     *
     *  @param instance server instance to provide the device for
     *
     *  Expects: the instance to have been specified at construction
     */
    std::unique_ptr<Tango::DeviceProxy> get_admin_proxy(std::string_view instance);

    /** Wait until the server stops.
     *
     *  Intended to be called if the server is being stopped by some means
     *  outside of the test infrastructure, e.g. the DServer Kill command.
     *
     *  @param timeout to wait for the server to stop
     *
     *  Returns the exit status of the server.
     *
     *  Throws: a `runtime_error` if the timeout is exceeded.
     *
     *  Expects: there was only one server specified at construction
     */
    ExitStatus wait_for_exit(std::chrono::milliseconds timeout = TestServer::k_default_timeout);

    /** Stop the associated TestServer instance if it has been started.
     *
     *  If the instance has a non-zero exit status, then diagnostics will be
     *  output with the Catch2 WARN macro.
     *
     *  Returns when the server has stopped or the timeout has been reached
     *
     *  @param timeout if the server takes longer than this timeout,
     *  a warning log will be generated
     *
     *  Expects: there was only one server specified at construction
     */
    void stop_server(std::chrono::milliseconds timeout = TestServer::k_default_timeout);

    /*
     * Return the disc location of the FileDatabase, throws if there is none.
     *
     * Expects: there was only one server specified at construction
     */
    std::string get_file_database_path();

    /*
     * Return the name of the Tango device class
     *
     * Expects: there was only one server specified at construction
     */
    std::string get_class_name();

    /**
     * Get the server redirection file
     *
     * Expects: there was only one server specified at construction
     */
    const std::string &get_redirect_file() const;

    /** Restart the server if it has been stopped using the same port.
     *
     * @param timeout -- how long to wait for the server to start
     *
     * Throws: a `runtime_error` if the server cannot be restarted
     *
     * Expects: `stop_server()` has been called previously
     *          there was only one server specified at construction
     */
    void restart_server(std::chrono::milliseconds timeout = TestServer::k_default_timeout);

  private:
    void add_server_job(const ServerDescriptor &desc);

    struct ServerJob
    {
        TestServer process;
        std::string instance_name;
        std::string class_name;
        std::string device_name;
        std::vector<std::string> extra_args;
        std::vector<std::string> extra_env;
        std::optional<std::string> filedb_path = std::nullopt;
    };

    ServerJob &get_only_job();
    const ServerJob &get_only_job() const;

    ServerJob &get_job(std::string_view instance);
    const ServerJob &get_job(std::string_view instance) const;

    std::vector<ServerJob> m_server_jobs;
};

namespace detail
{
constexpr const char *k_log_file_env_var = "TANGO_TEST_LOG_FILE";
// Setup a log appender to the file specified in the environment variable
// TANGO_TEST_LOG_FILE.  Each log is prefixed with `topic`, this allows
// multiple processes to log to this file at the same time.
//
// If the environment variable is not set, this does nothing.
//
// Expects: Tango::Logging::get_core_logger() != nullptr
void setup_topic_log_appender(std::string_view topic, const char *filename = nullptr);

// A unique identifier representing the random seed used by the test run to make
// it easier for the user to identify log files.  This is constructed during the
// testRunStarting Catch2 event.
extern std::string g_log_filename_prefix;

// Return a filename containing the test_case_name, starting with
// g_log_filename_prefix and ending with suffix.
//
// The filename will not exceed the path component size limit on the platform.
// And any spaces (' ') in the test_case_name will be replaced with underscores
// ('_').
std::string filename_from_test_case_name(std::string_view test_case_name, std::string_view suffix);
} // namespace detail

template <typename T>
void require_event(T &callback)
{
    // discard the event we get when we subscribe again
    auto maybe_initial_event = callback.pop_next_event();
    REQUIRE(maybe_initial_event != std::nullopt);
}

template <typename T>
void require_initial_events(T &callback)
{
    auto maybe_initial_event = callback.pop_next_event();
    REQUIRE(maybe_initial_event != std::nullopt);

    maybe_initial_event = callback.pop_next_event();
}

template <typename T, typename U>
void require_initial_events(T &callback, U initial_value)
{
    using namespace Catch::Matchers;
    using namespace TangoTest::Matchers;

    // We get the following two initial events (the fact there
    // are two is a side effect of the fix for #369):
    //
    // 1. In `subscribe_event` we do a `read_attribute` to
    // generate the first event
    // 2. Because we are the first subscriber to `"attr"`, the
    // polling loop starts and sends an event because it is the
    // first time it has read the attribute

    auto maybe_initial_event = callback.pop_next_event();
    REQUIRE(maybe_initial_event != std::nullopt);

    if constexpr(std::is_floating_point_v<U>)
    {
        REQUIRE_THAT(maybe_initial_event,
                     EventValueMatches(AnyLikeMatches<U>(WithinAbs(initial_value, static_cast<U>(0.0000001)))));
    }
    else
    {
        REQUIRE_THAT(maybe_initial_event, EventValueMatches(AnyLikeContains(initial_value)));
    }

    maybe_initial_event = callback.pop_next_event();
}

/// RAII class for event subscription
class Subscription : public Tango::detail::NonCopyable
{
  public:
    template <typename... Args>
    explicit Subscription(std::shared_ptr<Tango::DeviceProxy> dev, Args &&...args) :
        m_dev(dev),
        m_id(dev->subscribe_event(std::forward<Args>(args)...))
    {
    }

    ~Subscription()
    {
        m_dev->unsubscribe_event(m_id);
    }

  private:
    std::shared_ptr<Tango::DeviceProxy> m_dev;
    int m_id;
};

} // namespace TangoTest

/**
 * @brief Instantiate a TangoTest::AutoDeviceClass for template class DEVICE
 *
 * This macro expects that the DEVICE takes its base class as a template
 * parameter:
 *
 *     template<typename Base>
 *     class DEVICE : public Base {
 *         ...
 *     };
 *
 * It will instantiate a TangoTest::AutoDeviceClass with a base class using IDL
 * version MIN and onwards.
 *
 * @param DEVICE class template
 * @param MIN minimum DeviceImpl version to use
 */
#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(DEVICE, MIN) TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_##MIN(DEVICE)

#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_1(DEVICE)                           \
    TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE<Tango::DeviceImpl>, DEVICE##_1) \
    TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_2(DEVICE)

#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_2(DEVICE)                             \
    TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE<Tango::Device_2Impl>, DEVICE##_2) \
    TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_3(DEVICE)

#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_3(DEVICE)                             \
    TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE<Tango::Device_3Impl>, DEVICE##_3) \
    TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_4(DEVICE)

#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_4(DEVICE)                             \
    TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE<Tango::Device_4Impl>, DEVICE##_4) \
    TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_5(DEVICE)

#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_5(DEVICE)                             \
    TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE<Tango::Device_5Impl>, DEVICE##_5) \
    TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_6(DEVICE)

#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE_6(DEVICE) \
    TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE<Tango::Device_6Impl>, DEVICE##_6)

#endif
