#ifndef _INTERNAL_UTILS_H
#define _INTERNAL_UTILS_H

#include <tango/common/tango_const.h>

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Tango
{
class DeviceAttribute;
class DeviceProxy;
class Database;
} // namespace Tango

namespace Tango::detail
{

constexpr int INVALID_IDL_VERSION = 0;

/// @brief Check wether the given IDL version is at least the desired IDL version
///
/// Helper function for DeviceProxy/MultiAttribute which ignores unconnected
/// devices with version 0 (which is not a valid IDL version).
bool IDLVersionIsTooOld(int version, int desiredVersion);

template <typename T>
void stringify_vector(std::ostream &os, const std::vector<T> &vec, const std::string_view sep)
{
    const auto length = vec.size();
    if(length == 0)
    {
        return;
    }

    typename std::vector<T>::size_type i;

    TANGO_ASSERT(length > 0);
    for(i = 0; i < (length - 1); i++)
    {
        os << vec[i] << sep;
    }

    os << vec[i];
}

/// @brief Convert the given string to lower case
std::string to_lower(std::string str);

/// @brief Convert the given string to UPPER case
std::string to_upper(std::string str);

/// @brief Parse the given lower case string as boolean
///
/// Returns an optional without value in case of error.
std::optional<bool> to_boolean(std::string_view str);

/// @brief Lookup the environment variable `env_var` and return its contents as boolean
///
/// Return `default_value` in case it is not present, throws for unkonwn content
bool get_boolean_env_var(const char *env_var, bool default_value);

void stringify_any(std::ostream &os, const CORBA::Any &any);

void stringify_attribute_data(std::ostream &os, const DeviceAttribute &da);

/// @brief Query the database server for the list of defined databases (Command: DbGetCSDbServerList)
std::vector<std::string> get_databases_from_control_system(Database *db);

/// @brief Gather all prefixes of the form `tango://db_host.eu:10000` from the TANGO_HOST environment variable
std::vector<std::string> gather_fqdn_prefixes_from_env(Database *db);

/// @brief Append all prefix of the form `tango://db_host.eu:10000` in vs to prefixes if not already present
///
/// @param vs       Return value from get_databases_from_control_system()
/// @param prefixes Vector with existing prefixes
void append_fqdn_host_prefixes_from_db(const std::vector<std::string> &vs, std::vector<std::string> &prefixes);

/// @brief Given a device proxy with name `A/B/C` construct a fully qualified TRL
///
/// With SQL Database on host `my-db-host.eu` and port `10000`:
/// - tango://my-db-host.eu:10000/a/b/c
///
/// Without the SQL Database (either FileDatabase or no database at all)
/// and the DS on host `ds-host.eu` and port `12000`:
/// - tango://ds-host.eu:12000/a/b/c#dbase=no
///
/// The prefixes vector is returned by detail::gather_fqdn_prefixes_from_env and/or
/// detail::append_fqdn_host_prefixes_from_db.
std::string build_device_trl(DeviceProxy *device, const std::vector<std::string> &prefixes);

/// @brief Add the `idl5_` prefix to the event name
std::string add_idl_prefix(std::string event_name);

/// @brief Remove the `idlXX` prefix from the event name
std::string remove_idl_prefix(std::string event_name);

/// @brief Extract the IDL version `5` from a string like `idl5_change`
std::optional<int> extract_idl_version_from_event_name(const std::string &event_name);

/// @brief Insert `idl5_` after the last `.`
std::string insert_idl_for_compat(std::string event_name);

} // namespace Tango::detail

namespace Tango
{

std::ostream &operator<<(std::ostream &o_str, const AttrQuality &attr_quality);

std::ostream &operator<<(std::ostream &os, const ErrSeverity &error_severity);

} // namespace Tango

#endif // _INTERNAL_UTILS_H
