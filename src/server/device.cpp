//+==================================================================================================================
//
// file :        Device.cpp
//
// description :    C++ source code for the DeviceImpl class. This class is the root class for all derived Device
//                    classes. It is an abstract class. The DeviceImpl class is the CORBA servant which is "exported"
//                    onto the network and accessed by the client.
//
// project :        TANGO
//
// author(s) :        A.Gotz + E.Taurel
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//-================================================================================================================

#include <new>

#include <tango/server/basiccommand.h>
#include <tango/server/blackbox.h>
#include <tango/server/dserversignal.h>
#include <tango/server/classattribute.h>
#include <tango/server/eventsupplier.h>
#include <tango/server/pipe.h>
#include <tango/server/fwdattribute.h>
#include <tango/server/dserver.h>
#include <tango/server/log4tango.h>
#include <tango/client/apiexcept.h>
#include <tango/server/tango_clock.h>
#include <tango/common/git_revision.h>
#include <tango/server/logging.h>
#include <tango/client/DbDevice.h>
#include <tango/internal/telemetry/telemetry_kernel_macros.h>

#if defined(TANGO_USE_TELEMETRY)
  #include <opentelemetry/version.h>
#endif

namespace Tango
{

namespace
{
template <class T>
T &get_any_value(Tango::AttrValUnion &);

template <class T>
void set_union_value(Tango::AttrValUnion &val, T &arr);

template <class T>
T &get_union_value(Tango::AttrValUnion &val);

template <class T>
void data_in_object(Attribute &att, AttributeIdlData &aid, long index, bool del_seq);

template <class T>
void data_in_net_object(AttributeIdlData &aid, long index, long vers, PollObj *polled_att);

template <>
inline Tango::DevVarShortArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarShortArray tmp;
    val.short_att_value(tmp);
    return val.short_att_value();
}

template <>
inline Tango::DevVarLongArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarLongArray tmp;
    val.long_att_value(tmp);
    return val.long_att_value();
}

template <>
inline Tango::DevVarLong64Array &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarLong64Array tmp;
    val.long64_att_value(tmp);
    return val.long64_att_value();
}

template <>
inline Tango::DevVarDoubleArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarDoubleArray tmp;
    val.double_att_value(tmp);
    return val.double_att_value();
}

template <>
inline Tango::DevVarFloatArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarFloatArray tmp;
    val.float_att_value(tmp);
    return val.float_att_value();
}

template <>
inline Tango::DevVarStringArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarStringArray tmp;
    val.string_att_value(tmp);
    return val.string_att_value();
}

template <>
inline Tango::DevVarBooleanArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarBooleanArray tmp;
    val.bool_att_value(tmp);
    return val.bool_att_value();
}

template <>
inline Tango::DevVarUShortArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarUShortArray tmp;
    val.ushort_att_value(tmp);
    return val.ushort_att_value();
}

template <>
inline Tango::DevVarUCharArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarCharArray tmp;
    val.uchar_att_value(tmp);
    return val.uchar_att_value();
}

template <>
inline Tango::DevVarULongArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarULongArray tmp;
    val.ulong_att_value(tmp);
    return val.ulong_att_value();
}

template <>
inline Tango::DevVarULong64Array &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarULong64Array tmp;
    val.ulong64_att_value(tmp);
    return val.ulong64_att_value();
}

template <>
inline Tango::DevVarStateArray &get_any_value(Tango::AttrValUnion &val)
{
    Tango::DevVarStateArray tmp;
    val.state_att_value(tmp);
    return val.state_att_value();
}

template <>
inline Tango::DevVarShortArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.short_att_value();
}

template <>
inline Tango::DevVarLongArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.long_att_value();
}

template <>
inline Tango::DevVarLong64Array &get_union_value(Tango::AttrValUnion &val)
{
    return val.long64_att_value();
}

template <>
inline Tango::DevVarDoubleArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.double_att_value();
}

template <>
inline Tango::DevVarFloatArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.float_att_value();
}

template <>
inline Tango::DevVarStringArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.string_att_value();
}

template <>
inline Tango::DevVarBooleanArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.bool_att_value();
}

template <>
inline Tango::DevVarUShortArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.ushort_att_value();
}

template <>
inline Tango::DevVarUCharArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.uchar_att_value();
}

template <>
inline Tango::DevVarULongArray &get_union_value(Tango::AttrValUnion &val)
{
    return val.ulong_att_value();
}

template <>
inline Tango::DevVarULong64Array &get_union_value(Tango::AttrValUnion &val)
{
    return val.ulong64_att_value();
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarShortArray &arr)
{
    val.short_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarLongArray &arr)
{
    val.long_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarLong64Array &arr)
{
    val.long64_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarDoubleArray &arr)
{
    val.double_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarFloatArray &arr)
{
    val.float_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarStringArray &arr)
{
    val.string_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarBooleanArray &arr)
{
    val.bool_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarUShortArray &arr)
{
    val.ushort_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarUCharArray &arr)
{
    val.uchar_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarULongArray &arr)
{
    val.ulong_att_value(arr);
}

template <>
inline void set_union_value(Tango::AttrValUnion &val, Tango::DevVarULong64Array &arr)
{
    val.ulong64_att_value(arr);
}

template <class T>
inline void data_in_net_object(AttributeIdlData &aid, long index, long vers, PollObj *polled_att)
{
    if(aid.data_5 != nullptr)
    {
        AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
        set_union_value((*aid.data_5)[index].value,
                        get_union_value<typename tango_type_traits<T>::ArrayType>(att_val.value));
    }
    else if(aid.data_4 != nullptr)
    {
        if(vers >= 5)
        {
            AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
            set_union_value((*aid.data_4)[index].value,
                            get_union_value<typename tango_type_traits<T>::ArrayType>(att_val.value));
        }
        else
        {
            AttributeValue_4 &att_val = polled_att->get_last_attr_value_4(false);
            set_union_value((*aid.data_4)[index].value,
                            get_union_value<typename tango_type_traits<T>::ArrayType>(att_val.value));
        }
    }
    else
    {
        typename tango_type_traits<T>::ArrayType *tmp;
        if(vers >= 5)
        {
            AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
            typename tango_type_traits<T>::ArrayType &union_seq =
                get_union_value<typename tango_type_traits<T>::ArrayType>(att_val.value);
            tmp = new typename tango_type_traits<T>::ArrayType(
                union_seq.length(), union_seq.length(), const_cast<T *>(union_seq.get_buffer()), false);
        }
        else if(vers == 4)
        {
            AttributeValue_4 &att_val = polled_att->get_last_attr_value_4(false);
            typename tango_type_traits<T>::ArrayType &union_seq =
                get_union_value<typename tango_type_traits<T>::ArrayType>(att_val.value);
            tmp = new typename tango_type_traits<T>::ArrayType(
                union_seq.length(), union_seq.length(), const_cast<T *>(union_seq.get_buffer()), false);
        }
        else
        {
            const typename tango_type_traits<T>::ArrayType *tmp_cst;
            AttributeValue_3 &att_val = polled_att->get_last_attr_value_3(false);
            att_val.value >>= tmp_cst;
            tmp = new typename tango_type_traits<T>::ArrayType(
                tmp_cst->length(), tmp_cst->length(), const_cast<T *>(tmp_cst->get_buffer()), false);
        }
        (*aid.data_3)[index].value <<= tmp;
    }
}

template <class T>
inline void data_in_object(Attribute &att, AttributeIdlData &aid, long index, bool del_seq)
{
    auto *ptr = att.get_value_storage<T>();
    if(aid.data_5 != nullptr)
    {
        auto &the_seq = get_any_value<T>((*aid.data_5)[index].value);
        the_seq.replace(ptr->length(), ptr->length(), ptr->get_buffer(), ptr->release());
        if(ptr->release())
        {
            ptr->get_buffer(true);
        }
    }
    else if(aid.data_4 != nullptr)
    {
        auto &the_seq = get_any_value<T>((*aid.data_4)[index].value);
        the_seq.replace(ptr->length(), ptr->length(), ptr->get_buffer(), ptr->release());
        if(ptr->release() == true)
        {
            ptr->get_buffer(true);
        }
    }
    else
    {
        (*aid.data_3)[index].value <<= *ptr;
    }

    if(del_seq)
    {
        att.delete_seq();
    }
}

} // namespace

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::DeviceImpl
//
// description :    constructors for the device_impl class from the class object
//            pointer, the device name, the description field,
//            the =aqmz and the status.
//
// argument : in :    - cl_ptr : The class object pointer
//            - d_name : The device name
//            - de : The device description (default to "A TANGO device")
//            - st : The device state (default to UNKNOWN)
//            - sta : The device status (default to "Not initialised")
//
//--------------------------------------------------------------------------

DeviceImpl::DeviceImpl(
    DeviceClass *cl_ptr, std::string_view d_name, std::string_view de, Tango::DevState st, std ::string_view sta) :
    device_name(d_name),
    desc(de),
    device_status(sta),
    device_state(st),
    device_class(cl_ptr),
    ext(new DeviceImplExt),
    rft(Tango::kDefaultRollingThreshold),
    only_one(d_name),
    poll_mon(std::string{d_name} + " cache"),
    att_conf_mon(std::string{d_name} + " att_config")
{
    real_ctor();
}

void DeviceImpl::real_ctor()
{
    TANGO_LOG_DEBUG << "Entering DeviceImpl::real_ctor for device " << device_name << std::endl;
    version = DevVersion;
    blackbox_depth = 0;

    device_prev_state = device_state;

    //
    // Init lower case device name
    //

    device_name_lower = device_name;
    std::transform(device_name_lower.begin(), device_name_lower.end(), device_name_lower.begin(), ::tolower);

    //
    //  Write the device name into the per thread data for sub device diagnostics
    //

    Tango::Util *tg = Tango::Util::instance();
    tg->get_sub_dev_diag().set_associated_device(device_name_lower);

    //
    // Create the DbDevice object
    //

    try
    {
        db_dev = new DbDevice(device_name, Tango::Util::instance()->get_database());
    }
    catch(Tango::DevFailed &)
    {
        throw;
    }

    get_dev_system_resource();

    black_box_create();

    idl_version = 1;
    devintr_shared.th_running = false;

    //
    // Create the multi attribute object
    //

    dev_attr = new MultiAttribute(device_name, device_class, this);

    //
    // Create device pipe and finish the pipe config init since we now have device name
    //

    device_class->create_device_pipe(device_class, this);
    end_pipe_config();

    //
    // Build adm device name
    //

    adm_device_name = "dserver/";
    adm_device_name = adm_device_name + Util::instance()->get_ds_name();

    //
    // Init logging
    //

    init_logger();

    //
    // Set c++ version_info list values, got from tango_const.h and zmq.h
    //

    add_version_info("cppTango", TgLibVers);
    add_version_info("cppTango.git_revision", Tango::git_revision());
    add_version_info("omniORB", omniORB::versionString());
    add_version_info("zmq", std::to_string(ZMQ_VERSION));
    add_version_info("cppzmq", std::to_string(CPPZMQ_VERSION));
    add_version_info("idl", TANGO_IDL_VERSION_STR);

#if defined(TANGO_USE_TELEMETRY)
    add_version_info("opentelemetry-cpp", OPENTELEMETRY_VERSION);
#endif

    //
    // Init telemetry
    //
#if defined(TANGO_USE_TELEMETRY)
    // initialize the telemetry interface
    initialize_telemetry_interface();
    // attach the device's telemetry interface to the current thread
    Tango::telemetry::Interface::set_current(telemetry());
#endif

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::real_ctor for device " << device_name << std::endl;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::stop_polling
//
// description :    Stop all polling for a device. if the device is
//            polled, call this method before deleting it.
//
// argin(s) : - with_db_upd : Is it necessary to update db ?
//
//--------------------------------------------------------------------------

void DeviceImpl::stop_polling(bool with_db_upd)
{
    Tango::Util *tg = Tango::Util::instance();

    //
    // If the vector of polling info is empty, no need to do anything (polling
    // already stopped for all devices)
    //

    std::vector<PollingThreadInfo *> &v_th_info = tg->get_polling_threads_info();
    if(v_th_info.empty())
    {
        return;
    }

    //
    // Find out which thread is in charge of the device.
    //

    PollingThreadInfo *th_info;

    int poll_th_id = tg->get_polling_thread_id_by_name(device_name.c_str());
    if(poll_th_id == 0)
    {
        TangoSys_OMemStream o;
        o << "Can't find a polling thread for device " << device_name << std::ends;
        TANGO_THROW_EXCEPTION(API_PollingThreadNotFound, o.str());
    }

    th_info = tg->get_polling_thread_info_by_id(poll_th_id);

    TangoMonitor &mon = th_info->poll_mon;
    PollThCmd &shared_cmd = th_info->shared_data;

    {
        omni_mutex_lock sync(mon);
        if(shared_cmd.cmd_pending)
        {
            mon.wait();
        }
        shared_cmd.cmd_pending = true;
        shared_cmd.cmd_code = POLL_REM_DEV;
        shared_cmd.dev = this;

        mon.signal();

        //
        // Wait for thread to execute command
        //

        while(shared_cmd.cmd_pending)
        {
            int interupted = mon.wait(DEFAULT_TIMEOUT);

            if((shared_cmd.cmd_pending) && (interupted == 0))
            {
                TANGO_LOG_DEBUG << "TIME OUT" << std::endl;
                TANGO_THROW_EXCEPTION(API_CommandTimedOut, "Polling thread blocked !!");
            }
        }
    }

    is_polled(false);

    //
    // Update the pool conf first locally.
    // Also update the map<device name,thread id>
    // If this device was the only one for a polling thread, kill the thread
    // Then in Db if possible
    //

    bool kill_thread = false;
    int ind;

    if((ind = tg->get_dev_entry_in_pool_conf(device_name_lower)) == -1)
    {
        TangoSys_OMemStream o;
        o << "Can't find entry for device " << device_name << " in polling threads pool configuration !" << std::ends;
        TANGO_THROW_EXCEPTION(API_PolledDeviceNotInPoolConf, o.str());
    }

    std::vector<std::string> &pool_conf = tg->get_poll_pool_conf();
    std::string &conf_entry = pool_conf[ind];
    std::string::size_type pos;
    if((pos = conf_entry.find(',')) != std::string::npos)
    {
        pos = conf_entry.find(device_name_lower);
        if((pos + device_name_lower.size()) != conf_entry.size())
        {
            conf_entry.erase(pos, device_name_lower.size() + 1);
        }
        else
        {
            conf_entry.erase(pos - 1);
        }
    }
    else
    {
        auto iter = pool_conf.begin() + ind;
        pool_conf.erase(iter);
        kill_thread = true;
    }

    tg->remove_dev_from_polling_map(device_name_lower);

    //
    // Kill the thread if needed and join
    //

    if(kill_thread)
    {
        TangoMonitor &mon = th_info->poll_mon;
        PollThCmd &shared_cmd = th_info->shared_data;

        {
            omni_mutex_lock sync(mon);

            shared_cmd.cmd_pending = true;
            shared_cmd.cmd_code = POLL_EXIT;

            mon.signal();
        }

        void *dummy_ptr;
        TANGO_LOG_DEBUG << "POLLING: Joining with one polling thread" << std::endl;
        th_info->poll_th->join(&dummy_ptr);

        tg->remove_polling_thread_info_by_id(poll_th_id);
    }

    //
    // Update db
    //

    if((with_db_upd) && (Tango::Util::instance()->use_db()))
    {
        DbData send_data;
        send_data.emplace_back("polling_threads_pool_conf");
        send_data[0] << tg->get_poll_pool_conf();

        tg->get_dserver_device()->get_db_device()->put_property(send_data);
    }
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::~DeviceImpl
//
// description :    Destructor for the device class. It simply frees
//            the memory allocated for the black box object
//
//--------------------------------------------------------------------------

DeviceImpl::~DeviceImpl()
{
    TANGO_LOG_DEBUG << "Entering DeviceImpl destructor for device " << device_name << std::endl;

    //
    // Call user delete_device method
    //

    delete_device();

    //
    // Delete the black box
    //

    blackbox_ptr.reset();

    //
    // Delete the DbDevice object
    //

    delete db_dev;

    //
    // Unregister the signal from signal handler
    //

    DServerSignal::instance()->unregister_dev_signal(this);

    //
    // Delete the multi attribute object
    //

    delete dev_attr;

    //
    // Clean up previously executed in extension destructor
    // Deletes memory for ring buffer used for polling
    //

    for(unsigned long i = 0; i < poll_obj_list.size(); i++)
    {
        delete(poll_obj_list[i]);
    }

#if defined(TANGO_USE_TELEMETRY)
    cleanup_telemetry_interface();
#endif

    if((logger != nullptr) && logger != Logging::get_core_logger())
    {
        logger->remove_all_appenders();
        delete logger;
        logger = nullptr;
    }

    delete locker_client;
    delete old_locker_client;

    //
    // Delete the extension class instance
    //

    //
    // Clear our ptr in the device class vector
    //

    std::vector<DeviceImpl *> &dev_vect = get_device_class()->get_device_list();
    auto ite = find(dev_vect.begin(), dev_vect.end(), this);
    if(ite != dev_vect.end())
    {
        *ite = nullptr;
    }

    // remove any device level dynamic commands
    for(auto *entry : get_local_command_list())
    {
        delete entry;
        entry = nullptr;
    }

    TANGO_LOG_DEBUG << "Leaving DeviceImpl destructor for device " << device_name << std::endl;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::black_box_create
//
// description :    Private method to create the device black box.
//            The black box depth is a resource with a default
//            value if the resource is not defined
//
//--------------------------------------------------------------------------

void DeviceImpl::black_box_create()
{
    //
    // If the black box depth object attribute is null, create one with the
    // default depth
    //

    if(blackbox_depth == 0)
    {
        blackbox_ptr = std::make_unique<BlackBox>();
    }
    else
    {
        blackbox_ptr = std::make_unique<BlackBox>(blackbox_depth);
    }
}

//+----------------------------------------------------------------------------
//
// method :        DeviceImpl::get_dev_system_resource()
//
// description :    Method to retrieve some basic device resources
//            The resources to be retrived are :
//                - The black box depth
//                - The device description
//                - The polling ring buffer depth
//                - The polled command(s)
//                - The polled attribute(s)
//                - The non automatic polled command list
//                - The non automatic polled attribute list
//                - The polling too old factor
//                - The command polling ring depth (if any)
//                - The attribute polling ring depth (if any)
//
//-----------------------------------------------------------------------------

void DeviceImpl::get_dev_system_resource()
{
    //
    // Try to retrieve resources for device black box depth and device
    // description
    //

    Tango::Util *tg = Tango::Util::instance();
    if(tg->use_db())
    {
        DbData db_data;

        db_data.emplace_back("blackbox_depth");
        db_data.emplace_back("description");
        db_data.emplace_back("poll_ring_depth");
        db_data.emplace_back("polled_cmd");
        db_data.emplace_back("polled_attr");
        db_data.emplace_back("non_auto_polled_cmd");
        db_data.emplace_back("non_auto_polled_attr");
        db_data.emplace_back("poll_old_factor");
        db_data.emplace_back("cmd_poll_ring_depth");
        db_data.emplace_back("attr_poll_ring_depth");
        db_data.emplace_back("min_poll_period");
        db_data.emplace_back("cmd_min_poll_period");
        db_data.emplace_back("attr_min_poll_period");

        try
        {
            db_dev->get_property(db_data);
        }
        catch(Tango::DevFailed &)
        {
            TangoSys_OMemStream o;
            o << "Database error while trying to retrieve device prperties for device " << device_name.c_str()
              << std::ends;

            TANGO_THROW_EXCEPTION(API_DatabaseAccess, o.str());
        }

        if(!db_data[0].is_empty())
        {
            db_data[0] >> blackbox_depth;
        }
        if(!db_data[1].is_empty())
        {
            db_data[1] >> desc;
        }
        if(!db_data[2].is_empty())
        {
            long tmp_depth;
            db_data[2] >> tmp_depth;
            set_poll_ring_depth(tmp_depth);
        }
        if(!db_data[3].is_empty())
        {
            db_data[3] >> get_polled_cmd();
        }
        if(!db_data[4].is_empty())
        {
            db_data[4] >> get_polled_attr();
        }
        if(!db_data[5].is_empty())
        {
            db_data[5] >> get_non_auto_polled_cmd();
        }
        if(!db_data[6].is_empty())
        {
            db_data[6] >> get_non_auto_polled_attr();
        }
        if(!db_data[7].is_empty())
        {
            long tmp_poll;
            db_data[7] >> tmp_poll;
            set_poll_old_factor(tmp_poll);
        }
        else
        {
            set_poll_old_factor(DEFAULT_POLL_OLD_FACTOR);
        }
        if(!db_data[8].is_empty())
        {
            db_data[8] >> cmd_poll_ring_depth;
            unsigned long nb_prop = cmd_poll_ring_depth.size();
            if((nb_prop % 2) == 1)
            {
                cmd_poll_ring_depth.clear();
                TangoSys_OMemStream o;
                o << "System property cmd_poll_ring_depth for device " << device_name << " has wrong syntax"
                  << std::ends;
                TANGO_THROW_EXCEPTION(API_BadConfigurationProperty, o.str());
            }
            for(unsigned int i = 0; i < nb_prop; i = i + 2)
            {
                std::transform(cmd_poll_ring_depth[i].begin(),
                               cmd_poll_ring_depth[i].end(),
                               cmd_poll_ring_depth[i].begin(),
                               ::tolower);
            }
        }
        if(!db_data[9].is_empty())
        {
            db_data[9] >> attr_poll_ring_depth;
            unsigned long nb_prop = attr_poll_ring_depth.size();
            if((attr_poll_ring_depth.size() % 2) == 1)
            {
                attr_poll_ring_depth.clear();
                TangoSys_OMemStream o;
                o << "System property attr_poll_ring_depth for device " << device_name << " has wrong syntax"
                  << std::ends;
                TANGO_THROW_EXCEPTION(API_BadConfigurationProperty, o.str());
            }
            for(unsigned int i = 0; i < nb_prop; i = i + 2)
            {
                std::transform(attr_poll_ring_depth[i].begin(),
                               attr_poll_ring_depth[i].end(),
                               attr_poll_ring_depth[i].begin(),
                               ::tolower);
            }
        }

        //
        // The min. period related properties
        //

        if(!db_data[10].is_empty())
        {
            db_data[10] >> min_poll_period;
        }

        if(!db_data[11].is_empty())
        {
            db_data[11] >> cmd_min_poll_period;
            unsigned long nb_prop = cmd_min_poll_period.size();
            if((cmd_min_poll_period.size() % 2) == 1)
            {
                cmd_min_poll_period.clear();
                TangoSys_OMemStream o;
                o << "System property cmd_min_poll_period for device " << device_name << " has wrong syntax"
                  << std::ends;
                TANGO_THROW_EXCEPTION(API_BadConfigurationProperty, o.str());
            }
            for(unsigned int i = 0; i < nb_prop; i = i + 2)
            {
                std::transform(cmd_min_poll_period[i].begin(),
                               cmd_min_poll_period[i].end(),
                               cmd_min_poll_period[i].begin(),
                               ::tolower);
            }
        }

        if(!db_data[12].is_empty())
        {
            db_data[12] >> attr_min_poll_period;
            unsigned long nb_prop = attr_min_poll_period.size();
            if((attr_min_poll_period.size() % 2) == 1)
            {
                attr_min_poll_period.clear();
                TangoSys_OMemStream o;
                o << "System property attr_min_poll_period for device " << device_name << " has wrong syntax"
                  << std::ends;
                TANGO_THROW_EXCEPTION(API_BadConfigurationProperty, o.str());
            }
            for(unsigned int i = 0; i < nb_prop; i = i + 2)
            {
                std::transform(attr_min_poll_period[i].begin(),
                               attr_min_poll_period[i].end(),
                               attr_min_poll_period[i].begin(),
                               ::tolower);
            }
        }

        //
        // Since Tango V5 (IDL V3), State and Status are now polled as attributes
        // Change properties if necessary
        //

        if((get_polled_cmd()).size() != 0)
        {
            poll_lists_2_v5();
        }
    }
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::_default_POA
//
// description :    Return a pointer to the POA on which the device should
//            be activated. This method is required by CORBA to
//            create a POA with the IMPLICIT_ACTIVATION policy
//
//--------------------------------------------------------------------------

PortableServer::POA_ptr DeviceImpl::_default_POA()
{
    return Util::instance()->get_poa();
}

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//		DeviceImpl::add_version_info
//
// description :
//		add key=value pair to version_info list
//
//--------------------------------------------------------------------------------------------------------------------

void DeviceImpl::add_version_info(const std::string &key, const std::string &value)
{
    version_info.insert_or_assign(key, value);
    TANGO_LOG_DEBUG << "In DeviceImpl::add_version_info(key = " << key << ", value = " << value << ")" << std::endl;
}

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//		DeviceImpl::get_version_info
//
// description :
//		get current libraries versions list
//
//--------------------------------------------------------------------------------------------------------------------

Tango::DevInfoVersionList DeviceImpl::get_version_info()
{
    //
    // Convert version_info_list map into a DevInfoVersionList sequence
    //

    Tango::DevInfoVersionList dev_version_info;
    int version_length = 0;
    dev_version_info.length(version_info.size());

    for(const auto &[key, value] : version_info)
    {
        Tango::DevInfoVersion version_info_elt;
        version_info_elt.key = Tango::string_dup(key.c_str());
        version_info_elt.value = Tango::string_dup(value.c_str());
        dev_version_info[version_length] = version_info_elt;
        version_length++;
    }

    return dev_version_info;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::register_signal
//
// description :    Method to register a device on a signal. When the
//            signal is sent to the process, the signal_handler
//            method of this class will be executed
//
// in :         signo : The signal number
//
//--------------------------------------------------------------------------

#ifndef _TG_WINDOWS_
void DeviceImpl::register_signal(long signo, bool hand)
{
    TANGO_LOG_DEBUG << "DeviceImpl::register_signal() arrived for signal " << signo << std::endl;

    DServerSignal::instance()->register_dev_signal(signo, hand, this);

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::register_signal method()" << std::endl;
}
#else
void DeviceImpl::register_signal(long signo)
{
    TANGO_LOG_DEBUG << "DeviceImpl::register_signal() arrived for signal " << signo << std::endl;

    DServerSignal::instance()->register_dev_signal(signo, this);

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::register_signal method()" << std::endl;
}
#endif

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::unregister_signal
//
// description :    Method to unregister a device on a signal.
//
// in :         signo : The signal number
//
//--------------------------------------------------------------------------

void DeviceImpl::unregister_signal(long signo)
{
    TANGO_LOG_DEBUG << "DeviceImpl::unregister_signal() arrived for signal " << signo << std::endl;

    DServerSignal::instance()->unregister_dev_signal(signo, this);

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::unregister_signal method()" << std::endl;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::signal_handler
//
// description :    This is the signal handler for the device. This method
//            is defined as virtual and therefore, can be redefined
//            by DS programmers in their own classes derived from
//            DeviceImpl
//
// in :         signo : The signal number
//
//--------------------------------------------------------------------------

void DeviceImpl::signal_handler(long signo)
{
    TANGO_LOG_DEBUG << "DeviceImpl::signal_handler() arrived for signal " << signo << std::endl;

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::signal_handler method()" << std::endl;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::check_command_exist
//
// description :    This method check that a comamnd is supported by
//            the device and does not need input value.
//            The method throws an exception if the
//            command is not defined or needs an input value
//
// in :         cmd_name : The command name
//
//--------------------------------------------------------------------------

void DeviceImpl::check_command_exists(const std::string &cmd_name)
{
    std::vector<Command *> &cmd_list = device_class->get_command_list();
    unsigned long i;
    for(i = 0; i < cmd_list.size(); i++)
    {
        if(cmd_list[i]->get_lower_name() == cmd_name)
        {
            if(cmd_list[i]->get_in_type() != Tango::DEV_VOID)
            {
                TangoSys_OMemStream o;
                o << "Command " << cmd_name << " cannot be polled because it needs input value" << std::ends;
                TANGO_THROW_EXCEPTION(API_IncompatibleCmdArgumentType, o.str());
            }
            return;
        }
    }

    TangoSys_OMemStream o;
    o << "Command " << cmd_name << " not found" << std::ends;
    TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::get_command
//
// description :    This method returns a pointer to command object.
//            The method throws an exception if the
//            command is not defined
//
// in :         cmd_name : The command name
//
//--------------------------------------------------------------------------

Command *DeviceImpl::get_command(const std::string &cmd_name)
{
    std::vector<Command *> cmd_list = device_class->get_command_list();
    unsigned long i;
    for(i = 0; i < cmd_list.size(); i++)
    {
        if(cmd_list[i]->get_lower_name() == cmd_name)
        {
            return cmd_list[i];
        }
    }

    TangoSys_OMemStream o;
    o << "Command " << cmd_name << " not found" << std::ends;
    TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());

    //
    // This piece of code is added for VC++ compiler. As they advice, I have try to
    // use the __decspec(noreturn) for the throw_exception method, but it seems
    // that it does not work !
    //

    return nullptr;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::get_polled_obj_by_type_name
//
// description :    This method check that a comamnd is supported by
//            the device and does not need input value.
//            The method throws an exception if the
//            command is not defined or needs an input value
//
// in :         cmd_name : The command name
//
//--------------------------------------------------------------------------

std::vector<PollObj *>::iterator DeviceImpl::get_polled_obj_by_type_name(Tango::PollObjType obj_type,
                                                                         const std::string &obj_name)
{
    std::vector<PollObj *> &po_list = get_poll_obj_list();
    std::vector<PollObj *>::iterator ite;
    for(ite = po_list.begin(); ite < po_list.end(); ++ite)
    {
        omni_mutex_lock sync(**ite);
        {
            if((*ite)->get_type_i() == obj_type)
            {
                if((*ite)->get_name_i() == obj_name)
                {
                    return ite;
                }
            }
        }
    }

    TangoSys_OMemStream o;
    o << obj_name << " not found in list of polled object" << std::ends;
    TANGO_THROW_EXCEPTION(API_PollObjNotFound, o.str());
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::get_cmd_poll_ring_depth
//
// description :
//        This method returns the polling buffer depth. Most of the times, this is defined at device level
//        via the device "poll_ring_depth" property. Nevertheless, in some cases, this value can be overwritten via the
//        device "cmd_poll_ring_depth" property.
//
// args :
//         in :
//            - cmd_name : The command name
//
// return :
//         This method returns the polling buffer depth
//
//-------------------------------------------------------------------------------------------------------------------

long DeviceImpl::get_cmd_poll_ring_depth(const std::string &cmd_name)
{
    long ret;

    if(cmd_poll_ring_depth.size() == 0)
    {
        //
        // No specific depth defined
        //
        if(poll_ring_depth == 0)
        {
            ret = DefaultPollRingDepth;
        }
        else
        {
            ret = poll_ring_depth;
        }
    }
    else
    {
        unsigned long k;
        //
        // Try to find command in list of specific polling buffer depth
        //

        for(k = 0; k < cmd_poll_ring_depth.size(); k = k + 2)
        {
            if(cmd_poll_ring_depth[k] == cmd_name)
            {
                TangoSys_MemStream s;
                s << cmd_poll_ring_depth[k + 1];
                if(!(s >> ret))
                {
                    TangoSys_OMemStream o;
                    o << "System property cmd_poll_ring_depth for device " << device_name << " has wrong syntax"
                      << std::ends;
                    TANGO_THROW_EXCEPTION(API_BadConfigurationProperty, o.str());
                }
                break;
            }
        }
        if(k >= cmd_poll_ring_depth.size())
        {
            //
            // Not found
            //

            if(poll_ring_depth == 0)
            {
                ret = DefaultPollRingDepth;
            }
            else
            {
                ret = poll_ring_depth;
            }
        }
    }

    return ret;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::get_attr_poll_ring_depth
//
// description :
//        This method returns the polling buffer depth. Most of the times, this is defined at device level
//        via the device "poll_ring_depth" property. Nevertheless, in some cases, this value can be overwritten via the
//        device "attr_poll_ring_depth" property.
//
// args :
//         in :
//            - attr_name : The attribute name
//
// return :
//         This method returns the polling buffer depth
//
//--------------------------------------------------------------------------

long DeviceImpl::get_attr_poll_ring_depth(const std::string &attr_name)
{
    long ret;

    if(attr_poll_ring_depth.size() == 0)
    {
        if((attr_name == "state") || (attr_name == "status"))
        {
            ret = get_cmd_poll_ring_depth(attr_name);
        }
        else
        {
            //
            // No specific depth defined
            //

            if(poll_ring_depth == 0)
            {
                ret = DefaultPollRingDepth;
            }
            else
            {
                ret = poll_ring_depth;
            }
        }
    }
    else
    {
        unsigned long k;
        //
        // Try to find command in list of specific polling buffer depth
        //

        for(k = 0; k < attr_poll_ring_depth.size(); k = k + 2)
        {
            if(attr_poll_ring_depth[k] == attr_name)
            {
                TangoSys_MemStream s;
                s << attr_poll_ring_depth[k + 1];
                if(!(s >> ret))
                {
                    TangoSys_OMemStream o;
                    o << "System property attr_poll_ring_depth for device " << device_name << " has wrong syntax"
                      << std::ends;
                    TANGO_THROW_EXCEPTION(API_BadConfigurationProperty, o.str());
                }
                break;
            }
        }
        if(k >= attr_poll_ring_depth.size())
        {
            if((attr_name == "state") || (attr_name == "status"))
            {
                ret = get_cmd_poll_ring_depth(attr_name);
            }
            else
            {
                //
                // Not found
                //

                if(poll_ring_depth == 0)
                {
                    ret = DefaultPollRingDepth;
                }
                else
                {
                    ret = poll_ring_depth;
                }
            }
        }
    }

    return ret;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::dev_state
//
// description :
//        The default method called by the DevState command. If the device is ON, this method checks attribute
//        with a defined alarm and set the state to ALARM if one of these attribute is in alarm. Otherwise, simply
//        returns device state
//
//--------------------------------------------------------------------------------------------------------------------

Tango::DevState DeviceImpl::dev_state()
{
    NoSyncModelTangoMonitor mon(this);

    //
    // If we need to run att. conf loop, do it.
    // If the flag to force state is true, do not call state computation method, simply set it to ALARM
    //

    if(run_att_conf_loop)
    {
        att_conf_loop();
    }

    if(device_state != Tango::FAULT && force_alarm_state)
    {
        return Tango::ALARM;
    }
    else
    {
        if((device_state == Tango::ON) || (device_state == Tango::ALARM))
        {
            //
            // Build attribute lists
            //

            long vers = get_dev_idl_version();
            bool set_alrm = false;

            std::vector<long> attr_list = dev_attr->get_alarm_list();
            std::vector<long> attr_list_2 = get_alarmed_not_read();
            std::vector<long> attr_polled_list;
            long nb_wanted_attr;

            if(vers >= 3)
            {
                if(state_from_read)
                {
                    auto ite = attr_list_2.begin();
                    while(ite != attr_list_2.end())
                    {
                        Attribute &att = dev_attr->get_attr_by_ind(*ite);
                        if(att.is_polled())
                        {
                            ite = attr_list_2.erase(ite);
                        }
                        else
                        {
                            ++ite;
                        }
                    }
                    nb_wanted_attr = attr_list_2.size();
                }
                else
                {
                    auto ite = attr_list.begin();
                    while(ite != attr_list.end())
                    {
                        Attribute &att = dev_attr->get_attr_by_ind(*ite);
                        if(att.is_polled())
                        {
                            ite = attr_list.erase(ite);
                        }
                        else
                        {
                            ++ite;
                        }
                    }
                    nb_wanted_attr = attr_list.size();
                }
            }
            else
            {
                nb_wanted_attr = attr_list.size();
            }

            TANGO_LOG_DEBUG << "State: Number of attribute(s) to read: " << nb_wanted_attr << std::endl;

            if(nb_wanted_attr != 0)
            {
                //
                // Read the hardware
                //

                if(!state_from_read)
                {
                    read_attr_hardware(attr_list);
                }

                //
                // Set attr value
                //

                long i;
                std::vector<Tango::Attr *> &attr_vect = device_class->get_class_attr()->get_attr_list();

                for(i = 0; i < nb_wanted_attr; i++)
                {
                    //
                    // Starting with IDL 3, it is possible that some of the alarmed attribute have already been read.
                    //

                    long idx;
                    if((vers >= 3) && (state_from_read))
                    {
                        idx = attr_list_2[i];
                    }
                    else
                    {
                        idx = attr_list[i];
                    }

                    Attribute &att = dev_attr->get_attr_by_ind(idx);
                    att.save_alarm_quality();

                    try
                    {
                        att.wanted_date(false);
                        att.reset_value();

                        if(vers < 3)
                        {
                            read_attr(att);
                        }
                        else
                        {
                            //
                            // Otherwise, get it from device
                            //

                            if(!attr_vect[att.get_attr_idx()]->is_allowed(this, Tango::READ_REQ))
                            {
                                att.wanted_date(true);
                                continue;
                            }
                            attr_vect[att.get_attr_idx()]->read(this, att);
                            Tango::AttrQuality qua = att.get_quality();
                            if((qua != Tango::ATTR_INVALID) && (!att.value_is_set()))
                            {
                                TangoSys_OMemStream o;

                                o << "Read value for attribute ";
                                o << att.get_name();
                                o << " has not been updated";
                                o << "Hint: Did the server follow Tango V5 attribute reading framework ?" << std::ends;

                                TANGO_THROW_EXCEPTION(API_AttrValueNotSet, o.str());
                            }
                        }
                    }
                    catch(Tango::DevFailed &)
                    {
                        if(!att.value_is_set())
                        {
                            WARN_STREAM << "Attribute has no value, forcing INVALID quality for: " << att.get_name()
                                        << std::endl;
                            att.set_quality(Tango::ATTR_INVALID);
                        }

                        if(!att.get_wanted_date())
                        {
                            if(att.get_quality() != Tango::ATTR_INVALID)
                            {
                                att.delete_seq();
                            }
                            att.wanted_date(true);
                        }
                    }
                }

                //
                // Check alarm level
                //

                if(dev_attr->check_alarm())
                {
                    set_alrm = true;
                    if(device_state != Tango::ALARM)
                    {
                        device_state = Tango::ALARM;
                        ext->alarm_state_kernel = Tango::get_current_system_datetime();
                    }
                }
                else
                {
                    if(ext->alarm_state_kernel > ext->alarm_state_user)
                    {
                        device_state = Tango::ON;
                    }
                }

                //
                // Free the sequence created to store the attribute value
                //

                for(long i = 0; i < nb_wanted_attr; i++)
                {
                    long idx;
                    if((vers >= 3) && (state_from_read))
                    {
                        idx = attr_list_2[i];
                    }
                    else
                    {
                        idx = attr_list[i];
                    }
                    Tango::Attribute &att = dev_attr->get_attr_by_ind(idx);
                    if(!att.get_wanted_date())
                    {
                        if(att.get_quality() != Tango::ATTR_INVALID)
                        {
                            att.delete_seq();
                        }
                        att.wanted_date(true);
                    }
                }
            }
            else
            {
                if(ext->alarm_state_kernel > ext->alarm_state_user)
                {
                    device_state = Tango::ON;
                }
            }

            //
            // Check if one of the remaining attributes has its quality factor set to ALARM or WARNING. It is not
            // necessary to do this if we have already detected that the state must switch to ALARM
            //

            if((!set_alrm) && (device_state != Tango::ALARM))
            {
                if(dev_attr->is_att_quality_alarmed())
                {
                    if(device_state != Tango::ALARM)
                    {
                        device_state = Tango::ALARM;
                        ext->alarm_state_kernel = Tango::get_current_system_datetime();
                    }
                }
                else
                {
                    device_state = Tango::ON;
                }
            }
        }
    }

    return device_state;
}

//----------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::dev_status
//
// description :
//        The default method called by the DevStatus command. If the device is ON, this method add Attribute status
//        for all device attribute in alarm state.
//
//---------------------------------------------------------------------------------------------------------------------

Tango::ConstDevString DeviceImpl::dev_status()
{
    NoSyncModelTangoMonitor mon(this);

    const char *returned_str;

    if(run_att_conf_loop)
    {
        att_conf_loop();
    }

    if(device_state != Tango::FAULT && force_alarm_state)
    {
        alarm_status = "The device is in ALARM state.";

        //
        // First add message for attribute with wrong conf. in db
        //

        size_t nb_wrong_att = att_wrong_db_conf.size();
        if(nb_wrong_att != 0)
        {
            alarm_status = alarm_status + "\nAttribute";
            build_att_list_in_status_mess(nb_wrong_att, DeviceImpl::CONF);
            alarm_status = alarm_status + "wrong configuration";
            alarm_status = alarm_status + "\nTry accessing the faulty attribute(s) to get more information";
        }

        //
        // Add message for memorized attributes which failed during device startup
        //

        nb_wrong_att = att_mem_failed.size();
        if(nb_wrong_att != 0)
        {
            alarm_status = alarm_status + "\nMemorized attribute";
            build_att_list_in_status_mess(nb_wrong_att, DeviceImpl::MEM);
            alarm_status = alarm_status + "failed during device startup sequence";
        }

        //
        // Add message for forwarded attributes wrongly configured
        //

        nb_wrong_att = fwd_att_wrong_conf.size();
        if(nb_wrong_att != 0)
        {
            build_att_list_in_status_mess(nb_wrong_att, DeviceImpl::FWD);
        }

        returned_str = alarm_status.c_str();
    }
    else
    {
        if(device_status == StatusNotSet)
        {
            alarm_status = "The device is in ";
            alarm_status = alarm_status + DevStateName[device_state] + " state.";
            if(device_state == Tango::ALARM)
            {
                dev_attr->read_alarm(alarm_status);
                dev_attr->add_alarmed_quality_factor(alarm_status);
            }
            returned_str = alarm_status.c_str();
        }
        else
        {
            if(device_state == Tango::ALARM)
            {
                alarm_status = device_status;
                dev_attr->read_alarm(alarm_status);
                dev_attr->add_alarmed_quality_factor(alarm_status);
                returned_str = alarm_status.c_str();
            }
            else
            {
                returned_str = device_status.c_str();
            }
        }
    }

    return returned_str;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::command_inout
//
// description :    Method called for each command_inout operation executed
//            from any client
//            The call to this method is in a try bloc for the
//            Tango::DevFailed exception (generated by the idl
//            compiler in the tango_skel.cpp file). Therefore, it
//            is not necessary to propagate by re-throw the exceptions
//            thrown by the underlying functions/methods.
//
//--------------------------------------------------------------------------

CORBA::Any *DeviceImpl::command_inout(const char *in_cmd, const CORBA::Any &in_any)
{
    AutoTangoMonitor sync(this);

    std::string command(in_cmd);
    CORBA::Any *out_any;

    TANGO_LOG_DEBUG << "DeviceImpl::command_inout(): command received : " << command << std::endl;

    //
    //  Write the device name into the per thread data for
    //  sub device diagnostics.
    //  Keep the old name, to put it back at the end!
    //  During device access inside the same server,
    //  the thread stays the same!
    //
    SubDevDiag &sub = (Tango::Util::instance())->get_sub_dev_diag();
    std::string last_associated_device = sub.get_associated_device();
    sub.set_associated_device(get_name());

    //
    // Catch all exceptions to set back the associated device after execution
    //

    try
    {
        //
        // Record operation request in black box
        //

        if(store_in_bb)
        {
            blackbox_ptr->insert_cmd(in_cmd);
        }
        store_in_bb = true;

        //
        // Execute command
        //

        out_any = device_class->command_handler(this, command, in_any);
    }
    catch(...)
    {
        // set back the device attribution for the thread
        // and rethrow the exception.
        sub.set_associated_device(last_associated_device);
        throw;
    }

    // set back the device attribution for the thread
    sub.set_associated_device(last_associated_device);

    //
    // Return value to the caller
    //

    TANGO_LOG_DEBUG << "DeviceImpl::command_inout(): leaving method for command " << in_cmd << std::endl;
    return (out_any);
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::name
//
// description :    Method called when a client request the name attribute
//            This method is called for a IDL attribute which can
//            not throw exception to client ==> There is no point
//            to check if the allocation done by the string_dup
//            function failed.
//
//--------------------------------------------------------------------------

char *DeviceImpl::name()
{
    try
    {
        TANGO_LOG_DEBUG << "DeviceImpl::name arrived" << std::endl;

        //
        // Record attribute request in black box
        //

        blackbox_ptr->insert_corba_attr(Attr_Name);
    }
    catch(Tango::DevFailed &e)
    {
        CORBA::IMP_LIMIT lim;
        if(strcmp(e.errors[0].reason, API_CommandTimedOut) == 0)
        {
            lim.minor(TG_IMP_MINOR_TO);
        }
        else
        {
            lim.minor(TG_IMP_MINOR_DEVFAILED);
        }
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::name throwing IMP_LIMIT" << std::endl;
        throw lim;
    }
    catch(...)
    {
        CORBA::IMP_LIMIT lim;
        lim.minor(TG_IMP_MINOR_NON_DEVFAILED);
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::name throwing IMP_LIMIT" << std::endl;
        throw lim;
    }

    //
    // Return data to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::name" << std::endl;
    return CORBA::string_dup(device_name.c_str());
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::adm_name
//
// description :    Method called when a client request the adm_name attribute
//            This method is called for a IDL attribute which can
//            not throw exception to client ==> There is no point
//            to check if the allocation done by the string_dup
//            function failed.
//
//--------------------------------------------------------------------------

char *DeviceImpl::adm_name()
{
    try
    {
        TANGO_LOG_DEBUG << "DeviceImpl::adm_name arrived" << std::endl;

        //
        // Record attribute request in black box
        //

        blackbox_ptr->insert_corba_attr(Attr_AdmName);
    }
    catch(Tango::DevFailed &e)
    {
        CORBA::IMP_LIMIT lim;
        if(strcmp(e.errors[0].reason, API_CommandTimedOut) == 0)
        {
            lim.minor(TG_IMP_MINOR_TO);
        }
        else
        {
            lim.minor(TG_IMP_MINOR_DEVFAILED);
        }
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::adm_name throwing IMP_LIMIT" << std::endl;
        throw lim;
    }
    catch(...)
    {
        CORBA::IMP_LIMIT lim;
        lim.minor(TG_IMP_MINOR_NON_DEVFAILED);
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::adm_name throwing IMP_LIMIT" << std::endl;
        throw lim;
    }

    //
    // Return data to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::adm_name" << std::endl;
    return CORBA::string_dup(adm_device_name.c_str());
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::description
//
// description :    Method called when a client request the description
//            attribute
//            This method is called for a IDL attribute which can
//            not throw exception to client ==> There is no point
//            to check if the allocation done by the string_dup
//            function failed.
//
//--------------------------------------------------------------------------

char *DeviceImpl::description()
{
    try
    {
        TANGO_LOG_DEBUG << "DeviceImpl::description arrived" << std::endl;

        //
        // Record attribute request in black box
        //

        blackbox_ptr->insert_corba_attr(Attr_Description);
    }
    catch(Tango::DevFailed &e)
    {
        CORBA::IMP_LIMIT lim;
        if(strcmp(e.errors[0].reason, API_CommandTimedOut) == 0)
        {
            lim.minor(TG_IMP_MINOR_TO);
        }
        else
        {
            lim.minor(TG_IMP_MINOR_DEVFAILED);
        }
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::description throwing IMP_LIMIT" << std::endl;
        throw lim;
    }
    catch(...)
    {
        CORBA::IMP_LIMIT lim;
        lim.minor(TG_IMP_MINOR_NON_DEVFAILED);
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::description throwing IMP_LIMIT" << std::endl;
        throw lim;
    }

    //
    // Return data to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::description" << std::endl;
    return CORBA::string_dup(desc.c_str());
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::state
//
// description :    Method called when a client request the state
//            attribute
//
//--------------------------------------------------------------------------

Tango::DevState DeviceImpl::state()
{
    Tango::DevState tmp;
    std::string last_associated_device;

    try
    {
        AutoTangoMonitor sync(this);

        TANGO_LOG_DEBUG << "DeviceImpl::state (attribute) arrived" << std::endl;

        //
        //  Write the device name into the per thread data for
        //  sub device diagnostics.
        //  Keep the old name, to put it back at the end!
        //  During device access inside the same server,
        //  the thread stays the same!
        //

        SubDevDiag &sub = (Tango::Util::instance())->get_sub_dev_diag();
        last_associated_device = sub.get_associated_device();
        sub.set_associated_device(get_name());

        //
        // Record attribute request in black box
        //

        blackbox_ptr->insert_corba_attr(Attr_State);

        always_executed_hook();

        //
        // Return data to caller. If the state_cmd throw an exception, catch it and do
        // not re-throw it because we are in a CORBA attribute implementation

        tmp = dev_state();
    }
    catch(Tango::DevFailed &e)
    {
        if(last_associated_device.size() > 0)
        {
            // set back the device attribution for the thread
            (Tango::Util::instance())->get_sub_dev_diag().set_associated_device(last_associated_device);
        }

        CORBA::IMP_LIMIT lim;
        if(strcmp(e.errors[0].reason, API_CommandTimedOut) == 0)
        {
            lim.minor(TG_IMP_MINOR_TO);
        }
        else
        {
            lim.minor(TG_IMP_MINOR_DEVFAILED);
        }
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::state (attribute) throwing IMP_LIMIT" << std::endl;
        throw lim;
    }
    catch(...)
    {
        if(last_associated_device.size() > 0)
        {
            // set back the device attribution for the thread
            (Tango::Util::instance())->get_sub_dev_diag().set_associated_device(last_associated_device);
        }

        CORBA::IMP_LIMIT lim;
        lim.minor(TG_IMP_MINOR_NON_DEVFAILED);
        TANGO_LOG_DEBUG << "Leaving DeviceImpl::state (attribute) throwing IMP_LIMIT" << std::endl;
        throw lim;
    }

    if(last_associated_device.size() > 0)
    {
        // set back the device attribution for the thread
        (Tango::Util::instance())->get_sub_dev_diag().set_associated_device(last_associated_device);
    }

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::state (attribute)" << std::endl;
    return tmp;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::status
//
// description :    Method called when a client request the description
//            status
//            This method is called for a IDL attribute which can
//            not throw exception to client ==> There is no point
//            to check if the allocation done by the string_dup
//            function failed.
//
//--------------------------------------------------------------------------

char *DeviceImpl::status()
{
    char *tmp;
    std::string last_associated_device;

    try
    {
        AutoTangoMonitor sync(this);

        TANGO_LOG_DEBUG << "DeviceImpl::status (attribute) arrived" << std::endl;

        //
        //  Write the device name into the per thread data for
        //  sub device diagnostics.
        //  Keep the old name, to put it back at the end!
        //  During device access inside the same server,
        //  the thread stays the same!
        //

        SubDevDiag &sub = (Tango::Util::instance())->get_sub_dev_diag();
        last_associated_device = sub.get_associated_device();
        sub.set_associated_device(get_name());

        //
        // Record attribute request in black box
        //

        blackbox_ptr->insert_corba_attr(Attr_Status);

        //
        // Return data to caller. If the status_cmd method throw exception, catch it
        // and forget it because we are in a CORBA attribute implementation
        //

        always_executed_hook();
        tmp = CORBA::string_dup(dev_status());
    }
    catch(Tango::DevFailed &e)
    {
        if(last_associated_device.size() > 0)
        {
            // set back the device attribution for the thread
            (Tango::Util::instance())->get_sub_dev_diag().set_associated_device(last_associated_device);
        }

        if(strcmp(e.errors[0].reason, API_CommandTimedOut) == 0)
        {
            tmp = CORBA::string_dup("Not able to acquire device monitor");
        }
        else
        {
            tmp = CORBA::string_dup("Got exception    when trying to build device status");
        }
    }
    catch(...)
    {
        if(last_associated_device.size() > 0)
        {
            // set back the device attribution for the thread
            (Tango::Util::instance())->get_sub_dev_diag().set_associated_device(last_associated_device);
        }

        CORBA::IMP_LIMIT lim;
        lim.minor(TG_IMP_MINOR_NON_DEVFAILED);
        throw lim;
    }

    if(last_associated_device.size() > 0)
    {
        // set back the device attribution for the thread
        (Tango::Util::instance())->get_sub_dev_diag().set_associated_device(last_associated_device);
    }

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::status (attribute)" << std::endl;
    return tmp;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::black_box
//
// description :    CORBA operation to read n element(s) of the black-box.
//            This method returns black box element as strings
//
// argument: in :    - n : Number of elt to read
//
//--------------------------------------------------------------------------

Tango::DevVarStringArray *DeviceImpl::black_box(CORBA::Long n)
{
    TANGO_LOG_DEBUG << "DeviceImpl::black_box arrived" << std::endl;

    Tango::DevVarStringArray *ret = blackbox_ptr->read((long) n);

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_BlackBox);

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::black_box" << std::endl;
    return ret;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::command_list_query
//
// description :    CORBA operation to read the device command list.
//            This method returns command info in a sequence of
//            DevCmdInfo
//
//--------------------------------------------------------------------------

Tango::DevCmdInfoList *DeviceImpl::command_list_query()
{
    TANGO_LOG_DEBUG << "DeviceImpl::command_list_query arrived" << std::endl;

    //
    // Retrieve number of command and allocate memory to send back info
    //

    long nb_cmd = device_class->get_command_list().size();
    TANGO_LOG_DEBUG << nb_cmd << " command(s) for device" << std::endl;
    Tango::DevCmdInfoList *back = nullptr;

    try
    {
        back = new Tango::DevCmdInfoList(nb_cmd);
        back->length(nb_cmd);

        //
        // Populate the vector
        //

        for(long i = 0; i < nb_cmd; i++)
        {
            Tango::DevCmdInfo tmp;
            tmp.cmd_name = CORBA::string_dup(((device_class->get_command_list())[i]->get_name()).c_str());
            tmp.cmd_tag = (long) ((device_class->get_command_list())[i]->get_disp_level());
            tmp.in_type = (long) ((device_class->get_command_list())[i]->get_in_type());
            tmp.out_type = (long) ((device_class->get_command_list())[i]->get_out_type());
            std::string &str_in = (device_class->get_command_list())[i]->get_in_type_desc();
            if(str_in.size() != 0)
            {
                tmp.in_type_desc = CORBA::string_dup(str_in.c_str());
            }
            else
            {
                tmp.in_type_desc = CORBA::string_dup(NotSet);
            }
            std::string &str_out = (device_class->get_command_list())[i]->get_out_type_desc();
            if(str_out.size() != 0)
            {
                tmp.out_type_desc = CORBA::string_dup(str_out.c_str());
            }
            else
            {
                tmp.out_type_desc = CORBA::string_dup(NotSet);
            }

            (*back)[i] = tmp;
        }
    }
    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Command_list);

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::command_list_query" << std::endl;
    return back;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::command_query
//
// description :    CORBA operation to read a device command info.
//            This method returns command info for a specific
//            command.
//
//
//--------------------------------------------------------------------------

Tango::DevCmdInfo *DeviceImpl::command_query(const char *command)
{
    TANGO_LOG_DEBUG << "DeviceImpl::command_query arrived" << std::endl;

    Tango::DevCmdInfo *back = nullptr;
    std::string cmd(command);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    //
    // Allocate memory for the stucture sent back to caller. The ORB will free it
    //

    try
    {
        back = new Tango::DevCmdInfo();
    }
    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    //
    // Try to retrieve the command in the command list
    //

    long i;
    long nb_cmd = device_class->get_command_list().size();
    for(i = 0; i < nb_cmd; i++)
    {
        if(device_class->get_command_list()[i]->get_lower_name() == cmd)
        {
            back->cmd_name = CORBA::string_dup(((device_class->get_command_list())[i]->get_name()).c_str());
            back->cmd_tag = (long) ((device_class->get_command_list())[i]->get_disp_level());
            back->in_type = (long) ((device_class->get_command_list())[i]->get_in_type());
            back->out_type = (long) ((device_class->get_command_list())[i]->get_out_type());
            std::string &str_in = (device_class->get_command_list())[i]->get_in_type_desc();
            if(str_in.size() != 0)
            {
                back->in_type_desc = CORBA::string_dup(str_in.c_str());
            }
            else
            {
                back->in_type_desc = CORBA::string_dup(NotSet);
            }
            std::string &str_out = (device_class->get_command_list())[i]->get_out_type_desc();
            if(str_out.size() != 0)
            {
                back->out_type_desc = CORBA::string_dup(str_out.c_str());
            }
            else
            {
                back->out_type_desc = CORBA::string_dup(NotSet);
            }
            break;
        }
    }

    if(i == nb_cmd)
    {
        delete back;
        TANGO_LOG_DEBUG << "DeviceImpl::command_query(): command " << command << " not found" << std::endl;

        //
        // throw an exception to client
        //

        TangoSys_OMemStream o;

        o << "Command " << command << " not found" << std::ends;
        TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());
    }

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Command);

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::command_query" << std::endl;
    return back;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::info
//
// description :    CORBA operation to get device info
//
//--------------------------------------------------------------------------

Tango::DevInfo *DeviceImpl::info()
{
    TANGO_LOG_DEBUG << "DeviceImpl::info arrived" << std::endl;

    Tango::DevInfo *back = nullptr;

    //
    // Allocate memory for the stucture sent back to caller. The ORB will free it
    //

    try
    {
        back = new Tango::DevInfo();
    }
    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    //
    // Retrieve server host
    //

    Tango::Util *tango_ptr = Tango::Util::instance();
    back->server_host = CORBA::string_dup(tango_ptr->get_host_name().c_str());

    //
    // Fill-in remaining structure fields
    //

    back->dev_class = CORBA::string_dup(device_class->get_name().c_str());
    back->server_id = CORBA::string_dup(tango_ptr->get_ds_name().c_str());
    back->server_version = DevVersion;

    //
    // Build the complete info sent in the doc_url string
    //

    std::string doc_url("Doc URL = ");
    doc_url = doc_url + device_class->get_doc_url();

    //
    // Add TAG if it exist
    //

    std::string &svn_tag = device_class->get_svn_tag();
    if(svn_tag.size() != 0)
    {
        doc_url = doc_url + "\nSVN Tag = ";
        doc_url = doc_url + svn_tag;
    }
    else
    {
        std::string &cvs_tag = device_class->get_cvs_tag();
        if(cvs_tag.size() != 0)
        {
            doc_url = doc_url + "\nCVS Tag = ";
            doc_url = doc_url + cvs_tag;
        }
    }

    //
    // Add SCM location if defined
    //

    std::string &svn_location = device_class->get_svn_location();
    if(svn_location.size() != 0)
    {
        doc_url = doc_url + "\nSVN Location = ";
        doc_url = doc_url + svn_location;
    }
    else
    {
        std::string &cvs_location = device_class->get_cvs_location();
        if(cvs_location.size() != 0)
        {
            doc_url = doc_url + "\nCVS Location = ";
            doc_url = doc_url + cvs_location;
        }
    }

    back->doc_url = CORBA::string_dup(doc_url.c_str());

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Info);

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::info" << std::endl;
    return back;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::ping
//
// description :    CORBA operation to ping if a device to see it is alive
//
//--------------------------------------------------------------------------

void DeviceImpl::ping()
{
    TANGO_LOG_DEBUG << "DeviceImpl::ping arrived" << std::endl;

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Ping);

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::ping" << std::endl;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::get_attribute_config
//
// description :    CORBA operation to get attribute configuration.
//
// argument: in :    - names: name of attribute(s)
//
// This method returns a pointer to a AttributeConfigList with one AttributeConfig
// structure for each atribute
//
//--------------------------------------------------------------------------

Tango::AttributeConfigList *DeviceImpl::get_attribute_config(const Tango::DevVarStringArray &names)
{
    TANGO_LOG_DEBUG << "DeviceImpl::get_attribute_config arrived" << std::endl;

    TangoMonitor &mon = get_att_conf_monitor();
    AutoTangoMonitor sync(&mon);

    long nb_attr = names.length();
    Tango::AttributeConfigList *back = nullptr;
    bool all_attr = false;

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Get_Attr_Config);

    //
    // Get attribute number and device version
    //

    long nb_dev_attr = dev_attr->get_attr_nb();
    long vers = get_dev_idl_version();

    //
    // Check if the caller want to get config for all attribute
    // If the device implements IDL 3 (State and status as attributes)
    // and the client is an old one (not able to read state/status as
    // attribute), decrement attribute number
    //

    std::string in_name(names[0]);
    if(nb_attr == 1)
    {
        if(in_name == AllAttr)
        {
            all_attr = true;
            if(vers < 3)
            {
                nb_attr = nb_dev_attr;
            }
            else
            {
                nb_attr = nb_dev_attr - 2;
            }
        }
        else if(in_name == AllAttr_3)
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
        back = new Tango::AttributeConfigList(nb_attr);
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

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::get_attribute_config" << std::endl;

    return back;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::set_attribute_config
//
// description :    CORBA operation to set attribute configuration locally
//            and in the Tango database
//
// argument: in :    - new_conf: The new attribute(s) configuration. One
//                    AttributeConfig structure is needed for each
//                    attribute to update
//
//--------------------------------------------------------------------------

void DeviceImpl::set_attribute_config(const Tango::AttributeConfigList &new_conf)
{
    AutoTangoMonitor sync(this, true);

    TANGO_LOG_DEBUG << "DeviceImpl::set_attribute_config arrived" << std::endl;

    //
    // The attribute conf. is protected by two monitors. One protects access between
    // get and set attribute conf. The second one protects access between set and
    // usage. This is the classical device monitor
    //

    TangoMonitor &mon1 = get_att_conf_monitor();
    AutoTangoMonitor sync1(&mon1);

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Set_Attr_Config);

    //
    // Check if device is locked
    //

    check_lock("set_attribute_config");

    //
    // Return exception if the device does not have any attribute
    //

    long nb_dev_attr = dev_attr->get_attr_nb();
    if(nb_dev_attr == 0)
    {
        TANGO_THROW_EXCEPTION(API_AttrNotFound, "The device does not have any attribute");
    }

    //
    // Update attribute config first in database, then locally
    // Finally send attr conf. event
    //

    long nb_attr = new_conf.length();
    long i;

    try
    {
        for(i = 0; i < nb_attr; i++)
        {
            std::string tmp_name(new_conf[i].name);
            std::transform(tmp_name.begin(), tmp_name.end(), tmp_name.begin(), ::tolower);
            if((tmp_name == "state") || (tmp_name == "status"))
            {
                TANGO_THROW_EXCEPTION(API_AttrNotFound, "Cannot set config for attribute state or status");
            }

            Attribute &attr = dev_attr->get_attr_by_name(new_conf[i].name);
            bool old_alarm = attr.is_alarmed().any();
            std::vector<Attribute::AttPropDb> v_db;
            attr.set_properties(new_conf[i], device_name, false, v_db);
            if(Tango::Util::instance()->use_db())
            {
                attr.upd_database(v_db);
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

            push_att_conf_event(&attr);
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
            Attribute &att = dev_attr->get_attr_by_ind(j);
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
        e.errors[0].reason = CORBA::string_dup(s.c_str());

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

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::set_attribute_config" << std::endl;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::read_attributes
//
// description :    CORBA operation to read attribute(s) value.
//
// argument: in :    - names: name of attribute(s) to be read
//
// This method returns a pointer to a AttributeConfigList with one AttributeValue
// structure for each atribute. This structure contains the attribute value, the
// value quality and the date.
//
//--------------------------------------------------------------------------

Tango::AttributeValueList *DeviceImpl::read_attributes(const Tango::DevVarStringArray &names)
{
    AutoTangoMonitor sync(this, true);

    Tango::AttributeValueList *back = nullptr;

    TANGO_LOG_DEBUG << "DeviceImpl::read_attributes arrived" << std::endl;

    //
    //  Write the device name into the per thread data for
    //  sub device diagnostics.
    //  Keep the old name, to put it back at the end!
    //  During device access inside the same server,
    //  the thread stays the same!
    //
    SubDevDiag &sub = (Tango::Util::instance())->get_sub_dev_diag();
    std::string last_associated_device = sub.get_associated_device();
    sub.set_associated_device(get_name());

    // Catch all execeptions to set back the associated device after
    // execution
    try
    {
        //
        // Record operation request in black box
        //

        if(store_in_bb)
        {
            blackbox_ptr->insert_attr(names);
        }
        store_in_bb = true;

        //
        // Return exception if the device does not have any attribute
        // For device implementing IDL 3, substract 2 to the attributes
        // number for state and status which could be read only by
        // a "new" client
        //

        long vers = get_dev_idl_version();
        long nb_dev_attr = dev_attr->get_attr_nb();

        if(nb_dev_attr == 0)
        {
            TANGO_THROW_EXCEPTION(API_AttrNotFound, "The device does not have any attribute");
        }
        if(vers >= 3)
        {
            nb_dev_attr = nb_dev_attr - 2;
        }

        //
        // Build a sequence with the names of the attribute to be read.
        // This is necessary in case of the "AllAttr" shortcut is used
        // If all attributes are wanted, build this list
        //

        long i;
        Tango::DevVarStringArray real_names(nb_dev_attr);
        long nb_names = names.length();
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

        //
        // Retrieve index of wanted attributes in the device attribute list and clear
        // their value set flag
        //
        // In IDL release 3, possibility to write spectrum and
        // images attributes have been added. This implies some
        // changes in the struture returned for a read_attributes
        // Throw exception if users want to use these new features
        // through an old interface
        //
        //

        nb_names = real_names.length();
        std::vector<long> wanted_attr;
        std::vector<long> wanted_w_attr;

        for(i = 0; i < nb_names; i++)
        {
            long j = dev_attr->get_attr_ind_by_name(real_names[i]);
            if((dev_attr->get_attr_by_ind(j).get_writable() == Tango::READ_WRITE) ||
               (dev_attr->get_attr_by_ind(j).get_writable() == Tango::READ_WITH_WRITE))
            {
                wanted_w_attr.push_back(j);
                wanted_attr.push_back(j);
                Attribute &att = dev_attr->get_attr_by_ind(wanted_attr.back());
                Tango::AttrDataFormat format_type = att.get_data_format();
                if((format_type == Tango::SPECTRUM) || (format_type == Tango::IMAGE))
                {
                    TangoSys_OMemStream o;
                    o << "Client too old to get data for attribute " << real_names[i].in();
                    o << ".\nPlease, use a client linked with Tango V5";
                    o << " and a device inheriting from Device_3Impl" << std::ends;
                    TANGO_THROW_EXCEPTION(API_NotSupportedFeature, o.str());
                }
                att.reset_value();
                att.get_when().tv_sec = 0;
            }
            else
            {
                if(dev_attr->get_attr_by_ind(j).get_writable() == Tango::WRITE)
                {
                    wanted_w_attr.push_back(j);
                    Attribute &att = dev_attr->get_attr_by_ind(wanted_w_attr.back());
                    Tango::AttrDataFormat format_type = att.get_data_format();
                    if((format_type == Tango::SPECTRUM) || (format_type == Tango::IMAGE))
                    {
                        TangoSys_OMemStream o;
                        o << "Client too old to get data for attribute " << real_names[i].in();
                        o << ".\nPlease, use a client linked with Tango V5";
                        o << " and a device inheriting from Device_3Impl" << std::ends;
                        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, o.str());
                    }
                }
                else
                {
                    wanted_attr.push_back(j);
                    Attribute &att = dev_attr->get_attr_by_ind(wanted_attr.back());
                    att.reset_value();
                    att.get_when().tv_sec = 0;
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
        // Read the hardware for readable attribute
        //

        if(nb_wanted_attr != 0)
        {
            read_attr_hardware(wanted_attr);
        }

        //
        // Set attr value (for readable attribute)
        //

        std::vector<Tango::Attr *> &attr_vect = device_class->get_class_attr()->get_attr_list();

        for(i = 0; i < nb_wanted_attr; i++)
        {
            if(vers < 3)
            {
                read_attr(dev_attr->get_attr_by_ind(wanted_attr[i]));
            }
            else
            {
                Attribute &att = dev_attr->get_attr_by_ind(wanted_attr[i]);
                long idx = att.get_attr_idx();
                if(idx == -1)
                {
                    TangoSys_OMemStream o;

                    o << "It is not possible to read state/status as attributes with your\n";
                    o << "Tango software release. Please, re-link with Tango V5." << std::ends;

                    TANGO_THROW_EXCEPTION(API_NotSupportedFeature, o.str());
                }

                if(!attr_vect[idx]->is_allowed(this, Tango::READ_REQ))
                {
                    TangoSys_OMemStream o;

                    o << "It is currently not allowed to read attribute ";
                    o << att.get_name() << std::ends;

                    TANGO_THROW_EXCEPTION(API_AttrNotAllowed, o.str());
                }
                attr_vect[idx]->read(this, att);
            }
        }

        //
        // Set attr value for writable attribute
        //

        for(i = 0; i < nb_wanted_w_attr; i++)
        {
            Tango::AttrWriteType w_type = dev_attr->get_attr_by_ind(wanted_w_attr[i]).get_writable();
            if((w_type == Tango::READ_WITH_WRITE) || (w_type == Tango::WRITE))
            {
                dev_attr->get_attr_by_ind(wanted_w_attr[i]).set_rvalue();
            }
        }

        //
        // Allocate memory for the AttributeValue structures
        //

        try
        {
            back = new Tango::AttributeValueList(nb_names);
            back->length(nb_names);
        }
        catch(std::bad_alloc &)
        {
            TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
        }

        //
        // Build the sequence returned to caller for readable attributes and check
        // that all the wanted attributes set value has been updated
        //

        for(i = 0; i < nb_names; i++)
        {
            Attribute &att = dev_attr->get_attr_by_name(real_names[i]);
            Tango::AttrQuality qual = att.get_quality();
            if(qual != Tango::ATTR_INVALID)
            {
                if(!att.value_is_set())
                {
                    TangoSys_OMemStream o;
                    delete back;

                    try
                    {
                        std::string att_name(real_names[i]);
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

                    TANGO_THROW_EXCEPTION(API_AttrValueNotSet, o.str());
                }
                else
                {
                    Tango::AttrWriteType w_type = att.get_writable();
                    if((w_type == Tango::READ) || (w_type == Tango::READ_WRITE) || (w_type == Tango::READ_WITH_WRITE))
                    {
                        if((w_type == Tango::READ_WRITE) || (w_type == Tango::READ_WITH_WRITE))
                        {
                            dev_attr->add_write_value(att);
                        }

                        if((att.is_alarmed().any()) && (qual != Tango::ATTR_INVALID))
                        {
                            att.check_alarm();
                        }
                    }

                    CORBA::Any &a = (*back)[i].value;

                    att.extract_value(a);
                }
            }

            if(att.get_when().tv_sec == 0)
            {
                att.set_time();
            }

            (*back)[i].time = att.get_when();
            (*back)[i].quality = att.get_quality();
            (*back)[i].name = CORBA::string_dup(att.get_name().c_str());
            (*back)[i].dim_x = att.get_x();
            (*back)[i].dim_y = att.get_y();
        }
    }
    catch(...)
    {
        // set back the device attribution for the thread
        // and rethrow the exception.
        sub.set_associated_device(last_associated_device);
        throw;
    }

    // set back the device attribution for the thread
    sub.set_associated_device(last_associated_device);

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::read_attributes" << std::endl;
    return back;
}

//+-------------------------------------------------------------------------
//
// method :        DeviceImpl::write_attributes
//
// description :    CORBA operation to write attribute(s) value
//
// argument: in :    - values: The new attribute(s) value to be set.
//
//--------------------------------------------------------------------------

void DeviceImpl::write_attributes(const Tango::AttributeValueList &values)
{
    AutoTangoMonitor sync(this, true);

    TANGO_LOG_DEBUG << "DeviceImpl::write_attributes arrived" << std::endl;

    //
    //  Write the device name into the per thread data for
    //  sub device diagnostics.
    //  Keep the old name, to put it back at the end!
    //  During device access inside the same server,
    //  the thread stays the same!
    //
    SubDevDiag &sub = (Tango::Util::instance())->get_sub_dev_diag();
    std::string last_associated_device = sub.get_associated_device();
    sub.set_associated_device(get_name());

    // Catch all execeptions to set back the associated device after
    // execution
    try
    {
        //
        // Record operation request in black box
        //

        blackbox_ptr->insert_attr(values);

        //
        // Check if the device is locked
        //

        check_lock("write_attributes");

        //
        // Return exception if the device does not have any attribute
        //

        long nb_dev_attr = dev_attr->get_attr_nb();
        if(nb_dev_attr == 0)
        {
            TANGO_THROW_EXCEPTION(API_AttrNotFound, "The device does not have any attribute");
        }

        //
        // Retrieve index of wanted attributes in the device attribute list
        //

        long nb_updated_attr = values.length();
        std::vector<long> updated_attr;

        long i;
        for(i = 0; i < nb_updated_attr; i++)
        {
            updated_attr.push_back(dev_attr->get_attr_ind_by_name(values[i].name));
        }

        //
        // Check that these attributes are writable
        //

        for(i = 0; i < nb_updated_attr; i++)
        {
            if((dev_attr->get_attr_by_ind(updated_attr[i]).get_writable() == Tango::READ) ||
               (dev_attr->get_attr_by_ind(updated_attr[i]).get_writable() == Tango::READ_WITH_WRITE))
            {
                TangoSys_OMemStream o;

                o << "Attribute ";
                o << dev_attr->get_attr_by_ind(updated_attr[i]).get_name();
                o << " is not writable" << std::ends;

                TANGO_THROW_EXCEPTION(API_AttrNotWritable, o.str());
            }
        }

        //
        // Call the always_executed_hook
        //

        always_executed_hook();

        //
        // Set attribute internal value
        //

        for(i = 0; i < nb_updated_attr; i++)
        {
            try
            {
                dev_attr->get_w_attr_by_ind(updated_attr[i])
                    .check_written_value(values[i].value, (unsigned long) 1, (unsigned long) 0);
            }
            catch(Tango::DevFailed &)
            {
                for(long j = 0; j < i; j++)
                {
                    dev_attr->get_w_attr_by_ind(updated_attr[j]).rollback();
                }

                throw;
            }
        }

        //
        // Write the hardware
        //

        long vers = get_dev_idl_version();
        if(vers < 3)
        {
            write_attr_hardware(updated_attr);
            for(i = 0; i < nb_updated_attr; i++)
            {
                WAttribute &att = dev_attr->get_w_attr_by_ind(updated_attr[i]);
                att.copy_data(values[i].value);
            }
        }
        else
        {
            std::vector<long> att_in_db;

            for(i = 0; i < nb_updated_attr; i++)
            {
                WAttribute &att = dev_attr->get_w_attr_by_ind(updated_attr[i]);
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
                att.copy_data(values[i].value);
                if(att.is_memorized())
                {
                    att_in_db.push_back(i);
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
                    static_cast<Device_3Impl *>(this)->write_attributes_in_db(att_in_db, updated_attr);
                }
                catch(Tango::DevFailed &e)
                {
                    TANGO_RETHROW_EXCEPTION(e, API_AttrNotAllowed, "Failed to store memorized attribute value in db");
                }
            }
        }
    }
    catch(...)
    {
        // set back the device attribution for the thread
        // and rethrow the exception.
        sub.set_associated_device(last_associated_device);
        throw;
    }

    // set back the device attribution for the thread
    sub.set_associated_device(last_associated_device);

    //
    // Return to caller.
    //

    TANGO_LOG_DEBUG << "Leaving DeviceImpl::write_attributes" << std::endl;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::add_attribute
//
// description :
//        Add attribute to the device attribute(s) list
//
// argument:
//        in :
//            - new_attr: The new attribute to be added.
//
//--------------------------------------------------------------------------------------------------------------------

void DeviceImpl::add_attribute(Tango::Attr *new_attr)
{
    //
    // Take the device monitor in order to protect the attribute list
    //

    AutoTangoMonitor sync(this, true);

    std::vector<Tango::Attr *> &attr_list = device_class->get_class_attr()->get_attr_list();
    long old_attr_nb = attr_list.size();

    //
    // Check that this attribute is not already defined for this device. If it is already there, immediately returns.
    // Trick : If you add an attribute to a device, this attribute will be inserted in the device class attribute list.
    // Therefore, all devices created after this attribute addition will also have this attribute.
    //

    std::string &attr_name = new_attr->get_name();
    bool already_there = true;
    bool throw_ex = false;
    try
    {
        Tango::Attribute &al_attr = dev_attr->get_attr_by_name(attr_name.c_str());
        if((al_attr.get_data_type() != new_attr->get_type()) || (al_attr.get_data_format() != new_attr->get_format()) ||
           (al_attr.get_writable() != new_attr->get_writable()))
        {
            throw_ex = true;
        }
    }
    catch(Tango::DevFailed &)
    {
        already_there = false;
    }

    //
    // Throw exception if the device already have an attribute with the same name but with a different definition
    //

    if(throw_ex)
    {
        TangoSys_OMemStream o;

        o << "Device " << get_name() << " -> Attribute " << attr_name
          << " already exists for your device but with other definition";
        o << "\n(data type, data format or data write type)" << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrNotFound, o.str());
    }

    if(already_there)
    {
        delete new_attr;
        return;
    }

    //
    // If device is IDL 5 or more and if enabled, and if there is some client(s) listening on the device interface
    // change event, get device interface.
    //

    ZmqEventSupplier *event_supplier_zmq = Util::instance()->get_zmq_event_supplier();
    bool ev_client = event_supplier_zmq->any_dev_intr_client(this);

    if(idl_version >= MIN_IDL_DEV_INTR && is_intr_change_ev_enable())
    {
        if(ev_client)
        {
            bool th_running;
            {
                omni_mutex_lock lo(devintr_mon);
                th_running = devintr_shared.th_running;
            }

            if(!th_running)
            {
                devintr_shared.interface.get_interface(this);
            }
        }
    }

    //
    // Add this attribute in the MultiClassAttribute attr_list vector if it does not already exist
    //

    bool need_free = false;
    long i;

    for(i = 0; i < old_attr_nb; i++)
    {
        if((attr_list[i]->get_name() == attr_name) && (attr_list[i]->get_cl_name() == new_attr->get_cl_name()))
        {
            need_free = true;
            break;
        }
    }

    if(i == old_attr_nb)
    {
        attr_list.push_back(new_attr);

        //
        // Get all the properties defined for this attribute at class level
        //

        device_class->get_class_attr()->init_class_attribute(device_class->get_name(), old_attr_nb);
    }
    else
    {
        //
        // An attribute with the same name is already defined within the class. Check if the data type, data format and
        // write type are the same
        //

        if((attr_list[i]->get_type() != new_attr->get_type()) ||
           (attr_list[i]->get_format() != new_attr->get_format()) ||
           (attr_list[i]->get_writable() != new_attr->get_writable()))
        {
            TangoSys_OMemStream o;

            o << "Device " << get_name() << " -> Attribute " << attr_name
              << " already exists for your device class but with other definition";
            o << "\n(data type, data format or data write type)" << std::ends;

            TANGO_THROW_EXCEPTION(API_AttrNotFound, o.str());
        }
    }

    //
    // Add the attribute to the MultiAttribute object
    //

    if(new_attr->is_fwd())
    {
        dev_attr->add_fwd_attribute(device_name, device_class, i, new_attr);
    }
    else
    {
        dev_attr->add_attribute(device_name, device_class, i);
    }

    //
    // Eventually start or update device interface change event thread
    //

    push_dev_intr(ev_client);

    //
    // If attribute has to be polled (set by Pogo), start polling now
    //

    long per = new_attr->get_polling_period();
    if((!is_attribute_polled(attr_name)) && (per != 0))
    {
        poll_attribute(attr_name, per);
    }

    //
    // Free memory if needed
    //

    if(need_free)
    {
        delete new_attr;
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::remove_attribute
//
// description :
//        Remove attribute to the device attribute(s) list
//
// argument:
//        in :
//            - rem_attr: The attribute to be deleted.
//          - free_it : Free Attr object flag
//          - clean_db : Clean attribute related info in db
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceImpl::remove_attribute(Tango::Attr *rem_attr, bool free_it, bool clean_db)
{
    //
    // Take the device monitor in order to protect the attribute list
    //

    AutoTangoMonitor sync(this, true);

    //
    // Check that the class support this attribute
    //

    const std::string &attr_name = rem_attr->get_name();

    try
    {
        dev_attr->get_attr_by_name(attr_name.c_str());
    }
    catch(Tango::DevFailed &)
    {
        TangoSys_OMemStream o;

        o << "Attribute " << attr_name << " is not defined as attribute for your device.";
        o << "\nCan't remove it" << std::ends;

        TANGO_THROW_EXCEPTION(API_AttrNotFound, o.str());
    }

    //
    // If device is IDL 5 or more and if enabled, and if there is some client(s) listening on the device interface
    // change event, get device interface.
    //

    ZmqEventSupplier *event_supplier_zmq = Util::instance()->get_zmq_event_supplier();
    bool ev_client = event_supplier_zmq->any_dev_intr_client(this);

    if(idl_version >= MIN_IDL_DEV_INTR && is_intr_change_ev_enable())
    {
        if(ev_client)
        {
            bool th_running;
            {
                omni_mutex_lock lo(devintr_mon);
                th_running = devintr_shared.th_running;
            }

            if(!th_running)
            {
                devintr_shared.interface.get_interface(this);
            }
        }
    }

    //
    // stop any configured polling for this attribute first!
    //

    std::vector<std::string> &poll_attr = get_polled_attr();
    std::vector<std::string>::iterator ite_attr;

    std::string attr_name_low(attr_name);
    std::transform(attr_name_low.begin(), attr_name_low.end(), attr_name_low.begin(), ::tolower);

    //
    // Try to find the attribute in the list of polled attributes
    //

    Tango::Util *tg = Tango::Util::instance();
    ite_attr = find(poll_attr.begin(), poll_attr.end(), attr_name_low);
    if(ite_attr != poll_attr.end())
    {
        // stop the polling and clean-up the database

        DServer *adm_dev = tg->get_dserver_device();

        DevVarStringArray send;
        send.length(3);

        send[0] = CORBA::string_dup(device_name.c_str());
        send[1] = CORBA::string_dup("attribute");
        send[2] = CORBA::string_dup(attr_name.c_str());

        if(tg->is_svr_shutting_down())
        {
            //
            // There is no need to stop the polling because we are in the server shutdown sequence and the polling is
            // already stopped.
            //

            if(clean_db && Tango::Util::instance()->use_db())
            {
                //
                // Memorize the fact that the dynamic polling properties has to be removed from db.
                // The classical attribute properties as well
                //

                tg->get_polled_dyn_attr_names().push_back(attr_name_low);
                if(tg->get_full_polled_att_list().size() == 0)
                {
                    tg->get_full_polled_att_list() = poll_attr;
                    tg->get_dyn_att_dev_name() = device_name;
                }
            }
        }
        else
        {
            if(!tg->is_device_restarting(get_name()))
            {
                adm_dev->rem_obj_polling(&send, clean_db);
            }
        }
    }

    //
    // Now remove all configured attribute properties from the database. Do it in one go if the Db server support this
    //

    if(clean_db)
    {
        if((!tg->is_svr_shutting_down()) || (tg->get_database()->get_server_release() < 400))
        {
            Tango::Attribute &att_obj = dev_attr->get_attr_by_name(attr_name.c_str());
            att_obj.remove_configuration();
        }
        else
        {
            tg->get_all_dyn_attr_names().push_back(attr_name);
            if(tg->get_dyn_att_dev_name().size() == 0)
            {
                tg->get_dyn_att_dev_name() = device_name;
            }
        }
    }

    //
    // Remove attribute in MultiClassAttribute in case there is only one device in the class or it is the last device
    // in this class with this attribute
    //

    bool update_idx = false;
    unsigned long nb_dev = device_class->get_device_list().size();

    if(nb_dev <= 1)
    {
        device_class->get_class_attr()->remove_attr(attr_name, rem_attr->get_cl_name());
        update_idx = true;
    }
    else
    {
        std::vector<Tango::DeviceImpl *> dev_list = device_class->get_device_list();
        unsigned long nb_except = 0;
        for(unsigned long i = 0; i < nb_dev; i++)
        {
            try
            {
                Attribute &att = dev_list[i]->get_device_attr()->get_attr_by_name(attr_name.c_str());
                std::vector<Tango::Attr *> &attr_list = device_class->get_class_attr()->get_attr_list();
                if(attr_list[att.get_attr_idx()]->get_cl_name() != rem_attr->get_cl_name())
                {
                    nb_except++;
                }
            }
            catch(Tango::DevFailed &)
            {
                nb_except++;
            }
        }
        if(nb_except == (nb_dev - 1))
        {
            device_class->get_class_attr()->remove_attr(attr_name, rem_attr->get_cl_name());
            update_idx = true;
        }
    }

    //
    // Now, remove the attribute from the MultiAttribute object
    //

    dev_attr->remove_attribute(attr_name, update_idx);

    //
    // Delete Attr object if wanted
    //

    if((free_it) && (update_idx))
    {
        delete rem_attr;
    }

    //
    // Eventually start or update device interface change event thread
    //

    push_dev_intr(ev_client);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::remove_attribute
//
// description :
//        Remove attribute to the device attribute(s) list
//
// argument:
//        in :
//            - rem_attr: The name of the attribute to be deleted.
//          - free_it : Free Attr object flag
//          - clean_db : Clean attribute related info in db
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::remove_attribute(const std::string &rem_attr_name, bool free_it, bool clean_db)
{
    try
    {
        Attr &att = device_class->get_class_attr()->get_attr(rem_attr_name);
        remove_attribute(&att, free_it, clean_db);
    }
    catch(Tango::DevFailed &e)
    {
        TangoSys_OMemStream o;

        o << "Attribute " << rem_attr_name << " is not defined as attribute for your device.";
        o << "\nCan't remove it" << std::ends;

        TANGO_RETHROW_EXCEPTION(e, API_AttrNotFound, o.str());
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::add_command
//
// description :
//        Add command to the device command(s) list
//
// argument:
//        in :
//            - new_cmd: The new command to be added.
//            - device_level : flag set to true if the command must be added at the device level (instead of class
//            level)
//
//--------------------------------------------------------------------------------------------------------------------

void DeviceImpl::add_command(Tango::Command *new_cmd, bool device_level)
{
    //
    // Take the device monitor in order to protect the command list
    //

    AutoTangoMonitor sync(this, true);

    //
    // Check that this command is not already defined for this device. If it is already there, immediately returns.
    //

    std::string &cmd_name = new_cmd->get_name();
    bool already_there = true;
    bool throw_ex = false;
    try
    {
        Tango::Command &al_cmd = device_class->get_cmd_by_name(cmd_name);
        if((al_cmd.get_in_type() != new_cmd->get_in_type()) || (al_cmd.get_out_type() != new_cmd->get_out_type()))
        {
            throw_ex = true;
        }
    }
    catch(Tango::DevFailed &)
    {
        already_there = false;
    }

    if(!already_there)
    {
        already_there = true;
        try
        {
            Tango::Command &al_cmd_dev = get_local_cmd_by_name(cmd_name);
            if((al_cmd_dev.get_in_type() != new_cmd->get_in_type()) ||
               (al_cmd_dev.get_out_type() != new_cmd->get_out_type()))
            {
                throw_ex = true;
            }
        }
        catch(Tango::DevFailed &)
        {
            already_there = false;
        }
    }

    //
    // Throw exception if the device already have a command with the same name but with a different definition
    //

    if(throw_ex)
    {
        TangoSys_OMemStream o;

        o << "Device " << get_name() << " -> Command " << cmd_name
          << " already exists for your device but with other definition";
        o << "\n(command input data type or command output data type)" << std::ends;

        TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());
    }

    if(already_there)
    {
        delete new_cmd;
        return;
    }

    //
    // If device is IDL 5 or more and if enabled, and if there is some client(s) listening on the device interface
    // change event, get device interface.
    //

    ZmqEventSupplier *event_supplier_zmq = Util::instance()->get_zmq_event_supplier();
    bool ev_client = event_supplier_zmq->any_dev_intr_client(this);

    if(idl_version >= MIN_IDL_DEV_INTR && is_intr_change_ev_enable())
    {
        if(ev_client)
        {
            bool th_running;
            {
                omni_mutex_lock lo(devintr_mon);
                th_running = devintr_shared.th_running;
            }

            if(!th_running)
            {
                devintr_shared.interface.get_interface(this);
            }
        }
    }

    //
    // Add this command to the command list
    //

    if(!device_level)
    {
        std::vector<Tango::Command *> &cmd_list = device_class->get_command_list();
        cmd_list.push_back(new_cmd);
    }
    else
    {
        std::vector<Tango::Command *> &dev_cmd_list = get_local_command_list();
        dev_cmd_list.push_back(new_cmd);
    }

    //
    // Eventually start or update device interface change event thread
    //

    push_dev_intr(ev_client);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::remove_command
//
// description :
//        Remove command to the device command(s) list
//
// argument:
//        in :
//            - rem_cmd: The command to be deleted.
//          - free_it : Free Command object flag
//          - clean_db : Clean command related info in db
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceImpl::remove_command(Tango::Command *rem_cmd, bool free_it, bool clean_db)
{
    //
    // Take the device monitor in order to protect the command list
    //

    AutoTangoMonitor sync(this, true);

    //
    // Check that the class or the device support this command
    //

    std::string &cmd_name = rem_cmd->get_name();
    bool device_cmd = false;

    try
    {
        device_class->get_cmd_by_name(cmd_name);
    }
    catch(Tango::DevFailed &)
    {
        try
        {
            get_local_cmd_by_name(cmd_name);
            device_cmd = true;
        }
        catch(Tango::DevFailed &)
        {
            TangoSys_OMemStream o;

            o << "Command " << cmd_name << " is not defined as command for your device.";
            o << "\nCan't remove it" << std::ends;

            TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());
        }
    }

    //
    // If device is IDL 5 or more and if enabled, and if there is some client(s) listening on the device interface
    // change event, get device interface.
    //

    ZmqEventSupplier *event_supplier_zmq = Util::instance()->get_zmq_event_supplier();
    bool ev_client = event_supplier_zmq->any_dev_intr_client(this);

    if(idl_version >= MIN_IDL_DEV_INTR && is_intr_change_ev_enable())
    {
        if(ev_client)
        {
            bool th_running;
            {
                omni_mutex_lock lo(devintr_mon);
                th_running = devintr_shared.th_running;
            }

            if(!th_running)
            {
                devintr_shared.interface.get_interface(this);
            }
        }
    }

    //
    // stop any configured polling for this command first!
    //

    std::vector<std::string> &poll_cmd = get_polled_cmd();
    std::vector<std::string>::iterator ite_cmd;

    std::string cmd_name_low(cmd_name);
    std::transform(cmd_name_low.begin(), cmd_name_low.end(), cmd_name_low.begin(), ::tolower);

    //
    // Try to find the command in the list of polled commands
    //

    Tango::Util *tg = Tango::Util::instance();
    ite_cmd = find(poll_cmd.begin(), poll_cmd.end(), cmd_name_low);
    if(ite_cmd != poll_cmd.end())
    {
        // stop the polling and clean-up the database

        DServer *adm_dev = tg->get_dserver_device();

        DevVarStringArray send;
        send.length(3);

        send[0] = CORBA::string_dup(device_name.c_str());
        send[1] = CORBA::string_dup("command");
        send[2] = CORBA::string_dup(cmd_name.c_str());

        if(tg->is_svr_shutting_down())
        {
            //
            // There is no need to stop the polling because we are in the server shutdown sequence and the polling is
            // already stopped.
            //

            if(clean_db && Tango::Util::instance()->use_db())
            {
                //
                // Memorize the fact that the dynamic polling properties has to be removed from db.
                //

                tg->get_polled_dyn_cmd_names().push_back(cmd_name_low);
                if(tg->get_full_polled_cmd_list().size() == 0)
                {
                    tg->get_full_polled_cmd_list() = poll_cmd;
                    tg->get_dyn_cmd_dev_name() = device_name;
                }
            }
        }
        else
        {
            if(!tg->is_device_restarting(get_name()))
            {
                adm_dev->rem_obj_polling(&send, clean_db);
            }
        }
    }

    //
    // Now, remove the command from the command list
    //

    if(!device_cmd)
    {
        device_class->remove_command(cmd_name_low);
    }
    else
    {
        remove_local_command(cmd_name_low);
    }

    //
    // Delete Command object if wanted
    //

    if(free_it)
    {
        delete rem_cmd;
    }

    //
    // Eventually start or update device interface change event thread
    //

    push_dev_intr(ev_client);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::remove_command
//
// description :
//        Remove command to the device command(s) list
//
// argument:
//        in :
//            - rem_cmd_name: The name of the command to be deleted.
//          - free_it : Free Command object flag
//          - clean_db : Clean command related info in db
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::remove_command(const std::string &rem_cmd_name, bool free_it, bool clean_db)
{
    //
    // Search for command first at class level and then at device level (for dynamic cmd)
    //

    try
    {
        Command &cmd = device_class->get_cmd_by_name(rem_cmd_name);
        remove_command(&cmd, free_it, clean_db);
    }
    catch(Tango::DevFailed &)
    {
        try
        {
            Command &cmd = get_local_cmd_by_name(rem_cmd_name);
            remove_command(&cmd, free_it, clean_db);
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream o;

            o << "Command " << rem_cmd_name << " is not defined as a command for your device.";
            o << "\nCan't remove it" << std::ends;

            TANGO_RETHROW_EXCEPTION(e, API_CommandNotFound, o.str());
        }
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::poll_lists_2_v5
//
// description :
//        Started from Tango V5, state and status are polled as attributes. Previously, they were polled as commands.
//        If state or status are polled as commands, move them to the list of polled attributes
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::poll_lists_2_v5()
{
    bool db_update = false;

    std::vector<std::string> &poll_cmd = get_polled_cmd();
    std::vector<std::string> &poll_attr = get_polled_attr();

    std::vector<std::string>::iterator ite_state;
    std::vector<std::string>::iterator ite_status;

    //
    // Try to find state in list of polled command(s). If found, remove it from poll cmd and move it to poll attr
    //

    ite_state = find(poll_cmd.begin(), poll_cmd.end(), "state");
    if(ite_state != poll_cmd.end())
    {
        poll_attr.push_back(*ite_state);
        poll_attr.push_back(*(ite_state + 1));
        poll_cmd.erase(ite_state, ite_state + 2);
        db_update = true;
    }

    //
    // The same for status
    //

    ite_status = find(poll_cmd.begin(), poll_cmd.end(), "status");
    if(ite_status != poll_cmd.end())
    {
        poll_attr.push_back(*ite_status);
        poll_attr.push_back(*(ite_status + 1));
        poll_cmd.erase(ite_status, ite_status + 2);
        db_update = true;
    }

    //
    // Now update database if needed
    //

    if(db_update)
    {
        DbDatum p_cmd("polled_cmd");
        DbDatum p_attr("polled_attr");

        p_cmd << poll_cmd;
        p_attr << poll_attr;

        DbData db_data;
        db_data.push_back(p_cmd);
        db_data.push_back(p_attr);

        get_db_device()->put_property(db_data);
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::init_cmd_poll_ext_trig
//
// description :
//        Write the command name to the list of polled commands in the database. The polling period is set to 0 to
//        indicate that the polling buffer is filled externally from the device server code.
//
// args :
//        in :
//            - cmd_name : The command name
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceImpl::init_cmd_poll_ext_trig(const std::string &cmd_name)
{
    std::string cmd_lowercase(cmd_name);
    std::transform(cmd_lowercase.begin(), cmd_lowercase.end(), cmd_lowercase.begin(), ::tolower);

    //
    // never do the for the state or status commands, they are handled as attributes!
    //

    if(cmd_name == "state" || cmd_name == "status")
    {
        TangoSys_OMemStream o;

        o << "State and status are handled as attributes for the polling" << std::ends;
        TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());
    }

    //
    // check whether the command exists for the device and can be polled
    //

    check_command_exists(cmd_lowercase);

    //
    // check wether the database is used
    //

    Tango::Util *tg = Tango::Util::instance();
    if(tg->use_db())
    {
        std::vector<std::string> &poll_list = get_polled_cmd();
        Tango::DbData poll_data;
        bool found = false;

        poll_data.emplace_back("polled_cmd");

        if(!poll_list.empty())
        {
            //
            // search the attribute in the list of polled attributes
            //

            for(unsigned int i = 0; i < poll_list.size(); i = i + 2)
            {
                std::string name_lowercase(poll_list[i]);
                std::transform(name_lowercase.begin(), name_lowercase.end(), name_lowercase.begin(), ::tolower);

                if(name_lowercase == cmd_lowercase)
                {
                    poll_list[i + 1] = "0";
                    found = true;
                }
            }
        }

        if(!found)
        {
            poll_list.push_back(cmd_lowercase);
            poll_list.emplace_back("0");
        }

        poll_data[0] << poll_list;
        tg->get_database()->put_device_property(device_name, poll_data);
    }
}

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::init_cmd_poll_period
//
// description :
//        Checks the specified polling period for all commands of the device. If a polling period is specified for a
//        command the command name and the period are written to the list of polled commands in the database.
//        This happens only if the command is not yet in the list of polled commands.
//
//--------------------------------------------------------------------------------------------------------------------

void DeviceImpl::init_cmd_poll_period()
{
    //
    // check wether the database is used
    //

    Tango::Util *tg = Tango::Util::instance();
    if(tg->use_db())
    {
        std::vector<std::string> &poll_list = get_polled_cmd();
        Tango::DbData poll_data;

        poll_data.emplace_back("polled_cmd");

        //
        // get the command list
        //

        std::vector<Command *> &cmd_list = device_class->get_command_list();

        //
        // loop over the command list
        //

        unsigned long added_cmd = 0;
        unsigned long i;
        for(i = 0; i < cmd_list.size(); i++)
        {
            long poll_period;
            poll_period = cmd_list[i]->get_polling_period();

            //
            // check the validity of the polling period. must be longer than min polling period
            //

            if(poll_period < MIN_POLL_PERIOD)
            {
                continue;
            }

            //
            // never do the for the state or status commands, they are handled as attributes!
            //

            std::string cmd_name = cmd_list[i]->get_lower_name();
            if(cmd_name == "state" || cmd_name == "status")
            {
                continue;
            }

            //
            // Can only handle commands without input argument
            //

            if(cmd_list[i]->get_in_type() != Tango::DEV_VOID)
            {
                continue;
            }

            //
            // search the command in the list of polled commands
            //

            bool found = false;
            for(unsigned int i = 0; i < poll_list.size(); i = i + 2)
            {
                std::string name_lowercase(poll_list[i]);
                std::transform(name_lowercase.begin(), name_lowercase.end(), name_lowercase.begin(), ::tolower);

                if(name_lowercase == cmd_name)
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                std::string period_str;
                std::stringstream str;
                str << poll_period << std::ends;
                str >> period_str;

                poll_list.push_back(cmd_name);
                poll_list.push_back(period_str);
                added_cmd++;
            }
        }

        //
        // only write to the database when a polling need to be added
        //

        if(added_cmd > 0)
        {
            poll_data[0] << poll_list;
            tg->get_database()->put_device_property(device_name, poll_data);
        }
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::init_attr_poll_ext_trig
//
// description :
//        Write the attribute name to the list of polled attributes in the database. The polling period is set to 0
//        to indicate that the polling buffer is filled externally from the device server code.
//
// args :
//        in :
//            - attr_name : The attribute name
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::init_attr_poll_ext_trig(const std::string &attr_name)
{
    std::string attr_lowercase(attr_name);
    std::transform(attr_lowercase.begin(), attr_lowercase.end(), attr_lowercase.begin(), ::tolower);

    //
    // check whether the attribute exists for the device and can be polled
    //

    dev_attr->get_attr_by_name(attr_lowercase.c_str());

    //
    // check wether the database is used
    //

    Tango::Util *tg = Tango::Util::instance();
    if(tg->use_db())
    {
        std::vector<std::string> &poll_list = get_polled_attr();
        Tango::DbData poll_data;
        bool found = false;

        poll_data.emplace_back("polled_attr");

        //
        // read the polling configuration from the database
        //

        if(!poll_list.empty())
        {
            //
            // search the attribute in the list of polled attributes
            //

            for(unsigned int i = 0; i < poll_list.size(); i = i + 2)
            {
                //
                // Convert to lower case before comparison
                //

                std::string name_lowercase(poll_list[i]);
                std::transform(name_lowercase.begin(), name_lowercase.end(), name_lowercase.begin(), ::tolower);

                if(name_lowercase == attr_lowercase)
                {
                    if(poll_list[i + 1] == "0")
                    {
                        //
                        // The configuration is already correct, no need for further action
                        //

                        return;
                    }
                    else
                    {
                        poll_list[i + 1] = "0";
                        found = true;
                    }
                }
            }
        }

        if(!found)
        {
            poll_list.push_back(attr_lowercase);
            poll_list.emplace_back("0");
        }

        poll_data[0] << poll_list;
        tg->get_database()->put_device_property(device_name, poll_data);
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::init_attr_poll_period
//
// description :
//        Checks the specified polling period for all attributes of the device. If a polling period is specified for an
//        attribute the attribute name and the period are written to the list of polled attributes in the database.
//        This happens only if the attribute is not yet in the list of polled attributes.
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceImpl::init_attr_poll_period()
{
    //
    // check wether the database is used
    //

    Tango::Util *tg = Tango::Util::instance();
    if(tg->use_db())
    {
        std::vector<std::string> &poll_list = get_polled_attr();
        Tango::DbData poll_data;

        poll_data.emplace_back("polled_attr");

        //
        // get the multi attribute object
        //

        std::vector<Attribute *> &attr_list = dev_attr->get_attribute_list();

        //
        // loop over the attribute list
        //

        unsigned long added_attr = 0;
        unsigned long i;
        for(i = 0; i < attr_list.size(); i++)
        {
            std::string &attr_name = attr_list[i]->get_name_lower();

            //
            // Special case for state and status attributes. They are polled as attribute but they are managed by Pogo
            // as commands (historical reasons). If the polling is set in the state or status defined as command, report
            // this info when they are defined as attributes
            //

            if(attr_name == "state")
            {
                Command &state_cmd = device_class->get_cmd_by_name("state");
                long state_poll_period = state_cmd.get_polling_period();
                if(state_poll_period != 0)
                {
                    attr_list[i]->set_polling_period(state_poll_period);
                }
            }

            if(attr_name == "status")
            {
                Command &status_cmd = device_class->get_cmd_by_name("status");
                long status_poll_period = status_cmd.get_polling_period();
                if(status_poll_period != 0)
                {
                    attr_list[i]->set_polling_period(status_poll_period);
                }
            }

            long poll_period;
            poll_period = attr_list[i]->get_polling_period();

            //
            // check the validity of the polling period. must be longer than 20ms
            //

            if(poll_period < MIN_POLL_PERIOD)
            {
                continue;
            }

            //
            // search the attribute in the list of polled attributes
            //

            bool found = false;
            for(unsigned int i = 0; i < poll_list.size(); i = i + 2)
            {
                //
                // Convert to lower case before comparison
                //

                std::string name_lowercase(poll_list[i]);
                std::transform(name_lowercase.begin(), name_lowercase.end(), name_lowercase.begin(), ::tolower);

                if(name_lowercase == attr_name)
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                std::string period_str;
                std::stringstream str;
                str << poll_period << std::ends;
                str >> period_str;

                poll_list.push_back(attr_name);
                poll_list.push_back(period_str);
                added_attr++;
            }
        }

        //
        // only write to the database when a polling need to be added
        //

        if(added_attr > 0)
        {
            poll_data[0] << poll_list;
            tg->get_database()->put_device_property(device_name, poll_data);
        }

        //
        // Another loop to correctly initialize polling period data in Attribute instance
        //

        for(unsigned int i = 0; i < poll_list.size(); i = i + 2)
        {
            try
            {
                Attribute &att = dev_attr->get_attr_by_name(poll_list[i].c_str());
                std::stringstream ss;
                long per;
                ss << poll_list[i + 1];
                ss >> per;
                if(ss)
                {
                    att.set_polling_period(per);
                }
            }
            catch(Tango::DevFailed &)
            {
            }
        }
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::push_att_conf_event
//
// description :
//        Push an attribute configuration event
//
// args :
//        in :
//            - attr : The attribute
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::push_att_conf_event(Attribute *attr)
{
    EventSupplier *event_supplier_nd = nullptr;
    EventSupplier *event_supplier_zmq = nullptr;

    Tango::Util *tg = Tango::Util::instance();

    if(attr->use_notifd_event())
    {
        event_supplier_nd = tg->get_notifd_event_supplier();
    }
    if(attr->use_zmq_event())
    {
        event_supplier_zmq = tg->get_zmq_event_supplier();
    }

    if((event_supplier_nd != nullptr) || (event_supplier_zmq != nullptr))
    {
        EventSupplier::SuppliedEventData ad;
        ::memset(&ad, 0, sizeof(ad));

        long vers = get_dev_idl_version();
        if(vers <= 2)
        {
            Tango::AttributeConfig_2 attr_conf_2;
            attr->get_properties(attr_conf_2);
            ad.attr_conf_2 = &attr_conf_2;
            if(event_supplier_nd != nullptr)
            {
                event_supplier_nd->push_att_conf_events(this, ad, (Tango::DevFailed *) nullptr, attr->get_name());
            }
            if(event_supplier_zmq != nullptr)
            {
                event_supplier_zmq->push_att_conf_events(this, ad, (Tango::DevFailed *) nullptr, attr->get_name());
            }
        }
        else if(vers <= 4)
        {
            Tango::AttributeConfig_3 attr_conf_3;
            attr->get_properties(attr_conf_3);
            ad.attr_conf_3 = &attr_conf_3;
            if(event_supplier_nd != nullptr)
            {
                event_supplier_nd->push_att_conf_events(this, ad, (Tango::DevFailed *) nullptr, attr->get_name());
            }
            if(event_supplier_zmq != nullptr)
            {
                event_supplier_zmq->push_att_conf_events(this, ad, (Tango::DevFailed *) nullptr, attr->get_name());
            }
        }
        else
        {
            Tango::AttributeConfig_5 attr_conf_5;
            attr->get_properties(attr_conf_5);
            ad.attr_conf_5 = &attr_conf_5;
            if(event_supplier_nd != nullptr)
            {
                event_supplier_nd->push_att_conf_events(this, ad, (Tango::DevFailed *) nullptr, attr->get_name());
            }
            if(event_supplier_zmq != nullptr)
            {
                event_supplier_zmq->push_att_conf_events(this, ad, (Tango::DevFailed *) nullptr, attr->get_name());
            }
        }
    }
}

//+----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::get_client_ident()
//
// description :
//        Get client identification. This method returns a pointer to the client identification
//
//-----------------------------------------------------------------------------------------------------------------

Tango::client_addr *DeviceImpl::get_client_ident()
{
    omni_thread::value_t *ip = omni_thread::self()->get_value(Util::get_tssk_client_info());
    return (client_addr *) ip;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::lock
//
// description :
//        Lock the device
//
// args :
//        in :
//            - cl : The client identification
//            - validity : The lock validity
//
//-----------------------------------------------------------------------------------------------------------------

void DeviceImpl::lock(client_addr *cl, int validity)
{
    //
    // Check if the device is already locked and if it is a valid lock. If the lock is not valid any more, clear it
    //

    if(device_locked)
    {
        if(valid_lock())
        {
            if(*cl != *(locker_client))
            {
                TangoSys_OMemStream o;
                o << "Device " << get_name() << " is already locked by another client" << std::ends;
                TANGO_THROW_EXCEPTION(API_DeviceLocked, o.str());
            }
        }
        else
        {
            basic_unlock();
        }
    }

    //
    // Lock the device
    //

    device_locked = true;
    if(locker_client == nullptr)
    {
        locker_client = new client_addr(*cl);
    }

    locking_date = Tango::get_current_system_datetime();
    lock_validity = validity;
    lock_ctr++;

    //
    // Also lock root device(s) in case it is needed (due to forwarded attributes)
    //

    if(get_with_fwd_att())
    {
        lock_root_devices(validity, true);
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::relock
//
// description :
//        ReLock the device
//
// args :
//        in :
//            - cl : The client identification
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::relock(client_addr *cl)
{
    //
    // Check if the device is already locked and if it is a valid lock. A ReLock is valid only if the device is already
    // locked by the same client and if this lock is valid
    //

    if(device_locked)
    {
        if(valid_lock())
        {
            if(*cl != *(locker_client))
            {
                TangoSys_OMemStream o;
                o << get_name() << ": ";
                o << "Device " << get_name() << " is already locked by another client" << std::ends;
                TANGO_THROW_EXCEPTION(API_DeviceLocked, o.str());
            }

            device_locked = true;
            locking_date = Tango::get_current_system_datetime();
        }
        else
        {
            TangoSys_OMemStream o;
            o << get_name() << ": ";
            o << "Device " << get_name() << " is not locked. Can't re-lock it" << std::ends;
            TANGO_THROW_EXCEPTION(API_DeviceNotLocked, o.str());
        }
    }
    else
    {
        TangoSys_OMemStream o;
        o << get_name() << ": ";
        o << "Device " << get_name() << " is not locked. Can't re-lock it" << std::ends;
        TANGO_THROW_EXCEPTION(API_DeviceNotLocked, o.str());
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::unlock
//
// description :
//        Unlock the device
//
// args :
//        in :
//            - forced : Flag set to true if the unlock is forced
//
//-----------------------------------------------------------------------------------------------------------------

Tango::DevLong DeviceImpl::unlock(bool forced)
{
    //
    // Check if the device is already locked and if it is a valid lock. If the lock is not valid any more, clear it
    //

    if(device_locked)
    {
        if(valid_lock())
        {
            client_addr *cl = get_client_ident();

            if(!forced)
            {
                if(*cl != *(locker_client))
                {
                    TangoSys_OMemStream o;
                    o << "Device " << get_name() << " is locked by another client, can't unlock it" << std::ends;
                    TANGO_THROW_EXCEPTION(API_DeviceLocked, o.str());
                }
            }
        }
    }

    if(lock_ctr > 0)
    {
        lock_ctr--;
    }
    if((lock_ctr <= 0) || (forced))
    {
        basic_unlock(forced);
    }

    return lock_ctr;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::basic_unlock
//
// description :
//        Mark the device as unlocked
//
// args :
//        in :
//            - forced : Flag set to true if the unlock is forced
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::basic_unlock(bool forced)
{
    device_locked = false;
    if(forced)
    {
        old_locker_client = locker_client;
    }
    else
    {
        delete locker_client;
    }
    locker_client = nullptr;
    lock_ctr = 0;

    //
    // Also unlock root device(s) in case it is needed (due to forwarded attributes)
    //

    if(get_with_fwd_att())
    {
        lock_root_devices(0, false);
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::valid_lock
//
// description :
//        Check lock validity (according to lock validity time). This method returns true if the lock is still valid.
//        Otherwise, returns false
//
//-------------------------------------------------------------------------------------------------------------------

bool DeviceImpl::valid_lock()
{
    const time_t now = Tango::get_current_system_datetime();
    return now <= (locking_date + lock_validity);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::lock_status
//
// description :
//        Build a device locking status
//        This method returns a sequence with longs and strings. The strings contain:
//            1 - The locker process hostname
//            2 - The java main class (in case of Java locker)
//            3 - A string which summarizes the locking status
//         The longs contain:
//            1 - A locked flag (0 means not locked, 1 means locked)
//            2 - The locker process PID (C++ client)
//            3 - The locker UUID (Java client) which needs 4 longs
//
//------------------------------------------------------------------------------------------------------------------

Tango::DevVarLongStringArray *DeviceImpl::lock_status()
{
    auto *dvlsa = new Tango::DevVarLongStringArray();
    dvlsa->lvalue.length(6);
    dvlsa->svalue.length(3);

    //
    // Check if the device is already locked and if it is a valid lock. If the lock is not valid any more, clear it
    //

    if(device_locked)
    {
        if(valid_lock())
        {
            lock_stat = "Device " + device_name + " is locked by ";
            std::ostringstream ostr;
            ostr << *(locker_client) << std::ends;
            lock_stat = lock_stat + ostr.str();

            dvlsa->lvalue[0] = 1;
            dvlsa->lvalue[1] = locker_client->client_pid;
            const char *tmp = locker_client->client_ip;
            dvlsa->svalue[1] = CORBA::string_dup(tmp);
            if(locker_client->client_lang == Tango::JAVA)
            {
                dvlsa->svalue[2] = CORBA::string_dup(locker_client->java_main_class.c_str());

                Tango::DevULong64 tmp_data = locker_client->java_ident[0];
                dvlsa->lvalue[2] = (DevLong) ((tmp_data & 0xFFFFFFFF00000000LL) >> 32);
                dvlsa->lvalue[3] = (DevLong) (tmp_data & 0xFFFFFFFF);

                tmp_data = locker_client->java_ident[1];
                dvlsa->lvalue[4] = (DevLong) ((tmp_data & 0xFFFFFFFF00000000LL) >> 32);
                dvlsa->lvalue[5] = (DevLong) (tmp_data & 0xFFFFFFFF);
            }
            else
            {
                dvlsa->svalue[2] = CORBA::string_dup("Not defined");
                for(long loop = 2; loop < 6; loop++)
                {
                    dvlsa->lvalue[loop] = 0;
                }
            }
        }
        else
        {
            basic_unlock();
            lock_stat = "Device " + device_name + " is not locked";
            dvlsa->svalue[1] = CORBA::string_dup("Not defined");
            dvlsa->svalue[2] = CORBA::string_dup("Not defined");
            for(long loop = 0; loop < 6; loop++)
            {
                dvlsa->lvalue[loop] = 0;
            }
        }
    }
    else
    {
        lock_stat = "Device " + device_name + " is not locked";
        dvlsa->svalue[1] = CORBA::string_dup("Not defined");
        dvlsa->svalue[2] = CORBA::string_dup("Not defined");
        for(long loop = 0; loop < 6; loop++)
        {
            dvlsa->lvalue[loop] = 0;
        }
    }

    dvlsa->svalue[0] = CORBA::string_dup(lock_stat.c_str());

    return dvlsa;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::set_locking_param
//
// description :
//        Restore device locking parameter
//
// args :
//        in :
//            - cl : Locker
//              - old_cl : Previous locker
//              - date : Locking date
//              - ctr : Locking counter
//              - valid : Locking validity
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::set_locking_param(client_addr *cl, client_addr *old_cl, time_t date, DevLong ctr, DevLong valid)
{
    locker_client = cl;
    old_locker_client = old_cl;
    locking_date = date;
    lock_ctr = ctr;
    device_locked = true;
    lock_validity = valid;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::check_lock
//
// description :
//        Method called for each command_inout operation executed from any client on a Tango device.
//
// argument:
//        in :
//            - meth : Method name (for error message)
//            - cmd : Command name
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::check_lock(const char *meth, const char *cmd)
{
    if(device_locked)
    {
        if(valid_lock())
        {
            client_addr *cl = get_client_ident();
            if(!cl->client_ident)
            {
                //
                // Old client, before throwing the exception, in case the CORBA operation is a command_inout, checks if
                // the command is an "allowed" one
                //

                if(cmd != nullptr)
                {
                    if(!device_class->is_command_allowed(cmd))
                    {
                        throw_locked_exception(meth);
                    }
                }
                else
                {
                    throw_locked_exception(meth);
                }
            }

            if(*cl != *(locker_client))
            {
                //
                // Wrong client, before throwing the exception, in case the CORBA operation is a command_inout, checks
                // if the command is an "allowed" one
                //

                if(cmd != nullptr)
                {
                    if(!device_class->is_command_allowed(cmd))
                    {
                        throw_locked_exception(meth);
                    }
                }
                else
                {
                    throw_locked_exception(meth);
                }
            }
        }
        else
        {
            basic_unlock();
        }
    }
    else
    {
        client_addr *cl = get_client_ident();
        if(old_locker_client != nullptr)
        {
            if(*cl == (*old_locker_client))
            {
                TangoSys_OMemStream o;
                TangoSys_OMemStream o2;
                o << "Device " << get_name() << " has been unlocked by an administrative client!!!" << std::ends;
                o2 << "Device_Impl::" << meth << std::ends;
                Except::throw_exception(DEVICE_UNLOCKED_REASON, o.str(), o2.str());
            }
            delete old_locker_client;
            old_locker_client = nullptr;
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::throw_locked_exception()
//
// description :
//        Throw a DeviceLocked exception
//
// argument:
//        in :
//            - meth : Method name
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceImpl::throw_locked_exception(const char *meth)
{
    TangoSys_OMemStream o;
    TangoSys_OMemStream o2;
    o << "Device " << get_name() << " is locked by another client" << std::ends;
    o2 << "Device_Impl::" << meth << std::ends;
    Except::throw_exception(API_DeviceLocked, o.str(), o2.str());
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::data_into_net_obj
//
// description :
//        Put the attribute data within the object used on the wire to transfer the attribute. For IDL release <= 3,
//        it's an Any object. Then, it is an IDL union
//
// argument:
//        in :
//            - att :
//            - aid :
//            - index :
//            - w_type :
//            - del_seq :
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceImpl::data_into_net_object(
    Attribute &att, AttributeIdlData &aid, long index, AttrWriteType w_type, bool del_seq)
{
    TANGO_LOG_DEBUG << "DeviceImpl::data_into_net_object() called " << std::endl;

    //
    // A big switch according to attribute data type
    //

    switch(att.get_data_type())
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
    {
        data_in_object<Tango::DevVarShortArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_LONG:
    {
        data_in_object<Tango::DevVarLongArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_LONG64:
    {
        data_in_object<Tango::DevVarLong64Array>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_DOUBLE:
    {
        data_in_object<Tango::DevVarDoubleArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_STRING:
    {
        data_in_object<Tango::DevVarStringArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_FLOAT:
    {
        data_in_object<Tango::DevVarFloatArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_BOOLEAN:
    {
        data_in_object<Tango::DevVarBooleanArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_USHORT:
    {
        data_in_object<Tango::DevVarUShortArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_UCHAR:
    {
        data_in_object<Tango::DevVarCharArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_ULONG:
    {
        data_in_object<Tango::DevVarULongArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_ULONG64:
    {
        data_in_object<Tango::DevVarULong64Array>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_STATE:
    {
        data_in_object<Tango::DevVarStateArray>(att, aid, index, del_seq);
        break;
    }

    case Tango::DEV_ENCODED:
    {
        if(aid.data_3 != nullptr)
        {
            (*aid.data_3)[index].err_list.length(1);
            (*aid.data_3)[index].err_list[0].severity = Tango::ERR;
            (*aid.data_3)[index].err_list[0].reason = Tango::string_dup(API_NotSupportedFeature);
            (*aid.data_3)[index].err_list[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
            (*aid.data_3)[index].err_list[0].desc =
                Tango::string_dup("The DevEncoded data type is available only for device implementing IDL 4 and above");
            (*aid.data_3)[index].quality = Tango::ATTR_INVALID;
            (*aid.data_3)[index].name = Tango::string_dup(att.get_name().c_str());
            clear_att_dim((*aid.data_3)[index]);
        }
        else
        {
            Tango::DevVarEncodedArray *ptr = att.get_encoded_value();
            if(aid.data_5 != nullptr)
            {
                (*aid.data_5)[index].value.encoded_att_value(dummy_encoded_att_value);
                DevVarEncodedArray &the_seq = (*aid.data_5)[index].value.encoded_att_value();

                if((w_type == Tango::READ) || (w_type == Tango::WRITE))
                {
                    the_seq.length(1);
                }
                else
                {
                    the_seq.length(2);
                }

                the_seq[0].encoded_format = CORBA::string_dup((*ptr)[0].encoded_format);

                if(ptr->release())
                {
                    unsigned long nb_data = (*ptr)[0].encoded_data.length();
                    the_seq[0].encoded_data.replace(nb_data, nb_data, (*ptr)[0].encoded_data.get_buffer(true), true);
                    (*ptr)[0].encoded_data.replace(0, 0, nullptr, false);
                }
                else
                {
                    the_seq[0].encoded_data.replace((*ptr)[0].encoded_data.length(),
                                                    (*ptr)[0].encoded_data.length(),
                                                    (*ptr)[0].encoded_data.get_buffer());
                }

                if((w_type == Tango::READ_WRITE) || (w_type == Tango::READ_WITH_WRITE))
                {
                    the_seq[1].encoded_format = CORBA::string_dup((*ptr)[1].encoded_format);
                    the_seq[1].encoded_data.replace((*ptr)[1].encoded_data.length(),
                                                    (*ptr)[1].encoded_data.length(),
                                                    (*ptr)[1].encoded_data.get_buffer());
                }
            }
            else
            {
                (*aid.data_4)[index].value.encoded_att_value(dummy_encoded_att_value);
                DevVarEncodedArray &the_seq = (*aid.data_4)[index].value.encoded_att_value();

                if((w_type == Tango::READ) || (w_type == Tango::WRITE))
                {
                    the_seq.length(1);
                }
                else
                {
                    the_seq.length(2);
                }

                the_seq[0].encoded_format = CORBA::string_dup((*ptr)[0].encoded_format);

                if(ptr->release())
                {
                    unsigned long nb_data = (*ptr)[0].encoded_data.length();
                    the_seq[0].encoded_data.replace(nb_data, nb_data, (*ptr)[0].encoded_data.get_buffer(true), true);
                    (*ptr)[0].encoded_data.replace(0, 0, nullptr, false);
                }
                else
                {
                    the_seq[0].encoded_data.replace((*ptr)[0].encoded_data.length(),
                                                    (*ptr)[0].encoded_data.length(),
                                                    (*ptr)[0].encoded_data.get_buffer());
                }

                if((w_type == Tango::READ_WRITE) || (w_type == Tango::READ_WITH_WRITE))
                {
                    the_seq[1].encoded_format = CORBA::string_dup((*ptr)[1].encoded_format);
                    the_seq[1].encoded_data.replace((*ptr)[1].encoded_data.length(),
                                                    (*ptr)[1].encoded_data.length(),
                                                    (*ptr)[1].encoded_data.get_buffer());
                }
            }
            if(del_seq)
            {
                att.delete_seq();
            }
        }
        break;
    }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::polled_data_into_net_obj
//
// description :
//        Put the attribute data within the object used on the wire to transfer the attribute. For IDL release <= 3,
//        it's an Any object. Then, it is an IDL union
//
// argument:
//        in :
//            - aid :
//            - index :
//            - type :
//            - vers : Device IDl version
//            - polled_att :
//            - names :
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceImpl::polled_data_into_net_object(
    AttributeIdlData &aid, long index, long type, long vers, PollObj *polled_att, const DevVarStringArray &names)
{
    const Tango::DevVarStateArray *tmp_state;
    Tango::DevVarStateArray *new_tmp_state;
    Tango::DevState sta;

    switch(type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        data_in_net_object<Tango::DevShort>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_DOUBLE:
        data_in_net_object<Tango::DevDouble>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_LONG:
        data_in_net_object<Tango::DevLong>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_LONG64:
        data_in_net_object<Tango::DevLong64>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_STRING:
        data_in_net_object<Tango::DevString>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_FLOAT:
        data_in_net_object<Tango::DevFloat>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_BOOLEAN:
        data_in_net_object<Tango::DevBoolean>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_USHORT:
        data_in_net_object<Tango::DevUShort>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_UCHAR:
        data_in_net_object<Tango::DevUChar>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_ULONG:
        data_in_net_object<Tango::DevULong>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_ULONG64:
        data_in_net_object<Tango::DevULong64>(aid, index, vers, polled_att);
        break;

    case Tango::DEV_STATE:
        if(aid.data_5 != nullptr)
        {
            AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
            if(att_val.value._d() == DEVICE_STATE)
            {
                sta = att_val.value.dev_state_att();
                (*aid.data_5)[index].value.dev_state_att(sta);
            }
            else if(att_val.value._d() == ATT_STATE)
            {
                DevVarStateArray &union_seq = att_val.value.state_att_value();
                (*aid.data_5)[index].value.state_att_value(union_seq);
            }
        }
        else if(aid.data_4 != nullptr)
        {
            if(vers >= 5)
            {
                AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
                if(att_val.value._d() == DEVICE_STATE)
                {
                    sta = att_val.value.dev_state_att();
                    (*aid.data_4)[index].value.dev_state_att(sta);
                }
                else if(att_val.value._d() == ATT_STATE)
                {
                    DevVarStateArray &union_seq = att_val.value.state_att_value();
                    (*aid.data_4)[index].value.state_att_value(union_seq);
                }
            }
            else
            {
                AttributeValue_4 &att_val = polled_att->get_last_attr_value_4(false);
                if(att_val.value._d() == DEVICE_STATE)
                {
                    sta = att_val.value.dev_state_att();
                    (*aid.data_4)[index].value.dev_state_att(sta);
                }
                else if(att_val.value._d() == ATT_STATE)
                {
                    DevVarStateArray &union_seq = att_val.value.state_att_value();
                    (*aid.data_4)[index].value.state_att_value(union_seq);
                }
            }
        }
        else
        {
            if(vers >= 5)
            {
                AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
                if(att_val.value._d() == DEVICE_STATE)
                {
                    sta = att_val.value.dev_state_att();
                    (*aid.data_3)[index].value <<= sta;
                }
                else if(att_val.value._d() == ATT_STATE)
                {
                    DevVarStateArray &union_seq = att_val.value.state_att_value();
                    new_tmp_state = new DevVarStateArray(
                        union_seq.length(), union_seq.length(), const_cast<DevState *>(union_seq.get_buffer()), false);
                    (*aid.data_3)[index].value <<= new_tmp_state;
                }
            }
            else if(vers == 4)
            {
                AttributeValue_4 &att_val = polled_att->get_last_attr_value_4(false);
                if(att_val.value._d() == DEVICE_STATE)
                {
                    sta = att_val.value.dev_state_att();
                    (*aid.data_3)[index].value <<= sta;
                }
                else if(att_val.value._d() == ATT_STATE)
                {
                    DevVarStateArray &union_seq = att_val.value.state_att_value();
                    new_tmp_state = new DevVarStateArray(
                        union_seq.length(), union_seq.length(), const_cast<DevState *>(union_seq.get_buffer()), false);
                    (*aid.data_3)[index].value <<= new_tmp_state;
                }
            }
            else
            {
                AttributeValue_3 &att_val = polled_att->get_last_attr_value_3(false);
                CORBA::TypeCode_var ty;
                ty = att_val.value.type();

                if(ty->kind() == CORBA::tk_enum)
                {
                    att_val.value >>= sta;
                    (*aid.data_3)[index].value <<= sta;
                }
                else
                {
                    att_val.value >>= tmp_state;
                    new_tmp_state = new DevVarStateArray(tmp_state->length(),
                                                         tmp_state->length(),
                                                         const_cast<DevState *>(tmp_state->get_buffer()),
                                                         false);
                    (*aid.data_3)[index].value <<= new_tmp_state;
                }
            }
        }
        break;

    case Tango::DEV_ENCODED:
        if(aid.data_5 != nullptr)
        {
            AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
            DevVarEncodedArray &polled_seq = att_val.value.encoded_att_value();

            unsigned int nb_encoded = polled_seq.length();

            (*aid.data_5)[index].value.encoded_att_value(dummy_encoded_att_value);
            DevVarEncodedArray &the_seq = (*aid.data_5)[index].value.encoded_att_value();

            the_seq.length(nb_encoded);
            for(unsigned int loop = 0; loop < nb_encoded; loop++)
            {
                the_seq[loop].encoded_format = CORBA::string_dup(polled_seq[loop].encoded_format);
                unsigned char *tmp_enc = polled_seq[loop].encoded_data.get_buffer();
                unsigned int nb_data = polled_seq[loop].encoded_data.length();
                the_seq[loop].encoded_data.replace(nb_data, nb_data, tmp_enc);
            }
        }
        else if(aid.data_4 != nullptr)
        {
            if(vers >= 5)
            {
                AttributeValue_5 &att_val = polled_att->get_last_attr_value_5(false);
                DevVarEncodedArray &polled_seq = att_val.value.encoded_att_value();

                unsigned int nb_encoded = polled_seq.length();

                (*aid.data_4)[index].value.encoded_att_value(dummy_encoded_att_value);
                DevVarEncodedArray &the_seq = (*aid.data_4)[index].value.encoded_att_value();

                the_seq.length(nb_encoded);
                for(unsigned int loop = 0; loop < nb_encoded; loop++)
                {
                    the_seq[loop].encoded_format = CORBA::string_dup(polled_seq[loop].encoded_format);
                    unsigned char *tmp_enc = polled_seq[loop].encoded_data.get_buffer();
                    unsigned int nb_data = polled_seq[loop].encoded_data.length();
                    the_seq[loop].encoded_data.replace(nb_data, nb_data, tmp_enc);
                }
            }
            else
            {
                AttributeValue_4 &att_val = polled_att->get_last_attr_value_4(false);
                DevVarEncodedArray &polled_seq = att_val.value.encoded_att_value();

                unsigned int nb_encoded = polled_seq.length();

                (*aid.data_4)[index].value.encoded_att_value(dummy_encoded_att_value);
                DevVarEncodedArray &the_seq = (*aid.data_4)[index].value.encoded_att_value();

                the_seq.length(nb_encoded);
                for(unsigned int loop = 0; loop < nb_encoded; loop++)
                {
                    the_seq[loop].encoded_format = CORBA::string_dup(polled_seq[loop].encoded_format);
                    unsigned char *tmp_enc = polled_seq[loop].encoded_data.get_buffer();
                    unsigned int nb_data = polled_seq[loop].encoded_data.length();
                    the_seq[loop].encoded_data.replace(nb_data, nb_data, tmp_enc);
                }
            }
        }
        else
        {
            TangoSys_OMemStream o;
            o << "Data type for attribute " << names[index] << " is DEV_ENCODED.";
            o << " It's not possible to retrieve this data type through the interface you are using (IDL V3)"
              << std::ends;

            (*aid.data_3)[index].err_list.length(1);
            (*aid.data_3)[index].err_list[0].severity = Tango::ERR;
            (*aid.data_3)[index].err_list[0].reason = Tango::string_dup(API_NotSupportedFeature);
            (*aid.data_3)[index].err_list[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string s = o.str();
            (*aid.data_3)[index].err_list[0].desc = Tango::string_dup(s.c_str());
            (*aid.data_3)[index].quality = Tango::ATTR_INVALID;
            (*aid.data_3)[index].name = Tango::string_dup(names[index]);
            clear_att_dim((*aid.data_3)[index]);
        }
        break;
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//            DeviceImpl::att_conf_loop
//
// description :
//            Set flags in DeviceImpl if any of the device attributes has some wrong configuration in DB generating
//            startup exception when the server started.
//            In DeviceImpl class, this method set the force_alarm_state flag and fills in the
//          att_wrong_db_conf vector with attribute name(s) (for device status)
//
//--------------------------------------------------------------------------------------------------------------------

void DeviceImpl::att_conf_loop()
{
    std::vector<Attribute *> &att_list = get_device_attr()->get_attribute_list();

    //
    // Reset data before the new loop
    //

    std::vector<std::string> &wrong_conf_att_list = get_att_wrong_db_conf();
    wrong_conf_att_list.clear();

    std::vector<std::string> &mem_att_list = get_att_mem_failed();
    mem_att_list.clear();

    force_alarm_state = false;

    //
    // Run the loop for wrong attribute conf. or memorized att which failed at startup
    //

    for(size_t i = 0; i < att_list.size(); ++i)
    {
        if(att_list[i]->is_startup_exception() || att_list[i]->is_mem_exception())
        {
            force_alarm_state = true;
            if(att_list[i]->is_startup_exception())
            {
                wrong_conf_att_list.push_back(att_list[i]->get_name());
            }
            else
            {
                mem_att_list.push_back(att_list[i]->get_name());
            }
        }
    }

    if(!force_alarm_state && !fwd_att_wrong_conf.empty())
    {
        force_alarm_state = true;
    }

    run_att_conf_loop = false;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::check_att_conf
//
// description :
//
//--------------------------------------------------------------------------------------------------------------------

void DeviceImpl::check_att_conf()
{
    dev_attr->check_idl_release(this);

    if(run_att_conf_loop)
    {
        att_conf_loop();
    }
}

//----------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::build_att_list_in_status_mess
//
// description :
//        Build device status message in case of device with some
//            - att with wrong conf. in db
//            - some memorized attribute failed during the startup phase
//            - some wrongly configured forwarded attributes
//
// argument:
//        in :
//            - nb_att : Number of attributes in error
//            - att_type : Type of attribute error (conf or mem)
//
//---------------------------------------------------------------------------------------------------------------------

void DeviceImpl::build_att_list_in_status_mess(size_t nb_att, AttErrorType att_type)
{
    //
    // First, the wrongly configured forwarded attributes case
    //

    if(att_type == Tango::DeviceImpl::FWD)
    {
        std::stringstream alarm_msg;
        alarm_msg << alarm_status;
        for(size_t i = 0; i < nb_att; ++i)
        {
            alarm_msg << "\nForwarded attribute " + fwd_att_wrong_conf[i].att_name;
            if(fwd_att_wrong_conf[i].fae != FWD_ROOT_DEV_NOT_STARTED)
            {
                alarm_msg << " is not correctly configured! ";
            }
            else
            {
                alarm_msg << " is not reachable! ";
            }

            alarm_msg << "\nRoot attribute name = ";
            alarm_msg << fwd_att_wrong_conf[i].full_root_att_name;
            if(fwd_att_wrong_conf[i].fae != FWD_ROOT_DEV_NOT_STARTED)
            {
                alarm_msg << "\nYou can update it using the Jive tool";
            }
            alarm_msg << "\nError: ";

            alarm_msg << fwd_att_wrong_conf[i].fae;

            if(fwd_att_wrong_conf[i].fae == FWD_DOUBLE_USED)
            {
                Util *tg = Util::instance();
                std::string root_name(fwd_att_wrong_conf[i].full_root_att_name);
                std::transform(root_name.begin(), root_name.end(), root_name.begin(), ::tolower);
                std::string local_att_name = tg->get_root_att_reg().get_local_att_name(root_name);
                alarm_msg << local_att_name;
            }
        }
        alarm_status = alarm_msg.str();
    }
    else
    {
        //
        // For wrong conf. in db or memorized attributes which failed at startup
        //

        if(nb_att > 1)
        {
            alarm_status = alarm_status + "s";
        }
        alarm_status = alarm_status + " ";
        for(size_t i = 0; i < nb_att; ++i)
        {
            if(att_type == Tango::DeviceImpl::CONF)
            {
                alarm_status = alarm_status + att_wrong_db_conf[i];
            }
            else
            {
                alarm_status = alarm_status + att_mem_failed[i];
            }

            if((nb_att > 1) && (i <= nb_att - 2))
            {
                alarm_status = alarm_status + ", ";
            }
        }

        if(nb_att == 1)
        {
            alarm_status = alarm_status + " has ";
        }
        else
        {
            alarm_status = alarm_status + " have ";
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::is_there_subscriber
//
// description :
//        Returns true if there is some subscriber(s) listening on the event
//
// argument:
//        in :
//            - att_name : The attribute name
//            - event_type : The event type
//
//---------------------------------------------------------------------------------------------------------------------

bool DeviceImpl::is_there_subscriber(const std::string &att_name, EventType event_type)
{
    Attribute &att = dev_attr->get_attr_by_name(att_name.c_str());

    bool ret = false;

    switch(event_type)
    {
    case CHANGE_EVENT:
        ret = att.change_event_subscribed();
        break;

    case ALARM_EVENT:
        ret = att.alarm_event_subscribed();
        break;

    case PERIODIC_EVENT:
        ret = att.periodic_event_subscribed();
        break;

    case ARCHIVE_EVENT:
        ret = att.archive_event_subscribed();
        break;

    case USER_EVENT:
        ret = att.user_event_subscribed();
        break;

    case ATTR_CONF_EVENT:
        ret = att.attr_conf_event_subscribed();
        break;

    case DATA_READY_EVENT:
        ret = att.data_ready_event_subscribed();
        break;

    default:
        TANGO_THROW_EXCEPTION(API_UnsupportedFeature, "Unsupported event type");
        break;
    }

    return ret;
}

//----------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::rem_wrong_fwd_att
//
// description :
//        Remove one forwarded attribute from the list of errored forwarded attribute
//
// argument:
//        in :
//            - root_att_name : The root attribute name to be removed
//
//---------------------------------------------------------------------------------------------------------------------

void DeviceImpl::rem_wrong_fwd_att(const std::string &root_att_name)
{
    std::vector<FwdWrongConf>::iterator ite;
    for(ite = fwd_att_wrong_conf.begin(); ite != fwd_att_wrong_conf.end(); ++ite)
    {
        std::string local_name(ite->full_root_att_name);
        std::transform(local_name.begin(), local_name.end(), local_name.begin(), ::tolower);
        if(local_name == root_att_name)
        {
            fwd_att_wrong_conf.erase(ite);
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::update_wrong_fwd_att
//
// description :
//        Update error code for one forwarded attribute in the list of errored forwarded attribute
//
// argument:
//        in :
//            - root_att_name : The root attribute name to be removed
//            - err : The new error code
//
//---------------------------------------------------------------------------------------------------------------------

void DeviceImpl::update_wrong_conf_att(const std::string &root_att_name, FwdAttError err)
{
    std::vector<FwdWrongConf>::iterator ite;
    for(ite = fwd_att_wrong_conf.begin(); ite != fwd_att_wrong_conf.end(); ++ite)
    {
        std::string local_name(ite->full_root_att_name);
        std::transform(local_name.begin(), local_name.end(), local_name.begin(), ::tolower);
        if(local_name == root_att_name)
        {
            ite->fae = err;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::lock_root_devices
//
// description :
//        Lock/Unlock all root devices for all the forwarded attributes defined for this device
//
// argument:
//        in :
//            - validity : The lock validity interval (used only in case of locking)
//            - lock_action : Flag set to true if root device(s) must be locked. If false, root devices will be unlocked
//
//---------------------------------------------------------------------------------------------------------------------

void DeviceImpl::lock_root_devices(int validity, bool lock_action)
{
    //
    // Get list of root device(s)
    //

    std::vector<std::string> root_devs;
    std::vector<std::string>::iterator ite;
    std::vector<Attribute *> att_list = dev_attr->get_attribute_list();
    for(size_t j = 0; j < att_list.size(); j++)
    {
        if(att_list[j]->is_fwd_att())
        {
            FwdAttribute *fwd_att = static_cast<FwdAttribute *>(att_list[j]);
            const std::string &dev_name = fwd_att->get_fwd_dev_name();
            ite = find(root_devs.begin(), root_devs.end(), dev_name);
            if(ite == root_devs.end())
            {
                root_devs.push_back(dev_name);
            }
        }
    }

    //
    // Lock/Unlock all these devices
    //

    RootAttRegistry &rar = Util::instance()->get_root_att_reg();
    for(size_t loop = 0; loop < root_devs.size(); loop++)
    {
        DeviceProxy *dp = rar.get_root_att_dp(root_devs[loop]);

        if(lock_action)
        {
            dp->lock(validity);
        }
        else
        {
            dp->unlock();
        }
    }
}

//+----------------------------------------------------------------------------
//
// method :        DeviceImpl::get_local_cmd_by_name
//
// description :    Get a reference to a local Command object
//
// in :     cmd_name : The command name
//
//-----------------------------------------------------------------------------

Command &DeviceImpl::get_local_cmd_by_name(const std::string &cmd_name)
{
    std::vector<Command *>::iterator pos;

    pos = find_if(command_list.begin(),
                  command_list.end(),
                  [&](Command *cmd) -> bool
                  {
                      if(cmd_name.size() != cmd->get_lower_name().size())
                      {
                          return false;
                      }
                      std::string tmp_name(cmd_name);
                      std::transform(tmp_name.begin(), tmp_name.end(), tmp_name.begin(), ::tolower);
                      return cmd->get_lower_name() == tmp_name;
                  });

    if(pos == command_list.end())
    {
        TANGO_LOG_DEBUG << "DeviceImpl::get_cmd_by_name throwing exception" << std::endl;
        TangoSys_OMemStream o;

        o << cmd_name << " command not found" << std::ends;
        TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());
    }

    return *(*pos);
}

//+----------------------------------------------------------------------------
//
// method :        DeviceImpl::remove_local_command
//
// description :    Delete a command from the local command list
//
// in :     cmd_name : The command name (in lower case letter)
//
//-----------------------------------------------------------------------------

void DeviceImpl::remove_local_command(const std::string &cmd_name)
{
    std::vector<Command *>::iterator pos;

    pos = find_if(command_list.begin(),
                  command_list.end(),
                  [&](Command *cmd) -> bool
                  {
                      if(cmd_name.size() != cmd->get_lower_name().size())
                      {
                          return false;
                      }
                      return cmd->get_lower_name() == cmd_name;
                  });

    if(pos == command_list.end())
    {
        TANGO_LOG_DEBUG << "DeviceImpl::remove_local_command throwing exception" << std::endl;
        TangoSys_OMemStream o;

        o << cmd_name << " command not found" << std::ends;
        TANGO_THROW_EXCEPTION(API_CommandNotFound, o.str());
    }

    command_list.erase(pos);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::get_event_subscription_states
//
// description :
//        Return event info for the device with events subscribed
//
//------------------------------------------------------------------------------------------------------------------

DeviceEventSubscriptionState DeviceImpl::get_event_subscription_state()
{
    ZmqEventSupplier *event_supplier_zmq = Util::instance()->get_zmq_event_supplier();

    DeviceEventSubscriptionState events{};
    events.has_dev_intr_change_event_clients = event_supplier_zmq->any_dev_intr_client(this);
    events.attribute_events = dev_attr->get_event_subscription_states();
    events.pipe_events = get_pipe_event_subscription_states();

    return events;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::set_event_subscription_state
//
// description :
//      Set device interface change event subscription time
//
// argument :
//         in :
//            - eve : One structure in this vector for each device event subscribed
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::set_event_subscription_state(const DeviceEventSubscriptionState &events)
{
    if(events.has_dev_intr_change_event_clients)
    {
        set_event_intr_change_subscription(Tango::get_current_system_datetime());
    }

    dev_attr->set_event_subscription_states(events.attribute_events);
    set_pipe_event_subscription_states(events.pipe_events);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::push_dev_intr
//
// description :
//        Start or update device interface change event thread
//
// argument :
//         in :
//            - ev_client : Flag set to true if some clients are listening on the event
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::push_dev_intr(bool ev_client)
{
    //
    // If device is IDL 5 or more and if enabled, push a device interface change event but only if there is some
    // client(s) listening on the event.
    // This is done by starting a dedicated thread (if not already started). The rule of this thread is to delayed
    // the event in case of attributes/commands added/removed in a loop in order to minimize the event number.
    //

    if(idl_version >= MIN_IDL_DEV_INTR && is_intr_change_ev_enable() && ev_client)
    {
        bool th_running;
        {
            omni_mutex_lock lo(devintr_mon);
            th_running = devintr_shared.th_running;
        }

        if(!th_running)
        {
            devintr_shared.cmd_pending = false;
            devintr_thread = new DevIntrThread(devintr_shared, devintr_mon, this);
            devintr_shared.th_running = true;

            devintr_thread->start();
        }
        else
        {
            int interupted;

            omni_mutex_lock sync(devintr_mon);

            devintr_shared.cmd_pending = true;
            devintr_shared.cmd_code = DEV_INTR_SLEEP;

            devintr_mon.signal();

            TANGO_LOG_DEBUG << "Cmd sent to device interface change thread" << std::endl;

            while(devintr_shared.cmd_pending)
            {
                interupted = devintr_mon.wait(DEFAULT_TIMEOUT);

                if((devintr_shared.cmd_pending) && (interupted == 0))
                {
                    TANGO_LOG_DEBUG << "TIME OUT" << std::endl;
                    TANGO_THROW_EXCEPTION(API_CommandTimedOut, "Device interface change event thread blocked !!!");
                }
            }
        }
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::end_pipe_config
//
// description :
//        Get all pipe properties defined at device level and aggregate all the pipe properties defined at different
//        level (device, user default, class default
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::end_pipe_config()
{
    TANGO_LOG_DEBUG << "Entering end_pipe_config for device " << device_name << std::endl;

    std::vector<Pipe *> &pipe_list = device_class->get_pipe_list(device_name_lower);
    size_t nb_pipe = pipe_list.size();

    //
    // First get device pipe configuration from db
    //

    TANGO_LOG_DEBUG << nb_pipe << " pipe(s)" << std::endl;

    if(nb_pipe != 0)
    {
        Tango::Util *tg = Tango::Util::instance();
        Tango::DbData db_list;

        if(tg->use_db())
        {
            for(size_t i = 0; i < nb_pipe; i++)
            {
                db_list.emplace_back(pipe_list[i]->get_name());
            }

            //
            // On some small and old computers, this request could take time if at the same time some other processes
            // also access the device pipe properties table. This has been experimented at ESRF. Increase timeout to
            // cover this case
            //

            int old_db_timeout = 0;
            if(!tg->use_file_db())
            {
                old_db_timeout = tg->get_database()->get_timeout_millis();
            }
            try
            {
                if(old_db_timeout != 0)
                {
                    tg->get_database()->set_timeout_millis(6000);
                }
                tg->get_database()->get_device_pipe_property(device_name, db_list, tg->get_db_cache());
                if(old_db_timeout != 0)
                {
                    tg->get_database()->set_timeout_millis(old_db_timeout);
                }
            }
            catch(Tango::DevFailed &)
            {
                TANGO_LOG_DEBUG << "Exception while accessing database" << std::endl;

                tg->get_database()->set_timeout_millis(old_db_timeout);
                std::stringstream ss;
                ss << "Can't get device pipe properties for device " << device_name << std::ends;

                TANGO_THROW_EXCEPTION(API_DatabaseAccess, ss.str());
            }

            //
            // A loop for each pipe
            //

            long ind = 0;
            for(size_t i = 0; i < nb_pipe; i++)
            {
                //
                // If pipe has some properties defined at device level, build a vector of PipeProperty with them
                //

                long nb_prop = 0;
                std::vector<PipeProperty> dev_prop;

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
                        dev_prop.emplace_back(db_list[ind].name, tmp);
                    }
                    else
                    {
                        dev_prop.emplace_back(db_list[ind].name, db_list[ind].value_string[0]);
                    }
                    ind++;
                }

                Pipe *pi_ptr = pipe_list[i];

                //
                // Call method which will aggregate prop definition retrieved at different levels
                //

                set_pipe_prop(dev_prop, pi_ptr, LABEL);
                set_pipe_prop(dev_prop, pi_ptr, DESCRIPTION);
            }
        }
    }

    TANGO_LOG_DEBUG << "Leaving end_pipe_config for device " << device_name << std::endl;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        DeviceImpl::set_pipe_prop
//
// description :
//        Set a pipe property. A pipe property can be defined at device level in DB, by a user default or at
//        class level in DB. Properties defined in DB at device level are given to the method as the first
//        parameter. If the user has defined some user default, the pipe object already has them. The properties
//        defined at class level are available in the MultiClassPipe object available in the DeviceClass instance
//
// argument :
//        in:
//            - dev_prop : Pipe properties defined at device level
//            - pi_ptr : Pipe instance pointer
//            - ppt : Property type (label or description)
//
//------------------------------------------------------------------------------------------------------------------

void DeviceImpl::set_pipe_prop(std::vector<PipeProperty> &dev_prop, Pipe *pi_ptr, PipePropType ppt)
{
    TANGO_LOG_DEBUG << "Entering set_pipe_prop() method" << std::endl;
    //
    // Final init of pipe prop with following priorities:
    // - Device pipe
    // - User default
    // - Class pipe
    // The pipe instance we have here already has config set to user default (if any)
    //

    bool found = false;
    std::string req_p_name;
    if(ppt == LABEL)
    {
        req_p_name = "label";
    }
    else
    {
        req_p_name = "description";
    }

    std::vector<PipeProperty>::iterator dev_ite;
    for(dev_ite = dev_prop.begin(); dev_ite != dev_prop.end(); ++dev_ite)
    {
        std::string p_name = dev_ite->get_name();
        std::transform(p_name.begin(), p_name.end(), p_name.begin(), ::tolower);

        if(p_name == req_p_name)
        {
            found = true;
            break;
        }
    }

    if(found)
    {
        if(ppt == LABEL)
        {
            pi_ptr->set_label(dev_ite->get_value());
        }
        else
        {
            pi_ptr->set_desc(dev_ite->get_value());
        }
    }
    else
    {
        //
        // Prop not defined at device level. If the prop is still the lib default one, search if it is defined at class
        // level
        //

        bool still_default;
        if(ppt == LABEL)
        {
            still_default = pi_ptr->is_label_lib_default();
        }
        else
        {
            still_default = pi_ptr->is_desc_lib_default();
        }

        if(still_default)
        {
            try
            {
                std::vector<PipeProperty> &cl_pi_prop =
                    device_class->get_class_pipe()->get_prop_list(pi_ptr->get_name());

                bool found = false;
                std::vector<PipeProperty>::iterator class_ite;
                for(class_ite = cl_pi_prop.begin(); class_ite != cl_pi_prop.end(); ++class_ite)
                {
                    std::string p_name = class_ite->get_name();
                    std::transform(p_name.begin(), p_name.end(), p_name.begin(), ::tolower);

                    if(p_name == req_p_name)
                    {
                        found = true;
                        break;
                    }
                }

                if(found)
                {
                    if(ppt == LABEL)
                    {
                        pi_ptr->set_label(class_ite->get_value());
                    }
                    else
                    {
                        pi_ptr->set_desc(class_ite->get_value());
                    }
                }
            }
            catch(Tango::DevFailed &)
            {
            }
        }
    }

    TANGO_LOG_DEBUG << "Leaving set_pipe_prop() method" << std::endl;
}

PipeEventSubscriptionStates DeviceImpl::get_pipe_event_subscription_states()
{
    PipeEventSubscriptionStates result{};

    try
    {
        for(const auto &pipe : device_class->get_pipe_list(device_name_lower))
        {
            if(pipe->is_pipe_event_subscribed())
            {
                PipeEventSubscriptionState events{};
                events.pipe_name = pipe->get_name();
                events.has_pipe_event_clients = true;
                result.push_back(std::move(events));
            }
        }
    }
    catch(const DevFailed &)
    {
        // No pipes for this device, sliently ignore
    }

    return result;
}

void DeviceImpl::set_pipe_event_subscription_states(const PipeEventSubscriptionStates &events)
{
    for(const auto &pipe_events : events)
    {
        auto &pipe = device_class->get_pipe_by_name(pipe_events.pipe_name, device_name_lower);
        if(pipe_events.has_pipe_event_clients)
        {
            const auto now = Tango::get_current_system_datetime();
            pipe.set_event_subscription(now);
        }
    }
}

DbDevice *DeviceImpl::get_db_device()
{
    if(!Tango::Util::instance()->use_db())
    {
        TangoSys_OMemStream desc_mess;
        desc_mess << "Method not available for device ";
        desc_mess << device_name;
        desc_mess << " which is a non database device";

        TANGO_THROW_EXCEPTION(API_NonDatabaseDevice, desc_mess.str());
    }

    return db_dev;
}

} // namespace Tango
