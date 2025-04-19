//====================================================================================================================
//
// file :               Attribute.cpp
//
// description :        C++ source code for the Attribute class. This class is used to manage attribute.
//                        A Tango Device object instance has one MultiAttribute object which is an aggregate of
//                        Attribute or WAttribute objects
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
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU
// Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or
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
//====================================================================================================================

#include <tango/server/attribute.h>
#include <tango/server/classattribute.h>
#include <tango/server/eventsupplier.h>
#include <tango/server/tango_clock.h>
#include <tango/server/fwdattribute.h>
#include <tango/server/log4tango.h>
#include <tango/server/device.h>
#include <tango/client/Database.h>
#include <tango/internal/server/attribute_utils.h>

#include <functional>
#include <algorithm>

#ifdef _TG_WINDOWS_
  #include <sys/types.h>
#endif /* _TG_WINDOWS_ */

namespace
{ // anonymous

auto find_property(std::vector<Tango::AttrProperty> &prop_list, const char *prop_name)
{
    return std::find_if(std::begin(prop_list),
                        std::end(prop_list),
                        [&prop_name](Tango::AttrProperty &attr) { return attr.get_name() == prop_name; });
}

// Helper class to create zero-initialized Tango::AttributeValue_N instances.
// AttributeValue has user-defined constructor that leaves most of the fields
// uninitialized. We must value-initialize this wrapper which will first zero-
// initialize underlying value and then will call its constructor.
template <typename T>
struct ZeroInitialize
{
    T value;
};

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        check_range_coherency
//
// description :
//        Check coherency between min and max value for properties where a min and a max is used
//
// argument :
//        in :
//            - dev_name: The device name
//
//-------------------------------------------------------------------------------------------------------------------
void check_range_coherency(const std::string &dev_name,
                           Tango::Attribute &attr,
                           const Tango::Attr_CheckVal &min_value,
                           const Tango::Attr_CheckVal &max_value,
                           const Tango::Attr_CheckVal &min_alarm,
                           const Tango::Attr_CheckVal &max_alarm,
                           const Tango::Attr_CheckVal &min_warning,
                           const Tango::Attr_CheckVal &max_warning,
                           bool check_min_value,
                           bool check_max_value)
{
    //
    // Check ranges coherence for min and max value
    //

    const long data_type = attr.get_data_type();
    const std::string &name = attr.get_name();
    if(check_min_value && check_max_value)
    {
        if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE) &&
           (data_type != Tango::DEV_ENUM))
        {
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(min_value.sh >= max_value.sh)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_LONG:
                if(min_value.lg >= max_value.lg)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_LONG64:
                if(min_value.lg64 >= max_value.lg64)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_DOUBLE:
                if(min_value.db >= max_value.db)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_FLOAT:
                if(min_value.fl >= max_value.fl)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_USHORT:
                if(min_value.ush >= max_value.ush)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_UCHAR:
                if(min_value.uch >= max_value.uch)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ULONG:
                if(min_value.ulg >= max_value.ulg)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ULONG64:
                if(min_value.ulg64 >= max_value.ulg64)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ENCODED:
                if(min_value.uch >= max_value.uch)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_value", "max_value", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("min_value", dev_name, name, "Attribute::set_upd_properties()");
        }
    }

    //
    // Check ranges coherence for min and max alarm
    //

    const auto &alarm_conf = attr.is_alarmed();
    if(alarm_conf.test(Tango::Attribute::alarm_flags::min_level) &&
       alarm_conf.test(Tango::Attribute::alarm_flags::max_level))
    {
        if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE) &&
           (data_type != Tango::DEV_ENUM))
        {
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(min_alarm.sh >= max_alarm.sh)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_LONG:
                if(min_alarm.lg >= max_alarm.lg)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_LONG64:
                if(min_alarm.lg64 >= max_alarm.lg64)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_DOUBLE:
                if(min_alarm.db >= max_alarm.db)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_FLOAT:
                if(min_alarm.fl >= max_alarm.fl)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_USHORT:
                if(min_alarm.ush >= max_alarm.ush)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_UCHAR:
                if(min_alarm.uch >= max_alarm.uch)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ULONG:
                if(min_alarm.ulg >= max_alarm.ulg)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ULONG64:
                if(min_alarm.ulg64 >= max_alarm.ulg64)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ENCODED:
                if(min_alarm.uch >= max_alarm.uch)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_alarm", "max_alarm", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("min_alarm", dev_name, name, "Attribute::set_upd_properties()");
        }
    }

    //
    // Check ranges coherence for min and max warning
    //

    if(alarm_conf.test(Tango::Attribute::alarm_flags::min_warn) &&
       alarm_conf.test(Tango::Attribute::alarm_flags::max_warn))
    {
        if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE) &&
           (data_type != Tango::DEV_ENUM))
        {
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(min_warning.sh >= max_warning.sh)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_LONG:
                if(min_warning.lg >= max_warning.lg)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_LONG64:
                if(min_warning.lg64 >= max_warning.lg64)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_DOUBLE:
                if(min_warning.db >= max_warning.db)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_FLOAT:
                if(min_warning.fl >= max_warning.fl)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_USHORT:
                if(min_warning.ush >= max_warning.ush)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_UCHAR:
                if(min_warning.uch >= max_warning.uch)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ULONG:
                if(min_warning.ulg >= max_warning.ulg)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ULONG64:
                if(min_warning.ulg64 >= max_warning.ulg64)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;

            case Tango::DEV_ENCODED:
                if(min_warning.uch >= max_warning.uch)
                {
                    Tango::detail::throw_incoherent_val_err(
                        "min_warning", "max_warning", dev_name, name, "Attribute::set_upd_properties()");
                }
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("min_warning", dev_name, name, "Attribute::set_upd_properties()");
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        set_upd_properties()
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
void set_upd_properties(const T &conf,
                        const std::string &dev_name,
                        bool from_ds,
                        bool &startup_exceptions_clear,
                        Tango::Attribute &attr,
                        const Tango::Attr_CheckVal &min_value,
                        const Tango::Attr_CheckVal &max_value,
                        const Tango::Attr_CheckVal &min_alarm,
                        const Tango::Attr_CheckVal &max_alarm,
                        const Tango::Attr_CheckVal &min_warning,
                        const Tango::Attr_CheckVal &max_warning,
                        bool check_min_value,
                        bool check_max_value)
{
    //
    // Backup current configuration
    //

    T old_conf;
    if(!attr.is_fwd_att())
    {
        attr.get_properties(old_conf);
    }

    //
    // Set flags which disable attribute configuration roll back in case there are some device startup exceptions
    //

    if(attr.is_startup_exception())
    {
        startup_exceptions_clear = false;
    }

    try
    {
        //
        // Set properties locally. In case of exception bring the backed-up values
        //

        std::vector<Tango::Attribute::AttPropDb> v_db;
        attr.set_properties(conf, dev_name, from_ds, v_db);

        //
        // Check ranges coherence for min and max properties (min-max alarm / min-max value ...)
        //

        check_range_coherency(dev_name,
                              attr,
                              min_value,
                              max_value,
                              min_alarm,
                              max_alarm,
                              min_warning,
                              max_warning,
                              check_min_value,
                              check_max_value);

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
                attr.upd_database(v_db);
            }
        }
        catch(Tango::DevFailed &)
        {
            //
            // In case of exception, try to store old properties in the database and inform the user about the error
            //

            try
            {
                v_db.clear();
                attr.set_properties(old_conf, dev_name, from_ds, v_db);
                attr.upd_database(v_db);
            }
            catch(Tango::DevFailed &)
            {
                //
                // If the old values could not be restored, notify the user about possible database corruption
                //

                TangoSys_OMemStream o;

                o << "Device " << dev_name << "-> Attribute : " << attr.get_name();
                o << "\nDatabase error occurred whilst setting attribute properties. The database may be corrupted."
                  << std::ends;
                TANGO_THROW_EXCEPTION(Tango::API_CorruptedDatabase, o.str());
            }

            throw;
        }
    }
    catch(Tango::DevFailed &)
    {
        //
        // If there are any device startup exceptions, do not roll back the attribute configuration unless the new
        // configuration is correct
        //

        if(!attr.is_startup_exception() && startup_exceptions_clear && !attr.is_fwd_att())
        {
            std::vector<Tango::Attribute::AttPropDb> v_db;
            attr.set_properties(old_conf, dev_name, true, v_db);
        }

        throw;
    }
}

} // namespace

namespace Tango
{

void LastAttrValue::store(const AttributeValue_5 *attr_5,
                          const AttributeValue_4 *attr_4,
                          const AttributeValue_3 *attr_3,
                          const AttributeValue *attr,
                          DevFailed *error)
{
    if(error != nullptr)
    {
        except = *error;
        err = true;
    }
    else
    {
        if(attr_5 != nullptr)
        {
            quality = attr_5->quality;
            value_4 = attr_5->value;
        }
        else if(attr_4 != nullptr)
        {
            quality = attr_4->quality;
            value_4 = attr_4->value;
        }
        else if(attr_3 != nullptr)
        {
            quality = attr_3->quality;
            value = attr_3->value;
        }
        else if(attr != nullptr)
        {
            quality = attr->quality;
            value = attr->value;
        }

        err = false;
    }

    inited = true;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::Attribute
//
// description :
//        Constructor for the Attribute class from the attribute property vector, its type and the device name
//
// argument :
//        in :
//            - prop_list : The attribute property list
//            - tmp_attr :
//            - dev_name : The device name
//            - idx :
//
//--------------------------------------------------------------------------------------------------------------------

Attribute::Attribute(std::vector<AttrProperty> &prop_list, Attr &tmp_attr, const std::string &dev_name, long idx)

{
    //
    // Create the extension class
    //

    ext = new Attribute::AttributeExt();

    idx_in_attr = idx;
    d_name = dev_name;
    attr_serial_model = ATTR_BY_KERNEL;
    scalar_str_attr_release = false;

    //
    // Init the attribute name
    //

    name = tmp_attr.get_name();
    name_size = name.size();
    name_lower = name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    //
    // Clear alarm data
    //

    alarm_conf.reset();
    alarm.reset();

    //
    // Init the remaining attribute main characteristic
    //

    data_type = tmp_attr.get_type();
    writable = tmp_attr.get_writable();
    data_format = tmp_attr.get_format();
    disp_level = tmp_attr.get_disp_level();
    poll_period = tmp_attr.get_polling_period();
    writable_attr_name = tmp_attr.get_assoc();

    //
    // Init the event characteristics
    //

    change_event_implmented = tmp_attr.is_change_event();
    check_change_event_criteria = tmp_attr.is_check_change_criteria();
    alarm_event_implmented = tmp_attr.is_alarm_event();
    check_alarm_event_criteria = tmp_attr.is_check_alarm_criteria();
    archive_event_implmented = tmp_attr.is_archive_event();
    check_archive_event_criteria = tmp_attr.is_check_archive_criteria();
    dr_event_implmented = tmp_attr.is_data_ready_event();

    switch(data_format)
    {
    case Tango::SPECTRUM:
        max_x = static_cast<SpectrumAttr &>(tmp_attr).get_max_x();
        max_y = 0;
        dim_y = 0;
        break;

    case Tango::IMAGE:
        max_x = static_cast<ImageAttr &>(tmp_attr).get_max_x();
        max_y = static_cast<ImageAttr &>(tmp_attr).get_max_y();
        break;

    default:
        max_x = 1;
        max_y = 0;
        dim_x = 1;
        dim_y = 0;
    }

    //
    // Initialise optional properties
    //

    init_opt_prop(prop_list, dev_name);

    //
    // Initialise event related fields
    //

    init_event_prop(prop_list, dev_name, tmp_attr);

    //
    // Enum init (in case of)
    //

    if(data_type == DEV_ENUM)
    {
        init_enum_prop(prop_list);
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::~Attribute
//
// description :
//        Destructor for the Attribute class
//
//--------------------------------------------------------------------------------------------------------------------

Attribute::~Attribute()
{
    delete_seq();

    try
    {
        delete ext;
        delete[] loc_enum_ptr;
    }
    catch(omni_thread_fatal &)
    {
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::init_event_prop
//
// description :
//        Init the event related properties
//
// argument :
//         in :
//            - prop_list : The property vector
//            - dev_name : The device name
//            - att : The user attribute object
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::init_event_prop(std::vector<AttrProperty> &prop_list, const std::string &dev_name, Attr &att)
{
    rel_change[0] = INT_MAX;         // default for relative change is none
    rel_change[1] = INT_MAX;         // default for relative change is none
    abs_change[0] = INT_MAX;         // default for absolute change is none
    abs_change[1] = INT_MAX;         // default for absolute change is none
    archive_rel_change[0] = INT_MAX; // default for archive change is none
    archive_rel_change[1] = INT_MAX; // default for archive change is none
    archive_abs_change[0] = INT_MAX; // default for archive change is none
    archive_abs_change[1] = INT_MAX; // default for archive change is none

    notifd_event = false;
    zmq_event = false;

    std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
    size_t nb_user = def_user_prop.size();

    //
    // Init min and max relative change for change event
    //

    try
    {
        std::string rel_change_str;
        bool rel_change_defined = false;
        try
        {
            rel_change_str = get_attr_value(prop_list, "rel_change");
            rel_change_defined = true;
        }
        catch(...)
        {
        }

        if(rel_change_defined)
        {
            if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE))
            {
                std::vector<double> rel_change_tmp;
                std::vector<bool> rel_change_set_usr_def;
                std::vector<bool> unused;
                validate_change_properties(
                    dev_name, "rel_change", rel_change_str, rel_change_tmp, rel_change_set_usr_def, unused);
                rel_change[0] = rel_change_tmp[0];
                rel_change[1] = rel_change_tmp[1];

                if(rel_change_set_usr_def[0] || rel_change_set_usr_def[1])
                {
                    if(nb_user != 0)
                    {
                        size_t i;
                        for(i = 0; i < nb_user; i++)
                        {
                            if(def_user_prop[i].get_name() == "rel_change")
                            {
                                break;
                            }
                        }
                        if(i != nb_user)
                        {
                            std::vector<double> rel_change_usr_def;
                            validate_change_properties(
                                dev_name, "rel_change", def_user_prop[i].get_value(), rel_change_usr_def);
                            if(rel_change_set_usr_def[0])
                            {
                                rel_change[0] = rel_change_usr_def[0];
                            }
                            if(rel_change_set_usr_def[1])
                            {
                                rel_change[1] = rel_change_usr_def[1];
                            }
                        }
                    }
                }
                TANGO_LOG_INFO << "Attribute::Attribute(): rel_change = " << rel_change[0] << " " << rel_change[1]
                               << std::endl;
            }
            else
            {
                Tango::detail::throw_err_data_type("rel_change", dev_name, name, "Attribute::init_event_prop()");
            }
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("rel_change", e);
    }

    //
    // Init min and max absolute change for change event
    //

    try
    {
        std::string abs_change_str;
        bool abs_change_defined = false;
        try
        {
            abs_change_str = get_attr_value(prop_list, "abs_change");
            abs_change_defined = true;
        }
        catch(...)
        {
        }

        if(abs_change_defined)
        {
            if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE))
            {
                std::vector<double> abs_change_tmp;
                std::vector<bool> abs_change_set_usr_def;
                std::vector<bool> unused;
                validate_change_properties(
                    dev_name, "abs_change", abs_change_str, abs_change_tmp, abs_change_set_usr_def, unused);
                abs_change[0] = abs_change_tmp[0];
                abs_change[1] = abs_change_tmp[1];

                if(abs_change_set_usr_def[0] || abs_change_set_usr_def[1])
                {
                    if(nb_user != 0)
                    {
                        size_t i;
                        for(i = 0; i < nb_user; i++)
                        {
                            if(def_user_prop[i].get_name() == "abs_change")
                            {
                                break;
                            }
                        }
                        if(i != nb_user)
                        {
                            std::vector<double> abs_change_usr_def;
                            validate_change_properties(
                                dev_name, "abs_change", def_user_prop[i].get_value(), abs_change_usr_def);
                            if(abs_change_set_usr_def[0])
                            {
                                abs_change[0] = abs_change_usr_def[0];
                            }
                            if(abs_change_set_usr_def[1])
                            {
                                abs_change[1] = abs_change_usr_def[1];
                            }
                        }
                    }
                }
                TANGO_LOG_INFO << "Attribute::Attribute(): abs_change = " << abs_change[0] << " " << abs_change[1]
                               << std::endl;
            }
            else
            {
                Tango::detail::throw_err_data_type("abs_change", dev_name, name, "Attribute::init_event_prop()");
            }
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("abs_change", e);
    }

    //
    // Init min and max relative change for archive event
    //

    try
    {
        std::string archive_rel_change_str;
        bool archive_rel_change_defined = false;
        try
        {
            archive_rel_change_str = get_attr_value(prop_list, "archive_rel_change");
            archive_rel_change_defined = true;
        }
        catch(...)
        {
        }

        if(archive_rel_change_defined)
        {
            if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE))
            {
                std::vector<double> archive_rel_change_tmp;
                std::vector<bool> archive_rel_change_set_usr_def;
                std::vector<bool> unused;
                validate_change_properties(dev_name,
                                           "archive_rel_change",
                                           archive_rel_change_str,
                                           archive_rel_change_tmp,
                                           archive_rel_change_set_usr_def,
                                           unused);
                archive_rel_change[0] = archive_rel_change_tmp[0];
                archive_rel_change[1] = archive_rel_change_tmp[1];

                if(archive_rel_change_set_usr_def[0] || archive_rel_change_set_usr_def[1])
                {
                    if(nb_user != 0)
                    {
                        size_t i;
                        for(i = 0; i < nb_user; i++)
                        {
                            if(def_user_prop[i].get_name() == "archive_rel_change")
                            {
                                break;
                            }
                        }
                        if(i != nb_user)
                        {
                            std::vector<double> archive_rel_change_usr_def;
                            validate_change_properties(dev_name,
                                                       "archive_rel_change",
                                                       def_user_prop[i].get_value(),
                                                       archive_rel_change_usr_def);
                            if(archive_rel_change_set_usr_def[0])
                            {
                                archive_rel_change[0] = archive_rel_change_usr_def[0];
                            }
                            if(archive_rel_change_set_usr_def[1])
                            {
                                archive_rel_change[1] = archive_rel_change_usr_def[1];
                            }
                        }
                    }
                }
                TANGO_LOG_INFO << "Attribute::Attribute(): archive_rel_change = " << archive_rel_change[0] << " "
                               << archive_rel_change[1] << std::endl;
            }
            else
            {
                Tango::detail::throw_err_data_type(
                    "archive_rel_change", dev_name, name, "Attribute::init_event_prop()");
            }
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("archive_rel_change", e);
    }

    //
    // Init min and max absolute change for archive event
    //

    try
    {
        std::string archive_abs_change_str;
        bool archive_abs_change_defined = false;
        try
        {
            archive_abs_change_str = get_attr_value(prop_list, "archive_abs_change");
            archive_abs_change_defined = true;
        }
        catch(...)
        {
        }

        if(archive_abs_change_defined)
        {
            if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE))
            {
                std::vector<double> archive_abs_change_tmp;
                std::vector<bool> archive_abs_change_set_usr_def;
                std::vector<bool> unused;
                validate_change_properties(dev_name,
                                           "archive_abs_change",
                                           archive_abs_change_str,
                                           archive_abs_change_tmp,
                                           archive_abs_change_set_usr_def,
                                           unused);
                archive_abs_change[0] = archive_abs_change_tmp[0];
                archive_abs_change[1] = archive_abs_change_tmp[1];

                if(archive_abs_change_set_usr_def[0] || archive_abs_change_set_usr_def[1])
                {
                    if(nb_user != 0)
                    {
                        size_t i;
                        for(i = 0; i < nb_user; i++)
                        {
                            if(def_user_prop[i].get_name() == "archive_abs_change")
                            {
                                break;
                            }
                        }
                        if(i != nb_user)
                        {
                            std::vector<double> archive_abs_change_usr_def;
                            validate_change_properties(dev_name,
                                                       "archive_abs_change",
                                                       def_user_prop[i].get_value(),
                                                       archive_abs_change_usr_def);
                            if(archive_abs_change_set_usr_def[0])
                            {
                                archive_abs_change[0] = archive_abs_change_usr_def[0];
                            }
                            if(archive_abs_change_set_usr_def[1])
                            {
                                archive_abs_change[1] = archive_abs_change_usr_def[1];
                            }
                        }
                    }
                }
                TANGO_LOG_INFO << "Attribute::Attribute(): archive_abs_change = " << archive_abs_change[0] << " "
                               << archive_abs_change[1] << std::endl;
            }
            else
            {
                Tango::detail::throw_err_data_type(
                    "archive_abs_change", dev_name, name, "Attribute::init_event_prop()");
            }
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("archive_abs_change", e);
    }

    //
    // Init period for periodic event
    //

    try
    {
        event_period = (int) (DEFAULT_EVENT_PERIOD); // default for event period is 1 second

        std::string event_period_str;
        bool event_period_defined = false;
        try
        {
            event_period_str = get_attr_value(prop_list, "event_period");
            event_period_defined = true;
        }
        catch(...)
        {
        }

        if(event_period_defined)
        {
            TangoSys_MemStream str;
            int event_period_tmp = 0;
            str << event_period_str;
            if(str >> event_period_tmp && str.eof())
            {
                if(event_period_tmp > 0)
                {
                    event_period = event_period_tmp;
                }
                TANGO_LOG_INFO << "Attribute::Attribute(): event_period_str " << event_period_str
                               << " event_period = " << event_period << std::endl;
            }
            else
            {
                Tango::detail::throw_err_format("event_period", dev_name, name, "Attribute::init_event_prop()");
            }
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("event_period", e);
    }

    //
    // Init period for archive event
    //

    try
    {
        archive_period = INT_MAX;

        std::string archive_period_str;
        bool archive_period_defined = false;
        try
        {
            archive_period_str = get_attr_value(prop_list, "archive_period");
            archive_period_defined = true;
        }
        catch(...)
        {
        }

        if(archive_period_defined)
        {
            TangoSys_MemStream str;
            int archive_period_tmp = 0;
            str << archive_period_str;
            if(str >> archive_period_tmp && str.eof())
            {
                if(archive_period_tmp > 0)
                {
                    archive_period = archive_period_tmp;
                }
                TANGO_LOG_INFO << "Attribute::Attribute(): archive_period_str " << archive_period_str
                               << " archive_period = " << archive_period << std::endl;
            }
            else
            {
                Tango::detail::throw_err_format("archive_period", dev_name, name, "Attribute::init_event_prop()");
            }
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("archive_period", e);
    }

    //
    // Init remaining parameters
    //

    periodic_counter = 0;
    archive_periodic_counter = 0;
    last_periodic = {};
    archive_last_periodic = {};

    prev_change_event.inited = false;
    prev_change_event.err = false;
    prev_change_event.quality = Tango::ATTR_VALID;

    prev_alarm_event.inited = false;
    prev_alarm_event.err = false;
    prev_alarm_event.quality = Tango::ATTR_VALID;

    prev_archive_event.inited = false;
    prev_archive_event.err = false;
    prev_archive_event.quality = Tango::ATTR_VALID;

    //
    // do not start sending events automatically, wait for the first client to subscribe. Sending events automatically
    // will put an unnecessary load on the server because all attributes will be polled
    //

    event_change3_subscription = 0;
    event_change4_subscription = 0;
    event_change5_subscription = 0;

    event_alarm6_subscription = 0;

    event_archive3_subscription = 0;
    event_archive4_subscription = 0;
    event_archive5_subscription = 0;

    event_periodic3_subscription = 0;
    event_periodic4_subscription = 0;
    event_periodic5_subscription = 0;

    event_user3_subscription = 0;
    event_user4_subscription = 0;
    event_user5_subscription = 0;

    event_attr_conf_subscription = 0;
    event_attr_conf5_subscription = 0;

    event_data_ready_subscription = 0;

    //
    // Init client lib parameters
    //

    for(int i = 0; i < numEventType; i++)
    {
        client_lib[i].clear();
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::init_check_val_prop
//
// description :
//
//
// argument :
//         in :
//            - prop_list : The property vector
//            - dev_name : The device name (usefull for error)
//          - prop_name : Property name
//          - prop_comp : The property object used for checking if min > max
//      out :
//          - prop_str : property value as a string
//          - prop : The property object
//
//-------------------------------------------------------------------------------------------------------------------

bool Attribute::init_check_val_prop(std::vector<AttrProperty> &prop_list,
                                    const std::string &dev_name,
                                    const char *prop_name,
                                    std::string &prop_str,
                                    Tango::Attr_CheckVal &prop,
                                    const Tango::Attr_CheckVal &prop_comp)
{
    prop_str = get_attr_value(prop_list, prop_name);

    if(prop_str == AlrmValueNotSpec)
    {
        return false;
    }

    TangoSys_MemStream str;
    bool is_err_format = false;
    bool is_incoherent_val_err = false;

    bool min_prop = true;
    if(prop_name[1] == 'a')
    {
        min_prop = false;
    }

    str << prop_str;

    switch(data_type)
    {
    case Tango::DEV_SHORT:
        is_err_format = !(str >> prop.sh && str.eof());
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.sh >= prop_comp.sh)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.sh >= prop.sh)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_LONG:
        is_err_format = !(str >> prop.db && str.eof());
        prop.lg = (DevLong) prop.db;
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.lg >= prop_comp.lg)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.lg >= prop.lg)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_LONG64:
        is_err_format = !(str >> prop.db && str.eof());
        prop.lg64 = (DevLong64) prop.db;
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.lg64 >= prop_comp.lg64)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.lg64 >= prop.lg64)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_DOUBLE:
        is_err_format = !(str >> prop.db && str.eof());
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.db >= prop_comp.db)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.db >= prop.db)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_FLOAT:
        is_err_format = !(str >> prop.fl && str.eof());
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.fl >= prop_comp.fl)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.fl >= prop.fl)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_USHORT:
        is_err_format = !(str >> prop.ush && str.eof());
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.ush >= prop_comp.ush)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.ush >= prop.ush)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_UCHAR:
        is_err_format = !(str >> prop.sh && str.eof());
        prop.uch = (DevUChar) prop.sh;
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.uch >= prop_comp.uch)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.uch >= prop.uch)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_ULONG:
        is_err_format = !(str >> prop.db && str.eof());
        prop.ulg = (DevULong) prop.db;
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.ulg >= prop_comp.ulg)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.ulg >= prop.ulg)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_ULONG64:
        is_err_format = !(str >> prop.db && str.eof());
        prop.ulg64 = (DevULong64) prop.db;
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.ulg64 >= prop_comp.ulg64)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.ulg64 >= prop.ulg64)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    case Tango::DEV_ENCODED:
        is_err_format = !(str >> prop.sh && str.eof());
        prop.uch = (DevUChar) prop.sh;
        if(!is_err_format)
        {
            if(is_value_set(prop_name))
            {
                if(min_prop)
                {
                    if(prop.uch >= prop_comp.uch)
                    {
                        is_incoherent_val_err = true;
                    }
                }
                else
                {
                    if(prop_comp.uch >= prop.uch)
                    {
                        is_incoherent_val_err = true;
                    }
                }
            }
        }
        break;

    default:
        Tango::detail::throw_err_data_type(prop_name, dev_name, name, "Attribute::init_opt_prop()");
    }

    if(is_err_format)
    {
        Tango::detail::throw_err_format(prop_name, dev_name, name, "Attribute::init_opt_prop()");
    }
    else if(is_incoherent_val_err)
    {
        std::string opp_prop_name(prop_name);
        opp_prop_name[1] = (opp_prop_name[1] == 'a') ? ('i') : ('a'); // max_... ->  mix_... || min_... ->  man_...
        opp_prop_name[2] = (opp_prop_name[2] == 'x') ? ('n') : ('x'); // mix_... ->  min_... || man_... ->  max_...

        if(prop_name[1] == 'a')
        {
            Tango::detail::throw_incoherent_val_err(
                opp_prop_name.c_str(), prop_name, dev_name, name, "Attribute::init_opt_prop()");
        }
        else
        {
            Tango::detail::throw_incoherent_val_err(
                prop_name, opp_prop_name.c_str(), dev_name, name, "Attribute::init_opt_prop()");
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::init_opt_prop
//
// description :
//        Init the optional properties
//
// argument :
//         in :
//            - prop_list : The property vector
//            - dev_name : The device name (usefull for error)
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::init_opt_prop(std::vector<AttrProperty> &prop_list, const std::string &dev_name)
{
    //
    // Init the properties
    //

    init_string_prop(prop_list, label, "label");
    init_string_prop(prop_list, description, "description");
    init_string_prop(prop_list, unit, "unit");
    init_string_prop(prop_list, standard_unit, "standard_unit");
    init_string_prop(prop_list, display_unit, "display_unit");
    init_string_prop(prop_list, format, "format");

    //
    // Init the min alarm property
    //

    try
    {
        if(init_check_val_prop(prop_list, dev_name, "min_alarm", min_alarm_str, min_alarm, max_alarm))
        {
            alarm_conf.set(min_level);
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("min_alarm", e);
    }

    //
    // Init the max alarm property
    //

    try
    {
        if(init_check_val_prop(prop_list, dev_name, "max_alarm", max_alarm_str, max_alarm, min_alarm))
        {
            alarm_conf.set(max_level);
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("max_alarm", e);
    }

    //
    // Init the min value property
    //

    try
    {
        if(init_check_val_prop(prop_list, dev_name, "min_value", min_value_str, min_value, max_value))
        {
            check_min_value = true;
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("min_value", e);
    }

    //
    // Init the max value property
    //

    try
    {
        if(init_check_val_prop(prop_list, dev_name, "max_value", max_value_str, max_value, min_value))
        {
            check_max_value = true;
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("max_value", e);
    }

    //
    // Init the min warning property
    //

    try
    {
        if(init_check_val_prop(prop_list, dev_name, "min_warning", min_warning_str, min_warning, max_warning))
        {
            alarm_conf.set(min_warn);
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("min_warning", e);
    }

    //
    // Init the max warning property
    //

    try
    {
        if(init_check_val_prop(prop_list, dev_name, "max_warning", max_warning_str, max_warning, min_warning))
        {
            alarm_conf.set(max_warn);
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("max_warning", e);
    }

    //
    // Get delta_t property
    //

    bool delta_t_defined = false;
    try
    {
        delta_t_str = get_attr_value(prop_list, "delta_t");
        if(delta_t_str != "0")
        {
            if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) && (data_type != Tango::DEV_STATE))
            {
                delta_t = get_lg_attr_value(prop_list, "delta_t");
                if(delta_t != 0)
                {
                    delta_t_defined = true;
                }
            }
            else
            {
                Tango::detail::throw_err_data_type("delta_t", dev_name, name, "Attribute::init_opt_prop()");
            }
        }
        else
        {
            delta_t = 0;
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("delta_t", e);
    }

    //
    // Get delta_val property
    //

    bool delta_val_defined = false;
    try
    {
        delta_val_str = get_attr_value(prop_list, "delta_val");
        if(delta_val_str != AlrmValueNotSpec)
        {
            if((data_type != Tango::DEV_STRING) && (data_type != Tango::DEV_BOOLEAN) &&
               (data_type != Tango::DEV_STATE) && (data_type != Tango::DEV_ENUM))
            {
                TangoSys_MemStream str;
                str << delta_val_str;

                switch(data_type)
                {
                case Tango::DEV_SHORT:
                    if(!(str >> delta_val.sh && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    break;

                case Tango::DEV_LONG:
                    if(!(str >> delta_val.db && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    delta_val.lg = (DevLong) delta_val.db;
                    break;

                case Tango::DEV_LONG64:
                    if(!(str >> delta_val.db && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    delta_val.lg64 = (DevLong64) delta_val.db;
                    break;

                case Tango::DEV_DOUBLE:
                    if(!(str >> delta_val.db && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    break;

                case Tango::DEV_FLOAT:
                    if(!(str >> delta_val.fl && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    break;

                case Tango::DEV_USHORT:
                    if(!(str >> delta_val.ush && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    break;

                case Tango::DEV_UCHAR:
                    if(!(str >> delta_val.sh && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    delta_val.uch = (DevUChar) delta_val.sh;
                    break;

                case Tango::DEV_ULONG:
                    if(!(str >> delta_val.db && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    delta_val.ulg = (DevULong) delta_val.db;
                    break;

                case Tango::DEV_ULONG64:
                    if(!(str >> delta_val.db && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    delta_val.ulg64 = (DevULong64) delta_val.db;
                    break;

                case Tango::DEV_ENCODED:
                    if(!(str >> delta_val.sh && str.eof()))
                    {
                        Tango::detail::throw_err_format("delta_val", dev_name, name, "Attribute::init_opt_prop()");
                    }
                    delta_val.uch = (DevUChar) delta_val.sh;
                    break;
                }
                if(delta_t_defined)
                {
                    alarm_conf.set(rds); // set RDS flag only if both delta_t and delta_val are set
                }
                delta_val_defined = true;
            }
            else
            {
                Tango::detail::throw_err_data_type("delta_val", dev_name, name, "Attribute::init_opt_prop()");
            }
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("delta_val", e);
    }

    //
    // Throw exception if only one RDS property is defined
    //

    try
    {
        if(((delta_t_defined) && (!delta_val_defined)) || ((!delta_t_defined) && (delta_val_defined)))
        {
            TangoSys_OMemStream o;

            o << "RDS alarm properties (delta_t and delta_val) are not correctly defined for attribute " << name;
            o << " in device " << dev_name << std::ends;
            TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
        }
    }
    catch(DevFailed &e)
    {
        add_startup_exception("rds_alarm", e);
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::init_enum_prop
//
// description :
//        Get the enumeration labels from the property vector
//
// argument :
//         in :
//            - prop_list : The property vector
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::init_enum_prop(std::vector<AttrProperty> &prop_list)
{
    try
    {
        std::string tmp_enum_label = get_attr_value(prop_list, "enum_labels");
        build_check_enum_labels(tmp_enum_label);
    }
    catch(DevFailed &e)
    {
        std::string desc(e.errors[0].desc.in());
        if(desc.find("Property enum_labels is missing") != std::string::npos)
        {
            std::stringstream ss;
            ss << "The attribute " << name
               << " has the DEV_ENUM data type but there is no enumeration label(s) defined";

            e.errors[0].desc = Tango::string_dup(ss.str().c_str());
        }
        add_startup_exception("enum_labels", e);
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::build_check_enum_labels
//
// description :
//        Create vector of enum labels and  check that the same label is not used several times
//
// argument :
//         in :
//            - labs : The enum labels (labels1,label2,label3,...)
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::build_check_enum_labels(const std::string &labs)
{
    std::string::size_type pos = 0;
    std::string::size_type start = 0;
    bool exit = false;

    enum_labels.clear();

    //
    // Build vector with enum labels
    //

    while(!exit)
    {
        pos = labs.find(',', start);
        if(pos == std::string::npos)
        {
            std::string tmp = labs.substr(start);
            enum_labels.push_back(tmp);
            exit = true;
        }
        else
        {
            std::string tmp = labs.substr(start, pos - start);
            enum_labels.push_back(tmp);
            start = pos + 1;
        }
    }

    //
    // Check that all labels are different
    //

    std::vector<std::string> v_s = enum_labels;
    sort(v_s.begin(), v_s.end());
    for(size_t loop = 1; loop < v_s.size(); loop++)
    {
        if(v_s[loop - 1] == v_s[loop])
        {
            std::stringstream ss;
            ss << "Enumeration for attribute " << name << " has two similar labels (";
            ss << v_s[loop - 1] << ", " << v_s[loop] << ")";

            TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
        }
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::add_startup_exception
//
// description :
//        Stores an exception raised during the device startup sequence in a map
//
// argument :
//         in :
//            - prop_name : The property name for which the exception was raised
//            - except : The raised exception
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::add_startup_exception(std::string prop_name, const DevFailed &except)
{
    startup_exceptions.insert(std::pair<std::string, DevFailed>(prop_name, except));
    check_startup_exceptions = true;
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::throw_min_max_value
//
// description :     Throw a Tango DevFailed exception when an error on
//                    min/max value is detected
//
// in :    dev_name : The device name
//      memorized_value : The attribute memorized value
//      check_type : The type of check which was done (min_value or max_value)
//
//--------------------------------------------------------------------------

void Attribute::throw_min_max_value(const std::string &dev_name,
                                    const std::string &memorized_value,
                                    MinMaxValueCheck check_type)
{
    TangoSys_OMemStream o;

    o << "Device " << dev_name << "-> Attribute : " << name;
    o << "\nThis attribute is memorized and the memorized value (" << memorized_value << ") is ";
    if(check_type == MIN)
    {
        o << "below";
    }
    else
    {
        o << "above";
    }
    o << " the new limit!!" << std::ends;
    TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::is_polled
//
// description :     Check if the attribute polled
//
// This method returns a boolean set to true if the attribute is polled
//
//--------------------------------------------------------------------------

bool Attribute::is_polled()
{
    Tango::Util *tg = Util::instance();
    if(dev == nullptr)
    {
        dev = tg->get_device_by_name(d_name);
    }

    const std::string &att_name = get_name_lower();

    std::vector<std::string> &attr_list = dev->get_polled_attr();

    for(unsigned int i = 0; i < attr_list.size(); i = i + 2)
    {
        //
        //    Convert to lower case before comparison
        //

        std::string name_lowercase(attr_list[i]);
        std::transform(name_lowercase.begin(), name_lowercase.end(), name_lowercase.begin(), ::tolower);
        if(att_name == name_lowercase)
        {
            //
            // when the polling buffer is externally filled (polling period == 0)
            // mark the attribute as not polled! No events can be send by the polling thread!
            //

            return attr_list[i + 1] != "0";
        }
    }

    //
    // now check wether a polling period is set ( for example by pogo)
    //

    if(get_polling_period() > 0)
    {
        //
        // check the list of non_auto_polled attributes to verify wether
        // the polling was disabled
        //

        std::vector<std::string> &napa = dev->get_non_auto_polled_attr();
        for(unsigned int j = 0; j < napa.size(); j++)
        {
            if(TG_strcasecmp(napa[j].c_str(), att_name.c_str()) == 0)
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool Attribute::is_polled(DeviceImpl *the_dev)
{
    if((the_dev != nullptr) && (dev == nullptr))
    {
        dev = the_dev;
    }

    return is_polled();
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::is_writ_associated
//
// description :
//        Check if the attribute has an associated writable attribute
//
// returns:
//        This method returns a boolean set to true if the atribute has an associatied writable attribute
//
//-------------------------------------------------------------------------------------------------------------------

bool Attribute::is_writ_associated()
{
    return writable_attr_name != AssocWritNotSpec;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::get_attr_value
//
// description :
//        Retrieve a property value as a string from the vector of properties
//
// arguments :
//         in :
//            - prop_list : The property vector
//            - prop_name : the property name
//
//-------------------------------------------------------------------------------------------------------------------

std::string &Attribute::get_attr_value(std::vector<AttrProperty> &prop_list, const char *prop_name)
{
    auto pos = find_property(prop_list, prop_name);

    if(pos == prop_list.end())
    {
        TangoSys_OMemStream o;
        o << "Property " << prop_name << " is missing for attribute " << name << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    return pos->get_value();
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::get_lg_attr_value
//
// description :
//        Retrieve a property value as a long from the vector of properties
//
// argument :
//         in :
//            - prop_list : The property vector
//            - prop_name : the property name
//
//------------------------------------------------------------------------------------------------------------------

long Attribute::get_lg_attr_value(std::vector<AttrProperty> &prop_list, const char *prop_name)
{
    auto pos = find_property(prop_list, prop_name);

    if(pos == prop_list.end())
    {
        TangoSys_OMemStream o;
        o << "Property " << prop_name << " is missing for attribute " << name << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
    }

    pos->convert(prop_name);
    return pos->get_lg_value();
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::set_data_size
//
// description :
//        Compute the attribute amount of data
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::set_data_size()
{
    data_size = Tango::detail::compute_data_size(data_format, dim_x, dim_y);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::set_time
//
// description :
//        Set the date if the date flag is true
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::set_time()
{
    if(date)
    {
        when = make_TimeVal(std::chrono::system_clock::now());
    }
}

//+-------------------------------------------------------------------------
//
// method :        Attribute::check_alarm
//
// description :    Check if the attribute is in alarm
//
// This method returns a boolean set to true if the atribute is in alarm. In
// this case, it also set the attribute quality factor to ALARM
//
//--------------------------------------------------------------------------

bool Attribute::check_alarm()
{
    bool returned = quality == Tango::ATTR_ALARM || quality == Tango::ATTR_WARNING;

    //
    // Throw exception if no alarm is defined for this attribute
    //

    if(is_alarmed().none())
    {
        TangoSys_OMemStream o;

        o << "No alarm defined for attribute " << name << std::ends;
        TANGO_THROW_EXCEPTION(API_AttrNoAlarm, o.str());
    }

    //
    // If the attribute quality is different than VALID don`t do any checking to avoid
    // to override a user positioned quality value.
    // If no alarms levels are specified, just return without alarm.
    //

    if(!is_fwd_att() && quality != Tango::ATTR_VALID)
    {
        log_quality();
        return returned;
    }

    //
    // Get the monitor protecting device att config
    //

    TangoMonitor &mon1 = get_att_device()->get_att_conf_monitor();
    AutoTangoMonitor sync1(&mon1);

    //
    // If some alarm are defined on level, check attribute level
    //

    std::bitset<numFlags> &bs = is_alarmed();

    if((bs.test(Attribute::min_level)) || (bs.test(Attribute::max_level)))
    {
        if(check_level_alarm())
        {
            returned = true;
        }
    }

    if(!returned)
    {
        if((bs.test(Attribute::min_warn)) || (bs.test(Attribute::max_warn)))
        {
            if(check_warn_alarm())
            {
                returned = true;
            }
        }
    }

    //
    // If the RDS alarm is set, check attribute level
    //

    if((bs.test(Attribute::rds)))
    {
        if(check_rds_alarm())
        {
            returned = true;
        }
    }

    log_quality();

    return returned;
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::general_check_alarm
//
// description :   These methods check if the attribute is out of alarm/warning level
//                 and return a boolean set to true if the attribute out of the thresholds,
//                 and it also set the attribute quality factor to ALARM/WARNING
//
// in :     alarm_type: type of alarm to check
//          T max_value: max level threshold
//          T min_value: min level threshold
//
// out :    bool: true if value out of the limits
//
//--------------------------------------------------------------------------

template <typename T>
bool Attribute::general_check_alarm(const Tango::AttrQuality &alarm_type, const T &min_value, const T &max_value)
{
    alarm_flags min;
    alarm_flags max;

    switch(alarm_type)
    {
    case Tango::ATTR_ALARM:
    {
        min = min_level;
        max = max_level;
        break;
    }
    case Tango::ATTR_WARNING:
    {
        min = min_warn;
        max = max_warn;
        break;
    }
    default:
    {
        TANGO_THROW_EXCEPTION(API_InternalError, "Unknown alarm_type");
    }
    }

    bool real_returned = false;

    using ArrayType = typename tango_type_traits<T>::ArrayType;
    ArrayType *storage = get_value_storage<ArrayType>();

    if(alarm_conf.test(min))
    {
        for(std::uint32_t i = 0; i < data_size; i++)
        {
            if((*storage)[i] <= min_value)
            {
                quality = alarm_type;
                alarm.set(min);
                real_returned = true;
                break;
            }
        }
    }

    if(alarm_conf.test(max))
    {
        for(std::uint32_t i = 0; i < data_size; i++)
        {
            if((*storage)[i] >= max_value)
            {
                quality = alarm_type;
                alarm.set(max);
                real_returned = true;
                break;
            }
        }
    }

    return real_returned;
}

// unfortunately DevEncoded is special and cannot be templated
bool Attribute::general_check_devencoded_alarm(const Tango::AttrQuality &alarm_type,
                                               const unsigned char &min_value,
                                               const unsigned char &max_value)
{
    alarm_flags min;
    alarm_flags max;

    switch(alarm_type)
    {
    case Tango::ATTR_ALARM:
    {
        min = min_level;
        max = max_level;
        break;
    }
    case Tango::ATTR_WARNING:
    {
        min = min_level;
        max = max_level;
        break;
    }
    default:
    {
        TANGO_THROW_EXCEPTION(API_InternalError, "Unknown alarm_type");
    }
    }

    bool real_returned = false;

    const Tango::DevEncoded &value = (*attribute_value.get<Tango::DevVarEncodedArray>())[0];

    if(alarm_conf.test(min))
    {
        for(unsigned int i = 0; i < value.encoded_data.length(); i++)
        {
            if(value.encoded_data[i] <= min_value)
            {
                quality = alarm_type;
                alarm.set(min);
                real_returned = true;
                break;
            }
        }
    }

    if(alarm_conf.test(max))
    {
        for(unsigned int i = 0; i < value.encoded_data.length(); i++)
        {
            if(value.encoded_data[i] >= max_value)
            {
                quality = alarm_type;
                alarm.set(max);
                real_returned = true;
                break;
            }
        }
    }

    return real_returned;
}

//+-------------------------------------------------------------------------
//
// method :        Attribute::check_level_alarm
//
// description :    Check if the attribute is in alarm level
//
// This method returns a boolean set to true if the attribute is in alarm. In
// this case, it also set the attribute quality factor to ALARM
//
//--------------------------------------------------------------------------

bool Attribute::check_level_alarm()
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
        return general_check_alarm<Tango::DevShort>(Tango::ATTR_ALARM, min_alarm.sh, max_alarm.sh);

    case Tango::DEV_LONG:
        return general_check_alarm<Tango::DevLong>(Tango::ATTR_ALARM, min_alarm.lg, max_alarm.lg);

    case Tango::DEV_LONG64:
        return general_check_alarm<Tango::DevLong64>(Tango::ATTR_ALARM, min_alarm.lg64, max_alarm.lg64);

    case Tango::DEV_DOUBLE:
        return general_check_alarm<Tango::DevDouble>(Tango::ATTR_ALARM, min_alarm.db, max_alarm.db);

    case Tango::DEV_FLOAT:
        return general_check_alarm<Tango::DevFloat>(Tango::ATTR_ALARM, min_alarm.fl, max_alarm.fl);

    case Tango::DEV_USHORT:
        return general_check_alarm<Tango::DevUShort>(Tango::ATTR_ALARM, min_alarm.ush, max_alarm.ush);

    case Tango::DEV_UCHAR:
        return general_check_alarm<Tango::DevUChar>(Tango::ATTR_ALARM, min_alarm.uch, max_alarm.uch);

    case Tango::DEV_ULONG64:
        return general_check_alarm<Tango::DevULong64>(Tango::ATTR_ALARM, min_alarm.ulg64, max_alarm.ulg64);

    case Tango::DEV_ULONG:
        return general_check_alarm<Tango::DevULong>(Tango::ATTR_ALARM, min_alarm.ulg, max_alarm.ulg);

    case Tango::DEV_ENCODED:
        return general_check_devencoded_alarm(Tango::ATTR_ALARM, min_alarm.uch, max_alarm.uch);
    }

    return false;
}

//+-------------------------------------------------------------------------
//
// method :        Attribute::check_warn_alarm
//
// description :    Check if the attribute is in warning alarm
//
// This method returns a boolean set to true if the attribute is in alarm. In
// this case, it also set the attribute quality factor to WARNING
//
//--------------------------------------------------------------------------

bool Attribute::check_warn_alarm()
{
    switch(data_type)
    {
    case Tango::DEV_SHORT:
        return general_check_alarm<Tango::DevShort>(Tango::ATTR_WARNING, min_warning.sh, max_warning.sh);

    case Tango::DEV_LONG:
        return general_check_alarm<Tango::DevLong>(Tango::ATTR_WARNING, min_warning.lg, max_warning.lg);

    case Tango::DEV_LONG64:
        return general_check_alarm<Tango::DevLong64>(Tango::ATTR_WARNING, min_warning.lg64, max_warning.lg64);

    case Tango::DEV_DOUBLE:
        return general_check_alarm<Tango::DevDouble>(Tango::ATTR_WARNING, min_warning.db, max_warning.db);

    case Tango::DEV_FLOAT:
        return general_check_alarm<Tango::DevFloat>(Tango::ATTR_WARNING, min_warning.fl, max_warning.fl);

    case Tango::DEV_USHORT:
        return general_check_alarm<Tango::DevUShort>(Tango::ATTR_WARNING, min_warning.ush, max_warning.ush);

    case Tango::DEV_UCHAR:
        return general_check_alarm<Tango::DevUChar>(Tango::ATTR_WARNING, min_warning.uch, max_warning.uch);

    case Tango::DEV_ULONG64:
        return general_check_alarm<Tango::DevULong64>(Tango::ATTR_WARNING, min_warning.ulg64, max_warning.ulg64);

    case Tango::DEV_ULONG:
        return general_check_alarm<Tango::DevULong>(Tango::ATTR_WARNING, min_warning.ulg, max_warning.ulg);

    case Tango::DEV_ENCODED:
        return general_check_devencoded_alarm(Tango::ATTR_WARNING, min_warning.uch, max_warning.uch);
    }

    return false;
}

//+-------------------------------------------------------------------------
//
// method :        Attribute::delete_seq_and_reset_alarm
//
// description :    Delete the sequence created to store attribute
//            value and reset any alarms that may have been calculated by
//            set_alarm.
//
//--------------------------------------------------------------------------

void Attribute::delete_seq_and_reset_alarm()
{
    TANGO_LOG_DEBUG << "Attribute::delete_seq_and_reset_alarm() called " << std::endl;
    delete_seq();
    quality = Tango::ATTR_VALID;
    alarm.reset();
}

//+-------------------------------------------------------------------------
//
// method :        Attribute::delete_seq
//
// description :    Delete the sequence created to store attribute
//            value.
//            In case of a read_attributes CORBA operation,
//            this delete will be done automatically because the
//            sequence is inserted in an CORBA::Any object which will
//            delete the sequence when it is destroyed.
//            This method is usefull only to delete the sequence
//            created in case of the attribute is read during a
//            DevState command to evaluate  device state
//
//--------------------------------------------------------------------------

void Attribute::delete_seq()
{
    TANGO_LOG_DEBUG << "Attribute::delete_seq() called " << std::endl;
    attribute_value.reset();
    data_size = 0;
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::add_write_value
//
// description :    These methods add the associated writable attribute
//                  value to the attribute value sequence
//
// in :    val : The associated write attribute value
//
//--------------------------------------------------------------------------

void Attribute::add_write_value(Tango::DevVarShortArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarLongArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarDoubleArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarStringArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarFloatArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarBooleanArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarUShortArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarCharArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarLong64Array *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarULongArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarULong64Array *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevVarStateArray *val_ptr)
{
    add_write_value_impl(val_ptr);
}

void Attribute::add_write_value(Tango::DevEncoded &val_ptr)
{
    add_write_value_impl(val_ptr);
}

template <class T>
void Attribute::add_write_value_impl(T *val_ptr)
{
    T *storage = get_value_storage<T>();

    long nb_read = storage->length();
    storage->length(nb_read + val_ptr->length());
    for(unsigned int k = 0; k < val_ptr->length(); k++)
    {
        (*storage)[nb_read + k] = (*val_ptr)[k];
    }
}

void Attribute::add_write_value_impl(Tango::DevVarStringArray *val_ptr)
{
    auto *storage = attribute_value.get<Tango::DevVarStringArray>();

    long nb_read = storage->length();
    storage->length(nb_read + val_ptr->length());
    for(unsigned int k = 0; k < val_ptr->length(); k++)
    {
        (*storage)[nb_read + k] = Tango::string_dup((*val_ptr)[k]);
    }
}

void Attribute::add_write_value_impl(Tango::DevEncoded &val_ref)
{
    auto *storage = attribute_value.get<Tango::DevVarEncodedArray>();

    storage->length(2);
    (*storage)[1].encoded_format = CORBA::string_dup(val_ref.encoded_format);
    (*storage)[1].encoded_data.replace(
        val_ref.encoded_data.length(), val_ref.encoded_data.length(), val_ref.encoded_data.get_buffer());
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::Attribute_2_AttributeValue
//
// description :
//        Build an AttributeValue_3 object from the Attribute object content
//
// arguments
//         in :
//            - dev : The device to which the attribute belongs to
//        out :
//            - ptr : Pointer to the AttributeValue_3 object to be filled in
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::Attribute_2_AttributeValue(Tango::AttributeValue_3 *ptr, Tango::DeviceImpl *d)
{
    if((name_lower == "state") || (name_lower == "status"))
    {
        ptr->quality = Tango::ATTR_VALID;
        CORBA::Any &a = ptr->value;

        if(name_lower == "state")
        {
            a <<= d->get_state();
        }
        else
        {
            Tango::DevVarStringArray str_seq(1);
            str_seq.length(1);
            str_seq[0] = Tango::string_dup(d->get_status().c_str());

            a <<= str_seq;
        }

        ptr->time = make_TimeVal(std::chrono::system_clock::now());
        ptr->r_dim.dim_x = 1;
        ptr->r_dim.dim_y = 0;
        ptr->w_dim.dim_x = 0;
        ptr->w_dim.dim_y = 0;

        ptr->name = Tango::string_dup(name.c_str());
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

            Tango::DevVarShortArray *sh_seq;
            Tango::DevShort *sh_tmp_ptr;
            Tango::DevVarLongArray *lo_seq;
            Tango::DevLong *lo_tmp_ptr;
            Tango::DevVarDoubleArray *db_seq;
            Tango::DevDouble *db_tmp_ptr;
            Tango::DevVarStringArray *str_seq;
            Tango::DevString *str_tmp_ptr;
            Tango::DevVarFloatArray *fl_seq;
            Tango::DevFloat *fl_tmp_ptr;
            Tango::DevVarBooleanArray *bo_seq;
            Tango::DevBoolean *bo_tmp_ptr;
            Tango::DevVarUShortArray *ush_seq;
            Tango::DevUShort *ush_tmp_ptr;
            Tango::DevVarUCharArray *uch_seq;
            Tango::DevUChar *uch_tmp_ptr;
            Tango::DevVarLong64Array *lo64_seq;
            Tango::DevLong64 *lo64_tmp_ptr;
            Tango::DevVarULongArray *ulo_seq;
            Tango::DevULong *ulo_tmp_ptr;
            Tango::DevVarULong64Array *ulo64_seq;
            Tango::DevULong64 *ulo64_tmp_ptr;
            Tango::DevVarStateArray *state_seq;
            Tango::DevState *state_tmp_ptr;

            CORBA::Any &a = ptr->value;
            long seq_length;

            switch(data_type)
            {
            case Tango::DEV_SHORT:
            case Tango::DEV_ENUM:
                sh_tmp_ptr = get_short_value()->get_buffer();
                seq_length = get_short_value()->length();
                sh_seq = new Tango::DevVarShortArray(seq_length, seq_length, sh_tmp_ptr, false);
                a <<= *sh_seq;
                delete sh_seq;
                break;

            case Tango::DEV_LONG:
                lo_tmp_ptr = get_long_value()->get_buffer();
                seq_length = get_long_value()->length();
                lo_seq = new Tango::DevVarLongArray(seq_length, seq_length, lo_tmp_ptr, false);
                a <<= *lo_seq;
                delete lo_seq;
                break;

            case Tango::DEV_LONG64:
                lo64_tmp_ptr = get_long64_value()->get_buffer();
                seq_length = get_long64_value()->length();
                lo64_seq = new Tango::DevVarLong64Array(seq_length, seq_length, lo64_tmp_ptr, false);
                a <<= *lo64_seq;
                delete lo64_seq;
                break;

            case Tango::DEV_DOUBLE:
                db_tmp_ptr = get_double_value()->get_buffer();
                seq_length = get_double_value()->length();
                db_seq = new Tango::DevVarDoubleArray(seq_length, seq_length, db_tmp_ptr, false);
                a <<= *db_seq;
                delete db_seq;
                break;

            case Tango::DEV_STRING:
                str_tmp_ptr = get_string_value()->get_buffer();
                seq_length = get_string_value()->length();
                str_seq = new Tango::DevVarStringArray(seq_length, seq_length, str_tmp_ptr, false);
                a <<= *str_seq;
                delete str_seq;
                break;

            case Tango::DEV_FLOAT:
                fl_tmp_ptr = get_float_value()->get_buffer();
                seq_length = get_float_value()->length();
                fl_seq = new Tango::DevVarFloatArray(seq_length, seq_length, fl_tmp_ptr, false);
                a <<= *fl_seq;
                delete fl_seq;
                break;

            case Tango::DEV_BOOLEAN:
                bo_tmp_ptr = get_boolean_value()->get_buffer();
                seq_length = get_boolean_value()->length();
                bo_seq = new Tango::DevVarBooleanArray(seq_length, seq_length, bo_tmp_ptr, false);
                a <<= *bo_seq;
                delete bo_seq;
                break;

            case Tango::DEV_USHORT:
                ush_tmp_ptr = get_ushort_value()->get_buffer();
                seq_length = get_ushort_value()->length();
                ush_seq = new Tango::DevVarUShortArray(seq_length, seq_length, ush_tmp_ptr, false);
                a <<= *ush_seq;
                delete ush_seq;
                break;

            case Tango::DEV_UCHAR:
                uch_tmp_ptr = get_uchar_value()->get_buffer();
                seq_length = get_uchar_value()->length();
                uch_seq = new Tango::DevVarUCharArray(seq_length, seq_length, uch_tmp_ptr, false);
                a <<= *uch_seq;
                delete uch_seq;
                break;

            case Tango::DEV_ULONG:
                ulo_tmp_ptr = get_ulong_value()->get_buffer();
                seq_length = get_ulong_value()->length();
                ulo_seq = new Tango::DevVarULongArray(seq_length, seq_length, ulo_tmp_ptr, false);
                a <<= *ulo_seq;
                delete ulo_seq;
                break;

            case Tango::DEV_ULONG64:
                ulo64_tmp_ptr = get_ulong64_value()->get_buffer();
                seq_length = get_ulong64_value()->length();
                ulo64_seq = new Tango::DevVarULong64Array(seq_length, seq_length, ulo64_tmp_ptr, false);
                a <<= *ulo64_seq;
                delete ulo64_seq;
                break;

            case Tango::DEV_STATE:
                state_tmp_ptr = get_state_value()->get_buffer();
                seq_length = get_state_value()->length();
                state_seq = new Tango::DevVarStateArray(seq_length, seq_length, state_tmp_ptr, false);
                a <<= *state_seq;
                delete state_seq;
                break;
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
        }

        ptr->time = when;
        ptr->quality = quality;
        ptr->name = Tango::string_dup(name.c_str());
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::Attribute_2_AttributeValue
//
// description :
//        Build an AttributeValue_4 object from the Attribute object content
//
// arguments
//         in :
//            - d : The device to which the attribute belongs to
//        out :
//            - ptr_4 : Pointer to the AttributeValue_5 object to be filled in
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::Attribute_2_AttributeValue(Tango::AttributeValue_4 *ptr_4, Tango::DeviceImpl *d)
{
    Attribute_2_AttributeValue_base(ptr_4, d);

    Tango::AttributeIdlData aid;
    Tango::AttributeValueList_4 dummy_list(1, 1, ptr_4, false);
    aid.data_4 = &dummy_list;

    if((name_lower != "state") && (name_lower != "status") && (quality != Tango::ATTR_INVALID))
    {
        d->data_into_net_object(*this, aid, 0, writable, false);
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::Attribute_2_AttributeValue
//
// description :
//        Build an AttributeValue_5 object from the Attribute object content
//
// arguments
//         in :
//            - d : The device to which the attribute belongs to
//        out :
//            - ptr_5 : Pointer to the AttributeValue_5 object to be filled in
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::Attribute_2_AttributeValue(Tango::AttributeValue_5 *ptr_5, Tango::DeviceImpl *d)
{
    Attribute_2_AttributeValue_base(ptr_5, d);

    Tango::AttributeIdlData aid;
    Tango::AttributeValueList_5 dummy_list(1, 1, ptr_5, false);
    aid.data_5 = &dummy_list;

    if((name_lower != "state") && (name_lower != "status") && (quality != Tango::ATTR_INVALID))
    {
        d->data_into_net_object(*this, aid, 0, writable, false);
    }

    ptr_5->data_type = data_type;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::AttributeValue_4_2_AttributeValue_3
//
// description :
//        Build an AttributeValue_3 object from the AttributeValue_4 object. This method is used in case an event is
//        requested by a client knowing only IDL release 3
//
// args :
//         in :
//            - ptr_4 : Pointer to the AttributeValue_4 object
//      out :
//            - ptr_3 : Pointer to the AttributeValue_3 object to be filled in
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::AttributeValue_4_2_AttributeValue_3(const Tango::AttributeValue_4 *ptr_4,
                                                    Tango::AttributeValue_3 *ptr_3)
{
    //
    // First copy the data
    //

    if(ptr_4->quality != Tango::ATTR_INVALID)
    {
        AttrValUnion_2_Any(ptr_4, ptr_3->value);
    }

    //
    // The remaining fields
    //

    ptr_3->time = ptr_4->time;
    ptr_3->quality = ptr_4->quality;
    ptr_3->name = ptr_4->name;

    ptr_3->r_dim = ptr_4->r_dim;
    ptr_3->w_dim = ptr_4->w_dim;

    ptr_3->err_list = ptr_4->err_list;
}

void Attribute::AttributeValue_5_2_AttributeValue_3(const Tango::AttributeValue_5 *att_5,
                                                    Tango::AttributeValue_3 *att_3)
{
    //
    // First copy the data
    //

    if(att_5->quality != Tango::ATTR_INVALID)
    {
        AttrValUnion_2_Any(att_5, att_3->value);
    }

    //
    // The remaining fields
    //

    att_3->time = att_5->time;
    att_3->quality = att_5->quality;
    att_3->name = att_5->name;

    att_3->r_dim = att_5->r_dim;
    att_3->w_dim = att_5->w_dim;

    att_3->err_list = att_5->err_list;
}

void Attribute::AttributeValue_3_2_AttributeValue_4(const Tango::AttributeValue_3 *att_3,
                                                    Tango::AttributeValue_4 *att_4)
{
    // TODO: Code data from Any to Union

    //
    // The remaining fields
    //

    att_4->time = att_3->time;
    att_4->quality = att_3->quality;
    att_4->name = att_3->name;
    att_4->data_format = data_format;

    att_4->r_dim = att_3->r_dim;
    att_4->w_dim = att_3->w_dim;

    att_4->err_list = att_3->err_list;
}

void Attribute::AttributeValue_5_2_AttributeValue_4(const Tango::AttributeValue_5 *att_5,
                                                    Tango::AttributeValue_4 *att_4)
{
    //
    // First pass the data from one union to another WITHOUT copying them
    //

    AttrValUnion_fake_copy(att_5, att_4);

    //
    // The remaining fields
    //

    att_4->time = att_5->time;
    att_4->quality = att_5->quality;
    att_4->name = att_5->name;
    att_4->data_format = att_5->data_format;

    att_4->r_dim = att_5->r_dim;
    att_4->w_dim = att_5->w_dim;

    att_4->err_list = att_5->err_list;
}

void Attribute::AttributeValue_3_2_AttributeValue_5(const Tango::AttributeValue_3 *att_3,
                                                    Tango::AttributeValue_5 *att_5)
{
    // TODO: Code data from Any to Union

    //
    // The remaining fields
    //

    att_5->time = att_3->time;
    att_5->quality = att_3->quality;
    att_5->name = att_3->name;
    att_5->data_format = data_format;
    att_5->data_type = data_type;

    att_5->r_dim = att_3->r_dim;
    att_5->w_dim = att_3->w_dim;

    att_5->err_list = att_3->err_list;
}

void Attribute::AttributeValue_4_2_AttributeValue_5(const Tango::AttributeValue_4 *att_4,
                                                    Tango::AttributeValue_5 *att_5)
{
    //
    // First pass the data from one union to another WITHOUT copying them
    //

    AttrValUnion_fake_copy(att_4, att_5);

    //
    // The remaining fields
    //

    att_5->time = att_4->time;
    att_5->quality = att_4->quality;
    att_5->name = att_4->name;
    att_5->data_format = att_4->data_format;
    att_5->data_type = data_type;

    att_5->r_dim = att_4->r_dim;
    att_5->w_dim = att_4->w_dim;

    att_5->err_list = att_4->err_list;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::AttributeConfig_5_2_AttributeConfig_3
//
// description :
//        Build an AttributeConfig_3 object from the AttributeConfig_5 object. This method is used in case an event is
//        requested by a client knowing only IDL release 4
//
// argument:
//         in :
//            - conf5 : Reference to the AttributeConfig_5 object
//        out :
//            - conf3 : Reference to the AttributeConfig_3 object to be filled in
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::AttributeConfig_5_2_AttributeConfig_3(const Tango::AttributeConfig_5 &conf5,
                                                      Tango::AttributeConfig_3 &conf3)
{
    size_t j;

    conf3.name = conf5.name;
    conf3.writable = conf5.writable;
    conf3.data_format = conf5.data_format;
    conf3.data_type = conf5.data_type;
    conf3.max_dim_x = conf5.max_dim_x;
    conf3.max_dim_y = conf5.max_dim_y;
    conf3.description = conf5.description;
    conf3.label = conf5.label;
    conf3.unit = conf5.unit;
    conf3.standard_unit = conf5.standard_unit;
    conf3.display_unit = conf5.display_unit;
    conf3.format = conf5.format;
    conf3.min_value = conf5.min_value;
    conf3.max_value = conf5.max_value;
    conf3.writable_attr_name = conf5.writable_attr_name;
    conf3.level = conf5.level;
    conf3.extensions.length(conf5.extensions.length());
    for(j = 0; j < conf5.extensions.length(); j++)
    {
        conf3.extensions[j] = string_dup(conf5.extensions[j]);
    }
    for(j = 0; j < conf5.sys_extensions.length(); j++)
    {
        conf3.sys_extensions[j] = string_dup(conf5.sys_extensions[j]);
    }

    conf3.att_alarm.min_alarm = conf5.att_alarm.min_alarm;
    conf3.att_alarm.max_alarm = conf5.att_alarm.max_alarm;
    conf3.att_alarm.min_warning = conf5.att_alarm.min_warning;
    conf3.att_alarm.max_warning = conf5.att_alarm.max_warning;
    conf3.att_alarm.delta_t = conf5.att_alarm.delta_t;
    conf3.att_alarm.delta_val = conf5.att_alarm.delta_val;
    for(j = 0; j < conf5.att_alarm.extensions.length(); j++)
    {
        conf3.att_alarm.extensions[j] = string_dup(conf5.att_alarm.extensions[j]);
    }

    conf3.event_prop.ch_event.rel_change = conf5.event_prop.ch_event.rel_change;
    conf3.event_prop.ch_event.abs_change = conf5.event_prop.ch_event.abs_change;
    for(j = 0; j < conf5.event_prop.ch_event.extensions.length(); j++)
    {
        conf3.event_prop.ch_event.extensions[j] = string_dup(conf5.event_prop.ch_event.extensions[j]);
    }

    conf3.event_prop.per_event.period = conf5.event_prop.per_event.period;
    for(j = 0; j < conf5.event_prop.per_event.extensions.length(); j++)
    {
        conf3.event_prop.per_event.extensions[j] = string_dup(conf5.event_prop.per_event.extensions[j]);
    }

    conf3.event_prop.arch_event.rel_change = conf5.event_prop.arch_event.rel_change;
    conf3.event_prop.arch_event.abs_change = conf5.event_prop.arch_event.abs_change;
    conf3.event_prop.arch_event.period = conf5.event_prop.arch_event.period;
    for(j = 0; j < conf5.event_prop.ch_event.extensions.length(); j++)
    {
        conf3.event_prop.arch_event.extensions[j] = string_dup(conf5.event_prop.arch_event.extensions[j]);
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::AttributeConfig_3_2_AttributeConfig_5
//
// description :
//        Build an AttributeConfig_3 object from the AttributeConfig_5 object. This method is used in case an event is
//        requested by a client knowing only IDL release 4
//
// argument:
//         in :
//            - conf3 : Reference to the AttributeConfig_3 object
//        out :
//            - conf5 : Reference to the AttributeConfig_5 object to be filled in
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::AttributeConfig_3_2_AttributeConfig_5(const Tango::AttributeConfig_3 &conf3,
                                                      Tango::AttributeConfig_5 &conf5)
{
    size_t j;

    conf5.name = conf3.name;
    conf5.writable = conf3.writable;
    conf5.data_format = conf3.data_format;
    conf5.data_type = conf3.data_type;
    conf5.max_dim_x = conf3.max_dim_x;
    conf5.max_dim_y = conf3.max_dim_y;
    conf5.description = conf3.description;
    conf5.label = conf3.label;
    conf5.unit = conf3.unit;
    conf5.standard_unit = conf3.standard_unit;
    conf5.display_unit = conf3.display_unit;
    conf5.format = conf3.format;
    conf5.min_value = conf3.min_value;
    conf5.max_value = conf3.max_value;
    conf5.writable_attr_name = conf3.writable_attr_name;
    conf5.level = conf3.level;
    conf5.extensions.length(conf3.extensions.length());
    for(j = 0; j < conf3.extensions.length(); j++)
    {
        conf5.extensions[j] = string_dup(conf3.extensions[j]);
    }
    for(j = 0; j < conf3.sys_extensions.length(); j++)
    {
        conf5.sys_extensions[j] = string_dup(conf3.sys_extensions[j]);
    }

    conf5.att_alarm.min_alarm = conf3.att_alarm.min_alarm;
    conf5.att_alarm.max_alarm = conf3.att_alarm.max_alarm;
    conf5.att_alarm.min_warning = conf3.att_alarm.min_warning;
    conf5.att_alarm.max_warning = conf3.att_alarm.max_warning;
    conf5.att_alarm.delta_t = conf3.att_alarm.delta_t;
    conf5.att_alarm.delta_val = conf3.att_alarm.delta_val;
    for(j = 0; j < conf3.att_alarm.extensions.length(); j++)
    {
        conf5.att_alarm.extensions[j] = string_dup(conf3.att_alarm.extensions[j]);
    }

    conf5.event_prop.ch_event.rel_change = conf3.event_prop.ch_event.rel_change;
    conf5.event_prop.ch_event.abs_change = conf3.event_prop.ch_event.abs_change;
    for(j = 0; j < conf3.event_prop.ch_event.extensions.length(); j++)
    {
        conf5.event_prop.ch_event.extensions[j] = string_dup(conf3.event_prop.ch_event.extensions[j]);
    }

    conf5.event_prop.per_event.period = conf3.event_prop.per_event.period;
    for(j = 0; j < conf3.event_prop.per_event.extensions.length(); j++)
    {
        conf5.event_prop.per_event.extensions[j] = string_dup(conf3.event_prop.per_event.extensions[j]);
    }

    conf5.event_prop.arch_event.rel_change = conf3.event_prop.arch_event.rel_change;
    conf5.event_prop.arch_event.abs_change = conf3.event_prop.arch_event.abs_change;
    conf5.event_prop.arch_event.period = conf3.event_prop.arch_event.period;
    for(j = 0; j < conf3.event_prop.ch_event.extensions.length(); j++)
    {
        conf5.event_prop.arch_event.extensions[j] = string_dup(conf3.event_prop.arch_event.extensions[j]);
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::generic_fire_event
//
// description :
//        Fire a requested event for the attribute value.
//
// arguments:
//         in :
//            - EventType: type of event to be fired
//            - ptr : Pointer to a DevFailed exception to fire in case of an error to indicate.
//
//---------------------------------------------------------------------------------------------------------------------
void Attribute::generic_fire_event(const EventType &event_type,
                                   DevFailed *except,
                                   bool should_delete_seq,
                                   std::vector<std::string> filterable_names,
                                   std::vector<double> filterable_data)
{
    TANGO_LOG_DEBUG << "Attribute::generic_fire_event() for " << EventName[event_type] << " entering ..." << std::endl;

    if(event_type == CHANGE_EVENT && !is_alarm_event() && Util::instance()->is_auto_alarm_on_change_event())
    {
        generic_fire_event(ALARM_EVENT, except, false);
    }

    if(except != nullptr)
    {
        reset_value();
    }

    bool must_have_data = (name_lower != "state") && (name_lower != "status") && (quality != Tango::ATTR_INVALID);

    bool must_clean_data = must_have_data && value_is_set() && should_delete_seq;

    //
    // Check if it is needed to send an event
    //

    Tango::AttributeValue_3 *send_attr = nullptr;
    Tango::AttributeValue_4 *send_attr_4 = nullptr;
    Tango::AttributeValue_5 *send_attr_5 = nullptr;

    try
    {
        time_t event3_subscription = 0, event4_subscription = 0, event5_subscription = 0, event6_subscription = 0;

        int oldest_supported_idl = 3;

        time_t now = Tango::get_current_system_datetime();

        {
            omni_mutex_lock oml(EventSupplier::get_event_mutex());
            switch(event_type)
            {
            case CHANGE_EVENT:
            {
                event3_subscription = now - event_change3_subscription;
                event4_subscription = now - event_change4_subscription;
                event5_subscription = now - event_change5_subscription;
                event6_subscription =
                    now - event_change5_subscription; // this is odd, but we do not have event_change6_subscription
                break;
            }
            case ARCHIVE_EVENT:
            {
                event3_subscription = now - event_archive3_subscription;
                event4_subscription = now - event_archive4_subscription;
                event5_subscription = now - event_archive5_subscription;
                event6_subscription =
                    now - event_archive5_subscription; // this is odd, but we do not have event_archive6_subscription
                break;
            }
            case USER_EVENT:
            {
                event3_subscription = now - event_user3_subscription;
                event4_subscription = now - event_user4_subscription;
                event5_subscription = now - event_user5_subscription;
                event6_subscription =
                    now - event_user5_subscription; // this is odd, but we do not have event_user6_subscription
                break;
            }
            case ALARM_EVENT:
            {
                event6_subscription = now - event_alarm6_subscription;
                oldest_supported_idl = 6;
                break;
            }
            default:
                TANGO_ASSERT_ON_DEFAULT(event_type);
            }
        }

        //
        // Get the event supplier(s)
        //

        EventSupplier *event_supplier_nd = nullptr;
        EventSupplier *event_supplier_zmq = nullptr;
        bool pub_socket_created = false;

        Tango::Util *tg = Util::instance();
        if(use_notifd_event())
        {
            event_supplier_nd = tg->get_notifd_event_supplier();
            pub_socket_created = true;
        }
        if(use_zmq_event())
        {
            event_supplier_zmq = tg->get_zmq_event_supplier();
        }

        //
        // Get client lib and if it's possible to send event (ZMQ socket created)
        //
        std::vector<int> client_libs;
        {
            omni_mutex_lock oml(EventSupplier::get_event_mutex());
            client_libs = get_client_lib(event_type); // We want a copy
            if(use_zmq_event() && event_supplier_zmq != nullptr)
            {
                std::string &sock_endpoint = static_cast<ZmqEventSupplier *>(event_supplier_zmq)->get_event_endpoint();
                pub_socket_created = !sock_endpoint.empty();
            }
        }

        std::vector<int>::iterator ite;
        for(ite = client_libs.begin(); ite != client_libs.end(); ++ite)
        {
            switch(*ite)
            {
            case 6:
                if(oldest_supported_idl >= 6 && event6_subscription >= EVENT_RESUBSCRIBE_PERIOD)
                {
                    remove_client_lib(6, std::string(EventName[event_type]));
                }
                break;

            case 5:
                if(oldest_supported_idl >= 5 && event5_subscription >= EVENT_RESUBSCRIBE_PERIOD)
                {
                    remove_client_lib(5, std::string(EventName[event_type]));
                }
                break;

            case 4:
                if(oldest_supported_idl >= 4 && event4_subscription >= EVENT_RESUBSCRIBE_PERIOD)
                {
                    remove_client_lib(4, std::string(EventName[event_type]));
                }
                break;

            default:
                if(oldest_supported_idl >= 3 && event3_subscription >= EVENT_RESUBSCRIBE_PERIOD)
                {
                    remove_client_lib(3, std::string(EventName[event_type]));
                }
                break;
            }
        }

        //
        // Simply return if event supplier(s) are not created or there is no clients
        //

        if(((event_supplier_nd == nullptr) && (event_supplier_zmq == nullptr)) || client_libs.empty())
        {
            if(must_clean_data)
            {
                delete_seq_and_reset_alarm();
            }
            return;
        }

        //
        // Retrieve device object if not already done
        //

        if(dev == nullptr)
        {
            dev = tg->get_device_by_name(d_name);
        }

        if(except == nullptr && must_have_data && !value_is_set())
        {
            TangoSys_OMemStream o;

            o << "Value for attribute ";
            o << name;
            o << " has not been updated. Can't send " << EventName[event_type] << " event\n";
            o << "Set the attribute value (using set_value(...) method) before!" << std::ends;

            TANGO_THROW_EXCEPTION(API_AttrValueNotSet, o.str());
        }

        //
        // Build one AttributeValue_3, AttributeValue_4 or AttributeValue_5 object
        //

        long vers = dev->get_dev_idl_version();
        try
        {
            if(vers >= 5)
            {
                send_attr_5 = new Tango::AttributeValue_5{ZeroInitialize<Tango::AttributeValue_5>().value};
            }
            else if(vers == 4)
            {
                send_attr_4 = new Tango::AttributeValue_4{ZeroInitialize<Tango::AttributeValue_4>().value};
            }
            else
            {
                send_attr = new Tango::AttributeValue_3{ZeroInitialize<Tango::AttributeValue_3>().value};
            }
        }
        catch(std::bad_alloc &)
        {
            TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
        }

        //
        // Don`t try to access the attribute data when an exception was indicated
        //

        if(except == nullptr)
        {
            if(send_attr_5 != nullptr)
            {
                Attribute_2_AttributeValue(send_attr_5, dev);
            }
            else if(send_attr_4 != nullptr)
            {
                Attribute_2_AttributeValue(send_attr_4, dev);
            }
            else
            {
                Attribute_2_AttributeValue(send_attr, dev);
            }
        }

        //
        // Fire event
        //

        bool criteria_fulfilled = false;

        switch(event_type)
        {
        case CHANGE_EVENT:
            criteria_fulfilled = is_check_change_criteria();
            break;
        case ALARM_EVENT:
            criteria_fulfilled = is_check_alarm_criteria();
            break;
        case ARCHIVE_EVENT:
            criteria_fulfilled = is_check_archive_criteria();
            break;
        case USER_EVENT:
            break;
        default:
            TANGO_ASSERT_ON_DEFAULT(event_type);
        }

        //
        // Create the structure used to send data to event system
        //

        EventSupplier::SuppliedEventData ad;
        ::memset(&ad, 0, sizeof(ad));

        if(send_attr_5 != nullptr)
        {
            ad.attr_val_5 = send_attr_5;
        }
        else if(send_attr_4 != nullptr)
        {
            ad.attr_val_4 = send_attr_4;
        }
        else if(send_attr != nullptr)
        {
            ad.attr_val_3 = send_attr;
        }

        if(criteria_fulfilled)
        {
            //
            // Eventually push the event (if detected)
            // When we have both notifd and zmq event supplier, do not detect the event
            // two times. The detect_and_push_events() method returns true if the event
            // is detected.
            //

            auto now_timeval = PollClock::now(); // for archive event

            bool send_event = false;
            if(event_supplier_nd != nullptr)
            {
                switch(event_type)
                {
                case CHANGE_EVENT:
                    send_event = event_supplier_nd->detect_and_push_change_event(dev, ad, *this, name, except, true);
                    break;
                case ARCHIVE_EVENT:
                    send_event = event_supplier_nd->detect_and_push_archive_event(
                        dev, ad, *this, name, except, now_timeval, true);
                    break;
                default: // ALARM_EVENT cannot be sent with notify
                    TANGO_ASSERT_ON_DEFAULT(event_type);
                }
            }
            if(event_supplier_zmq != nullptr)
            {
                if(event_supplier_nd != nullptr)
                {
                    if(send_event && pub_socket_created)
                    {
                        std::vector<std::string> f_names;
                        std::vector<double> f_data;
                        std::vector<std::string> f_names_lg;
                        std::vector<long> f_data_lg;

                        event_supplier_zmq->push_event_loop(
                            dev, event_type, f_names, f_data, f_names_lg, f_data_lg, ad, *this, except);
                    }
                }
                else
                {
                    if(pub_socket_created)
                    {
                        switch(event_type)
                        {
                        case CHANGE_EVENT:
                            event_supplier_zmq->detect_and_push_change_event(dev, ad, *this, name, except, true);
                            break;
                        case ALARM_EVENT:
                            event_supplier_zmq->detect_and_push_alarm_event(dev, ad, *this, name, except, true);
                            break;
                        case ARCHIVE_EVENT:
                            event_supplier_zmq->detect_and_push_archive_event(
                                dev, ad, *this, name, except, now_timeval, true);
                            break;
                        default:
                            TANGO_ASSERT_ON_DEFAULT(event_type);
                        }
                    }
                }
            }
        }
        else
        {
            //
            // Send event, if the read_attribute failed or if it is the first time
            // that the read_attribute succeed after a failure.
            // Same thing if the attribute quality factor changes to INVALID
            //
            // This is done only to be able to set-up the same filters with events
            // comming with the standard mechanism or coming from a manual fire event call.
            //

            std::vector<std::string> filterable_names_lg;
            std::vector<long> filterable_data_lg;

            if(event_type != USER_EVENT)
            {
                bool force_event = false;
                bool quality_change = false;
                double delta_change_rel = 0.0;
                double delta_change_abs = 0.0;

                {
                    omni_mutex_lock oml(EventSupplier::get_event_mutex());

                    LastAttrValue *prev_event;
                    switch(event_type)
                    {
                    case CHANGE_EVENT:
                        prev_event = &prev_change_event;
                        break;
                    case ALARM_EVENT:
                        prev_event = &prev_alarm_event;
                        break;
                    case ARCHIVE_EVENT:
                        prev_event = &prev_archive_event;
                        break;
                    default:
                        TANGO_ASSERT_ON_DEFAULT(event_type);
                    }

                    const AttrQuality previous_quality = prev_event->quality;

                    if(event_type == ARCHIVE_EVENT)
                    {
                        // Execute detect_change only to calculate the delta_change_rel and
                        // delta_change_abs and force_change !

                        if((event_supplier_nd != nullptr) || (event_supplier_zmq != nullptr))
                        {
                            EventSupplier *event_supplier =
                                event_supplier_nd != nullptr ? event_supplier_nd : event_supplier_zmq;
                            event_supplier->detect_change(
                                *this, ad, true, delta_change_rel, delta_change_abs, except, force_event, dev);
                        }
                    }
                    else if(event_type == CHANGE_EVENT || event_type == ALARM_EVENT)
                    {
                        if((except != nullptr) || quality == Tango::ATTR_INVALID || prev_event->err ||
                           previous_quality == Tango::ATTR_INVALID)
                        {
                            force_event = true;
                        }
                    }

                    prev_event->store(send_attr_5, send_attr_4, send_attr, nullptr, except);

                    quality_change = (previous_quality != prev_event->quality);
                }

                filterable_names.emplace_back("forced_event");
                if(force_event)
                {
                    filterable_data.push_back(1.0);
                }
                else
                {
                    filterable_data.push_back(0.0);
                }

                filterable_names.emplace_back("quality");
                if(quality_change)
                {
                    filterable_data.push_back(1.0);
                }
                else
                {
                    filterable_data.push_back(0.0);
                }

                if(event_type == ARCHIVE_EVENT)
                {
                    filterable_names.emplace_back("counter");
                    filterable_data_lg.push_back(-1);

                    filterable_names.emplace_back("delta_change_rel");
                    filterable_data.push_back(delta_change_rel);
                    filterable_names.emplace_back("delta_change_abs");
                    filterable_data.push_back(delta_change_abs);
                }
            }

            //
            // Finally push the event(s)
            //

            if(event_supplier_nd != nullptr && event_type != ALARM_EVENT)
            {
                event_supplier_nd->push_event(dev,
                                              EventName[event_type],
                                              filterable_names,
                                              filterable_data,
                                              filterable_names_lg,
                                              filterable_data_lg,
                                              ad,
                                              name,
                                              except,
                                              false);
            }

            if(event_supplier_zmq != nullptr && pub_socket_created)
            {
                event_supplier_zmq->push_event_loop(dev,
                                                    event_type,
                                                    filterable_names,
                                                    filterable_data,
                                                    filterable_names_lg,
                                                    filterable_data_lg,
                                                    ad,
                                                    *this,
                                                    except);
            }
        }

        //
        // Return allocated memory
        //

        if(send_attr_5 != nullptr)
        {
            delete send_attr_5;
            send_attr_5 = nullptr;
        }
        else if(send_attr_4 != nullptr)
        {
            delete send_attr_4;
            send_attr_4 = nullptr;
        }
        else
        {
            delete send_attr;
            send_attr = nullptr;
        }

        //
        // Delete the data values allocated in the attribute
        //

        if(must_clean_data)
        {
            delete_seq_and_reset_alarm();
        }
    }
    catch(...)
    {
        if(send_attr_5 != nullptr)
        {
            delete send_attr_5;
        }
        else if(send_attr_4 != nullptr)
        {
            delete send_attr_4;
        }
        else
        {
            delete send_attr;
        }

        if(must_clean_data)
        {
            delete_seq_and_reset_alarm();
        }
        throw;
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::fire_change_event
//
// description :
//        Fire a change event for the attribute value.
//
// arguments:
//         in :
//            - ptr : Pointer to a DevFailed exception to fire in case of an error to indicate.
//
//---------------------------------------------------------------------------------------------------------------------

void Attribute::fire_change_event(DevFailed *except)
{
    generic_fire_event(CHANGE_EVENT, except);
}

/**
 * @name Attribute::fire_alarm_event
 *
 * @brief Fire an alarm event for the attribute value.
 * @arg in:
 *          - ptr: Pointer to a DevFailed exception to fire in case of an error
 *                  to indicate.
 **/
void Attribute::fire_alarm_event(DevFailed *except)
{
    generic_fire_event(ALARM_EVENT, except);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::fire_archive_event
//
// description :
//        Fire a archive change event for the attribute value.
//
// arguments:
//         in :
//            - ptr : Pointer to a DevFailed exception to fire in case of an error to indicate.
//
//-------------------------------------------------------------------------------------------------------------------

void Attribute::fire_archive_event(DevFailed *except)
{
    generic_fire_event(ARCHIVE_EVENT, except);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::fire_event
//
// description :
//        Fire a user event for the attribute value.
//
// arguments:
//         in :
//            - filt_names : The filterable fields name
//            - filt_vals : The filterable fields value (as double)
//            - except : Pointer to a DevFailed exception to fire in case of an error to indicate.
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::fire_event(const std::vector<std::string> &filt_names,
                           const std::vector<double> &filt_vals,
                           DevFailed *except)
{
    generic_fire_event(USER_EVENT, except, true, filt_names, filt_vals);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::fire_error_periodic_event
//
// description :
//        Fire a periodic event with error set. This is used when attribute polling is stopped
//
// arguments:
//         in :
//            - except : Pointer to a DevFailed exception to fire in case of an error to indicate.
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::fire_error_periodic_event(DevFailed *except)
{
    TANGO_LOG_DEBUG << "Attribute::fire_error_periodic_event() entring ..." << std::endl;

    //
    // Check if it is needed to send an event
    //

    time_t now;
    time_t periodic3_subscription, periodic4_subscription, periodic5_subscription;

    now = Tango::get_current_system_datetime();

    {
        omni_mutex_lock oml(EventSupplier::get_event_mutex());
        periodic3_subscription = now - event_periodic3_subscription;
        periodic4_subscription = now - event_periodic4_subscription;
        periodic5_subscription = now - event_periodic5_subscription;
    }

    std::vector<int> client_libs = get_client_lib(PERIODIC_EVENT); // We want a copy

    std::vector<int>::iterator ite;
    for(ite = client_libs.begin(); ite != client_libs.end(); ++ite)
    {
        switch(*ite)
        {
        case 6:
            if(periodic5_subscription >= EVENT_RESUBSCRIBE_PERIOD)
            {
                remove_client_lib(6, std::string(EventName[PERIODIC_EVENT]));
            }
            break;

        case 5:
            if(periodic5_subscription >= EVENT_RESUBSCRIBE_PERIOD)
            {
                remove_client_lib(5, std::string(EventName[PERIODIC_EVENT]));
            }
            break;

        case 4:
            if(periodic4_subscription >= EVENT_RESUBSCRIBE_PERIOD)
            {
                remove_client_lib(4, std::string(EventName[PERIODIC_EVENT]));
            }
            break;

        default:
            if(periodic3_subscription >= EVENT_RESUBSCRIBE_PERIOD)
            {
                remove_client_lib(3, std::string(EventName[PERIODIC_EVENT]));
            }
            break;
        }
    }

    //
    // Get the event supplier, and simply return if not created
    //

    EventSupplier *event_supplier_nd = nullptr;
    EventSupplier *event_supplier_zmq = nullptr;

    Tango::Util *tg = Util::instance();
    if(use_notifd_event())
    {
        event_supplier_nd = tg->get_notifd_event_supplier();
    }
    if(use_zmq_event())
    {
        event_supplier_zmq = tg->get_zmq_event_supplier();
    }

    if(((event_supplier_nd == nullptr) && (event_supplier_zmq == nullptr)) || client_libs.empty())
    {
        return;
    }

    //
    // Retrieve device object if not already done
    //

    if(dev == nullptr)
    {
        dev = tg->get_device_by_name(d_name);
    }

    //
    // Create the structure used to send data to event system
    //

    EventSupplier::SuppliedEventData ad;
    ::memset(&ad, 0, sizeof(ad));

    //
    // Fire event
    //

    std::vector<std::string> filterable_names_lg, filt_names;
    std::vector<long> filterable_data_lg;
    std::vector<double> filt_vals;

    if(event_supplier_nd != nullptr)
    {
        event_supplier_nd->push_event(dev,
                                      "periodic_event",
                                      filt_names,
                                      filt_vals,
                                      filterable_names_lg,
                                      filterable_data_lg,
                                      ad,
                                      name,
                                      except,
                                      false);
    }
    if(event_supplier_zmq != nullptr)
    {
        event_supplier_zmq->push_event_loop(
            dev, PERIODIC_EVENT, filt_names, filt_vals, filterable_names_lg, filterable_data_lg, ad, *this, except);
    }
}

//+-------------------------------------------------------------------------
//
// operator overloading :     set_quality
//
// description :     Set the attribute quality factor
//
//--------------------------------------------------------------------------

void Attribute::set_quality(Tango::AttrQuality qua, bool send_event)
{
    quality = qua;
    if(send_event)
    {
        fire_change_event();
    }
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::remove_configuration()
//
// description :     Remove the attribute configuration from the database.
//                     This method can be used to clean-up all the configuration
//                  of an attribute to come back to its default values or the
//                  remove all configuration of a dynamic attribute before deleting it.
//
//                     The method removes all configured attribute properties
//                  and removes the attribute from the list of polled attributes.
//--------------------------------------------------------------------------

void Attribute::remove_configuration()
{
    TANGO_LOG_DEBUG << "Entering remove_configuration() method for attribute " << name << std::endl;

    Tango::Util *tg = Tango::Util::instance();

    //
    // read all configured properties of the attribute from the database and
    // delete them!
    //

    DbData db_read_data;
    DbData db_delete_data;

    db_read_data.emplace_back(name);
    db_delete_data.emplace_back(name);

    //
    // Implement a reconnection schema. The first exception received if the db
    // server is down is a COMM_FAILURE exception. Following exception received
    // from following calls are TRANSIENT exception
    //

    bool retry = true;
    while(retry)
    {
        try
        {
            tg->get_database()->get_device_attribute_property(d_name, db_read_data);
            retry = false;
        }
        catch(CORBA::COMM_FAILURE &)
        {
            tg->get_database()->reconnect(true);
        }
    }

    long nb_prop = 0;
    db_read_data[0] >> nb_prop;

    for(int k = 1; k < (nb_prop + 1); k++)
    {
        std::string &prop_name = db_read_data[k].name;
        db_delete_data.emplace_back(prop_name);
    }

    //
    // Implement a reconnection schema. The first exception received if the db
    // server is down is a COMM_FAILURE exception. Following exception received
    // from following calls are TRANSIENT exception
    //

    if(nb_prop > 0)
    {
        retry = true;
        while(retry)
        {
            try
            {
                tg->get_database()->delete_device_attribute_property(d_name, db_delete_data);
                retry = false;
            }
            catch(CORBA::COMM_FAILURE &)
            {
                tg->get_database()->reconnect(true);
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::get_att_device
//
// description :     Return a pointer to the attribute device
//
//--------------------------------------------------------------------------

DeviceImpl *Attribute::get_att_device()
{
    if(dev == nullptr)
    {
        Tango::Util *tg = Tango::Util::instance();
        dev = tg->get_device_by_name(d_name);
    }

    return dev;
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::set_attr_serial_method
//
// description :     Set attribute serialization method
//
//--------------------------------------------------------------------------

void Attribute::set_attr_serial_model(AttrSerialModel ser_model)
{
    if(ser_model == Tango::ATTR_BY_USER)
    {
        Tango::Util *tg = Tango::Util::instance();
        if(tg->get_serial_model() != Tango::BY_DEVICE)
        {
            TANGO_THROW_EXCEPTION(API_AttrNotAllowed,
                                  "Attribute serial model by user is not allowed when the process is not in BY_DEVICE "
                                  "serialization model");
        }
    }

    attr_serial_model = ser_model;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::get_att_device_class
//
// description :
//        Return a pointer to the attribute device class
//
// argument :
//        in :
//            - dev_name: The device name
//
//-------------------------------------------------------------------------------------------------------------------

DeviceClass *Attribute::get_att_device_class(const std::string &dev_name)
{
    //
    // Get device class. When the server is started, it's an easy task. When the server is in its starting phase, it's
    // more tricky. Get from the DeviceClass list the first one for which the device_factory method has not yet been
    // fully executed. This is the DeviceClass with the device in its init_device() method has called
    // Attribute::set_properties()
    //

    Tango::Util *tg = Tango::Util::instance();
    Tango::DeviceClass *dev_class = nullptr;

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        Tango::DeviceImpl *dev = get_att_device();
        dev_class = dev->get_device_class();
    }
    else
    {
        const std::vector<DeviceClass *> &tmp_cl_list = *tg->get_class_list();
        size_t loop;
        for(loop = 0; loop < tmp_cl_list.size(); ++loop)
        {
            if(!tmp_cl_list[loop]->get_device_factory_done())
            {
                break;
            }
            else
            {
                // If a server wants to set attribute properties during a "write memorized attribute value at init"
                // phase, we might be in this case...
                std::vector<DeviceImpl *> &dev_list = tmp_cl_list[loop]->get_device_list();
                // Check whether our device is listed in this class
                for(size_t i = 0; i < dev_list.size(); ++i)
                {
                    if(dev_list[i]->get_name() == dev_name)
                    {
                        // Our device is listed in this class, returns the corresponding DeviceClass pointer
                        return tmp_cl_list[loop];
                    }
                }
            }
        }

        if(loop != tmp_cl_list.size())
        {
            dev_class = tmp_cl_list[loop];
        }
        else
        {
            std::stringstream o;
            o << "Device " << dev_name << "-> Attribute : " << name;
            o << "\nCan't retrieve device class!" << std::ends;

            TANGO_THROW_EXCEPTION(API_CantRetrieveClass, o.str());
        }
    }

    return dev_class;
}

//+-------------------------------------------------------------------------
//
// method :         Attribute::log_quality
//
// description :    Send a logging message (on the device) when the attribute
//                  quality factor changes
//
//--------------------------------------------------------------------------

void Attribute::log_quality()
{
    //
    // Set device if not already done
    //

    if(dev == nullptr)
    {
        Tango::Util *tg = Tango::Util::instance();
        dev = tg->get_device_by_name(d_name);
    }

    const bool has_quality_changed = quality != old_quality;
    const bool has_alarm_changed = alarm != old_alarm;
    const bool is_alarm_set = alarm.any();

    if(has_quality_changed)
    {
        if(!is_alarm_set)
        {
            switch(quality)
            {
            case ATTR_INVALID:
                DEV_ERROR_STREAM(dev) << "INVALID quality for attribute " << name << std::endl;
                break;

            case ATTR_CHANGING:
                DEV_INFO_STREAM(dev) << "CHANGING quality for attribute " << name << std::endl;
                break;

            case ATTR_VALID:
                DEV_INFO_STREAM(dev) << "INFO quality for attribute " << name << std::endl;
                break;

            case ATTR_WARNING:
                DEV_WARN_STREAM(dev) << "User defined WARNING quality for attribute " << name << std::endl;
                break;

            case ATTR_ALARM:
                DEV_ERROR_STREAM(dev) << "User defined ALARM quality for attribute " << name << std::endl;
                break;

            default:
                TANGO_ASSERT_ON_DEFAULT(quality);
            }
        }
        else
        {
            log_alarm_quality();
        }
    }
    else if(has_alarm_changed)
    {
        log_alarm_quality();
    }
}

void Attribute::log_alarm_quality() const
{
    if(alarm[min_level])
    {
        DEV_ERROR_STREAM(dev) << "MIN ALARM for attribute " << name << std::endl;
    }
    else if(alarm[max_level])
    {
        DEV_ERROR_STREAM(dev) << "MAX ALARM for attribute " << name << std::endl;
    }
    else if(alarm[rds])
    {
        DEV_WARN_STREAM(dev) << "RDS (Read Different Set) ALARM for attribute " << name << std::endl;
    }
    else if(alarm[min_warn])
    {
        DEV_WARN_STREAM(dev) << "MIN WARNING for attribute " << name << std::endl;
    }
    else if(alarm[max_warn])
    {
        DEV_WARN_STREAM(dev) << "MAX WARNING for attribute " << name << std::endl;
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::set_format_notspec()
//
// description :
//        Set the attribute format property to the default value which depends on attribute data type
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::set_format_notspec()
{
    switch(data_type)
    {
    case DEV_SHORT:
    case DEV_LONG:
    case DEV_LONG64:
    case DEV_UCHAR:
    case DEV_USHORT:
    case DEV_ULONG:
    case DEV_ULONG64:
        format = FormatNotSpec_INT;
        break;

    case DEV_STRING:
    case DEV_ENUM:
        format = FormatNotSpec_STR;
        break;

    case DEV_STATE:
    case DEV_ENCODED:
    case DEV_BOOLEAN:
        format = AlrmValueNotSpec;
        break;

    case DEV_FLOAT:
    case DEV_DOUBLE:
        format = FormatNotSpec_FL;
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::is_format_notspec()
//
// description :
//        Set the attribute format property to the default value which depends on attribute data type
//
// argument :
//        in :
//            - format : The format property string
//
// return :
//        This method retruns true if the format string is the default value
//
//--------------------------------------------------------------------------------------------------------------------

bool Attribute::is_format_notspec(const char *format)
{
    bool ret = false;

    switch(data_type)
    {
    case DEV_SHORT:
    case DEV_LONG:
    case DEV_LONG64:
    case DEV_UCHAR:
    case DEV_USHORT:
    case DEV_ULONG:
    case DEV_ULONG64:
        if(TG_strcasecmp(format, FormatNotSpec_INT) == 0)
        {
            ret = true;
        }
        break;

    case DEV_STRING:
    case DEV_ENUM:
        if(TG_strcasecmp(format, FormatNotSpec_STR) == 0)
        {
            ret = true;
        }
        break;

    case DEV_STATE:
    case DEV_ENCODED:
    case DEV_BOOLEAN:
        if(TG_strcasecmp(format, AlrmValueNotSpec) == 0)
        {
            ret = true;
        }
        break;

    case DEV_FLOAT:
    case DEV_DOUBLE:
        if(TG_strcasecmp(format, FormatNotSpec_FL) == 0)
        {
            ret = true;
        }
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }

    return ret;
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::def_format_in_dbdatum()
//
// description :
//        Insert the default format string in a DbDatum instance. This default value depends on the attribute
//        data type
//
// argument :
//        in :
//            - db : Reference to the DbDatum object
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::def_format_in_dbdatum(DbDatum &db)
{
    switch(data_type)
    {
    case DEV_SHORT:
    case DEV_LONG:
    case DEV_LONG64:
    case DEV_UCHAR:
    case DEV_USHORT:
    case DEV_ULONG:
    case DEV_ULONG64:
        db << FormatNotSpec_INT;
        break;

    case DEV_STRING:
    case DEV_ENUM:
        db << FormatNotSpec_STR;
        break;

    case DEV_STATE:
    case DEV_ENCODED:
    case DEV_BOOLEAN:
        db << AlrmValueNotSpec;
        break;

    case DEV_FLOAT:
    case DEV_DOUBLE:
        db << FormatNotSpec_FL;
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(data_type);
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::xxx_event_subscribed()
//
// description :
//        Returns true if there are some subscriber listening for event. This is a method family. There is one method
//        for each event type
//        Cannot replce this methods family by one method with event type as parameter for compatibility reason
//
//--------------------------------------------------------------------------------------------------------------------

bool Attribute::change_event_subscribed()
{
    bool ret = false;
    const time_t now = Tango::get_current_system_datetime();

    if(event_change5_subscription != 0)
    {
        ret = now - event_change5_subscription <= EVENT_RESUBSCRIBE_PERIOD;
    }

    if(!ret)
    {
        if(event_change4_subscription != 0)
        {
            ret = now - event_change4_subscription <= EVENT_RESUBSCRIBE_PERIOD;
        }

        if(!ret)
        {
            if(event_change3_subscription != 0)
            {
                ret = now - event_change3_subscription <= EVENT_RESUBSCRIBE_PERIOD;
            }
        }
    }
    return ret;
}

/**
 * @brief Returns true if there is a subscriber listening for the alarm event.
 * @note This method belongs to a group of methods that is responsible to
 * handle all the event types.
 * @attention Please note that to ensure backwards compatibility this method
 * group cannot be replaced with a single method that would accept the event
 * type as a parameter.
 **/
bool Attribute::alarm_event_subscribed()
{
    if(event_alarm6_subscription != 0)
    {
        const auto now{::time(nullptr)};
        if((now - event_alarm6_subscription) <= EVENT_RESUBSCRIBE_PERIOD)
        {
            return true;
        }
    }

    return false;
}

bool Attribute::periodic_event_subscribed()
{
    bool ret = false;
    const time_t now = Tango::get_current_system_datetime();

    if(event_periodic5_subscription != 0)
    {
        ret = now - event_periodic5_subscription <= EVENT_RESUBSCRIBE_PERIOD;
    }

    if(!ret)
    {
        if(event_periodic4_subscription != 0)
        {
            ret = now - event_periodic4_subscription <= EVENT_RESUBSCRIBE_PERIOD;
        }

        if(!ret)
        {
            if(event_periodic3_subscription != 0)
            {
                ret = now - event_periodic3_subscription <= EVENT_RESUBSCRIBE_PERIOD;
            }
        }
    }
    return ret;
}

bool Attribute::archive_event_subscribed()
{
    bool ret = false;
    const time_t now = Tango::get_current_system_datetime();

    if(event_archive5_subscription != 0)
    {
        ret = now - event_archive5_subscription <= EVENT_RESUBSCRIBE_PERIOD;
    }

    if(!ret)
    {
        if(event_archive4_subscription != 0)
        {
            ret = now - event_archive4_subscription <= EVENT_RESUBSCRIBE_PERIOD;
        }

        if(!ret)
        {
            if(event_archive3_subscription != 0)
            {
                ret = now - event_archive3_subscription <= EVENT_RESUBSCRIBE_PERIOD;
            }
        }
    }
    return ret;
}

bool Attribute::user_event_subscribed()
{
    bool ret = false;
    const time_t now = Tango::get_current_system_datetime();

    if(event_user5_subscription != 0)
    {
        ret = now - event_user5_subscription <= EVENT_RESUBSCRIBE_PERIOD;
    }

    if(!ret)
    {
        if(event_user4_subscription != 0)
        {
            ret = now - event_user4_subscription <= EVENT_RESUBSCRIBE_PERIOD;
        }

        if(!ret)
        {
            if(event_user3_subscription != 0)
            {
                ret = now - event_user3_subscription <= EVENT_RESUBSCRIBE_PERIOD;
            }
        }
    }
    return ret;
}

bool Attribute::attr_conf_event_subscribed()
{
    bool ret = false;

    if(event_attr_conf5_subscription != 0)
    {
        const time_t now = Tango::get_current_system_datetime();
        ret = now - event_attr_conf5_subscription <= EVENT_RESUBSCRIBE_PERIOD;
    }

    if(!ret)
    {
        if(event_attr_conf_subscription != 0)
        {
            const time_t now = Tango::get_current_system_datetime();
            ret = now - event_attr_conf_subscription <= EVENT_RESUBSCRIBE_PERIOD;
        }
    }
    return ret;
}

bool Attribute::data_ready_event_subscribed()
{
    bool ret = false;

    if(event_data_ready_subscription != 0)
    {
        const time_t now = Tango::get_current_system_datetime();
        ret = now - event_data_ready_subscription <= EVENT_RESUBSCRIBE_PERIOD;
    }

    return ret;
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::set_client_lib()
//
// description :
//        Set client lib (for event compatibility)
//
// argument :
//        in :
//            - _l : Client lib release
//            - ev_name : Event name
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::set_client_lib(int client_lib_version, EventType event_type)
{
    TANGO_LOG_DEBUG << "Attribute::set_client_lib(" << client_lib_version << "," << EventName[event_type] << ")"
                    << std::endl;

    if(0 == count(client_lib[event_type].begin(), client_lib[event_type].end(), client_lib_version))
    {
        client_lib[event_type].push_back(client_lib_version);
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::remove_client_lib()
//
// description :
//        Remove a client lib (for event compatibility)
//
// argument :
//        in :
//            - _l : Client lib release
//            - ev_name : Event name
//
//--------------------------------------------------------------------------------------------------------------------

void Attribute::remove_client_lib(int _l, const std::string &ev_name)
{
    int i;
    for(i = 0; i < numEventType; i++)
    {
        if(ev_name == EventName[i])
        {
            break;
        }
    }

    auto pos = find(client_lib[i].begin(), client_lib[i].end(), _l);
    if(pos != client_lib[i].end())
    {
        client_lib[i].erase(pos);
    }
}

void Attribute::extract_value(CORBA::Any &dest)
{
    switch(get_data_type())
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
    {
        _extract_value<Tango::DevVarShortArray>(dest);
        break;
    }

    case Tango::DEV_LONG:
    {
        _extract_value<Tango::DevVarLongArray>(dest);
        break;
    }

    case Tango::DEV_LONG64:
    {
        _extract_value<Tango::DevVarLong64Array>(dest);
        break;
    }

    case Tango::DEV_DOUBLE:
    {
        _extract_value<Tango::DevVarDoubleArray>(dest);
        break;
    }

    case Tango::DEV_STRING:
    {
        _extract_value<Tango::DevVarStringArray>(dest);
        break;
    }

    case Tango::DEV_FLOAT:
    {
        _extract_value<Tango::DevVarFloatArray>(dest);
        break;
    }

    case Tango::DEV_BOOLEAN:
    {
        _extract_value<Tango::DevVarBooleanArray>(dest);
        break;
    }

    case Tango::DEV_USHORT:
    {
        _extract_value<Tango::DevVarUShortArray>(dest);
        break;
    }

    case Tango::DEV_UCHAR:
    {
        _extract_value<Tango::DevVarCharArray>(dest);
        break;
    }

    case Tango::DEV_ULONG:
    {
        _extract_value<Tango::DevVarULongArray>(dest);
        break;
    }

    case Tango::DEV_ULONG64:
    {
        _extract_value<Tango::DevVarULong64Array>(dest);
        break;
    }

    case Tango::DEV_STATE:
    {
        _extract_value<Tango::DevVarStateArray>(dest);
        break;
    }
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        set_min_alarm()
//
// description :
//        Sets minimum alarm attribute property. Throws exception in case the data type of provided property does not
//        match the attribute data type
//
// args :
//         in :
//            - new_min_alarm : The minimum alarm property to be set
//
//-------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::set_min_alarm(const T &new_min_alarm)
{
    //
    // Check type validity
    //
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
       (data_type == Tango::DEV_ENUM))
    {
        Tango::detail::throw_err_data_type("min_alarm", d_name, name, "Attribute::set_min_alarm()");
    }

    else if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
            (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    //    Check coherence with max_alarm
    //

    auto &alarm_conf = is_alarmed();

    if(alarm_conf.test(Tango::Attribute::alarm_flags::max_level))
    {
        T max_alarm_tmp;
        memcpy((void *) &max_alarm_tmp, (const void *) &max_alarm, sizeof(T));
        if(new_min_alarm >= max_alarm_tmp)
        {
            Tango::detail::throw_incoherent_val_err(
                "min_alarm", "max_alarm", d_name, name, "Attribute::set_min_alarm()");
        }
    }

    //
    // Store new min alarm as a string
    //

    TangoSys_MemStream str;
    str.precision(Tango::TANGO_FLOAT_PRECISION);
    if(Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR)
    {
        str << (short) new_min_alarm; // to represent the numeric value
    }
    else
    {
        str << new_min_alarm;
    }
    std::string min_alarm_tmp_str;
    min_alarm_tmp_str = str.str();

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
    // Store the new alarm locally
    //

    Tango::Attr_CheckVal old_min_alarm;
    memcpy((void *) &old_min_alarm, (void *) &min_alarm, sizeof(T));
    memcpy((void *) &min_alarm, (void *) &new_min_alarm, sizeof(T));

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
            if(def_user_prop[i].get_name() == "min_alarm")
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
        if(user_defaults && min_alarm_tmp_str == usr_def_val)
        {
            Tango::DbDatum attr_dd(name), prop_dd("min_alarm");
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
                Tango::detail::upd_att_prop_db(min_alarm, "min_alarm", d_name, name, get_data_type());
            }
            catch(Tango::DevFailed &)
            {
                memcpy((void *) &min_alarm, (void *) &old_min_alarm, sizeof(T));
                throw;
            }
        }
    }

    //
    // Set the min_alarm flag
    //

    alarm_conf.set(Tango::Attribute::alarm_flags::min_level);

    //
    // Store new alarm as a string
    //

    min_alarm_str = min_alarm_tmp_str;

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }

    //
    // Delete device startup exception related to min_alarm if there is any
    //

    Tango::detail::delete_startup_exception(*this, "min_alarm", d_name, check_startup_exceptions, startup_exceptions);
}

template void Attribute::set_min_alarm(const Tango::DevBoolean &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevUChar &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevShort &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevUShort &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevLong &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevULong &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevLong64 &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevULong64 &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevFloat &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevDouble &new_min_alarm);
template void Attribute::set_min_alarm(const Tango::DevState &new_min_alarm);

template <>
void Attribute::set_min_alarm(const Tango::DevEncoded &)
{
    std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
    TANGO_THROW_EXCEPTION(API_MethodArgument, err_msg.c_str());
}

void Attribute::set_min_alarm(const std::string &new_min_alarm_str)
{
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE))
    {
        Tango::detail::throw_err_data_type("min_alarm", d_name, name, "Attribute::set_min_alarm()");
    }

    std::string min_alarm_str_tmp = new_min_alarm_str;

    Tango::DeviceClass *dev_class = get_att_device_class(d_name);
    Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
    Tango::Attr &att = mca->get_attr(name);
    std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
    std::vector<AttrProperty> &def_class_prop = att.get_class_properties();

    std::string usr_def_val;
    std::string class_def_val;
    bool user_defaults = false;
    bool class_defaults = false;

    user_defaults = Tango::detail::prop_in_list("min_alarm", usr_def_val, def_user_prop);

    class_defaults = Tango::detail::prop_in_list("min_alarm", class_def_val, def_class_prop);

    bool set_value = true;

    bool dummy;
    if(class_defaults)
    {
        if(TG_strcasecmp(new_min_alarm_str.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("min_alarm", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, min_alarm_str);
        }
        else if((TG_strcasecmp(new_min_alarm_str.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_min_alarm_str.c_str(), class_def_val.c_str()) == 0))
        {
            min_alarm_str_tmp = class_def_val;
        }
        else if(strlen(new_min_alarm_str.c_str()) == 0)
        {
            if(user_defaults)
            {
                min_alarm_str_tmp = usr_def_val;
            }
            else
            {
                set_value = false;

                Tango::detail::avns_in_db("min_alarm", name, d_name);
                Tango::detail::avns_in_att(*this, d_name, dummy, min_alarm_str);
            }
        }
    }
    else if(user_defaults)
    {
        if(TG_strcasecmp(new_min_alarm_str.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("min_alarm", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, min_alarm_str);
        }
        else if((TG_strcasecmp(new_min_alarm_str.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_min_alarm_str.c_str(), usr_def_val.c_str()) == 0) ||
                (strlen(new_min_alarm_str.c_str()) == 0))
        {
            min_alarm_str_tmp = usr_def_val;
        }
    }
    else
    {
        if((TG_strcasecmp(new_min_alarm_str.c_str(), AlrmValueNotSpec) == 0) ||
           (TG_strcasecmp(new_min_alarm_str.c_str(), NotANumber) == 0) || (strlen(new_min_alarm_str.c_str()) == 0))
        {
            set_value = false;

            Tango::detail::avns_in_db("min_alarm", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, min_alarm_str);
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
            str.precision(TANGO_FLOAT_PRECISION);
            str << min_alarm_str_tmp;
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                set_min_alarm((DevShort) db);
                break;

            case Tango::DEV_LONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                set_min_alarm((DevLong) db);
                break;

            case Tango::DEV_LONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                set_min_alarm((DevLong64) db);
                break;

            case Tango::DEV_DOUBLE:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                set_min_alarm(db);
                break;

            case Tango::DEV_FLOAT:
                if(!(str >> fl && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                set_min_alarm(fl);
                break;

            case Tango::DEV_USHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                (db < 0.0) ? set_min_alarm((DevUShort) (-db)) : set_min_alarm((DevUShort) db);
                break;

            case Tango::DEV_UCHAR:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                (db < 0.0) ? set_min_alarm((DevUChar) (-db)) : set_min_alarm((DevUChar) db);
                break;

            case Tango::DEV_ULONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                (db < 0.0) ? set_min_alarm((DevULong) (-db)) : set_min_alarm((DevULong) db);
                break;

            case Tango::DEV_ULONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                (db < 0.0) ? set_min_alarm((DevULong64) (-db)) : set_min_alarm((DevULong64) db);
                break;

            case Tango::DEV_ENCODED:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_alarm", d_name, name, "Attribute::set_min_alarm()");
                }
                (db < 0.0) ? set_min_alarm((DevUChar) (-db)) : set_min_alarm((DevUChar) db);
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("min_alarm", d_name, name, "Attribute::set_min_alarm()");
        }
    }
}

template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::get_min_alarm(T &min_al)
{
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << get_name()
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }
    else if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
            (data_type == Tango::DEV_ENUM))
    {
        std::stringstream err_msg;
        err_msg << "Minimum alarm has no meaning for the attribute's (" << get_name()
                << ") data type : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, err_msg.str().c_str());
    }

    auto &alarm_conf = is_alarmed();
    if(!alarm_conf[Tango::Attribute::alarm_flags::min_level])
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrNotAllowed, "Minimum alarm not defined for this attribute");
    }

    memcpy((void *) &min_al, (void *) &min_alarm, sizeof(T));
    //    ::get_min_alarm(min_al, *this, min_alarm);
}

template void Attribute::get_min_alarm(Tango::DevBoolean &min_al);
template void Attribute::get_min_alarm(Tango::DevUChar &min_al);
template void Attribute::get_min_alarm(Tango::DevShort &min_al);
template void Attribute::get_min_alarm(Tango::DevUShort &min_al);
template void Attribute::get_min_alarm(Tango::DevLong &min_al);
template void Attribute::get_min_alarm(Tango::DevULong &min_al);
template void Attribute::get_min_alarm(Tango::DevLong64 &min_al);
template void Attribute::get_min_alarm(Tango::DevULong64 &min_al);
template void Attribute::get_min_alarm(Tango::DevFloat &min_al);
template void Attribute::get_min_alarm(Tango::DevDouble &min_al);
template void Attribute::get_min_alarm(Tango::DevState &min_al);
template void Attribute::get_min_alarm(Tango::DevString &min_al);
template void Attribute::get_min_alarm(Tango::DevEncoded &min_al);

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        set_max_alarm()
//
// description :
//        Sets maximum alarm attribute property
//        Throws exception in case the data type of provided property does not match the attribute data type
//
// args :
//         in :
//            - new_max_alarm : The maximum alarm property to be set
//
//------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::set_max_alarm(const T &new_max_alarm)
{
    //
    // Check type validity
    //
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
       (data_type == Tango::DEV_ENUM))
    {
        Tango::detail::throw_err_data_type("max_alarm", d_name, name, "Attribute::set_max_alarm()");
    }

    else if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
            (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    //    Check coherence with min_alarm
    //

    auto &alarm_conf = is_alarmed();
    if(alarm_conf.test(Tango::Attribute::alarm_flags::min_level))
    {
        T min_alarm_tmp;
        memcpy((void *) &min_alarm_tmp, (const void *) &min_alarm, sizeof(T));
        if(new_max_alarm <= min_alarm_tmp)
        {
            Tango::detail::throw_incoherent_val_err(
                "min_alarm", "max_alarm", d_name, name, "Attribute::set_max_alarm()");
        }
    }

    //
    // Store new max alarm as a string
    //

    TangoSys_MemStream str;
    str.precision(Tango::TANGO_FLOAT_PRECISION);
    if(Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR)
    {
        str << (short) new_max_alarm; // to represent the numeric value
    }
    else
    {
        str << new_max_alarm;
    }
    std::string max_alarm_tmp_str;
    max_alarm_tmp_str = str.str();

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
    // Store the new alarm locally
    //

    Tango::Attr_CheckVal old_max_alarm;
    memcpy((void *) &old_max_alarm, (void *) &max_alarm, sizeof(T));
    memcpy((void *) &max_alarm, (void *) &new_max_alarm, sizeof(T));

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
            if(def_user_prop[i].get_name() == "max_alarm")
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
        if(user_defaults && max_alarm_tmp_str == usr_def_val)
        {
            Tango::DbDatum attr_dd(name), prop_dd("max_alarm");
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
                Tango::detail::upd_att_prop_db(max_alarm, "max_alarm", d_name, name, get_data_type());
            }
            catch(Tango::DevFailed &)
            {
                memcpy((void *) &max_alarm, (void *) &old_max_alarm, sizeof(T));
                throw;
            }
        }
    }

    //
    // Set the max_alarm flag
    //

    alarm_conf.set(Tango::Attribute::alarm_flags::max_level);

    //
    // Store new alarm as a string
    //

    max_alarm_str = max_alarm_tmp_str;

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }

    //
    // Delete device startup exception related to max_alarm if there is any
    //

    Tango::detail::delete_startup_exception(*this, "max_alarm", d_name, check_startup_exceptions, startup_exceptions);
}

template void Attribute::set_max_alarm(const Tango::DevBoolean &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevUChar &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevShort &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevUShort &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevLong &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevULong &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevLong64 &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevULong64 &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevFloat &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevDouble &new_max_alarm);
template void Attribute::set_max_alarm(const Tango::DevState &new_max_alarm);

template <>
void Attribute::set_max_alarm(const Tango::DevEncoded &)
{
    std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
    TANGO_THROW_EXCEPTION(API_MethodArgument, err_msg.c_str());
}

void Attribute::set_max_alarm(const std::string &new_max_alarm_str)
{
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE))
    {
        Tango::detail::throw_err_data_type("max_alarm", d_name, name, "Attribute::set_max_alarm()");
    }

    std::string max_alarm_str_tmp = new_max_alarm_str;

    Tango::DeviceClass *dev_class = get_att_device_class(d_name);
    Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
    Tango::Attr &att = mca->get_attr(name);
    std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
    std::vector<AttrProperty> &def_class_prop = att.get_class_properties();

    std::string usr_def_val;
    std::string class_def_val;
    bool user_defaults = false;
    bool class_defaults = false;

    user_defaults = Tango::detail::prop_in_list("max_alarm", usr_def_val, def_user_prop);

    class_defaults = Tango::detail::prop_in_list("max_alarm", class_def_val, def_class_prop);

    bool set_value = true;

    bool dummy;
    if(class_defaults)
    {
        if(TG_strcasecmp(new_max_alarm_str.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("max_alarm", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, max_alarm_str);
        }
        else if((TG_strcasecmp(new_max_alarm_str.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_max_alarm_str.c_str(), class_def_val.c_str()) == 0))
        {
            max_alarm_str_tmp = class_def_val;
        }
        else if(strlen(new_max_alarm_str.c_str()) == 0)
        {
            if(user_defaults)
            {
                max_alarm_str_tmp = usr_def_val;
            }
            else
            {
                set_value = false;

                Tango::detail::avns_in_db("max_alarm", name, d_name);
                Tango::detail::avns_in_att(*this, d_name, dummy, max_alarm_str);
            }
        }
    }
    else if(user_defaults)
    {
        if(TG_strcasecmp(new_max_alarm_str.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("max_alarm", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, max_alarm_str);
        }
        else if((TG_strcasecmp(new_max_alarm_str.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_max_alarm_str.c_str(), usr_def_val.c_str()) == 0) ||
                (strlen(new_max_alarm_str.c_str()) == 0))
        {
            max_alarm_str_tmp = usr_def_val;
        }
    }
    else
    {
        if((TG_strcasecmp(new_max_alarm_str.c_str(), AlrmValueNotSpec) == 0) ||
           (TG_strcasecmp(new_max_alarm_str.c_str(), NotANumber) == 0) || (strlen(new_max_alarm_str.c_str()) == 0))
        {
            set_value = false;

            Tango::detail::avns_in_db("max_alarm", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, max_alarm_str);
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
            str.precision(TANGO_FLOAT_PRECISION);
            str << max_alarm_str_tmp;
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                set_max_alarm((DevShort) db);
                break;

            case Tango::DEV_LONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                set_max_alarm((DevLong) db);
                break;

            case Tango::DEV_LONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                set_max_alarm((DevLong64) db);
                break;

            case Tango::DEV_DOUBLE:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                set_max_alarm(db);
                break;

            case Tango::DEV_FLOAT:
                if(!(str >> fl && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                set_max_alarm(fl);
                break;

            case Tango::DEV_USHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                (db < 0.0) ? set_max_alarm((DevUShort) (-db)) : set_max_alarm((DevUShort) db);
                break;

            case Tango::DEV_UCHAR:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                (db < 0.0) ? set_max_alarm((DevUChar) (-db)) : set_max_alarm((DevUChar) db);
                break;

            case Tango::DEV_ULONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                (db < 0.0) ? set_max_alarm((DevULong) (-db)) : set_max_alarm((DevULong) db);
                break;

            case Tango::DEV_ULONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                (db < 0.0) ? set_max_alarm((DevULong64) (-db)) : set_max_alarm((DevULong64) db);
                break;

            case Tango::DEV_ENCODED:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_alarm", d_name, name, "Attribute::set_max_alarm()");
                }
                (db < 0.0) ? set_max_alarm((DevUChar) (-db)) : set_max_alarm((DevUChar) db);
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("max_alarm", d_name, name, "Attribute::set_max_alarm()");
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        get_max_alarm()
//
// description :
//        Gets attribute's maximum alarm value and assigns it to the variable provided as a parameter
//        Throws exception in case the data type of provided parameter does not match the attribute data type
//        or if maximum alarm is not defined
//
// args :
//         out :
//            - max_al : The variable to be assigned the attribute's maximum alarm value
//
//-------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::get_max_alarm(T &max_al)
{
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << get_name()
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }
    else if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
            (data_type == Tango::DEV_ENUM))
    {
        std::stringstream err_msg;
        err_msg << "Maximum alarm has no meaning for the attribute's (" << get_name()
                << ") data type : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, err_msg.str().c_str());
    }

    auto &alarm_conf = is_alarmed();
    if(!alarm_conf[Tango::Attribute::alarm_flags::max_level])
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrNotAllowed, "Maximum alarm not defined for this attribute");
    }

    memcpy((void *) &max_al, (void *) &max_alarm, sizeof(T));
}

template void Attribute::get_max_alarm(Tango::DevBoolean &max_al);
template void Attribute::get_max_alarm(Tango::DevUChar &max_al);
template void Attribute::get_max_alarm(Tango::DevShort &max_al);
template void Attribute::get_max_alarm(Tango::DevUShort &max_al);
template void Attribute::get_max_alarm(Tango::DevLong &max_al);
template void Attribute::get_max_alarm(Tango::DevULong &max_al);
template void Attribute::get_max_alarm(Tango::DevLong64 &max_al);
template void Attribute::get_max_alarm(Tango::DevULong64 &max_al);
template void Attribute::get_max_alarm(Tango::DevFloat &max_al);
template void Attribute::get_max_alarm(Tango::DevDouble &max_al);
template void Attribute::get_max_alarm(Tango::DevState &max_al);
template void Attribute::get_max_alarm(Tango::DevString &max_al);
template void Attribute::get_max_alarm(Tango::DevEncoded &max_al);

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        set_min_warning()
//
// description :
//        Sets minimum warning attribute property
//        Throws exception in case the data type of provided property does not match the attribute data type
//
// args :
//         in :
//            - new_min_warning : The minimum warning property to be set
//
//-------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::set_min_warning(const T &new_min_warning)
{
    //
    // Check type validity
    //
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
       (data_type == Tango::DEV_ENUM))
    {
        Tango::detail::throw_err_data_type("min_warning", d_name, name, "Attribute::set_min_warning()");
    }

    else if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
            (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    //    Check coherence with max_warning
    //

    auto &alarm_conf = is_alarmed();
    if(alarm_conf.test(Tango::Attribute::alarm_flags::max_warn))
    {
        T max_warning_tmp;
        memcpy((void *) &max_warning_tmp, (const void *) &max_warning, sizeof(T));
        if(new_min_warning >= max_warning_tmp)
        {
            Tango::detail::throw_incoherent_val_err(
                "min_warning", "max_warning", d_name, name, "Attribute::set_min_warning()");
        }
    }

    //
    // Store new min warning as a string
    //

    TangoSys_MemStream str;
    str.precision(Tango::TANGO_FLOAT_PRECISION);
    if(Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR)
    {
        str << (short) new_min_warning; // to represent the numeric value
    }
    else
    {
        str << new_min_warning;
    }
    std::string min_warning_tmp_str;
    min_warning_tmp_str = str.str();

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
    // Store the new warning locally
    //

    Tango::Attr_CheckVal old_min_warning;
    memcpy((void *) &old_min_warning, (void *) &min_warning, sizeof(T));
    memcpy((void *) &min_warning, (void *) &new_min_warning, sizeof(T));

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
            if(def_user_prop[i].get_name() == "min_warning")
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
        if(user_defaults && min_warning_tmp_str == usr_def_val)
        {
            Tango::DbDatum attr_dd(name), prop_dd("min_warning");
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
                Tango::detail::upd_att_prop_db(min_warning, "min_warning", d_name, name, get_data_type());
            }
            catch(Tango::DevFailed &)
            {
                memcpy((void *) &min_warning, (void *) &old_min_warning, sizeof(T));
                throw;
            }
        }
    }

    //
    // Set the min_warn flag
    //

    alarm_conf.set(Tango::Attribute::alarm_flags::min_warn);

    //
    // Store new warning as a string
    //

    min_warning_str = min_warning_tmp_str;

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }

    //
    // Delete device startup exception related to min_warning if there is any
    //

    Tango::detail::delete_startup_exception(*this, "min_warning", d_name, check_startup_exceptions, startup_exceptions);
}

template void Attribute::set_min_warning(const Tango::DevBoolean &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevUChar &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevShort &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevUShort &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevLong &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevULong &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevLong64 &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevULong64 &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevFloat &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevDouble &new_min_warning);
template void Attribute::set_min_warning(const Tango::DevState &new_min_warning);

template <>
void Attribute::set_min_warning(const Tango::DevEncoded &)
{
    std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
    TANGO_THROW_EXCEPTION(API_MethodArgument, err_msg.c_str());
}

void Attribute::set_min_warning(const std::string &new_min_warning)
{
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE))
    {
        Tango::detail::throw_err_data_type("min_warning", d_name, name, "Attribute::set_min_warning()");
    }

    std::string min_warning_str_tmp = new_min_warning;

    Tango::DeviceClass *dev_class = get_att_device_class(d_name);
    Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
    Tango::Attr &att = mca->get_attr(name);
    std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
    std::vector<AttrProperty> &def_class_prop = att.get_class_properties();

    std::string usr_def_val;
    std::string class_def_val;
    bool user_defaults = false;
    bool class_defaults = false;

    user_defaults = Tango::detail::prop_in_list("min_warning", usr_def_val, def_user_prop);

    class_defaults = Tango::detail::prop_in_list("min_warning", class_def_val, def_class_prop);

    bool set_value = true;

    bool dummy;
    if(class_defaults)
    {
        if(TG_strcasecmp(new_min_warning.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("min_warning", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, min_warning_str);
        }
        else if((TG_strcasecmp(new_min_warning.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_min_warning.c_str(), class_def_val.c_str()) == 0))
        {
            min_warning_str_tmp = class_def_val;
        }
        else if(strlen(new_min_warning.c_str()) == 0)
        {
            if(user_defaults)
            {
                min_warning_str_tmp = usr_def_val;
            }
            else
            {
                set_value = false;

                Tango::detail::avns_in_db("min_warning", name, d_name);
                Tango::detail::avns_in_att(*this, d_name, dummy, min_warning_str);
            }
        }
    }
    else if(user_defaults)
    {
        if(TG_strcasecmp(new_min_warning.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("min_warning", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, min_warning_str);
        }
        else if((TG_strcasecmp(new_min_warning.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_min_warning.c_str(), usr_def_val.c_str()) == 0) ||
                (strlen(new_min_warning.c_str()) == 0))
        {
            min_warning_str_tmp = usr_def_val;
        }
    }
    else
    {
        if((TG_strcasecmp(new_min_warning.c_str(), AlrmValueNotSpec) == 0) ||
           (TG_strcasecmp(new_min_warning.c_str(), NotANumber) == 0) || (strlen(new_min_warning.c_str()) == 0))
        {
            set_value = false;

            Tango::detail::avns_in_db("min_warning", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, min_warning_str);
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
            str.precision(TANGO_FLOAT_PRECISION);
            str << min_warning_str_tmp;
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                set_min_warning((DevShort) db);
                break;

            case Tango::DEV_LONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                set_min_warning((DevLong) db);
                break;

            case Tango::DEV_LONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                set_min_warning((DevLong64) db);
                break;

            case Tango::DEV_DOUBLE:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                set_min_warning(db);
                break;

            case Tango::DEV_FLOAT:
                if(!(str >> fl && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                set_min_warning(fl);
                break;

            case Tango::DEV_USHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                (db < 0.0) ? set_min_warning((DevUShort) (-db)) : set_min_warning((DevUShort) db);
                break;

            case Tango::DEV_UCHAR:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                (db < 0.0) ? set_min_warning((DevUChar) (-db)) : set_min_warning((DevUChar) db);
                break;

            case Tango::DEV_ULONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                (db < 0.0) ? set_min_warning((DevULong) (-db)) : set_min_warning((DevULong) db);
                break;

            case Tango::DEV_ULONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                (db < 0.0) ? set_min_warning((DevULong64) (-db)) : set_min_warning((DevULong64) db);
                break;

            case Tango::DEV_ENCODED:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("min_warning", d_name, name, "Attribute::set_min_warning()");
                }
                (db < 0.0) ? set_min_warning((DevUChar) (-db)) : set_min_warning((DevUChar) db);
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("min_warning", d_name, name, "Attribute::set_min_warning()");
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        get_min_warning()
//
// description :
//        Gets attribute's minimum warning value and assigns it to the variable provided as a parameter
//        Throws exception in case the data type of provided parameter does not match the attribute data type
//        or if minimum warning is not defined
//
// args :
//         out :
//            - min_war : The variable to be assigned the attribute's minimum warning value
//
//-------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::get_min_warning(T &min_warn)
{
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << get_name()
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }
    else if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
            (data_type == Tango::DEV_ENUM))
    {
        std::stringstream err_msg;
        err_msg << "Minimum warning has no meaning for the attribute's (" << get_name()
                << ") data type : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, err_msg.str().c_str());
    }

    auto &alarm_conf = is_alarmed();
    if(!alarm_conf[Tango::Attribute::alarm_flags::min_warn])
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrNotAllowed, "Minimum warning not defined for this attribute");
    }

    memcpy((void *) &min_warn, (void *) &min_warning, sizeof(T));
}

template void Attribute::get_min_warning(Tango::DevBoolean &min_warn);
template void Attribute::get_min_warning(Tango::DevUChar &min_warn);
template void Attribute::get_min_warning(Tango::DevShort &min_warn);
template void Attribute::get_min_warning(Tango::DevUShort &min_warn);
template void Attribute::get_min_warning(Tango::DevLong &min_warn);
template void Attribute::get_min_warning(Tango::DevULong &min_warn);
template void Attribute::get_min_warning(Tango::DevLong64 &min_warn);
template void Attribute::get_min_warning(Tango::DevULong64 &min_warn);
template void Attribute::get_min_warning(Tango::DevFloat &min_warn);
template void Attribute::get_min_warning(Tango::DevDouble &min_warn);
template void Attribute::get_min_warning(Tango::DevState &min_warn);
template void Attribute::get_min_warning(Tango::DevString &min_warn);
template void Attribute::get_min_warning(Tango::DevEncoded &min_warn);

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        set_max_warning()
//
// description :
//        Sets maximum warning attribute property
//        Throws exception in case the data type of provided property does not match the attribute data type
//
// args :
//         in :
//            - new_max_warning : The maximum warning property to be set
//
//-----------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::set_max_warning(const T &new_max_warning)
{
    //
    // Check type validity
    //
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
       (data_type == Tango::DEV_ENUM))
    {
        Tango::detail::throw_err_data_type("max_warning", d_name, name, "Attribute::set_max_warning()");
    }

    else if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
            (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    //    Check coherence with min_warning
    //

    auto &alarm_conf = is_alarmed();
    if(alarm_conf.test(Tango::Attribute::alarm_flags::min_warn))
    {
        T min_warning_tmp;
        memcpy((void *) &min_warning_tmp, (const void *) &min_warning, sizeof(T));
        if(new_max_warning <= min_warning_tmp)
        {
            Tango::detail::throw_incoherent_val_err(
                "min_warning", "max_warning", d_name, name, "Attribute::set_max_warning()");
        }
    }

    //
    // Store new max warning as a string
    //

    TangoSys_MemStream str;
    str.precision(Tango::TANGO_FLOAT_PRECISION);
    if(Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR)
    {
        str << (short) new_max_warning; // to represent the numeric value
    }
    else
    {
        str << new_max_warning;
    }
    std::string max_warning_tmp_str;
    max_warning_tmp_str = str.str();

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
    // Store the new warning locally
    //

    Tango::Attr_CheckVal old_max_warning;
    memcpy((void *) &old_max_warning, (void *) &max_warning, sizeof(T));
    memcpy((void *) &max_warning, (void *) &new_max_warning, sizeof(T));

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
            if(def_user_prop[i].get_name() == "max_warning")
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
        if(user_defaults && max_warning_tmp_str == usr_def_val)
        {
            Tango::DbDatum attr_dd(name), prop_dd("max_warning");
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
                Tango::detail::upd_att_prop_db(max_warning, "max_warning", d_name, name, get_data_type());
            }
            catch(Tango::DevFailed &)
            {
                memcpy((void *) &max_warning, (void *) &old_max_warning, sizeof(T));
                throw;
            }
        }
    }

    //
    // Set the max_warn flag
    //

    alarm_conf.set(Tango::Attribute::alarm_flags::max_warn);

    //
    // Store new warning as a string
    //

    max_warning_str = max_warning_tmp_str;

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }

    //
    // Delete device startup exception related to max_warning if there is any
    //

    Tango::detail::delete_startup_exception(*this, "max_warning", d_name, check_startup_exceptions, startup_exceptions);
}

template void Attribute::set_max_warning(const Tango::DevBoolean &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevUChar &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevShort &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevUShort &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevLong &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevULong &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevLong64 &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevULong64 &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevFloat &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevDouble &new_max_warning);
template void Attribute::set_max_warning(const Tango::DevState &new_max_warning);

template <>
void Attribute::set_max_warning(const Tango::DevEncoded &)
{
    std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
    TANGO_THROW_EXCEPTION(API_MethodArgument, err_msg.c_str());
}

void Attribute::set_max_warning(const std::string &new_max_warning)
{
    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE))
    {
        Tango::detail::throw_err_data_type("max_warning", d_name, name, "Attribute::set_max_warning()");
    }

    std::string max_warning_str_tmp = new_max_warning;

    Tango::DeviceClass *dev_class = get_att_device_class(d_name);
    Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
    Tango::Attr &att = mca->get_attr(name);
    std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
    std::vector<AttrProperty> &def_class_prop = att.get_class_properties();

    std::string usr_def_val;
    std::string class_def_val;
    bool user_defaults = false;
    bool class_defaults = false;

    user_defaults = Tango::detail::prop_in_list("max_warning", usr_def_val, def_user_prop);

    class_defaults = Tango::detail::prop_in_list("max_warning", class_def_val, def_class_prop);

    bool set_value = true;

    bool dummy;
    if(class_defaults)
    {
        if(TG_strcasecmp(new_max_warning.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("max_warning", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, max_warning_str);
        }
        else if((TG_strcasecmp(new_max_warning.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_max_warning.c_str(), class_def_val.c_str()) == 0))
        {
            max_warning_str_tmp = class_def_val;
        }
        else if(strlen(new_max_warning.c_str()) == 0)
        {
            if(user_defaults)
            {
                max_warning_str_tmp = usr_def_val;
            }
            else
            {
                set_value = false;

                Tango::detail::avns_in_db("max_warning", name, d_name);
                Tango::detail::avns_in_att(*this, d_name, dummy, max_warning_str);
            }
        }
    }
    else if(user_defaults)
    {
        if(TG_strcasecmp(new_max_warning.c_str(), AlrmValueNotSpec) == 0)
        {
            set_value = false;

            Tango::detail::avns_in_db("max_warning", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, max_warning_str);
        }
        else if((TG_strcasecmp(new_max_warning.c_str(), NotANumber) == 0) ||
                (TG_strcasecmp(new_max_warning.c_str(), usr_def_val.c_str()) == 0) ||
                (strlen(new_max_warning.c_str()) == 0))
        {
            max_warning_str_tmp = usr_def_val;
        }
    }
    else
    {
        if((TG_strcasecmp(new_max_warning.c_str(), AlrmValueNotSpec) == 0) ||
           (TG_strcasecmp(new_max_warning.c_str(), NotANumber) == 0) || (strlen(new_max_warning.c_str()) == 0))
        {
            set_value = false;

            Tango::detail::avns_in_db("max_warning", name, d_name);
            Tango::detail::avns_in_att(*this, d_name, dummy, max_warning_str);
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
            str.precision(TANGO_FLOAT_PRECISION);
            str << max_warning_str_tmp;
            switch(data_type)
            {
            case Tango::DEV_SHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                set_max_warning((DevShort) db);
                break;

            case Tango::DEV_LONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                set_max_warning((DevLong) db);
                break;

            case Tango::DEV_LONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                set_max_warning((DevLong64) db);
                break;

            case Tango::DEV_DOUBLE:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                set_max_warning(db);
                break;

            case Tango::DEV_FLOAT:
                if(!(str >> fl && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                set_max_warning(fl);
                break;

            case Tango::DEV_USHORT:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                (db < 0.0) ? set_max_warning((DevUShort) (-db)) : set_max_warning((DevUShort) db);
                break;

            case Tango::DEV_UCHAR:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                (db < 0.0) ? set_max_warning((DevUChar) (-db)) : set_max_warning((DevUChar) db);
                break;

            case Tango::DEV_ULONG:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                (db < 0.0) ? set_max_warning((DevULong) (-db)) : set_max_warning((DevULong) db);
                break;

            case Tango::DEV_ULONG64:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                (db < 0.0) ? set_max_warning((DevULong64) (-db)) : set_max_warning((DevULong64) db);
                break;

            case Tango::DEV_ENCODED:
                if(!(str >> db && str.eof()))
                {
                    Tango::detail::throw_err_format("max_warning", d_name, name, "Attribute::set_max_warning()");
                }
                (db < 0.0) ? set_max_warning((DevUChar) (-db)) : set_max_warning((DevUChar) db);
                break;
            }
        }
        else
        {
            Tango::detail::throw_err_data_type("max_warning", d_name, name, "Attribute::set_max_warning()");
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        get_max_warning()
//
// description :
//        Gets attribute's maximum warning value and assigns it to the variable provided as a parameter
//        Throws exception in case the data type of provided parameter does not match the attribute data type
//        or if maximum warning is not defined
//
// args :
//         out :
//            - max_war : The variable to be assigned the attribute's maximum warning value
//
//-------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::get_max_warning(T &max_warn)
{
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << get_name()
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }
    else if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
            (data_type == Tango::DEV_ENUM))
    {
        std::stringstream err_msg;
        err_msg << "Maximum warning has no meaning for the attribute's (" << get_name()
                << ") data type : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, err_msg.str().c_str());
    }

    auto &alarm_conf = is_alarmed();
    if(!alarm_conf[Tango::Attribute::alarm_flags::max_warn])
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrNotAllowed, "Maximum warning not defined for this attribute");
    }

    memcpy((void *) &max_warn, (void *) &max_warning, sizeof(T));
}

template void Attribute::get_max_warning(Tango::DevBoolean &max_warn);
template void Attribute::get_max_warning(Tango::DevUChar &max_warn);
template void Attribute::get_max_warning(Tango::DevShort &max_warn);
template void Attribute::get_max_warning(Tango::DevUShort &max_warn);
template void Attribute::get_max_warning(Tango::DevLong &max_warn);
template void Attribute::get_max_warning(Tango::DevULong &max_warn);
template void Attribute::get_max_warning(Tango::DevLong64 &max_warn);
template void Attribute::get_max_warning(Tango::DevULong64 &max_warn);
template void Attribute::get_max_warning(Tango::DevFloat &max_warn);
template void Attribute::get_max_warning(Tango::DevDouble &max_warn);
template void Attribute::get_max_warning(Tango::DevState &max_warn);
template void Attribute::get_max_warning(Tango::DevString &max_warn);
template void Attribute::get_max_warning(Tango::DevEncoded &max_warn);

void Attribute::set_upd_properties(const AttributeConfig &attr_conf)
{
    set_upd_properties(attr_conf, d_name);
}

void Attribute::set_upd_properties(const AttributeConfig_3 &attr_conf)
{
    set_upd_properties(attr_conf, d_name);
}

void Attribute::set_upd_properties(const AttributeConfig_5 &attr_conf)
{
    set_upd_properties(attr_conf, d_name);
}

void Attribute::set_upd_properties(const AttributeConfig &attr_conf, const std::string &d_name, bool f_s)
{
    ::set_upd_properties(attr_conf,
                         d_name,
                         f_s,
                         startup_exceptions_clear,
                         *this,
                         min_value,
                         max_value,
                         min_alarm,
                         max_alarm,
                         min_warning,
                         max_warning,
                         check_min_value,
                         check_max_value);
}

void Attribute::set_upd_properties(const AttributeConfig_3 &attr_conf, const std::string &d_name, bool f_s)
{
    ::set_upd_properties(attr_conf,
                         d_name,
                         f_s,
                         startup_exceptions_clear,
                         *this,
                         min_value,
                         max_value,
                         min_alarm,
                         max_alarm,
                         min_warning,
                         max_warning,
                         check_min_value,
                         check_max_value);
}

void Attribute::set_upd_properties(const AttributeConfig_5 &attr_conf, const std::string &d_name, bool f_s)
{
    ::set_upd_properties(attr_conf,
                         d_name,
                         f_s,
                         startup_exceptions_clear,
                         *this,
                         min_value,
                         max_value,
                         min_alarm,
                         max_alarm,
                         min_warning,
                         max_warning,
                         check_min_value,
                         check_max_value);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        get_properties()
//
// description :
//        Gets attribute's properties in one call
//
// args :
//         out :
//            - props : The variable to be assigned the attribute's properties value
//
//-------------------------------------------------------------------------------------------------------------------
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::get_properties(MultiAttrProp<T> &props)
{
    //
    // Check data type
    //
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       !(data_type == Tango::DEV_ENUM && Tango::tango_type_traits<T>::type_value() == Tango::DEV_SHORT) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
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
    Tango::AutoTangoMonitor sync1(mon_ptr);

    Tango::AttributeConfig_5 conf;
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

template void Attribute::get_properties(MultiAttrProp<DevBoolean> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevUChar> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevShort> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevUShort> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevLong> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevULong> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevLong64> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevULong64> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevFloat> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevDouble> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevState> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevEncoded> &attr_prop);
template void Attribute::get_properties(MultiAttrProp<DevString> &attr_prop);

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
template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::set_properties(const MultiAttrProp<T> &props)
{
    //
    // Check data type
    //
    if(!(data_type == Tango::DEV_ENCODED && Tango::tango_type_traits<T>::type_value() == Tango::DEV_UCHAR) &&
       !(data_type == Tango::DEV_ENUM && Tango::tango_type_traits<T>::type_value() == Tango::DEV_SHORT) &&
       (data_type != Tango::tango_type_traits<T>::type_value()))
    {
        std::stringstream err_msg;
        err_msg << "Attribute (" << name
                << ") data type does not match the type provided : " << Tango::tango_type_traits<T>::type_value();
        TANGO_THROW_EXCEPTION(Tango::API_IncompatibleAttrDataType, err_msg.str().c_str());
    }

    //
    // Check if the user set values of properties which do not have any meaning for particular attribute data types
    //

    if((data_type == Tango::DEV_STRING) || (data_type == Tango::DEV_BOOLEAN) || (data_type == Tango::DEV_STATE) ||
       (data_type == Tango::DEV_ENUM))
    {
        if(TG_strcasecmp(props.min_alarm, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("min_alarm", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.max_alarm, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("max_alarm", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.min_value, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("min_value", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.max_value, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("max_value", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.min_warning, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("min_warning", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.max_warning, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("max_warning", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.delta_t, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("delta_t", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.delta_val, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("delta_val", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.rel_change, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("rel_change", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.abs_change, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("abs_change", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.archive_rel_change, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("archive_rel_change", d_name, name, "Attribute::set_properties()");
        }
        if(TG_strcasecmp(props.archive_abs_change, Tango::AlrmValueNotSpec) != 0)
        {
            Tango::detail::throw_err_data_type("archive_abs_change", d_name, name, "Attribute::set_properties()");
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
    Tango::AutoTangoMonitor sync1(mon_ptr);

    //
    // Get current attribute configuration (to retrieve un-mutable properties) and update properties with provided
    // values
    //

    Tango::AttributeConfig_5 conf;
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
        Tango::FwdAttribute *fwd_attr = static_cast<Tango::FwdAttribute *>(this);
        fwd_attr->upd_att_config_base(conf.label.in());
        fwd_attr->upd_att_config(conf);
    }
    else
    {
        ::set_upd_properties(conf,
                             d_name,
                             true,
                             startup_exceptions_clear,
                             *this,
                             min_value,
                             max_value,
                             min_alarm,
                             max_alarm,
                             min_warning,
                             max_warning,
                             check_min_value,
                             check_max_value);
    }

    //
    // Push a att conf event
    //

    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        get_att_device()->push_att_conf_event(this);
    }
}

template void Attribute::set_properties(const MultiAttrProp<DevBoolean> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevUChar> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevShort> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevUShort> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevLong> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevULong> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevLong64> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevULong64> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevFloat> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevDouble> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevState> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevEncoded> &attr_prop);
template void Attribute::set_properties(const MultiAttrProp<DevString> &attr_prop);

template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> *>
void Attribute::set_value(T *p_data, long x, long y, bool release)
{
    TANGO_LOG_DEBUG << "Attribute::set_value() called " << std::endl;

    using ArrayType = typename Tango::tango_type_traits<T>::ArrayType;

    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::tango_type_traits<T>::type_value())
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::tango_type_traits<T>::type_value() << " for attribute " << name
          << ". Attribute data type is " << (Tango::CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str());
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

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    data_size = Tango::detail::compute_data_size(data_format, dim_x, dim_y);
    quality = Tango::ATTR_VALID;

    //
    // Throw exception if pointer is null and data_size != 0
    //

    if(data_size != 0)
    {
        CHECK_PTR(p_data, name);
    }

    // Save data to proper seq
    if((data_format == Tango::SCALAR) && (release))
    {
        T *tmp_ptr = new T[1];
        *tmp_ptr = *p_data;
        attribute_value.set(std::make_unique<ArrayType>(data_size, data_size, tmp_ptr, release));
        delete_data_if_needed(p_data, release);
    }
    else
    {
        attribute_value.set(std::make_unique<ArrayType>(data_size, data_size, p_data, release));
    }
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
void Attribute::set_value(Tango::DevShort *p_data, long x, long y, bool release)
{
    TANGO_LOG_DEBUG << "Attribute::set_value() called " << std::endl;

    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::DEV_SHORT && data_type != Tango::DEV_ENUM)
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::DEV_SHORT << " for attribute " << name
          << ". Attribute data type is " << (Tango::CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str());
    }

    //
    // Check that data size is less than the given max
    //

    if((x > max_x) || (y > max_y))
    {
        delete_data_if_needed(p_data, release);

        std::stringstream ss;
        ss << "Data size for attribute " << name << " exceeds given limit";

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, ss.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    data_size = Tango::detail::compute_data_size(data_format, dim_x, dim_y);
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

    if(data_type == Tango::DEV_ENUM)
    {
        if(enum_labels.size() == 0)
        {
            delete_data_if_needed(p_data, release);

            std::stringstream ss;
            ss << "Attribute " << name << " data type is enum but no enum labels are defined!";

            TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, ss.str());
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

                TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, ss.str());
            }
        }
    }

    if((data_format == Tango::SCALAR) && (release))
    {
        Tango::DevShort *tmp_ptr = new Tango::DevShort[1];
        *tmp_ptr = *p_data;
        attribute_value.set(std::make_unique<Tango::DevVarShortArray>(data_size, data_size, tmp_ptr, release));
        delete_data_if_needed(p_data, release);
    }
    else
    {
        attribute_value.set(std::make_unique<Tango::DevVarShortArray>(data_size, data_size, p_data, release));
    }
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
void Attribute::set_value(Tango::DevString *p_data, long x, long y, bool release)
{
    TANGO_LOG_DEBUG << "Attribute::set_value() called " << std::endl;

    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::DEV_STRING)
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::DEV_STRING << " for attribute " << name
          << ". Attribute data type is " << (Tango::CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str());
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

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    data_size = Tango::detail::compute_data_size(data_format, dim_x, dim_y);
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
        attribute_value.set(std::make_unique<Tango::DevVarStringArray>(data_size, data_size, strvec, release));
    }
    else
    {
        attribute_value.set(std::make_unique<Tango::DevVarStringArray>(data_size, data_size, p_data, release));
    }

    delete_data_if_needed(p_data, release);
    //
    // Get time
    //

    set_time();
}

// Special set_value realisation for Tango::DevEncoded due to different memory management of Tango::DevString (aka
// char*[])
void Attribute::set_value(Tango::DevEncoded *p_data, long x, long y, bool release)
{
    TANGO_LOG_DEBUG << "Attribute::set_value() called " << std::endl;

    //
    // Throw exception if type is not correct
    //

    if(data_type != Tango::DEV_ENCODED)
    {
        delete_data_if_needed(p_data, release);

        std::stringstream o;

        o << "Invalid incoming data type " << Tango::DEV_ENCODED << " for attribute " << name
          << ". Attribute data type is " << (Tango::CmdArgType) data_type << std::ends;

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str());
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

        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str());
    }

    //
    // Compute data size and set default quality to valid.
    //

    dim_x = x;
    dim_y = y;
    data_size = Tango::detail::compute_data_size(data_format, dim_x, dim_y);
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
        Tango::DevEncoded *tmp_ptr = new Tango::DevEncoded[1];

        tmp_ptr->encoded_format = p_data->encoded_format;

        unsigned long nb_data = p_data->encoded_data.length();
        tmp_ptr->encoded_data.replace(nb_data, nb_data, p_data->encoded_data.get_buffer(true), true);
        p_data->encoded_data.replace(0, 0, nullptr, false);

        attribute_value.set(std::make_unique<Tango::DevVarEncodedArray>(data_size, data_size, tmp_ptr, release));
    }
    else
    {
        attribute_value.set(std::make_unique<Tango::DevVarEncodedArray>(data_size, data_size, p_data, release));
    }

    delete_data_if_needed(p_data, release);
    //
    // Reset alarm flags
    //

    alarm.reset();

    //
    // Get time
    //

    set_time();
}

template void Attribute::set_value(Tango::DevBoolean *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevFloat *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevDouble *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevState *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevUChar *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevUShort *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevLong *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevULong *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevLong64 *p_data, long x, long y, bool release);
template void Attribute::set_value(Tango::DevULong64 *p_data, long x, long y, bool release);
} // namespace Tango
