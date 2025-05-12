#ifndef TANGO_COMMON_UTILS_TYPE_INFO_H
#define TANGO_COMMON_UTILS_TYPE_INFO_H

#include <tango/common/tango_const.h>

#include <string>

namespace Tango::detail
{

/**
 * @brief Return type name for a CORBA::Any
 *
 * @param any The CORBA::Any
 */
std::string corba_any_to_type_name(const CORBA::Any &any);

/**
 * @brief Return the type name for a AttrUnion dtype
 *
 * @param d The dtype
 */
std::string attr_union_dtype_to_type_name(Tango::AttributeDataType d);

} // namespace Tango::detail

#endif
