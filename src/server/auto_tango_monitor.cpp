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

#include <tango/server/auto_tango_monitor.h>
#include <tango/server/tango_monitor.h>
#include <tango/server/device.h>
#include <tango/server/utils.h>

namespace Tango
{

//---------------------------------------------------------------------------------------------------------------
//
// class :
//        AutoTangoMonitor
//
// description :
//        This class is only a helper class used to get a TangoMonitor object during its construction and to
//        it during its destruction. It makes developer life easier.
//
//---------------------------------------------------------------------------------------------------------------
AutoTangoMonitor::AutoTangoMonitor(Tango::DeviceImpl *dev, bool force)
{
    SerialModel ser = Util::instance()->get_serial_model();

    switch(ser)
    {
    case NO_SYNC:
        if(force)
        {
            mon = &(dev->only_one);
        }
        else
        {
            mon = nullptr;
        }
        break;

    case BY_DEVICE:
        mon = &(dev->only_one);
        break;

    case BY_CLASS:
        mon = &(dev->device_class->only_one);
        break;

    case BY_PROCESS:
        mon = &(Util::instance()->only_one);
        break;
    }
    if(mon != nullptr)
    {
        mon->get_monitor();
    }
}

AutoTangoMonitor::AutoTangoMonitor(Tango::DeviceClass *dev_cl)
{
    SerialModel ser = Util::instance()->get_serial_model();

    switch(ser)
    {
    case NO_SYNC:
    case BY_DEVICE:
        mon = nullptr;
        break;

    case BY_CLASS:
        mon = &(dev_cl->only_one);
        mon->get_monitor();
        break;

    case BY_PROCESS:
        mon = &(Util::instance()->only_one);
        mon->get_monitor();
        break;
    }
}

AutoTangoMonitor::AutoTangoMonitor(Tango::TangoMonitor *m) :
    mon(m)
{
    if(mon != nullptr)
    {
        mon->get_monitor();
    }
}

AutoTangoMonitor::~AutoTangoMonitor()
{
    if(mon != nullptr)
    {
        mon->rel_monitor();
    }
}

AutoTangoMonitor::AutoTangoMonitor(const std::shared_ptr<Tango::TangoMonitor> &m) :
    AutoTangoMonitor(m.get())
{
}

//---------------------------------------------------------------------------------------------------------------
//
// class :
//        NoSyncModelTangoMonitor
//
// description :
//        This class is only a helper class used to get a TangoMonitor object during its construction and to
//        it during its destruction only if the device server process is in NO_SYNC synchronisation model
//
//---------------------------------------------------------------------------------------------------------------
NoSyncModelTangoMonitor::NoSyncModelTangoMonitor(Tango::DeviceImpl *dev)
{
    SerialModel ser = Util::instance()->get_serial_model();
    if(ser == NO_SYNC)
    {
        mon = &(dev->only_one);
        mon->get_monitor();
    }
    else
    {
        mon = nullptr;
    }
}

NoSyncModelTangoMonitor::~NoSyncModelTangoMonitor()
{
    if(mon != nullptr)
    {
        mon->rel_monitor();
    }
}

} // namespace Tango
