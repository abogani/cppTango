//====================================================================================================================
//
// file :               Tango.h
//
// description :        Main include for Tango device server
//
// project :            TANGO
//
// author(s) :          A.Gotz + E.Taurel
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//===================================================================================================================

#ifndef _TANGO_SERVER_H
#define _TANGO_SERVER_H

//
// Include the Tango config file
//
#include <tango/server/tango_config.h>

//
// Include IDL generated files which includes CORBA include files
//

#if defined(_TG_WINDOWS_) && defined(TANGO_HAS_DLL) && !defined(_TANGO_LIB)
  #define USE_stub_in_nt_dll
#endif

// FIXME remove once https://gitlab.com/tango-controls/cppTango/-/issues/786 is fixed
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdeprecated"
#endif

#include <tango/idl/tango.h>

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#include <tango/server/tango_current_function.h>

//
// Some Windows specific include (necessary when used with MFC)
//

#ifdef _TG_WINDOWS_
  #if (_WIN32_WINNT >= 0x0400)
    #include <winsock2.h>
    #include <mswsock.h>
    #include <algorithm>
    #include <limits>
  #else
    #include <winsock.h>
  #endif
#endif

//
// Include API files (device and database)
//

#include <tango/client/apiexcept.h>
#include <tango/client/devasyn.h>
#include <tango/client/Database.h>
#include <tango/client/AttributeProxy.h>
#include <tango/client/dbapi.h>
#include <tango/client/devapi.h>
#include <tango/client/group.h>
#include <tango/client/filedatabase.h>
#include <tango/client/devapi_attr_templ.h>

//
// Include Tango utility files
//

#include <tango/common/tango_const.h>
#include <tango/common/tango_type_traits.h>
#include <tango/common/utils/assert.h>
#include <tango/server/except.h>
#include <tango/server/attrmanip.h>
#include <tango/server/seqvec.h>

#if !defined(TANGO_CLIENT)
  #include <tango/server/log4tango.h>
#endif

//
// Telemetry for both clients & servers
//
#include <tango/common/telemetry/telemetry.h>

//
// Include Tango files in order to simplfy device server developer include
// file list
//

#ifndef TANGO_CLIENT
  #include <tango/server/attrprop.h>
  #include <tango/server/tango_monitor.h>
  #include <tango/server/device.h>
  #include <tango/server/utils.h>
  #include <tango/server/auto_tango_monitor.h>
  #include <tango/server/device_2.h>
  #include <tango/server/device_3.h>
  #include <tango/server/device_4.h>
  #include <tango/server/device_5.h>
  #include <tango/server/device_6.h>
  #include <tango/server/command.h>
  #include <tango/server/pipedesc.h>
  #include <tango/server/pipe.h>
  #include <tango/server/w_pipe.h>
  #include <tango/server/pipe_templ.h>
  #include <tango/server/dserver.h>
  #include <tango/server/utils_spec_templ.h>
  #include <tango/server/attrprop_templ.h>
  #include <tango/server/attrsetval_templ.h>
  #include <tango/server/w_attrsetval_templ.h>
  #include <tango/server/fwdattribute.h>
#endif

#include <tango/client/event.h>

//
// minor is also defined (Linux) in sysmacros.h. We want the compiler to
// use the SystemException::minor() method !!
//

#ifdef minor
  #undef minor
#endif

#endif /* TANGO_SERVER_H */
