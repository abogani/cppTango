//+===================================================================================================================
//
// file :               Device_6.cpp
//
// description :        C++ source code for the Device_6Impl class. This class is the root class for all derived Device
//						classes starting with Tango 9
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

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wsometimes-uninitialized"
  #pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include <tango/tango.h>

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif

#include <tango/server/device_6.h>
#include <tango/server/eventsupplier.h>
#include <tango/server/device_3_templ.h>
#include <tango/common/telemetry/telemetry.h>

namespace Tango
{

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//		Device_6Impl::Device_6Impl
//
// description :
//		Constructors for the Device_6impl class from the class object pointer, the device name, the description
// field,
// 		the state and the status. Device_6Impl inherits from DeviceImpl. These constructors simply call the
// correct
//		DeviceImpl class constructor
//
//--------------------------------------------------------------------------------------------------------------------

Device_6Impl::Device_6Impl(DeviceClass *device_class, const std::string &dev_name) :
    Device_5Impl(device_class, dev_name),
    ext_6(nullptr)
{
    idl_version = 6;
}

Device_6Impl::Device_6Impl(DeviceClass *device_class, const std::string &dev_name, const std::string &desc) :
    Device_5Impl(device_class, dev_name, desc),
    ext_6(nullptr)
{
    idl_version = 6;
}

Device_6Impl::Device_6Impl(DeviceClass *device_class,
                           const std::string &dev_name,
                           const std::string &desc,
                           Tango::DevState dev_state,
                           const std::string &dev_status) :
    Device_5Impl(device_class, dev_name, desc, dev_state, dev_status),
    ext_6(nullptr)
{
    idl_version = 6;
}

Device_6Impl::Device_6Impl(DeviceClass *device_class,
                           const char *dev_name,
                           const char *desc,
                           Tango::DevState dev_state,
                           const char *dev_status) :
    Device_5Impl(device_class, dev_name, desc, dev_state, dev_status),
    ext_6(nullptr)
{
    idl_version = 6;
}

//+--------------------------------------------------------------------------------------------------------------------
//
// method :
//		Device_6Impl::info_6
//
// description :
//		CORBA operation to get device info
//
//--------------------------------------------------------------------------------------------------------------------

Tango::DevInfo_6 *Device_6Impl::info_6()
{
    TANGO_LOG_DEBUG << "Device_6Impl::info_6 arrived" << std::endl;

    Tango::DevInfo_6 *back = nullptr;
    //
    // Allocate memory for the stucture sent back to caller. The ORB will free it
    //

    try
    {
        back = new Tango::DevInfo_6;
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
    // Get current libraries version_info list
    //

    back->version_info = get_version_info();

    //
    // Record operation request in black box
    //

    blackbox_ptr->insert_op(Op_Info_6);

    //
    // Return to caller
    //

    TANGO_LOG_DEBUG << "Leaving Device_6Impl::info_6" << std::endl;
    return back;
}

} // namespace Tango
