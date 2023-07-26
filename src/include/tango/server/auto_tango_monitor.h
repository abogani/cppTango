//=====================================================================================================================
//
// file :               auto_tango_monitor.h
//
// description :        Include file for two utility classes related to monitor
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

#ifndef _AUTO_TANGO_MONITOR_H
#define _AUTO_TANGO_MONITOR_H

namespace Tango
{

//-------------------------------------------------------------------------------------------------------------------
//
// class :
//		AutoTangoMonitor
//
// description :
//		This class is only a helper class used to get a TangoMonitor object during its construction and to
//		it during its destruction. It makes developer life easier.
//
//-------------------------------------------------------------------------------------------------------------------

class AutoTangoMonitor
{
public:
	AutoTangoMonitor(Tango::DeviceImpl *dev, bool force = false);

	AutoTangoMonitor(Tango::DeviceClass *dev_cl);

	AutoTangoMonitor(Tango::TangoMonitor *m);

	~AutoTangoMonitor();

private:
	TangoMonitor 				*mon;
	omni_thread::ensure_self	auto_self;
};

//---------------------------------------------------------------------------------------------------------------------
//
// class :
//		NoSyncModelTangoMonitor
//
// description :
//		This class is only a helper class used to get a TangoMonitor object during its construction and to
//		it during its destruction only if the device server process is in NO_SYNC synchronisation model
//
//--------------------------------------------------------------------------------------------------------------------

class NoSyncModelTangoMonitor
{
public:
	NoSyncModelTangoMonitor(Tango::DeviceImpl *dev);

	~NoSyncModelTangoMonitor();

private:
	TangoMonitor 			*mon;
	omni_thread::ensure_self	auto_self;
};


} // End of Tango namespace

#endif /* AUTO_TANGO_MONITOR */
