//////////////////////////////////////////////////////////////////
//
// Connection.h - include file for TANGO class Connection
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
//
//
//
///////////////////////////////////////////////////////////////

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <tango/common/tango_const.h>
#include <tango/client/CallBack.h>
#include <tango/client/DeviceData.h>
#include <tango/server/readers_writers_lock.h>

#include <chrono>
#include <optional>

#include <string>
#include <memory>

namespace Tango
{
class Connection;

/**
 * Possible asynchronous request type
 *
 * @ingroup Client
 * @headerfile tango.h
 */
enum asyn_req_type
{
    POLLING,   ///< Polling mode request
    CALL_BACK, ///< Callback mode request
    ALL_ASYNCH ///< All request
};

class TgRequest
{
  public:
    enum ReqType
    {
        CMD_INOUT,
        READ_ATTR,
        WRITE_ATTR_SINGLE,
        WRITE_ATTR
    };

    TgRequest(CORBA::Request_ptr re, ReqType ty) :
        request(re),
        req_type(ty),
        cb_ptr(nullptr),
        arrived(false),
        dev(nullptr)
    {
    }

    TgRequest(CORBA::Request_ptr re, ReqType ty, CallBack *cb) :
        request(re),
        req_type(ty),
        cb_ptr(cb),
        arrived(false),
        dev(nullptr)
    {
    }

    TgRequest(Tango::Connection *con, ReqType ty, CallBack *cb) :
        request(nullptr),
        req_type(ty),
        cb_ptr(cb),
        arrived(false),
        dev(con)
    {
    }

    CORBA::Request_ptr request;
    ReqType req_type;
    CallBack *cb_ptr;
    bool arrived;
    Connection *dev;
};

/****************************************************************************************
 *                                                                                         *
 *                     The Connection class                                                *
 *                     --------------------                                                *
 *                                                                                         *
 ***************************************************************************************/

/**
 * Base class for Tango device access
 *
 * Base class for Tango device access. This class is pure virtual and can be instantiated
 * as is.
 *
 *
 *
 *
 * @ingroup Client
 * @headerfile tango.h
 */

class Connection
{
  protected:
    ///@privatesection
    bool dbase_used;   // Dev. with database
    bool from_env_var; // DB from TANGO_HOST

    std::string host; // DS host (if dbase_used=false)
    std::string port; // DS port (if dbase_used=false)
    int port_num;     // DS port (as number)

    std::string db_host; // DB host
    std::string db_port; // DB port
    int db_port_num;     // DB port (as number)

    std::string ior;
    long pasyn_ctr;
    long pasyn_cb_ctr;

    Tango::Device_var device;
    Tango::Device_2_var device_2;

    int timeout;

    int connection_state;

    // the idl version of the peer
    int version;

    // the ultimate idl version supported by the server in which the peer is running - introduced for the
    // telemetry service to offer the related features to any device inheriting from device_4impl or higher.
    int server_version;

    // returns a ClntIdent initialized according to the IDL version of the device (peer) and the ultimate IDL version
    // supported by the server in which the peer is running. Added for the telemetry service introduced in IDLv6. Any
    // device with IDL version >= 4 running in a server in which the ultimate version of the IDL is >= 6, will benefit
    // from the telemetry features offered at the kernel level (low level profiling). This will limit the discontinuity
    // in the tracing information. See ClntIdent in tango.idl v6 for details.
    ClntIdent get_client_identification() const;

    Tango::DevSource source;

    bool check_acc;
    AccessControlType access;

    virtual std::string get_corba_name(bool) = 0;
    virtual std::string build_corba_name() = 0;
    virtual int get_lock_ctr() = 0;
    virtual void set_lock_ctr(int) = 0;

    DeviceData redo_synch_cmd(const TgRequest &);

    int get_env_var(const char *, std::string &);
    int get_env_var_from_file(const std::string &, const char *, std::string &);

    void set_connection_state(int);

    void check_and_reconnect();
    void check_and_reconnect(Tango::DevSource &);
    void check_and_reconnect(Tango::AccessControlType &);
    void check_and_reconnect(Tango::DevSource &, Tango::AccessControlType &);

    long add_asyn_request(CORBA::Request_ptr, TgRequest::ReqType);
    void remove_asyn_request(long);

    void add_asyn_cb_request(CORBA::Request_ptr, CallBack *, Connection *, TgRequest::ReqType);
    void remove_asyn_cb_request(Connection *, CORBA::Request_ptr);
    long get_pasyn_cb_ctr();

    class ConnectionExt
    {
      public:
        ConnectionExt() { }

        ~ConnectionExt() { }

        ConnectionExt &operator=(const ConnectionExt &);

        bool has_alt_adr{false};
    };

    std::unique_ptr<ConnectionExt> ext;

    bool tr_reco;
    Tango::Device_3_var device_3;

    std::optional<std::chrono::steady_clock::time_point> prev_failed_t0;

    Tango::Device_4_var device_4;
    omni_mutex adm_dev_mutex;
    omni_mutex asyn_mutex;
    ReadersWritersLock con_to_mon;

    int user_connect_timeout;
    bool tango_host_localhost;

    Tango::Device_5_var device_5;

    Tango::Device_6_var device_6;

  public:
    ///@publicsection
    /**@name Miscellaneous methods */
    //@{
    /**
     * Set device timeout
     *
     * Set client side timeout for device in milliseconds. Any method which takes longer than this time to execute
     * will throw an exception.
     *
     * @param [in] timeout The timeout value in mS
     */
    virtual void set_timeout_millis(int timeout);
    /**
     * Get device timeout
     *
     * Get the client side timeout in milliseconds.
     *
     * @return The device timeout (in mS)
     */
    virtual int get_timeout_millis();

    /**
     * Get device IDL version
     *
     * Get the version of the Tango Device IDL interface implemented by the device
     *
     * @return The device IDL version
     */
    int get_idl_version() const
    {
        return version;
    }

    /**
     * Get server IDL version
     *
     * Get the ultimate idl version supported by the server in which the device is running
     *
     * @return The server IDL version
     */
    int get_server_idl_version() const
    {
        return server_version;
    }

    /**
     * Get device source
     *
     * Get the device data source used by command_inout or read_attribute methods. The DevSource is an enumerated
     * type which can be one of {DEV, CACHE, CACHE_DEV}. See chapter on Advanced Feature in
     * <a href=http://www.esrf.eu/computing/cs/tango/tango_doc/kernel_doc/ds_prog/index.html target=new>Tango book</a>
     * for all details regarding polling.
     *
     * @return The device source flag
     */
    virtual Tango::DevSource get_source();
    /**
     * Set device source
     *
     * Set the data source (device, polling buffer, polling buffer than device) for command_inout and read_attribute
     * methods. The DevSource is an enumerated type which can be one of {DEV, CACHE, CACHE_DEV}. The
     * default value is CACHE_DEV. See chapter on Advanced Feature for all details regarding polling
     *
     * @param [in] sou The device source
     */
    virtual void set_source(Tango::DevSource sou);

    /**
     * Set device transparency (reconnection) mode
     *
     * If flag is true, no exception will be thrown in case of network communication error between client and
     * server. The API will try to re-build the network connection between client and server as soon as an error is
     * detected. See @ref recon for more details on reconnection and exception
     *
     * @param [in] val The device transparency flag
     */
    virtual void set_transparency_reconnection(bool val)
    {
        tr_reco = val;
    }

    /**
     * Get device transparency (reconnection) mode
     *
     * Returns the transparency reconnection flag.
     * See @ref recon for more details on reconnection and exception
     *
     * @return The device transparency flag
     */
    virtual bool get_transparency_reconnection()
    {
        return tr_reco;
    }

    //@}

    /** @name Synchronous command oriented methods */
    //@{
    /**
     * Execute a command (without input data)
     *
     * Execute a command on a device which takes no input arguments (void). The result is returned in a DeviceData
     * object
     *
     * @param [in] cmd_name The command name
     * @return The command result
     * @throws ConnectionFailed, CommunicationFailed, DeviceUnlocked, DevFailed from device
     */
    virtual DeviceData command_inout(const std::string &cmd_name);
    /**
     * Execute a command (with input data)
     *
     * Execute a command on a device. Input arguments are passed in a DeviceData object, output is returned as
     * a DeviceData object.
     *
     * @param [in] cmd_name The command name
     * @param [in] d_in Command input data
     * @return The command result
     * @throws ConnectionFailed, CommunicationFailed, DeviceUnlocked, DevFailed from device
     */
    virtual DeviceData command_inout(const std::string &cmd_name, const DeviceData &d_in);
    //@}

    /** @name Aynchronous command oriented methods */
    //@{
    /**
     * Execute a command asynchronously (with input argument)
     *
     * Execute asynchronously (polling model) a command on a device. Input arguments are passed in a DeviceData
     * object (see following chapters on how to insert data into DeviceData object). The last argument
     * is a fire and forget flag. If this flag is set to true, this means that the client does not care at all about the
     * server answer and will even not try to get it. A false default value is provided. Please, note that device
     * re-connection will not take place (in case it is needed) if the fire and forget mode is used. Therefore, an
     * application using only fire and forget requests is not able to automatically re-connnect to device. This call
     * returns an asynchronous call identifier which is needed to get the command result.
     *
     * @param [in] cmd_name The command name
     * @param [in] argin Command input data
     * @param [in] forget Fire and forget flag
     * @return The call identifier
     * @throws ConnectionFailed
     */
    virtual long command_inout_asynch(const std::string &cmd_name, const DeviceData &argin, bool forget = false);
    /**
     * Execute a command asynchronously
     *
     * Execute asynchronously (polling model) a command on a device which takes no input argument. The last
     * argument is a fire and forget flag. If this flag is set to true, this means that the client does not care at all
     * about the server answer and will even not try to get it. A false default value is provided. Please, note that
     * device re-connection will not take place (in case it is needed) if the fire and forget mode is used. Therefore,
     * an application using only fire and forget requests is not able to automatically re-connnect to device. This
     * call returns an asynchronous call identifier which is needed to get the command result.
     *
     * @param [in] cmd_name The command name
     * @param [in] forget Fire and forget flag
     * @return The call identifier
     * @throws ConnectionFailed
     */
    virtual long command_inout_asynch(const std::string &cmd_name, bool forget = false);
    /**
     * Check an asynchronous command_inout answer is arrived
     *
     * Check if the answer of an asynchronous command_inout is arrived (polling model). id is the asynchronous
     * call identifier. If the reply is arrived and if it is a valid reply, it is returned to the caller in a DeviceData
     * object. If the reply is an exception, it is re-thrown by this call. An exception is also thrown in case of the
     * reply is not yet arrived. Example :
     * @code
     * Tango::DeviceProxy dev("...");
     * long asyn_id;
     * asyn_id = dev.command_inout_asynch("MyCmd");
     * ...
     * ...
     * ...
     * Tango::DeviceData arg;
     * try
     * {
     *    arg = dev.command_inout_reply(asyn_id);
     * }
     * catch(Tango::AsynReplyNotArrived)
     * {
     *    std::cerr << "Command not arrived !" << std::endl;
     * }
     * catch (Tango::DevFailed &e)
     * {
     *    Tango::Except::print_exception(e);
     * }
     * @endcode
     *
     * @param [in] id The call identifier
     * @return The command result
     * @throws AsynCall, AsynReplyNotArrived, CommunicationFailed, DevFailed from device
     */
    virtual DeviceData command_inout_reply(long id);
    /**
     * Check an asynchronous command_inout answer is arrived with timeout
     *
     * Check if the answer of an asynchronous command_inout is arrived (polling model). id is the asynchronous
     * call identifier. If the reply is arrived and if it is a valid reply, it is returned to the caller in a DeviceData
     * object. If the reply is an exception, it is re-thrown by this call. If the reply is not yet arrived, the call
     will
     * wait (blocking the process) for the time specified in timeout. If after timeout milliseconds, the reply is still
     * not there, an exception is thrown. If timeout is set to 0, the call waits until the reply arrived.
     *
     * @param [in] id The call identifier
     8 @param [in] timeout The timeout value
     * @return The command result
     * @throws AsynCall, AsynReplyNotArrived, CommunicationFailed, DevFailed from device
     */
    virtual DeviceData command_inout_reply(long id, long timeout);
    /**
     * Execute a command asynchronously with callback
     *
     * Execute asynchronously (callback model) a command on a device which takes no input argument. The
     * last argument is a reference to a callback object. This callback object should be an instance of a user class
     * inheriting from the Tango::CallBack class with the cmd_ended() method overloaded.
     *
     * @param [in] cmd_name The command name
     * @param [in] cb The call-back object
     * @throws ConnectionFailed
     */
    virtual void command_inout_asynch(const std::string &cmd_name, CallBack &cb);
    /**
     * Execute a command asynchronously with input value and callback
     *
     * Execute asynchronously (callback model) a command on a device. Input arguments are passed in a DeviceData
     * object (see following chapters on how to insert data into DeviceData object). The last argument is
     * a reference to a callback object. This callback object should be an instance of a user class inheriting from
     * the Tango::CallBack class with the cmd_ended() method overloaded.
     *
     * @param [in] cmd_name The command name
     * @param [in] argin The command input data
     * @param [in] cb The call-back object
     * @throws ConnectionFailed
     */
    virtual void command_inout_asynch(const std::string &cmd_name, const DeviceData &argin, CallBack &cb);
    //@}

    /** @name Asynchronous attribute related methods */
    //@{
    /**
     * Fire callback methods
     *
     * Fire callback methods for device asynchronous requests with already arrived replied. Returns immediately
     * if there is no replies already arrived or if there is no asynchronous request for the device. Example :
     * @code
     * class MyCallBack: Tango::CallBack
     * {
     * public:
     *    MyCallback(double d):data(d) {}
     *    virtual void cmd_ended(Tango::CmdDoneEvent *);
     * private:
     *    double data;
     * };
     *
     * void MyCallBack::cmd_ended(Tango CmdDoneEvent *cmd)
     * {
     *    if (cmd->err == true)
     *       Tango::Except::print_error_stack(cmd->errors);
     *    else
     *    {
     *       short cmd_result;
     *       cmd->argout >> cmd_result;
     *       std::cout << "Command result = " << cmd_result << std::endl;
     *       std::cout << "Callback personal data = " << data << std::endl;
     *    }
     * }
     *
     * int main(int argc, char *argv[])
     * {
     *    ....
     *    ....
     *    Tango::DeviceProxy dev("...");
     *    double my_data = ...;
     *    MyCallBack cb(my_data);
     *    ...
     *    dev.command_inout_asynch("MyCmd",cb);
     *    ...
     *    ...
     *    ...
     *    dev.get_asynch_replies();
     *    ...
     *    ...
     * }
     * @endcode
     *
     */
    virtual void get_asynch_replies();
    /**
     * Fire callback methds with timeout
     *
     * Fire callback methods for device asynchronous requests (command and attributes) with already arrived
     * replied. Wait and block the caller for timeout milliseconds if they are some device asynchronous requests
     * which are not yet arrived. Returns immediately if there is no asynchronous request for the device. If
     * timeout is set to 0, the call waits until all the asynchronous requests sent to the device has received a reply.
     *
     * @param [in] timeout The timeout value
     */
    virtual void get_asynch_replies(long timeout);
    /**
     * Cancel a pending asynchronous request
     *
     * Cancel a pending asynchronous request. id is the asynchronous call identifier. This is a call local to the
     * client. It simply allows the caller not to get the answer of the asynchronous request. It does not interrupt
     * the call execution on the remote device.
     *
     * @param [in] id The call identifier
     * @throws AsynCall
     */
    virtual void cancel_asynch_request(long id);
    /**
     * Cancel all pending asynchronous request
     *
     * Cancel all pending polling asynchronous requests. This is a call local to the client. It simply allows the
     * caller not to get the answers of the asynchronous requests. It does not interrupt the call execution on the
     * remote devices.
     *
     */
    virtual void cancel_all_polling_asynch_request();
    //@}

    ///@privatesection
    virtual std::string dev_name() = 0;

    Connection(CORBA::ORB_var orb = CORBA::ORB::_nil());
    Connection(bool dummy);
    virtual ~Connection();
    Connection(const Connection &);
    Connection &operator=(const Connection &);

    std::string &get_db_host()
    {
        return db_host;
    }

    std::string &get_db_port()
    {
        return db_port;
    }

    int get_db_port_num()
    {
        return db_port_num;
    }

    bool get_from_env_var()
    {
        return from_env_var;
    }

    static void get_fqdn(std::string &);

    bool is_dbase_used()
    {
        return dbase_used;
    }

    std::string &get_dev_host()
    {
        return host;
    }

    std::string &get_dev_port()
    {
        return port;
    }

    void connect(const std::string &name);
    virtual void reconnect(bool);
    bool is_connected();

    Tango::Device_var &get_device()
    {
        return device;
    } // For CORBA expert !!

    Tango::Device_4_ptr get_device_4()
    {
        return Device_4::_duplicate(device_4);
    }

    Tango::Device_5_ptr get_device_5()
    {
        return Device_5::_duplicate(device_5);
    }

    virtual CORBA::Any_var command_inout(const std::string &, const CORBA::Any &);

    //
    // Asynchronous methods
    //

    void Cb_Cmd_Request(CORBA::Request_ptr, Tango::CallBack *);
    void Cb_ReadAttr_Request(CORBA::Request_ptr, Tango::CallBack *);
    void Cb_WriteAttr_Request(CORBA::Request_ptr req, Tango::CallBack *cb_ptr);
    void dec_asynch_counter(asyn_req_type ty);

    //
    // Control access related methods
    //

    AccessControlType get_access_control()
    {
        return access;
    }

    void set_access_control(AccessControlType acc)
    {
        access = acc;
    }

    AccessControlType get_access_right()
    {
        return get_access_control();
    }

    // NOLINTNEXTLINE(bugprone-tagged-union-member-count)
    friend class FwdAttribute;

  private:
    void omni420_timeout(int, char *);
    DeviceData omni420_except(int, char *, TgRequest &);
    void toIOR(const char *, IOP::IOR &);
};
} // namespace Tango
#endif /* _CONNECTION_H */
