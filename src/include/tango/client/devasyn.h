//
// devsyn.h - include file for TANGO api device asynchronous calls
//
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

#ifndef _DEVASYN_H
#define _DEVASYN_H

#include <map>
#include <string>

#include <tango/client/Connection.h>
#include <tango/client/CallBack.h>
#include <tango/client/DeviceProxy.h>

namespace Tango
{

//------------------------------------------------------------------------------
class DeviceData;
class DeviceAttribute;
class NamedDevFailedList;
class EventData;
class AttrConfEventData;
class DataReadyEventData;
class DevIntrChangeEventData;
class PipeEventData;
class EventDataList;
class AttrConfEventDataList;
class DataReadyEventDataList;
class DevIntrChangeEventDataList;
class PipeEventDataList;
class EventConsumer;
class EventConsumerKeepAliveThread;

/********************************************************************************
 *                                                                              *
 *                         CmdDoneEvent class                                        *
 *                                                                                 *
 *******************************************************************************/

/**
 * Asynchronous command execution callback data
 *
 * This class is used to pass data to the callback method in asynchronous callback model for command execution.
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class CmdDoneEvent
{
  public:
    ///@privatesection
    CmdDoneEvent(DeviceProxy *dev, std::string &cmd, DeviceData &arg, DevErrorList &err_in) :
        device(dev),
        cmd_name(cmd),
        argout(arg),
        errors(err_in)
    {
        err = errors.length() != 0;
    }

    ///@publicsection
    Tango::DeviceProxy *device; ///< The DeviceProxy object on which the call was executed
    std::string &cmd_name;      ///< The command name
    DeviceData &argout;         ///< The command argout
    bool err;                   ///< A boolean flag set to true if the command failed. False otherwise
    DevErrorList &errors;       ///< The error stack

  private:
    class CmdDoneEventExt
    {
      public:
        CmdDoneEventExt() { }
    };

    std::unique_ptr<CmdDoneEventExt> ext;
};

/********************************************************************************
 *                                                                                 *
 *                         AttrReadEvent class                                        *
 *                                                                                 *
 *******************************************************************************/

/**
 * Asynchronous read attribute execution callback data
 *
 * This class is used to pass data to the callback method in asynchronous callback model for read_attribute(s)
 * execution
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class AttrReadEvent
{
  public:
    ///@privatesection
    AttrReadEvent(DeviceProxy *dev,
                  std::vector<std::string> &att_names,
                  std::vector<DeviceAttribute> *arg,
                  DevErrorList &err_in) :
        device(dev),
        attr_names(att_names),
        argout(arg),
        errors(err_in)
    {
        err = errors.length() != 0;
    }

    ///@publicsection
    Tango::DeviceProxy *device{nullptr};           ///< The DeviceProxy object on which the call was executed
    std::vector<std::string> &attr_names;          ///< The attribute name list
    std::vector<DeviceAttribute> *argout{nullptr}; ///< The attribute data (callback function owns the memory)
    bool err{false};      ///< A boolean flag set to true if the request failed. False otherwise
    DevErrorList &errors; ///< The error stack

  private:
    class AttrReadEventExt
    {
      public:
        AttrReadEventExt() { }
    };

    std::unique_ptr<AttrReadEventExt> ext;
};

/********************************************************************************
 *                                                                                 *
 *                         AttrWrittenEvent class                                    *
 *                                                                                 *
 *******************************************************************************/

/**
 * Asynchronous write attribute execution callback data
 *
 * This class is used to pass data to the callback method in asynchronous callback model for write_attribute(s)
 * execution.
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class AttrWrittenEvent
{
  public:
    ///@privatesection
    AttrWrittenEvent(DeviceProxy *dev, std::vector<std::string> &att_names, NamedDevFailedList &err_in) :
        device(dev),
        attr_names(att_names),
        errors(err_in)
    {
        err = errors.call_failed();
    }

    ///@publicsection
    Tango::DeviceProxy *device{nullptr};  ///< The DeviceProxy object on which the call was executed
    std::vector<std::string> &attr_names; ///< The attribute name list
    bool err{false};                      ///< A boolean flag set to true if the request failed. False otherwise
    NamedDevFailedList &errors;           ///< The error stack

  private:
    class AttrWrittenEventExt
    {
      public:
        AttrWrittenEventExt() { }
    };

    std::unique_ptr<AttrWrittenEventExt> ext;
};

//------------------------------------------------------------------------------

class UniqIdent : public omni_mutex
{
  public:
    UniqIdent()
    {
        omni_mutex_lock sync(*this);
        ctr = 0;
    }

    long get_ident()
    {
        omni_mutex_lock sync(*this);
        return ++ctr;
    }

    long ctr;
};

class AsynReq : public omni_mutex
{
  public:
    AsynReq(UniqIdent *ptr) :
        ui_ptr(ptr),
        cond(this)
    {
    }

    ~AsynReq()
    {
        delete ui_ptr;
    }

    TgRequest &get_request(long);
    TgRequest &get_request(CORBA::Request_ptr);
    TgRequest *get_request(Tango::Connection *);

    long store_request(CORBA::Request_ptr, TgRequest::ReqType);
    void store_request(CORBA::Request_ptr, CallBack *, Connection *, TgRequest::ReqType);

    void remove_request(long);
    void remove_request(Connection *, CORBA::Request_ptr);

    size_t get_request_nb()
    {
        omni_mutex_lock sync(*this);
        return asyn_poll_req_table.size();
    }

    size_t get_cb_request_nb()
    {
        omni_mutex_lock sync(*this);
        return cb_req_table.size();
    }

    size_t get_cb_request_nb_i()
    {
        return cb_req_table.size();
    }

    void mark_as_arrived(CORBA::Request_ptr req);

    std::multimap<Connection *, TgRequest> &get_cb_dev_table()
    {
        return cb_dev_table;
    }

    void mark_as_cancelled(long);
    void mark_all_polling_as_cancelled();

    void wait()
    {
        cond.wait();
    }

    void signal()
    {
        omni_mutex_lock sync(*this);
        cond.signal();
    }

  protected:
    std::map<long, TgRequest> asyn_poll_req_table;
    UniqIdent *ui_ptr;

    std::multimap<Connection *, TgRequest> cb_dev_table;
    std::map<CORBA::Request_ptr, TgRequest> cb_req_table;

    std::vector<long> cancelled_request;

  private:
    omni_condition cond;
    bool remove_cancelled_request(long);
};

} // namespace Tango

#endif /* _DEVASYN_H */
