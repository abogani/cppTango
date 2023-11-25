
#include <tango/common/utils/thread_specific_storage.h>

namespace Tango::utils::tss
{

thread_local std::shared_ptr<Tango::telemetry::Interface> current_telemetry_interface =
    std::make_shared<Tango::telemetry::Interface>();

} // namespace Tango::utils::tss
