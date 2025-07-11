//====================================================================================================================
//
// file :               Tango_const.h
//
// description :        Include for Tango system constant definition
//
// project :            TANGO
//
// author(s) :          A.Gotz + E.Taurel
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//=====================================================================================================================

#ifndef _TANGO_CONST_H
#define _TANGO_CONST_H

// FIXME remove once https://gitlab.com/tango-controls/cppTango/-/issues/786 is fixed
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdeprecated"
#endif

#include <tango/idl/tango.h>

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#include <tango/server/tango_config.h>
#include <tango/server/exception_reason_consts.h>

#include <tango/common/tango_version.h>

#include <string>
#include <vector>

namespace Tango
{

//
// A short inline function to hide the CORBA::string_dup function
//
inline char *string_dup(const std::string &s)
{
    return CORBA::string_dup(s.c_str());
}

inline char *string_dup(const char *s)
{
    return CORBA::string_dup(s);
}

// A short inline function to hide the CORBA::string_free function
inline void string_free(char *s)
{
    CORBA::string_free(s);
}

//
// Some general interest define
//

#define TBS(s) #s
#define XTBS(s) TBS(s)

// NOLINTBEGIN(readability-identifier-naming)
const char *const TgLibVers = XTBS(TANGO_VERSION_MAJOR.TANGO_VERSION_MINOR.TANGO_VERSION_PATCH);
const char *const TgLibMajorVers = XTBS(TANGO_VERSION_MAJOR);

const int TgLibVersNb = (TANGO_VERSION_MAJOR * 10000) + (TANGO_VERSION_MINOR * 100) + TANGO_VERSION_PATCH;

const int DevVersion = 6; // IDL version number
const int DefaultMaxSeq = 20;
const int DefaultBlackBoxDepth = 50;
const int DefaultPollRingDepth = 10;

const char *const InitialOutput = "Initial Output";
const char *const DSDeviceDomain = "dserver";
const char *const DefaultDocUrl = "http://www.tango-controls.org";
const char *const EnvVariable = "TANGO_HOST";
const char *const WindowsEnvVariable = "TANGO_ROOT";
const char *const DbObjName = "database";
const char *const NotSet = "Uninitialised";
const char *const ResNotDefined = "0";
const char *const MessBoxTitle = "Tango Device Server";
const char *const StatusNotSet = "Not initialised";
const char *const TangoHostNotSet = "Undef";
const char *const RootAttNotDef = "Not defined";

const bool DefaultWritAttrProp = false;
const char *const AllAttr = "All attributes";
const char *const AllAttr_3 = "All attributes_3";
const char *const AllPipe = "All pipes";
const char *const AllCmd = "All commands";

const char *const PollCommand = "command";
const char *const PollAttribute = "attribute";
const char *const NoClass = "noclass";
// NOLINTEND(readability-identifier-naming)

const char *const LOCAL_POLL_REQUEST = "_local";
const int LOCAL_REQUEST_STR_SIZE = 6;

const int MIN_POLL_PERIOD = 5;

const int DEFAULT_TIMEOUT = 3200;
const int DEFAULT_POLL_OLD_FACTOR = 4;

const int TG_IMP_MINOR_TO = 10;
const int TG_IMP_MINOR_DEVFAILED = 11;
const int TG_IMP_MINOR_NON_DEVFAILED = 12;

const char *const TANGO_PY_MOD_NAME = "_PyTango.pyd";
const char *const DATABASE_CLASS = "DataBase";

const int TANGO_FLOAT_PRECISION = 15;

const char *const SCALAR_PIPE = "Scalar";
const char *const ARRAY_PIPE = "Array";

//
// omniORB default configuration file
//

#ifdef _TG_WINDOWS_
const char *const DEFAULT_OMNI_CONF_FILE = "C:\\OMNIORB.CFG";
#else
const char *const DEFAULT_OMNI_CONF_FILE = "/etc/omniORB.cfg";
#endif

//
// Event related define
//

const int EVENT_HEARTBEAT_PERIOD = 10;
const int EVENT_RESUBSCRIBE_PERIOD = 600;
const int DEFAULT_EVENT_PERIOD = 1000;
const char *const HEARTBEAT = "Event heartbeat";

//
// ZMQ event system related define
//

const int ZMQ_EVENT_PROT_VERSION = 1;
const char *const HEARTBEAT_METHOD_NAME = "push_heartbeat_event";
const char *const EVENT_METHOD_NAME = "push_zmq_event";
const char *const HEARTBEAT_EVENT_NAME = "heartbeat";
const char *const CTRL_SOCK_ENDPOINT = "inproc://control";
const char *const MCAST_PROT = "epgm://";
const int MCAST_HOPS = 5;
const int PGM_RATE = 80 * 1024;
const int PGM_IVL = 20 * 1000;
const int MAX_SOCKET_SUB = 10;
const int PUB_HWM = 1000;
const int SUB_HWM = 1000;
const int SUB_SEND_HWM = 10000;
const int DEFAULT_LINGER = 0;

//
// Event when using a file as database stuff
//

const char *const NOTIFD_CHANNEL = "notifd_channel";

//
// Locking feature related defines
//

const int DEFAULT_LOCK_VALIDITY = 10;
const char *const DEVICE_UNLOCKED_REASON = "API_DeviceUnlocked";
const int MIN_LOCK_VALIDITY = 2;
const char *const TG_LOCAL_HOST = "localhost";

//
// Client timeout as defined by omniORB4.0.0
//

const char *const CLNT_TIMEOUT_STR = "3000";
const int CLNT_TIMEOUT = 3000;
const int NARROW_CLNT_TIMEOUT = 100;

//
// Connection and call timeout for database device
//

const int DB_CONNECT_TIMEOUT = 25000;
const int DB_RECONNECT_TIMEOUT = 20000;
const int DB_TIMEOUT = 13000;
const int DB_START_PHASE_RETRIES = 3;

//
// Access Control related defines
// WARNING: these string are also used within the Db stored procedure
// introduced in Tango V6.1. If you chang eit here, don't forget to
// also update the stored procedure
//

const char *const CONTROL_SYSTEM = "CtrlSystem";
const char *const SERVICE_PROP_NAME = "Services";
const char *const AUTO_ALARM_ON_CHANGE_PROP_NAME = "AutoAlarmOnChangeEvent";
const char *const ACCESS_SERVICE = "AccessControl";

//
// Polling threads pool related defines
//

const int DEFAULT_POLLING_THREADS_POOL_SIZE = 1;

//
// Max transfer size 256 MBytes (in byte). Needed by omniORB
//

const char *const MAX_TRANSFER_SIZE = "268435456";

//
// Max GIOP connection per server . Needed by omniORB
//

const char *const MAX_GIOP_PER_SERVER = "128";

//
// Tango name length
//

// NOLINTBEGIN(readability-identifier-naming)
const unsigned int MaxServerNameLength = 255;
const int MaxDevPropLength = 255;
// NOLINTEND(readability-identifier-naming)

//
// For forwarded attribute implementation
//

const int MIN_IDL_CONF5 = 5;
const int MIN_IDL_DEV_INTR = 5;
const int ALL_EVENTS = 0;

const int MIN_IDL_ZMQ_EVENT = 4;

//
// For event compatibility
//

const int ATT_CONF_REL_NB = 1; // Number of att. conf release on top of original one

//
// For device interface change event
//

const int DEV_INTR_THREAD_SLEEP_TIME = 50;

//
// For pipe
//

const int MAX_DATA_ELT_IN_PIPE_BLOB = 20;

//
// Files used to retrieve env. variables
//

const char *const USER_ENV_VAR_FILE = ".tangorc";

const char *const TANGO_RC_FILE = "/etc/tangorc";
const char *const WINDOWS_ENV_VAR_FILE = "tangorc";

//
// Logging targets (as string)
//

// NOLINTBEGIN(readability-identifier-naming)
const char *const kLogTargetConsole = "console";
const char *const kLogTargetFile = "file";
const char *const kLogTargetDevice = "device";
// NOLINTEND(readability-identifier-naming)

//
// Logging target [type/name] separator
//

// NOLINTNEXTLINE(readability-identifier-naming)
const char *const kLogTargetSep = "::";

//
// TANGO <rolling log files> threshold
//

// NOLINTBEGIN(readability-identifier-naming)
// Min RollingFileAppender threshold (~500kB)
const size_t kMinRollingThreshold = 500;
// Default RollingFileAppender threshold (~20MB)
const size_t kDefaultRollingThreshold = 20 * 1024;
// Max RollingFileAppender threshold (~1GB)
const size_t kMaxRollingThreshold = 1024 * 1024;
// NOLINTEND(readability-identifier-naming)

//
// The optional attribute properties
//

// NOLINTBEGIN(readability-identifier-naming)
const char *const AlrmValueNotSpec = "Not specified";
const char *const AssocWritNotSpec = "None";
const char *const LabelNotSpec = "No label";
const char *const DescNotSpec = "No description";
const char *const UnitNotSpec = "";
const char *const StdUnitNotSpec = "No standard unit";
const char *const DispUnitNotSpec = "No display unit";
const char *const FormatNotSpec_FL = "%6.2f";
const char *const FormatNotSpec_INT = "%d";
const char *const FormatNotSpec_STR = "%s";
const char *const FormatNotSpec = FormatNotSpec_FL;

const char *const NotANumber = "NaN";

const char *const MemNotUsed = "Not used yet";
const char *const MemAttrPropName = "__value";
const char *const RootAttrPropName = "__root_att";
// NOLINTEND(readability-identifier-naming)

// For DevEnum data type

typedef DevShort DevEnum;

//
// Many, many typedef
//

typedef const char *ConstDevString; // Pseudo Tango type to ease POGO job
typedef DevVarCharArray DevVarUCharArray;

class DeviceImpl;

typedef bool (DeviceImpl::*StateMethPtr)(const CORBA::Any &);

typedef void (DeviceImpl::*CmdMethPtr)();

// NOLINTBEGIN(readability-identifier-naming)
typedef void (DeviceImpl::*CmdMethPtr_Bo)(DevBoolean);
typedef void (DeviceImpl::*CmdMethPtr_Sh)(DevShort);
typedef void (DeviceImpl::*CmdMethPtr_Lg)(DevLong);
typedef void (DeviceImpl::*CmdMethPtr_Fl)(DevFloat);
typedef void (DeviceImpl::*CmdMethPtr_Db)(DevDouble);
typedef void (DeviceImpl::*CmdMethPtr_US)(DevUShort);
typedef void (DeviceImpl::*CmdMethPtr_UL)(DevULong);
typedef void (DeviceImpl::*CmdMethPtr_Str)(DevString);
typedef void (DeviceImpl::*CmdMethPtr_ChA)(const DevVarCharArray *);
typedef void (DeviceImpl::*CmdMethPtr_ShA)(const DevVarShortArray *);
typedef void (DeviceImpl::*CmdMethPtr_LgA)(const DevVarLongArray *);
typedef void (DeviceImpl::*CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef void (DeviceImpl::*CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef void (DeviceImpl::*CmdMethPtr_USA)(const DevVarUShortArray *);
typedef void (DeviceImpl::*CmdMethPtr_ULA)(const DevVarULongArray *);
typedef void (DeviceImpl::*CmdMethPtr_StrA)(const DevVarStringArray *);
typedef void (DeviceImpl::*CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef void (DeviceImpl::*CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef void (DeviceImpl::*CmdMethPtr_Sta)(DevState);

typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr)();
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr)();
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr)();
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr)();
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr)();
typedef DevUShort (DeviceImpl::*US_CmdMethPtr)();
typedef DevULong (DeviceImpl::*UL_CmdMethPtr)();
typedef DevString (DeviceImpl::*Str_CmdMethPtr)();
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr)();
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr)();
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr)();
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr)();
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr)();
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr)();
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr)();
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr)();
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr)();
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr)();
typedef DevState (DeviceImpl::*Sta_CmdMethPtr)();

typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_Bo)(DevBoolean);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_Sh)(DevShort);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_Lg)(DevLong);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_Fl)(DevFloat);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_Db)(DevDouble);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_US)(DevUShort);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_UL)(DevULong);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_Str)(DevString);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevBoolean (DeviceImpl::*Bo_CmdMethPtr_Sta)(DevState);

typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_Bo)(DevBoolean);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_Sh)(DevShort);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_Lg)(DevLong);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_Fl)(DevFloat);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_Db)(DevDouble);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_US)(DevUShort);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_UL)(DevULong);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_Str)(DevString);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevShort (DeviceImpl::*Sh_CmdMethPtr_Sta)(DevState);

typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_Bo)(DevBoolean);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_Sh)(DevShort);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_Lg)(DevLong);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_Fl)(DevFloat);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_Db)(DevDouble);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_US)(DevUShort);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_UL)(DevULong);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_Str)(DevString);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevLong (DeviceImpl::*Lg_CmdMethPtr_Sta)(DevState);

typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_Bo)(DevBoolean);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_Sh)(DevShort);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_Lg)(DevLong);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_Fl)(DevFloat);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_Db)(DevDouble);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_US)(DevUShort);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_UL)(DevULong);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_Str)(DevString);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevFloat (DeviceImpl::*Fl_CmdMethPtr_Sta)(DevState);

typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_Bo)(DevBoolean);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_Sh)(DevShort);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_Lg)(DevLong);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_Fl)(DevFloat);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_Db)(DevDouble);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_US)(DevUShort);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_UL)(DevULong);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_Str)(DevString);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevDouble (DeviceImpl::*Db_CmdMethPtr_Sta)(DevState);

typedef DevUShort (DeviceImpl::*US_CmdMethPtr_Bo)(DevBoolean);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_Sh)(DevShort);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_Lg)(DevLong);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_Fl)(DevFloat);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_Db)(DevDouble);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_US)(DevUShort);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_UL)(DevULong);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_Str)(DevString);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevUShort (DeviceImpl::*US_CmdMethPtr_Sta)(DevState);

typedef DevULong (DeviceImpl::*UL_CmdMethPtr_Bo)(DevBoolean);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_Sh)(DevShort);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_Lg)(DevLong);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_Fl)(DevFloat);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_Db)(DevDouble);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_US)(DevUShort);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_UL)(DevULong);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_Str)(DevString);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevULong (DeviceImpl::*UL_CmdMethPtr_Sta)(DevState);

typedef DevString (DeviceImpl::*Str_CmdMethPtr_Bo)(DevBoolean);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_Sh)(DevShort);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_Lg)(DevLong);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_Fl)(DevFloat);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_Db)(DevDouble);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_US)(DevUShort);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_UL)(DevULong);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_Str)(DevString);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevString (DeviceImpl::*Str_CmdMethPtr_Sta)(DevState);

typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_Sh)(DevShort);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_Lg)(DevLong);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_Db)(DevDouble);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_US)(DevUShort);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_UL)(DevULong);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_Str)(DevString);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarCharArray *(DeviceImpl::*ChA_CmdMethPtr_Sta)(DevState);

typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_Sh)(DevShort);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_Lg)(DevLong);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_Db)(DevDouble);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_US)(DevUShort);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_UL)(DevULong);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_Str)(DevString);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarShortArray *(DeviceImpl::*ShA_CmdMethPtr_Sta)(DevState);

typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_Sh)(DevShort);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_Lg)(DevLong);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_Db)(DevDouble);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_US)(DevUShort);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_UL)(DevULong);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_Str)(DevString);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarLongArray *(DeviceImpl::*LgA_CmdMethPtr_Sta)(DevState);

typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_Sh)(DevShort);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_Lg)(DevLong);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_Db)(DevDouble);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_US)(DevUShort);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_UL)(DevULong);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_Str)(DevString);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarFloatArray *(DeviceImpl::*FlA_CmdMethPtr_Sta)(DevState);

typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_Sh)(DevShort);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_Lg)(DevLong);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_Db)(DevDouble);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_US)(DevUShort);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_UL)(DevULong);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_Str)(DevString);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarDoubleArray *(DeviceImpl::*DbA_CmdMethPtr_Sta)(DevState);

typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_Sh)(DevShort);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_Lg)(DevLong);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_Db)(DevDouble);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_US)(DevUShort);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_UL)(DevULong);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_Str)(DevString);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarUShortArray *(DeviceImpl::*USA_CmdMethPtr_Sta)(DevState);

typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_Sh)(DevShort);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_Lg)(DevLong);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_Db)(DevDouble);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_US)(DevUShort);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_UL)(DevULong);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_Str)(DevString);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarULongArray *(DeviceImpl::*ULA_CmdMethPtr_Sta)(DevState);

typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_Sh)(DevShort);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_Lg)(DevLong);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_Db)(DevDouble);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_US)(DevUShort);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_UL)(DevULong);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_Str)(DevString);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarStringArray *(DeviceImpl::*StrA_CmdMethPtr_Sta)(DevState);

typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_Sh)(DevShort);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_Lg)(DevLong);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_Db)(DevDouble);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_US)(DevUShort);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_UL)(DevULong);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_Str)(DevString);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarLongStringArray *(DeviceImpl::*LSA_CmdMethPtr_Sta)(DevState);

typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_Bo)(DevBoolean);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_Sh)(DevShort);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_Lg)(DevLong);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_Fl)(DevFloat);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_Db)(DevDouble);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_US)(DevUShort);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_UL)(DevULong);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_Str)(DevString);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevVarDoubleStringArray *(DeviceImpl::*DSA_CmdMethPtr_Sta)(DevState);

typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_Bo)(DevBoolean);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_Sh)(DevShort);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_Lg)(DevLong);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_Fl)(DevFloat);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_Db)(DevDouble);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_US)(DevUShort);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_UL)(DevULong);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_Str)(DevString);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_ChA)(const DevVarCharArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_ShA)(const DevVarShortArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_LgA)(const DevVarLongArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_FlA)(const DevVarFloatArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_DbA)(const DevVarDoubleArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_USA)(const DevVarUShortArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_ULA)(const DevVarULongArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_StrA)(const DevVarStringArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_LSA)(const DevVarLongStringArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_DSA)(const DevVarDoubleStringArray *);
typedef DevState *(DeviceImpl::*Sta_CmdMethPtr_Sta)(DevState);

// NOLINTEND(readability-identifier-naming)

//
// Some enum and structures
//

enum CmdArgType
{
    DEV_VOID = 0,
    DEV_BOOLEAN = 1,
    DEV_SHORT = 2,
    DEV_LONG = 3,
    DEV_FLOAT = 4,
    DEV_DOUBLE = 5,
    DEV_USHORT = 6,
    DEV_ULONG = 7,
    DEV_STRING = 8,
    DEVVAR_CHARARRAY = 9,
    DEVVAR_SHORTARRAY = 10,
    DEVVAR_LONGARRAY = 11,
    DEVVAR_FLOATARRAY = 12,
    DEVVAR_DOUBLEARRAY = 13,
    DEVVAR_USHORTARRAY = 14,
    DEVVAR_ULONGARRAY = 15,
    DEVVAR_STRINGARRAY = 16,
    DEVVAR_LONGSTRINGARRAY = 17,
    DEVVAR_DOUBLESTRINGARRAY = 18,
    DEV_STATE = 19,
    CONST_DEV_STRING = 20,
    DEVVAR_BOOLEANARRAY = 21,
    DEV_UCHAR = 22,
    DEV_LONG64 = 23,
    DEV_ULONG64 = 24,
    DEVVAR_LONG64ARRAY = 25,
    DEVVAR_ULONG64ARRAY = 26,
    // We skip 27 deliberately here.  This used to be an unused enum variant
    // called DEV_INT.  By explicitly setting the values here we preserve
    // binary compatibility for the later enum variants.
    DEV_ENCODED = 28,
    DEV_ENUM = 29,
    DEV_PIPE_BLOB = 30,
    DEVVAR_STATEARRAY = 31,
    DEVVAR_ENCODEDARRAY = 32,
    DATA_TYPE_UNKNOWN = 100
};

enum MessBoxType
{
    STOP = 0,
    INFO
};

enum PollObjType
{
    POLL_CMD = 0,
    POLL_ATTR,
    EVENT_HEARTBEAT,
    STORE_SUBDEV
};

enum PollCmdCode
{
    POLL_ADD_OBJ = 0,
    POLL_REM_OBJ,
    POLL_START,
    POLL_STOP,
    POLL_UPD_PERIOD,
    POLL_REM_DEV,
    POLL_EXIT,
    POLL_REM_EXT_TRIG_OBJ,
    POLL_ADD_HEARTBEAT,
    POLL_REM_HEARTBEAT
};

enum SerialModel
{
    BY_DEVICE = 0,
    BY_CLASS,
    BY_PROCESS,
    NO_SYNC
};

enum AttReqType
{
    READ_REQ = 0,
    WRITE_REQ
};

typedef AttReqType PipeReqType;

enum LockCmdCode
{
    LOCK_ADD_DEV = 0,
    LOCK_REM_DEV,
    LOCK_UNLOCK_ALL_EXIT,
    LOCK_EXIT
};

//
// The polled device structure
//

typedef struct _PollDevice
{
    std::string dev_name;
    std::vector<long> ind_list;
} PollDevice;

//
// Logging levels
//

enum LogLevel
{
    LOG_OFF = 0,
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

//
// Logging targets
//

enum LogTarget
{
    LOG_CONSOLE = 0,
    LOG_FILE,
    LOG_DEVICE
};

/// List of command types
///
/// For converting a type from CmdArgType to a number use data_type_to_string().
const char *const CmdArgTypeName[] = {"DevVoid",
                                      "DevBoolean",
                                      "DevShort",
                                      "DevLong",
                                      "DevFloat",
                                      "DevDouble",
                                      "DevUShort",
                                      "DevULong",
                                      "DevString",
                                      "DevVarCharArray",
                                      "DevVarShortArray",
                                      "DevVarLongArray",
                                      "DevVarFloatArray",
                                      "DevVarDoubleArray",
                                      "DevVarUShortArray",
                                      "DevVarULongArray",
                                      "DevVarStringArray",
                                      "DevVarLongStringArray",
                                      "DevVarDoubleStringArray",
                                      "DevState",
                                      "ConstDevString",
                                      "DevVarBooleanArray",
                                      "DevUChar",
                                      "DevLong64",
                                      "DevULong64",
                                      "DevVarLong64Array",
                                      "DevVarULong64Array",
                                      "Unknown", // Corresponds to the form DEV_INT which is no longer used
                                      "DevEncoded",
                                      "DevEnum",
                                      "DevPipeBlob",
                                      "DevVarStateArray",
                                      "DevVarEncodedArray",
                                      "Unknown"};

/// Convert data types to strings
constexpr const char *data_type_to_string(int type)
{
    return (type >= DEV_VOID && type < (static_cast<int>(sizeof(CmdArgTypeName) / sizeof(CmdArgTypeName[0]))))
               ? CmdArgTypeName[type]
               : "Unknown";
}

//
// The state name
//

const char *const DevStateName[] = {"ON",
                                    "OFF",
                                    "CLOSE",
                                    "OPEN",
                                    "INSERT",
                                    "EXTRACT",
                                    "MOVING",
                                    "STANDBY",
                                    "FAULT",
                                    "INIT",
                                    "RUNNING",
                                    "ALARM",
                                    "DISABLE",
                                    "UNKNOWN"};

/**
 * Possible event type
 *
 * @ingroup Client
 * @headerfile tango.h
 */

enum EventType
{
    CHANGE_EVENT = 0, ///< Change event
    // We skip 1 deliberately here. This used to be the long time ago
    // deprecated QUALITY_EVENT. By explicitly setting the values here we
    // preserve binary compatibility for the later enum variants.
    PERIODIC_EVENT = 2,         ///< Periodic event
    ARCHIVE_EVENT = 3,          ///< Archive event
    USER_EVENT = 4,             ///< User event
    ATTR_CONF_EVENT = 5,        ///< Attribute configuration change event
    DATA_READY_EVENT = 6,       ///< Data ready event
    INTERFACE_CHANGE_EVENT = 7, ///< Device interface change event
    PIPE_EVENT = 8,             ///< Device pipe event
    ALARM_EVENT = 9,            ///< Alarm event
    numEventType = 10
};

const char *const EventName[] = {"change",
                                 "_unused_event",
                                 "periodic",
                                 "archive",
                                 "user_event",
                                 "attr_conf",
                                 "data_ready",
                                 "intr_change",
                                 "pipe",
                                 "alarm"};

const char *const CONF_TYPE_EVENT = EventName[ATTR_CONF_EVENT];
const char *const DATA_READY_TYPE_EVENT = EventName[DATA_READY_EVENT];

enum AttrSerialModel
{
    ATTR_NO_SYNC = 0,
    ATTR_BY_KERNEL,
    ATTR_BY_USER
};

/**
 * Possible error management with write_read_attribute call
 *
 * @ingroup Client
 * @headerfile tango.h
 */

enum ErrorManagementType
{
    ABORT_ON_ERROR = 0, ///< Do not read attribute(s) if one of the written attribute(s) failed
    CONTINUE_ON_ERROR,  ///< Read attribute(s) even if one of the written attribute(s) failed
    numErrorManagementType
};

enum KeepAliveCmdCode
{
    EXIT_TH = 0
};

enum AccessControlType
{
    ACCESS_READ = 0,
    ACCESS_WRITE
};

enum MinMaxValueCheck
{
    MIN = 0,
    MAX
};

enum ChannelType
{
    ZMQ = 0,
    NOTIFD
};

enum ZmqCmdCode
{
    ZMQ_END = 0,
    ZMQ_CONNECT_HEARTBEAT,
    ZMQ_DISCONNECT_HEARTBEAT,
    ZMQ_CONNECT_EVENT,
    ZMQ_DISCONNECT_EVENT,
    ZMQ_CONNECT_MCAST_EVENT,
    ZMQ_DELAY_EVENT,
    ZMQ_RELEASE_EVENT
};

typedef struct _SendEventType
{
    _SendEventType() { }

    bool change{false};
    bool alarm{false};
    bool archive{false};
    bool periodic{false};
} SendEventType;

typedef struct _OptAttrProp
{
    const char *name;
    const char *default_value;
} OptAttrProp;

typedef enum _FwdAttError
{
    FWD_NO_ERROR = 0,
    FWD_WRONG_ATTR,
    FWD_WRONG_DEV,
    FWD_ROOT_DEV_LOCAL_DEV,
    FWD_MISSING_ROOT,
    FWD_WRONG_SYNTAX,
    FWD_ROOT_DEV_NOT_STARTED,
    FWD_DOUBLE_USED,
    FWD_TOO_OLD_LOCAL_DEVICE,
    FWD_TOO_OLD_ROOT_DEVICE,
    FWD_CONF_LOOP,
    FWD_ERR_UNKNOWN
} FwdAttError;

typedef struct _AttributeIdlData
{
    AttributeValueList_3 *data_3;
    AttributeValueList_4 *data_4;
    AttributeValueList_5 *data_5;

    _AttributeIdlData()
    {
        data_3 = nullptr;
        data_4 = nullptr;
        data_5 = nullptr;
    }
} AttributeIdlData;

template <typename T>
class PointerWithLock;

} // namespace Tango

#endif /* TANGO_CONST_H */
