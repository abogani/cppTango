//+============================================================================
//
// file :               w_attribute.cpp
//
// description :        C++ source code for the WAttribute class.
//            This class is used to manage attribute.
//            A Tango Device object instance has one
//            MultiAttribute object which is an aggregate of
//            Attribute or WAttribute objects
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
//
//
//-============================================================================

#include <tango/server/attribute.h>
#include <tango/server/w_attribute.h>
#include <tango/server/classattribute.h>
#include <tango/server/deviceclass.h>
#include <tango/server/utils.h>
#include <tango/server/device.h>
#include <tango/server/tango_clock.h>
#include <tango/common/utils/type_info.h>
#include <tango/server/seqvec.h>
#include <tango/client/Database.h>
#include <tango/internal/server/attribute_utils.h>

#include <cmath>

#ifdef _TG_WINDOWS_
  #include <sys/types.h>
  #include <float.h>
#endif /* _TG_WINDOWS_ */

namespace
{

template <class T>
void _update_value(T &old_value, T &write_value, const typename Tango::tango_type_traits<T>::ArrayType &seq)
{
    old_value = write_value;
    write_value = seq[0];
}

template <>
void _update_value(Tango::DevString &old_value, Tango::DevString &write_value, const Tango::DevVarStringArray &seq)
{
    Tango::string_free(old_value);
    old_value = Tango::string_dup(write_value);
    Tango::string_free(write_value);

    write_value = Tango::string_dup(seq[0]);
}

template <class T>
const T &get_value(const Tango::AttrValUnion &);

template <>
inline const Tango::DevVarDoubleArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.double_att_value();
}

template <>
inline const Tango::DevVarFloatArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.float_att_value();
}

template <>
inline const Tango::DevVarLongArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.long_att_value();
}

template <>
inline const Tango::DevVarULongArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.ulong_att_value();
}

template <>
inline const Tango::DevVarLong64Array &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.long64_att_value();
}

template <>
inline const Tango::DevVarULong64Array &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.ulong64_att_value();
}

template <>
inline const Tango::DevVarShortArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.short_att_value();
}

template <>
inline const Tango::DevVarUShortArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.ushort_att_value();
}

template <>
inline const Tango::DevVarBooleanArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.bool_att_value();
}

template <>
inline const Tango::DevVarCharArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.uchar_att_value();
}

template <>
inline const Tango::DevVarStringArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.string_att_value();
}

template <>
inline const Tango::DevVarStateArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.state_att_value();
}

template <>
inline const Tango::DevVarEncodedArray &get_value(const Tango::AttrValUnion &att_union)
{
    return att_union.encoded_att_value();
}

void _throw_incompatible_exception(Tango::CmdArgType expected, const std::string &found)
{
    std::stringstream o;

    o << "Incompatible attribute type: expected Tango::" << expected
      << " (even for single value), found Tango::" << found << std::ends;
    Tango::Except::throw_exception((const char *) Tango::API_IncompatibleAttrDataType,
                                   o.str(),
                                   (const char *) "WAttribute::check_written_value()");
}

template <class T>
void _copy_data(T &last_written_value, const CORBA::Any &any)
{
    const T *ptr;
    any >>= ptr;
    last_written_value = *ptr;
}

template <class T>
void _copy_data(T &last_written_value, const Tango::AttrValUnion &the_union)
{
    last_written_value = get_value<T>(the_union);
}

template <class T>
bool _check_for_nan(Tango::Util *)
{
    return false;
}

template <>
inline bool _check_for_nan<Tango::DevDouble>(Tango::Util *tg)
{
    return !tg->is_wattr_nan_allowed();
}

template <>
inline bool _check_for_nan<Tango::DevFloat>(Tango::Util *tg)
{
    return !tg->is_wattr_nan_allowed();
}

template <class T>
void check_enum(Tango::WAttribute &,
                const std::vector<std::string> &,
                const typename Tango::tango_type_traits<T>::ArrayType &,
                const size_t)
{
}

template <>
void check_enum<Tango::DevShort>(Tango::WAttribute &attr,
                                 const std::vector<std::string> &enum_labels,
                                 const Tango::DevVarShortArray &seq,
                                 const size_t nb_data)
{
    const auto data_type = attr.get_data_type();
    const auto &name = attr.get_name();
    //
    // If the attribute is enumerated, check the input value compared to the enum labels
    //
    if(data_type == Tango::DEV_ENUM)
    {
        std::size_t max_val = enum_labels.size();
        for(std::size_t i = 0; i < nb_data; i++)
        {
            if(seq[i] < 0 || static_cast<std::size_t>(seq[i]) >= max_val)
            {
                std::stringstream o;
                o << "Set value for attribute " << name << " is negative or above the maximun authorized (" << max_val
                  << ") for at least element " << i;

                TANGO_THROW_EXCEPTION(Tango::API_WAttrOutsideLimit, o.str());
            }
        }
    }
}

template <typename T,
          std::enable_if_t<!(std::is_same_v<T, Tango::DevDouble> || std::is_same_v<T, Tango::DevFloat>), T> * = nullptr>
void check_nan(const std::string &, const T &, const size_t)
{
}

template <typename T,
          std::enable_if_t<(std::is_same_v<T, Tango::DevDouble> || std::is_same_v<T, Tango::DevFloat>), T> * = nullptr>
void check_nan(const std::string &, const T &, size_t);

template <typename T,
          std::enable_if_t<(std::is_same_v<T, Tango::DevDouble> || std::is_same_v<T, Tango::DevFloat>), T> *>
void check_nan(const std::string &name, const T &val, const size_t i)
{
    if(std::isfinite(val) == 0)
    {
        std::stringstream o;

        o << "Set value for attribute " << name << " is a NaN or INF value (at least element " << i << ")" << std::ends;

        TANGO_THROW_EXCEPTION(Tango::API_WAttrOutsideLimit, o.str());
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        check_data_limits()
//
// description :
//        Check if the data received from client is valid.
//      It will check for nan value if needed, if itis not below the min (if one defined) or above the max
//      (if one defined), and for enum if it is in the accepted range.
//       This method throws exception in case of threshold violation.
//
// args :
//        in :
//          - nb_data : Data number
//          - seq : The received data
//          - min : The min allowed value
//          - max : The max allowed value
//
//------------------------------------------------------------------------------------------------------------------

template <class T>
void check_data_limits(Tango::WAttribute &attr,
                       const size_t nb_data,
                       const typename Tango::tango_type_traits<T>::ArrayType &seq,
                       Tango::Attr_CheckVal &min,
                       Tango::Attr_CheckVal &max,
                       const std::string &d_name,
                       const std::vector<std::string> &enum_labels,
                       const bool check_min_value,
                       const bool check_max_value)
{
    const T &min_value = min.get_value<T>();
    const T &max_value = max.get_value<T>();
    const auto &name = attr.get_name();
    //
    // If the server is in its starting phase, gives a nullptr
    // to the AutoLock object
    //

    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        mon_ptr = &(attr.get_att_device()->get_att_conf_monitor());
    }

    Tango::AutoTangoMonitor sync1(mon_ptr);

    bool check_for_nan = _check_for_nan<T>(tg);

    if(check_for_nan || check_min_value || check_max_value)
    {
        for(size_t i = 0; i < nb_data; ++i)
        {
            if(check_for_nan)
            {
                check_nan(name, seq[i], i);
            }
            if(check_min_value)
            {
                if(seq[i] < min_value)
                {
                    std::stringstream o;

                    o << "Set value for attribute " << name << " is below the minimum authorized (at least element "
                      << i << ")" << std::ends;
                    TANGO_THROW_EXCEPTION(Tango::API_WAttrOutsideLimit, o.str());
                }
            }
            if(check_max_value)
            {
                if(seq[i] > max_value)
                {
                    std::stringstream o;

                    o << "Set value for attribute " << name << " is above the maximum authorized (at least element "
                      << i << ")" << std::ends;
                    TANGO_THROW_EXCEPTION(Tango::API_WAttrOutsideLimit, o.str());
                }
            }
        }
    }

    check_enum<T>(attr, enum_labels, seq, nb_data);
}

template <>
void check_data_limits<Tango::DevEncoded>(Tango::WAttribute &attr,
                                          const size_t nb_data,
                                          const Tango::DevVarEncodedArray &seq,
                                          Tango::Attr_CheckVal &min,
                                          Tango::Attr_CheckVal &max,
                                          const std::string &d_name,
                                          const std::vector<std::string> &,
                                          const bool check_min_value,
                                          const bool check_max_value)
{
    const Tango::DevUChar &min_value = min.uch;
    const Tango::DevUChar &max_value = max.uch;
    const auto &name = attr.get_name();
    //
    // If the server is in its starting phase, gives a nullptr
    // to the AutoLock object
    //

    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        mon_ptr = &(attr.get_att_device()->get_att_conf_monitor());
    }

    Tango::AutoTangoMonitor sync1(mon_ptr);

    if(check_min_value || check_max_value)
    {
        for(size_t i = 0; i < nb_data; ++i)
        {
            size_t nb_data_elt = seq[i].encoded_data.length();
            for(size_t j = 0; j < nb_data_elt; ++j)
            {
                if(check_min_value)
                {
                    if(seq[i].encoded_data[j] < min_value)
                    {
                        std::stringstream o;

                        o << "Set value for attribute " << name << " is below the minimum authorized (at least element "
                          << i << ")" << std::ends;
                        TANGO_THROW_EXCEPTION(Tango::API_WAttrOutsideLimit, o.str());
                    }
                }
                if(check_max_value)
                {
                    if(seq[i].encoded_data[j] > max_value)
                    {
                        std::stringstream o;

                        o << "Set value for attribute " << name << " is above the maximum authorized (at least element "
                          << i << ")" << std::ends;
                        TANGO_THROW_EXCEPTION(Tango::API_WAttrOutsideLimit, o.str());
                    }
                }
            }
        }
    }
}

template <>
void check_data_limits<Tango::DevBoolean>(Tango::WAttribute &,
                                          const size_t,
                                          const Tango::DevVarBooleanArray &,
                                          Tango::Attr_CheckVal &,
                                          Tango::Attr_CheckVal &,
                                          const std::string &,
                                          const std::vector<std::string> &,
                                          const bool,
                                          const bool)
{
}

template <>
void check_data_limits<Tango::DevString>(Tango::WAttribute &,
                                         const size_t,
                                         const Tango::DevVarStringArray &,
                                         Tango::Attr_CheckVal &,
                                         Tango::Attr_CheckVal &,
                                         const std::string &,
                                         const std::vector<std::string> &,
                                         const bool,
                                         const bool)
{
}

void check_length(const unsigned int nb_data, unsigned long x, unsigned long y)
{
    if(((y == 0u) && nb_data != x) || ((y != 0u) && nb_data != (x * y)))
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrIncorrectDataNumber, "Incorrect data number");
    }
}

template <class T, class V>
void update_internal_sequence(Tango::WAttribute &attr,
                              Tango::Attr_CheckVal &min_value,
                              Tango::Attr_CheckVal &max_value,
                              long &w_dim_x,
                              long &w_dim_y,
                              T &old_value,
                              T &write_value,
                              V &write_value_ptr,
                              const std::string &d_name,
                              const std::vector<std::string> &enum_labels,
                              bool check_min_value,
                              bool check_max_value,
                              const typename Tango::tango_type_traits<T>::ArrayType &seq,
                              size_t x,
                              size_t y)
{
    auto nb_data = seq.length();
    check_length(nb_data, x, y);

    //
    // Check the incoming value
    //
    const auto &data_format = attr.get_data_format();
    check_data_limits<T>(
        attr, nb_data, seq, min_value, max_value, d_name, enum_labels, check_min_value, check_max_value);

    write_value_ptr = seq.get_buffer();

    if(data_format == Tango::SCALAR)
    {
        _update_value(old_value, write_value, seq);
        w_dim_x = 1;
        w_dim_y = 0;
    }
    else
    {
        w_dim_x = x;
        w_dim_y = y;
    }
}

template <class T, class V>
void _update_written_value(Tango::WAttribute &attr,
                           Tango::Attr_CheckVal &min_value,
                           Tango::Attr_CheckVal &max_value,
                           long &w_dim_x,
                           long &w_dim_y,
                           T &old_value,
                           T &write_value,
                           V &write_value_ptr,
                           const std::string &d_name,
                           const std::vector<std::string> &enum_labels,
                           bool check_min_value,
                           bool check_max_value,
                           const CORBA::Any &any,
                           std::size_t x,
                           std::size_t y)
{
    //
    // Check data type inside the any and data number
    //
    using ArrayType = typename Tango::tango_type_traits<T>::ArrayType;

    ArrayType *ptr;

    if((any >>= ptr) == false)
    {
        std::string found = Tango::detail::corba_any_to_type_name(any);
        Tango::CmdArgType expected = Tango::tango_type_traits<ArrayType>::type_value();
        _throw_incompatible_exception(expected, found);
    }

    update_internal_sequence(attr,
                             min_value,
                             max_value,
                             w_dim_x,
                             w_dim_y,
                             old_value,
                             write_value,
                             write_value_ptr,
                             d_name,
                             enum_labels,
                             check_min_value,
                             check_max_value,
                             *ptr,
                             x,
                             y);
}

template <class T, class V>
void _update_written_value(Tango::WAttribute &attr,
                           Tango::Attr_CheckVal &min_value,
                           Tango::Attr_CheckVal &max_value,
                           long &w_dim_x,
                           long &w_dim_y,
                           T &old_value,
                           T &write_value,
                           V &write_value_ptr,
                           const std::string &d_name,
                           const std::vector<std::string> &enum_labels,
                           bool check_min_value,
                           bool check_max_value,
                           const Tango::AttrValUnion &att_union,
                           std::size_t x,
                           std::size_t y)
{
    //
    // Check data type inside the union
    //
    using ArrayType = typename Tango::tango_type_traits<T>::ArrayType;

    if(att_union._d() != Tango::tango_type_traits<T>::att_type_value())
    {
        std::string found = Tango::detail::attr_union_dtype_to_type_name(att_union._d());
        Tango::CmdArgType expected = Tango::tango_type_traits<ArrayType>::type_value();
        _throw_incompatible_exception(expected, found);
    }

    const ArrayType &seq = get_value<ArrayType>(att_union);

    update_internal_sequence(attr,
                             min_value,
                             max_value,
                             w_dim_x,
                             w_dim_y,
                             old_value,
                             write_value,
                             write_value_ptr,
                             d_name,
                             enum_labels,
                             check_min_value,
                             check_max_value,
                             seq,
                             x,
                             y);
}

} // namespace

namespace Tango
{

namespace details
{
template <class T>
static bool check_rds_out_of_range(const typename tango_type_traits<T>::ArrayType &array_val,
                                   const typename tango_type_traits<T>::ArrayType &val_seq,
                                   const Tango::AttrDataFormat data_format,
                                   const Tango::Attr_CheckVal &delta_val,
                                   Tango::AttrQuality &quality,
                                   std::bitset<Tango::Attribute::alarm_flags::numFlags> &alarm)
{
    auto nb_written = array_val.length();
    auto nb_read = (data_format == Tango::SCALAR) ? 1 : val_seq.length();
    auto nb_data = std::min(nb_read, nb_written);

    for(std::size_t i = 0; i != nb_data; ++i)
    {
        auto last_val = array_val[i];
        auto curr_val = val_seq[i];
        bool out_of_range = false;
        if constexpr(std::is_same_v<T, double> || std::is_same_v<T, float>)
        {
            if(std::isnan(last_val) != std::isnan(curr_val))
            {
                out_of_range = true;
            }
        }
        if(!out_of_range)
        {
            if constexpr(std::is_signed_v<T>)
            {
                if(last_val < 0 && curr_val > std::numeric_limits<T>::max() + last_val)
                {
                    out_of_range = true;
                }
                else if(last_val >= 0 && curr_val <= std::numeric_limits<T>::min() + last_val)
                {
                    out_of_range = true;
                }
                else
                {
                    auto delta = last_val > curr_val ? last_val - curr_val : curr_val - last_val;
                    out_of_range = (delta >= delta_val.get_value<T>());
                }
            }
            else
            {
                auto delta = last_val > curr_val ? last_val - curr_val : curr_val - last_val;
                out_of_range = (delta >= delta_val.get_value<T>());
            }
        }
        if(out_of_range)
        {
            quality = Tango::ATTR_ALARM;
            alarm.set(Tango::Attribute::alarm_flags::rds);
            return true;
        }
    }
    return false;
}

static bool check_rds_out_of_range(const Tango::DevEncoded &encoded_val,
                                   const Tango::DevVarEncodedArray &enc_seq,
                                   const Tango::Attr_CheckVal &delta_val,
                                   Tango::AttrQuality &quality,
                                   std::bitset<Tango::Attribute::alarm_flags::numFlags> &alarm)
{
    if(::strcmp(encoded_val.encoded_format.in(), enc_seq[0].encoded_format.in()) != 0)
    {
        quality = Tango::ATTR_ALARM;
        alarm.set(Tango::Attribute::alarm_flags::rds);
        return true;
    }

    auto nb_written = encoded_val.encoded_data.length();
    auto nb_read = enc_seq[0].encoded_data.length();
    auto nb_data = (nb_written > nb_read) ? nb_read : nb_written;
    for(std::size_t i = 0; i < nb_data; ++i)
    {
        auto delta = encoded_val.encoded_data[i] - enc_seq[0].encoded_data[i];
        if(std::abs(delta) >= delta_val.uch)
        {
            quality = Tango::ATTR_ALARM;
            alarm.set(Tango::Attribute::alarm_flags::rds);
            return true;
        }
    }
    return false;
}
} // namespace details

//+-------------------------------------------------------------------------
//
// method :         WAttribute::WAttribute
//
// description :     constructor for the WAttribute class from the
//            attribute property vector, its type and the device
//            name
//
// argument : in :     - prop_list : The attribute property list
//            - type : The attrubute data type
//            - dev_name : The device name
//
//--------------------------------------------------------------------------

WAttribute::WAttribute(std::vector<AttrProperty> &prop_list, Attr &tmp_attr, const std::string &dev_name, long idx) :
    Attribute(prop_list, tmp_attr, dev_name, idx),

    w_ext(new WAttributeExt)

{
    //
    // Init some data
    //

    short_val = old_short_val = 0;
    long_val = old_long_val = 0;
    double_val = old_double_val = 0.0;
    float_val = old_float_val = 0.0;
    boolean_val = old_boolean_val = true;
    ushort_val = old_ushort_val = 0;
    uchar_val = old_uchar_val = 0;
    long64_val = old_long64_val = 0;
    ulong_val = old_ulong_val = 0;
    ulong64_val = old_ulong64_val = 0;
    dev_state_val = old_dev_state_val = Tango::UNKNOWN;
    str_val = CORBA::string_dup("Not initialised");
    old_str_val = CORBA::string_dup("Not initialised");
    encoded_val.encoded_data.length(0);
    encoded_val.encoded_format = CORBA::string_dup("Not initialised");
    old_encoded_val.encoded_data.length(0);
    old_encoded_val.encoded_format = CORBA::string_dup("Not initialised");

    short_array_val.length(1);
    short_array_val[0] = 0;
    long_array_val.length(1);
    long_array_val[0] = 0;
    double_array_val.length(1);
    double_array_val[0] = 0.0;
    str_array_val.length(1);
    str_array_val[0] = CORBA::string_dup("Not initialised");
    float_array_val.length(1);
    float_array_val[0] = 0.0;
    boolean_array_val.length(1);
    boolean_array_val[0] = true;
    ushort_array_val.length(1);
    ushort_array_val[0] = 0;
    uchar_array_val.length(1);
    uchar_array_val[0] = 0;
    long64_array_val.length(1);
    long64_array_val[0] = 0;
    ulong_array_val.length(1);
    ulong_array_val[0] = 0;
    ulong64_array_val.length(1);
    ulong64_array_val[0] = 0;
    state_array_val.length(1);
    state_array_val[0] = Tango::UNKNOWN;

    short_ptr = &short_val;
    w_dim_x = 1;
    w_dim_y = 0;
    write_date.tv_sec = write_date.tv_usec = 0;

    //
    // Init memorized field and eventually get the memorized value
    //

    set_memorized(tmp_attr.get_memorized());
    set_memorized_init(tmp_attr.get_memorized_init());

    if(is_memorized())
    {
        try
        {
            mem_value = get_attr_value(prop_list, MemAttrPropName);
        }
        catch(Tango::DevFailed &)
        {
            mem_value = MemNotUsed;
        }
    }
}

//+-------------------------------------------------------------------------
//
// method :         WAttribute::~WAttribute
//
// description :     destructor for the WAttribute class
//
//--------------------------------------------------------------------------

WAttribute::~WAttribute()
{
    Tango::string_free(str_val);
    Tango::string_free(old_str_val);
    //    Tango::string_free(encoded_val.encoded_format);
    //    Tango::string_free(old_encoded_val.encoded_format);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        WAttribute::set_rvalue
//
// description :
//        This method is used when a Writable attribute is set to set the value in the Attribute class. This is
//        necessary for the read_attribute CORBA operation which takes its data from this internal Attribute
//        class data. It is used in the read_attributes code in the device class
//
//--------------------------------------------------------------------------------------------------------------------

void WAttribute::set_rvalue()
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        if(data_format == Tango::SCALAR)
        {
            set_value(&short_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevShort *>(short_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_LONG:
        if(data_format == Tango::SCALAR)
        {
            set_value(&long_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevLong *>(long_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_LONG64:
        if(data_format == Tango::SCALAR)
        {
            set_value(&long64_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevLong64 *>(long64_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_DOUBLE:
        if(data_format == Tango::SCALAR)
        {
            set_value(&double_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevDouble *>(double_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_STRING:
        if(data_format == Tango::SCALAR)
        {
            set_value(&str_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevString *>(str_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_FLOAT:
        if(data_format == Tango::SCALAR)
        {
            set_value(&float_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevFloat *>(float_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_BOOLEAN:
        if(data_format == Tango::SCALAR)
        {
            set_value(&boolean_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevBoolean *>(boolean_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_USHORT:
        if(data_format == Tango::SCALAR)
        {
            set_value(&ushort_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevUShort *>(ushort_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_UCHAR:
        if(data_format == Tango::SCALAR)
        {
            set_value(&uchar_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevUChar *>(uchar_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_ULONG:
        if(data_format == Tango::SCALAR)
        {
            set_value(&ulong_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevULong *>(ulong_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_ULONG64:
        if(data_format == Tango::SCALAR)
        {
            set_value(&ulong64_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevULong64 *>(ulong64_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_STATE:
        if(data_format == Tango::SCALAR)
        {
            set_value(&dev_state_val, 1, 0, false);
        }
        else
        {
            set_value(const_cast<DevState *>(state_array_val.get_buffer()), w_dim_x, w_dim_y, false);
        }
        break;

    case Tango::DEV_ENCODED:
        set_value(&encoded_val, 1L, 0, false);
        break;
    }
}

//+-------------------------------------------------------------------------
//
// method :         WAttribute::check_written_value
//
// description :     Check the value sent by the caller and copy incoming data
//                    for SCALAR attribute only
//
// in :            any : Reference to the CORBA Any object
//
//--------------------------------------------------------------------------

void WAttribute::check_written_value(const CORBA::Any &any, unsigned long x, unsigned long y)
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevShort>(),
                              get_write_value<Tango::DevShort>(),
                              get_write_value_ptr<const Tango::DevShort *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_LONG:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevLong>(),
                              get_write_value<Tango::DevLong>(),
                              get_write_value_ptr<const Tango::DevLong *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_LONG64:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevLong64>(),
                              get_write_value<Tango::DevLong64>(),
                              get_write_value_ptr<const Tango::DevLong64 *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_DOUBLE:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevDouble>(),
                              get_write_value<Tango::DevDouble>(),
                              get_write_value_ptr<const Tango::DevDouble *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_STRING:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevString>(),
                              get_write_value<Tango::DevString>(),
                              get_write_value_ptr<const Tango::ConstDevString *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_FLOAT:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevFloat>(),
                              get_write_value<Tango::DevFloat>(),
                              get_write_value_ptr<const Tango::DevFloat *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_USHORT:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevUShort>(),
                              get_write_value<Tango::DevUShort>(),
                              get_write_value_ptr<const Tango::DevUShort *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_UCHAR:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevUChar>(),
                              get_write_value<Tango::DevUChar>(),
                              get_write_value_ptr<const Tango::DevUChar *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_ULONG:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevULong>(),
                              get_write_value<Tango::DevULong>(),
                              get_write_value_ptr<const Tango::DevULong *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_ULONG64:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevULong64>(),
                              get_write_value<Tango::DevULong64>(),
                              get_write_value_ptr<const Tango::DevULong64 *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_STATE:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevState>(),
                              get_write_value<Tango::DevState>(),
                              get_write_value_ptr<const Tango::DevState *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_BOOLEAN:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevBoolean>(),
                              get_write_value<Tango::DevBoolean>(),
                              get_write_value_ptr<const Tango::DevBoolean *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_ENCODED:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevEncoded>(),
                              get_write_value<Tango::DevEncoded>(),
                              get_write_value_ptr<const Tango::DevEncoded *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }
}

void WAttribute::check_written_value(const Tango::AttrValUnion &any, unsigned long x, unsigned long y)
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevShort>(),
                              get_write_value<Tango::DevShort>(),
                              get_write_value_ptr<const Tango::DevShort *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_LONG:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevLong>(),
                              get_write_value<Tango::DevLong>(),
                              get_write_value_ptr<const Tango::DevLong *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_LONG64:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevLong64>(),
                              get_write_value<Tango::DevLong64>(),
                              get_write_value_ptr<const Tango::DevLong64 *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_DOUBLE:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevDouble>(),
                              get_write_value<Tango::DevDouble>(),
                              get_write_value_ptr<const Tango::DevDouble *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_STRING:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevString>(),
                              get_write_value<Tango::DevString>(),
                              get_write_value_ptr<const Tango::ConstDevString *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_FLOAT:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevFloat>(),
                              get_write_value<Tango::DevFloat>(),
                              get_write_value_ptr<const Tango::DevFloat *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_USHORT:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevUShort>(),
                              get_write_value<Tango::DevUShort>(),
                              get_write_value_ptr<const Tango::DevUShort *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_UCHAR:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevUChar>(),
                              get_write_value<Tango::DevUChar>(),
                              get_write_value_ptr<const Tango::DevUChar *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_ULONG:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevULong>(),
                              get_write_value<Tango::DevULong>(),
                              get_write_value_ptr<const Tango::DevULong *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_ULONG64:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevULong64>(),
                              get_write_value<Tango::DevULong64>(),
                              get_write_value_ptr<const Tango::DevULong64 *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_STATE:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevState>(),
                              get_write_value<Tango::DevState>(),
                              get_write_value_ptr<const Tango::DevState *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_BOOLEAN:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevBoolean>(),
                              get_write_value<Tango::DevBoolean>(),
                              get_write_value_ptr<const Tango::DevBoolean *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    case Tango::DEV_ENCODED:
        _update_written_value(*this,
                              min_value,
                              max_value,
                              w_dim_x,
                              w_dim_y,
                              get_old_value<Tango::DevEncoded>(),
                              get_write_value<Tango::DevEncoded>(),
                              get_write_value_ptr<const Tango::DevEncoded *>(),
                              d_name,
                              enum_labels,
                              check_min_value,
                              check_max_value,
                              any,
                              x,
                              y);
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }
}

//+-------------------------------------------------------------------------
//
// method :         WAttribute::get_write_value_length
//
// description :     Returm to the caller the length of the new value to
//            be written into the attribute
//
//--------------------------------------------------------------------------

long WAttribute::get_write_value_length()
{
    switch(data_format)
    {
    case Tango::SCALAR:
        return 1;
    case Tango::SPECTRUM:
        return w_dim_x;
    case Tango::IMAGE:
        return w_dim_x * w_dim_y;
    default:
        return 0;
    }
}

template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void WAttribute::set_write_value(T *val, size_t x, size_t y)
{
    size_t nb_data;

    if(y == 0)
    {
        nb_data = x;
    }
    else
    {
        nb_data = x * y;
    }

    typename Tango::tango_type_traits<T>::ArrayType tmp_seq(
        static_cast<CORBA::ULong>(nb_data), static_cast<CORBA::ULong>(nb_data), val, false);

    CORBA::Any tmp_any;
    tmp_any <<= tmp_seq;
    check_written_value(tmp_any, x, y);
    copy_data(tmp_any);
    set_user_set_write_value(true);
}

template void WAttribute::set_write_value(Tango::DevBoolean *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevState *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevString *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevUChar *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevFloat *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevDouble *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevShort *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevUShort *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevLong *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevULong *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevLong64 *val, size_t x, size_t y);
template void WAttribute::set_write_value(Tango::DevULong64 *val, size_t x, size_t y);

template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void WAttribute::set_write_value(T val)
{
    set_write_value(&val, 1, 0);
}

template void WAttribute::set_write_value(Tango::DevBoolean val);
template void WAttribute::set_write_value(Tango::DevState val);
template void WAttribute::set_write_value(Tango::DevUChar val);
template void WAttribute::set_write_value(Tango::DevFloat val);
template void WAttribute::set_write_value(Tango::DevDouble val);
template void WAttribute::set_write_value(Tango::DevShort val);
template void WAttribute::set_write_value(Tango::DevUShort val);
template void WAttribute::set_write_value(Tango::DevLong val);
template void WAttribute::set_write_value(Tango::DevULong val);
template void WAttribute::set_write_value(Tango::DevLong64 val);
template void WAttribute::set_write_value(Tango::DevULong64 val);

template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void WAttribute::set_write_value(std::vector<T> &val, size_t x, size_t y)
{
    if constexpr(std::is_same_v<T, bool> || std::is_same_v<T, std::string>)
    {
        typename Tango::tango_type_traits<T>::ArrayType tmp_seq;
        tmp_seq << val;

        CORBA::Any tmp_any;
        tmp_any <<= tmp_seq;
        check_written_value(tmp_any, x, y);
        copy_data(tmp_any);
        set_user_set_write_value(true);
    }
    else
    {
        set_write_value(val.data(), x, y);
    }
}

template void WAttribute::set_write_value(std::vector<Tango::DevBoolean> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevState> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevString> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevUChar> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevFloat> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevDouble> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevShort> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevUShort> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevLong> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevULong> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevLong64> &val, size_t x, size_t y);
template void WAttribute::set_write_value(std::vector<Tango::DevULong64> &val, size_t x, size_t y);

void WAttribute::set_write_value(std::string &val)
{
    std::vector<std::string> seq;
    seq.push_back(val);
    set_write_value(seq, 1, 0);
}

void WAttribute::set_write_value(std::vector<std::string> &val, size_t x, size_t y)
{
    Tango::DevVarStringArray tmp_seq;
    tmp_seq << val;
    CORBA::Any tmp_any;
    tmp_any <<= tmp_seq;
    check_written_value(tmp_any, x, y);
    copy_data(tmp_any);
    set_user_set_write_value(true);
}

void WAttribute::set_write_value(Tango::DevString val)
{
    Tango::DevVarStringArray tmp_seq(1);
    tmp_seq.length(1);
    tmp_seq[0] = Tango::string_dup(val);
    CORBA::Any tmp_any;
    tmp_any <<= tmp_seq;
    check_written_value(tmp_any, 1, 0);
    copy_data(tmp_any);
    set_user_set_write_value(true);
}

void WAttribute::set_write_value(Tango::DevEncoded *, TANGO_UNUSED(long x), TANGO_UNUSED(long y))
{
    //
    // Dummy method just to make compiler happy when using fill_attr_polling_buffer for DevEncoded
    // attribute
    // Should never be called
    //

    TANGO_THROW_EXCEPTION(API_NotSupportedFeature, "This is a not supported call in case of DevEncoded attribute");
}

//+-------------------------------------------------------------------------
//
// method :         WAttribute::rollback
//
// description :     Reset the internal data to its value before the
//            set_write_value method was applied (Useful in case of
//            error in the set_write_value method)
//
//--------------------------------------------------------------------------

void WAttribute::rollback()
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        short_val = old_short_val;
        break;

    case Tango::DEV_LONG:
        long_val = old_long_val;
        break;

    case Tango::DEV_LONG64:
        long64_val = old_long64_val;
        break;

    case Tango::DEV_DOUBLE:
        double_val = old_double_val;
        break;

    case Tango::DEV_STRING:
        Tango::string_free(str_val);
        str_val = CORBA::string_dup(old_str_val);
        break;

    case Tango::DEV_FLOAT:
        float_val = old_float_val;
        break;

    case Tango::DEV_BOOLEAN:
        boolean_val = old_boolean_val;
        break;

    case Tango::DEV_USHORT:
        ushort_val = old_ushort_val;
        break;

    case Tango::DEV_UCHAR:
        Tango::string_free(str_val);
        break;

    case Tango::DEV_ULONG:
        ulong_val = old_ulong_val;
        break;

    case Tango::DEV_ULONG64:
        ulong64_val = old_ulong64_val;
        break;

    case Tango::DEV_STATE:
        dev_state_val = old_dev_state_val;
        break;

    case Tango::DEV_ENCODED:
        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, "This is a not supported call in case of DevEncoded attribute");
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }
}

void WAttribute::copy_data(const CORBA::Any &any)
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        _copy_data(get_last_written_value<Tango::DevVarShortArray>(), any);
        break;

    case Tango::DEV_LONG:
        _copy_data(get_last_written_value<Tango::DevVarLongArray>(), any);
        break;

    case Tango::DEV_LONG64:
        _copy_data(get_last_written_value<Tango::DevVarLong64Array>(), any);
        break;

    case Tango::DEV_DOUBLE:
        _copy_data(get_last_written_value<Tango::DevVarDoubleArray>(), any);
        break;

    case Tango::DEV_STRING:
        _copy_data(get_last_written_value<Tango::DevVarStringArray>(), any);
        break;

    case Tango::DEV_FLOAT:
        _copy_data(get_last_written_value<Tango::DevVarFloatArray>(), any);
        break;

    case Tango::DEV_BOOLEAN:
        _copy_data(get_last_written_value<Tango::DevVarBooleanArray>(), any);
        break;

    case Tango::DEV_USHORT:
        _copy_data(get_last_written_value<Tango::DevVarUShortArray>(), any);
        break;

    case Tango::DEV_UCHAR:
        _copy_data(get_last_written_value<Tango::DevVarCharArray>(), any);
        break;

    case Tango::DEV_ULONG:
        _copy_data(get_last_written_value<Tango::DevVarULongArray>(), any);
        break;

    case Tango::DEV_ULONG64:
        _copy_data(get_last_written_value<Tango::DevVarULong64Array>(), any);
        break;

    case Tango::DEV_STATE:
        _copy_data(get_last_written_value<Tango::DevVarStateArray>(), any);
        break;
    case Tango::DEV_ENCODED:
        // do nothing
        break;
    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }
}

void WAttribute::copy_data(const Tango::AttrValUnion &any)
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        _copy_data(get_last_written_value<Tango::DevVarShortArray>(), any);
        break;

    case Tango::DEV_LONG:
        _copy_data(get_last_written_value<Tango::DevVarLongArray>(), any);
        break;

    case Tango::DEV_LONG64:
        _copy_data(get_last_written_value<Tango::DevVarLong64Array>(), any);
        break;

    case Tango::DEV_DOUBLE:
        _copy_data(get_last_written_value<Tango::DevVarDoubleArray>(), any);
        break;

    case Tango::DEV_STRING:
        _copy_data(get_last_written_value<Tango::DevVarStringArray>(), any);
        break;

    case Tango::DEV_FLOAT:
        _copy_data(get_last_written_value<Tango::DevVarFloatArray>(), any);
        break;

    case Tango::DEV_BOOLEAN:
        _copy_data(get_last_written_value<Tango::DevVarBooleanArray>(), any);
        break;

    case Tango::DEV_USHORT:
        _copy_data(get_last_written_value<Tango::DevVarUShortArray>(), any);
        break;

    case Tango::DEV_UCHAR:
        _copy_data(get_last_written_value<Tango::DevVarCharArray>(), any);
        break;

    case Tango::DEV_ULONG:
        _copy_data(get_last_written_value<Tango::DevVarULongArray>(), any);
        break;

    case Tango::DEV_ULONG64:
        _copy_data(get_last_written_value<Tango::DevVarULong64Array>(), any);
        break;

    case Tango::DEV_STATE:
        _copy_data(get_last_written_value<Tango::DevVarStateArray>(), any);
        break;
    case Tango::DEV_ENCODED:
        // do nothing
        break;
    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }
}

//+-------------------------------------------------------------------------
//
// method :         WAttribute::set_written_date
//
// description :     Memorized when the attribute is written
//
//--------------------------------------------------------------------------

void WAttribute::set_written_date()
{
    write_date = make_timeval(std::chrono::system_clock::now());
}

//+-------------------------------------------------------------------------
//
// method :         WAttribute::check_rds_alarm
//
// description :     Check if the attribute is in read different from set
//            alarm.
//
// This method returns true if the attribute has a read too different than the
// the last set value. Otherwise, returns false.
//
//--------------------------------------------------------------------------

bool WAttribute::check_rds_alarm()
{
    bool ret = false;

    //
    // Return immediately if the attribute has never been written
    //

    if(write_date.tv_sec == 0)
    {
        return false;
    }

    //
    // First, check if it is necessary to check attribute value
    // Give some time to the device to change its output
    //

    auto time_diff = std::chrono::system_clock::now() - make_system_time(write_date);

    if(time_diff >= std::chrono::milliseconds(delta_t))
    {
        //
        // Now check attribute value with again a switch on attribute data type
        //

        switch(data_type)
        {
        case Tango::DEV_SHORT:
            ret = details::check_rds_out_of_range<Tango::DevShort>(get_last_written_value<Tango::DevVarShortArray>(),
                                                                   *get_value_storage<Tango::DevVarShortArray>(),
                                                                   data_format,
                                                                   delta_val,
                                                                   quality,
                                                                   alarm);
            break;

        case Tango::DEV_LONG:
            ret = details::check_rds_out_of_range<Tango::DevLong>(get_last_written_value<Tango::DevVarLongArray>(),
                                                                  *get_value_storage<Tango::DevVarLongArray>(),
                                                                  data_format,
                                                                  delta_val,
                                                                  quality,
                                                                  alarm);
            break;

        case Tango::DEV_LONG64:
            ret = details::check_rds_out_of_range<Tango::DevLong64>(get_last_written_value<Tango::DevVarLong64Array>(),
                                                                    *get_value_storage<Tango::DevVarLong64Array>(),
                                                                    data_format,
                                                                    delta_val,
                                                                    quality,
                                                                    alarm);
            break;

        case Tango::DEV_DOUBLE:
            ret = details::check_rds_out_of_range<Tango::DevDouble>(get_last_written_value<Tango::DevVarDoubleArray>(),
                                                                    *get_value_storage<Tango::DevVarDoubleArray>(),
                                                                    data_format,
                                                                    delta_val,
                                                                    quality,
                                                                    alarm);
            break;

        case Tango::DEV_FLOAT:
            ret = details::check_rds_out_of_range<Tango::DevFloat>(get_last_written_value<Tango::DevVarFloatArray>(),
                                                                   *get_value_storage<Tango::DevVarFloatArray>(),
                                                                   data_format,
                                                                   delta_val,
                                                                   quality,
                                                                   alarm);
            break;

        case Tango::DEV_USHORT:
            ret = details::check_rds_out_of_range<Tango::DevUShort>(get_last_written_value<Tango::DevVarUShortArray>(),
                                                                    *get_value_storage<Tango::DevVarUShortArray>(),
                                                                    data_format,
                                                                    delta_val,
                                                                    quality,
                                                                    alarm);
            break;

        case Tango::DEV_UCHAR:
            ret = details::check_rds_out_of_range<Tango::DevUChar>(get_last_written_value<Tango::DevVarUCharArray>(),
                                                                   *get_value_storage<Tango::DevVarUCharArray>(),
                                                                   data_format,
                                                                   delta_val,
                                                                   quality,
                                                                   alarm);
            break;

        case Tango::DEV_ULONG:
            ret = details::check_rds_out_of_range<Tango::DevULong>(get_last_written_value<Tango::DevVarULongArray>(),
                                                                   *get_value_storage<Tango::DevVarULongArray>(),
                                                                   data_format,
                                                                   delta_val,
                                                                   quality,
                                                                   alarm);
            break;

        case Tango::DEV_ULONG64:
            ret =
                details::check_rds_out_of_range<Tango::DevULong64>(get_last_written_value<Tango::DevVarULong64Array>(),
                                                                   *get_value_storage<Tango::DevVarULong64Array>(),
                                                                   data_format,
                                                                   delta_val,
                                                                   quality,
                                                                   alarm);
            break;

        case Tango::DEV_ENCODED:
            ret = details::check_rds_out_of_range(
                encoded_val, *get_value_storage<Tango::DevVarEncodedArray>(), delta_val, quality, alarm);
            break;
        }
    }

    return ret;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        set_min_value()
//
// description :
//        Sets minimum value attribute property. Throws exception in case the data type of provided
//        property does not match the attribute data type
//
// args :
//        in :
//             - new_min_value : The minimum value property to be set
//
//-----------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T> || std::is_same_v<T, std::string>> *>
void WAttribute::set_min_value(const T &new_min_value)
{
    //
    // Check type validity
    //
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE))
    {
        Tango::detail::throw_err_data_type("min_value", d_name, name, "set_min_value()");
    }

    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    //    Check coherence with max_value
    //

    if(is_max_value())
    {
        T max_value_tmp;
        memcpy((void *) &max_value_tmp, (const void *) &max_value, sizeof(T));
        if(new_min_value >= max_value_tmp)
        {
            Tango::detail::throw_incoherent_val_err("min_value", "max_value", d_name, name, "set_min_value()");
        }
    }

    //
    // Store new min value as a string
    //

    TangoSys_MemStream str;
    str.precision(Tango::TANGO_FLOAT_PRECISION);
    if(Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR)
    {
        str << (short) new_min_value; // to represent the numeric value
    }
    else
    {
        str << new_min_value;
    }
    std::string min_value_tmp_str = str.str();

    //
    // Get the monitor protecting device att config
    // If the server is in its starting phase, give a nullptr to the AutoLock object
    //

    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        mon_ptr = &(get_att_device()->get_att_conf_monitor());
    }
    Tango::AutoTangoMonitor sync1(mon_ptr);

    //
    // Store the new value locally
    //

    Tango::Attr_CheckVal old_min_value;
    memcpy((void *) &old_min_value, (void *) &min_value, sizeof(T));
    memcpy((void *) &min_value, (void *) &new_min_value, sizeof(T));

    //
    // Then, update database
    //

    const auto &def_user_prop =
        get_att_device_class(d_name)->get_class_attr()->get_attr(name).get_user_default_properties();
    size_t nb_user = def_user_prop.size();

    std::string usr_def_val;
    bool user_defaults = false;
    if(nb_user != 0)
    {
        size_t i;
        for(i = 0; i < nb_user; i++)
        {
            if(def_user_prop[i].get_name() == "min_value")
            {
                break;
            }
        }
        if(i != nb_user) // user defaults defined
        {
            user_defaults = true;
            usr_def_val = def_user_prop[i].get_value();
        }
    }

    if(Tango::Util::instance()->use_db())
    {
        if(user_defaults && min_value_tmp_str == usr_def_val)
        {
            Tango::DbDatum attr_dd(name), prop_dd("min_value");
            Tango::DbData db_data;
            db_data.push_back(attr_dd);
            db_data.push_back(prop_dd);

            bool retry = true;
            while(retry)
            {
                try
                {
                    tg->get_database()->delete_device_attribute_property(d_name, db_data);
                    retry = false;
                }
                catch(CORBA::COMM_FAILURE &)
                {
                    tg->get_database()->reconnect(true);
                }
            }
        }
        else
        {
            try
            {
                Tango::detail::upd_att_prop_db(min_value, "min_value", d_name, name, get_data_type());
            }
            catch(Tango::DevFailed &)
            {
                memcpy((void *) &min_value, (void *) &old_min_value, sizeof(T));
                throw;
            }
        }
    }

    //
    // Set the min_value flag
    //

    check_min_value = true;

    //
    // Store new value as a string
    //

    min_value_str = min_value_tmp_str;

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }

    //
    // Delete device startup exception related to min_value if there is any
    //

    Tango::detail::delete_startup_exception(*this, "min_value", d_name, check_startup_exceptions, startup_exceptions);
}

template <>
void WAttribute::set_min_value(const std::string &new_min_value_str)
{
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
       (data_type == Tango::DEV_ENUM))
    {
        Tango::detail::throw_err_data_type("min_value", d_name, name, "set_min_value()");
    }

    std::string min_value_str_tmp = new_min_value_str;

    std::string usr_def_val;
    std::string class_def_val;
    bool user_defaults = false;
    bool class_defaults = false;

    const auto &def_user_prop =
        get_att_device_class(d_name)->get_class_attr()->get_attr(name).get_user_default_properties();
    const auto &def_class_prop = get_att_device_class(d_name)->get_class_attr()->get_attr(name).get_class_properties();
    user_defaults = Tango::detail::prop_in_list("min_value", usr_def_val, def_user_prop);

    class_defaults = Tango::detail::prop_in_list("min_value", class_def_val, def_class_prop);

    bool set_value = true;

    if(class_defaults)
    {
        if(TG_strcasecmp(new_min_value_str.c_str(), Tango::AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("min_value", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, check_min_value, min_value_str);
        }
        else if((TG_strcasecmp(new_min_value_str.c_str(), Tango::NotANumber) == 0) ||
                (TG_strcasecmp(new_min_value_str.c_str(), class_def_val.c_str()) == 0))
        {
            min_value_str_tmp = class_def_val;
        }
        else if(strlen(new_min_value_str.c_str()) == 0)
        {
            if(user_defaults)
            {
                min_value_str_tmp = usr_def_val;
            }
            else
            {
                set_value = false;

                Tango::detail::avns_in_db("min_value", name, d_name);
                Tango::detail::avns_in_att(*this, d_name, check_min_value, min_value_str);
            }
        }
    }
    else if(user_defaults)
    {
        if(TG_strcasecmp(new_min_value_str.c_str(), Tango::AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("min_value", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, check_min_value, min_value_str);
        }
        else if((TG_strcasecmp(new_min_value_str.c_str(), Tango::NotANumber) == 0) ||
                (TG_strcasecmp(new_min_value_str.c_str(), usr_def_val.c_str()) == 0) ||
                (strlen(new_min_value_str.c_str()) == 0))
        {
            min_value_str_tmp = usr_def_val;
        }
    }
    else
    {
        if((TG_strcasecmp(new_min_value_str.c_str(), Tango::AlrmValueNotSpec) == 0) ||
           (TG_strcasecmp(new_min_value_str.c_str(), Tango::NotANumber) == 0) ||
           (strlen(new_min_value_str.c_str()) == 0))
        {
            set_value = false;

            Tango::detail::avns_in_db("min_value", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, check_min_value, min_value_str);
        }
    }

    if(set_value)
    {
        if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE))
        {
            double db;
            float fl;

            TangoSys_MemStream str;
            str.precision(Tango::TANGO_FLOAT_PRECISION);
            str << min_value_str_tmp;
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                set_min_value((Tango::DevShort) db);
                break;

            case Tango::DEV_LONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                set_min_value((Tango::DevLong) db);
                break;

            case Tango::DEV_LONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                set_min_value((Tango::DevLong64) db);
                break;

            case Tango::DEV_DOUBLE:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                set_min_value(db);
                break;

            case Tango::DEV_FLOAT:
                if(!(str >> fl && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                set_min_value(fl);
                break;

            case Tango::DEV_USHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                (db < 0.0) ? set_min_value((Tango::DevUShort)(-db)) : set_min_value((Tango::DevUShort) db);
                break;

            case Tango::DEV_UCHAR:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                (db < 0.0) ? set_min_value((Tango::DevUChar)(-db)) : set_min_value((Tango::DevUChar) db);
                break;

            case Tango::DEV_ULONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                (db < 0.0) ? set_min_value((Tango::DevULong)(-db)) : set_min_value((Tango::DevULong) db);
                break;

            case Tango::DEV_ULONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                (db < 0.0) ? set_min_value((Tango::DevULong64)(-db)) : set_min_value((Tango::DevULong64) db);
                break;

            case Tango::DEV_ENCODED:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_value", d_name, name, "set_min_value()");
                }
                (db < 0.0) ? set_min_value((Tango::DevUChar)(-db)) : set_min_value((Tango::DevUChar) db);
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("min_value", d_name, name, "set_min_value()");
        }
    }
}

template <>
void WAttribute::set_min_value(const Tango::DevEncoded &)
{
    std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
    TANGO_THROW_EXCEPTION(Tango::API_MethodArgument, err_msg.c_str());
}

template void WAttribute::set_min_value(const Tango::DevBoolean &value);
template void WAttribute::set_min_value(const Tango::DevUChar &value);
template void WAttribute::set_min_value(const Tango::DevShort &value);
template void WAttribute::set_min_value(const Tango::DevUShort &value);
template void WAttribute::set_min_value(const Tango::DevLong &value);
template void WAttribute::set_min_value(const Tango::DevULong &value);
template void WAttribute::set_min_value(const Tango::DevLong64 &value);
template void WAttribute::set_min_value(const Tango::DevULong64 &value);
template void WAttribute::set_min_value(const Tango::DevFloat &value);
template void WAttribute::set_min_value(const Tango::DevDouble &value);
template void WAttribute::set_min_value(const Tango::DevState &value);

void WAttribute::set_min_value(char *new_min_value_str)
{
    set_min_value(std::string(new_min_value_str));
}

void WAttribute::set_min_value(const char *new_min_value_str)
{
    set_min_value(std::string(new_min_value_str));
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        get_min_value()
//
// description :
//        Gets attribute's minimum value and assigns it to the variable provided as a parameter
//        Throws exception in case the data type of provided parameter does not match the attribute data type
//        or if minimum value is not defined
//
// args :
//        in :
//             - min_val : The variable to be assigned the attribute's minimum value
//
//------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void WAttribute::get_min_value(T &min_val)
{
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << get_name()
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    if(!is_min_value())
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrNotAllowed, "Minimum value not defined for this attribute");
    }

    memcpy((void *) &min_val, (void *) &min_value, sizeof(T));
}

template void WAttribute::get_min_value(Tango::DevBoolean &value);
template void WAttribute::get_min_value(Tango::DevUChar &value);
template void WAttribute::get_min_value(Tango::DevShort &value);
template void WAttribute::get_min_value(Tango::DevUShort &value);
template void WAttribute::get_min_value(Tango::DevLong &value);
template void WAttribute::get_min_value(Tango::DevULong &value);
template void WAttribute::get_min_value(Tango::DevLong64 &value);
template void WAttribute::get_min_value(Tango::DevULong64 &value);
template void WAttribute::get_min_value(Tango::DevFloat &value);
template void WAttribute::get_min_value(Tango::DevDouble &value);
template void WAttribute::get_min_value(Tango::DevState &value);
template void WAttribute::get_min_value(Tango::DevString &value);
template void WAttribute::get_min_value(Tango::DevEncoded &value);

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        set_max_value()
//
// description :
//        Sets maximum value attribute property
//        Throws exception in case the data type of provided property does not match the attribute data type
//
// args :
//        in :
//            - new_max_value : The maximum value property to be set
//
//-------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T> || std::is_same_v<T, std::string>> *>
void WAttribute::set_max_value(const T &new_max_value)
{
    //
    // Check type validity
    //
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE))
    {
        Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
    }

    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    //    Check coherence with max_value
    //

    if(is_min_value())
    {
        T min_value_tmp;
        memcpy((void *) &min_value_tmp, (const void *) &min_value, sizeof(T));
        if(new_max_value <= min_value_tmp)
        {
            Tango::detail::throw_incoherent_val_err("min_value", "max_value", d_name, name, "set_max_value()");
        }
    }

    //
    // Store new max value as a string
    //

    TangoSys_MemStream str;
    str.precision(Tango::TANGO_FLOAT_PRECISION);
    if(Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR)
    {
        str << (short) new_max_value; // to represent the numeric value
    }
    else
    {
        str << new_max_value;
    }
    std::string max_value_tmp_str = str.str();

    //
    // Get the monitor protecting device att config
    // If the server is in its starting phase, give a nullptr to the AutoLock object
    //

    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        mon_ptr = &(get_att_device()->get_att_conf_monitor());
    }
    Tango::AutoTangoMonitor sync1(mon_ptr);

    //
    // Store the new value locally
    //

    Tango::Attr_CheckVal old_max_value;
    memcpy((void *) &old_max_value, (void *) &max_value, sizeof(T));
    memcpy((void *) &max_value, (void *) &new_max_value, sizeof(T));

    //
    // Then, update database
    //

    const auto &def_user_prop =
        get_att_device_class(d_name)->get_class_attr()->get_attr(name).get_user_default_properties();
    size_t nb_user = def_user_prop.size();

    std::string usr_def_val;
    bool user_defaults = false;
    if(nb_user != 0)
    {
        size_t i;
        for(i = 0; i < nb_user; i++)
        {
            if(def_user_prop[i].get_name() == "max_value")
            {
                break;
            }
        }
        if(i != nb_user) // user defaults defined
        {
            user_defaults = true;
            usr_def_val = def_user_prop[i].get_value();
        }
    }

    if(Tango::Util::instance()->use_db())
    {
        if(user_defaults && max_value_tmp_str == usr_def_val)
        {
            Tango::DbDatum attr_dd(name), prop_dd("max_value");
            Tango::DbData db_data;
            db_data.push_back(attr_dd);
            db_data.push_back(prop_dd);

            bool retry = true;
            while(retry)
            {
                try
                {
                    tg->get_database()->delete_device_attribute_property(d_name, db_data);
                    retry = false;
                }
                catch(CORBA::COMM_FAILURE &)
                {
                    tg->get_database()->reconnect(true);
                }
            }
        }
        else
        {
            try
            {
                Tango::detail::upd_att_prop_db(max_value, "max_value", d_name, name, get_data_type());
            }
            catch(Tango::DevFailed &)
            {
                memcpy((void *) &max_value, (void *) &old_max_value, sizeof(T));
                throw;
            }
        }
    }

    //
    // Set the max_value flag
    //

    check_max_value = true;

    //
    // Store new value as a string
    //

    max_value_str = max_value_tmp_str;

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }

    //
    // Delete device startup exception related to max_value if there is any
    //

    Tango::detail::delete_startup_exception(*this, "max_value", d_name, check_startup_exceptions, startup_exceptions);
}

template <>
void WAttribute::set_max_value(const std::string &new_max_value_str)
{
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE))
    {
        Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
    }

    std::string max_value_str_tmp = new_max_value_str;

    std::string usr_def_val;
    std::string class_def_val;
    bool user_defaults = false;
    bool class_defaults = false;

    const auto &def_user_prop =
        get_att_device_class(d_name)->get_class_attr()->get_attr(name).get_user_default_properties();
    const auto &def_class_prop = get_att_device_class(d_name)->get_class_attr()->get_attr(name).get_class_properties();
    user_defaults = Tango::detail::prop_in_list("max_value", usr_def_val, def_user_prop);

    class_defaults = Tango::detail::prop_in_list("max_value", class_def_val, def_class_prop);

    bool set_value = true;

    if(class_defaults)
    {
        if(TG_strcasecmp(new_max_value_str.c_str(), Tango::AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("max_value", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, check_max_value, max_value_str);
        }
        else if((TG_strcasecmp(new_max_value_str.c_str(), Tango::NotANumber) == 0) ||
                (TG_strcasecmp(new_max_value_str.c_str(), class_def_val.c_str()) == 0))
        {
            max_value_str_tmp = class_def_val;
        }
        else if(strlen(new_max_value_str.c_str()) == 0)
        {
            if(user_defaults)
            {
                max_value_str_tmp = usr_def_val;
            }
            else
            {
                set_value = false;

                Tango::detail::avns_in_db("max_value", name, d_name);
                Tango::detail::avns_in_att(*this, d_name, check_max_value, max_value_str);
            }
        }
    }
    else if(user_defaults)
    {
        if(TG_strcasecmp(new_max_value_str.c_str(), Tango::AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("max_value", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, check_max_value, max_value_str);
        }
        else if((TG_strcasecmp(new_max_value_str.c_str(), Tango::NotANumber) == 0) ||
                (TG_strcasecmp(new_max_value_str.c_str(), usr_def_val.c_str()) == 0) ||
                (strlen(new_max_value_str.c_str()) == 0))
        {
            max_value_str_tmp = usr_def_val;
        }
    }
    else
    {
        if((TG_strcasecmp(new_max_value_str.c_str(), Tango::AlrmValueNotSpec) == 0) ||
           (TG_strcasecmp(new_max_value_str.c_str(), Tango::NotANumber) == 0) ||
           (strlen(new_max_value_str.c_str()) == 0))
        {
            set_value = false;

            Tango::detail::avns_in_db("max_value", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, check_max_value, max_value_str);
        }
    }

    if(set_value)
    {
        if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE) &&
           (data_type != Tango::DEV_ENUM))
        {
            double db;
            float fl;

            TangoSys_MemStream str;
            str.precision(Tango::TANGO_FLOAT_PRECISION);
            str << max_value_str_tmp;
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                set_max_value((Tango::DevShort) db);
                break;

            case Tango::DEV_LONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                set_max_value((Tango::DevLong) db);
                break;

            case Tango::DEV_LONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                set_max_value((Tango::DevLong64) db);
                break;

            case Tango::DEV_DOUBLE:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                set_max_value(db);
                break;

            case Tango::DEV_FLOAT:
                if(!(str >> fl && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                set_max_value(fl);
                break;

            case Tango::DEV_USHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                (db < 0.0) ? set_max_value((Tango::DevUShort)(-db)) : set_max_value((Tango::DevUShort) db);
                break;

            case Tango::DEV_UCHAR:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                (db < 0.0) ? set_max_value((Tango::DevUChar)(-db)) : set_max_value((Tango::DevUChar) db);
                break;

            case Tango::DEV_ULONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                (db < 0.0) ? set_max_value((Tango::DevULong)(-db)) : set_max_value((Tango::DevULong) db);
                break;

            case Tango::DEV_ULONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                (db < 0.0) ? set_max_value((Tango::DevULong64)(-db)) : set_max_value((Tango::DevULong64) db);
                break;

            case Tango::DEV_ENCODED:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
                }
                (db < 0.0) ? set_max_value((Tango::DevUChar)(-db)) : set_max_value((Tango::DevUChar) db);
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("max_value", d_name, name, "set_max_value()");
        }
    }
}

template <>
void WAttribute::set_max_value(const Tango::DevEncoded &)
{
    std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
    TANGO_THROW_EXCEPTION(Tango::API_MethodArgument, err_msg.c_str());
}

template void WAttribute::set_max_value(const Tango::DevBoolean &value);
template void WAttribute::set_max_value(const Tango::DevUChar &value);
template void WAttribute::set_max_value(const Tango::DevShort &value);
template void WAttribute::set_max_value(const Tango::DevUShort &value);
template void WAttribute::set_max_value(const Tango::DevLong &value);
template void WAttribute::set_max_value(const Tango::DevULong &value);
template void WAttribute::set_max_value(const Tango::DevLong64 &value);
template void WAttribute::set_max_value(const Tango::DevULong64 &value);
template void WAttribute::set_max_value(const Tango::DevFloat &value);
template void WAttribute::set_max_value(const Tango::DevDouble &value);
template void WAttribute::set_max_value(const Tango::DevState &value);

void WAttribute::set_max_value(char *new_max_value_str)
{
    set_max_value(std::string(new_max_value_str));
}

void WAttribute::set_max_value(const char *new_max_value_str)
{
    set_max_value(std::string(new_max_value_str));
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        get_max_value()
//
// description :
//        Gets attribute's maximum value and assigns it to the variable provided as a parameter
//        Throws exception in case the data type of provided parameter does not match the attribute data type
//        or if maximum value is not defined
//
// args :
//        in :
//             - max_val : The variable to be assigned the attribute's maximum value
//
//------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void WAttribute::get_max_value(T &max_val)
{
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << get_name()
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    if(!is_max_value())
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrNotAllowed, "Maximum value not defined for this attribute");
    }

    memcpy((void *) &max_val, (void *) &max_value, sizeof(T));
}

template void WAttribute::get_max_value(Tango::DevBoolean &value);
template void WAttribute::get_max_value(Tango::DevUChar &value);
template void WAttribute::get_max_value(Tango::DevShort &value);
template void WAttribute::get_max_value(Tango::DevUShort &value);
template void WAttribute::get_max_value(Tango::DevLong &value);
template void WAttribute::get_max_value(Tango::DevULong &value);
template void WAttribute::get_max_value(Tango::DevLong64 &value);
template void WAttribute::get_max_value(Tango::DevULong64 &value);
template void WAttribute::get_max_value(Tango::DevFloat &value);
template void WAttribute::get_max_value(Tango::DevDouble &value);
template void WAttribute::get_max_value(Tango::DevState &value);
template void WAttribute::get_max_value(Tango::DevString &value);
template void WAttribute::get_max_value(Tango::DevEncoded &value);

//+-------------------------------------------------------------------------
//
// method :         WAttribute::mem_value_below_above()
//
// description :     Check if the attribute last written value is below
//                  (or above) the new threshold sent by the requester
//
// Arg in :            check_type : Which check has to be done: Below or above
//
// This method returns true if the new threshold wanted by the user is not
// coherent with the memorized value. Otherwise, returns false.
//
//--------------------------------------------------------------------------

bool WAttribute::mem_value_below_above(MinMaxValueCheck check_type, std::string &ret_mem_value)
{
    bool ret = false;

    if(mem_value == MemNotUsed)
    {
        return false;
    }

    //
    // Check last written attribute value with the new threshold
    //

    long nb_written, i;
    std::stringstream ss;

    DevLong lg_val;
    DevShort sh_val;
    DevLong64 lg64_val;
    DevDouble db_val;
    DevFloat fl_val;
    DevUShort ush_val;
    DevUChar uch_val;
    DevULong ulg_val;
    DevULong64 ulg64_val;

    Tango::Util *tg = Tango::Util::instance();
    bool svr_starting = tg->is_svr_starting();

    switch(data_type)
    {
    case Tango::DEV_SHORT:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> sh_val;
            if(check_type == MIN)
            {
                if(sh_val < min_value.sh)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(sh_val > max_value.sh)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = short_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(short_array_val[i] < min_value.sh)
                    {
                        ss << short_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(short_array_val[i] > max_value.sh)
                    {
                        ss << short_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_LONG:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> lg_val;
            if(check_type == MIN)
            {
                if(lg_val < min_value.lg)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(lg_val > max_value.lg)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = long_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(long_array_val[i] < min_value.lg)
                    {
                        ss << long_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(long_array_val[i] > max_value.lg)
                    {
                        ss << long_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_LONG64:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> lg64_val;
            if(check_type == MIN)
            {
                if(lg64_val < min_value.lg64)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(lg64_val > max_value.lg64)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = long64_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(long64_array_val[i] < min_value.lg64)
                    {
                        ss << long64_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(long64_array_val[i] > max_value.lg64)
                    {
                        ss << long64_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_DOUBLE:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> db_val;
            if(check_type == MIN)
            {
                if(db_val < min_value.db)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(db_val > max_value.db)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = double_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(double_array_val[i] < min_value.db)
                    {
                        ss << double_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(double_array_val[i] > max_value.db)
                    {
                        ss << double_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_FLOAT:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> fl_val;
            if(check_type == MIN)
            {
                if(fl_val < min_value.fl)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(fl_val > max_value.fl)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = float_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(float_array_val[i] < min_value.fl)
                    {
                        ss << float_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(float_array_val[i] > max_value.fl)
                    {
                        ss << float_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_USHORT:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> ush_val;
            if(check_type == MIN)
            {
                if(ush_val < min_value.ush)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(ush_val > max_value.ush)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = ushort_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(ushort_array_val[i] < min_value.ush)
                    {
                        ss << ushort_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(ushort_array_val[i] > max_value.ush)
                    {
                        ss << ushort_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_UCHAR:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> uch_val;
            if(check_type == MIN)
            {
                if(uch_val < min_value.uch)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(uch_val > max_value.uch)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = uchar_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(uchar_array_val[i] < min_value.uch)
                    {
                        ss << uchar_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(uchar_array_val[i] > max_value.uch)
                    {
                        ss << uchar_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_ULONG:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> ulg_val;
            if(check_type == MIN)
            {
                if(ulg_val < min_value.ulg)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(ulg_val > max_value.ulg)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = ulong_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(ulong_array_val[i] < min_value.ulg)
                    {
                        ss << ulong_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(ulong_array_val[i] > max_value.ulg)
                    {
                        ss << ulong_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_ULONG64:
        if(svr_starting)
        {
            ss << mem_value;
            ss >> ulg64_val;
            if(check_type == MIN)
            {
                if(ulg64_val < min_value.ulg64)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
            else
            {
                if(ulg64_val > max_value.ulg64)
                {
                    ret_mem_value = mem_value;
                    ret = true;
                }
            }
        }
        else
        {
            nb_written = ulong64_array_val.length();
            for(i = 0; i < nb_written; i++)
            {
                if(check_type == MIN)
                {
                    if(ulong64_array_val[i] < min_value.ulg64)
                    {
                        ss << ulong64_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if(ulong64_array_val[i] > max_value.ulg64)
                    {
                        ss << ulong64_array_val[i];
                        ret_mem_value = ss.str();
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;

    case Tango::DEV_ENUM:
        [[fallthrough]];
    case Tango::DEV_STRING:
        [[fallthrough]];
    case Tango::DEV_BOOLEAN:
        [[fallthrough]];
    case Tango::DEV_STATE:
    {
        TangoSys_OMemStream msg;
        msg << "This is a not supported call in case of " << data_type_to_string(data_type) << " attribute";
        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, msg.str().c_str());
    }
    break;

    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }

    return ret;
}

} // namespace Tango
