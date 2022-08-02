//+============================================================================
//
// file :               w_attribute.cpp
//
// description :        C++ source code for the WAttribute class.
//			This class is used to manage attribute.
//			A Tango Device object instance has one
//			MultiAttribute object which is an aggregate of
//			Attribute or WAttribute objects
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//						European Synchrotron Radiation Facility
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


#include <tango/tango.h>
#include <tango/server/attribute.h>
#include <tango/server/w_attribute.h>
#include <tango/server/classattribute.h>
#include <tango/server/tango_clock.h>

#include <functional>
#include <algorithm>

#ifdef _TG_WINDOWS_
#include <sys/types.h>
#include <float.h>
#endif /* _TG_WINDOWS_ */

namespace Tango
{

//+-------------------------------------------------------------------------
//
// method : 		WAttribute::WAttribute
//
// description : 	constructor for the WAttribute class from the
//			attribute property vector, its type and the device
//			name
//
// argument : in : 	- prop_list : The attribute property list
//			- type : The attrubute data type
//			- dev_name : The device name
//
//--------------------------------------------------------------------------


WAttribute::WAttribute(std::vector<AttrProperty> &prop_list,
                       Attr &tmp_attr, const std::string &dev_name, long idx)
    : Attribute(prop_list, tmp_attr, dev_name, idx),
      long_ptr(NULL), double_ptr(NULL), str_ptr(NULL), float_ptr(NULL),
      boolean_ptr(NULL), ushort_ptr(NULL), uchar_ptr(NULL), encoded_ptr(NULL),
      string_allocated(false), memorized(false), memorized_init(false), w_ext(new WAttributeExt),
      long64_ptr(NULL), ulong_ptr(NULL), ulong64_ptr(NULL), state_ptr(NULL), uswv(false), mem_write_failed(false)
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

    if (is_memorized() == true)
    {
        try
        {
            mem_value = get_attr_value(prop_list, MemAttrPropName);
        }
        catch (Tango::DevFailed &)
        {
            mem_value = MemNotUsed;
        }
    }
}

//+-------------------------------------------------------------------------
//
// method : 		WAttribute::~WAttribute
//
// description : 	destructor for the WAttribute class
//
//--------------------------------------------------------------------------

WAttribute::~WAttribute()
{
    Tango::string_free(str_val);
    Tango::string_free(old_str_val);
//	Tango::string_free(encoded_val.encoded_format);
//	Tango::string_free(old_encoded_val.encoded_format);
}


//+------------------------------------------------------------------------------------------------------------------
//
// method :
//		WAttribute::set_rvalue
//
// description :
//		This method is used when a Writable attribute is set to set the value in the Attribute class. This is
//		necessary for the read_attribute CORBA operation which takes its data from this internal Attribute
//		class data. It is used in the read_attributes code in the device class
//
//--------------------------------------------------------------------------------------------------------------------

void WAttribute::set_rvalue()
{
    switch (data_type)
    {
        case Tango::DEV_SHORT:
        case Tango::DEV_ENUM:
            if (data_format == Tango::SCALAR)
            {
                set_value(&short_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevShort *>(short_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_LONG:
            if (data_format == Tango::SCALAR)
            {
                set_value(&long_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevLong *>(long_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_LONG64:
            if (data_format == Tango::SCALAR)
            {
                set_value(&long64_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevLong64 *>(long64_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_DOUBLE:
            if (data_format == Tango::SCALAR)
            {
                set_value(&double_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevDouble *>(double_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_STRING:
            if (data_format == Tango::SCALAR)
            {
                set_value(&str_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevString *>(str_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_FLOAT:
            if (data_format == Tango::SCALAR)
            {
                set_value(&float_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevFloat *>(float_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_BOOLEAN:
            if (data_format == Tango::SCALAR)
            {
                set_value(&boolean_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevBoolean *>(boolean_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_USHORT:
            if (data_format == Tango::SCALAR)
            {
                set_value(&ushort_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevUShort *>(ushort_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_UCHAR:
            if (data_format == Tango::SCALAR)
            {
                set_value(&uchar_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevUChar *>(uchar_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_ULONG:
            if (data_format == Tango::SCALAR)
            {
                set_value(&ulong_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevULong *>(ulong_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_ULONG64:
            if (data_format == Tango::SCALAR)
            {
                set_value(&ulong64_val, 1, 0, false);
            }
            else
            {
                set_value(const_cast<DevULong64 *>(ulong64_array_val.get_buffer()), w_dim_x, w_dim_y, false);
            }
            break;

        case Tango::DEV_STATE:
            if (data_format == Tango::SCALAR)
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
// method : 		WAttribute::check_written_value
//
// description : 	Check the value sent by the caller and copy incoming data
//					for SCALAR attribute only
//
// in :			any : Reference to the CORBA Any object
//
//--------------------------------------------------------------------------

void WAttribute::check_written_value(const CORBA::Any &any, unsigned long x, unsigned long y)
{
    _update_any_written_value(any, x, y);
}

void WAttribute::check_written_value(const Tango::AttrValUnion &att_union, unsigned long x, unsigned long y)
{
    _update_any_written_value(att_union, x, y);
}

//+-------------------------------------------------------------------------
//
// method : 		WAttribute::get_write_value_length
//
// description : 	Returm to the caller the length of the new value to
//			be written into the attribute
//
//--------------------------------------------------------------------------

long WAttribute::get_write_value_length()
{
    switch (data_format)
    {
        case Tango::SCALAR:   return 1;
        case Tango::SPECTRUM: return w_dim_x;
        case Tango::IMAGE:    return w_dim_x * w_dim_y;
        default:              return 0;
    }
}


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
// method : 		WAttribute::rollback
//
// description : 	Reset the internal data to its value before the
//			set_write_value method was applied (Useful in case of
//			error in the set_write_value method)
//
//--------------------------------------------------------------------------

void WAttribute::rollback()
{
    switch (data_type)
    {
        case Tango::DEV_SHORT :
        case Tango::DEV_ENUM :
            short_val = old_short_val;
            break;

        case Tango::DEV_LONG :
            long_val = old_long_val;
            break;

        case Tango::DEV_LONG64 :
            long64_val = old_long64_val;
            break;

        case Tango::DEV_DOUBLE :
            double_val = old_double_val;
            break;

        case Tango::DEV_STRING :
            Tango::string_free(str_val);
            str_val = CORBA::string_dup(old_str_val);
            break;

        case Tango::DEV_FLOAT :
            float_val = old_float_val;
            break;

        case Tango::DEV_BOOLEAN :
            boolean_val = old_boolean_val;
            break;

        case Tango::DEV_USHORT :
            ushort_val = old_ushort_val;
            break;

        case Tango::DEV_UCHAR :
            Tango::string_free(str_val);
            break;

        case Tango::DEV_ULONG :
            ulong_val = old_ulong_val;
            break;

        case Tango::DEV_ULONG64 :
            ulong64_val = old_ulong64_val;
            break;

        case Tango::DEV_STATE :
            dev_state_val = old_dev_state_val;
            break;

        default:
            break;
    }
}

void WAttribute::copy_data(const CORBA::Any &any)
{
    _copy_any_data(any);
}

void WAttribute::copy_data(const Tango::AttrValUnion &the_union)
{
    _copy_any_data(the_union);
}

//+-------------------------------------------------------------------------
//
// method : 		WAttribute::set_written_date
//
// description : 	Memorized when the attribute is written
//
//--------------------------------------------------------------------------

void WAttribute::set_written_date()
{
    write_date = make_timeval(std::chrono::system_clock::now());
}

//+-------------------------------------------------------------------------
//
// method : 		WAttribute::check_rds_alarm
//
// description : 	Check if the attribute is in read different from set
//			alarm.
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

    if (write_date.tv_sec == 0)
    {
        return false;
    }

//
// First, check if it is necessary to check attribute value
// Give some time to the device to change its output
//

    auto time_diff = std::chrono::system_clock::now() - make_system_time(write_date);

    if (time_diff >= std::chrono::milliseconds(delta_t))
    {

//
// Now check attribute value with again a switch on attribute data type
//

        long nb_written, nb_read, nb_data, i;

        switch (data_type)
        {
            case Tango::DEV_SHORT:
                nb_written = short_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.sh_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    short delta = (data_format == Tango::SCALAR) ? short_array_val[0] - tmp_sh[0] : short_array_val[i]
                        - (*value.sh_seq)[i];
                    if (abs(delta) >= delta_val.sh)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_LONG:
                nb_written = long_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.lg_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    DevLong delta = (data_format == Tango::SCALAR) ? long_array_val[0] - tmp_lo[0] : long_array_val[i]
                        - (*value.lg_seq)[i];
                    if (abs(delta) >= delta_val.lg)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_LONG64:
                nb_written = long64_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.lg64_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    DevLong64 delta =
                        (data_format == Tango::SCALAR) ? long64_array_val[0] - get_tmp_scalar_long64()[0] :
                        long64_array_val[i] - (*value.lg64_seq)[i];

                    DevLong64 abs_delta;
                    if (delta < 0)
                    {
                        abs_delta = -delta;
                    }
                    else
                    {
                        abs_delta = delta;
                    }

                    if (abs_delta >= delta_val.lg64)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_DOUBLE:
                nb_written = double_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.db_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    // check for NAN values
                    if (data_format == Tango::SCALAR)
                    {
                        if (std::isnan(double_array_val[0]) || std::isnan(tmp_db[0]))
                        {
                            // send an alarm if only read or set value are NAN
                            if (!(std::isnan(double_array_val[0]) && std::isnan(tmp_db[0])))
                            {
                                quality = Tango::ATTR_ALARM;
                                alarm.set(rds);
                                ret = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (std::isnan(double_array_val[i]) || std::isnan((*value.db_seq)[i]))
                        {
                            // send an alarm if only read or set value are NAN
                            if (!(std::isnan(double_array_val[i]) && std::isnan((*value.db_seq)[i])))
                            {
                                quality = Tango::ATTR_ALARM;
                                alarm.set(rds);
                                ret = true;
                                break;
                            }
                        }
                    }


                    double delta =
                        (data_format == Tango::SCALAR) ? double_array_val[0] - tmp_db[0] : double_array_val[i]
                            - (*value.db_seq)[i];
                    if (fabs(delta) >= delta_val.db)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_FLOAT:
                nb_written = float_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.fl_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    // check for NAN values
                    if (data_format == Tango::SCALAR)
                    {
                        if (std::isnan(float_array_val[0]) || std::isnan(tmp_fl[0]))
                        {
                            // send an alarm if only read or set value are NAN
                            if (!(std::isnan(float_array_val[0]) && std::isnan(tmp_fl[0])))
                            {
                                quality = Tango::ATTR_ALARM;
                                alarm.set(rds);
                                ret = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (std::isnan(float_array_val[i]) || std::isnan((*value.fl_seq)[i]))
                        {
                            // send an alarm if only read or set value are NAN
                            if (!(std::isnan(float_array_val[i]) && std::isnan((*value.fl_seq)[i])))
                            {
                                quality = Tango::ATTR_ALARM;
                                alarm.set(rds);
                                ret = true;
                                break;
                            }
                        }
                    }

                    float delta = (data_format == Tango::SCALAR) ? float_array_val[0] - tmp_fl[0] : float_array_val[i]
                        - (*value.fl_seq)[i];
                    double delta_d = (double) delta;
                    if (((float) fabs(delta_d)) >= delta_val.fl)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_USHORT:
                nb_written = ushort_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.ush_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    unsigned short delta =
                        (data_format == Tango::SCALAR) ? ushort_array_val[0] - tmp_ush[0] : ushort_array_val[i]
                            - (*value.ush_seq)[i];
                    if (delta >= delta_val.ush)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_UCHAR:
                nb_written = uchar_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.cha_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    unsigned char delta =
                        (data_format == Tango::SCALAR) ? uchar_array_val[0] - tmp_cha[0] : uchar_array_val[i]
                            - (*value.cha_seq)[i];
                    if (delta >= delta_val.uch)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_ULONG:
                nb_written = ulong_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.ulg_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    DevLong delta = (data_format == Tango::SCALAR) ? ulong_array_val[0] - get_tmp_scalar_ulong()[0] :
                                    ulong_array_val[i] - (*value.ulg_seq)[i];
                    if ((unsigned int) abs(delta) >= delta_val.ulg)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_ULONG64:
                nb_written = ulong64_array_val.length();
                nb_read = (data_format == Tango::SCALAR) ? 1 : value.ulg64_seq->length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    DevLong64 delta =
                        (data_format == Tango::SCALAR) ? ulong64_array_val[0] - get_tmp_scalar_ulong64()[0] :
                        ulong64_array_val[i] - (*value.ulg64_seq)[i];

                    DevULong64 abs_delta;
                    if (delta < 0)
                    {
                        abs_delta = -delta;
                    }
                    else
                    {
                        abs_delta = delta;
                    }

                    if (abs_delta >= delta_val.ulg64)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;

            case Tango::DEV_ENCODED:
                nb_written = ::strlen(encoded_val.encoded_format.in());
                nb_read = ::strlen((*value.enc_seq)[0].encoded_format.in());
                if (nb_written != nb_read)
                {
                    quality = Tango::ATTR_ALARM;
                    alarm.set(rds);
                    ret = true;
                    break;
                }
                if (::strcmp(encoded_val.encoded_format.in(), (*value.enc_seq)[0].encoded_format.in()) != 0)
                {
                    quality = Tango::ATTR_ALARM;
                    alarm.set(rds);
                    ret = true;
                    break;
                }

                nb_written = encoded_val.encoded_data.length();
                nb_read = (*value.enc_seq)[0].encoded_data.length();
                nb_data = (nb_written > nb_read) ? nb_read : nb_written;
                for (i = 0; i < nb_data; i++)
                {
                    unsigned char delta = encoded_val.encoded_data[i] - (*value.enc_seq)[0].encoded_data[i];
                    if (delta >= delta_val.uch)
                    {
                        quality = Tango::ATTR_ALARM;
                        alarm.set(rds);
                        ret = true;
                        break;
                    }
                }
                break;
        }
    }

    return ret;
}

void WAttribute::set_min_value(char *new_min_value_str)
{
    set_min_value(std::string(new_min_value_str));
}

void WAttribute::set_min_value(const char *new_min_value_str)
{
    set_min_value(std::string(new_min_value_str));
}

void WAttribute::set_max_value(char *new_max_value_str)
{
    set_max_value(std::string(new_max_value_str));
}

void WAttribute::set_max_value(const char *new_max_value_str)
{
    set_max_value(std::string(new_max_value_str));
}

//+-------------------------------------------------------------------------
//
// method : 		WAttribute::mem_value_below_above()
//
// description : 	Check if the attribute last written value is below
//                  (or above) the new threshold sent by the requester
//
// Arg in :			check_type : Which check has to be done: Below or above
//
// This method returns true if the new threshold wanted by the user is not
// coherent with the memorized value. Otherwise, returns false.
//
//--------------------------------------------------------------------------

bool WAttribute::mem_value_below_above(MinMaxValueCheck check_type, std::string &ret_mem_value)
{
    bool ret = false;

    if (mem_value == MemNotUsed)
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

    switch (data_type)
    {
        case Tango::DEV_SHORT:
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> sh_val;
                if (check_type == MIN)
                {
                    if (sh_val < min_value.sh)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (sh_val > max_value.sh)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = short_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (short_array_val[i] < min_value.sh)
                        {
                            ss << short_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (short_array_val[i] > max_value.sh)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> lg_val;
                if (check_type == MIN)
                {
                    if (lg_val < min_value.lg)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (lg_val > max_value.lg)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = long_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (long_array_val[i] < min_value.lg)
                        {
                            ss << long_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (long_array_val[i] > max_value.lg)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> lg64_val;
                if (check_type == MIN)
                {
                    if (lg64_val < min_value.lg64)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (lg64_val > max_value.lg64)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = long64_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (long64_array_val[i] < min_value.lg64)
                        {
                            ss << long64_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (long64_array_val[i] > max_value.lg64)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> db_val;
                if (check_type == MIN)
                {
                    if (db_val < min_value.db)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (db_val > max_value.db)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = double_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (double_array_val[i] < min_value.db)
                        {
                            ss << double_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (double_array_val[i] > max_value.db)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> fl_val;
                if (check_type == MIN)
                {
                    if (fl_val < min_value.fl)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (fl_val > max_value.fl)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = float_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (float_array_val[i] < min_value.fl)
                        {
                            ss << float_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (float_array_val[i] > max_value.fl)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> ush_val;
                if (check_type == MIN)
                {
                    if (ush_val < min_value.ush)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (ush_val > max_value.ush)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = ushort_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (ushort_array_val[i] < min_value.ush)
                        {
                            ss << ushort_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (ushort_array_val[i] > max_value.ush)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> uch_val;
                if (check_type == MIN)
                {
                    if (uch_val < min_value.uch)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (uch_val > max_value.uch)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = uchar_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (uchar_array_val[i] < min_value.uch)
                        {
                            ss << uchar_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (uchar_array_val[i] > max_value.uch)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> ulg_val;
                if (check_type == MIN)
                {
                    if (ulg_val < min_value.ulg)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (ulg_val > max_value.ulg)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = ulong_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (ulong_array_val[i] < min_value.ulg)
                        {
                            ss << ulong_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (ulong_array_val[i] > max_value.ulg)
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
            if (svr_starting == true)
            {
                ss << mem_value;
                ss >> ulg64_val;
                if (check_type == MIN)
                {
                    if (ulg64_val < min_value.ulg64)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
                else
                {
                    if (ulg64_val > max_value.ulg64)
                    {
                        ret_mem_value = mem_value;
                        ret = true;
                    }
                }
            }
            else
            {
                nb_written = ulong64_array_val.length();
                for (i = 0; i < nb_written; i++)
                {
                    if (check_type == MIN)
                    {
                        if (ulong64_array_val[i] < min_value.ulg64)
                        {
                            ss << ulong64_array_val[i];
                            ret_mem_value = ss.str();
                            ret = true;
                            break;
                        }
                    }
                    else
                    {
                        if (ulong64_array_val[i] > max_value.ulg64)
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

        default:
            break;
    }

    return ret;
}

} // End of Tango namespace
