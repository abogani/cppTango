//=============================================================================
//
// file :               utils.h
//
// description :        Include for utility functions or classes
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

#ifndef _UTILS_H
#define _UTILS_H

#include <tango/server/tango_config.h>
#include <tango/server/pollext.h>
#include <tango/server/pollthread.h>
#include <tango/server/subdev_diag.h>
#include <tango/server/rootattreg.h>
#include <tango/server/tango_monitor.h>
#include <tango/client/ApiUtil.h>

#ifndef _TG_WINDOWS_
  #include <unistd.h>
#else
  #include <tango/windows/coutbuf.h>
  #include <tango/windows/w32win.h>
#endif /* _TG_WINDOWS_ */
#include <iostream>
#include <new>
#include <algorithm>

namespace Tango
{

class DeviceImpl;
class DeviceClass;
class DServer;
class AutoTangoMonitor;
class Util;
class NotifdEventSupplier;
class ZmqEventSupplier;
class DbServerCache;
class SubDevDiag;
class FwdAttr;

struct PollingThreadInfo;
struct DevDbUpd;

#ifdef _TG_WINDOWS_
class CoutBuf;
class W32Win;
#endif

// A global variable that indicates whether a thread is considered a library
// thread, that is: either an omniORB worker thread handling the RPC request
// or one of: main thread, ServRestartThread, KillThread, PollThread, ThSig.
// TODO: This is not intended to be accessed from outside the library and
// should be moved into a private header once we have that.
extern thread_local bool is_tango_library_thread;

// Template metaprogramming to infer
// underlying type for DevVarArray types.
// and vice versa
template <class T>
struct get_type;

template <>
struct get_type<_CORBA_Sequence_Boolean>
{
    using type = Tango::DevBoolean;
};

template <>
struct get_type<_CORBA_Sequence_Octet>
{
    using type = Tango::DevUChar;
};

template <template <class> class T, class U>
struct get_type<T<U>>
{
    using type = U;
};

class Interceptors
{
  public:
    Interceptors() { }

    virtual ~Interceptors() { }

    virtual void create_thread() { }

    virtual void delete_thread() { }
};

//=============================================================================
//
//            The Util class
//
// description :    This class contains all properties and methods
//            which a device server process requires only once e.g.
//            the orb and boa pointers....
//            This class is a singleton ( The constructor is
//            protected and the _instance data member is static)
//
//            This class must be created at the beginning of each
//            device server process
//
//=============================================================================

/**
 * This class is a used to store TANGO device server process data and to provide
 * the user with a set of utilities method. This class is implemented using
 * the singleton design pattern. Therefore a device server process can have only
 * one instance of this class and its constructor is not public.
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

class Util
{
    friend class Tango::AutoTangoMonitor;
    friend class Tango::ApiUtil;

  public:
    /**@name Singleton related methods
     * These methods follow the singleton design pattern (only one instance
     * of a class) */
    //@{

    /**
     * Create and get the singleton object reference.
     *
     * This method returns a reference to the object of the Util class.
     * If the class singleton object has not been created, it will be
     * instantiated
     *
     * @param argc The process command line argument number
     * @param argv The process commandline arguments
     * @return The Util object reference
     */
    static Util *init(int argc, char *argv[]);
#ifdef _TG_WINDOWS_
    /**
     * Create and get the singleton object reference.
     *
     * This method returns a reference to the object of the Util class.
     * If the class singleton object has not been created, it will be
     * instantiated. This method must be used only for non-console mode windows
     * device server
     *
     * @param AppInst The application instance
     * @param CmdShow The display window flag
     * @return The Util object reference
     */
    static Util *init(HINSTANCE AppInst, int CmdShow);
#endif

    /**
     * Get the singleton object reference.
     *
     * This method returns a reference to the object of the Util class.
     * If the class has not been initialised with it's init method, this method
     * print a message and abort the device server process
     *
     * @return The Util object reference
     */
    static Util *instance(bool exit = true);
    //@}

    /**@name Destructor
     * Only one destructor is defined for this class */
    //@{
    /**
     * The class destructor.
     */
    ~Util();

    //@}

    /**@name Get/Set instance data */
    //@{
    /**
     * Get a reference to the CORBA ORB
     *
     * This is a CORBA _duplicate of the original reference
     *
     * @return The CORBA ORB
     */
    CORBA::ORB_var get_orb()
    {
        return CORBA::ORB::_duplicate(orb);
    }

    /**
     * Get a reference to the CORBA Portable Object Adapter (POA)
     *
     * This is a CORBA _dupilcate of the original reference to the object POA.
     * For classical device server, thisis the root POA. For no database device
     * server, this is a specific POA with the USER_ID policy.
     *
     * @return The CORBA root POA
     */
    PortableServer::POA_ptr get_poa()
    {
        return PortableServer::POA::_duplicate(_poa);
    }

    /**
     * Set the process trace level.
     *
     * @param level The new process level
     */
    void set_trace_level(int level)
    {
        _tracelevel = level;
    }

    /**
     * Get the process trace level.
     *
     * @return The process trace level
     */
    int get_trace_level()
    {
        return _tracelevel;
    }

    /**
     * Get the device server instance name.
     *
     * @return The device server instance name
     */
    std::string &get_ds_inst_name()
    {
        return ds_instance_name;
    }

    /**
     * Get the device server's unmodified executable name.
     *
     * @return The device server's unmodified executable name
     */
    const std::string &get_ds_unmodified_exec_name()
    {
        return ds_unmodified_exec_name;
    }

    /**
     * Get the device server executable name.
     *
     * @return The device server executable name
     */
    std::string &get_ds_exec_name()
    {
        return ds_exec_name;
    }

    /**
     * Get the device server name.
     *
     * The device server name is the device server executable name/the device
     * server instance name
     * @return The device server name
     */
    std::string &get_ds_name()
    {
        return ds_name;
    }

    /**
     * Get the host name where the device server process is running.
     *
     * @return The host name
     */
    std::string &get_host_name()
    {
        return hostname;
    }

    /**
     * Get the device server process identifier as a String
     *
     * @return The device server process identifier as a string
     */
    std::string &get_pid_str()
    {
        return pid_str;
    }

    /**
     * Get the device server process identifier
     *
     * @return The device server process identifier
     */
    TangoSys_Pid get_pid()
    {
        return pid;
    }

    /**
     * Get the TANGO library version number.
     *
     * @return The Tango library release number coded in 3 digits
     * (for instance 550,551,552,600,....)
     */
    long get_tango_lib_release();

    /**
     * Get the IDL TANGO version.
     *
     * @return The device server version
     */
    std::string &get_version_str()
    {
        return version_str;
    }

    /**
     * Get the device server version.
     *
     * @return The device server version
     */
    std::string &get_server_version()
    {
        return server_version;
    }

    /**
     * Set the device server version.
     *
     * @param vers The device server version
     */
    void set_server_version(const char *vers)
    {
        server_version = vers;
    }

    /**
     * Set the DeviceClass list pointer
     *
     * @param list The DeviceClass ptr vector address
     */
    void set_class_list(std::vector<DeviceClass *> *list)
    {
        cl_list_ptr = list;
        cl_list = *list;
    }

    /**
     * Add a DeviceClass to the DeviceClass list pointer
     *
     * @param cl The DeviceClass ptr
     */
    void add_class_to_list(DeviceClass *cl)
    {
        cl_list.push_back(cl);
    }

    /**
     * Get the DeviceClass list pointer
     *
     * @return The DeviceClass ptr vector address
     */
    const std::vector<DeviceClass *> *get_class_list()
    {
        return &cl_list;
    }

    /**
     * Set the serialization model
     *
     * @param ser The new serialization model. The serialization model must be one
     * of BY_DEVICE, BY_CLASS, BY_PROCESS or NO_SYNC
     */
    void set_serial_model(SerialModel ser)
    {
        ser_model = ser;
    }

    /**
     * Get the serialization model
     *
     * @return The serialization model. This serialization model is one of
     * BY_DEVICE, BY_CLASS, BY_PROCESS or NO_SYNC
     */
    SerialModel get_serial_model()
    {
        return ser_model;
    }

    /**
     * Get a reference to the notifd TANGO EventSupplier object
     *
     * @return The notifd EventSupplier object
     */
    NotifdEventSupplier *get_notifd_event_supplier()
    {
        return nd_event_supplier;
    }

    /**
     * Get a reference to the ZMQ TANGO EventSupplier object
     *
     * @return The zmq EventSupplier object
     */
    ZmqEventSupplier *get_zmq_event_supplier()
    {
        return zmq_event_supplier;
    }

    /**
     * Set device server process event buffer high water mark (HWM)
     *
     * @param val The new event buffer high water mark in number of events
     */
    void set_ds_event_buffer_hwm(DevLong val)
    {
        if(user_pub_hwm == -1)
        {
            user_pub_hwm = val;
        }
    }

    /**
     * Get _UseDb flag.
     *
     */
    bool use_db() const
    {
        return _UseDb;
    }

    /**
     * Get _FileDb flag.
     *
     */
    bool use_file_db() const
    {
        return _FileDb;
    }

    //@}

    /** @name Polling related methods */
    //@{
    /**
     * Trigger polling for polled command.
     *
     * This method send the order to the polling thread to poll one object
     * registered with an update period defined as "externally triggerred"
     *
     * @param dev The TANGO device
     * @param name The command name which must be polled
     * @exception DevFailed If the call failed
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void trigger_cmd_polling(DeviceImpl *dev, const std::string &name);

    /**
     * Trigger polling for polled attribute.
     *
     * This method send the order to the polling thread to poll one object
     * registered with an update period defined as "externally triggerred"
     *
     * @param dev The TANGO device
     * @param name The attribute name which must be polled
     * @exception DevFailed If the call failed
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void trigger_attr_polling(DeviceImpl *dev, const std::string &name);

    /**
     * Fill polling buffer for polled attribute.
     *
     * This method fills the polling buffer for one polled attribute
     * registered with an update period defined as "externally triggerred"
     * (polling period set to 0)
     *
     * @param dev The TANGO device
     * @param att_name The attribute name which must be polled
     * @param data The data stack with one element for each history element
     * @exception DevFailed If the call failed
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */

    template <typename T>
    void fill_attr_polling_buffer(DeviceImpl *dev, std::string &att_name, AttrHistoryStack<T> &data);

    /**
     * Fill polling buffer for polled command.
     *
     * This method fills the polling buffer for one polled command
     * registered with an update period defined as "externally triggerred"
     * (polling period set to 0)
     *
     * @param dev The TANGO device
     * @param cmd_name The command name which must be polled
     * @param data The data stack with one element for each history element
     * @exception DevFailed If the call failed
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */

    template <typename T>
    void fill_cmd_polling_buffer(DeviceImpl *dev, std::string &cmd_name, CmdHistoryStack<T> &data);

    /**
     * Set the polling threads pool size
     *
     * @param thread_nb The maximun number of threads in the polling threads pool
     */
    void set_polling_threads_pool_size(unsigned long thread_nb)
    {
        poll_pool_size = thread_nb;
    }

    /**
     * Get the polling threads pool size
     *
     * @return The maximun number of threads in the polling threads pool
     */
    unsigned long get_polling_threads_pool_size()
    {
        return poll_pool_size;
    }

    /**
     * Set the polling thread algorithm to the algorithum used before Tango 9
     *
     * @param val Polling algorithm flag
     */
    void set_polling_before_9(bool val)
    {
        polling_bef_9_def = true;
        polling_bef_9 = val;
    }

    //@}

    /**@name Miscellaneous methods */
    //@{
    /**
     * Check if the device server process is in its starting phase
     *
     * @return A boolean set to true if the server is in its starting phase.
     */
    bool is_svr_starting()
    {
        return svr_starting;
    }

    /**
     * Check if the device server process is in its shutting down sequence
     *
     * @return A boolean set to true if the server is in its shutting down phase.
     */
    bool is_svr_shutting_down()
    {
        return svr_stopping;
    }

    /**
     * Check if the device is actually restarted by the device server
     * process admin device with its DevRestart command
     *
     * @return A boolean set to true if the device is restarting.
     */
    bool is_device_restarting(const std::string &d_name);
    //@}

    /**@name Database related methods */
    //@{
    /**
     * Connect the process to the TANGO database.
     *
     * If the connection to the database failed, a message is displayed on the
     * screen and the process is aborted
     */
    void connect_db();

    /**
     * Reread the file database
     */
    void reset_filedatabase();

    /**
     * Get a reference to the TANGO database object
     *
     * @return The database object
     */
    Database *get_database()
    {
        return db;
    }

    /**
     * Unregister a device server process from the TANGO database.
     *
     * If the database call fails, a message is displayed on the screen and the
     * process is aborted
     */
    void unregister_server();
    //@}

    /** @name Device reference related methods */
    //@{
    /**
     * Get the list of device references for a given TANGO class.
     *
     * Return the list of references for all devices served by one implementation
     * of the TANGO device pattern implemented in the  process
     *
     * @param class_name The TANGO device class name
     * @return The device reference list
     * @exception DevFailed If in the device server process there is no TANGO
     * device pattern implemented the TANGO device class given as parameter
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    std::vector<DeviceImpl *> &get_device_list_by_class(const std::string &class_name);

    /**
     * Get the list of device references for a given TANGO class.
     *
     * Return the list of references for all devices served by one implementation
     * of the TANGO device pattern implemented in the  process
     *
     * @param class_name The TANGO device class name
     * @return The device reference list
     * @exception DevFailed If in the device server process there is no TANGO
     * device pattern implemented the TANGO device class given as parameter
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    std::vector<DeviceImpl *> &get_device_list_by_class(const char *class_name);

    /**
     * Get a device reference from its name
     *
     * @param dev_name The TANGO device name
     * @return The device reference
     * @exception DevFailed If in the device is not served by one device pattern
     * implemented in this process.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    DeviceImpl *get_device_by_name(const std::string &dev_name);

    /**
     * Get a device reference from its name
     *
     * @param dev_name The TANGO device name
     * @return The device reference
     * @exception DevFailed If in the device is not served by one device pattern
     * implemented in this process.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    DeviceImpl *get_device_by_name(const char *dev_name);

    /**
     * Get a reference to the dserver device attached to the device server process
     *
     * @return A reference to the dserver device
     */
    DServer *get_dserver_device();

    /**
     * Get DeviceList from name.
     *
     * It is possible to use a wild card ('*') in the name parameter
     *  (e.g. "*", "/tango/tangotest/n*", ...)
     *
     * @param name The device name
     * @return The DeviceClass ptr vector address
     */
    std::vector<DeviceImpl *> get_device_list(const std::string &name);
    //@}

    /** @name Device pattern related methods */
    //@{
    /**
     * Initialise all the device server pattern(s) embedded in a device server
     * process.
     *
     * @exception DevFailed If the device pattern initialisation failed
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void server_init(bool with_window = false);

    /**
     * Run the CORBA event loop
     *
     * This method runs the CORBA event loop. For UNIX or Linux operating system,
     * this method does not return. For Windows in a non-console mode,
     * this method start a thread which enter the CORBA event loop.
     */
    void server_run();

    /**
     * Cleanup a Tango device server process before exit
     *
     * This method cleanup a Tango device server and relinquish all computer
     * resources before the process exit
     */
    void server_cleanup();

    /**
     * Set the server event loop
     *
     * This method registers an event loop function in a Tango server.
     * This function will be called by the process main thread in an infinite loop
     * The process will not use the classical ORB blocking event loop.
     * It is the user responsability to code this function in a way that it implements
     * some kind of blocking in order not to load the computer CPU
     *
     * @param f_ptr The event loop function pointer. This function will not receive
     * any argument. It returns a boolean value. If this boolean is set to true,
     * the device server process exit.
     */
    void server_set_event_loop(bool (*f_ptr)())
    {
        ev_loop_func = f_ptr;
    }

    //@}

    /**@name Class data members */
    //@{
    /**
     * The process trace level
     */
    TANGO_IMP static int _tracelevel;
    /**
     * The database use flag (Use with extreme care). Implemented for device
     * server started without database usage.
     */
    TANGO_IMP static bool _UseDb;
    /**
     * A daemon process flag. If this flag is set to true, the server
     * process will not exit if it not able to connect to the database.
     * Instead, it will loop until the connection suceeds. The default
     * value is false.
     */
    TANGO_IMP static bool _daemon;
    /**
     * The loop sleeping time in case of the _daemon flag set to true.
     * This sleeping time is the number of seconds the process will
     * sleep before it tries again to connect to the database. The default
     * value is 60 seconds.
     */
    TANGO_IMP static long _sleep_between_connect;
    //@}

    /// @privatesection

    TANGO_IMP static bool _FileDb;

    /// @publicsection

#ifdef _TG_WINDOWS_
    /**@name Windows specific methods */
    //@{
    /**
     * Get the console window instance
     *
     * @return The device server graphical interface console window instance
     */

    HWND get_console_window();

    /**
     * Get the main window instance
     *
     * @return The device server graphical interface main window instance
     */
    HWND get_ds_main_window();

    /**
     * Get a pointer to the debug object
     *
     * @return A pointer to the debug object
     */
    CoutBuf *get_debug_object();

    TANGO_IMP static bool _service;

    /**
     * Get the text displayed on main server window.
     *
     * @return The text to be displayed
     */
    std::string &get_main_window_text()
    {
        return main_win_text;
    }

    /**
     * Set the text displayed on main server window.
     *
     * @param     txt     The new text to be displayed at the bottom of the
     * main window
     */
    void set_main_window_text(const std::string &txt)
    {
        main_win_text = txt;
    }

//@}
#endif

  protected:
    /**
     * Constructs a newly allocated Util object.
     *
     * This constructor is protected following the singleton pattern
     *
     * @param argc The process command line argument number
     * @param argv The process commandline arguments
     *
     */
    Util(int argc, char *argv[]);
#ifdef _TG_WINDOWS_
    /**
     * Constructs a newly allocated Util object for Windows non-console
     * device server.
     *
     * This constructor is protected following the singleton pattern
     *
     * @param AppInst The applicationinstance
     * @param CmdShow The display window flag
     *
     */
    Util(HINSTANCE AppInst, int CmdShow);
#endif

    //
    // The extension class
    //

  private:
    class UtilExt
    {
      public:
        bool endpoint_publish_specified = false; // PublishEndpoint specified on cmd line
        std::string endpoint_publish;            // PublishEndpoint as gathered from the environment
    };

  public:
    /// @privatesection
    void set_interceptors(Interceptors *in)
    {
        inter = in;
    }

    Interceptors *get_interceptors()
    {
        return inter;
    }

    std::map<std::string, std::vector<std::string>> &get_cmd_line_name_list()
    {
        return cmd_line_name_list;
    }

    void get_cmd_line_name_list(const std::string &, std::vector<std::string> &);

    TangoMonitor &get_heartbeat_monitor()
    {
        return poll_mon;
    }

    PollThCmd &get_heartbeat_shared_cmd()
    {
        return shared_data;
    }

    bool poll_status()
    {
        return poll_on;
    }

    void poll_status(bool status)
    {
        poll_on = status;
    }

    //
    // Some methods are duplicated here (with different names). It is for compatibility reason
    //

    void polling_configure();

    PollThread *get_polling_thread_object()
    {
        return heartbeat_th;
    }

    PollThread *get_heartbeat_thread_object()
    {
        return heartbeat_th;
    }

    void clr_poll_th_ptr()
    {
        heartbeat_th = nullptr;
    }

    void clr_heartbeat_th_ptr()
    {
        heartbeat_th = nullptr;
    }

    int get_polling_thread_id()
    {
        return heartbeat_th_id;
    }

    int get_heartbeat_thread_id()
    {
        return heartbeat_th_id;
    }

    void stop_heartbeat_thread();

    std::string &get_svr_port_num()
    {
        return svr_port_num;
    }

    void create_notifd_event_supplier();
    void create_zmq_event_supplier();

    std::shared_ptr<DbServerCache> get_db_cache()
    {
        return db_cache;
    }

    void unvalidate_db_cache()
    {
        if(db_cache)
        {
            db_cache.reset();
        }
    }

    void set_svr_starting(bool val)
    {
        svr_starting = val;
    }

    void set_svr_shutting_down(bool val)
    {
        svr_stopping = val;
    }

    std::vector<std::string> &get_polled_dyn_attr_names()
    {
        return polled_dyn_attr_names;
    }

    std::vector<std::string> &get_polled_dyn_cmd_names()
    {
        return polled_dyn_cmd_names;
    }

    std::vector<std::string> &get_full_polled_att_list()
    {
        return polled_att_list;
    }

    std::vector<std::string> &get_full_polled_cmd_list()
    {
        return polled_cmd_list;
    }

    std::string &get_dyn_att_dev_name()
    {
        return dyn_att_dev_name;
    }

    std::string &get_dyn_cmd_dev_name()
    {
        return dyn_cmd_dev_name;
    }

    std::vector<std::string> &get_all_dyn_attr_names()
    {
        return all_dyn_attr;
    }

    void clean_attr_polled_prop();
    void clean_cmd_polled_prop();
    void clean_dyn_attr_prop();

    int create_poll_thread(const char *, bool, bool, int smallest_upd = -1);
    void stop_all_polling_threads();

    std::vector<PollingThreadInfo *> &get_polling_threads_info()
    {
        return poll_ths;
    }

    PollingThreadInfo *get_polling_thread_info_by_id(int);
    int get_polling_thread_id_by_name(const char *);
    void check_pool_conf(DServer *, unsigned long);
    int check_dev_poll(std::vector<std::string> &, std::vector<std::string> &, DeviceImpl *);
    void split_string(const std::string &, char, std::vector<std::string> &);
    void upd_polling_prop(const std::vector<DevDbUpd> &, DServer *);
    int get_th_polled_devs(const std::string &, std::vector<std::string> &);
    void get_th_polled_devs(long, std::vector<std::string> &);
    void build_first_pool_conf(std::vector<std::string> &);
    bool is_dev_already_in_pool_conf(const std::string &, std::vector<std::string> &, int);

    std::vector<std::string> &get_poll_pool_conf()
    {
        return poll_pool_conf;
    }

    int get_dev_entry_in_pool_conf(const std::string &);
    void remove_dev_from_polling_map(const std::string &dev_name);
    void remove_polling_thread_info_by_id(int);

    bool is_server_event_loop_set()
    {
        return ev_loop_func != nullptr;
    }

    void set_shutdown_server(bool val)
    {
        shutdown_server = val;
    }

    void shutdown_ds();

    SubDevDiag &get_sub_dev_diag()
    {
        return sub_dev_diag;
    }

    bool get_endpoint_specified()
    {
        return endpoint_specified;
    }

    void set_endpoint_specified(bool val)
    {
        endpoint_specified = val;
    }

    std::string &get_specified_ip()
    {
        return specified_ip;
    }

    void set_specified_ip(const std::string &val)
    {
        specified_ip = val;
    }

    bool get_endpoint_publish_specified()
    {
        return ext->endpoint_publish_specified;
    }

    void set_endpoint_publish_specified(bool val)
    {
        ext->endpoint_publish_specified = val;
    }

    std::string &get_endpoint_publish()
    {
        return ext->endpoint_publish;
    }

    void set_endpoint_publish(const std::string &val)
    {
        ext->endpoint_publish = val;
    }

    DevLong get_user_pub_hwm()
    {
        return user_pub_hwm;
    }

    void add_restarting_device(const std::string &d_name)
    {
        restarting_devices.push_back(d_name);
    }

    void delete_restarting_device(const std::string &d_name);

    bool is_wattr_nan_allowed()
    {
        return wattr_nan_allowed;
    }

    void set_wattr_nan_allowed(bool val)
    {
        wattr_nan_allowed = val;
    }

    bool is_auto_alarm_on_change_event()
    {
        return auto_alarm_on_change_event;
    }

    void set_auto_alarm_on_change_event(bool val)
    {
        auto_alarm_on_change_event = val;
    }

    RootAttRegistry &get_root_att_reg()
    {
        return root_att_reg;
    }

    void event_name_2_event_type(const std::string &, EventType &);

    void validate_cmd_line_classes();

    static void tango_host_from_fqan(const std::string &, std::string &);
    static void tango_host_from_fqan(const std::string &, std::string &, int &);

    bool is_polling_bef_9_def()
    {
        return polling_bef_9_def;
    }

    bool get_polling_bef_9()
    {
        return polling_bef_9;
    }

    // thread specific storage key accessor for client information/identification
    // return: the (omni) thread specific storage key dedicated to client information/identification
    static omni_thread::key_t get_tssk_client_info()
    {
        return Util::tssk_client_info;
    }

  private:
    TANGO_IMP static Util *_instance;

    static bool _constructed;
#ifdef _TG_WINDOWS_
    static bool _win;
    int argc;
    char **argv;
    int nCmd;
    CoutBuf *pcb;
    W32Win *ds_window;
    std::string main_win_text;
    bool go;
    TangoMonitor mon;

    void build_argc_argv();
    void install_cons_handler();

    class ORBWin32Loop : public omni_thread
    {
        Util *util;

      public:
        ORBWin32Loop(Util *u) :
            util(u)
        {
        }

        virtual ~ORBWin32Loop() { }

        void *run_undetached(void *);

        void start()
        {
            start_undetached();
        }

      private:
        void wait_for_go();
    };
    friend class ORBWin32Loop;
    ORBWin32Loop *loop_th;
#endif

    CORBA::ORB_var orb;
    PortableServer::POA_var _poa;

    std::string ds_instance_name;        // The instance name
    std::string ds_unmodified_exec_name; // The server's unmodified exec. name
    std::string ds_exec_name;            // The server exec. name
    std::string ds_name;                 // The server name

    std::string hostname; // The host name
    std::string pid_str;  // The process PID (as string)
    TangoSys_Pid pid;     // The process PID

    std::string version_str;    // Tango version
    std::string server_version; // Device server version

    std::string database_file_name;

    Database *db{nullptr}; // The db proxy

    void effective_job(int, char *[]);
    void create_CORBA_objects();
    void misc_init();
    void init_host_name();
    void server_perform_work();
    void server_already_running();
    static void print_err_message(const char *, Tango::MessBoxType type = Tango::STOP);

    void print_err_message(const std::string &mess, Tango::MessBoxType type = Tango::STOP)
    {
        print_err_message(mess.c_str(), type);
    }

    void check_args(int, char *[]);
    void print_help_message(bool extended, bool with_database = false);

    DeviceImpl *find_device_name_core(const std::string &);
    void check_orb_endpoint(int, char **);
    void validate_sort(const std::vector<std::string> &);
    void check_end_point_specified(int, char **);

    bool print_help_once_connected;                         // Print the help message once we are connected to db.
    const std::vector<DeviceClass *> *cl_list_ptr{nullptr}; // Ptr to server device class list
    std::unique_ptr<UtilExt> ext;                           // Class extension
    std::vector<DeviceClass *> cl_list;                     // Full class list ptr

    //
    // Ported from the extension class
    //

    std::map<std::string, std::vector<std::string>>
        cmd_line_name_list; // Command line map <Class name, device name list>

    PollThread *heartbeat_th{nullptr};               // The heartbeat thread object
    int heartbeat_th_id{0};                          // The heartbeat thread identifier
    PollThCmd shared_data;                           // The shared buffer
    TangoMonitor poll_mon;                           // The monitor
    bool poll_on{false};                             // Polling on flag
    SerialModel ser_model{BY_DEVICE};                // The serialization model
    TangoMonitor only_one;                           // Serialization monitor
    NotifdEventSupplier *nd_event_supplier{nullptr}; // The notifd event supplier object

    std::shared_ptr<DbServerCache> db_cache; // The db cache
    Interceptors *inter{nullptr};            // The user interceptors

    bool svr_starting{true};  // Server is starting flag
    bool svr_stopping{false}; // Server is shutting down flag

    std::vector<std::string> polled_dyn_attr_names; // Dynamic att. names (used for polling clean-up)
    std::vector<std::string> polled_dyn_cmd_names;  // Dynamic cmd. names (used for polling clean-up)
    std::vector<std::string> polled_att_list;       // Full polled att list
    std::vector<std::string> polled_cmd_list;       // Full polled cmd list
    std::vector<std::string> all_dyn_attr;          // All dynamic attr name list
    std::string dyn_att_dev_name;                   // Device name (use for dyn att clean-up)
    std::string dyn_cmd_dev_name;                   // Device name (use for dyn cmd clean-up)

    unsigned long poll_pool_size;               // Polling threads pool size
    std::vector<std::string> poll_pool_conf;    // Polling threads pool conf.
    std::map<std::string, int> dev_poll_th_map; // Link between device name and polling thread id
    std::vector<PollingThreadInfo *> poll_ths;  // Polling threads
    bool conf_needs_db_upd{false};              // Polling conf needs to be udated in db

    bool (*ev_loop_func)(){nullptr}; // Ptr to user event loop
    bool shutdown_server{false};     // Flag to exit the manual event loop

    SubDevDiag sub_dev_diag;   // Object to handle sub device diagnostics
    bool _dummy_thread{false}; // The main DS thread is not the process main thread

    std::string svr_port_num; // Server port when using file as database

    ZmqEventSupplier *zmq_event_supplier{nullptr}; // The zmq event supplier object
    bool endpoint_specified{false};                // Endpoint specified on cmd line
    std::string specified_ip;                      // IP address specified in the endpoint
    DevLong user_pub_hwm{-1};                      // User defined pub HWM

    std::vector<std::string> restarting_devices; // Restarting devices name
    bool wattr_nan_allowed{false};               // NaN allowed when writing attribute
    RootAttRegistry root_att_reg;                // Root attribute(s) registry

    // If set, then alarm events are automatically pushed to alarm event
    // subscribes when a user calls push_change_event, if is_alarm_event is not
    // set for the attribute.
    bool auto_alarm_on_change_event{true};

    bool polling_bef_9_def{false}; // Is polling algo requirement defined
    bool polling_bef_9;            // use Tango < 9 polling algo. flag

    // thread specific storage key for client info (addr & more)
    //---------------------------------------------------------------------------------
    static omni_thread::key_t tssk_client_info;
};

//***************************************************************************
//
//              Some inline methods
//
//***************************************************************************

//-----------------------------------------------------------------------------
//
// method :         Util::is_device_restarting()
//
// description :     Return a boolean if the device with name given as parameter
//                  is actually executing a RestartDevice command
//
// args: - d_name : - The device name
//
// Returns true if the devce is restarting. False otherwise
//
//-----------------------------------------------------------------------------

inline bool Util::is_device_restarting(const std::string &d_name)
{
    bool ret = false;

    if(!restarting_devices.empty())
    {
        std::vector<std::string>::iterator pos;
        pos = std::find(restarting_devices.begin(), restarting_devices.end(), d_name);
        if(pos != restarting_devices.end())
        {
            ret = true;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// method :         Util::check_orb_endpoint()
//
// description :
//
// args: - argc :
//       - argv
//
//
//-----------------------------------------------------------------------------

inline void Util::check_orb_endpoint(int argc, char *argv[])
{
    long arg_nb;
    for(arg_nb = 2; arg_nb < argc; arg_nb++)
    {
        if(::strcmp(argv[arg_nb], "-ORBendPoint") == 0)
        {
            arg_nb++;
            const char *value = argv[arg_nb];
            if(value == nullptr)
            {
                TangoSys_OMemStream o;

                o << "Missing value for argument -ORBendPoint" << std::ends;
                TANGO_THROW_EXCEPTION(API_InvalidArgs, o.str());
            }

            std::string endpoint = value;
            std::string::size_type pos;
            if((pos = endpoint.rfind(':')) == std::string::npos)
            {
                std::cerr << "Strange ORB endPoint specification" << std::endl;
                print_help_message(false);
            }
            svr_port_num = endpoint.substr(++pos);
            break;
        }
    }
    if(arg_nb == argc)
    {
        std::cerr << "Missing ORB endPoint specification" << std::endl;
        print_help_message(false);
    }
}

//-----------------------------------------------------------------------------
//
// method :         Util::event_name_2_event_type()
//
// description :
//
// args: - event_name :
//       - et
//
//
//-----------------------------------------------------------------------------

inline void Util::event_name_2_event_type(const std::string &event_name, EventType &et)
{
    if(event_name == "change")
    {
        et = CHANGE_EVENT;
    }
    else if(event_name == "alarm")
    {
        et = ALARM_EVENT;
    }
    else if(event_name == "periodic")
    {
        et = PERIODIC_EVENT;
    }
    else if(event_name == "archive")
    {
        et = ARCHIVE_EVENT;
    }
    else if(event_name == "user_event")
    {
        et = USER_EVENT;
    }
    else if(event_name == "attr_conf" || event_name == "attr_conf_5")
    {
        et = ATTR_CONF_EVENT;
    }
    else if(event_name == "data_ready")
    {
        et = DATA_READY_EVENT;
    }
    else if(event_name == "intr_change")
    {
        et = INTERFACE_CHANGE_EVENT;
    }
    else if(event_name == "pipe")
    {
        et = PIPE_EVENT;
    }
    else
    {
        std::stringstream sss;
        sss << "Util::event_name_2_event_type: invalid event name specified ['" << event_name << "' is invalid]";
        TANGO_THROW_EXCEPTION(API_InvalidArgs, sss.str());
    }
}

//+-------------------------------------------------------------------------
//
// function :         return_empty_any
//
// description :     Return from a command when the command does not have
//            any output argument
//
// arguments :         in : - cmd : The command name
//
//--------------------------------------------------------------------------

/**
 * Create and return an empty CORBA Any object.
 *
 * Create an empty CORBA Any object. Could be used by command which does
 * not return anything to the client. This method also prints a message on
 * screen (level 4) before it returns
 *
 * @param cmd The cmd name which use this empty Any. Only used to create the
 * thrown exception (in case of) and in the displayed message
 * @return The empty CORBA Any
 * @exception DevFailed If the Any object creation failed.
 * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a> to
 * read <b>DevFailed</b> exception specification
 */
inline CORBA::Any *return_empty_any(const char *cmd)
{
    CORBA::Any *out_any = nullptr;
    try
    {
        out_any = new CORBA::Any();
    }
    catch(std::bad_alloc &)
    {
        TangoSys_MemStream o;

        o << cmd << "::execute";
        TANGO_THROW_EXCEPTION(API_MemoryAllocation, "Can't allocate memory in server");
    }
    return (out_any);
}

void clear_att_dim(Tango::AttributeValue_3 &att_val);
void clear_att_dim(Tango::AttributeValue_4 &att_val);
void clear_att_dim(Tango::AttributeValue_5 &att_val);

//-----------------------------------------------------------------------
//
//            Polling threads pool related class/struct
//
//-----------------------------------------------------------------------

struct PollingThreadInfo
{
    int thread_id{0};                                // The polling thread identifier
    PollThread *poll_th{nullptr};                    // The polling thread object
    PollThCmd shared_data;                           // The shared buffer
    TangoMonitor poll_mon;                           // The monitor
    std::vector<std::string> polled_devices;         // Polled devices for this thread
    int nb_polled_objects{0};                        // Polled objects number in this thread
    int smallest_upd{0};                             // Smallest thread update period
    std::vector<DevVarLongStringArray *> v_poll_cmd; // Command(s) to send

    PollingThreadInfo() :

        poll_mon("Polling_thread_mon")

    {
        shared_data.cmd_pending = false;
        shared_data.trigger = false;
    }
};

struct DevDbUpd
{
    unsigned long class_ind;
    unsigned long dev_ind;
    int mod_prop;
};

//------------------------------------------------------------------------
//
//            Python device server classes
//
//-----------------------------------------------------------------------

long _convert_tango_lib_release();

// Helper functions to display types, format and so on as strings.
std::ostream &operator<<(std::ostream &o_str, const CmdArgType &type);
std::ostream &operator<<(std::ostream &o_str, const AttrDataFormat &format);
std::ostream &operator<<(std::ostream &o_str, const AttrWriteType &writable);
std::ostream &operator<<(std::ostream &o_str, const PipeWriteType &writable);
std::ostream &operator<<(std::ostream &o_str, const DispLevel &level);
std::ostream &operator<<(std::ostream &o_str, const FwdAttError &fae);
} // namespace Tango

#endif /* UTILS */
