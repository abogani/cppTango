#include <tango/tango.h>
#include <tango/internal/utils.h>

namespace Tango::detail
{

bool IDLVersionIsTooOld(int version, int desiredVersion)
{
    return version > detail::INVALID_IDL_VERSION && version < desiredVersion;
}

} // namespace Tango::detail
