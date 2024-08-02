//+===================================================================================================================
//
// file :               attsetval_templ.h
//
// description :        C++ source code for the Attribute class template methods when they are not specialized and
//                        related to attribute value setting
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//
//-===================================================================================================================

#ifndef _ATTRSETVAL_TPP
#define _ATTRSETVAL_TPP

#include <type_traits>

namespace Tango
{

/// @cond DO_NOT_DOCUMENT

/*
 * method: Attribute::set_value
 *
 * These methods store the attribute value inside the Attribute object, set current time as readout time
 * and calculate attribute quality factor (if warning/alarm levels are defined)
 *
 * The value will be later send to the client either as result of attribute readout of event value,
 * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
 *
 * After readout or in case of failure value will be destroyed.
 *
 * When release is true, we expect p_data to be allocated with operator new for SCALAR attributes and operator new[]
 * otherwise. For the DevString's, when release is true we expect value to have been allocated with Tango::string_dup or
 * equivalent.
 *
 * @param T *p_data: pointer to value
 * @param long x=1: the attribute x dimension
 * @param long y=0: the attribute y dimension
 * @param bool release: The release flag. If true, memory pointed to by p_data will be
 *           freed after being send to the client. Default value is false.
 * @exception DevFailed If the attribute data type is not coherent, enum labels are not defined, wrong size of data, or
 * wrong Enum value.
 *
 */

// General set_value realisation for almost all data types
template <class T, std::enable_if_t<!std::is_enum_v<T> || std::is_same_v<T, Tango::DevState>, T> *>
inline void Attribute::set_value(T *p_data, long x, long y, bool release)
{
    using ArrayType = typename tango_type_traits<T>::ArrayType;

    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::tango_type_traits<T>::type_value())
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::tango_type_traits<T>::type_value() << " for attribute " << name
          << ". Attribute data type is " << (CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Check that data size is less than the given max
    //

    if((x > max_x) || (y > max_y))
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
          << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    set_data_size();
    quality = Tango::ATTR_VALID;

    //
    // Throw exception if pointer is null and data_size != 0
    //

    if(data_size != 0)
    {
        CHECK_PTR(p_data, name);
    }

    // Get proper storage
    auto *tmp = get_value_storage<ArrayType>();

    // Save data to proper seq
    if((data_format == Tango::SCALAR) && (release))
    {
        T *tmp_ptr = new T[1];
        *tmp_ptr = *p_data;
        *tmp = new ArrayType(data_size, data_size, tmp_ptr, release);
        delete_data_if_needed(p_data, release);
    }
    else
    {
        *tmp = new ArrayType(data_size, data_size, p_data, release);
    }

    value_flag = true;

    //
    // Reset alarm flags
    //

    alarm.reset();

    //
    // Get time
    //

    set_time();
}

// Special set_value realisation for scalar C++11 scoped enum with short as underlying data type or old enum
template <class T, std::enable_if_t<std::is_enum_v<T> && !std::is_same_v<T, Tango::DevState>, T> *>
inline void Attribute::set_value(T *enum_ptr, long x, long y, bool release)
{
    //
    // Throw exception if attribute data type is not correct
    //

    if(data_type != Tango::DEV_ENUM)
    {
        delete_data_if_needed(enum_ptr, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::DEV_ENUM << " for attribute " << name
          << ". Attribute data type is " << (CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    bool short_enum = std::is_same<short, std::underlying_type_t<T>>::value;
    bool uns_int_enum = std::is_same<unsigned int, std::underlying_type_t<T>>::value;

    if(!short_enum && !uns_int_enum)
    {
        delete_data_if_needed(enum_ptr, release);

        std::stringstream o;
        o << "Invalid enumeration type. Supported types are C++11 scoped enum with short as underlying data type or "
             "old enum."
          << std::endl;

        TANGO_THROW_EXCEPTION(API_IncompatibleArgumentType, o.str());
    }

    //
    // Check if the input type is an enum and if it is from the valid type
    //

    if(std::is_enum<T>::value == false)
    {
        delete_data_if_needed(enum_ptr, release);
        TANGO_THROW_EXCEPTION(API_IncompatibleArgumentType, "The input argument data type is not an enumeration");
    }

    //
    // Check if enum labels are defined
    //

    if(enum_labels.size() == 0)
    {
        delete_data_if_needed(enum_ptr, release);

        std::stringstream o;
        o << "Attribute " << name << " data type is enum but no enum labels are defined!";

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Check enum type
    //

    DeviceImpl *dev = get_att_device();
    Tango::DeviceClass *dev_class = dev->get_device_class();
    Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
    Tango::Attr &att = mca->get_attr(name);

    if(!att.same_type(typeid(T)))
    {
        delete_data_if_needed(enum_ptr, release);

        std::stringstream o;
        o << "Invalid enumeration type. Requested enum type is " << att.get_enum_type();
        TANGO_THROW_EXCEPTION(API_IncompatibleArgumentType, o.str());
    }

    //
    // Check that data size is less than the given max
    //

    if((x > max_x) || (y > max_y))
    {
        delete_data_if_needed(enum_ptr, release);

        std::stringstream o;

        o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
          << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    set_data_size();
    quality = Tango::ATTR_VALID;

    //
    // Throw exception if pointer is null and data_size != 0
    //

    if(data_size != 0)
    {
        CHECK_PTR(enum_ptr, name);
    }

    if(data_size > enum_nb)
    {
        if(enum_nb != 0)
        {
            delete[] loc_enum_ptr;
        }
        loc_enum_ptr = new short[data_size];
        enum_nb = data_size;
    }

    short max_val = (short) enum_labels.size() - 1;
    for(std::uint32_t i = 0; i < data_size; i++)
    {
        loc_enum_ptr[i] = (short) enum_ptr[i];
        if(loc_enum_ptr[i] < 0 || loc_enum_ptr[i] > max_val)
        {
            delete_data_if_needed(enum_ptr, release);
            enum_nb = 0;

            std::stringstream o;
            o << "Wrong value for attribute " << name << ". Element " << i << " (value = " << loc_enum_ptr[i]
              << ") is negative or above the limit defined by the enum (" << max_val << ").";

            delete[] loc_enum_ptr;

            TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
        }
    }

    delete_data_if_needed(enum_ptr, release);

    if((data_format == Tango::SCALAR) && (release))
    {
        value.sh_seq = new Tango::DevVarShortArray(data_size, data_size, loc_enum_ptr, false);
    }
    else
    {
        value.sh_seq = new Tango::DevVarShortArray(data_size, data_size, loc_enum_ptr, release);
    }

    value_flag = true;

    //
    // Reset alarm flags
    //

    alarm.reset();

    //
    // Get time
    //

    set_time();
}

// Special set_value realisation for Tango::DevShort due to the fact, that Tango::DevEnum
// is effectively Tango::DevShort under hood and when one calls set_value for DevEnum -
// cpp gives him Tango::DevShort realisation
template <>
inline void Attribute::set_value(Tango::DevShort *p_data, long x, long y, bool release)
{
    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::DEV_SHORT && data_type != Tango::DEV_ENUM)
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::DEV_SHORT << " for attribute " << name
          << ". Attribute data type is " << (CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Check that data size is less than the given max
    //

    if((x > max_x) || (y > max_y))
    {
        delete_data_if_needed(p_data, release);

        std::stringstream ss;
        ss << "Data size for attribute " << name << " exceeds given limit";

        TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    set_data_size();
    quality = Tango::ATTR_VALID;

    //
    // Throw exception if pointer is null and data_size != 0
    //

    if(data_size != 0)
    {
        CHECK_PTR(p_data, name);
    }

    //
    // For DevEnum, check that the enum labels are defined. Also check the enum value
    //

    if(data_type == DEV_ENUM)
    {
        if(enum_labels.size() == 0)
        {
            delete_data_if_needed(p_data, release);

            std::stringstream ss;
            ss << "Attribute " << name << " data type is enum but no enum labels are defined!";

            TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
        }

        int max_val = enum_labels.size() - 1;
        for(std::uint32_t i = 0; i < data_size; i++)
        {
            if(p_data[i] < 0 || p_data[i] > max_val)
            {
                delete_data_if_needed(p_data, release);

                std::stringstream ss;
                ss << "Wrong value for attribute " << name;
                ss << ". Element " << i << " (value = " << p_data[i]
                   << ") is negative or above the limit defined by the enum (" << max_val << ").";

                TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
            }
        }
    }

    if((data_format == Tango::SCALAR) && (release))
    {
        Tango::DevShort *tmp_ptr = new Tango::DevShort[1];
        *tmp_ptr = *p_data;
        value.sh_seq = new Tango::DevVarShortArray(data_size, data_size, tmp_ptr, release);
        delete_data_if_needed(p_data, release);
    }
    else
    {
        value.sh_seq = new Tango::DevVarShortArray(data_size, data_size, p_data, release);
    }

    value_flag = true;

    //
    // Reset alarm flags
    //

    alarm.reset();

    //
    // Get time
    //

    set_time();
}

// Special set_value realisation for Tango::DevString due to different memory management of Tango::DevString (aka
// char*[])
template <>
inline void Attribute::set_value(Tango::DevString *p_data, long x, long y, bool release)
{
    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::DEV_STRING)
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::DEV_STRING << " for attribute " << name
          << ". Attribute data type is " << (CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Check that data size is less than the given max
    //

    if((x > max_x) || (y > max_y))
    {
        delete_data_if_needed(p_data, release);

        TangoSys_OMemStream o;

        o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
          << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    set_data_size();
    quality = Tango::ATTR_VALID;

    //
    // Throw exception if pointer is null and data size != 0
    //

    if(data_size != 0)
    {
        CHECK_PTR(p_data, name);
    }

    if(release)
    {
        char **strvec = Tango::DevVarStringArray::allocbuf(data_size);
        if(is_fwd_att())
        {
            for(std::uint32_t i = 0; i < data_size; i++)
            {
                strvec[i] = Tango::string_dup(p_data[i]);
            }
        }
        else
        {
            for(std::uint32_t i = 0; i < data_size; i++)
            {
                strvec[i] = p_data[i];
            }
        }
        value.str_seq = new Tango::DevVarStringArray(data_size, data_size, strvec, release);
    }
    else
    {
        value.str_seq = new Tango::DevVarStringArray(data_size, data_size, p_data, release);
    }

    delete_data_if_needed(p_data, release);

    value_flag = true;

    //
    // Get time
    //

    set_time();
}

// Special set_value realisation for Tango::DevEncoded due to different memory management of Tango::DevString (aka
// char*[])
template <>
inline void Attribute::set_value(Tango::DevEncoded *p_data, long x, long y, bool release)
{
    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::DEV_ENCODED)
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::DEV_ENCODED << " for attribute " << name
          << ". Attribute data type is " << (CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Check that data size is less than the given max
    //

    if((x > max_x) || (y > max_y))
    {
        delete_data_if_needed(p_data, release);

        TangoSys_OMemStream o;

        o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
          << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    set_data_size();
    quality = Tango::ATTR_VALID;

    //
    // Throw exception if pointer is null and data size != 0
    //

    if(data_size != 0)
    {
        CHECK_PTR(p_data, name);
    }

    //
    // If the data is wanted from the DevState command, store it in a sequence.
    // If the attribute  has an associated writable attribute, store data in a
    // temporary buffer (the write value must be added before the data is sent
    // back to the caller)
    //

    if(release)
    {
        DevEncoded *tmp_ptr = new DevEncoded[1];

        tmp_ptr->encoded_format = p_data->encoded_format;

        unsigned long nb_data = p_data->encoded_data.length();
        tmp_ptr->encoded_data.replace(nb_data, nb_data, p_data->encoded_data.get_buffer(true), true);
        p_data->encoded_data.replace(0, 0, nullptr, false);

        value.enc_seq = new Tango::DevVarEncodedArray(data_size, data_size, tmp_ptr, true);
    }
    else
    {
        value.enc_seq = new Tango::DevVarEncodedArray(data_size, data_size, p_data, release);
    }

    delete_data_if_needed(p_data, release);

    value_flag = true;

    //
    // Reset alarm flags
    //

    alarm.reset();

    //
    // Get time
    //

    set_time();
}

/*
 * method: Attribute::set_value_date_quality
 *
 * These methods store the attribute value inside the Attribute object,
 * with readout time and attribute quality factor provided by user.
 *
 *
 */

template <class T>
inline void
    Attribute::set_value_date_quality(T *p_data, time_t t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);

    if(qual == Tango::ATTR_INVALID)
    {
        delete_seq();
    }
}

template <class T>
inline void Attribute::set_value_date_quality(
    T *p_data, const TangoTimestamp &t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);

    if(qual == Tango::ATTR_INVALID)
    {
        delete_seq();
    }
}

template <>
inline void Attribute::set_value_date_quality(
    Tango::DevEncoded *p_data, time_t t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);
}

template <>
inline void Attribute::set_value_date_quality(
    Tango::DevEncoded *p_data, const TangoTimestamp &t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);
}

/// @endcond

} // namespace Tango
#endif // _ATTRSETVAL_TPP
