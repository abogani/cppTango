#ifndef TANGO_TESTS_BDD_UTILS_UTILS_H
#define TANGO_TESTS_BDD_UTILS_UTILS_H

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "utils/auto_device_class.h"
#include "utils/bdd_server.h"

#include <tango/tango.h>

#include <memory>

namespace TangoTest
{

std::string make_nodb_fqtrl(int port, std::string_view device_name);

// TODO Multiple devices and/or multiple device servers
class Context
{
  public:
    Context(const std::string &instance_name, const std::string &tmpl_name, int idlversion);
    Context(const Context &) = delete;
    Context &operator=(Context &) = delete;

    ~Context();

    std::string info();

    std::unique_ptr<Tango::DeviceProxy> get_proxy();

  private:
    BddServer m_server;
};

} // namespace TangoTest

//+ Instantiate a TangoTest::AutoDeviceClass for template class DEVICE
///
/// This macro expects that the DEVICE takes its base class as a template
/// parameter:
///
///     template<typename Base>
///     class DEVICE : public Base {
///         ...
///     };
///
/// It will instantiate a TangoTest::AutoDeviceClass with a base class using IDL
/// version MIN and onwards.
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
