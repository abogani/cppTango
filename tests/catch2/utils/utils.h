#ifndef TANGO_TESTS_CATCH2_UTILS_UTILS_H
#define TANGO_TESTS_CATCH2_UTILS_UTILS_H

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "utils/auto_device_class.h"
#include "utils/test_server.h"
#include "utils/callback_mock.h"
#include "utils/matchers.h"
#include "utils/generators.h"

#include <tango/tango.h>

#include <memory>

namespace TangoTest
{

std::string make_nodb_fqtrl(int port, std::string_view device_name);

const char *get_current_log_file_path();

// TODO Multiple devices and/or multiple device servers
class Context
{
  public:
    /**
     * env is a vector with entries of the form "key1=value1", "key2=value2"
     */
    Context(const std::string &instance_name,
            const std::string &tmpl_name,
            int idlversion,
            std::vector<const char *> env = {});
    Context(const Context &) = delete;
    Context &operator=(Context &) = delete;

    ~Context() = default;

    std::unique_ptr<Tango::DeviceProxy> get_proxy();

    /** Stop the associated TestServer instance if it has been started.
     *
     *  If the instance has a non-zero exit status, then diagnostics will be
     *  output with the Catch2 WARN macro.
     *
     *  @param timeout if the server takes longer than this timeout,
     *  a warning log will be generated
     */
    void stop_server(std::chrono::milliseconds timeout = TestServer::k_default_timeout);

  private:
    TestServer m_server;
};

class DevFailedReasonMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    DevFailedReasonMatcher(const std::string &e);

    bool match(const Tango::DevFailed &exception) const;

    std::string describe() const override;

  private:
    const std::string reason;
};

inline DevFailedReasonMatcher DevFailedReasonEquals(std::string const &message)
{
    return DevFailedReasonMatcher(message);
}

namespace detail
{
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
