#ifndef TANGO_TESTS_BDD_UTILS_UTILS_H
#define TANGO_TESTS_BDD_UTILS_UTILS_H

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "utils/auto_device_class.h"

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
/// It will instantiate a TangoTest::AutoDeviceClass for DEVICE<TANGO_BASE_CLASS>.

#define TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(DEVICE) \
    TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE<TANGO_BASE_CLASS>, DEVICE)

#endif
