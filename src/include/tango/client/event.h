//===================================================================================================================
//
//  file :          event.h
//
//     description :   C++ include file for implementing the TANGO event server and client singleton classes -
//                    EventSupplier and EventConsumer.
//                     These classes are used to send events from the server to the notification service and to receive
//                    events from the notification service.
//
//  author(s) :     A.Gotz (goetz@esrf.fr)
//
// Copyright (C) :      2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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

#ifndef _EVENTAPI_H
#define _EVENTAPI_H

#include <tango/server/attribute.h>
#include <tango/server/except.h>
#include <tango/common/tango_const.h>
#include <tango/client/devapi.h>
#include <tango/client/ApiUtil.h>

#include <zmq.hpp>

namespace Tango
{

#ifndef _USRDLL
extern "C"
{
#endif
    void leavefunc();
#ifndef _USRDLL
}
#endif

class DeviceProxy;

/********************************************************************************
 *                                                                                 *
 *                         EventData class                                            *
 *                                                                                 *
 *******************************************************************************/

/**
 * Event callback execution data
 *
 * This class is used to pass data to the callback method when an event is sent to the client
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class EventData
{
  public:
    ///@privatesection
    EventData() { }

    EventData(DeviceProxy *dev,
              const std::string &nam,
              const std::string &evt,
              Tango::DeviceAttribute *attr_value_in,
              const DevErrorList &errors_in);

    virtual ~EventData();
    EventData(const EventData &);
    EventData &operator=(const EventData &);
    /**
     * The date when the event arrived
     */
    Tango::TimeVal reception_date;

    Tango::TimeVal &get_date()
    {
        return reception_date;
    }

    ///@publicsection
    std::unique_ptr<DeviceAttribute>
        get_attr_err_info(); ///< Construct and return a DeviceAttribute containing the error stack

    DeviceProxy *device{nullptr};         ///< The DeviceProxy object on which the call was executed
    std::string attr_name;                ///< The attribute name
    std::string event;                    ///< The event name
    DeviceAttribute *attr_value{nullptr}; ///< The attribute data
    bool err{false};                      ///< A boolean flag set to true if the request failed. False otherwise
    DevErrorList errors;                  ///< The error stack

  private:
    void set_time();
};

class FwdEventData : public EventData
{
  public:
    FwdEventData() = default;
    FwdEventData(
        DeviceProxy *, const std::string &, const std::string &, Tango::DeviceAttribute *, const DevErrorList &);
    FwdEventData(DeviceProxy *,
                 const std::string &,
                 const std::string &,
                 Tango::DeviceAttribute *,
                 const DevErrorList &,
                 zmq::message_t *);

    void set_av_5(const AttributeValue_5 *_p)
    {
        av_5 = _p;
    }

    const AttributeValue_5 *get_av_5()
    {
        return av_5;
    }

    zmq::message_t *get_zmq_mess_ptr()
    {
        return event_data;
    }

  private:
    const AttributeValue_5 *av_5{nullptr};
    zmq::message_t *event_data{nullptr};
};

/********************************************************************************
 *                                                                                 *
 *                         EventDataList class                                        *
 *                                                                                 *
 *******************************************************************************/
class EventDataList : public std::vector<EventData *>
{
  public:
    EventDataList() :
        std::vector<EventData *>(0)
    {
    }

    ~EventDataList()
    {
        if(size() > 0)
        {
            EventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }
        }
    }

    void clear()
    {
        if(size() > 0)
        {
            EventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }

            this->vector<EventData *>::clear();
        }
    }
};

/********************************************************************************
 *                                                                                 *
 *                         AttrConfEventData class                                    *
 *                                                                                 *
 *******************************************************************************/

/**
 * Attribute configuration change event callback execution data
 *
 * This class is used to pass data to the callback method when an attribute configuration event is sent to the
 * client
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class AttrConfEventData
{
  public:
    ///@privatesection
    AttrConfEventData() { }

    AttrConfEventData(DeviceProxy *dev,
                      const std::string &nam,
                      const std::string &evt,
                      Tango::AttributeInfoEx *attr_conf_in,
                      const DevErrorList &errors_in);
    virtual ~AttrConfEventData();
    AttrConfEventData(const AttrConfEventData &);
    AttrConfEventData &operator=(const AttrConfEventData &);
    /**
     * The date when the event arrived
     */
    Tango::TimeVal reception_date;

    Tango::TimeVal &get_date()
    {
        return reception_date;
    }

    ///@publicsection
    DeviceProxy *device{nullptr};        ///< The DeviceProxy object on which the call was executed
    std::string attr_name;               ///< The attribute name
    std::string event;                   ///< The event name
    AttributeInfoEx *attr_conf{nullptr}; ///< The attribute configuration
    bool err{false};                     ///< A boolean flag set to true if the request failed. False otherwise
    DevErrorList errors;                 ///< The error stack

  private:
    void set_time();
};

class FwdAttrConfEventData : public AttrConfEventData
{
  public:
    FwdAttrConfEventData() = default;
    FwdAttrConfEventData(
        DeviceProxy *, const std::string &, const std::string &, Tango::AttributeInfoEx *, const DevErrorList &);

    void set_fwd_attr_conf(const AttributeConfig_5 *_p)
    {
        fwd_attr_conf = _p;
    }

    const AttributeConfig_5 *get_fwd_attr_conf()
    {
        return fwd_attr_conf;
    }

  private:
    const AttributeConfig_5 *fwd_attr_conf{nullptr};
};

/********************************************************************************
 *                                                                                 *
 *                         AttrConfEventDataList class                                *
 *                                                                                 *
 *******************************************************************************/
class AttrConfEventDataList : public std::vector<AttrConfEventData *>
{
  public:
    AttrConfEventDataList() :
        std::vector<AttrConfEventData *>(0)
    {
    }

    ~AttrConfEventDataList()
    {
        if(size() > 0)
        {
            AttrConfEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }
        }
    }

    void clear()
    {
        if(size() > 0)
        {
            AttrConfEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }

            this->vector<AttrConfEventData *>::clear();
        }
    }
};

/********************************************************************************
 *                                                                                 *
 *                         DataReadyEventData class                                *
 *                                                                                 *
 *******************************************************************************/

/**
 * Data ready event callback execution data
 *
 * This class is used to pass data to the callback method when an attribute data ready event is sent to the client.
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class DataReadyEventData
{
  public:
    ///@privatesection
    DataReadyEventData() { }

    DataReadyEventData(DeviceProxy *, AttDataReady *, const std::string &evt, const DevErrorList &);

    ~DataReadyEventData() { }

    DataReadyEventData(const DataReadyEventData &);
    DataReadyEventData &operator=(const DataReadyEventData &);
    /**
     * The date when the event arrived
     */
    Tango::TimeVal reception_date;

    Tango::TimeVal &get_date()
    {
        return reception_date;
    }

    ///@publicsection
    DeviceProxy *device{nullptr}; ///< The DeviceProxy object on which the call was executed
    std::string attr_name;        ///< The attribute name
    std::string event;            ///< The event name
    int attr_data_type;           ///< The attribute data type
    int ctr;                      ///< The user counter. Set to 0 if not defined when sent by the server

    bool err{false};     ///< A boolean flag set to true if the request failed. False otherwise
    DevErrorList errors; ///< The error stack

  private:
    void set_time();
};

/********************************************************************************
 *                                                                                 *
 *                         DataReadyEventDataList class                            *
 *                                                                                 *
 *******************************************************************************/

class DataReadyEventDataList : public std::vector<DataReadyEventData *>
{
  public:
    DataReadyEventDataList() :
        std::vector<DataReadyEventData *>(0)
    {
    }

    ~DataReadyEventDataList()
    {
        if(size() > 0)
        {
            DataReadyEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }
        }
    }

    void clear()
    {
        if(size() > 0)
        {
            DataReadyEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }

            this->vector<DataReadyEventData *>::clear();
        }
    }
};

/********************************************************************************
 *                                                                                 *
 *                         DevIntrChangeEventData class                            *
 *                                                                                 *
 *******************************************************************************/

/**
 * Device interface change event callback execution data
 *
 * This class is used to pass data to the callback method when a device interface change event is sent to the client.
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class DevIntrChangeEventData
{
  public:
    ///@privatesection
    DevIntrChangeEventData() { }

    DevIntrChangeEventData(DeviceProxy *,
                           const std::string &,
                           const std::string &,
                           DevCmdInfoList_2 *,
                           AttributeConfigList_5 *,
                           bool,
                           const DevErrorList &);
    DevIntrChangeEventData(DeviceProxy *,
                           const std::string &,
                           const std::string &,
                           CommandInfoList *,
                           AttributeInfoListEx *,
                           bool,
                           const DevErrorList &);

    ~DevIntrChangeEventData() { }

    DevIntrChangeEventData(const DevIntrChangeEventData &);
    DevIntrChangeEventData &operator=(const DevIntrChangeEventData &);
    /**
     * The date when the event arrived
     */
    Tango::TimeVal reception_date;

    Tango::TimeVal &get_date()
    {
        return reception_date;
    }

    ///@publicsection
    DeviceProxy *device{nullptr}; ///< The DeviceProxy object on which the call was executed
    std::string event;            ///< The event name
    std::string device_name;      ///< The device name
    CommandInfoList cmd_list;     ///< Device command list info
    AttributeInfoListEx att_list; ///< Device attribute list info
    bool dev_started{false};      ///< Device started flag (true when event sent due to device being (re)started
                                  ///< and with only a possible but not sure interface change)

    bool err{false};     ///< A boolean flag set to true if the request failed. False otherwise
    DevErrorList errors; ///< The error stack

  private:
    void set_time();
};

/********************************************************************************
 *                                                                                 *
 *                         DevIntrChangeEventDataList class                        *
 *                                                                                 *
 *******************************************************************************/

class DevIntrChangeEventDataList : public std::vector<DevIntrChangeEventData *>
{
  public:
    DevIntrChangeEventDataList() :
        std::vector<DevIntrChangeEventData *>(0)
    {
    }

    ~DevIntrChangeEventDataList()
    {
        if(size() > 0)
        {
            DevIntrChangeEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }
        }
    }

    void clear()
    {
        if(size() > 0)
        {
            DevIntrChangeEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }

            this->vector<DevIntrChangeEventData *>::clear();
        }
    }
};

/********************************************************************************
 *                                                                                 *
 *                         PipeEventData class                                        *
 *                                                                                 *
 *******************************************************************************/

/**
 * Pipe event callback execution data
 *
 * This class is used to pass data to the callback method when a pipe event is sent to the client
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class PipeEventData
{
  public:
    ///@privatesection
    PipeEventData() { }

    PipeEventData(DeviceProxy *dev,
                  const std::string &nam,
                  const std::string &evt,
                  Tango::DevicePipe *pipe_value_in,
                  const DevErrorList &errors_in);

    ~PipeEventData();
    PipeEventData(const PipeEventData &);
    PipeEventData &operator=(const PipeEventData &);
    /**
     * The date when the event arrived
     */
    Tango::TimeVal reception_date;

    Tango::TimeVal &get_date()
    {
        return reception_date;
    }

    ///@publicsection
    DeviceProxy *device{nullptr};    ///< The DeviceProxy object on which the call was executed
    std::string pipe_name;           ///< The pipe name
    std::string event;               ///< The event name
    DevicePipe *pipe_value{nullptr}; ///< The pipe data
    bool err{false};                 ///< A boolean flag set to true if the request failed. False otherwise
    DevErrorList errors;             ///< The error stack

  private:
    void set_time();
};

/********************************************************************************
 *                                                                                 *
 *                         PipeEventDataList class                                    *
 *                                                                                 *
 *******************************************************************************/

class PipeEventDataList : public std::vector<PipeEventData *>
{
  public:
    PipeEventDataList() :
        std::vector<PipeEventData *>(0)
    {
    }

    ~PipeEventDataList()
    {
        if(size() > 0)
        {
            PipeEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }
        }
    }

    void clear()
    {
        if(size() > 0)
        {
            PipeEventDataList::iterator vpos;
            for(vpos = begin(); vpos != end(); ++vpos)
            {
                delete(*vpos);
            }

            this->vector<PipeEventData *>::clear();
        }
    }
};

/********************************************************************************
 *                                                                                 *
 *                         EventQueue class                                        *
 *                                                                                 *
 *******************************************************************************/
class EventQueue
{
  public:
    EventQueue();
    EventQueue(long max_size);
    ~EventQueue();

    void insert_event(EventData *new_event);
    void insert_event(AttrConfEventData *new_event);
    void insert_event(DataReadyEventData *new_event);
    void insert_event(DevIntrChangeEventData *new_event);
    void insert_event(PipeEventData *new_event);

    int size();
    TimeVal get_last_event_date();

    bool is_empty()
    {
        return event_buffer.empty();
    }

    void get_events(EventDataList &event_list);
    void get_events(AttrConfEventDataList &event_list);
    void get_events(DataReadyEventDataList &event_list);
    void get_events(DevIntrChangeEventDataList &event_list);
    void get_events(PipeEventDataList &event_list);
    void get_events(CallBack *cb);

  private:
    void inc_indexes();

    std::vector<EventData *> event_buffer;
    std::vector<AttrConfEventData *> conf_event_buffer;
    std::vector<DataReadyEventData *> ready_event_buffer;
    std::vector<DevIntrChangeEventData *> dev_inter_event_buffer;
    std::vector<PipeEventData *> pipe_event_buffer;

    long max_elt;
    long insert_elt;
    long nb_elt;

    omni_mutex modification_mutex;
};

//--------------------------------------------------------------------------------------------------------------------
//
// tries to execute user callback if specified and printout error, if failed, otherwise stores data in the event queue
//
//--------------------------------------------------------------------------------------------------------------------
template <typename T>
void safe_execute_callback_or_store_data(Tango::CallBack *callback,
                                         T *event_data,
                                         bool err_missed_event,
                                         T *missed_data,
                                         const std::string &method_name,
                                         const std::string &event_name,
                                         Tango::EventQueue *ev_queue)
{
    // if callback methods were specified, call them!
    if(callback != nullptr)
    {
        try
        {
            if(err_missed_event)
            {
                callback->push_event(missed_data);
            }
            callback->push_event(event_data);
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::ApiUtil *au = Tango::ApiUtil::instance();
            std::stringstream ss;
            ss << method_name << " got DevFailed exception: \n\n";
            ss << e.errors[0].desc;
            ss << "\nin callback method of: ";
            ss << event_name;
            au->print_error_message(ss.str().c_str());
        }
        catch(const std::exception &e)
        {
            Tango::ApiUtil *au = Tango::ApiUtil::instance();
            std::stringstream ss;
            ss << method_name << " got std::exception: \n\n";
            ss << e.what();
            ss << "\nin callback method of: ";
            ss << event_name;
            au->print_error_message(ss.str().c_str());
        }
        catch(...)
        {
            Tango::ApiUtil *au = Tango::ApiUtil::instance();
            std::stringstream ss;
            ss << method_name << " got unknown exception\n\n";
            ss << "in callback method of: ";
            ss << event_name;
            au->print_error_message(ss.str().c_str());
        }
        delete event_data;
    }

    // no callback method, the event has to be inserted
    // into the event queue
    else
    {
        if(err_missed_event)
        {
            auto *missed_data_copy = new T;
            *missed_data_copy = *missed_data;

            ev_queue->insert_event(missed_data_copy);
        }
        ev_queue->insert_event(event_data);
    }
}

// overload for calls without missed event
template <typename T>
void safe_execute_callback_or_store_data(CallBack *callback,
                                         T *event_data,
                                         const std::string &method_name,
                                         const std::string &event_name,
                                         EventQueue *ev_queue)
{
    safe_execute_callback_or_store_data(
        callback, event_data, false, static_cast<T *>(nullptr), method_name, event_name, ev_queue);
}

} // namespace Tango

#endif // _EVENTAPI_H
