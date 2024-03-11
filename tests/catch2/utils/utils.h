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

#include <tango/tango.h>

#include <memory>

namespace TangoTest
{

constexpr int IDL_MAX = Tango::DevVersion + 1;

std::string make_nodb_fqtrl(int port, std::string_view device_name);

// TODO Multiple devices and/or multiple device servers
class Context
{
  public:
    Context(const std::string &instance_name, const std::string &tmpl_name, int idlversion);
    Context(const Context &) = delete;
    Context &operator=(Context &) = delete;

    ~Context() = default;

    std::string info();

    std::unique_ptr<Tango::DeviceProxy> get_proxy();

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
// A unique identifier representing the random seed used by the test run.
// The intention is this that uid is easier to spot compared a string of numbers.
constexpr static size_t k_log_filename_prefix_length = 4;
extern char g_log_filename_prefix[k_log_filename_prefix_length];
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
