//+=============================================================================
//
// file :               subdev_diag.cpp
//
// description :        Collect information on all used sub devices
//                      in a device server.
//
// project :            TANGO
//
// author(s) :          J.Meyer
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
//-=============================================================================

#include <tango/server/subdev_diag.h>
#include <tango/server/dserver.h>
#include <tango/server/device.h>

#include <tango/client/Database.h>

namespace Tango
{

namespace
{
thread_local std::string thread_local_device_name = "No associated device name!";
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::~SubDevDiag()
//
// description :    Destructor to free the map data
//
//-----------------------------------------------------------------------------

SubDevDiag::~SubDevDiag()
{
    TANGO_LOG_DEBUG << "SubDevDiag::~SubDevDiag() entering ... " << std::endl;

    // lock the sub device map
    omni_mutex_lock l(sub_dev_map_mutex);

    // remove all sub devices
    sub_device_map.clear();
    sub_device_startup_map.clear();
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::set_associated_device()
//
// description :    Set the device name that should be asscociated
//                  to a thread in the device server
//
// in :             dev_name - device name
//
//-----------------------------------------------------------------------------

void SubDevDiag::set_associated_device(std::string dev_name)
{
    TANGO_LOG_DEBUG << "SubDevDiag::set_associated_device() entering ... ";
    // Setting a subdevice name is only allowed from the library threads.
    if(is_tango_library_thread)
    {
        thread_local_device_name = std::move(dev_name);
    }
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::get_associated_device()
//
// description :    Get the device name that is asscociated
//                  with the current thread of the device server
//
// return :         associated device name
//
//-----------------------------------------------------------------------------

std::string SubDevDiag::get_associated_device()
{
    TANGO_LOG_DEBUG << "SubDevDiag::get_associated_device() entering ... " << std::endl;
    std::string dev_name{};
    if(is_tango_library_thread)
    {
        dev_name = thread_local_device_name;
    }
    TANGO_LOG_DEBUG << "SubDevDiag::get_associated_device() found : " << dev_name << std::endl;
    return dev_name;
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::register_sub_device()
//
// description :    Register a sub device for an associated device
//                  in the list of sub devices of the device server
//
// in :             dev_name = associated device name
//                  sub_dev_name = sub device name
//
//-----------------------------------------------------------------------------

void SubDevDiag::register_sub_device(std::string dev_name, std::string sub_dev_name)
{
    TANGO_LOG_DEBUG << "SubDevDiag::register_sub_device() dev_name = " << dev_name << " sub_dev_name = " << sub_dev_name
                    << std::endl;

    bool found = false;

    // be sure that all names are lower case letters
    std::transform(dev_name.begin(), dev_name.end(), dev_name.begin(), ::tolower);
    std::transform(sub_dev_name.begin(), sub_dev_name.end(), sub_dev_name.begin(), ::tolower);

    // lock the sub device map
    omni_mutex_lock l(sub_dev_map_mutex);

    // Find whether a sub device list for the device is already available
    std::map<std::string, SubDeviceList>::iterator ipos;
    ipos = sub_device_map.find(dev_name);

    if(ipos == sub_device_map.end())
    {
        // device not known, add a new sub device
        sub_device_map[dev_name].sub_devices.push_back(sub_dev_name);
        sub_device_map[dev_name].modified = true;
    }
    else
    {
        // Check whether the sub device name is alreay in the list
        for(unsigned int i = 0; i < ipos->second.sub_devices.size(); i++)
        {
            if(ipos->second.sub_devices[i] == sub_dev_name)
            {
                // Name is already in the list
                found = true;
                break;
            }
        }
        if(!found)
        {
            // name is not in the list, add the sub device
            ipos->second.sub_devices.push_back(sub_dev_name);
            ipos->second.modified = true;
        }
    }
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::remove_sub_devices()
//
// description :    Remove all sub devices for a device of the server
//
// in :             dev_name = device name
//
//-----------------------------------------------------------------------------

void SubDevDiag::remove_sub_devices(std::string dev_name)
{
    TANGO_LOG_DEBUG << "SubDevDiag::remove_sub_device() dev_name = " << dev_name << std::endl;

    // be sure that all names are lower case letters
    std::transform(dev_name.begin(), dev_name.end(), dev_name.begin(), ::tolower);

    // lock the sub device map
    omni_mutex_lock l(sub_dev_map_mutex);

    // remove the list of sub devices for a device
    std::map<std::string, SubDeviceList>::iterator ipos;
    ipos = sub_device_map.find(dev_name);

    if(ipos != sub_device_map.end())
    {
        sub_device_map.erase(ipos);
    }
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::remove_sub_devices()
//
// description :    Remove all sub devices of the server
//
// in :             dev_name = device name
//
//-----------------------------------------------------------------------------

void SubDevDiag::remove_sub_devices()
{
    TANGO_LOG_DEBUG << "SubDevDiag::remove_sub_devices() remove ALL " << std::endl;

    // lock the sub device map
    omni_mutex_lock l(sub_dev_map_mutex);
    // remove all sub devices
    sub_device_map.clear();
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::get_sub_devices()
//
// description :    Read the list of sub devices for the device server
//                  The returned strings are formated as:
//                    "device_name sub_device_name"
//                  or
//                    sub_device_name
//                  when no associated device could be identified.
//
// return :         An array of formated strings
//
//-----------------------------------------------------------------------------

Tango::DevVarStringArray *SubDevDiag::get_sub_devices()
{
    TANGO_LOG_DEBUG << "SubDevDiag::get_sub_devices() entering ... " << std::endl;

    Tango::DevVarStringArray *ret;
    std::vector<std::string> sub_dev_list;
    std::string tmp;

    // lock the sub device map
    omni_mutex_lock l(sub_dev_map_mutex);

    try
    {
        std::map<std::string, SubDeviceList>::iterator ipos;
        for(ipos = sub_device_map.begin(); ipos != sub_device_map.end(); ++ipos)
        {
            for(unsigned int i = 0; i < ipos->second.sub_devices.size(); i++)
            {
                if(ipos->first.empty())
                {
                    tmp = ipos->second.sub_devices[i];
                }
                else
                {
                    tmp = ipos->first + " " + ipos->second.sub_devices[i];
                }

                sub_dev_list.push_back(tmp);
            }
        }

        ret = new Tango::DevVarStringArray(DefaultMaxSeq);
        ret->length(sub_dev_list.size());
        for(unsigned int k = 0; k < sub_dev_list.size(); k++)
        {
            (*ret)[k] = Tango::string_dup(sub_dev_list[k].c_str());
        }

        return (ret);
    }

    catch(std::bad_alloc &)
    {
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }

    // Should never reach here. To make compiler happy

    ret = nullptr;
    return ret;
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::store_sub_devices()
//
// description :    Store the list of sub devices for the devices of
//                  the server.
//                  The sub device names are stored as a string array
//                  under the device property "sub_devices".
//                  Sub device names without an associated device,
//                  will be stored under the name of the administration
//                  device.
//
//                  Database access will only happen when the list of
//                  sub devices was modified and when the list is different
//                  from the list read into the db_cache during the server
//                  startup.
//
//-----------------------------------------------------------------------------

void SubDevDiag::store_sub_devices()
{
    TANGO_LOG_DEBUG << "SubDevDiag::store_sub_devices() entering ... " << std::endl;

    Tango::Util *tg = Tango::Util::instance();

    // lock the sub device map
    omni_mutex_lock l(sub_dev_map_mutex);

    // loop over the sub device map

    std::map<std::string, SubDeviceList>::iterator ipos;
    for(ipos = sub_device_map.begin(); ipos != sub_device_map.end(); ++ipos)
    {
        // Check whether the list was modified
        if(ipos->second.modified)
        {
            // Check whether for modifications compared to
            // the list read into db_cache during startup

            // check the number of sub devices
            if(ipos->second.sub_devices.size() == sub_device_startup_map[ipos->first].sub_devices.size())
            {
                // find sub device names in the start-up list
                bool is_equal = true;
                for(unsigned int i = 0; i < ipos->second.sub_devices.size(); i++)
                {
                    bool found = false;
                    for(unsigned int k = 0; k < sub_device_startup_map[ipos->first].sub_devices.size(); k++)
                    {
                        if(ipos->second.sub_devices[i] == sub_device_startup_map[ipos->first].sub_devices[k])
                        {
                            found = true;
                            break;
                        }
                    }

                    if(!found)
                    {
                        is_equal = false;
                        break;
                    }
                }

                if(is_equal)
                {
                    // sub device names are equal to the names
                    // read from the database at server start-up.
                    // Clear the modification flag
                    ipos->second.modified = false;

                    continue;
                }
            }

            // write the sub device list as device property
            try
            {
                DbDatum list("__SubDevices");
                DbData db_data;
                list << ipos->second.sub_devices;
                db_data.push_back(list);

                // Check for a valid database object.
                // In the database server itself or any server
                // running without a database the database object is
                // not initialised.
                if(Tango::Util::instance()->use_db())
                {
                    if(ipos->first.empty())
                    {
                        DServer *adm_dev = tg->get_dserver_device();
                        tg->get_database()->put_device_property(adm_dev->get_name(), db_data);
                    }
                    else
                    {
                        tg->get_database()->put_device_property(ipos->first, db_data);
                    }
                }

                // clear the modification flag
                ipos->second.modified = false;
            }
            catch(Tango::DevFailed &)
            {
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// method :         SubDevDiag::get_sub_devices_from_cache()
//
// description :    Read the list of sub devices from the
//                  database cache. The cache is filled at
//                  server sart-up.
//
//-----------------------------------------------------------------------------
void SubDevDiag::get_sub_devices_from_cache()
{
    TANGO_LOG_DEBUG << "SubDevDiag::get_sub_devices_from_cache() entering ... " << std::endl;

    std::shared_ptr<DbServerCache> db_cache;
    Tango::Util *tg = Tango::Util::instance();

    try
    {
        db_cache = tg->get_db_cache();
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        db_cache.reset();
    }

    if(db_cache)
    {
        // get the name of the admin device
        DServer *adm_dev = tg->get_dserver_device();
        std::string adm_name = adm_dev->get_name();
        // be sure that all names are lower case letters
        std::transform(adm_name.begin(), adm_name.end(), adm_name.begin(), ::tolower);

        // get all devices served
        std::vector<DeviceImpl *> dev_list = tg->get_device_list("*");

        for(unsigned int k = 0; k < dev_list.size(); k++)
        {
            std::string dev_name = dev_list[k]->get_name();
            // be sure that all names are lower case letters
            std::transform(dev_name.begin(), dev_name.end(), dev_name.begin(), ::tolower);

            DevVarStringArray *property_names = new DevVarStringArray;
            property_names->length(2);
            (*property_names)[0] = string_dup(dev_name.c_str());
            (*property_names)[1] = string_dup("__SubDevices");

            try
            {
                const DevVarStringArray *property_values = db_cache->get_dev_property(property_names);
                if(atol((*property_values)[3]) > 0)
                {
                    // if the device is the admin device, set dev_name to ""
                    // to have the same syntax as in the dynamically created
                    // sub device map.

                    if(dev_name == adm_name)
                    {
                        dev_name = "";
                    }

                    for(unsigned int i = 4; i < property_values->length(); i++)
                    {
                        sub_device_startup_map[dev_name].sub_devices.emplace_back((*property_values)[i].in());
                    }
                }
            }

            catch(Tango::DevFailed &)
            {
                std::cerr << "Sub device not found in DB cache for " << dev_name << std::endl;
            }

            delete property_names;
        }
    }
    else
    {
        std::cerr << "No database cache found to initialise sub device map!" << std::endl;
    }
}

} // namespace Tango
