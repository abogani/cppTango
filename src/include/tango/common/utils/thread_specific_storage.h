#ifndef TANGO_COMMON_UTILS_TSS_H
#define TANGO_COMMON_UTILS_TSS_H

#include <map>
#include <omnithread.h>
#include <tango/idl/tango.h>
#include <tango/common/log4tango/Logger.h>
#if defined(TELEMETRY_ENABLED)
  #include <tango/common/telemetry/telemetry.h>
#endif

namespace Tango::utils::tss
{

#if defined(TELEMETRY_ENABLED)
extern thread_local std::shared_ptr<Tango::telemetry::Interface> current_telemetry_interface;
#endif

} // namespace Tango::utils::tss

#endif // TANGO_COMMON_UTILS_TSS_H