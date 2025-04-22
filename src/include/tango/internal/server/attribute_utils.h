#ifndef _INTERNAL_ATTRIBUTE_UTILS_H
#define _INTERNAL_ATTRIBUTE_UTILS_H

#include <tango/common/tango_const.h>

#include <string>
#include <map>

namespace Tango
{
union _Attr_CheckVal;
typedef _Attr_CheckVal Attr_CheckVal;
class Attribute;
} // namespace Tango

namespace Tango::detail
{

enum PropType
{
    MIN_VALUE = 0,
    MAX_VALUE,
    MIN_WARNING,
    MAX_WARNING,
    MIN_ALARM,
    MAX_ALARM
};

//+-------------------------------------------------------------------------
//
// method :         Attribute::upd_att_prop_db
//
// description :     Update the tango database with the new attribute
//            values
//
//--------------------------------------------------------------------------
void upd_att_prop_db(const Tango::Attr_CheckVal &new_value,
                     const char *prop_name,
                     const std::string &d_name,
                     const std::string &name,
                     long data_type);
//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        delete_startup_exception
//
// description :
//        Deletes the exception related to the property name from startup_exceptions map
//
// argument :
//         in :
//            - prop_name : The property name as a key for which the exception is to be deleted from
//                        startup_exceptions map
//          - dev_name : The device name
//
//-------------------------------------------------------------------------------------------------------------------
void delete_startup_exception(Tango::Attribute &attr,
                              std::string prop_name,
                              std::string dev_name,
                              bool &check_startup_exceptions,
                              std::map<std::string, Tango::DevFailed> &startup_exceptions);

//+-------------------------------------------------------------------------
//
// method :         throw_err_data_type
//
// description :     Throw a Tango DevFailed exception when an error on
//                    data type is detected
//
// in :        prop_name : The property name
//            dev_name : The device name
//            origin : The origin of the exception
//
//--------------------------------------------------------------------------
void throw_err_data_type(const char *prop_name,
                         const std::string &dev_name,
                         const std::string &name,
                         const char *origin);

//+-------------------------------------------------------------------------
//
// method :         Attribute::throw_incoherent_val_err
//
// description :     Throw a Tango DevFailed exception when the min or max
//                property is incoherent with its counterpart
//
// in :            min_prop : The min property name
//            max_prop : The max property name
//            dev_name : The device name
//            origin : The origin of the exception
//
//--------------------------------------------------------------------------
void throw_incoherent_val_err(const char *min_prop,
                              const char *max_prop,
                              const std::string &dev_name,
                              const std::string &name,
                              const char *origin);
//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::throw_err_format
//
// description :
//        Throw a Tango DevFailed exception when an error format is detected in the string which should be converted
//        to a number
//
// argument :
//         in :
//            - prop_name : The property name
//            - dev_name : The device name
//            - origin : The origin of the exception
//
//--------------------------------------------------------------------------------------------------------------------
void throw_err_format(const char *prop_name, const std::string &dev_name, const std::string &name, const char *origin);

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//        avns_in_att()
//
// description :
//        Store in att the famous AVNS (AlrmValueNotSpec) for a specific attribute property
//
// Arguments:
//        in :
//            - pt : Property type
//
//---------------------------------------------------------------------------------------------------------------------
void avns_in_att(Tango::Attribute &attr, const std::string &d_name, bool &check_value, std::string &value_str);

//+-------------------------------------------------------------------------
//
// method :         avns_in_db()
//
// description :     Store in db the famous AVNS (AlrmValueNotSpec)
//                  for a specific attribute property
//
// Arg in :            prop_name : Property name
//
//--------------------------------------------------------------------------

void avns_in_db(const char *prop_name, const std::string &name, const std::string &dev_name);

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        prop_in_list
//
// description :
//        Search for a property in a list
//
// args:
//        in :
//            - prop_name : The property name
//          - list_size : The size list
//          - list : The list
//      out :
//            - prop_str : String initialized with prop. value (if found)
//
//------------------------------------------------------------------------------------------------------------------
bool prop_in_list(const char *prop_name, std::string &prop_str, const std::vector<AttrProperty> &list);

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        compute_data_size
//
// description :
//        Compute the attribute amount of data
//
//--------------------------------------------------------------------------------------------------------------------
long compute_data_size(const Tango::AttrDataFormat &data_format, long dim_x, long dim_y);
} // namespace Tango::detail
#endif
