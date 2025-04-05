#ifndef _CALLBACK_H
#define _CALLBACK_H

#include <memory>

namespace Tango
{
class CmdDoneEvent;
class AttrReadEvent;
class AttrWrittenEvent;
class EventData;
class AttrConfEventData;
class DataReadyEventData;
class DevIntrChangeEventData;
class PipeEventData;

/********************************************************************************
 *                                                                                 *
 *                         CallBack class                                            *
 *                                                                                 *
 *******************************************************************************/

/**
 * Event and asynchronous (callback model) calls base class
 *
 * When using the event push model (callback automatically executed), there are some cases (same callback
 * used for events coming from different devices hosted in device server process running on different hosts)
 * where the callback method could be executed concurently by different threads started by the ORB. The
 * user has to code his callback method in a thread safe manner.
 *
 *
 * @headerfile tango.h
 * @ingroup Client
 */

class CallBack
{
    friend class EventConsumer;
    friend class EventConsumerKeepAliveThread;

  public:
#ifdef GEN_DOC
    /**
     * Asynchronous command execution callback method
     *
     * This method is defined as being empty and must be overloaded by the user when the asynchronous callback
     * model is used. This is the method which will be executed when the server reply from a command_inout is
     * received in both push and pull sub-mode.
     *
     * @param cde The command data
     */
    virtual void cmd_ended(CmdDoneEvent *cde) { }

    /**
     * Asynchronous read attribute execution callback method
     *
     * This method is defined as being empty and must be overloaded by the user when the asynchronous callback
     * model is used. This is the method which will be executed when the server reply from a read_attribute(s) is
     * received in both push and pull sub-mode.
     *
     * @param are The read attribute data
     */
    virtual void attr_read(AttrReadEvent *are) { }

    /**
     * Asynchronous write attribute execution callback method
     *
     * This method is defined as being empty and must be overloaded by the user when the asynchronous callback
     * model is used. This is the method which will be executed when the server reply from a write_attribute(s)
     * is received in both push and pull sub-mode.
     *
     * @param awe The write attribute data
     */
    virtual void attr_written(AttrWrittenEvent *awe) { }

    /**
     * Event callback method
     *
     * This method is defined as being empty and must be overloaded by the user when events are used. This is
     * the method which will be executed when the server send event(s) to the client.
     *
     * @param ed The event data
     */
    virtual void push_event(EventData *ed) { }

    /**
     * attribute configuration change event callback method
     *
     * This method is defined as being empty and must be overloaded by the user when events are used. This
     * is the method which will be executed when the server send attribute configuration change event(s) to the
     * client.
     *
     * @param ace The attribute configuration change event data
     */
    virtual void push_event(AttrConfEventData *ace) { }

    /**
     * Data ready event callback method
     *
     * This method is defined as being empty and must be overloaded by the user when events are used. This is
     * the method which will be executed when the server send attribute data ready event(s) to the client.
     *
     * @param dre The data ready event data
     */
    virtual void push_event(DataReadyEventData *dre) { }

    /**
     * Device interface change event callback method
     *
     * This method is defined as being empty and must be overloaded by the user when events are used. This is
     * the method which will be executed when the server send device interface change event(s) to the client.
     *
     * @param dic The device interface change event data
     */
    virtual void push_event(DevIntrChangeEventData *dic) { }

    /**
     * Pipe event callback method
     *
     * This method is defined as being empty and must be overloaded by the user when events are used. This is
     * the method which will be executed when the server send pipe event(s) to the client.
     *
     * @param ped The pipe event data
     */
    virtual void push_event(PipeEventData *ped) { }
#else
    virtual void cmd_ended(CmdDoneEvent *) { }

    virtual void attr_read(AttrReadEvent *) { }

    virtual void attr_written(AttrWrittenEvent *) { }

    virtual void push_event(EventData *) { }

    virtual void push_event(AttrConfEventData *) { }

    virtual void push_event(DataReadyEventData *) { }

    virtual void push_event(DevIntrChangeEventData *) { }

    virtual void push_event(PipeEventData *) { }
#endif

    /// @privatesection
    virtual ~CallBack() { }

  private:
    class CallBackExt
    {
      public:
        CallBackExt() { }
    };

    std::unique_ptr<CallBackExt> ext;
};
} // namespace Tango
#endif
