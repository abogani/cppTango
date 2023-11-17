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

} // namespace Tango
