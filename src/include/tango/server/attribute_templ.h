//+===================================================================================================================
//
// file :               Attribute_templ.h
//
// description :        C++ source code for the Attribute class template methods when they are not specialized
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2011,2012,2013,2014,2015
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

#ifndef _ATTRIBUTE_TPP
#define _ATTRIBUTE_TPP

#include <tango/server/tango_clock.h>
#include <tango/client/Database.h>
#include <string>

namespace Tango
{

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::check_hard_coded_properties()
//
// description :
//        Check if the user tries to change attribute properties considered as hard coded
//      Throw exception in case of
//
// args :
//         in :
//            - user_conf : The attribute configuration sent by the user
//
//------------------------------------------------------------------------------------------------------------------

template <typename T>
void Attribute::check_hard_coded_properties(const T &user_conf)
{
    //
    // Check attribute name
    //

    std::string user_att_name(user_conf.name.in());
    std::transform(user_att_name.begin(), user_att_name.end(), user_att_name.begin(), ::tolower);
    if(user_att_name != get_name_lower())
    {
        throw_hard_coded_prop("name");
    }

    //
    // Check data type
    //

    if(user_conf.data_type != data_type)
    {
        throw_hard_coded_prop("data_type");
    }

    //
    // Check data format
    //

    if(user_conf.data_format != data_format)
    {
        throw_hard_coded_prop("data_format");
    }

    //
    // Check writable
    //

    if(user_conf.writable != writable)
    {
        throw_hard_coded_prop("writable");
    }

    //
    // Check max_dim_x
    //

    if(user_conf.max_dim_x != max_x)
    {
        throw_hard_coded_prop("max_dim_x");
    }

    //
    // Check max_dim_y
    //

    if(user_conf.max_dim_y != max_y)
    {
        throw_hard_coded_prop("max_dim_y");
    }

    //
    // Check writable_attr_name
    //

    std::string local_w_name(writable_attr_name);
    std::transform(local_w_name.begin(), local_w_name.end(), local_w_name.begin(), ::tolower);
    std::string user_w_name(user_conf.writable_attr_name.in());
    std::transform(user_w_name.begin(), user_w_name.end(), user_w_name.begin(), ::tolower);

    if(user_w_name != local_w_name)
    {
        throw_hard_coded_prop("writable_attr_name");
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::set_hard_coded_properties()
//
// description :
//        Set some "hard coded" attribute properties. This method is used only in case of forwarded attribute
//
// args :
//         in :
//            - user_conf : The attribute configuration sent by the user
//
//------------------------------------------------------------------------------------------------------------------

template <typename T>
void Attribute::set_hard_coded_properties(const T &user_conf)
{
    data_type = user_conf.data_type;
    data_format = user_conf.data_format;
    writable = user_conf.writable;
    max_x = user_conf.max_dim_x;
    max_y = user_conf.max_dim_y;
    writable_attr_name = user_conf.writable_attr_name;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::get_properties()
//
// description :
//        Gets attribute's properties in one call
//
// args :
//         out :
//            - props : The variable to be assigned the attribute's properties value
//
//-------------------------------------------------------------------------------------------------------------------

template <typename T>
void Attribute::get_properties(Tango::MultiAttrProp<T> &props)
{
    //
    // Check data type
    //

    if(!(data_type == DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == DEV_UCHAR) &&
       !(data_type == DEV_ENUM && Tango::tango_type_traits<T>::type_value() == DEV_SHORT) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    // Get the monitor protecting device att config
    // If the server is in its starting phase, gives a nullptr to the AutoLock object
    //

    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        mon_ptr = &(get_att_device()->get_att_conf_monitor());
    }
    AutoTangoMonitor sync1(mon_ptr);

    AttributeConfig_5 conf;
    get_properties(conf);

    props.label = conf.label;
    props.description = conf.description;
    props.unit = conf.unit;
    props.standard_unit = conf.standard_unit;
    props.display_unit = conf.display_unit;
    props.format = conf.format;
    props.min_alarm = conf.att_alarm.min_alarm;
    props.max_alarm = conf.att_alarm.max_alarm;
    props.min_value = conf.min_value;
    props.max_value = conf.max_value;
    props.min_warning = conf.att_alarm.min_warning;
    props.max_warning = conf.att_alarm.max_warning;
    props.delta_t = conf.att_alarm.delta_t;
    props.delta_val = conf.att_alarm.delta_val;
    props.event_period = conf.event_prop.per_event.period;
    props.archive_period = conf.event_prop.arch_event.period;
    props.rel_change = conf.event_prop.ch_event.rel_change;
    props.abs_change = conf.event_prop.ch_event.abs_change;
    props.archive_rel_change = conf.event_prop.arch_event.rel_change;
    props.archive_abs_change = conf.event_prop.arch_event.abs_change;
    props.enum_labels = enum_labels;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::set_properties()
//
// description :
//        Sets attribute's properties in one call
//
// args :
//         in :
//            - props : The new attribute's properties value
//
//-------------------------------------------------------------------------------------------------------------------

template <typename T>
void Attribute::set_properties(Tango::MultiAttrProp<T> &props)
{
    //
    // Check data type
    //

    if(!(data_type == DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == DEV_UCHAR) &&
       !(data_type == DEV_ENUM && Tango::tango_type_traits<T>::type_value() == DEV_SHORT) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    // Check if the user set values of properties which do not have any meaning for particular attribute data types
    //

    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
       (data_type == Tango::DEV_ENUM))
    {
        if(TG_strcasecmp(props.min_alarm, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("min_alarm", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.max_alarm, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("max_alarm", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.min_value, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("min_value", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.max_value, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("max_value", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.min_warning, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("min_warning", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.max_warning, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("max_warning", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.delta_t, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("delta_t", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.delta_val, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("delta_val", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.rel_change, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("rel_change", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.abs_change, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("abs_change", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.archive_rel_change, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("archive_rel_change", d_name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.archive_abs_change, AlrmValueNotSpec) != 0)
        {
            throw_err_data_type("archive_abs_change", d_name, "Attribute::set_properties()");
        }
    }

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
    AutoTangoMonitor sync1(mon_ptr);

    //
    // Get current attribute configuration (to retrieve un-mutable properties) and update properties with provided
    // values
    //

    AttributeConfig_5 conf;
    get_properties(conf);

    conf.label = CORBA::string_dup(props.label.c_str());
    conf.description = CORBA::string_dup(props.description.c_str());
    conf.unit = CORBA::string_dup(props.unit.c_str());
    conf.standard_unit = CORBA::string_dup(props.standard_unit.c_str());
    conf.display_unit = CORBA::string_dup(props.display_unit.c_str());
    conf.format = CORBA::string_dup(props.format.c_str());
    conf.att_alarm.min_alarm = CORBA::string_dup(props.min_alarm);
    conf.att_alarm.max_alarm = CORBA::string_dup(props.max_alarm);
    conf.min_value = CORBA::string_dup(props.min_value);
    conf.max_value = CORBA::string_dup(props.max_value);
    conf.att_alarm.min_warning = CORBA::string_dup(props.min_warning);
    conf.att_alarm.max_warning = CORBA::string_dup(props.max_warning);
    conf.att_alarm.delta_t = CORBA::string_dup(props.delta_t);
    conf.att_alarm.delta_val = CORBA::string_dup(props.delta_val);
    conf.event_prop.per_event.period = CORBA::string_dup(props.event_period);
    conf.event_prop.arch_event.period = CORBA::string_dup(props.archive_period);
    conf.event_prop.ch_event.rel_change = CORBA::string_dup(props.rel_change);
    conf.event_prop.ch_event.abs_change = CORBA::string_dup(props.abs_change);
    conf.event_prop.arch_event.rel_change = CORBA::string_dup(props.archive_rel_change);
    conf.event_prop.arch_event.abs_change = CORBA::string_dup(props.archive_abs_change);

    conf.enum_labels.length(props.enum_labels.size());
    for(size_t loop = 0; loop < props.enum_labels.size(); loop++)
    {
        conf.enum_labels[loop] = CORBA::string_dup(props.enum_labels[loop].c_str());
    }

    //
    // Set properties and update database
    //

    if(is_fwd_att())
    {
        FwdAttribute *fwd_attr = static_cast<FwdAttribute *>(this);
        fwd_attr->upd_att_config_base(conf.label.in());
        fwd_attr->upd_att_config(conf);
    }
    else
    {
        set_upd_properties(conf, d_name, true);
    }

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::set_upd_properties()
//
// description :
//        Set new attribute configuration AND update database (if required)
//
// args :
//         in :
//            - conf : The new attribute configuration
//            - dev_name : The device name
//            - from_ds : Flag set to true if the call is from a DS process
//
//-------------------------------------------------------------------------------------------------------------------

template <typename T>
void Attribute::set_upd_properties(const T &conf, const std::string &dev_name, bool from_ds)
{
    //
    // Backup current configuration
    //

    T old_conf;
    if(!is_fwd_att())
    {
        get_properties(old_conf);
    }

    //
    // Set flags which disable attribute configuration roll back in case there are some device startup exceptions
    //

    bool is_startup_exception = check_startup_exceptions;
    if(is_startup_exception)
    {
        startup_exceptions_clear = false;
    }

    try
    {
        //
        // Set properties locally. In case of exception bring the backed-up values
        //

        std::vector<AttPropDb> v_db;
        set_properties(conf, dev_name, from_ds, v_db);

        //
        // Check ranges coherence for min and max properties (min-max alarm / min-max value ...)
        //

        check_range_coherency(dev_name);

        //
        // At this point the attribute configuration is correct. Clear the device startup exceptions flag
        //

        startup_exceptions_clear = true;

        //
        // Update database
        //

        try
        {
            if(Tango::Util::instance()->use_db())
            {
                upd_database(v_db);
            }
        }
        catch(DevFailed &)
        {
            //
            // In case of exception, try to store old properties in the database and inform the user about the error
            //

            try
            {
                v_db.clear();
                set_properties(old_conf, dev_name, from_ds, v_db);
                upd_database(v_db);
            }
            catch(DevFailed &)
            {
                //
                // If the old values could not be restored, notify the user about possible database corruption
                //

                TangoSys_OMemStream o;

                o << "Device " << dev_name << "-> Attribute : " << name;
                o << "\nDatabase error occurred whilst setting attribute properties. The database may be corrupted."
                  << std::ends;
                TANGO_THROW_EXCEPTION(API_CorruptedDatabase, o.str());
            }

            throw;
        }
    }
    catch(DevFailed &)
    {
        //
        // If there are any device startup exceptions, do not roll back the attribute configuration unless the new
        // configuration is correct
        //

        if(!is_startup_exception && startup_exceptions_clear && !is_fwd_att())
        {
            std::vector<AttPropDb> v_db;
            set_properties(old_conf, dev_name, true, v_db);
        }

        throw;
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::Attribute_2_AttributeValue_base
//
// description :
//        Build an AttributeValue_X object (the base part) from the Attribute object content
//
// arguments
//         in :
//            - d : The device to which the attribute belongs to
//        out :
//            - ptr : Pointer to the AttributeValue_X object to be filled in
//
//--------------------------------------------------------------------------------------------------------------------

template <typename T>
void Attribute::Attribute_2_AttributeValue_base(T *ptr, Tango::DeviceImpl *d)
{
    if((name_lower == "state") || (name_lower == "status"))
    {
        ptr->quality = Tango::ATTR_VALID;

        if(name_lower == "state")
        {
            ptr->value.dev_state_att(d->get_state());
        }
        else
        {
            Tango::DevVarStringArray str_seq(1);
            str_seq.length(1);
            str_seq[0] = CORBA::string_dup(d->get_status().c_str());

            ptr->value.string_att_value(str_seq);
        }

        ptr->time = make_TimeVal(std::chrono::system_clock::now());
        ptr->r_dim.dim_x = 1;
        ptr->r_dim.dim_y = 0;
        ptr->w_dim.dim_x = 0;
        ptr->w_dim.dim_y = 0;

        ptr->name = CORBA::string_dup(name.c_str());
        ptr->data_format = data_format;
    }
    else
    {
        if(quality != Tango::ATTR_INVALID)
        {
            MultiAttribute *m_attr = d->get_device_attr();

            // Add the attribute setpoint to the value sequence

            if((writable == Tango::READ_WRITE) || (writable == Tango::READ_WITH_WRITE))
            {
                m_attr->add_write_value(*this);
            }

            // check for alarms to position the data quality value.
            if(is_alarmed().any())
            {
                check_alarm();
            }

            ptr->r_dim.dim_x = dim_x;
            ptr->r_dim.dim_y = dim_y;
            if((writable == Tango::READ_WRITE) || (writable == Tango::READ_WITH_WRITE))
            {
                WAttribute &assoc_att = m_attr->get_w_attr_by_ind(get_assoc_ind());
                ptr->w_dim.dim_x = assoc_att.get_w_dim_x();
                ptr->w_dim.dim_y = assoc_att.get_w_dim_y();
            }
            else
            {
                ptr->w_dim.dim_x = 0;
                ptr->w_dim.dim_y = 0;
            }
        }
        else
        {
            ptr->r_dim.dim_x = 0;
            ptr->r_dim.dim_y = 0;
            ptr->w_dim.dim_x = 0;
            ptr->w_dim.dim_y = 0;
            ptr->value.union_no_data(true);
        }

        ptr->time = when;
        ptr->quality = quality;
        ptr->data_format = data_format;
        ptr->name = CORBA::string_dup(name.c_str());
    }
}

template <typename T, typename V>
void Attribute::AttrValUnion_fake_copy(const T *src, V *dst)
{
    switch(src->value._d())
    {
    case ATT_BOOL:
    {
        const DevVarBooleanArray &tmp_seq = src->value.bool_att_value();
        DevVarBooleanArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<bool *>(tmp_seq.get_buffer()), false);
        dst->value.bool_att_value(tmp_seq_4);
    }
    break;

    case ATT_SHORT:
    {
        const DevVarShortArray &tmp_seq = src->value.short_att_value();
        DevVarShortArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<short *>(tmp_seq.get_buffer()), false);
        dst->value.short_att_value(tmp_seq_4);
    }
    break;

    case ATT_LONG:
    {
        const DevVarLongArray &tmp_seq = src->value.long_att_value();
        DevVarLongArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevLong *>(tmp_seq.get_buffer()), false);
        dst->value.long_att_value(tmp_seq_4);
    }
    break;

    case ATT_LONG64:
    {
        const DevVarLong64Array &tmp_seq = src->value.long64_att_value();
        DevVarLong64Array tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevLong64 *>(tmp_seq.get_buffer()), false);
        dst->value.long64_att_value(tmp_seq_4);
    }
    break;

    case ATT_FLOAT:
    {
        const DevVarFloatArray &tmp_seq = src->value.float_att_value();
        DevVarFloatArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<float *>(tmp_seq.get_buffer()), false);
        dst->value.float_att_value(tmp_seq_4);
    }
    break;

    case ATT_DOUBLE:
    {
        const DevVarDoubleArray &tmp_seq = src->value.double_att_value();
        DevVarDoubleArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<double *>(tmp_seq.get_buffer()), false);
        dst->value.double_att_value(tmp_seq_4);
    }
    break;

    case ATT_UCHAR:
    {
        const DevVarCharArray &tmp_seq = src->value.uchar_att_value();
        DevVarCharArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<unsigned char *>(tmp_seq.get_buffer()), false);
        dst->value.uchar_att_value(tmp_seq_4);
    }
    break;

    case ATT_USHORT:
    {
        const DevVarUShortArray &tmp_seq = src->value.ushort_att_value();
        DevVarUShortArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevUShort *>(tmp_seq.get_buffer()), false);
        dst->value.ushort_att_value(tmp_seq_4);
    }
    break;

    case ATT_ULONG:
    {
        const DevVarULongArray &tmp_seq = src->value.ulong_att_value();
        DevVarULongArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevULong *>(tmp_seq.get_buffer()), false);
        dst->value.ulong_att_value(tmp_seq_4);
    }
    break;

    case ATT_ULONG64:
    {
        const DevVarULong64Array &tmp_seq = src->value.ulong64_att_value();
        DevVarULong64Array tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevULong64 *>(tmp_seq.get_buffer()), false);
        dst->value.ulong64_att_value(tmp_seq_4);
    }
    break;

    case ATT_STRING:
    {
        const DevVarStringArray &tmp_seq = src->value.string_att_value();
        DevVarStringArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevString *>(tmp_seq.get_buffer()), false);
        dst->value.string_att_value(tmp_seq_4);
    }
    break;

    case ATT_STATE:
    {
        const DevVarStateArray &tmp_seq = src->value.state_att_value();
        DevVarStateArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevState *>(tmp_seq.get_buffer()), false);
        dst->value.state_att_value(tmp_seq_4);
    }
    break;

    case DEVICE_STATE:
    {
        const DevState &sta = src->value.dev_state_att();
        dst->value.dev_state_att(sta);
    }
    break;

    case ATT_ENCODED:
    {
        const DevVarEncodedArray &tmp_seq = src->value.encoded_att_value();
        DevVarEncodedArray tmp_seq_4(
            tmp_seq.length(), tmp_seq.length(), const_cast<DevEncoded *>(tmp_seq.get_buffer()), false);
        dst->value.encoded_att_value(tmp_seq_4);
    }
    break;

    case ATT_NO_DATA:
        dst->value.union_no_data(true);
        break;
    }
}

template <typename T>
void Attribute::AttrValUnion_2_Any(const T *src, CORBA::Any &dst)
{
    switch(src->value._d())
    {
    case ATT_BOOL:
    {
        const DevVarBooleanArray &tmp_seq = src->value.bool_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_SHORT:
    {
        const DevVarShortArray &tmp_seq = src->value.short_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_LONG:
    {
        const DevVarLongArray &tmp_seq = src->value.long_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_LONG64:
    {
        const DevVarLong64Array &tmp_seq = src->value.long64_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_FLOAT:
    {
        const DevVarFloatArray &tmp_seq = src->value.float_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_DOUBLE:
    {
        const DevVarDoubleArray &tmp_seq = src->value.double_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_UCHAR:
    {
        const DevVarCharArray &tmp_seq = src->value.uchar_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_USHORT:
    {
        const DevVarUShortArray &tmp_seq = src->value.ushort_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_ULONG:
    {
        const DevVarULongArray &tmp_seq = src->value.ulong_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_ULONG64:
    {
        const DevVarULong64Array &tmp_seq = src->value.ulong64_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_STRING:
    {
        const DevVarStringArray &tmp_seq = src->value.string_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_STATE:
    {
        const DevVarStateArray &tmp_seq = src->value.state_att_value();
        dst <<= tmp_seq;
    }
    break;

    case DEVICE_STATE:
    {
        const DevState &sta = src->value.dev_state_att();
        dst <<= sta;
    }
    break;

    case ATT_ENCODED:
    {
        const DevVarEncodedArray &tmp_seq = src->value.encoded_att_value();
        dst <<= tmp_seq;
    }
    break;

    case ATT_NO_DATA:
        break;
    }
}

template <class T>
inline void Attribute::_extract_value(CORBA::Any &dest)
{
    auto *ptr = get_value_storage<T>();
    dest <<= *ptr;
    delete_seq();
}

} // namespace Tango
#endif // _ATTRIBUTE_TPP
