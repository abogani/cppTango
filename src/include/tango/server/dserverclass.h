//=============================================================================
//
// file :               DServerClass.h
//
// description :        Include for the DServerClass class. This class is a
//                      singleton class i.e only one object of this class
//            can be created.
//            It contains all properties and methods
//            which the DServer requires only once e.g. the
//            commands.
//            This file also includes class declaration for all the
//            commands available on device of the DServer class
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
//=============================================================================

#ifndef _DSERVERCLASS_H
#define _DSERVERCLASS_H

#include <tango/tango.h>

namespace Tango
{

//=============================================================================
//
//            The DevRestart class
//
// description :    Class to implement the DevRestart command. This command
//            needs one input argument and no outout argument.
//            The input argument is the name of the device to be
//            re-started.
//            This class delete and re-create a device
//
//=============================================================================

class DevRestartCmd : public Command
{
  public:
    DevRestartCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevRestartCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevRestartServerCmd class
//
// description :    Class to implement the DevKill command. This
//            command does not take any input argument. It simply
//            kills the device server.
//
//=============================================================================

class DevRestartServerCmd : public Command
{
  public:
    DevRestartServerCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out);

    ~DevRestartServerCmd() override { }

    CORBA::Any *execute(DeviceImpl *, const CORBA::Any &) override;
};

//=============================================================================
//
//            The DevQueryClassCmd class
//
// description :    Class to implement the DevQueryClass command. This
//            command does not take any input argument and return a
//            list of all the classes created inside the device
//            server process
//
//=============================================================================

class DevQueryClassCmd : public Command
{
  public:
    DevQueryClassCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevQueryClassCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevQueryDeviceCmd class
//
// description :    Class to implement the DevQueryDevice command. This
//            command does not take any input argument and return a
//            list of all the devices created inside the device
//            server process
//
//=============================================================================

class DevQueryDeviceCmd : public Command
{
  public:
    DevQueryDeviceCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevQueryDeviceCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevQuerySubDeviceCmd class
//
// description :    Class to implement the DevQuerySubDevice command. This
//            command does not take any input argument and returns a
//            list of all the sub devices connections opened inside the device
//            server process
//
//=============================================================================

class DevQuerySubDeviceCmd : public Command
{
  public:
    DevQuerySubDeviceCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevQuerySubDeviceCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevQueryEventSystemCmd class
//
// description :    Class to implement the DevQueryEventSystem command. This
//            command does not take any input argument and returns a
//            single string containing a JSON object holding information about
//            the ZMQ event supplier and consumer.
//
//=============================================================================

class DevQueryEventSystemCmd : public Command
{
  public:
    DevQueryEventSystemCmd(const char *cmd_name, Tango::CmdArgType argin, Tango::CmdArgType argout, const char *desc);

    ~DevQueryEventSystemCmd() override = default;

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevEnableEventSystemPerfMonCmd class
//
// description :    Class to implement the EnableEventSystemPerfMon command. This
//            command does takes a boolean input argument and does not return anything.
//
//            If called with true, performance samples will be collected for
//            events being published and received by the server.  These
//            performance samples can then be queried with the QueryEventSystem.
//
//            If called with false, collection of performance samples with stop.
//
//=============================================================================

class DevEnableEventSystemPerfMonCmd : public Command
{
  public:
    DevEnableEventSystemPerfMonCmd(const char *cmd_name,
                                   Tango::CmdArgType argin,
                                   Tango::CmdArgType argout,
                                   const char *desc);

    ~DevEnableEventSystemPerfMonCmd() override = default;

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevKillCmd class
//
// description :    Class to implement the DevKill command. This
//            command does not take any input argument. It simply
//            kills the device server.
//
//=============================================================================

class DevKillCmd : public Command
{
  public:
    DevKillCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out);

    ~DevKillCmd() override { }

    CORBA::Any *execute(DeviceImpl *, const CORBA::Any &) override;
};

//=============================================================================
//
//            The DevSetTraceLevelCmd class
//
// description :    Class to implement the DevSetTracelevel command.
//            It updates device server trace level with the input
//            argument
//
//=============================================================================

class DevSetTraceLevelCmd : public Command
{
  public:
    DevSetTraceLevelCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevSetTraceLevelCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevGetTraceLevel class
//
// description :    Class to implement the DevGetTracelevel command.
//            It simply returns the device server trace level
//
//=============================================================================

class DevGetTraceLevelCmd : public Command
{
  public:
    DevGetTraceLevelCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevGetTraceLevelCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevSetTraceOutputCmd class
//
// description :    Class to implement the DevSetTraceOutput command.
//            It set the server output to the input parameter
//
//=============================================================================

class DevSetTraceOutputCmd : public Command
{
  public:
    DevSetTraceOutputCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevSetTraceOutputCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevGetTraceOutputCmd class
//
// description :    Class to implement the DevGetTracelevel command.
//            It simply returns the device server trace level
//
//=============================================================================

class DevGetTraceOutputCmd : public Command
{
  public:
    DevGetTraceOutputCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~DevGetTraceOutputCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The QueryWizardClassPropertyCmd class
//
// description :    Class to implement the QueryWizardClassProperty command.
//            This command takes one input argument which is
//            the class name and return a
//            list of all the class properties definition registered
//            in the wizard for the specified class.
//
//=============================================================================

class QueryWizardClassPropertyCmd : public Command
{
  public:
    QueryWizardClassPropertyCmd(
        const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc);

    ~QueryWizardClassPropertyCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The QueryWizardDevPropertyCmd class
//
// description :    Class to implement the QueryWizardDevProperty command.
//            This command takes one input argument which is
//            the class name and return a
//            list of all the device properties definition registered
//            in the wizard for the specified class.
//
//=============================================================================

class QueryWizardDevPropertyCmd : public Command
{
  public:
    QueryWizardDevPropertyCmd(
        const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc);

    ~QueryWizardDevPropertyCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The QueryEventChannelIOR class
//
// description :    Class to implement the QueryEventChannelIOR command.
//            This command does not take any input argument and return
//            the event channel IOR. This command only exits foe DS
//            started with the -file option
//
//=============================================================================

class QueryEventChannelIORCmd : public Command
{
  public:
    QueryEventChannelIORCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *desc);

    ~QueryEventChannelIORCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The LockDeviceCmd class
//
// description :    Class to implement the LockDevice command.
//            This command takes one input argument which is
//            the device names and return an
//            integer which is the client locking thread period
//
//=============================================================================

class LockDeviceCmd : public Command
{
  public:
    LockDeviceCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc);

    ~LockDeviceCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The ReLockDevicesCmd class
//
// description :    Class to implement the ReLockDevices command.
//            This command takes one input argument which is
//            the vector with device names to be re-locked
//
//=============================================================================

class ReLockDevicesCmd : public Command
{
  public:
    ReLockDevicesCmd(const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc);

    ~ReLockDevicesCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The UnLockDeviceCmd class
//
// description :    Class to implement the UnLockDevice command.
//            This command takes one input argument which is
//            the device name to be unlocked
//
//=============================================================================

class UnLockDeviceCmd : public Command
{
  public:
    UnLockDeviceCmd(
        const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc);

    ~UnLockDeviceCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The DevLockStatusCmd class
//
// description :    Class to implement the DevLockStatus command.
//            This command takes one input argument which is
//            the device name for which you want to retrieve locking status
//
//=============================================================================

class DevLockStatusCmd : public Command
{
  public:
    DevLockStatusCmd(
        const char *cmd_name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc);

    ~DevLockStatusCmd() override { }

    CORBA::Any *execute(DeviceImpl *device, const CORBA::Any &in_any) override;
};

//=============================================================================
//
//            The EventSubscriptionChangeCmd class
//
// description :    Class to implement the EventSubscriptionChange command.
//            This command takes one arguments which are
//            the event for which the user subscribe to
//
//=============================================================================

class EventSubscriptionChangeCmd : public Tango::Command
{
  public:
    EventSubscriptionChangeCmd(const char *, Tango::CmdArgType, Tango::CmdArgType, const char *, const char *);
    EventSubscriptionChangeCmd(const char *, Tango::CmdArgType, Tango::CmdArgType);

    ~EventSubscriptionChangeCmd() override { }

    bool is_allowed(Tango::DeviceImpl *, const CORBA::Any &) override;
    CORBA::Any *execute(Tango::DeviceImpl *, const CORBA::Any &) override;
};

//=============================================================================
//
//            The ZmqEventSubscriptionChangeCmd class
//
// description :    Class to implement the ZmqEventSubscriptionChange command.
//            This command takes one arguments which are
//            the event for which the user subscribe to
//
//=============================================================================

class ZmqEventSubscriptionChangeCmd : public Tango::Command
{
  public:
    static const std::string in_desc;
    static const std::string out_desc;
    ZmqEventSubscriptionChangeCmd();

    ~ZmqEventSubscriptionChangeCmd() override { }

    bool is_allowed(Tango::DeviceImpl *, const CORBA::Any &) override;
    CORBA::Any *execute(Tango::DeviceImpl *, const CORBA::Any &) override;
};

//=============================================================================
//
//            The EventConfirmSubscriptionCmd class
//
// description :    Class to implement the EventConfirmSubscription command.
//            This command takes a list of event for which the client confirm
//            the subscription. This command returns nothing
//
//=============================================================================

class EventConfirmSubscriptionCmd : public Tango::Command
{
  public:
    EventConfirmSubscriptionCmd(const char *, Tango::CmdArgType, Tango::CmdArgType, const char *);
    EventConfirmSubscriptionCmd(const char *, Tango::CmdArgType, Tango::CmdArgType);

    ~EventConfirmSubscriptionCmd() override { }

    bool is_allowed(Tango::DeviceImpl *, const CORBA::Any &) override;
    CORBA::Any *execute(Tango::DeviceImpl *, const CORBA::Any &) override;
};

//=============================================================================
//
//            The DServerClass class
//
// description :    This class is a singleton ( The constructor is
//            protected and the _instance data member is static)
//            It contains all properties and methods
//            which the DServer objects requires only once e.g. the
//            commands.
//
//=============================================================================

class DServerClass : public DeviceClass
{
  public:
    static DServerClass *instance();
    static DServerClass *init();

    ~DServerClass() override { }

    void command_factory() override;
    void device_factory(const Tango::DevVarStringArray *devlist) override;

  protected:
    DServerClass(const std::string &);
    TANGO_IMP static DServerClass *_instance;
};

} // namespace Tango

#endif // _DSERVERCLASS_H
