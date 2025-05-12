#ifndef _TANGO_BASE_CLASS_H
#define _TANGO_BASE_CLASS_H

// FIXME remove once https://gitlab.com/tango-controls/cppTango/-/issues/786 is fixed
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdeprecated"
#endif

#include <tango/idl/tango.h>

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#define TANGO_BASE_CLASS Tango::Device_6Impl
// include the corresponding header
#include <tango/server/device_6.h>

#endif
