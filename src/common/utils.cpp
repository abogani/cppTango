#include <tango/tango.h>
#include <tango/internal/utils.h>

namespace Tango
{
namespace detail
{

bool IDLVersionIsTooOld(int version, int desiredVersion)
{
    return version > detail::INVALID_IDL_VERSION && version < desiredVersion;
}

} // namespace detail
} // namespace Tango
