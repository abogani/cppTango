//===================================================================================================================
//
// file :               utils_spec_templ.h
//
// description :        C++ source code for the Util::fill_cmd_polling_buffer method template specialization
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
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation, either version 3 of the License, or
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
//==================================================================================================================

#ifndef _UTILS_SPEC_TPP
#define _UTILS_SPEC_TPP

#include <tango/server/tango_clock.h>

namespace Tango
{

//+---------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::fill_cmd_polling_buffer
//
// description :
//        Fill attribute polling buffer with your own data
//        These are template specialization for data type
//                DevBoolean
//                DevUChar
//        This is required because the insertion of these data type in a CORBA Any object requires specific code
//
// args :
//        in :
//             - dev : The device
//            - cmd_name : The command name
//            - data : The command data to be stored in the polling buffer
//
//----------------------------------------------------------------------------------------------------------------

template <>
inline void Util::fill_cmd_polling_buffer(DeviceImpl *dev, std::string &cmd_name, CmdHistoryStack<DevBoolean> &data)
{
    //
    // Check that the device is polled
    //

    if(!dev->is_polled())
    {
        TangoSys_OMemStream o;
        o << "Device " << dev->get_name() << " is not polled" << std::ends;

        TANGO_THROW_EXCEPTION(API_DeviceNotPolled, o.str());
    }

    //
    // Command name in lower case letters and check that it is marked as polled
    //

    std::string obj_name(cmd_name);
    std::transform(obj_name.begin(), obj_name.end(), obj_name.begin(), ::tolower);

    dev->get_polled_obj_by_type_name(Tango::POLL_CMD, obj_name);

    //
    // Check that history is not larger than polling buffer
    //

    size_t nb_elt = data.length();
    long nb_poll = dev->get_cmd_poll_ring_depth(cmd_name);

    if(nb_elt > (size_t) nb_poll)
    {
        TangoSys_OMemStream o;
        o << "The polling buffer depth for command " << cmd_name;
        o << " for device " << dev->get_name();
        o << " is only " << nb_poll;
        o << " which is less than " << nb_elt << "!" << std::ends;

        TANGO_THROW_EXCEPTION(API_DeviceNotPolled, o.str());
    }

    //
    // Take the device monitor before the loop
    // In case of large element number, it is time cousuming to take/release
    // the monitor in the loop
    //

    dev->get_poll_monitor().get_monitor();

    //
    // A loop on each record
    //

    size_t i;
    Tango::DevFailed *save_except = nullptr;
    bool cmd_failed;
    CORBA::Any *any_ptr = nullptr;

    for(i = 0; i < nb_elt; i++)
    {
        save_except = nullptr;
        cmd_failed = false;

        if((data.get_data())[i].err.length() != 0)
        {
            cmd_failed = true;
            try
            {
                save_except = new Tango::DevFailed((data.get_data())[i].err);
            }
            catch(std::bad_alloc &)
            {
                dev->get_poll_monitor().rel_monitor();
                TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
            }
        }
        else
        {
            //
            // Allocate memory for the Any object
            //

            try
            {
                any_ptr = new CORBA::Any();
            }
            catch(std::bad_alloc &)
            {
                dev->get_poll_monitor().rel_monitor();
                TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
            }

            //
            // Set command value in Any object
            //

            DevBoolean *tmp_ptr = (data.get_data())[i].ptr;
            CORBA::Any::from_boolean tmp(*tmp_ptr);
            (*any_ptr) <<= tmp;

            if((data.get_data())[i].release)
            {
                delete tmp_ptr;
            }
        }

        //
        // Fill one slot of polling buffer
        //

        try
        {
            auto ite = dev->get_polled_obj_by_type_name(Tango::POLL_CMD, obj_name);
            auto when = make_poll_time(data.get_data()[i].tp);
            auto zero = PollClock::duration::zero();
            if(!cmd_failed)
            {
                (*ite)->insert_data(any_ptr, when, zero);
            }
            else
            {
                (*ite)->insert_except(save_except, when, zero);
            }
        }
        catch(Tango::DevFailed &)
        {
            if(!cmd_failed)
            {
                delete any_ptr;
            }
            else
            {
                delete save_except;
            }
        }
    }

    dev->get_poll_monitor().rel_monitor();
}

template <>
inline void Util::fill_cmd_polling_buffer(DeviceImpl *dev, std::string &cmd_name, CmdHistoryStack<DevUChar> &data)
{
    //
    // Check that the device is polled
    //

    if(!dev->is_polled())
    {
        TangoSys_OMemStream o;
        o << "Device " << dev->get_name() << " is not polled" << std::ends;

        TANGO_THROW_EXCEPTION(API_DeviceNotPolled, o.str());
    }

    //
    // Command name in lower case letters and check that it is marked as polled
    //

    std::string obj_name(cmd_name);
    std::transform(obj_name.begin(), obj_name.end(), obj_name.begin(), ::tolower);

    dev->get_polled_obj_by_type_name(Tango::POLL_CMD, obj_name);

    //
    // Check that history is not larger than polling buffer
    //

    size_t nb_elt = data.length();
    long nb_poll = dev->get_cmd_poll_ring_depth(cmd_name);

    if(nb_elt > (size_t) nb_poll)
    {
        TangoSys_OMemStream o;
        o << "The polling buffer depth for command " << cmd_name;
        o << " for device " << dev->get_name();
        o << " is only " << nb_poll;
        o << " which is less than " << nb_elt << "!" << std::ends;

        TANGO_THROW_EXCEPTION(API_DeviceNotPolled, o.str());
    }

    //
    // Take the device monitor before the loop
    // In case of large element number, it is time cousuming to take/release
    // the monitor in the loop
    //

    dev->get_poll_monitor().get_monitor();

    //
    // A loop on each record
    //

    size_t i;
    Tango::DevFailed *save_except = nullptr;
    bool cmd_failed;
    CORBA::Any *any_ptr = nullptr;

    for(i = 0; i < nb_elt; i++)
    {
        save_except = nullptr;
        cmd_failed = false;

        if((data.get_data())[i].err.length() != 0)
        {
            cmd_failed = true;
            try
            {
                save_except = new Tango::DevFailed((data.get_data())[i].err);
            }
            catch(std::bad_alloc &)
            {
                dev->get_poll_monitor().rel_monitor();
                TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
            }
        }
        else
        {
            //
            // Allocate memory for the Any object
            //

            try
            {
                any_ptr = new CORBA::Any();
            }
            catch(std::bad_alloc &)
            {
                dev->get_poll_monitor().rel_monitor();
                TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
            }

            //
            // Set command value in Any object
            //

            DevUChar *tmp_ptr = (data.get_data())[i].ptr;
            CORBA::Any::from_octet tmp(*tmp_ptr);
            (*any_ptr) <<= tmp;

            if((data.get_data())[i].release)
            {
                delete tmp_ptr;
            }
        }

        //
        // Fill one slot of polling buffer
        //

        try
        {
            auto ite = dev->get_polled_obj_by_type_name(Tango::POLL_CMD, obj_name);
            auto when = make_poll_time(data.get_data()[i].tp);
            auto zero = PollClock::duration::zero();
            if(!cmd_failed)
            {
                (*ite)->insert_data(any_ptr, when, zero);
            }
            else
            {
                (*ite)->insert_except(save_except, when, zero);
            }
        }
        catch(Tango::DevFailed &)
        {
            if(!cmd_failed)
            {
                delete any_ptr;
            }
            else
            {
                delete save_except;
            }
        }
    }

    dev->get_poll_monitor().rel_monitor();
}

} // namespace Tango

#endif /* UTILS_SPEC_TPP */
