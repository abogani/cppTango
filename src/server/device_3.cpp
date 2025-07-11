//====================================================================================================================
//
// file :               Device_3.cpp
//
// description :        C++ source code for the DeviceImpl and DeviceClass classes. These classes are the root class
//                        for all derived Device classes. They are abstract classes. The DeviceImpl class is the
//                        CORBA servant which is "exported" onto the network and accessed by the client.
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//====================================================================================================================

#include <tango/server/device_3.h>
#include <tango/server/eventsupplier.h>
#include <tango/server/tango_clock.h>
#include <tango/server/fwdattribute.h>
#include <new>
#include <tango/internal/telemetry/telemetry_kernel_macros.h>
#include <tango/internal/utils.h>
#include <tango/client/Database.h>

namespace
{

#if defined(TANGO_USE_TELEMETRY)

void report_attr_error(const std::vector<std::string> &vec)
{
    if(vec.empty())
    {
        return;
    }

    std::stringstream sstr;
    sstr << "failed to read the following attribute(s): ";
    Tango::detail::stringify_vector(sstr, vec, ", ");
    TANGO_TELEMETRY_SET_ERROR_STATUS(sstr.str());
}

#endif // TANGO_USE_TELEMETRY
template <typename T>
void error_from_devfailed(T &back, Tango::DevFailed &e, const char *na)
{
    back.err_list = e.errors;
    back.quality = Tango::ATTR_INVALID;
    back.name = Tango::string_dup(na);
    clear_att_dim(back);
}

template <typename T>
void error_from_errorlist(T &back, Tango::DevErrorList &e, const char *na)
{
    back.err_list = e;
    back.quality = Tango::ATTR_INVALID;
    back.name = Tango::string_dup(na);
    clear_att_dim(back);
}

template <typename T>
void one_error(T &back, const char *reas, const char *ori, const std::string &mess, const char *na)
{
    back.err_list.length(1);

    back.err_list[0].severity = Tango::ERR;
    back.err_list[0].reason = Tango::string_dup(reas);
    back.err_list[0].origin = Tango::string_dup(ori);
    back.err_list[0].desc = Tango::string_dup(mess.c_str());

    back.quality = Tango::ATTR_INVALID;
    back.name = Tango::string_dup(na);
    clear_att_dim(back);
}

template <typename T>
void one_error(T &back, const char *reas, const char *ori, const std::string &mess, Tango::Attribute &att)
{
    one_error(back, reas, ori, mess, att.get_name().c_str());
}

template <typename T, typename V>
void init_polled_out_data(T &back, const V &att_val)
{
    back.quality = att_val.quality;
    back.time = att_val.time;
    back.r_dim = att_val.r_dim;
    back.w_dim = att_val.w_dim;
    back.name = Tango::string_dup(att_val.name);
}

template <typename T>
void init_out_data(T &back, Tango::Attribute &att, const Tango::AttrWriteType &w_type, Tango::MultiAttribute *dev_attr)
{
    back.time = att.get_when();
    back.quality = att.get_quality();
    back.name = Tango::string_dup(att.get_name().c_str());
    back.r_dim.dim_x = att.get_x();
    back.r_dim.dim_y = att.get_y();
    if((w_type == Tango::READ_WRITE) || (w_type == Tango::READ_WITH_WRITE))
    {
        Tango::WAttribute &assoc_att = dev_attr->get_w_attr_by_ind(att.get_assoc_ind());
        back.w_dim.dim_x = assoc_att.get_w_dim_x();
        back.w_dim.dim_y = assoc_att.get_w_dim_y();
    }
    else
    {
        if(w_type == Tango::WRITE)
        {
            // for write only attributes read and set value are the same!
            back.w_dim.dim_x = att.get_x();
            back.w_dim.dim_y = att.get_y();
        }
        else
        {
            // Tango::Read : read only attributes
            back.w_dim.dim_x = 0;
            back.w_dim.dim_y = 0;
        }
    }
}

template <typename T>
void init_out_data_quality(T &back, Tango::Attribute &att, Tango::AttrQuality qual)
{
    back.time = att.get_when();
    back.quality = qual;
    back.name = Tango::string_dup(att.get_name().c_str());
    back.r_dim.dim_x = att.get_x();
    back.r_dim.dim_y = att.get_y();
    back.r_dim.dim_x = 0;
    back.r_dim.dim_y = 0;
    back.w_dim.dim_x = 0;
    back.w_dim.dim_y = 0;
}

template <typename T>
void base_status2attr(T &back)
{
    back.time = Tango::make_TimeVal(std::chrono::system_clock::now());
    back.quality = Tango::ATTR_VALID;
    back.name = Tango::string_dup("Status");
    back.r_dim.dim_x = 1;
    back.r_dim.dim_y = 0;
    back.w_dim.dim_x = 0;
    back.w_dim.dim_y = 0;
}

template <typename T>
void base_state2attr(T &back)
{
    back.time = Tango::make_TimeVal(std::chrono::system_clock::now());
    back.quality = Tango::ATTR_VALID;
    back.name = Tango::string_dup("State");
    back.r_dim.dim_x = 1;
    back.r_dim.dim_y = 0;
    back.w_dim.dim_x = 0;
    back.w_dim.dim_y = 0;
}

//+---------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::set_attribute_config_3_local
//
// description :
//        Set attribute configuration for both AttributeConfig_3 and AttributeConfig_5
//
// args :
//        in :
//             - new_conf : The new attribute configuration
//            - dummy_arg : Dummy and unnused arg. Just to help template coding
//            - fwd_cb : Set to true if called from fwd att call back
//            - caller_idl : IDL release used by caller
//
//----------------------------------------------------------------------------------------------------------------

template <typename T, typename V>
void set_attribute_config_3_local(
    Tango::Device_3Impl *device, const T &new_conf, TANGO_UNUSED(const V &dummy_arg), bool fwd_cb, int caller_idl)
{
    TANGO_LOG_DEBUG << "Entering Device_3Impl::set_attribute_config_3_local" << std::endl;

    auto *dev_attr = device->get_device_attr();
    const auto &device_name = device->get_name();
    //
    // Return exception if the device does not have any attribute
    //

    long nb_dev_attr = dev_attr->get_attr_nb();
    if(nb_dev_attr == 0)
    {
        TANGO_THROW_EXCEPTION(Tango::API_AttrNotFound, "The device does not have any attribute");
    }

    //
    // Get some event related data
    //

    Tango::EventSupplier *event_supplier_nd = nullptr;
    Tango::EventSupplier *event_supplier_zmq = nullptr;

    Tango::Util *tg = Tango::Util::instance();

    //
    // Update attribute config first locally then in database
    //

    long nb_attr = new_conf.length();
    long i;

    Tango::EventSupplier::SuppliedEventData ad;
    ::memset(&ad, 0, sizeof(ad));

    try
    {
        for(i = 0; i < nb_attr; i++)
        {
            Tango::Attribute &attr = dev_attr->get_attr_by_name(new_conf[i].name);
            bool old_alarm = attr.is_alarmed().any();

            //
            // Special case for forwarded attributes
            //

            if(attr.is_fwd_att())
            {
                Tango::FwdAttribute &fwd_attr = static_cast<Tango::FwdAttribute &>(attr);
                if(fwd_cb)
                {
                    fwd_attr.set_att_config(new_conf[i]);
                }
                else
                {
                    fwd_attr.upd_att_config_base(new_conf[i].label.in());
                    fwd_attr.upd_att_config(new_conf[i]);
                }
            }
            else
            {
                attr.set_upd_properties(new_conf[i], device_name);
            }

            //
            // In case the attribute quality factor was set to ALARM, reset it to VALID
            //

            if((attr.get_quality() == Tango::ATTR_ALARM) && (old_alarm) && (!attr.is_alarmed().any()))
            {
                attr.set_quality(Tango::ATTR_VALID);
            }

            //
            // Send the event
            //

            if(attr.use_notifd_event())
            {
                event_supplier_nd = tg->get_notifd_event_supplier();
            }
            else
            {
                event_supplier_nd = nullptr;
            }

            if(attr.use_zmq_event())
            {
                event_supplier_zmq = tg->get_zmq_event_supplier();
            }
            else
            {
                event_supplier_zmq = nullptr;
            }

            if((event_supplier_nd != nullptr) || (event_supplier_zmq != nullptr))
            {
                std::string tmp_name(new_conf[i].name);

                //
                // The event data has to be the new attribute conf which could be different than the one we received (in
                // case some of the parameters are reset to lib/user/class default value)
                //

                V mod_conf;
                attr.get_prop(mod_conf);

                const V *tmp_ptr = &mod_conf;

                Tango::AttributeConfig_3 conf3;
                Tango::AttributeConfig_5 conf5;
                Tango::AttributeConfig_3 *tmp_conf_ptr;
                Tango::AttributeConfig_5 *tmp_conf_ptr5;

                if(device->get_dev_idl_version() > 4)
                {
                    std::vector<int> cl_lib = attr.get_client_lib(Tango::ATTR_CONF_EVENT);

                    if(caller_idl <= 4)
                    {
                        //
                        // Even if device is IDL 5, the change has been done from one old client (IDL4) thus with
                        // AttributeConfig_3. If a new client is listening to event, don't forget to send it.
                        //

                        for(size_t i = 0; i < cl_lib.size(); i++)
                        {
                            if(cl_lib[i] >= 5)
                            {
                                attr.AttributeConfig_3_2_AttributeConfig_5(mod_conf, conf5);
                                attr.add_config_5_specific(conf5);
                                tmp_conf_ptr5 = &conf5;

                                ::memcpy(&(ad.attr_conf_5), // NOLINT(bugprone-bitwise-pointer-cast)
                                         &(tmp_conf_ptr5),
                                         sizeof(V *));
                            }
                            else
                            {
                                ::memcpy( // NOLINT(bugprone-bitwise-pointer-cast)
                                    &(ad.attr_conf_3),
                                    &(tmp_ptr),
                                    sizeof(V *));
                            }

                            if(event_supplier_nd != nullptr)
                            {
                                event_supplier_nd->push_att_conf_events(
                                    device, ad, (Tango::DevFailed *) nullptr, tmp_name);
                            }
                            if(event_supplier_zmq != nullptr)
                            {
                                event_supplier_zmq->push_att_conf_events(
                                    device, ad, (Tango::DevFailed *) nullptr, tmp_name);
                            }

                            if(cl_lib[i] >= 5)
                            {
                                ad.attr_conf_5 = nullptr;
                            }
                            else
                            {
                                ad.attr_conf_3 = nullptr;
                            }
                        }
                    }
                    else
                    {
                        for(size_t i = 0; i < cl_lib.size(); i++)
                        {
                            if(cl_lib[i] < 5)
                            {
                                attr.AttributeConfig_5_2_AttributeConfig_3(mod_conf, conf3);
                                tmp_conf_ptr = &conf3;

                                ::memcpy(&(ad.attr_conf_3), // NOLINT(bugprone-bitwise-pointer-cast)
                                         &(tmp_conf_ptr),
                                         sizeof(V *));
                            }
                            else
                            {
                                ::memcpy( // NOLINT(bugprone-bitwise-pointer-cast)
                                    &(ad.attr_conf_5),
                                    &(tmp_ptr),
                                    sizeof(V *));
                            }

                            if(event_supplier_nd != nullptr)
                            {
                                event_supplier_nd->push_att_conf_events(
                                    device, ad, (Tango::DevFailed *) nullptr, tmp_name);
                            }
                            if(event_supplier_zmq != nullptr)
                            {
                                event_supplier_zmq->push_att_conf_events(
                                    device, ad, (Tango::DevFailed *) nullptr, tmp_name);
                            }

                            if(cl_lib[i] >= 5)
                            {
                                ad.attr_conf_5 = nullptr;
                            }
                            else
                            {
                                ad.attr_conf_3 = nullptr;
                            }
                        }
                    }
                }
                else
                {
                    ::memcpy(&(ad.attr_conf_3), &(tmp_ptr), sizeof(V *)); // NOLINT(bugprone-bitwise-pointer-cast)

                    if(event_supplier_nd != nullptr)
                    {
                        event_supplier_nd->push_att_conf_events(device, ad, (Tango::DevFailed *) nullptr, tmp_name);
                    }
                    if(event_supplier_zmq != nullptr)
                    {
                        event_supplier_zmq->push_att_conf_events(device, ad, (Tango::DevFailed *) nullptr, tmp_name);
                    }
                }
            }
        }
    }
    catch(Tango::DevFailed &e)
    {
        //
        // Re build the list of "alarmable" attribute
        //

        dev_attr->get_alarm_list().clear();
        for(long j = 0; j < nb_dev_attr; j++)
        {
            Tango::Attribute &att = dev_attr->get_attr_by_ind(j);
            if(att.is_alarmed().any())
            {
                if(att.get_writable() != Tango::WRITE)
                {
                    dev_attr->get_alarm_list().push_back(j);
                }
            }
        }

        //
        // Change the exception reason flag
        //

        TangoSys_OMemStream o;

        o << e.errors[0].reason;
        if(i != 0)
        {
            o << "\nAll previous attribute(s) have been successfully updated";
        }
        if(i != (nb_attr - 1))
        {
            o << "\nAll remaining attribute(s) have not been updated";
        }
        o << std::ends;

        std::string s = o.str();
        e.errors[0].reason = Tango::string_dup(s.c_str());
        throw;
    }

    //
    // Re build the list of "alarmable" attribute
    //

    dev_attr->get_alarm_list().clear();
    for(i = 0; i < nb_dev_attr; i++)
    {
        Tango::Attribute &attr = dev_attr->get_attr_by_ind(i);
        Tango::AttrWriteType w_type = attr.get_writable();
        if(attr.is_alarmed().any())
        {
            if(w_type != Tango::WRITE)
            {
                dev_attr->get_alarm_list().push_back(i);
            }
        }
    }

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving Device_3Impl::set_attribute_config_3_local" << std::endl;
}
} // namespace

namespace Tango
{

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::Device_3Impl
//
// description :
//        Constructors for the device_impl class from the class object pointer, the device name, the description field,
//        the state and the status. Device_3Impl inherits from DeviceImpl. These constructors simply call the correct
//        DeviceImpl class constructor
//
//--------------------------------------------------------------------------------------------------------------------

Device_3Impl::Device_3Impl(DeviceClass *device_class, const std::string &dev_name) :
    Device_2Impl(device_class, dev_name),
    ext_3(new Device_3ImplExt)
{
    real_ctor();
}

Device_3Impl::Device_3Impl(DeviceClass *device_class, const std::string &dev_name, const std::string &desc) :
    Device_2Impl(device_class, dev_name, desc),
    ext_3(new Device_3ImplExt)
{
    real_ctor();
}

Device_3Impl::Device_3Impl(DeviceClass *device_class,
                           const std::string &dev_name,
                           const std::string &desc,
                           Tango::DevState dev_state,
                           const std::string &dev_status) :
    Device_2Impl(device_class, dev_name, desc, dev_state, dev_status),
    ext_3(new Device_3ImplExt)
{
    real_ctor();
}

Device_3Impl::Device_3Impl(DeviceClass *device_class,
                           const char *dev_name,
                           const char *desc,
                           Tango::DevState dev_state,
                           const char *dev_status) :
    Device_2Impl(device_class, dev_name, desc, dev_state, dev_status),
    ext_3(new Device_3ImplExt)
{
    real_ctor();
}

void Device_3Impl::real_ctor()
{
    idl_version = 3;
    add_state_status_attrs();

    init_cmd_poll_period();
    init_attr_poll_period();

    Tango::Util *tg = Tango::Util::instance();
    if(!tg->use_db())
    {
        init_poll_no_db();
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::read_attributes_3
//
// description :
//        Method called for each read_attributes operation executed from any client on a Tango device version 3.
//
//--------------------------------------------------------------------------------------------------------------------

Tango::AttributeValueList_3 *Device_3Impl::read_attributes_3(const Tango::DevVarStringArray &names,
                                                             Tango::DevSource source)
{
    TANGO_LOG_DEBUG << "Device_3Impl::read_attributes_3 arrived for dev " << get_name() << ", att[0] = " << names[0]
                    << std::endl;

    //
    // Record operation request in black box
    //

    if(store_in_bb)
    {
        blackbox_ptr->insert_attr(names, idl_version, source);
    }
    store_in_bb = true;

    //
    // Build a sequence with the names of the attribute to be read. This is necessary in case of the "AllAttr" shortcut
    // is used. If all attributes are wanted, build this list
    //

    unsigned long nb_names = names.length();
    unsigned long nb_dev_attr = dev_attr->get_attr_nb();
    Tango::DevVarStringArray real_names(nb_names);
    unsigned long i;

    if(nb_names == 1)
    {
        std::string att_name(names[0]);
        if(att_name == AllAttr)
        {
            real_names.length(nb_dev_attr);
            for(i = 0; i < nb_dev_attr; i++)
            {
                real_names[i] = dev_attr->get_attr_by_ind(i).get_name().c_str();
            }
        }
        else
        {
            real_names = names;
        }
    }
    else
    {
        real_names = names;
    }
    nb_names = real_names.length();

    //
    // Allocate memory for the AttributeValue structures
    //

    AttributeIdlData aid;

    try
    {
        aid.data_3 = new Tango::AttributeValueList_3(nb_names);
        aid.data_3->length(nb_names);
    }
    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    //
    // If the source parameter specifies device, call the read_attributes method which does not throw exception except
    // for major fault (cannot allocate memory,....)
    //

    std::vector<long> idx_in_back;

    if(source == Tango::DEV)
    {
        try
        {
            AutoTangoMonitor sync(this);
            read_attributes_no_except(real_names, aid, false, idx_in_back);
        }
        catch(...)
        {
            delete aid.data_3;
            throw;
        }
    }
    else if(source == Tango::CACHE)
    {
        try
        {
            TangoMonitor &mon = get_poll_monitor();
            AutoTangoMonitor sync(&mon);
            read_attributes_from_cache(real_names, aid);
        }
        catch(...)
        {
            delete aid.data_3;
            throw;
        }
    }
    else
    {
        //
        // It must be now CACHE_DEVICE (no other choice), first try to get values from cache
        //

        try
        {
            TangoMonitor &mon = get_poll_monitor();
            AutoTangoMonitor sync(&mon);
            read_attributes_from_cache(real_names, aid);
        }
        catch(...)
        {
            delete aid.data_3;
            throw;
        }

        //
        // Now, build the list of attributes which it was not possible to get their value from cache
        //

        Tango::DevVarStringArray names_from_device(nb_names);
        long nb_attr = 0;

        for(i = 0; i < nb_names; i++)
        {
            long nb_err = (*aid.data_3)[i].err_list.length();
            if(nb_err != 0)
            {
                nb_err--;
                if((strcmp((*aid.data_3)[i].err_list[nb_err].reason, API_AttrNotPolled) == 0) ||
                   (strcmp((*aid.data_3)[i].err_list[nb_err].reason, API_NoDataYet) == 0) ||
                   (strcmp((*aid.data_3)[i].err_list[nb_err].reason, API_NotUpdatedAnyMore) == 0) ||
                   (strcmp((*aid.data_3)[i].err_list[nb_err].origin, "DServer::add_obj_polling") == 0))
                {
                    nb_attr++;
                    names_from_device.length(nb_attr);
                    names_from_device[nb_attr - 1] = real_names[i];
                    idx_in_back.push_back(i);

                    (*aid.data_3)[i].err_list.length(0);
                }
            }
        }

        if(nb_attr != 0)
        {
            //
            // Try to get their values from device
            //

            try
            {
                AutoTangoMonitor sync(this);
                read_attributes_no_except(names_from_device, aid, true, idx_in_back);
            }
            catch(...)
            {
                delete aid.data_3;
                throw;
            }
        }
    }

    return aid.data_3;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::read_attributes_no_except
//
// description :
//        Read attributes from device but do not throw exception if it fails. This method is mainly a copy of the
//        original DeviceImpl::read_attributes method.
//
// arguments:
//        in :
//            - names: The names of the attribute to read
//            - aid : Structure with pointers for data to be returned to caller (several pointers acoording to
//                    caller IDL level)
//            - second_try : Flag set to true when this method is called due to a client request with source set to
//                           CACHE_DEVICE and the reading the value from the cache failed for one reason or another
//            - idx : Vector with index in back array for which we have to read data (to store error at the right
//                    place in the array for instance)
//
//-------------------------------------------------------------------------------------------------------------------

void Device_3Impl::read_attributes_no_except(const Tango::DevVarStringArray &names,
                                             Tango::AttributeIdlData &aid,
                                             bool second_try,
                                             std::vector<long> &idx)
{
    //
    //  Write the device name into the per thread data for sub device diagnostics.
    //  Keep the old name, to put it back at the end!
    //  During device access inside the same server, the thread stays the same!
    //

    SubDevDiag &sub = (Tango::Util::instance())->get_sub_dev_diag();
    std::string last_associated_device = sub.get_associated_device();
    sub.set_associated_device(get_name());

    //
    // Catch all exceptions to set back the associated device after execution
    //

#if defined(TANGO_USE_TELEMETRY)
    // Reference comment for multiple locations:
    // Due to the current implementation of Tango, errors related to attribute read (or writing) are tricky to trace
    // with details (attributes name). We choose to build a list of "bad attributes" (i.e. the list of attributes we
    // failed to read or write) then set the status of current telemetry span in case that list is not empty when we
    // reach the end of the method. The implementation is so that we to put some code at several locations to do the
    // job. That's ugly but no choice, we have to deal with that.
    std::vector<std::string> bad_attributes;
#endif

    try
    {
        //
        // Retrieve index of wanted attributes in the device attribute list and clear their value set flag
        //

        long nb_names = names.length();
        std::vector<AttIdx> wanted_attr;
        std::vector<AttIdx> wanted_w_attr;
        bool state_wanted = false;
        bool status_wanted = false;
        long state_idx, status_idx;
        long i;

        state_idx = status_idx = -1;

        for(i = 0; i < nb_names; i++)
        {
            AttIdx x;
            x.idx_in_names = i;
            std::string att_name(names[i]);
            std::transform(att_name.begin(), att_name.end(), att_name.begin(), ::tolower);

            if(att_name == "state")
            {
                x.idx_in_multi_attr = -1;
                x.failed = false;
                wanted_attr.push_back(x);
                state_wanted = true;
                state_idx = i;
            }
            else if(att_name == "status")
            {
                x.idx_in_multi_attr = -1;
                x.failed = false;
                wanted_attr.push_back(x);
                status_wanted = true;
                status_idx = i;
            }
            else
            {
                try
                {
                    long j;

                    j = dev_attr->get_attr_ind_by_name(names[i]);
                    if((dev_attr->get_attr_by_ind(j).get_writable() == Tango::READ_WRITE) ||
                       (dev_attr->get_attr_by_ind(j).get_writable() == Tango::READ_WITH_WRITE))
                    {
                        x.idx_in_multi_attr = j;
                        x.failed = false;
                        Attribute &att = dev_attr->get_attr_by_ind(x.idx_in_multi_attr);
                        if(att.is_startup_exception())
                        {
                            att.throw_startup_exception("Device_3Impl::read_attributes_no_except()");
                        }
                        wanted_w_attr.push_back(x);
                        wanted_attr.push_back(x);
                        att.get_when().tv_sec = 0;
                        att.save_alarm_quality();
                    }
                    else
                    {
                        if(dev_attr->get_attr_by_ind(j).get_writable() == Tango::WRITE)
                        {
                            //
                            // If the attribute is a forwarded one, force reading it from  the root device. Another
                            // client could have written its value
                            //

                            if(dev_attr->get_attr_by_ind(j).is_fwd_att())
                            {
                                x.idx_in_multi_attr = j;
                                x.failed = false;
                                Attribute &att = dev_attr->get_attr_by_ind(x.idx_in_multi_attr);
                                if(att.is_startup_exception())
                                {
                                    att.throw_startup_exception("Device_3Impl::read_attributes_no_except()");
                                }
                                wanted_attr.push_back(x);
                                att.get_when().tv_sec = 0;
                                att.save_alarm_quality();
                            }
                            else
                            {
                                x.idx_in_multi_attr = j;
                                x.failed = false;
                                Attribute &att = dev_attr->get_attr_by_ind(x.idx_in_multi_attr);
                                if(att.is_startup_exception())
                                {
                                    att.throw_startup_exception("Device_3Impl::read_attributes_no_except()");
                                }
                                wanted_w_attr.push_back(x);
                            }
                        }
                        else
                        {
                            x.idx_in_multi_attr = j;
                            x.failed = false;
                            Attribute &att = dev_attr->get_attr_by_ind(x.idx_in_multi_attr);
                            if(att.is_startup_exception())
                            {
                                att.throw_startup_exception("Device_3Impl::read_attributes_no_except()");
                            }
                            wanted_attr.push_back(x);
                            att.get_when().tv_sec = 0;
                            att.save_alarm_quality();
                        }
                    }
                }
                catch(Tango::DevFailed &e)
                {
                    long index;
                    if(!second_try)
                    {
                        index = i;
                    }
                    else
                    {
                        index = idx[i];
                    }

                    if(aid.data_5 != nullptr)
                    {
                        error_from_devfailed((*aid.data_5)[index], e, names[i]);
                    }
                    else if(aid.data_4 != nullptr)
                    {
                        error_from_devfailed((*aid.data_4)[index], e, names[i]);
                    }
                    else
                    {
                        error_from_devfailed((*aid.data_3)[index], e, names[i]);
                    }
                    TANGO_TELEMETRY_TRACK_BAD_ATTR(names[i].in());
                }
            }
        }

        long nb_wanted_attr = wanted_attr.size();
        long nb_wanted_w_attr = wanted_w_attr.size();

        //
        // Call the always_executed_hook
        //

        always_executed_hook();

        //
        // Read the hardware for readable attribute but not for state/status
        // Warning:  If the state is one of the wanted attribute, check and eventually add all the alarmed attributes
        // index
        //

        if(nb_wanted_attr != 0)
        {
            std::vector<long> tmp_idx;
            for(i = 0; i < nb_wanted_attr; i++)
            {
                long ii = wanted_attr[i].idx_in_multi_attr;
                if(ii != -1)
                {
                    tmp_idx.push_back(ii);
                }
            }
            if(state_wanted)
            {
                if((device_state == Tango::ON) || (device_state == Tango::ALARM))
                {
                    add_alarmed(tmp_idx);
                }
            }

            if(!tmp_idx.empty())
            {
                read_attr_hardware(tmp_idx);
            }
        }

        //
        // Set attr value (for readable attribute) but not for state/status
        //

        for(i = 0; i < nb_wanted_attr; i++)
        {
            if(wanted_attr[i].idx_in_multi_attr != -1)
            {
                Attribute &att = dev_attr->get_attr_by_ind(wanted_attr[i].idx_in_multi_attr);
                bool is_allowed_failed = false;

                try
                {
                    std::vector<Tango::Attr *> &attr_vect = device_class->get_class_attr()->get_attr_list();
                    if(!attr_vect[att.get_attr_idx()]->is_allowed(this, Tango::READ_REQ))
                    {
                        is_allowed_failed = true;
                        TangoSys_OMemStream o;

                        o << "It is currently not allowed to read attribute ";
                        o << att.get_name() << std::ends;

                        TANGO_THROW_EXCEPTION(API_AttrNotAllowed, o.str());
                    }

                    //
                    // Take the attribute mutex before calling the user read method
                    //

                    if((att.get_attr_serial_model() == ATTR_BY_KERNEL) &&
                       (aid.data_4 != nullptr || aid.data_5 != nullptr))
                    {
                        TANGO_LOG_DEBUG << "Locking attribute mutex for attribute " << att.get_name() << std::endl;
                        omni_mutex *attr_mut = att.get_attr_mutex();
                        if(attr_mut->trylock() == 0)
                        {
                            TANGO_LOG_DEBUG << "Mutex for attribute " << att.get_name() << " is already taken.........."
                                            << std::endl;
                            attr_mut->lock();
                        }
                    }

                    //
                    // Call the user read method except if the attribute is writable and memorized and if the write
                    // failed during the device startup sequence
                    //

                    att.reset_value();

                    if(!att.is_mem_exception())
                    {
                        attr_vect[att.get_attr_idx()]->read(this, att);
                    }
                    else
                    {
                        Tango::WAttribute &w_att = static_cast<Tango::WAttribute &>(att);
                        Tango::DevFailed df(w_att.get_mem_exception());

                        TangoSys_OMemStream o;
                        o << "Attribute " << w_att.get_name() << " is a memorized attribute.";
                        o << " It failed during the write call of the device startup sequence";
                        TANGO_RETHROW_EXCEPTION(df, API_MemAttFailedDuringInit, o.str());
                    }

                    //
                    // Check alarm
                    //

                    if((att.is_alarmed().any()) && (att.get_quality() != Tango::ATTR_INVALID))
                    {
                        att.check_alarm();
                    }
                }
                catch(Tango::DevFailed &e)
                {
                    long index;
                    if(!second_try)
                    {
                        index = wanted_attr[i].idx_in_names;
                    }
                    else
                    {
                        index = idx[wanted_attr[i].idx_in_names];
                    }

                    wanted_attr[i].failed = true;

                    if(aid.data_5 != nullptr)
                    {
                        if((att.get_attr_serial_model() == ATTR_BY_KERNEL) && (!is_allowed_failed))
                        {
                            TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                            << " due to error" << std::endl;
                            omni_mutex *attr_mut = att.get_attr_mutex();
                            attr_mut->unlock();
                        }
                        error_from_devfailed((*aid.data_5)[index], e, names[wanted_attr[i].idx_in_names]);
                    }
                    else if(aid.data_4 != nullptr)
                    {
                        if((att.get_attr_serial_model() == ATTR_BY_KERNEL) && (!is_allowed_failed))
                        {
                            TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                            << " due to error" << std::endl;
                            omni_mutex *attr_mut = att.get_attr_mutex();
                            attr_mut->unlock();
                        }
                        error_from_devfailed((*aid.data_4)[index], e, names[wanted_attr[i].idx_in_names]);
                    }
                    else
                    {
                        error_from_devfailed((*aid.data_3)[index], e, names[wanted_attr[i].idx_in_names]);
                    }
                    TANGO_TELEMETRY_TRACK_BAD_ATTR(names[wanted_attr[i].idx_in_names].in());
                }
                catch(...)
                {
                    long index;
                    if(!second_try)
                    {
                        index = wanted_attr[i].idx_in_names;
                    }
                    else
                    {
                        index = idx[wanted_attr[i].idx_in_names];
                    }

                    wanted_attr[i].failed = true;
                    Tango::DevErrorList del;
                    del.length(1);

                    del[0].severity = Tango::ERR;
                    del[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
                    del[0].reason = Tango::string_dup(API_CorbaSysException);
                    del[0].desc = Tango::string_dup("Unforseen exception when trying to read attribute. It was even "
                                                    "not a Tango DevFailed exception");

                    if(aid.data_5 != nullptr)
                    {
                        if((att.get_attr_serial_model() == ATTR_BY_KERNEL) && (!is_allowed_failed))
                        {
                            TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                            << " due to a severe error which is not a DevFailed" << std::endl;
                            omni_mutex *attr_mut = att.get_attr_mutex();
                            attr_mut->unlock();
                        }
                        error_from_errorlist((*aid.data_5)[index], del, names[wanted_attr[i].idx_in_names]);
                    }
                    else if(aid.data_4 != nullptr)
                    {
                        if((att.get_attr_serial_model() == ATTR_BY_KERNEL) && (!is_allowed_failed))
                        {
                            TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                            << " due to severe error which is not a DevFailed" << std::endl;
                            omni_mutex *attr_mut = att.get_attr_mutex();
                            attr_mut->unlock();
                        }
                        error_from_errorlist((*aid.data_4)[index], del, names[wanted_attr[i].idx_in_names]);
                    }
                    else
                    {
                        error_from_errorlist((*aid.data_3)[index], del, names[wanted_attr[i].idx_in_names]);
                    }
                    TANGO_TELEMETRY_TRACK_BAD_ATTR(names[wanted_attr[i].idx_in_names].in());
                }
            }
        }

        //
        // Set attr value for writable attribute
        //

        for(i = 0; i < nb_wanted_w_attr; i++)
        {
            Attribute &att = dev_attr->get_attr_by_ind(wanted_w_attr[i].idx_in_multi_attr);
            Tango::AttrWriteType w_type = att.get_writable();
            try
            {
                if(att.is_mem_exception())
                {
                    Tango::WAttribute &w_att = static_cast<Tango::WAttribute &>(att);
                    Tango::DevFailed df(w_att.get_mem_exception());

                    TangoSys_OMemStream o;
                    o << "Attribute " << w_att.get_name() << " is a memorized attribute.";
                    o << " It failed during the write call of the device startup sequence";
                    TANGO_RETHROW_EXCEPTION(df, API_MemAttFailedDuringInit, o.str());
                }
                else
                {
                    if((w_type == Tango::READ_WITH_WRITE) || (w_type == Tango::WRITE))
                    {
                        att.set_rvalue();
                    }
                }
            }
            catch(Tango::DevFailed &e)
            {
                long index;
                if(!second_try)
                {
                    index = wanted_w_attr[i].idx_in_names;
                }
                else
                {
                    index = idx[wanted_w_attr[i].idx_in_names];
                }

                wanted_w_attr[i].failed = true;
                AttrSerialModel atsm = att.get_attr_serial_model();

                if(aid.data_5 != nullptr)
                {
                    if((atsm != ATTR_NO_SYNC) && (w_type == Tango::READ_WITH_WRITE))
                    {
                        TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                        << " due to error" << std::endl;
                        omni_mutex *attr_mut =
                            (atsm == ATTR_BY_KERNEL) ? att.get_attr_mutex() : att.get_user_attr_mutex();
                        attr_mut->unlock();
                    }
                    error_from_devfailed((*aid.data_5)[index], e, names[wanted_w_attr[i].idx_in_names]);
                }
                else if(aid.data_4 != nullptr)
                {
                    if((atsm != ATTR_NO_SYNC) && (w_type == Tango::READ_WITH_WRITE))
                    {
                        TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                        << " due to error" << std::endl;
                        omni_mutex *attr_mut =
                            (atsm == ATTR_BY_KERNEL) ? att.get_attr_mutex() : att.get_user_attr_mutex();
                        attr_mut->unlock();
                    }
                    error_from_devfailed((*aid.data_4)[index], e, names[wanted_w_attr[i].idx_in_names]);
                }
                else
                {
                    error_from_devfailed((*aid.data_3)[index], e, names[wanted_w_attr[i].idx_in_names]);
                }
                TANGO_TELEMETRY_TRACK_BAD_ATTR(names[wanted_w_attr[i].idx_in_names].in());
            }
        }

        //
        // If necessary, read state and/or status
        // If the device has some alarmed attributes and some of them have already been read and failed, it is not
        // necessary to read state, simply copy faulty alarmed attribute error message to the state attribute error
        // messages
        //

        Tango::DevState d_state = Tango::UNKNOWN;
        Tango::ConstDevString d_status = nullptr;

        if(state_wanted)
        {
            try
            {
                alarmed_not_read(wanted_attr);
                state_from_read = true;
                if(is_alarm_state_forced())
                {
                    d_state = DeviceImpl::dev_state();
                }
                else
                {
                    d_state = dev_state();
                }
                state_from_read = false;
            }
            catch(Tango::DevFailed &e)
            {
                state_from_read = false;
                if(aid.data_5 != nullptr)
                {
                    error_from_devfailed((*aid.data_5)[state_idx], e, names[state_idx]);
                }
                else if(aid.data_4 != nullptr)
                {
                    error_from_devfailed((*aid.data_4)[state_idx], e, names[state_idx]);
                }
                else
                {
                    error_from_devfailed((*aid.data_3)[state_idx], e, names[state_idx]);
                }
                TANGO_TELEMETRY_TRACK_BAD_ATTR(names[state_idx].in());
            }
        }

        if(status_wanted)
        {
            try
            {
                if(is_alarm_state_forced())
                {
                    d_status = DeviceImpl::dev_status();
                }
                else
                {
                    d_status = dev_status();
                }
            }
            catch(Tango::DevFailed &e)
            {
                if(aid.data_5 != nullptr)
                {
                    error_from_devfailed((*aid.data_5)[status_idx], e, names[status_idx]);
                }
                else if(aid.data_4 != nullptr)
                {
                    error_from_devfailed((*aid.data_4)[status_idx], e, names[status_idx]);
                }
                else
                {
                    error_from_devfailed((*aid.data_3)[status_idx], e, names[status_idx]);
                }
                TANGO_TELEMETRY_TRACK_BAD_ATTR(names[status_idx].in());
            }
        }

        //
        // Build the sequence returned to caller for readable attributes and check that all the wanted attributes set
        // value have been updated
        //

        for(i = 0; i < nb_names; i++)
        {
            long index;
            if(!second_try)
            {
                index = i;
            }
            else
            {
                index = idx[i];
            }

            unsigned long nb_err;
            if(aid.data_5 != nullptr)
            {
                nb_err = (*aid.data_5)[index].err_list.length();
            }
            else if(aid.data_4 != nullptr)
            {
                nb_err = (*aid.data_4)[index].err_list.length();
            }
            else
            {
                nb_err = (*aid.data_3)[index].err_list.length();
            }

            if((state_wanted) && (state_idx == i))
            {
                if(aid.data_5 != nullptr)
                {
                    if(nb_err == 0)
                    {
                        state2attr(d_state, (*aid.data_5)[index]);
                    }
                }
                else if(aid.data_4 != nullptr)
                {
                    if(nb_err == 0)
                    {
                        state2attr(d_state, (*aid.data_4)[index]);
                    }
                }
                else
                {
                    if(nb_err == 0)
                    {
                        state2attr(d_state, (*aid.data_3)[index]);
                    }
                }
                continue;
            }

            if((status_wanted) && (status_idx == i))
            {
                if(aid.data_5 != nullptr)
                {
                    if(nb_err == 0)
                    {
                        status2attr(d_status, (*aid.data_5)[index]);
                    }
                }
                else if(aid.data_4 != nullptr)
                {
                    if(nb_err == 0)
                    {
                        status2attr(d_status, (*aid.data_4)[index]);
                    }
                }
                else
                {
                    if(nb_err == 0)
                    {
                        status2attr(d_status, (*aid.data_3)[index]);
                    }
                }
                continue;
            }

            if(nb_err == 0)
            {
                Attribute &att = dev_attr->get_attr_by_name(names[i]);
                Tango::AttrQuality qual = att.get_quality();
                if(qual != Tango::ATTR_INVALID)
                {
                    if(!att.value_is_set())
                    {
                        TangoSys_OMemStream o;

                        try
                        {
                            std::string att_name(names[i]);
                            std::transform(att_name.begin(), att_name.end(), att_name.begin(), ::tolower);

                            auto ite = get_polled_obj_by_type_name(Tango::POLL_ATTR, att_name);
                            auto upd = (*ite)->get_upd();
                            if(upd == PollClock::duration::zero())
                            {
                                o << "Attribute ";
                                o << att.get_name();
                                o << " value is available only by CACHE.\n";
                                o << "Attribute values are set by external polling buffer filling" << std::ends;
                            }
                            else
                            {
                                o << "Read value for attribute ";
                                o << att.get_name();
                                o << " has not been updated" << std::ends;
                            }
                        }
                        catch(Tango::DevFailed &)
                        {
                            o << "Read value for attribute ";
                            o << att.get_name();
                            o << " has not been updated" << std::ends;
                        }

                        std::string s = o.str();

                        const char *reas = API_AttrValueNotSet;
                        AttrSerialModel atsm = att.get_attr_serial_model();

                        if(aid.data_5 != nullptr)
                        {
                            if((i != state_idx) && (i != status_idx) && (atsm != ATTR_NO_SYNC) &&
                               (att.get_writable() != Tango::WRITE))
                            {
                                TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                                << " due to error" << std::endl;
                                omni_mutex *attr_mut =
                                    (atsm == ATTR_BY_KERNEL) ? att.get_attr_mutex() : att.get_user_attr_mutex();
                                attr_mut->unlock();
                            }
                            one_error((*aid.data_5)[index], reas, TANGO_EXCEPTION_ORIGIN, s, att);
                        }
                        else if(aid.data_4 != nullptr)
                        {
                            if((i != state_idx) && (i != status_idx) && (atsm != ATTR_NO_SYNC) &&
                               (att.get_writable() != Tango::WRITE))
                            {
                                TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                                << " due to error" << std::endl;
                                omni_mutex *attr_mut =
                                    (atsm == ATTR_BY_KERNEL) ? att.get_attr_mutex() : att.get_user_attr_mutex();
                                attr_mut->unlock();
                            }
                            one_error((*aid.data_4)[index], reas, TANGO_EXCEPTION_ORIGIN, s, att);
                        }
                        else
                        {
                            one_error((*aid.data_3)[index], reas, TANGO_EXCEPTION_ORIGIN, s, att);
                        }
                    }
                    else
                    {
                        try
                        {
                            Tango::AttrWriteType w_type = att.get_writable();
                            if((w_type == Tango::READ) || (w_type == Tango::READ_WRITE) ||
                               (w_type == Tango::READ_WITH_WRITE))
                            {
                                if((w_type == Tango::READ_WRITE) || (w_type == Tango::READ_WITH_WRITE))
                                {
                                    dev_attr->add_write_value(att);
                                }
                            }

                            //
                            // Data into the network object
                            //

                            data_into_net_object(att, aid, index, w_type, true);

                            //
                            // Init remaining elements
                            //

                            if(att.get_when().tv_sec == 0)
                            {
                                att.set_time();
                            }

                            AttrSerialModel atsm = att.get_attr_serial_model();
                            if(aid.data_5 != nullptr)
                            {
                                if((atsm != ATTR_NO_SYNC) && ((att.is_fwd_att()) || (w_type != Tango::WRITE)))
                                {
                                    TANGO_LOG_DEBUG << "Giving attribute mutex to CORBA structure for attribute "
                                                    << att.get_name() << std::endl;
                                    if(atsm == ATTR_BY_KERNEL)
                                    {
                                        GIVE_ATT_MUTEX_5(aid.data_5, index, att);
                                    }
                                    else
                                    {
                                        GIVE_USER_ATT_MUTEX_5(aid.data_5, index, att);
                                    }
                                }
                                init_out_data((*aid.data_5)[index], att, w_type, dev_attr);
                                (*aid.data_5)[index].data_format = att.get_data_format();
                                (*aid.data_5)[index].data_type = att.get_data_type();
                            }
                            else if(aid.data_4 != nullptr)
                            {
                                if((atsm != ATTR_NO_SYNC) && ((att.is_fwd_att()) || (w_type != Tango::WRITE)))
                                {
                                    TANGO_LOG_DEBUG << "Giving attribute mutex to CORBA structure for attribute "
                                                    << att.get_name() << std::endl;
                                    if(atsm == ATTR_BY_KERNEL)
                                    {
                                        GIVE_ATT_MUTEX(aid.data_4, index, att);
                                    }
                                    else
                                    {
                                        GIVE_USER_ATT_MUTEX(aid.data_4, index, att);
                                    }
                                }
                                init_out_data((*aid.data_4)[index], att, w_type, dev_attr);
                                (*aid.data_4)[index].data_format = att.get_data_format();
                            }
                            else
                            {
                                init_out_data((*aid.data_3)[index], att, w_type, dev_attr);
                            }
                        }
                        catch(Tango::DevFailed &e)
                        {
                            if(aid.data_5 != nullptr)
                            {
                                TANGO_LOG_DEBUG << "Asking CORBA structure to release attribute mutex for attribute "
                                                << att.get_name() << std::endl;
                                if(att.get_writable() != Tango::WRITE)
                                {
                                    REL_ATT_MUTEX_5(aid.data_5, index, att);
                                }
                                error_from_devfailed((*aid.data_5)[index], e, att.get_name().c_str());
                            }
                            else if(aid.data_4 != nullptr)
                            {
                                TANGO_LOG_DEBUG << "Asking CORBA structure to release attribute mutex for attribute "
                                                << att.get_name() << std::endl;
                                if(att.get_writable() != Tango::WRITE)
                                {
                                    REL_ATT_MUTEX(aid.data_4, index, att);
                                }
                                error_from_devfailed((*aid.data_4)[index], e, att.get_name().c_str());
                            }
                            else
                            {
                                error_from_devfailed((*aid.data_3)[index], e, att.get_name().c_str());
                            }
                            TANGO_TELEMETRY_TRACK_BAD_ATTR(att.get_name());
                        }
                    }
                }
                else
                {
                    if(qual != Tango::ATTR_INVALID)
                    {
                        qual = Tango::ATTR_INVALID;
                    }
                    if(att.get_when().tv_sec == 0)
                    {
                        att.set_time();
                    }

                    AttrSerialModel atsm = att.get_attr_serial_model();

                    if(aid.data_5 != nullptr)
                    {
                        if((atsm != ATTR_NO_SYNC) && (att.get_writable() != Tango::WRITE))
                        {
                            TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                            << " due to error" << std::endl;
                            omni_mutex *attr_mut =
                                (atsm == ATTR_BY_KERNEL) ? att.get_attr_mutex() : att.get_user_attr_mutex();
                            attr_mut->unlock();
                        }

                        init_out_data_quality((*aid.data_5)[index], att, qual);
                        (*aid.data_5)[index].data_format = att.get_data_format();
                        (*aid.data_5)[index].data_type = att.get_data_type();
                    }
                    else if(aid.data_4 != nullptr)
                    {
                        if((atsm != ATTR_NO_SYNC) && (att.get_writable() != Tango::WRITE))
                        {
                            TANGO_LOG_DEBUG << "Releasing attribute mutex for attribute " << att.get_name()
                                            << " due to error" << std::endl;
                            omni_mutex *attr_mut =
                                (atsm == ATTR_BY_KERNEL) ? att.get_attr_mutex() : att.get_user_attr_mutex();
                            attr_mut->unlock();
                        }

                        init_out_data_quality((*aid.data_4)[index], att, qual);
                        (*aid.data_4)[index].data_format = att.get_data_format();
                    }
                    else
                    {
                        init_out_data_quality((*aid.data_3)[index], att, qual);
                    }
                }
            }
        }
    }
    catch(...)
    {
        //
        // Set back the device attribution for the thread and rethrow the exception.
        //
        sub.set_associated_device(last_associated_device);
        throw;
    }

#if defined(TANGO_USE_TELEMETRY)
    report_attr_error(bad_attributes);
#endif

    //
    // Set back the device attribution for the thread
    //

    sub.set_associated_device(last_associated_device);

    TANGO_LOG_DEBUG << "Leaving Device_3Impl::read_attributes_no_except" << std::endl;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::read_attributes_from_cache
//
// description :
//        Read attributes from cache but do not throw exception if it fails. This method is mainly a copy of the
//        original DeviceImpl::read_attributes method.
//
// argument:
//        in :
//            - names: The names of the attribute to read
//            - aid : Structure with pointers to data which should be returned to the caller. This method is called
//            for
//                    3 differents IDL releases (3, 4 and 5). In this struct, there is one ptr for each possible
//                    IDL. Only the ptr for the caller IDL is not null.
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::read_attributes_from_cache(const Tango::DevVarStringArray &names, Tango::AttributeIdlData &aid)
{
    unsigned long nb_names = names.length();
    TANGO_LOG_DEBUG << "Reading " << nb_names << " attr in read_attributes_from_cache()" << std::endl;

    //
    // Check that device supports the wanted attribute and that the attribute is polled. If some are non polled,
    // store their index in the real_names sequence in a vector
    //

    unsigned long i;
    std::vector<PollObj *> &poll_list = get_poll_obj_list();
    std::vector<long> non_polled;
    unsigned long nb_poll = poll_list.size();
    unsigned long j;

#if defined(TANGO_USE_TELEMETRY)
    // see comment at Device_3Impl::read_attributes_no_except
    std::vector<std::string> bad_attributes;
#endif

    for(i = 0; i < nb_names; i++)
    {
        try
        {
            dev_attr->get_attr_ind_by_name(names[i]);
            for(j = 0; j < nb_poll; j++)
            {
                if(TG_strcasecmp(poll_list[j]->get_name().c_str(), names[i]) == 0)
                {
                    break;
                }
            }
            if(j == nb_poll)
            {
                non_polled.push_back(i);
            }
        }
        catch(Tango::DevFailed &e)
        {
            if(aid.data_5 != nullptr)
            {
                error_from_devfailed((*aid.data_5)[i], e, names[i]);
            }
            else if(aid.data_4 != nullptr)
            {
                error_from_devfailed((*aid.data_4)[i], e, names[i]);
            }
            else
            {
                error_from_devfailed((*aid.data_3)[i], e, names[i]);
            }
            TANGO_TELEMETRY_TRACK_BAD_ATTR(names[i].in());
        }
    }

    //
    // If some attributes are not polled but their polling update period is defined, and the attribute is not in the
    // device list of attr which should not be polled, start to poll them
    //

    std::vector<long> poll_period;
    unsigned long not_polled_attr = 0;

    if(!non_polled.empty())
    {
        //
        // Check that it is possible to start polling for the non polled attribute
        //

        for(i = 0; i < non_polled.size(); i++)
        {
            Attribute &att = dev_attr->get_attr_by_name(names[non_polled[i]]);
            poll_period.push_back(att.get_polling_period());

            if(poll_period.back() == 0)
            {
                TangoSys_OMemStream o;
                o << "Attribute " << att.get_name() << " not polled" << std::ends;
                std::string s = o.str();

                const char *reas = API_AttrNotPolled;

                if(aid.data_5 != nullptr)
                {
                    one_error((*aid.data_5)[non_polled[i]], reas, TANGO_EXCEPTION_ORIGIN, s, att);
                }
                else if(aid.data_4 != nullptr)
                {
                    one_error((*aid.data_4)[non_polled[i]], reas, TANGO_EXCEPTION_ORIGIN, s, att);
                }
                else
                {
                    one_error((*aid.data_3)[non_polled[i]], reas, TANGO_EXCEPTION_ORIGIN, s, att);
                }
                TANGO_TELEMETRY_TRACK_BAD_ATTR(att.get_name());
                not_polled_attr++;
                continue;
            }
        }

        //
        // Leave method if number of attributes which should not be polled is equal to the requested attribute
        // number
        //

        if(not_polled_attr == nb_names)
        {
            return;
        }
    }

    //
    // For each attribute, check that some data are available in cache and that they are not too old
    //

    for(i = 0; i < nb_names; i++)
    {
        if(aid.data_5 != nullptr)
        {
            if((*aid.data_5)[i].err_list.length() != 0)
            {
                continue;
            }
        }
        else if(aid.data_4 != nullptr)
        {
            if((*aid.data_4)[i].err_list.length() != 0)
            {
                continue;
            }
        }
        else
        {
            if((*aid.data_3)[i].err_list.length() != 0)
            {
                continue;
            }
        }

        PollObj *polled_attr = nullptr;
        unsigned long j;
        for(j = 0; j < poll_list.size(); j++)
        {
            if((poll_list[j]->get_type() == Tango::POLL_ATTR) &&
               (TG_strcasecmp(poll_list[j]->get_name().c_str(), names[i]) == 0))
            {
                polled_attr = poll_list[j];
                break;
            }
        }

        //
        // In some cases where data from polling are required by a DS for devices marked as polled but for which the
        // polling is not sarted yet, polled_attr could be nullptr at the end of this loop. Return "No data yet" in
        // this case
        //

        if(polled_attr == nullptr)
        {
            TangoSys_OMemStream o;
            o << "No data available in cache for attribute " << names[i] << std::ends;
            std::string s = o.str();

            const char *reas = API_NoDataYet;

            if(aid.data_5 != nullptr)
            {
                one_error((*aid.data_5)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
                (*aid.data_5)[i].data_format = FMT_UNKNOWN;
            }
            else if(aid.data_4 != nullptr)
            {
                one_error((*aid.data_4)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
                (*aid.data_4)[i].data_format = FMT_UNKNOWN;
            }
            else
            {
                one_error((*aid.data_3)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
            }
            TANGO_TELEMETRY_TRACK_BAD_ATTR(names[i].in());
            continue;
        }

        //
        // Check that some data is available in cache
        //

        if(polled_attr->is_ring_empty())
        {
            TangoSys_OMemStream o;
            o << "No data available in cache for attribute " << names[i] << std::ends;
            std::string s = o.str();

            const char *reas = API_NoDataYet;

            if(aid.data_5 != nullptr)
            {
                one_error((*aid.data_5)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
            }
            else if(aid.data_4 != nullptr)
            {
                one_error((*aid.data_4)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
            }
            else
            {
                one_error((*aid.data_3)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
            }
            TANGO_TELEMETRY_TRACK_BAD_ATTR(names[i].in());
            continue;
        }

        //
        // Check that data are still refreshed by the polling thread
        // Skip this test for object with external polling triggering (upd = 0)
        //

        auto tmp_upd = polled_attr->get_upd();
        if(tmp_upd != PollClock::duration::zero())
        {
            auto last = polled_attr->get_last_insert_date();
            auto now = PollClock::now();
            auto diff_d = now - last;
            if(diff_d > polled_attr->get_authorized_delta())
            {
                TangoSys_OMemStream o;
                o << "Data in cache for attribute " << names[i];
                o << " not updated any more" << std::ends;
                std::string s = o.str();

                const char *reas = API_NotUpdatedAnyMore;

                if(aid.data_5 != nullptr)
                {
                    one_error((*aid.data_5)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
                }
                else if(aid.data_4 != nullptr)
                {
                    one_error((*aid.data_4)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
                }
                else
                {
                    one_error((*aid.data_3)[i], reas, TANGO_EXCEPTION_ORIGIN, s, names[i]);
                }
                TANGO_TELEMETRY_TRACK_BAD_ATTR(names[i].in());
                continue;
            }
        }

        //
        // Get attribute data type
        //

        Attribute &att = dev_attr->get_attr_by_name(names[i]);
        long type = att.get_data_type();

        //
        // Finally, after all these checks, get value and store it in the sequence sent back to user
        // In order to avoid unnecessary copy, don't use the assignement operator of the AttributeValue structure
        // which copy each element and therefore also copy the Any object. The Any assignement operator is a deep
        // copy! Create a new sequence using the attribute buffer and insert it into the Any. The sequence inside
        // the source Any has been created using the attribute data buffer.
        //

        try
        {
            long vers = get_dev_idl_version();
            {
                omni_mutex_lock sync(*polled_attr);

                Tango::AttrQuality qual;

                //
                // Get device IDL release. Since release 4, devices are polled using read_attribute_4
                //

                if(vers >= 5)
                {
                    AttributeValue_5 &att_val = polled_attr->get_last_attr_value_5(false);
                    qual = att_val.quality;
                }
                else if(vers == 4)
                {
                    AttributeValue_4 &att_val = polled_attr->get_last_attr_value_4(false);
                    qual = att_val.quality;
                }
                else
                {
                    AttributeValue_3 &att_val = polled_attr->get_last_attr_value_3(false);
                    qual = att_val.quality;
                }

                //
                // Copy the polled data into the Any or the union
                //

                if(qual != Tango::ATTR_INVALID)
                {
                    polled_data_into_net_object(aid, i, type, vers, polled_attr, names);
                }

                //
                // Init remaining structure members according to IDL client (aid.xxxx) and IDL device (vers)
                //

                if(aid.data_5 != nullptr)
                {
                    AttributeValue_5 &att_val = polled_attr->get_last_attr_value_5(false);
                    init_polled_out_data((*aid.data_5)[i], att_val);
                    (*aid.data_5)[i].data_format = att_val.data_format;
                    (*aid.data_5)[i].data_type = att_val.data_type;
                }
                else if(aid.data_4 != nullptr)
                {
                    if(vers >= 5)
                    {
                        AttributeValue_5 &att_val = polled_attr->get_last_attr_value_5(false);
                        init_polled_out_data((*aid.data_4)[i], att_val);
                        (*aid.data_4)[i].data_format = att_val.data_format;
                    }
                    else
                    {
                        AttributeValue_4 &att_val = polled_attr->get_last_attr_value_4(false);
                        init_polled_out_data((*aid.data_4)[i], att_val);
                        (*aid.data_4)[i].data_format = att_val.data_format;
                    }
                }
                else
                {
                    if(vers >= 5)
                    {
                        AttributeValue_5 &att_val = polled_attr->get_last_attr_value_5(false);
                        init_polled_out_data((*aid.data_3)[i], att_val);
                    }
                    else if(vers == 4)
                    {
                        AttributeValue_4 &att_val = polled_attr->get_last_attr_value_4(false);
                        init_polled_out_data((*aid.data_3)[i], att_val);
                    }
                    else
                    {
                        AttributeValue_3 &att_val = polled_attr->get_last_attr_value_3(false);
                        init_polled_out_data((*aid.data_3)[i], att_val);
                    }
                }
            }
        }
        catch(Tango::DevFailed &e)
        {
            if(aid.data_5 != nullptr)
            {
                error_from_devfailed((*aid.data_5)[i], e, names[i]);
            }
            else if(aid.data_4 != nullptr)
            {
                error_from_devfailed((*aid.data_4)[i], e, names[i]);
            }
            else
            {
                error_from_devfailed((*aid.data_3)[i], e, names[i]);
            }
            TANGO_TELEMETRY_TRACK_BAD_ATTR(names[i].in());
        }
    }

#if defined(TANGO_USE_TELEMETRY)
    report_attr_error(bad_attributes);
#endif
}

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::write_attributes_3
//
// description :
//        CORBA operation to write attribute(s) value
//
// argument:
//        in :
//            - values: The new attribute(s) value to be set.
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::write_attributes_3(const Tango::AttributeValueList &values)
{
    AutoTangoMonitor sync(this, true);

    TANGO_LOG_DEBUG << "Device_3Impl::write_attributes_3 arrived" << std::endl;

    //
    // Record operation request in black box. If this method is executed with the request to store info in
    // blackbox (store_in_bb == true), this means that the request arrives through a Device_2 CORBA interface.
    // Check locking feature in this case. Otherwise the request has arrived through Device_4 and the check
    // is already done
    //

    if(store_in_bb)
    {
        blackbox_ptr->insert_attr(values, idl_version);
        check_lock("write_attributes_3");
    }
    store_in_bb = true;

    //
    // Call the method really doing the job
    //

    write_attributes_34(&values, nullptr);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::write_attributes_34
//
// description :
//        Method to write the attribute. This method is common to the IDL interface 3 and 4.
//
// argument:
//        in :
//            - values_3: The new attribute(s) value to be set in IDL V3
//            - values_4: The new attribute(s) value to be set in IDL V4
//
//------------------------------------------------------------------------------------------------------------------

void Device_3Impl::write_attributes_34(const Tango::AttributeValueList *values_3,
                                       const Tango::AttributeValueList_4 *values_4)
{
    //
    // Return exception if the device does not have any attribute
    //

    unsigned long nb_dev_attr = dev_attr->get_attr_nb();
    if(nb_dev_attr == 0)
    {
        TANGO_THROW_EXCEPTION(API_AttrNotFound, "The device does not have any attribute");
    }

    unsigned long nb_failed = 0;
    Tango::NamedDevErrorList errs;

    //
    //  Write the device name into the per thread data for sub device diagnostics.
    //  Keep the old name, to put it back at the end!
    //  During device access inside the same server, the thread stays the same!
    //

    SubDevDiag &sub = (Tango::Util::instance())->get_sub_dev_diag();
    std::string last_associated_device = sub.get_associated_device();
    sub.set_associated_device(get_name());

#if defined(TANGO_USE_TELEMETRY)
    // see comment at Device_3Impl::read_attributes_no_except
    std::vector<std::string> bad_attributes;
#endif

    //
    // Catch all exceptions to set back the associated device after execution
    //

    try
    {
        //
        // Retrieve index of wanted attributes in the device attribute list
        //

        std::vector<AttIdx> updated_attr;
        unsigned long nb_updated_attr;
        if(values_3 != nullptr)
        {
            nb_updated_attr = values_3->length();
        }
        else
        {
            nb_updated_attr = values_4->length();
        }

        unsigned long i;
        for(i = 0; i < nb_updated_attr; i++)
        {
            const char *single_att_name;
            long single_att_dimx, single_att_dimy;

            if(values_3 != nullptr)
            {
                single_att_name = (*values_3)[i].name;
                single_att_dimx = (*values_3)[i].dim_x;
                single_att_dimy = (*values_3)[i].dim_y;
            }
            else
            {
                single_att_name = (*values_4)[i].name;
                single_att_dimx = (*values_4)[i].w_dim.dim_x;
                single_att_dimy = (*values_4)[i].w_dim.dim_y;
            }

            try
            {
                AttIdx idxs;
                idxs.idx_in_names = i;
                idxs.idx_in_multi_attr = dev_attr->get_attr_ind_by_name(single_att_name);
                updated_attr.push_back(idxs);

                //
                // Check that these attributes are writable.
                // For attributes which are not scalar, also check that their dimensions are correct
                //

                Attribute &att = dev_attr->get_attr_by_ind(updated_attr.back().idx_in_multi_attr);
                if((att.get_writable() == Tango::READ) || (att.get_writable() == Tango::READ_WITH_WRITE))
                {
                    TangoSys_OMemStream o;

                    o << "Attribute ";
                    o << att.get_name();
                    o << " is not writable" << std::ends;

                    updated_attr.pop_back();
                    TANGO_THROW_EXCEPTION(API_AttrNotWritable, o.str());
                }

                if(att.get_data_format() != Tango::SCALAR)
                {
                    TangoSys_OMemStream o;
                    bool err = false;

                    if(att.get_max_dim_x() < single_att_dimx)
                    {
                        err = true;
                        o << "X ";
                    }

                    if(!err)
                    {
                        if(att.get_max_dim_y() < single_att_dimy)
                        {
                            err = true;
                            o << "Y ";
                        }
                    }

                    if(err)
                    {
                        o << "dimesion is greater than the max defined for attribute ";
                        o << att.get_name();
                        o << std::ends;

                        updated_attr.pop_back();
                        TANGO_THROW_EXCEPTION(API_WAttrOutsideLimit, o.str());
                    }
                }

                //
                // Check if there are some startup exceptions for the attribute (due to invalid attribute properties
                // configuration). If so, do not allow to write the attribute.
                //

                if(att.is_startup_exception())
                {
                    updated_attr.pop_back();
                    att.throw_startup_exception("DeviceImpl::write_attributes()");
                }
            }
            catch(Tango::DevFailed &e)
            {
                nb_failed++;
                errs.length(nb_failed);
                errs[nb_failed - 1].name = Tango::string_dup(single_att_name);
                errs[nb_failed - 1].index_in_call = i;
                errs[nb_failed - 1].err_list = e.errors;
                TANGO_TELEMETRY_TRACK_BAD_ATTR(single_att_name);
            }
        }

        //
        // Call the always_executed_hook
        //

        if(nb_failed != nb_updated_attr)
        {
            always_executed_hook();
        }

        //
        // Set attribute internal value
        //

        std::vector<AttIdx>::iterator ctr;
        for(ctr = updated_attr.begin(); ctr < updated_attr.end(); ++ctr)
        {
            const char *single_att_name;
            long single_att_dimx, single_att_dimy;

            if(values_3 != nullptr)
            {
                single_att_name = (*values_3)[ctr->idx_in_names].name;
                single_att_dimx = (*values_3)[ctr->idx_in_names].dim_x;
                single_att_dimy = (*values_3)[ctr->idx_in_names].dim_y;
            }
            else
            {
                single_att_name = (*values_4)[ctr->idx_in_names].name;
                single_att_dimx = (*values_4)[ctr->idx_in_names].w_dim.dim_x;
                single_att_dimy = (*values_4)[ctr->idx_in_names].w_dim.dim_y;
            }

            try
            {
                if(values_3 == nullptr)
                {
                    dev_attr->get_w_attr_by_ind(ctr->idx_in_multi_attr)
                        .check_written_value((*values_4)[ctr->idx_in_names].value,
                                             (unsigned long) single_att_dimx,
                                             (unsigned long) single_att_dimy);
                }
                else
                {
                    dev_attr->get_w_attr_by_ind(ctr->idx_in_multi_attr)
                        .check_written_value((*values_3)[ctr->idx_in_names].value,
                                             (unsigned long) single_att_dimx,
                                             (unsigned long) single_att_dimy);
                }
            }
            catch(Tango::DevFailed &e)
            {
                nb_failed++;
                errs.length(nb_failed);
                errs[nb_failed - 1].name = Tango::string_dup(single_att_name);
                errs[nb_failed - 1].index_in_call = ctr->idx_in_names;
                errs[nb_failed - 1].err_list = e.errors;
                TANGO_TELEMETRY_TRACK_BAD_ATTR(single_att_name);
                ctr = updated_attr.erase(ctr);
                if(ctr >= updated_attr.end())
                {
                    break;
                }
                else
                {
                    if(ctr == updated_attr.begin())
                    {
                        break;
                    }
                    else
                    {
                        --ctr;
                    }
                }
            }
        }

        //
        // Write the hardware. Call this method one attribute at a time in order to correctly initialized the
        // MultiDevFailed exception in case one of the attribute failed.
        //

        if(nb_failed != nb_updated_attr)
        {
            std::vector<AttIdx>::iterator ite;
            std::vector<long> att_idx;

            for(ite = updated_attr.begin(); ite != updated_attr.end();)
            {
                WAttribute &att = dev_attr->get_w_attr_by_ind((*ite).idx_in_multi_attr);
                att_idx.push_back(ite->idx_in_multi_attr);

                try
                {
                    att.reset_value();
                    att.set_user_set_write_value(false);
                    std::vector<Tango::Attr *> &attr_vect = device_class->get_class_attr()->get_attr_list();
                    if(!attr_vect[att.get_attr_idx()]->is_allowed(this, Tango::WRITE_REQ))
                    {
                        TangoSys_OMemStream o;

                        o << "It is currently not allowed to write attribute ";
                        o << att.get_name();
                        o << ". The device state is " << Tango::DevStateName[get_state()] << std::ends;

                        TANGO_THROW_EXCEPTION(API_AttrNotAllowed, o.str());
                    }
                    attr_vect[att.get_attr_idx()]->write(this, att);

                    //
                    // If the write call succeed and if the attribute was memorized but with an exception thrown
                    // during the device startup sequence, clear the memorized exception
                    //

                    if(att.get_mem_write_failed())
                    {
                        att.clear_mem_exception();
                        set_run_att_conf_loop(true);
                    }

                    ++ite;
                }
                catch(Tango::MultiDevFailed &e)
                {
                    nb_failed++;
                    if(att.get_data_format() == SCALAR)
                    {
                        att.rollback();
                    }
                    errs.length(nb_failed);
                    if(values_3 != nullptr)
                    {
                        auto *attr_name = Tango::string_dup((*values_3)[(*ite).idx_in_names].name);
                        errs[nb_failed - 1].name = attr_name;
                        TANGO_TELEMETRY_TRACK_BAD_ATTR(attr_name);
                    }
                    else
                    {
                        auto *attr_name = Tango::string_dup((*values_4)[(*ite).idx_in_names].name);
                        errs[nb_failed - 1].name = attr_name;
                        TANGO_TELEMETRY_TRACK_BAD_ATTR(attr_name);
                    }
                    errs[nb_failed - 1].index_in_call = (*ite).idx_in_names;
                    errs[nb_failed - 1].err_list = e.errors[0].err_list;
                    ite = updated_attr.erase(ite);
                    att_idx.pop_back();
                }
                catch(Tango::DevFailed &e)
                {
                    nb_failed++;
                    if(att.get_data_format() == SCALAR)
                    {
                        att.rollback();
                    }
                    errs.length(nb_failed);
                    if(values_3 != nullptr)
                    {
                        auto *attr_name = Tango::string_dup((*values_3)[(*ite).idx_in_names].name);
                        errs[nb_failed - 1].name = attr_name;
                        TANGO_TELEMETRY_TRACK_BAD_ATTR(attr_name);
                    }
                    else
                    {
                        auto *attr_name = Tango::string_dup((*values_4)[(*ite).idx_in_names].name);
                        errs[nb_failed - 1].name = attr_name;
                        TANGO_TELEMETRY_TRACK_BAD_ATTR(attr_name);
                    }
                    errs[nb_failed - 1].index_in_call = (*ite).idx_in_names;
                    errs[nb_failed - 1].err_list = e.errors;
                    ite = updated_attr.erase(ite);
                    att_idx.pop_back();
                }
            }

            //
            // Call the write_attr_hardware method
            // If it throws DevFailed exception, mark all attributes has failure
            // If it throws NamedDevFailedList exception, mark only referenced attributes as faulty
            //

            long vers = get_dev_idl_version();
            if(vers >= 4)
            {
                try
                {
                    write_attr_hardware(att_idx);
                }
                catch(Tango::MultiDevFailed &e)
                {
                    for(unsigned long loop = 0; loop < e.errors.length(); loop++)
                    {
                        nb_failed++;
                        errs.length(nb_failed);

                        std::vector<AttIdx>::iterator ite_att;

                        for(ite_att = updated_attr.begin(); ite_att != updated_attr.end(); ++ite_att)
                        {
                            if(TG_strcasecmp(dev_attr->get_w_attr_by_ind(ite_att->idx_in_multi_attr).get_name().c_str(),
                                             e.errors[loop].name) == 0)
                            {
                                errs[nb_failed - 1].index_in_call = ite_att->idx_in_names;
                                errs[nb_failed - 1].name = Tango::string_dup(e.errors[loop].name);
                                errs[nb_failed - 1].err_list = e.errors[loop].err_list;

                                WAttribute &att = dev_attr->get_w_attr_by_ind(ite_att->idx_in_multi_attr);
                                if(att.get_data_format() == SCALAR)
                                {
                                    att.rollback();
                                }
                                TANGO_TELEMETRY_TRACK_BAD_ATTR(att.get_name());
                                break;
                            }
                        }
                        updated_attr.erase(ite_att);
                    }
                }
                catch(Tango::DevFailed &e)
                {
                    std::vector<long>::iterator ite;
                    for(ite = att_idx.begin(); ite != att_idx.end(); ++ite)
                    {
                        WAttribute &att = dev_attr->get_w_attr_by_ind(*ite);
                        nb_failed++;
                        if(att.get_data_format() == SCALAR)
                        {
                            att.rollback();
                        }
                        errs.length(nb_failed);
                        errs[nb_failed - 1].name = Tango::string_dup(att.get_name().c_str());

                        std::vector<AttIdx>::iterator ite_att;
                        for(ite_att = updated_attr.begin(); ite_att != updated_attr.end(); ++ite_att)
                        {
                            if(ite_att->idx_in_multi_attr == *ite)
                            {
                                errs[nb_failed - 1].index_in_call = ite_att->idx_in_names;
                                break;
                            }
                        }

                        errs[nb_failed - 1].err_list = e.errors;
                        TANGO_TELEMETRY_TRACK_BAD_ATTR(att.get_name());
                    }
                    updated_attr.clear();
                }
            }
        }

        //
        // Copy data into Attribute object, store the memorized one in db and if the attribute has a RDS alarm, set
        // the write date
        //
        // Warning: Do not copy caller value if the user has manually set the attribute written value in its write
        // method
        //
        // WARNING: --> The DevEncoded data type is supported only as SCALAR and is not memorizable.
        // Therefore, no need to call copy_data
        //

        std::vector<long> att_in_db;

        for(i = 0; i < updated_attr.size(); i++)
        {
            WAttribute &att = dev_attr->get_w_attr_by_ind(updated_attr[i].idx_in_multi_attr);

            if(values_3 != nullptr)
            {
                if(!att.get_user_set_write_value())
                {
                    att.copy_data((*values_3)[updated_attr[i].idx_in_names].value);
                }
            }
            else
            {
                if(!att.get_user_set_write_value())
                {
                    att.copy_data((*values_4)[updated_attr[i].idx_in_names].value);
                }
            }

            if(att.is_memorized())
            {
                att_in_db.push_back(i);
                if(att.get_mem_value() == MemNotUsed)
                {
                    att.set_mem_value("Set");
                }
            }
            if(att.is_alarmed().test(Attribute::rds))
            {
                att.set_written_date();
            }
        }

        if((Tango::Util::instance()->use_db()) && (!att_in_db.empty()))
        {
            try
            {
                write_attributes_in_db(att_in_db, updated_attr);
            }
            catch(Tango::DevFailed &e)
            {
                errs.length(nb_failed + att_in_db.size());
                for(i = 0; i < att_in_db.size(); i++)
                {
                    const char *single_att_name;

                    if(values_3 != nullptr)
                    {
                        single_att_name = (*values_3)[updated_attr[att_in_db[i]].idx_in_names].name;
                    }
                    else
                    {
                        single_att_name = (*values_4)[updated_attr[att_in_db[i]].idx_in_names].name;
                    }
                    errs[nb_failed + i].name = Tango::string_dup(single_att_name);
                    errs[nb_failed + i].index_in_call = updated_attr[att_in_db[i]].idx_in_names;
                    errs[nb_failed + i].err_list = e.errors;
                    TANGO_TELEMETRY_TRACK_BAD_ATTR(single_att_name);
                }
                nb_failed = nb_failed + att_in_db.size();
            }
        }
    }
    catch(...)
    {
        //
        // Set back the device attribution for the thread and rethrow the exception.
        //

        sub.set_associated_device(last_associated_device);
        throw;
    }

#if defined(TANGO_USE_TELEMETRY)
    report_attr_error(bad_attributes);
#endif

    //
    // Set back the device attribution for the thread
    //

    sub.set_associated_device(last_associated_device);

    //
    // Return to caller.
    //

    TANGO_LOG_DEBUG << "Leaving Device_3Impl::write_attributes_34" << std::endl;

    if(nb_failed != 0)
    {
        throw Tango::MultiDevFailed(errs);
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::read_attribute_history_3
//
// description :
//        CORBA operation to read attribute value history from the polling buffer.
//
// argument:
//        in :
//            - name : attribute name
//            - n : history depth (in record number)
//
// return :
//         This method returns a pointer to a DevAttrHistoryList with one DevAttrHistory structure for each
//        attribute record
//
//--------------------------------------------------------------------------------------------------------------------

Tango::DevAttrHistoryList_3 *Device_3Impl::read_attribute_history_3(const char *name, CORBA::Long n)
{
    TangoMonitor &mon = get_poll_monitor();
    AutoTangoMonitor sync(&mon);

    TANGO_LOG_DEBUG << "Device_3Impl::read_attribute_history_3 arrived" << std::endl;

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Read_Attr_history_3);

    Tango::DevAttrHistoryList_3 *back = nullptr;
    std::vector<PollObj *> &poll_list = get_poll_obj_list();
    long nb_poll = poll_list.size();

    //
    // Check that the device supports this attribute. This method returns an exception in case of unsupported
    // attribute
    //

    Attribute &att = dev_attr->get_attr_by_name(name);

    std::string attr_str(name);
    std::transform(attr_str.begin(), attr_str.end(), attr_str.begin(), ::tolower);

    //
    // Check that the wanted attribute is polled.
    //

    long j;
    PollObj *polled_attr = nullptr;
    for(j = 0; j < nb_poll; j++)
    {
        if((poll_list[j]->get_type() == Tango::POLL_ATTR) && (poll_list[j]->get_name() == attr_str))
        {
            polled_attr = poll_list[j];
            break;
        }
    }
    if(polled_attr == nullptr)
    {
        TangoSys_OMemStream o;
        o << "Attribute " << attr_str << " not polled" << std::ends;
        TANGO_THROW_EXCEPTION(API_AttrNotPolled, o.str());
    }

    //
    // Check that some data is available in cache
    //

    if(polled_attr->is_ring_empty())
    {
        TangoSys_OMemStream o;
        o << "No data available in cache for attribute " << attr_str << std::ends;
        TANGO_THROW_EXCEPTION(API_NoDataYet, o.str());
    }

    //
    // Set the number of returned records
    //

    long in_buf = polled_attr->get_elt_nb_in_buffer();
    n = std::min<long>(n, in_buf);

    //
    // Allocate memory for the returned value
    //

    try
    {
        back = new Tango::DevAttrHistoryList_3(n);
        back->length(n);
    }
    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    //
    // Get attribute value history
    //

    long vers = get_dev_idl_version();

    if(vers < 4)
    {
        polled_attr->get_attr_history(n, back, att.get_data_type());
    }
    else
    {
        polled_attr->get_attr_history_43(n, back, att.get_data_type());
    }

    TANGO_LOG_DEBUG << "Leaving Device_3Impl::command_inout_history_3 method" << std::endl;
    return back;
}

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::info_3
//
// description :
//        CORBA operation to get device info
//
//--------------------------------------------------------------------------------------------------------------------

Tango::DevInfo_3 *Device_3Impl::info_3()
{
    TANGO_LOG_DEBUG << "Device_3Impl::info_3 arrived" << std::endl;

    Tango::DevInfo_3 *back = nullptr;

    //
    // Allocate memory for the stucture sent back to caller. The ORB will free it
    //

    try
    {
        back = new Tango::DevInfo_3();
    }
    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    //
    // Retrieve server host
    //

    Tango::Util *tango_ptr = Tango::Util::instance();
    back->server_host = Tango::string_dup(tango_ptr->get_host_name().c_str());

    //
    // Fill-in remaining structure fields
    //

    back->dev_class = Tango::string_dup(device_class->get_name().c_str());
    back->server_id = Tango::string_dup(tango_ptr->get_ds_name().c_str());
    back->server_version = DevVersion;

    //
    // Build the complete info sent in the doc_url string
    //

    std::string doc_url("Doc URL = ");
    doc_url = doc_url + device_class->get_doc_url();
    std::string &cvs_tag = device_class->get_cvs_tag();
    if(cvs_tag.size() != 0)
    {
        doc_url = doc_url + "\nCVS Tag = ";
        doc_url = doc_url + cvs_tag;
    }
    std::string &cvs_location = device_class->get_cvs_location();
    if(cvs_location.size() != 0)
    {
        doc_url = doc_url + "\nCVS Location = ";
        doc_url = doc_url + cvs_location;
    }
    back->doc_url = Tango::string_dup(doc_url.c_str());

    //
    // Set the device type
    //

    back->dev_type = Tango::string_dup(device_class->get_type().c_str());

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Info_3);

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving Device_3Impl::info_3" << std::endl;
    return back;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::get_attribute_config_3
//
// description :
//        CORBA operation to get attribute configuration.
//
//         WARNING !!!!!!!!!!!!!!!!!!
//
//         This is the release 3 of this CORBA operation which returns much more parameter than in release 2
//         The code has been duplicated in order to keep it clean (avoid many "if" on version number in a common
//         method)
//
// argument:
//        in :
//            - names: name of attribute(s)
//
// return :
//         This method returns a pointer to a AttributeConfigList_3 with one AttributeConfig_3 structure for each
//         atribute
//
//--------------------------------------------------------------------------------------------------------------------

Tango::AttributeConfigList_3 *Device_3Impl::get_attribute_config_3(const Tango::DevVarStringArray &names)
{
    TangoMonitor &mon = get_att_conf_monitor();
    AutoTangoMonitor sync(&mon);

    TANGO_LOG_DEBUG << "Device_3Impl::get_attribute_config_3 arrived" << std::endl;

    long nb_attr = names.length();
    Tango::AttributeConfigList_3 *back = nullptr;
    bool all_attr = false;

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Get_Attr_Config_3);

    //
    // Get attribute number and device version
    //

    long nb_dev_attr = dev_attr->get_attr_nb();

    //
    // Check if the caller want to get config for all attribute.
    // If the device implements IDL 3 (State and status as attributes) and the client is an old one (not able to
    // read state/status as attribute), decrement attribute number
    //

    std::string in_name(names[0]);
    if(nb_attr == 1)
    {
        if(in_name == AllAttr_3)
        {
            all_attr = true;
            nb_attr = nb_dev_attr;
        }
    }

    //
    // Allocate memory for the AttributeConfig structures
    //

    try
    {
        back = new Tango::AttributeConfigList_3(nb_attr);
        back->length(nb_attr);
    }
    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    //
    // Fill in these structures
    //

    for(long i = 0; i < nb_attr; i++)
    {
        try
        {
            if(all_attr)
            {
                Attribute &attr = dev_attr->get_attr_by_ind(i);
                attr.get_properties((*back)[i]);
            }
            else
            {
                Attribute &attr = dev_attr->get_attr_by_name(names[i]);
                attr.get_properties((*back)[i]);
            }
        }
        catch(Tango::DevFailed &)
        {
            delete back;
            throw;
        }
    }

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving Device_3Impl::get_attribute_config_3" << std::endl;

    return back;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::set_attribute_config_3
//
// description :
//        CORBA operation to set attribute configuration locally and in the Tango database
//
// argument:
//        in :
//            - new_conf: The new attribute(s) configuration. One AttributeConfig structure is needed for each
//                        attribute to update
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::set_attribute_config_3(const Tango::AttributeConfigList_3 &new_conf)
{
    AutoTangoMonitor sync(this, true);

    TANGO_LOG_DEBUG << "DeviceImpl::set_attribute_config_3 arrived" << std::endl;

    //
    // The attribute conf. is protected by two monitors. One protects access between get and set attribute conf.
    // The second one protects access between set and usage. This is the classical device monitor
    //

    TangoMonitor &mon1 = get_att_conf_monitor();
    AutoTangoMonitor sync1(&mon1);

    //
    // Record operation request in black box
    // If this method is executed with the request to store info in blackbox (store_in_bb == true), this means that
    // the request arrives through a Device_2 CORBA interface. Check locking feature in this case. Otherwise the
    // request has arrived through Device_4 and the check is already done
    //

    if(store_in_bb)
    {
        blackbox_ptr->insert_op(Op_Set_Attr_Config_3);
        check_lock("set_attribute_config_3");
    }
    store_in_bb = true;

    set_attribute_config_3_local(new_conf, false, idl_version);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::write_attributes_in_db
//
// description :
//        Method to write memorized attributes in database
//
// argument:
//        in :
//            - name : attribute name
//            - n : history depth (in record number)
//
// return:
//         This method returns a pointer to a DevAttrHistoryList with one DevAttrHistory structure for each
//         attribute
//        record
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::write_attributes_in_db(const std::vector<long> &att_in_db, const std::vector<AttIdx> &updated_attr)
{
    //
    // Store memorized attribute in db
    //

    Tango::Util *tg = Tango::Util::instance();
    Tango::Database *db = tg->get_database();

    Tango::DbData db_data;

    for(unsigned long i = 0; i < att_in_db.size(); i++)
    {
        Tango::DbDatum tmp_db;

        //
        // Update one property
        //

        long idx = att_in_db[i];
        WAttribute &att = dev_attr->get_w_attr_by_ind(updated_attr[idx].idx_in_multi_attr);
        tmp_db.name = att.get_name();
        tmp_db << (short) 1;
        db_data.push_back(tmp_db);

        //
        // Init property value
        //

        tmp_db.name = MemAttrPropName;
        const char *ptr;
        switch(att.get_data_type())
        {
        case Tango::DEV_SHORT:
        case Tango::DEV_ENUM:
            tmp_db << (*att.get_last_written_sh())[0];
            break;

        case Tango::DEV_LONG:
            tmp_db << (*att.get_last_written_lg())[0];
            break;

        case Tango::DEV_LONG64:
            tmp_db << (*att.get_last_written_lg64())[0];
            break;

        case Tango::DEV_DOUBLE:
            tmp_db << (*att.get_last_written_db())[0];
            break;

        case Tango::DEV_STRING:
            ptr = (*att.get_last_written_str())[0].in();
            tmp_db << ptr;
            break;

        case Tango::DEV_FLOAT:
            tmp_db << (*att.get_last_written_fl())[0];
            break;

        case Tango::DEV_BOOLEAN:
            tmp_db << (*att.get_last_written_boo())[0];
            break;

        case Tango::DEV_USHORT:
            tmp_db << (*att.get_last_written_ush())[0];
            break;

        case Tango::DEV_UCHAR:
            tmp_db << (*att.get_last_written_uch())[0];
            break;

        case Tango::DEV_ULONG:
            tmp_db << (*att.get_last_written_ulg())[0];
            break;

        case Tango::DEV_ULONG64:
            tmp_db << (*att.get_last_written_ulg64())[0];
            break;

        case Tango::DEV_STATE:
        {
            Tango::DevState tmp_state = (*att.get_last_written_state())[0];
            tmp_db << (short) tmp_state;
        }
        break;
        }
        db_data.push_back(tmp_db);
    }

    db->put_device_attribute_property(device_name, db_data);
}

void Device_3Impl::write_attributes_in_db(const std::vector<long> &att_in_db, const std::vector<long> &updated_attr)
{
    std::vector<AttIdx> v;
    for(unsigned int i = 0; i < updated_attr.size(); i++)
    {
        AttIdx ai;
        ai.idx_in_multi_attr = updated_attr[i];
        v.push_back(ai);
    }

    write_attributes_in_db(att_in_db, v);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::add_state_status_attrs
//
// description :
//        Add state and status in the device attribute list
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::add_state_status_attrs()
{
    //
    // First, create the State attribute with default properties
    //

    Tango::Attr att_state("State", Tango::DEV_STATE);
    std::vector<AttrProperty> prop_list_state;
    std::string att_name("State");
    get_attr_props("State", prop_list_state);
    dev_attr->add_default(prop_list_state, device_name, att_name, Tango::DEV_STATE);

    dev_attr->add_attr(new Attribute(prop_list_state, att_state, device_name, -1));

    //
    // Now, create the status attribute also with default properties
    //

    Tango::Attr att_status("Status", Tango::DEV_STRING);
    std::vector<AttrProperty> prop_list_status;
    att_name = "Status";
    get_attr_props("Status", prop_list_status);
    dev_attr->add_default(prop_list_status, device_name, att_name, Tango::DEV_STRING);

    dev_attr->add_attr(new Attribute(prop_list_status, att_status, device_name, -1));
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::get_attr_props
//
// description :
//        Get attribute properties. This method is used to retrieve properties for state and status.
//
// argument:
//        in :
//            - attr_name : The attribute name
//        inout :
//            - prop_list : The attribute property vector
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::get_attr_props(const char *attr_name, std::vector<AttrProperty> &prop_list)
{
    Tango::Util *tg = Tango::Util::instance();

    if(tg->use_db())
    {
        Tango::DbData db_list;
        db_list.emplace_back(attr_name);

        //
        // Get attr prop from db cache
        //

        try
        {
            tg->get_database()->get_device_attribute_property(device_name, db_list, tg->get_db_cache());
        }
        catch(Tango::DevFailed &)
        {
            TANGO_LOG_DEBUG << "Exception while accessing database" << std::endl;

            TangoSys_OMemStream o;
            o << "Can't get device attribute properties for device " << device_name << ", attribute " << attr_name
              << std::ends;

            TANGO_THROW_EXCEPTION(API_DatabaseAccess, o.str());
        }

        //
        // Insert AttrProperty element in suplied vector for att. properties found in DB
        //

        long ind = 0;

        long nb_prop = 0;
        db_list[ind] >> nb_prop;
        ind++;

        for(long j = 0; j < nb_prop; j++)
        {
            if(db_list[ind].size() > 1)
            {
                std::string tmp(db_list[ind].value_string[0]);
                long nb = db_list[ind].size();
                for(int k = 1; k < nb; k++)
                {
                    tmp = tmp + ",";
                    tmp = tmp + db_list[ind].value_string[k];
                }
                prop_list.emplace_back(db_list[ind].name, tmp);
            }
            else
            {
                prop_list.emplace_back(db_list[ind].name, db_list[ind].value_string[0]);
            }
            ind++;
        }
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::add_alarmed
//
// description :
//        Method to add alarmed attributes (if not already there) in the attribute list passed as argument
//
// argument:
//        in :
//            - att_list : The attribute index in the multi attribute instance
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::add_alarmed(std::vector<long> &att_list)
{
    std::vector<long> &alarmed_list = dev_attr->get_alarm_list();
    long nb_wanted_attr = alarmed_list.size();

    if(nb_wanted_attr != 0)
    {
        for(int i = 0; i < nb_wanted_attr; i++)
        {
            long nb_attr = att_list.size();
            bool found = false;

            for(int j = 0; j < nb_attr; j++)
            {
                if(att_list[j] == alarmed_list[i])
                {
                    found = true;
                    break;
                }
            }

            //
            // If not found, add it
            //

            if(!found)
            {
                att_list.push_back(alarmed_list[i]);
            }
        }
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::alarmed_not_read
//
// description :
//        This method find all the attributes which will be read by the state (because alarmed) and which have been
//        already read. It builds a vector with the list of attribute not read
//
// argument:
//        in :
//            - wanted_attr : The list of attribute to be read by this call
//
//---------------------------------------------------------------------------------------------------------------------

void Device_3Impl::alarmed_not_read(const std::vector<AttIdx> &wanted_attr)
{
    std::vector<long> &alarmed_list = dev_attr->get_alarm_list();
    long nb_alarmed_attr = alarmed_list.size();
    long nb_attr = wanted_attr.size();

    alrmd_not_read.clear();

    for(int i = 0; i < nb_alarmed_attr; i++)
    {
        bool found = false;
        for(int j = 0; j < nb_attr; j++)
        {
            if(alarmed_list[i] == wanted_attr[j].idx_in_multi_attr)
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            alrmd_not_read.push_back(alarmed_list[i]);
        }
    }
}

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::state2attr
//
// description :
//        Method to send a device state as an attribute object
//
// argument:
//        in :
//            - state : The device state
//            - back : The AttributeValue structure
//
//---------------------------------------------------------------------------------------------------------------------

void Device_3Impl::state2attr(Tango::DevState state, Tango::AttributeValue_3 &back)
{
    base_state2attr(back);

    back.value <<= state;
}

void Device_3Impl::state2attr(Tango::DevState state, Tango::AttributeValue_4 &back)
{
    base_state2attr(back);

    back.value.dev_state_att(state);
    back.data_format = Tango::SCALAR;
}

void Device_3Impl::state2attr(Tango::DevState state, Tango::AttributeValue_5 &back)
{
    base_state2attr(back);

    back.value.dev_state_att(state);
    back.data_format = Tango::SCALAR;
    back.data_type = DEV_STATE;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Device_3Impl::status2attr
//
// description :
//        Method to send a device status string as an attribute object
//
// argument:
//        in :
//            - status : The device status
//            - back : The AttributeValue structure
//
//--------------------------------------------------------------------------------------------------------------------

void Device_3Impl::status2attr(Tango::ConstDevString status, Tango::AttributeValue_3 &back)
{
    base_status2attr(back);

    Tango::DevVarStringArray str_seq(1);
    str_seq.length(1);
    str_seq[0] = Tango::string_dup(status);
    back.value <<= str_seq;
}

void Device_3Impl::status2attr(Tango::ConstDevString status, Tango::AttributeValue_4 &back)
{
    base_status2attr(back);

    Tango::DevVarStringArray str_seq(1);
    str_seq.length(1);
    str_seq[0] = Tango::string_dup(status);
    back.value.string_att_value(str_seq);

    back.data_format = Tango::SCALAR;
}

void Device_3Impl::status2attr(Tango::ConstDevString status, Tango::AttributeValue_5 &back)
{
    base_status2attr(back);

    Tango::DevVarStringArray str_seq(1);
    str_seq.length(1);
    str_seq[0] = Tango::string_dup(status);
    back.value.string_att_value(str_seq);

    back.data_format = Tango::SCALAR;
    back.data_type = DEV_STRING;
}

void Device_3Impl::set_attribute_config_3_local(const Tango::AttributeConfigList_3 &new_conf,
                                                bool fwd_cb,
                                                int caller_idl)
{
    ::set_attribute_config_3_local(this, new_conf, new_conf[0], fwd_cb, caller_idl);
}

void Device_3Impl::set_attribute_config_3_local(const Tango::AttributeConfigList_5 &new_conf,
                                                bool fwd_cb,
                                                int caller_idl)
{
    ::set_attribute_config_3_local(this, new_conf, new_conf[0], fwd_cb, caller_idl);
}
} // namespace Tango
