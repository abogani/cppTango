//+===================================================================================================================
//
// file :                Tango_utils.cpp
//
// description :        C++ source for all the utilities used by Tango device server and mainly for the Tango class
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
//-==================================================================================================================

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4290)
#endif

#include <omniORB4/internal/orbOptions.h>
#include <omniORB4/callDescriptor.h>

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#include <tango/server/dserver.h>
#include <tango/server/dserversignal.h>
#include <tango/server/dserverclass.h>
#include <tango/server/eventsupplier.h>
#include <tango/internal/net.h>

#ifndef _TG_WINDOWS_
  #include <unistd.h>
  #include <netdb.h>
  #include <sys/socket.h>
#else
  #include <process.h>
  #include <tango/windows/coutbuf.h>
  #include <tango/windows/ntservice.h>
  #include <ws2tcpip.h>
#endif /* _TG_WINDOWS_ */

#include <tango/client/Database.h>
#include <omniORB4/omniInterceptors.h>
#include <cstdlib>

#include <tango/common/pointer_with_lock.h>

namespace
{

/// Search the environment for the given OMNIORB variable
///
/// Search order:
/// - ARGV
/// - Environment variable
/// - OMNIORB configuration file
std::optional<std::string> get_omniorb_variable(int argc, char *argv[], const std::string &name)
{
    //
    // First look at command line arg
    //

    for(int i = 2; i < argc; i++)
    {
        std::string param = "-ORB" + name;
        if(::strcmp(param.c_str(), argv[i]) == 0)
        {
            const char *value = argv[i + 1];
            if(value == nullptr)
            {
                TangoSys_OMemStream o;

                o << "Missing value for argument " << param << std::ends;
                TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, o.str());
            }

            return Tango::detail::parse_hostname_from_CORBA_URI(value);
        }
    }

    //
    // Then look in env. variables
    //
    Tango::DummyDeviceProxy d;
    std::string env_var;
    std::string param = "ORB" + name;
    if(d.get_env_var(param.c_str(), env_var) == 0)
    {
        return Tango::detail::parse_hostname_from_CORBA_URI(env_var);
    }

    //
    // Finally, look in config file but file name may be specified as command line option or as env. variable!!
    //

    //
    // First get file name
    //

    std::string fname;
    bool found = false;
    for(int i = 2; i < argc; i++)
    {
        if(::strcmp("-ORBconfigFile", argv[i]) == 0)
        {
            const char *value = argv[i + 1];
            if(value == nullptr)
            {
                TangoSys_OMemStream o;

                o << "Missing value for argument -ORBconfigFile" << std::ends;
                TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, o.str());
            }
            fname = value;

            found = true;
            break;
        }
    }

    if(!found)
    {
        Tango::DummyDeviceProxy d;
        std::string env_var;
        if(d.get_env_var("ORBconfigFile", env_var) == 0)
        {
            fname = env_var;
            found = true;
        }
    }

    if(!found)
    {
        fname = Tango::DEFAULT_OMNI_CONF_FILE;
    }

    //
    // Now, look into the file if it exist
    //

    std::string line;
    std::ifstream conf_file(fname.c_str());

    if(conf_file.is_open())
    {
        while(getline(conf_file, line))
        {
            if(line[0] == '#')
            {
                continue;
            }

            std::string::size_type pos = line.find(name);
            if(pos != std::string::npos)
            {
                std::string::iterator ite = remove(line.begin(), line.end(), ' ');
                line.erase(ite, line.end());

                pos = line.find('=');
                if(pos != std::string::npos)
                {
                    std::string value = line.substr(pos + 1);
                    if((pos = value.find('#')) != std::string::npos)
                    {
                        value.erase(pos);
                    }

                    //
                    // Option found in file, extract host ip
                    //
                    auto ip = Tango::detail::parse_hostname_from_CORBA_URI(value);
                    conf_file.close();

                    return ip;
                }
            }
        }
        conf_file.close();
    }
    else
    {
        if(fname != Tango::DEFAULT_OMNI_CONF_FILE)
        {
            std::stringstream ss;
            ss << "Can't open omniORB configuration file (" << fname << ") to check " << name << "option" << std::endl;
            TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, ss.str());
        }
    }

    return std::optional<std::string>();
}

} // anonymous namespace

namespace Tango
{

Util *Util::_instance = nullptr;
int Util::_tracelevel = 0;
bool Util::_UseDb = true;
bool Util::_FileDb = false;
bool Util::_daemon = false;
long Util::_sleep_between_connect = 60;
bool Util::_constructed = false;
#ifdef _TG_WINDOWS_
bool Util::_win = false;
bool Util::_service = false;
#endif

// By default all threads are considered external. Library threads will set
// this global flag to true after construction.
thread_local bool is_tango_library_thread = false;

//+-------------------------------------------------------------------------------------------------------------------
// NOTE: about omni_thread::key_t
//+-------------------------------------------------------------------------------------------------------------------
//    - an omni_thread::key_t is a unique process-wide identifier
//                `--> an omni_thread::key_t must be instantiated by calling 'omni_thread::allocate_key' (thread safe
//                static function).
//                `--> once created, a key is usable by any thread in the process
//    - the value associated with a key is unique (i.e.) and stored on a 'per omni_thread instance' basis
//                `--> omni_thread::[set_value, get_value, remove_value] are NOT static member of the omni_thread
//                `--> omni_thread::[set_value, get_value, remove_value] are NOT thread safe!
//                `--> keys are unique but values are NOT - i.e., two different threads can have a different value
//                associated to the same key
//+-------------------------------------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------------------------------------------------
// tssk_client_info: a omni_thread::key_t used to store/retrieve client info. It is notably used by the blackbox.
//+-------------------------------------------------------------------------------------------------------------------
omni_thread::key_t Util::tssk_client_info = omni_thread::allocate_key();

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::init()
//
// description :
//        static method to create/retrieve the Tango object. This method is the only one which enables a user to
//        create the object
//
// arguments :
//         in :
//            - argc : The command line argument number
//            - argv : The command line argument
//
//------------------------------------------------------------------------------------------------------------------

Util *Util::init(int argc, char *argv[])
{
    if(_instance == nullptr)
    {
        _instance = new Util(argc, argv);
    }
    return _instance;
}

#ifdef _TG_WINDOWS_
Util *Util::init(HINSTANCE hi, int nCmd)
{
    if(_instance == nullptr)
    {
        _instance = new Util(hi, nCmd);
    }
    return _instance;
}
#endif

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::instance()
//
// description :
//        static method to retrieve the Tango object once it has been initialised
//
//-------------------------------------------------------------------------------------------------------------------

Util *Util::instance(bool exit)
{
    if(_instance == nullptr)
    {
        if(exit)
        {
            Util::print_err_message("Tango is not initialised !!!\nExiting");
        }
        else
        {
            TANGO_THROW_EXCEPTION(API_UtilSingletonNotCreated, "Util singleton not created");
        }
    }
    return _instance;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::Util()
//
// description :
//        Constructor of the Tango class.
//
//-----------------------------------------------------------------------------

#ifdef _TG_WINDOWS
Util::Util(int argc, char *argv[]) :
    cl_list_ptr(nullptr),
    mon("Windows startup"),
    ext(new UtilExt),
    heartbeat_th(nullptr),
    heartbeat_th_id(0),
    poll_mon("utils_poll"),
    poll_on(false),
    ser_model(BY_DEVICE),
    only_one("process"),
    nd_event_supplier(nullptr),
    inter(nullptr),
    svr_starting(true),
    svr_stopping(false),
    poll_pool_size(ULONG_MAX),
    conf_needs_db_upd(false),
    ev_loop_func(nullptr),
    shutdown_server(false),
    _dummy_thread(false),
    zmq_event_supplier(nullptr),
    endpoint_specified(false),
    user_pub_hwm(-1),
    wattr_nan_allowed(false),
    polling_bef_9_def(false)
#else
Util::Util(int argc, char *argv[]) :

    ext(new UtilExt),

    poll_mon("utils_poll"),

    only_one("process"),

    poll_pool_size(ULONG_MAX)

#endif
{
    shared_data.cmd_pending = false;
    shared_data.trigger = false;

    //
    // Do the job
    //

    effective_job(argc, argv);
    _constructed = true;

    //
    // For Windows, install a console handler to ignore LOGOFF event
    //

#ifdef _TG_WINDOWS_
    install_cons_handler();
#endif
}

void Util::effective_job(int argc, char *argv[])
{
    try
    {
        //
        // Manage command line option (personal name and -v option)
        //

        check_args(argc, argv);

#if defined(TANGO_USE_TELEMETRY)
        // create a telemetry configuration early on, might throw in case the
        // configuration is invalid
        TANGO_UNUSED(telemetry::Configuration config);
#endif

        //
        // Create the signal object
        // It is necessary to create this object before the ORB is initialised.
        // Otherwise, threads created by thread started by the ORB_init will not have
        // the correct signal mask (set by the DServerSignal object) and the device
        // server signal feature will not work.
        // The call to initialise() will create the background thread that
        // dispatches signals to user-provided handlers.
        //

        DServerSignal::instance()->initialise();

        //
        // Check if the user specified a endPoint on the command line or using one
        // env. variable
        // If true, extract the IP address from the end point and store it
        // for future use in the ZMQ publiher(s)
        //

        check_end_point_specified(argc, argv);

        //
        // Destroy the ORB created as a client (in case there is one)
        // Also destroy database objsect stored in the ApiUtil object. This is needed in case of CS running TAC
        // because the TAC device is stored in the db object and it references the destroyed ORB
        //

        ApiUtil *au = Tango::ApiUtil::instance();
        CORBA::ORB_var orb_clnt = au->get_orb();
        bool log_client_orb_deleted = false;
        if(!CORBA::is_nil(orb_clnt))
        {
            log_client_orb_deleted = true;
            orb_clnt->destroy();
            au->set_orb(CORBA::ORB::_nil());

            size_t nb_db = au->get_db_vect().size();
            for(size_t ctr = 0; ctr < nb_db; ctr++)
            {
                delete au->get_db_vect()[ctr];
            }
            au->get_db_vect().clear();
        }

        //
        // Initialise CORBA ORB
        //

#ifdef _TG_WINDOWS_
        WORD rel = MAKEWORD(2, 2);
        WSADATA dat;
        int err = WSAStartup(rel, &dat);
        if(err != 0)
        {
            std::stringstream ss;
            ss << "WSAStartup function failed with error " << err;
            print_err_message(ss.str());
        }

        if(LOBYTE(dat.wVersion) != 2 || HIBYTE(dat.wVersion) != 2)
        {
            WSACleanup();
            print_err_message("Could not find a usable version of Winsock.dll");
        }
#endif

        if(get_endpoint_specified())
        {
            const char *options[][2] = {{"clientCallTimeOutPeriod", CLNT_TIMEOUT_STR},
                                        {"serverCallTimeOutPeriod", "5000"},
                                        {"maxServerThreadPoolSize", "100"},
                                        {"threadPerConnectionUpperLimit", "55"},
                                        {"threadPerConnectionLowerLimit", "50"},
                                        {"supportCurrent", "0"},
                                        {"verifyObjectExistsAndType", "0"},
                                        {"maxGIOPConnectionPerServer", MAX_GIOP_PER_SERVER},
                                        {"giopMaxMsgSize", MAX_TRANSFER_SIZE},
#ifndef _TG_WINDOWS_
                                        {"endPoint", "giop:unix:"},
#endif
                                        {"throwTransientOnTimeOut", "1"},
                                        {"exceptionIdInAny", "0"},
                                        {nullptr, nullptr}};

            orb = CORBA::ORB_init(argc, argv, "omniORB4", options);
        }
        else
        {
            const char *options[][2] = {{"endPointPublish", "all(addr)"},
                                        {"clientCallTimeOutPeriod", CLNT_TIMEOUT_STR},
                                        {"serverCallTimeOutPeriod", "5000"},
                                        {"maxServerThreadPoolSize", "100"},
                                        {"threadPerConnectionUpperLimit", "55"},
                                        {"threadPerConnectionLowerLimit", "50"},
                                        {"supportCurrent", "0"},
                                        {"verifyObjectExistsAndType", "0"},
                                        {"maxGIOPConnectionPerServer", MAX_GIOP_PER_SERVER},
                                        {"giopMaxMsgSize", MAX_TRANSFER_SIZE},
#ifndef _TG_WINDOWS_
                                        {"endPoint", "giop:tcp::"},
                                        {"endPoint", "giop:unix:"},
#endif
                                        {"throwTransientOnTimeOut", "1"},
                                        {"exceptionIdInAny", "0"},
                                        {nullptr, nullptr}};

            orb = CORBA::ORB_init(argc, argv, "omniORB4", options);
        }

        //
        // Init host name
        //

        init_host_name();

        //
        // Connect to the database
        //

        if(_UseDb)
        {
            connect_db();

            //
            // Display help message if requested by user. This is done after the process is
            // connected to the database becaue a call to the database server is done in the
            // print_help_message() method
            //

            if(print_help_once_connected)
            {
                print_help_message(false, true);
            }
        }
        else
        {
            db = nullptr;
            ApiUtil *au = ApiUtil::instance();
            au->in_server(true);
        }

        //
        // Create the server CORBA objects
        //

        create_CORBA_objects();

        //
        // Initialize logging stuffs
        //
        Logging::init(ds_name, _tracelevel, ((!_FileDb) && _UseDb), db, this);

        if(log_client_orb_deleted)
        {
            TANGO_LOG_INFO << "Client ORB was initialized in before the server ORB, all proxies must be invalidated"
                           << std::endl;
        }
        TANGO_LOG_DEBUG << "Connected to database" << std::endl;
        if(!get_db_cache())
        {
            TANGO_LOG_DEBUG << "DbServerCache unavailable, will call db..." << std::endl;
        }

        //
        // Check if the server is not already running somewhere else
        //

        if((_UseDb) && (!_FileDb))
        {
            server_already_running();
        }

        //
        // Get process PID and Tango version
        //

        misc_init();

        //
        // Automatically create the EventSupplier objects
        //
        // In the future this could be created only when the
        // first event is fired ...
        //

        create_notifd_event_supplier();
        create_zmq_event_supplier();

        //
        // Create the heartbeat thread and start it
        //

        heartbeat_th = new PollThread(shared_data, poll_mon, true);
        heartbeat_th->start();
        heartbeat_th_id = heartbeat_th->id();
        TANGO_LOG_DEBUG << "Heartbeat thread Id = " << heartbeat_th_id << std::endl;

        TANGO_LOG_DEBUG << "Tango object singleton constructed" << std::endl;
    }
    catch(CORBA::Exception &)
    {
        throw;
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::create_CORBA_objects()
//
// description :
//        Create some CORBA objects needed later-on
//
//-------------------------------------------------------------------------------------------------------------------

void Util::create_CORBA_objects()
{
    //
    // Install an omniORB interceptors to store client name in blackbox and allocate a key for per thread specific
    // storage
    //

    omni::omniInterceptors *intercep = omniORB::getInterceptors();

    // install the client call interceptor
    // works for both colocated and remote calls so that the client info is properly setup in any case
    // see section 10.3 of the omniORB documentation for details
    // ---------------------------------------------------------------------------
    // since the adoption of omniORB 4.3, it is used for both local and remote calls.
    // ---------------------------------------------------------------------------
    omniCallDescriptor::addInterceptor(Tango::client_call_interceptor);

    intercep->createThread.add(
        [](omni::omniInterceptors::createThread_T::info_T &info)
        {
            // Mark this thread as a library thread. This will allow setting
            // associated device name in SubDevDiag.
            is_tango_library_thread = true;

            // Try accessing the Tango-level thread creation/deletion interceptors.
            // They may be optionally set by the application, otherwise the pointer
            // will be null.
            Interceptors *interceptors = nullptr;
            try
            {
                Util *tg = Util::instance(false);
                interceptors = tg->get_interceptors();
            }
            catch(const Tango::DevFailed &)
            {
                // Silently ignore all errors here. Util::instance will throw an
                // exception if the global instance pointer was not yet set (and
                // the constructor is still running). This could happen with the
                // omniORB's socket-watching threads that start early during ORB
                // initialization. Not calling the interceptors for such threads
                // is fine since those threads never call user code directly.
            }

            if(interceptors != nullptr)
            {
                interceptors->create_thread();
            }

            // Run the thread and wait until it exists. This is mandatory.
            info.run();

            if(interceptors != nullptr)
            {
                interceptors->delete_thread();
            }
        });

    //
    // Get some CORBA object references
    //

    CORBA::Object_var poaObj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var root_poa = PortableServer::POA::_narrow(poaObj);

    //
    // If the database is not used, we must used the omniINSPOA poa in both cases of database on file or nodb
    // remember that when you have database on file you have _UseDb == true and  _FileDb == true
    //

    PortableServer::POA_var nodb_poa;
    if((!_UseDb) || (_FileDb))
    {
        CORBA::Object_var poaInsObj = orb->resolve_initial_references("omniINSPOA");
        nodb_poa = PortableServer::POA::_narrow(poaInsObj);
    }

    //
    // Store POA. This is the same test but inverted
    //

    if((_UseDb) && (!_FileDb))
    {
        _poa = PortableServer::POA::_duplicate(root_poa);
    }
    else if((!_UseDb) || (_FileDb))
    {
        _poa = PortableServer::POA::_duplicate(nodb_poa);
    }
}

#ifdef _TG_WINDOWS_
//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::Util()
//
// description :
//        Constructor of the Tango class when used in a non-console Windows device server. On top of the UNIX way of
//        building a Util singleton, for Windows non-console mode, it is necessary to :
//         - Build a UNIX like argc,argv from the command line
//         - Initialise the ORB
//         - Create a debug output window if verbose mode is requested and change TANGO_LOG so that it prints into this
//             window
//
// arguments :
//         in :
//            - hInst : The application instance
//            - nCmdShow : The display window flag
//
//-------------------------------------------------------------------------------------------------------------------

Util::Util(HINSTANCE hInst, int nCmdShow) :
    cl_list_ptr(nullptr),
    mon("Windows startup"),
    ext(new UtilExt),
    heartbeat_th(nullptr),
    heartbeat_th_id(0),
    poll_mon("utils_poll"),
    poll_on(false),
    ser_model(BY_DEVICE),
    only_one("process"),
    nd_event_supplier(nullptr),
    inter(nullptr),
    svr_starting(true),
    svr_stopping(false),
    poll_pool_size(ULONG_MAX),
    conf_needs_db_upd(false),
    ev_loop_func(nullptr),
    shutdown_server(false),
    _dummy_thread(false),
    zmq_event_supplier(nullptr),
    endpoint_specified(false),
    user_pub_hwm(-1),
    wattr_nan_allowed(false),
    polling_bef_9_def(false)
{
    //
    // This method should be called from a Windows graphic program
    //

    _win = true;

    //
    // Some more init
    //

    shared_data.cmd_pending = false;
    shared_data.trigger = false;

    //
    // Build UNIX like command argument(s)
    //

    build_argc_argv();

    //
    // Really do the job now
    //

    effective_job(argc, argv);

    //
    // Store the nCmdShow parameter an mark the object has being completely
    // constructed. Usefull, in case one of the method previously called
    // failed, the orb variable (type ORB_var) is alaready destroyed therefore
    // releasing the orb pointer.
    //

    nCmd = nCmdShow;
    _constructed = true;
}
#endif

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::ckeck_args()
//
// description :
//        Check the command line arguments. The first one is mandatory and is the server personal name. A -v option
//        is authorized with an optional argument. The other option should be ORBacus option
//
// arguments :
//         in :
//            - argc : The command line argument number
//            - argv : The command line argument
//
//-------------------------------------------------------------------------------------------------------------------

void Util::check_args(int argc, char *argv[])
{
    // Early check that we at least have a program name...
    if(argc < 1)
    {
        ds_unmodified_exec_name = "?";
        print_help_message(true);
    }

    // Keep an unmodified copy of the executable for later use
    // with realpath from the std C library. realpath resolves
    // the full path for a given filename.
    ds_unmodified_exec_name = argv[0];

    // Start by parsing the executable name, this has been moved at the beginning of the
    // function because the print_help_message function relies on ds_unmodified_exec_name.
    char *tmp;
#ifdef _TG_WINDOWS_
    if((tmp = strrchr(argv[0], '\\')) == 0)
    {
        if((tmp = strrchr(argv[0], '/')) == 0)
        {
            ds_exec_name = argv[0];
        }
        else
        {
            tmp++;
            ds_exec_name = tmp;
        }
    }
    else
    {
        tmp++;
        ds_exec_name = tmp;
    }
#else
    if((tmp = strrchr(argv[0], '/')) == nullptr)
    {
        ds_exec_name = argv[0];
    }
    else
    {
        tmp++;
        ds_exec_name = tmp;
    }
#endif

    //
    // For Windows only. Remove the .exe after the executable name
    //

#ifdef _TG_WINDOWS_
    std::string::size_type pos;
    if((pos = ds_exec_name.find('.')) != std::string::npos)
    {
        ds_exec_name.erase(pos, ds_exec_name.size());
    }
#endif /* _TG_WINDOWS_ */

    // If we don't have an argument for the instance, print the help message early.
    if(argc < 2)
    {
        print_help_message(true);
    }

    // Now that we've stored the exec name, we decode the first argument which is either
    // the instance name, a help flag. If this is a help flag, we set the display usage
    std::string first_arg(argv[1]);
    print_help_once_connected = false;

    // For example, the Database DS sets _UseDb to false to disable -?, -h, -help.
    if((argc == 2) && (_UseDb))
    {
        if((first_arg == "-?") || (first_arg == "-help") || (first_arg == "-h"))
        {
            print_help_once_connected = true;
        }
    }

    if((!print_help_once_connected) && (argv[1][0] == '-'))
    {
        print_help_message(false);
    }
    ds_instance_name = argv[1];

    if(argc > 2)
    {
        long ind = 2;
        std::string dlist;
        while(ind < argc)
        {
            if(argv[ind][0] == '-')
            {
                switch(argv[ind][1])
                {
                    //
                    // The verbose option
                    //

                case 'v':
                    if(strlen(argv[ind]) == 2)
                    {
                        if((argc - 1) > ind)
                        {
                            if(argv[ind + 1][0] == '-')
                            {
                                set_trace_level(4);
                            }
                            else
                            {
                                std::cerr << "Unknown option " << argv[ind] << std::endl;
                                print_help_message(false);
                            }
                        }
                        else
                        {
                            set_trace_level(4);
                        }
                        ind++;
                    }
                    else
                    {
                        long level = atol(&(argv[ind][2]));
                        if(level == 0)
                        {
                            std::cerr << "Unknown option " << argv[ind] << std::endl;
                            print_help_message(false);
                        }
                        else
                        {
                            set_trace_level(level);
                            ind++;
                        }
                    }
                    break;

                    //
                    // Device server without database
                    //

                case 'n':
                    if(strcmp(argv[ind], "-nodb") != 0)
                    {
                        std::cerr << "Unknown option " << argv[ind] << std::endl;
                        print_help_message(false);
                    }
                    else
                    {
                        if(_FileDb)
                        {
                            print_help_message(false);
                        }
                        _UseDb = false;
                        ind++;

                        check_orb_endpoint(argc, argv);
                    }
                    break;

                case 'f':
                    if(strncmp(argv[ind], "-file=", 6) != 0)
                    {
                        std::cerr << "Unknown option " << argv[ind] << std::endl;
                        print_help_message(false);
                    }
                    else
                    {
                        if(!_UseDb)
                        {
                            print_help_message(false);
                        }
                        Tango::Util::_FileDb = true;
                        database_file_name = argv[ind];
                        database_file_name.erase(0, 6);
#ifdef _TG_WINDOWS_
                        replace(database_file_name.begin(), database_file_name.end(), '\\', '/');
#endif
                        TANGO_LOG_DEBUG << "File name = <" << database_file_name << ">" << std::endl;
                        ind++;

                        //
                        // Try to find the ORB endPoint option
                        //

                        check_orb_endpoint(argc, argv);
                    }
                    break;

                    //
                    // Device list (for device server without database)
                    //

                case 'd':
                    if(strcmp(argv[ind], "-dbg") == 0)
                    {
                        ind++;
                        break;
                    }

                    if(strcmp(argv[ind], "-dlist") != 0)
                    {
                        std::cerr << "Unknown option " << argv[ind] << std::endl;
                        print_help_message(false);
                    }
                    else
                    {
                        if(_UseDb)
                        {
                            print_help_message(false);
                        }

                        ind++;
                        if(ind == argc)
                        {
                            print_help_message(false);
                        }
                        else
                        {
                            dlist = argv[ind];

                            //
                            // Extract each device name
                            //

                            std::string::size_type start = 0;
                            std::string str;
                            std::string::size_type pos;
                            // vector<std::string> &list = get_cmd_line_name_list();
                            std::vector<std::string> dev_list;

                            while((pos = dlist.find(',', start)) != std::string::npos)
                            {
                                str = dlist.substr(start, pos - start);
                                start = pos + 1;
                                dev_list.push_back(str);
                            }
                            if(start != dlist.size())
                            {
                                str = dlist.substr(start);
                                dev_list.push_back(str);
                            }

                            validate_sort(dev_list);
                        }
                    }
                    break;

                default:
                    ind++;
                    break;
                }
            }
            else
            {
                if(strncmp(argv[ind - 1], "-v", 2) == 0)
                {
                    print_help_message(false);
                }
                ind++;
            }
        }
    }

    //
    // Build server name
    //

    //    long ctr;
    //    for (ctr = 0;ctr < ds_exec_name.size();ctr++)
    //        ds_exec_name[ctr] = std::tolower(ds_exec_name[ctr]);
    //    for (ctr = 0;ctr < ds_instance_name.size();ctr++)
    //        ds_instance_name[ctr] = std::tolower(ds_instance_name[ctr]);
    ds_name = ds_exec_name;
    ds_name.append("/");
    ds_name.append(ds_instance_name);

    //
    // Check that the server name is not too long
    //

    if(ds_name.size() > MaxServerNameLength)
    {
        TangoSys_OMemStream o;

        o << "The device server name is too long! Max length is " << MaxServerNameLength << " characters" << std::ends;
        print_err_message(o.str(), Tango::INFO);
    }

    //
    // If the server is the database server (DS started with _UseDb set to false but without the
    // nodb option), we need its port
    //

    if((!_UseDb) && (svr_port_num.empty()))
    {
        check_orb_endpoint(argc, argv);
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::validate_sort
//
// description :
//        Validate and sort the list of devices passed by the user to the DS in the command line
//
// arguments :
//        in :
//            - dev_list : Vector with device name list
//
//-------------------------------------------------------------------------------------------------------------------

void Util::validate_sort(const std::vector<std::string> &dev_list)
{
    unsigned long i, j;
    std::vector<std::string> dev_no_class;

    //
    //  First, create a vector with device name in lower case letters without class name
    //

    for(i = 0; i < dev_list.size(); i++)
    {
        std::string tmp(dev_list[i]);
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

        std::string::size_type pos = tmp.find("::");
        if(pos == std::string::npos)
        {
            dev_no_class.push_back(tmp);
        }
        else
        {
            std::string tmp_no_class = tmp.substr(pos + 2);
            dev_no_class.push_back(tmp_no_class);
        }
    }

    //
    // Check that the same device name is not used twice and check that their syntax is correct (2 / characters)
    //

    for(i = 0; i < dev_no_class.size(); i++)
    {
        int nb_char = count(dev_no_class[i].begin(), dev_no_class[i].end(), '/');
        if(nb_char != 2)
        {
            std::stringstream ss;
            ss << "Device name " << dev_list[i]
               << " does not have the correct syntax (domain/family/member or ClassName::domain/family/member)";
            print_err_message(ss.str());
        }
        for(j = 0; j < dev_no_class.size(); j++)
        {
            if(i == j)
            {
                continue;
            }
            else
            {
                if(dev_no_class[i] == dev_no_class[j])
                {
                    print_err_message("Each device must have different name", Tango::INFO);
                }
            }
        }
    }

    //
    // Sort this device name list and store them in a map according to the class name. Command line device name
    // syntax is "ClassName::device_name" eg: MyClass::my/device/name
    //

    std::map<std::string, std::vector<std::string>> &the_map = get_cmd_line_name_list();
    std::string::size_type pos;

    for(i = 0; i < dev_list.size(); i++)
    {
        pos = dev_list[i].find("::");
        if(pos == std::string::npos)
        {
            auto ite = the_map.find(NoClass);
            if(ite == the_map.end())
            {
                std::vector<std::string> v_s;
                v_s.push_back(dev_list[i]);
                the_map.insert(std::pair<std::string, std::vector<std::string>>(NoClass, v_s));
            }
            else
            {
                ite->second.push_back(dev_list[i]);
            }
        }
        else
        {
            std::string cl_name = dev_list[i].substr(0, pos);
            std::transform(cl_name.begin(), cl_name.end(), cl_name.begin(), ::tolower);

            auto ite = the_map.find(cl_name);
            if(ite == the_map.end())
            {
                std::vector<std::string> v_s;
                std::string d_name = dev_list[i].substr(pos + 2);
                v_s.push_back(d_name);
                the_map.insert(std::pair<std::string, std::vector<std::string>>(cl_name, v_s));
            }
            else
            {
                std::string d_name = dev_list[i].substr(pos + 2);
                ite->second.push_back(d_name);
            }
        }
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::print_help_message()
//
// description :
//        Print the help message that describe the usage of the device server command line.
//
// arguments :
//         in :
//            - extended : True to get the extended help with all options descriptions.
//            - with_database : True to print the list of server instances at the end, this requires a connected db.
//
//-------------------------------------------------------------------------------------------------------------------

void Util::print_help_message(bool extended, bool with_database)
{
    TangoSys_OMemStream o;

    if(extended)
    {
        o << "Usage: " << ds_unmodified_exec_name << " <INSTANCE_NAME> [OPTIONS]\n";
        o << "\n";
        o << "Start this Tango device server. By default it connects to a database specified\n";
        o << "in the TANGO_HOST environment variable or the configuration files:\n";
#ifdef _TG_WINDOWS_
        o << "%" << WindowsEnvVariable << "%/" << WINDOWS_ENV_VAR_FILE << "\n";
#else
        o << "~/" << USER_ENV_VAR_FILE << " or " << TANGO_RC_FILE << "\n";
#endif
        o << "\n";
        o << "Arguments:\n";
        o << "  <INSTANCE_NAME>\n";
        o << "      Name of this device server instance. When using a database, this must\n";
        o << "      match a registered instance name that is not yet running.\n";
        o << "\n";
        o << "Options:\n";
        o << "  -?, -h, -help\n";
        o << "      Print only usage followed by the list of registered instance names.\n";
        o << "  -v[LEVEL]\n";
        o << "      Enable verbose output, with optional level number: 1 or 2 for INFO, 3 and\n";
        o << "      more for DEBUG. Defaults to OFF.\n";
        o << "  -nodb\n";
        o << "      Do not connect to a database. This also requires an explicit device list\n";
        o << "      given by -dlist option. This is incompatible with -file option. Example:\n";
        o << "      $ /usr/local/bin/TangoTest test -nodb -dlist sys/test/1\n";
        o << "        -ORBendPoint giop:tcp:localhost:12345\n";
        o << "  -dlist <DEV>[,...]\n";
        o << "      Explicit comma-separated list of devices to create when -nodb is used.\n";
        o << "  -file=<PATH>\n";
        o << "      Do not connect to a database and use a file database. This is not\n";
        o << "      compatible with -nodb option.\n";
        o << "  -ORBendPoint <ENDPOINT>\n";
        o << "      Explicitly specify the ORB endpoint. For example 'giop:tcp:[host]:[port]'\n";
        o << "      that determines the network interface and port which the server listens on\n";
        o << "      and the service port. Defaults to 'giop:tcp::', listening every interface\n";
        o << "      and OS chooses a port. See omniORB doc Chapter 7.\n";
        o << "  -ORBendPointPublish <RULES>\n";
        o << "      By default, the OBR publishes in its IORs the first available address for\n";
        o << "      each endpoints the server listens on. This option can be used to selects\n";
        o << "      which endpoints are published. Specifying a full endpoint URI forces it\n";
        o << "      to be published, even if the server is not listening it. More on rules in\n";
        o << "      omniORB doc Chapter 7.\n";
    }
    else
    {
        o << "Usage: " << ds_unmodified_exec_name << " <INSTANCE_NAME> [-?|-h|-help] [-v[LEVEL]]\n";
        o << "                  [-nodb [-dlist <DEV>,...] | -file=<PATH>]\n";
        o << "                  [-ORBendPoint <ENDPOINT>] [-ORBendPointPublish <RULES>]";
    }

    // This function should only ever be called with 'with_database' if connected to db,
    // but we double check that here to avoid null deref.
    if(db != nullptr && with_database)
    {
        std::string str("dserver/");
        str.append(ds_exec_name);
        str.append("/*");

        std::vector<std::string> db_inst;

        try
        {
            DbDatum received = db->get_device_member(str);
            received >> db_inst;
        }
        catch(Tango::DevFailed &e)
        {
            std::string reason(e.errors[0].reason.in());
            if(reason == API_ReadOnlyMode)
            {
                o << "\nWarning: Control System configured with AccessControl but can't communicate with AccessControl "
                     "server";
            }
            o << std::ends;
            print_err_message(o.str(), Tango::INFO);
        }

        //
        // Add instance name list to message
        //

        o << "\nRegistered instance names in the database:";

        if(db_inst.empty())
        {
            o << " none yet.\n";
        }
        else
        {
            o << "\n";
            for(unsigned long i = 0; i < db_inst.size(); i++)
            {
                o << "  " << db_inst[i] << "\n";
            }
        }
    }

    o << std::ends;
    print_err_message(o.str(), Tango::INFO);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::connect_db()
//
// description :
//        This method builds a connection to the Tango database servant. It uses the db_host and db_port object
//        variables. The Tango database server implements its CORBA object as named servant.
//
//--------------------------------------------------------------------------------------------------------------------

void Util::connect_db()
{
    //
    // Try to create the Database object
    //

    if(_daemon)
    {
        bool connected = false;
        while(!connected)
        {
            try
            {
#ifdef _TG_WINDOWS_
                if(_service == true)
                {
                    db = new Database(orb.in(), ds_exec_name, ds_instance_name);
                }
                else
                {
                    if(_FileDb == false)
                    {
                        db = new Database(orb.in());
                    }
                    else
                    {
                        db = new Database(database_file_name);
                    }
                }
#else
                if(!_FileDb)
                {
                    db = new Database(orb.in());
                }
                else
                {
                    db = new Database(database_file_name);
                }
#endif
                db->set_tango_utils(this);
                connected = true;
            }
            catch(Tango::DevFailed &e)
            {
                if(strcmp(e.errors[0].reason.in(), API_TangoHostNotSet) == 0)
                {
                    print_err_message(e.errors[0].desc.in());
                }
                else
                {
                    TANGO_LOG_DEBUG << "Can't contact db server, will try later" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(_sleep_between_connect));
                }
            }
            catch(CORBA::Exception &)
            {
                TANGO_LOG_DEBUG << "Can't contact db server, will try later" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(_sleep_between_connect));
            }
        }
    }
    else
    {
        try
        {
#ifdef _TG_WINDOWS_
            if(_service == true)
            {
                db = new Database(orb.in(), ds_exec_name, ds_instance_name);
            }
            else
            {
                if(_FileDb == false)
                {
                    db = new Database(orb.in());
                }
                else
                {
                    db = new Database(database_file_name);
                }
            }
#else
            if(!_FileDb)
            {
                db = new Database(orb.in());
            }
            else
            {
                db = new Database(database_file_name);
            }
#endif
            db->set_tango_utils(this);
        }
        catch(Tango::DevFailed &e)
        {
            if(e.errors.length() == 2)
            {
                if(strcmp(e.errors[1].reason.in(), API_CantConnectToDatabase) == 0)
                {
                    TangoSys_OMemStream o;

                    o << "Can't build connection to TANGO database server, exiting";
                    print_err_message(o.str());
                }
                else
                {
                    print_err_message(e.errors[1].desc.in());
                }
            }
            else
            {
                print_err_message(e.errors[0].desc.in());
            }
        }
        catch(CORBA::Exception &)
        {
            TangoSys_OMemStream o;

            o << "Can't build connection to TANGO database server, exiting";
            print_err_message(o.str());
        }
    }

    if(CORBA::is_nil(db->get_dbase()) && !_FileDb)
    {
        TangoSys_OMemStream o;

        o << "Can't build connection to TANGO database server, exiting" << std::ends;
        print_err_message(o.str());
    }

    //
    // Set a timeout on the database device
    //

    if(!_FileDb)
    {
        db->set_timeout_millis(DB_TIMEOUT);
    }

    //
    // Also copy this database object ptr into the ApiUtil object. Therefore,
    // the same database connection will be used also for DeviceProxy
    // object created within the server
    //

    ApiUtil *au = ApiUtil::instance();
    au->get_db_vect().push_back(db);
    au->in_server(true);

    //
    // Try to create the db cache which will be used during the process
    // startup sequence
    // For servers with many devices and properties, this could take time
    // specially during a massive DS startup (after power cut for instance)
    // Filling the DS cache is a relatively heavy command for the DB server
    // Trying to minimize retry in this case could be a good idea.
    // Therefore, change DB device timeout to execute this command
    //

    if(!_FileDb)
    {
        std::string &inst_name = get_ds_inst_name();
        if(inst_name != "-?")
        {
            db->set_timeout_millis(DB_TIMEOUT * 4);
            set_svr_starting(false);
            try
            {
                db_cache = std::make_shared<DbServerCache>(db, get_ds_name(), get_host_name());
            }
            catch(Tango::DevFailed &e)
            {
                std::string base_desc(e.errors[0].desc.in());
                if(base_desc.find("TRANSIENT_CallTimedout") != std::string::npos)
                {
                    std::cerr << "DB timeout while trying to fill the DB server cache. Will use traditional way"
                              << std::endl;
                }
            }
            catch(...)
            {
                std::cerr << "Unknown exception while trying to fill database cache..." << std::endl;
            }
            db->set_timeout_millis(DB_TIMEOUT);
            set_svr_starting(true);
        }
    }
}

void Util::reset_filedatabase()
{
    delete db;
    db = new Database(database_file_name);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::misc_init()
//
// description :
//        This method initialises miscellaneous variable which are needed later in the device server startup sequence.
//        These variables are :
//                The process ID
//                The Tango version
//
//-------------------------------------------------------------------------------------------------------------------

void Util::misc_init()
{
    //
    // Get PID
    //
#ifdef _TG_WINDOWS_
    pid = _getpid();
#else
    pid = getpid();
#endif
    pid_str = std::to_string(pid);

    //
    // Convert Tango version number to string (for device export)
    //
    version_str = std::to_string(DevVersion);

    //
    // Init server version to a default value
    //

    server_version = "x.y";

    //
    // Init text to be displayed on main window with a default value
    //

#ifdef _TG_WINDOWS_
    main_win_text = "TANGO collaboration\n";
    main_win_text = main_win_text + "http://www.tango-controls.org";
#endif

    //
    // Check if the user has defined his own publisher hwm (for zmq event tuning)
    //

    std::string var;
    if(ApiUtil::get_env_var("TANGO_DS_EVENT_BUFFER_HWM", var) == 0)
    {
        int pub_hwm = -1;
        std::istringstream iss(var);
        iss >> pub_hwm;
        if(iss)
        {
            user_pub_hwm = pub_hwm;
        }
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::init_host_name()
//
// description :
//        This method initialises the process host name which is needed later in the device server startup sequence.
//
//-------------------------------------------------------------------------------------------------------------------

void Util::init_host_name()
{
    //
    // Get the FQDN host name (Fully qualified domain name)
    // If it is not returned by the system call "gethostname",
    // try with the getnameinfo/getaddrinfo system calls providing
    // IP address obtained by calling ApiUtil::get_ip_from_if()
    //
    // All supported OS have the getaddrinfo() call
    //
    char buffer[Tango::detail::TANGO_MAX_HOSTNAME_LEN];
    if(gethostname(buffer, Tango::detail::TANGO_MAX_HOSTNAME_LEN) == 0)
    {
        hostname = buffer;
        std::transform(hostname.begin(),
                       hostname.end(),
                       hostname.begin(),
                       ::tolower); // to retain consistency with getnameinfo() which always returns lowercase

        std::string::size_type pos = hostname.find('.');

        if(pos == std::string::npos)
        {
            struct addrinfo hints;

            memset(&hints, 0, sizeof(struct addrinfo));

            hints.ai_family = AF_UNSPEC; // supports both IPv4 and IPv6
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags =
                AI_NUMERICHOST; // inhibits resolution of node parameter if it is not a numeric network address
            hints.ai_flags |= AI_ADDRCONFIG;

            struct addrinfo *info, *ptr;
            char tmp_host[NI_MAXHOST];
            bool host_found = false;
            std::vector<std::string> host_names;

            ApiUtil *au = ApiUtil::instance();
            std::vector<std::string> ip_list;
            au->get_ip_from_if(ip_list); // returns a list of numeric network addresses

            for(size_t i = 0; i < ip_list.size() && !host_found; i++)
            {
                if(getaddrinfo(ip_list[i].c_str(), nullptr, &hints, &info) == 0)
                {
                    ptr = info;
                    while(ptr != nullptr)
                    {
                        if(getnameinfo(ptr->ai_addr, ptr->ai_addrlen, tmp_host, NI_MAXHOST, nullptr, 0, NI_NAMEREQD) ==
                           0)
                        {
                            std::string myhost(tmp_host);
#ifdef _TG_WINDOWS_
                            //
                            // On windows, getnameinfo may return name in uppercase letters
                            //
                            std::transform(myhost.begin(), myhost.end(), myhost.begin(), ::tolower);
#endif
                            std::string::size_type pos = myhost.find('.');
                            if(pos != std::string::npos)
                            {
                                host_names.push_back(myhost);
                            }
                        }
                        ptr = ptr->ai_next;
                    }
                    freeaddrinfo(info);
                }
            }

            // Several cases to find out the real name of the network interface on which the server is running.
            // First be sure that we got at least one name from call(s) to getnameinfo()
            // Then, we have to know if the -ORBendPoint option was specified (command line/environment/config file)
            // If it was not, select the name which is the same than the computer hostname. If not found, select the
            // first name returned by getnameinfo() If the endPoint option is used by the host name is not specified
            // within the option (Db case for instance), do the same than in previous case If a host is specified in
            // endPoint option: If it is specified as a name, search for this name in the list returned by getnameinfo
            // calls and select it If it is specified as a IP address, search for this IP in ip_list vector and select
            // this name in the result of getnameinfo() call

            if(host_names.size() != 0)
            {
                if(!get_endpoint_specified())
                {
                    bool found = false;
                    for(size_t loop = 0; loop < host_names.size(); loop++)
                    {
                        std::string::size_type pos = host_names[loop].find('.');
                        std::string canon = host_names[loop].substr(0, pos);
                        if(hostname == canon)
                        {
                            found = true;
                            hostname = host_names[loop];
                            break;
                        }
                    }

                    if(!found)
                    {
                        hostname = host_names[0];
                    }
                }
                else
                {
                    std::string &spec_ip = get_specified_ip();
                    if(spec_ip.empty())
                    {
                        bool found = false;
                        for(size_t loop = 0; loop < host_names.size(); loop++)
                        {
                            std::string::size_type pos = host_names[loop].find('.');
                            std::string canon = host_names[loop].substr(0, pos);
                            if(hostname == canon)
                            {
                                found = true;
                                hostname = host_names[loop];
                                break;
                            }
                        }

                        if(!found)
                        {
                            hostname = host_names[0];
                        }
                    }
                    else
                    {
                        int nb_dot = count(spec_ip.begin(), spec_ip.end(), '.');
                        if(nb_dot == 3)
                        {
                            int ind = -1;
                            for(int loop = 0; loop < (int) ip_list.size(); loop++)
                            {
                                if(ip_list[loop] == spec_ip)
                                {
                                    ind = loop;
                                    break;
                                }
                            }

                            if(ind != -1 && (((int) host_names.size() - 1) >= ind))
                            {
                                hostname = host_names[ind];
                            }
                        }
                        else
                        {
                            std::string::size_type pos = spec_ip.find('.');
                            std::string canon_spec;
                            if(pos != std::string::npos)
                            {
                                canon_spec = spec_ip.substr(0, pos);
                            }
                            else
                            {
                                canon_spec = spec_ip;
                            }

                            for(size_t loop = 0; loop < host_names.size(); loop++)
                            {
                                std::string::size_type pos = host_names[loop].find('.');
                                std::string canon = host_names[loop].substr(0, pos);
                                if(canon_spec == canon)
                                {
                                    hostname = host_names[loop];
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        print_err_message("Cant retrieve server host name");
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::create_notifd_event_supplier()
//
// description :
//        This method create the notifd event_supplier if possible
//
//-------------------------------------------------------------------------------------------------------------------

void Util::create_notifd_event_supplier()
{
    if(_UseDb)
    {
        try
        {
            nd_event_supplier = NotifdEventSupplier::create(orb, ds_name, this);
            nd_event_supplier->connect();
        }
        catch(...)
        {
            nd_event_supplier = nullptr;
            if(_FileDb)
            {
                TANGO_LOG_ERROR << "Can't create notifd event supplier. Notifd event not available" << std::endl;
            }
        }
    }
    else
    {
        nd_event_supplier = nullptr;
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::create_zmq_event_supplier()
//
// description :
//        This method create the zmq event_supplier if possible
//
//-------------------------------------------------------------------------------------------------------------------

void Util::create_zmq_event_supplier()
{
    try
    {
        zmq_event_supplier = ZmqEventSupplier::create(this);
    }
    catch(...)
    {
        zmq_event_supplier = nullptr;
        if(_FileDb)
        {
            TANGO_LOG_ERROR << "Can't create zmq event supplier. Zmq event not available" << std::endl;
        }
        throw;
    }
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::~Util()
//
// description :
//        Tango singleton object destructor. This destructor shutdown everything before the process dies. This means
//          - Send kill command to the polling thread
//            - Join with this polling thread
//            - Unregister server in database
//            - Delete devices (except the admin one)
//            - Shutdown the ORB
//            - Cleanup Logging
//
//-------------------------------------------------------------------------------------------------------------------

Util::~Util()
{
#ifdef _TG_WINDOWS_
    if(ds_window != nullptr)
    {
        stop_all_polling_threads();
        stop_heartbeat_thread();
        clr_heartbeat_th_ptr();

        unregister_server();
        get_dserver_device()->delete_devices();
        if(_FileDb == true)
        {
            delete db;
        }
        orb->shutdown(true);
        // JM : 9.8.2005 : destroy() should be called at the exit of run()!
        // orb->destroy();
        Logging::cleanup();
    }
#endif
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::server_already_running()
//
// description :
//        Check if the same device server is not already running somewhere else and refuse to start in this case
//
//-------------------------------------------------------------------------------------------------------------------

void Util::server_already_running()
{
    TANGO_LOG_DEBUG << "Entering Util::server_already_running method" << std::endl;

    //
    // Build device name and try to import it from database or from cache if available
    //

    std::string dev_name(DSDeviceDomain);
    dev_name.append(1, '/');
    dev_name.append(ds_name);

    Tango::Device_var dev;

    try
    {
        const Tango::DevVarLongStringArray *db_dev;
        CORBA::Any_var received;
        if(db_cache)
        {
            db_dev = db_cache->import_adm_dev();
        }
        else
        {
            CORBA::Any send;
            send <<= dev_name.c_str();

            received = db->get_dbase()->command_inout("DbImportDevice", send);
            if(!(received.inout() >>= db_dev))
            {
                TangoSys_OMemStream o;
                o << "Database error while trying to import " << dev_name << std::ends;

                TANGO_THROW_EXCEPTION(API_DatabaseAccess, o.str());
            }
        }

        //
        // If the device is not imported, leave function
        //

        if((db_dev->lvalue)[0] == 0)
        {
            TANGO_LOG_DEBUG << "Leaving Util::server_already_running method" << std::endl;
            return;
        }

        CORBA::Object_var obj = orb->string_to_object((db_dev->svalue)[1]);

        //
        // Try to narrow the reference to a Tango::Device object
        //

        dev = Tango::Device::_narrow(obj);
    }
    catch(Tango::DevFailed &)
    {
        TangoSys_OMemStream o;

        o << "The device server " << ds_name << " is not defined in database. Exiting!" << std::ends;
        print_err_message(o.str());
    }
    catch(CORBA::TRANSIENT &)
    {
        TANGO_LOG_DEBUG << "Leaving Util::server_already_running method" << std::endl;
        return;
    }
    catch(CORBA::OBJECT_NOT_EXIST &)
    {
        TANGO_LOG_DEBUG << "Leaving Util::server_already_running method" << std::endl;
        return;
    }
    catch(CORBA::NO_RESPONSE &)
    {
        print_err_message("This server is already running but is blocked!");
    }
    catch(CORBA::COMM_FAILURE &)
    {
        TANGO_LOG_DEBUG << "Leaving Util::server_already_running method" << std::endl;
        return;
    }

    if(CORBA::is_nil(dev))
    {
        TANGO_LOG_DEBUG << "Leaving Util::server_already_running method" << std::endl;
        return;
    }

    //
    // Now, get the device name from the server
    //

    try
    {
        CORBA::String_var n = dev->name();
        unsigned long ctr;
        char *tmp_ptr = n.inout();
        for(ctr = 0; ctr < strlen(tmp_ptr); ctr++)
        {
            tmp_ptr[ctr] = tolower(tmp_ptr[ctr]);
        }
        for(ctr = 0; ctr < dev_name.length(); ctr++)
        {
            dev_name[ctr] = tolower(dev_name[ctr]);
        }
        if(n.in() == dev_name)
        {
            print_err_message("This server is already running, exiting!");
        }
    }
    catch(Tango::DevFailed &)
    {
        //
        // It is necessary to catch this exception because it is thrown by the print_err_message method under windows
        //

        throw;
    }
    catch(CORBA::NO_RESPONSE &)
    {
        try
        {
            print_err_message("This server is already running but is blocked!");
        }
        catch(Tango::DevFailed &)
        {
            throw;
        }
    }
    catch(CORBA::SystemException &)
    {
    }
    catch(CORBA::Exception &)
    {
    }

    TANGO_LOG_DEBUG << "Leaving Util::server_already_running method" << std::endl;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::server_init()
//
// description :
//        To initialise all classes in the device server process
//
//-------------------------------------------------------------------------------------------------------------------

void Util::server_init(TANGO_UNUSED(bool with_window))
{
    is_tango_library_thread = true;

#ifdef _TG_WINDOWS_
    if(Util::_service == true)
    {
        omni_thread::create_dummy();
        _dummy_thread = true;
    }

    omni_thread *th = omni_thread::self();
    if(th == nullptr)
    {
        th = omni_thread::create_dummy();
        _dummy_thread = true;
    }
#else
    omni_thread *th = omni_thread::self();
    if(th == nullptr)
    {
        th = omni_thread::create_dummy();
        _dummy_thread = true;
    }
#endif

#ifdef _TG_WINDOWS_
    if(with_window == true)
    {
        //
        // Create device server windows
        //

        ds_window = new W32Win(this, nCmd);

        //
        // Change TANGO_LOG that it uses the graphical console window
        //
    }
    else
    {
        ds_window = 0;
    }

    if(_win == true)
    {
        go = false;

        loop_th = new ORBWin32Loop(this);
        loop_th->start();
    }
    else
    {
#endif /* WIN 32 */

        //
        // Initialise main class
        //

        DServerClass::init();
        DServer *dserver = get_dserver_device();
        dserver->server_init_hook();

        //
        // Configure polling from the polling properties.
        //

        polling_configure();

        //
        // Delete the db cache if it has been used
        //
        if(db_cache)
        {
            // extract sub device information before deleting cache!
            get_sub_dev_diag().get_sub_devices_from_cache();

            db_cache.reset();
        }

        //
        // In case of process with forwarded attributes with root attribute inside this process as well
        //

        RootAttRegistry &rar = get_root_att_reg();
        if(!rar.empty() && rar.is_root_dev_not_started_err())
        {
            if(EventConsumer::keep_alive_thread != nullptr)
            {
                auto event_consumer = ApiUtil::instance()->get_zmq_event_consumer();
                EventConsumer::keep_alive_thread->fwd_not_conected_event(event_consumer);
            }
        }

#ifdef _TG_WINDOWS_
    }
#endif /* _TG_WINDOWS_ */
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::server_perform_work()
//
// description :
//        To call orb->run() or call orb->perform_work() and user-defined ev_loop_func() in a while loop
//      if user has setup the event loop function
//
//------------------------------------------------------------------------------------------------------------------
void Util::server_perform_work()
{
    if(ev_loop_func != nullptr)
    {
        //
        // If the user has installed its own event management function, call it in a loop
        //

        bool user_shutdown_server;

        while(!shutdown_server)
        {
            if(!is_svr_shutting_down())
            {
                if(orb->work_pending())
                {
                    orb->perform_work();
                }

                user_shutdown_server = (*ev_loop_func)();
                if(user_shutdown_server)
                {
                    shutdown_ds();
                    shutdown_server = true;
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    }
    else
    {
        orb->run();
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::server_run()
//
// description :
//        To start the CORBA event loop
//
//------------------------------------------------------------------------------------------------------------------

void Util::server_run()
{
    //
    // The server is now started
    //

#ifndef _TG_WINDOWS_
    set_svr_starting(false);
#else
    if(_win == false)
    {
        set_svr_starting(false);
    }
#endif

    //
    // For Windows in a non-MSDOS window, start the ORB in its own thread. The main
    // thread is used for windows management.
    //

#ifdef _TG_WINDOWS_
    if(_win == true)
    {
        omni_mutex_lock syc(mon);

        //
        // Start the ORB thread (and loop)
        //

        go = true;
        mon.signal();
    }
    else
    {
        if(_service == true)
        {
            NTService *serv = NTService::instance();
            serv->statusUpdate(SERVICE_RUNNING);
            if(serv->stopped_ == false)
            {
                // JM : 9.8.2005 : destroy() should be called at the exit of run()!
                try
                {
                    server_perform_work();
                    server_cleanup();
                }
                catch(CORBA::Exception &)
                {
                    server_cleanup();
                    throw;
                }
            }
        }
        else
        {
            TANGO_LOG << "Ready to accept request" << std::endl;

            // JM : 9.8.2005 : destroy() should be called at the exit of run()!
            try
            {
                server_perform_work();
                server_cleanup();
            }
            catch(CORBA::Exception &)
            {
                server_cleanup();
                throw;
            }
        }
    }
#else
    TANGO_LOG << "Ready to accept request" << std::endl;

    // JM : 9.8.2005 : destroy() should be called at the exit of run()!
    try
    {
        server_perform_work();
        server_cleanup();
    }
    catch(CORBA::Exception &e)
    {
        server_cleanup();
        throw;
    }
#endif
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::server_cleanup()
//
// description :
//        To relinquish computer resource before process exit
//
//------------------------------------------------------------------------------------------------------------------

void Util::server_cleanup()
{
#ifndef _TG_WINDOWS_

    // uninstall the client call interceptor
    // see section 10.3 of the omniORB documentation - see also cppTango issue #865
    omniCallDescriptor::removeInterceptor(Tango::client_call_interceptor);

    //
    // Destroy the ORB
    //
    if(_constructed)
    {
        orb->destroy();
        // JM : 8.9.2005 : mark as already destroyed
        _constructed = false;
    }
#else
    if(ds_window == nullptr)
    {
        if(_constructed == true)
        {
            orb->destroy();
            // JM : 8.9.2005 : mark as already destroyed
            _constructed = false;
        }
    }
#endif

    if(_dummy_thread)
    {
        omni_thread::release_dummy();
    }

    // We explicitly cleanup the DServerSignal singleton here rather than
    // relying on the static object de-initialiser as there is no guarantee what
    // order static objects get de-initialised between translation units.
    //
    // The DServerSignal::ThSig thread accesses logging static objects and these
    // might not exist when the DServerSignal singleton is being de-initialised.
    DServerSignal::cleanup_singleton();

    // Similarly, if we were shutdown by the kill command, then we need to wait
    // for the kill thread to be finished before we return.
    DServer::wait_for_kill_thread();

    // clean-up the logging system
    Logging::cleanup();
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::get_device_list_by_class()
//
// description :
//        To return a reference to the vector of device for a specific class
//
// arguments :
//         in :
//            - class_name : The class name
//
// returns :
//        Reference to the vector with device pointers
//
//------------------------------------------------------------------------------------------------------------------

std::vector<DeviceImpl *> &Util::get_device_list_by_class(const std::string &class_name)
{
    if(cl_list_ptr == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_DeviceNotFound, "It's too early to call this method. Devices are not created yet!");
    }

    //
    // Retrieve class list. Don't use the get_dserver_device() method followed by
    // the get_class_list(). In case of several classes embedded within
    // the same server and the use of this method in the object creation, it
    // will fail because the end of the dserver object creation is after the
    // end of the last server device creation.
    //

    const std::vector<DeviceClass *> &tmp_cl_list = *cl_list_ptr;

    //
    // Check if the wanted class really exists
    //

    int nb_class = tmp_cl_list.size();
    int i;
    for(i = 0; i < nb_class; i++)
    {
        if(tmp_cl_list[i]->get_name() == class_name)
        {
            break;
        }
    }

    //
    // Also check if it it the DServer class
    //

    if(TG_strcasecmp(class_name.c_str(), "DServer") == 0)
    {
        return DServerClass::instance()->get_device_list();
    }

    //
    // Throw exception if the class is not found
    //

    if(i == nb_class)
    {
        TangoSys_OMemStream o;
        o << "Class " << class_name << " not found" << std::ends;
        TANGO_THROW_EXCEPTION(API_ClassNotFound, o.str());
    }

    return tmp_cl_list[i]->get_device_list();
}

std::vector<DeviceImpl *> &Util::get_device_list_by_class(const char *class_name)
{
    std::string class_str(class_name);

    return get_device_list_by_class(class_str);
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::get_device_by_name()
//
// description :
//        To return a reference to the device object from its name
//
// arguments :
//         in :
//            - dev_name : The device name
//
// returns :
//        Pointer to the device object
//
//-------------------------------------------------------------------------------------------------------------------

DeviceImpl *Util::get_device_by_name(const std::string &dev_name)
{
    std::string dev_name_lower(dev_name);
    std::transform(dev_name_lower.begin(), dev_name_lower.end(), dev_name_lower.begin(), ::tolower);

    DeviceImpl *ret_ptr = find_device_name_core(dev_name_lower);

    //
    // If the device is not found, may be the name we have received is an alias ?
    //

    if(ret_ptr == nullptr)
    {
        std::string d_name;

        if(_UseDb)
        {
            try
            {
                db->get_device_alias(dev_name_lower, d_name);
            }
            catch(Tango::DevFailed &)
            {
            }
        }

        if(d_name.size() != 0)
        {
            std::transform(d_name.begin(), d_name.end(), d_name.begin(), ::tolower);

            ret_ptr = find_device_name_core(d_name);

            //
            // If the name given to this method is a valid alias name, store the alias name in device object for
            // possible future call to this method (save some db calls)
            //

            if(ret_ptr != nullptr)
            {
                ret_ptr->set_alias_name_lower(dev_name_lower);
            }
        }
    }

    //
    // Throw exception if the device is not found
    //

    if(ret_ptr == nullptr)
    {
        TangoSys_OMemStream o;
        o << "Device " << dev_name << " not found" << std::ends;
        TANGO_THROW_EXCEPTION(API_DeviceNotFound, o.str());
    }

    return ret_ptr;
}

DeviceImpl *Util::find_device_name_core(const std::string &dev_name)
{
    //
    // Retrieve class list. Don't use the get_dserver_device() method followed by the get_class_list(). In case of
    // several classes embedded within the same server and the use of this method in the object creation, it will fail
    // because the end of the dserver object creation is after the end of the last server device creation.
    //

    const std::vector<DeviceClass *> &tmp_cl_list = *cl_list_ptr;
    DeviceImpl *ret_ptr = nullptr;

    //
    // Check if the wanted device exists in each class
    //

    int nb_class = tmp_cl_list.size();
    int i, j, nb_dev;
    bool found = false;

    for(i = 0; i < nb_class; i++)
    {
        std::vector<DeviceImpl *> &dev_list = get_device_list_by_class(tmp_cl_list[i]->get_name());
        nb_dev = dev_list.size();

        for(j = 0; j < nb_dev; j++)
        {
            std::string name(dev_list[j]->get_name());
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            if(name == dev_name)
            {
                found = true;
                ret_ptr = dev_list[j];
                break;
            }
            std::string &alias_name = dev_list[j]->get_alias_name_lower();
            if(alias_name.size() != 0)
            {
                if(alias_name == dev_name)
                {
                    found = true;
                    ret_ptr = dev_list[j];
                    break;
                }
            }
        }
        if(found)
        {
            break;
        }
    }

    //
    // Check also the dserver device
    //

    if(!found && dev_name.find("dserver/") == 0)
    {
        DServerClass *ds_class = DServerClass::instance();
        std::vector<DeviceImpl *> &devlist = ds_class->get_device_list();
        std::string name(devlist[0]->get_name());
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if(name == dev_name)
        {
            ret_ptr = devlist[0];
            j--;
        }
    }

    //
    // Return to caller. The returned value is nullptr if the device is not found
    //

    return ret_ptr;
}

DeviceImpl *Util::get_device_by_name(const char *dev_name)
{
    std::string name_str(dev_name);

    return get_device_by_name(name_str);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::get_dserver_device()
//
// description :
//        To return a pointer to the dserver device automatically attached to each device server process
//
//-------------------------------------------------------------------------------------------------------------------

DServer *Util::get_dserver_device()
{
    return (DServer *) ((DServerClass::instance()->get_device_list())[0]);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::get_device_list
//
// description :
//        Helper method to get device list from a wild card. If no device is found, does not throw exception, just
//        return an empty vector
//
// arguments :
//         in :
//            - pattern: The wildcard (e.g. "*", "/tango/tangotest/*", ...)
//
// returns :
//        The list of devices which name matches the wildcard
//
//-------------------------------------------------------------------------------------------------------------------

std::vector<DeviceImpl *> Util::get_device_list(const std::string &pattern)
{
    TANGO_LOG_DEBUG << "In Util::get_device_list" << std::endl;

    // the returned list
    std::vector<DeviceImpl *> dl(0);

    //
    // ------------------------------------------------------------------
    // CASE I: pattern does not contain any '*' char - it's a device name
    //

    if(pattern.find('*') == std::string::npos)
    {
        DeviceImpl *dev = nullptr;
        try
        {
            dev = get_device_by_name(pattern);
        }
        catch(Tango::DevFailed &)
        {
        }

        //
        // add dev to the list
        //

        if(dev != nullptr)
        {
            dl.push_back(dev);
        }

        return dl;
    }

    //
    // for the two remaining cases, we need the list of all DeviceClasses.
    //

    const std::vector<DeviceClass *> dcl(*(get_class_list()));

    // a vector to store a given class' devices
    std::vector<DeviceImpl *> temp_dl;

    //
    // ------------------------------------------------------------------
    // CASE II: pattern == "*" - return a list containing all devices
    //

    if(pattern == "*")
    {
        for(unsigned int i = 0; i < dcl.size(); i++)
        {
            temp_dl = dcl[i]->get_device_list();
            dl.insert(dl.end(), temp_dl.begin(), temp_dl.end());
        }
        return dl;
    }

    //
    // ------------------------------------------------------------------
    // CASE III: pattern contains at least one '*' char
    //

    std::string::size_type pos;
    std::string::size_type last_pos = 0;
    std::string token;
    std::vector<std::string> tokens(0);

    //
    // build the token list
    //

    bool done = false;
    do
    {
        pos = pattern.find('*', last_pos);
        if(pos != 0)
        {
            if(pos == std::string::npos)
            {
                if(last_pos >= pattern.size())
                {
                    break;
                }
                pos = pattern.size();
                done = true;
            }
            token.assign(pattern.begin() + last_pos, pattern.begin() + pos);
            TANGO_LOG_DEBUG << "Found pattern " << token << std::endl;
            tokens.push_back(token);
        }
        last_pos = pos + 1;
    } while(!done);
    // look for token(s) in device names
    unsigned int i, j, k;
    std::string dev_name;
    // for each DeviceClass...
    for(i = 0; i < dcl.size(); i++)
    {
        // ...get device list
        temp_dl = dcl[i]->get_device_list();
        // for each device in in list...
        for(j = 0; j < temp_dl.size(); j++)
        {
            // get device name
            dev_name = temp_dl[j]->get_name();
            // make sure each char is lower case
            std::transform(dev_name.begin(), dev_name.end(), dev_name.begin(), ::tolower);
            // then look for token(s) in device name
            // to be added to the list, device_name must contains
            // every token in the right order.
            for(k = 0, pos = 0; k < tokens.size(); k++)
            {
                pos = dev_name.find(tokens[k], pos);
                if(pos == std::string::npos)
                {
                    break;
                }
            }
            // if dev_name matches the pattern, add the device to the list
            if(k == tokens.size())
            {
                TANGO_LOG_DEBUG << "Device " << temp_dl[j]->get_name() << " match pattern" << std::endl;
                dl.push_back(temp_dl[j]);
            }
        }
    }

    TANGO_LOG_DEBUG << "Returning a device list containing " << dl.size() << " items" << std::endl;
    return dl;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::unregister_server()
//
// description :
//        Unregister the server from the database
//
//-------------------------------------------------------------------------------------------------------------------

void Util::unregister_server()
{
    TANGO_LOG_DEBUG << "Entering Util::unregister_server method" << std::endl;

    //
    // Mark all the devices belonging to this server as unexported
    //

    if((_UseDb) && (!_FileDb))
    {
        try
        {
            db->unexport_server(ds_name);
        }
        catch(Tango::DevFailed &e)
        {
            Except::print_exception(e);
            throw;
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            throw;
        }
    }
    TANGO_LOG_DEBUG << "Leaving Util::unregister_server method" << std::endl;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::print_err_message()
//
// description :
//        Print error message in the classical console or with a message box For Linux OS, this method exits.
//        If it is called under Windows in a graphical environment, it throws exception
//
// argument :
//         in :
//            - err_mess : The error message
//            - type : The error type
//
//------------------------------------------------------------------------------------------------------------------

void Util::print_err_message(const char *err_mess, TANGO_UNUSED(Tango::MessBoxType type))
{
#ifdef _TG_WINDOWS_
    if(_win == true)
    {
        switch(type)
        {
        case Tango::STOP:
            MessageBox((HWND) nullptr, err_mess, MessBoxTitle, MB_ICONSTOP);
            break;

        case Tango::INFO:
            MessageBox((HWND) nullptr, err_mess, MessBoxTitle, MB_ICONINFORMATION);
            break;
        }
        TANGO_THROW_EXCEPTION(API_StartupSequence, "Error in device server startup sequence");
    }
    else
    {
        std::cerr << err_mess << std::endl;
        exit(-1);
    }
#else
    std::cerr << err_mess << std::endl;
    _exit(-1);
#endif
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::get_tango_lib_vers()
//
// description :
//        Return a number set to the Tango release number coded with 3 digits (550, 551,552,600)
//
//------------------------------------------------------------------------------------------------------------------

long Util::get_tango_lib_release()
{
    return _convert_tango_lib_release();
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::clean_dyn_attr_prop()
//
// description :
//        Clean in database the dynamic attribute property(ies)
//
//-------------------------------------------------------------------------------------------------------------------

void Util::clean_dyn_attr_prop()
{
    if(_UseDb)
    {
        DbData send_data;

        for(unsigned long loop = 0; loop < all_dyn_attr.size(); loop++)
        {
            DbDatum db_dat(all_dyn_attr[loop]);
            send_data.push_back(db_dat);
        }

        db->delete_all_device_attribute_property(dyn_att_dev_name, send_data);
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::delete_restarting_device()
//
// description :
//        Delete a device from the vector of restarting device
//
// arguments:
//        in :
//            - d_name : - The device name
//
//-------------------------------------------------------------------------------------------------------------------

void Util::delete_restarting_device(const std::string &d_name)
{
    std::vector<std::string>::iterator pos;
    pos = remove(restarting_devices.begin(), restarting_devices.end(), d_name);
    restarting_devices.erase(pos, restarting_devices.end());
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::get_cmd_line_name_list()
//
// description :
//        Get command line device name list for a specified class
//
// arguments:
//        in :
//            - cl_name : The class name
//        out :
//            - name_list : Vector where device name must be stored
//
//-------------------------------------------------------------------------------------------------------------------

void Util::get_cmd_line_name_list(const std::string &cl_name, std::vector<std::string> &name_list)
{
    std::string local_cl_name(cl_name);
    std::transform(local_cl_name.begin(), local_cl_name.end(), local_cl_name.begin(), ::tolower);

    auto pos = cmd_line_name_list.find(local_cl_name);
    if(pos != cmd_line_name_list.end())
    {
        name_list.insert(name_list.end(), pos->second.begin(), pos->second.end());
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::validate_cmd_line_classes()
//
// description :
//        Validate the class name used in DS command line argument
//
//-------------------------------------------------------------------------------------------------------------------

void Util::validate_cmd_line_classes()
{
    std::map<std::string, std::vector<std::string>>::iterator pos;

    for(pos = cmd_line_name_list.begin(); pos != cmd_line_name_list.end(); ++pos)
    {
        if(pos->first == NoClass)
        {
            continue;
        }

        bool found = false;
        for(size_t loop = 0; loop < cl_list.size(); loop++)
        {
            std::string cl_name = cl_list[loop]->get_name();
            std::transform(cl_name.begin(), cl_name.end(), cl_name.begin(), ::tolower);

            if(cl_name == pos->first)
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            std::stringstream ss;
            ss << "Class name " << pos->first
               << " used on command line device declaration but this class is not embedded in DS process";
            TANGO_THROW_EXCEPTION(API_WrongCmdLineArgs, ss.str());
        }
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::tango_host_from_fqan()
//
// description :
//        Utility method to get tango host from a fully qualified Tango attribute name
//        Note that it works also if the fqan is only a fully qualified Tango device name (fqdn)
//
// arguments:
//        in :
//            - fqan : The fully qualified Tango attribute name
//        out :
//            - tg_host : The tango host (host:port)
//
//-------------------------------------------------------------------------------------------------------------------

void Util::tango_host_from_fqan(const std::string &fqan, std::string &tg_host)
{
    std::string lower_fqan(fqan);
    std::transform(lower_fqan.begin(), lower_fqan.end(), lower_fqan.begin(), ::tolower);

    std::string start = lower_fqan.substr(0, 5);
    if(start != "tango")
    {
        std::stringstream ss;
        ss << "The provided fqan (" << fqan << ") is not a valid Tango attribute name" << std::endl;
        TANGO_THROW_EXCEPTION(API_InvalidArgs, ss.str());
    }

    std::string::size_type pos = lower_fqan.find('/', 8);
    tg_host = fqan.substr(8, pos - 8);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::tango_host_from_fqan()
//
// description :
//        Utility method to get tango host from a fully qualified Tango attribute name
//        Note that it works also if the fqan is only a fully qualified Tango device name (fqdn)
//
// arguments:
//        in :
//            - fqan : The fully qualified Tango attribute name
//        out :
//            - host : The Tango host (only the host name part)
//            - port : The Tango db server port
//
//-------------------------------------------------------------------------------------------------------------------

void Util::tango_host_from_fqan(const std::string &fqan, std::string &host, int &port)
{
    std::string tmp_tg_host;
    tango_host_from_fqan(fqan, tmp_tg_host);

    std::string::size_type pos = tmp_tg_host.find(':');
    host = tmp_tg_host.substr(0, pos);

    std::string tmp_port = tmp_tg_host.substr(pos + 1);
    std::stringstream ss;
    ss << tmp_port;
    ss >> port;
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::check_end_point_specified()
//
// description :
//      Check if the user specified a endPoint
//          - on the command line
//          - using one env. variable
//          - in  the omniORB config file (/etc/omniORB.cfg)
//      If true, extract the IP address from the end point and store it for future use in the ZMQ publisher(s)
//
//-------------------------------------------------------------------------------------------------------------------

void Util::check_end_point_specified(int argc, char *argv[])
{
    {
        const std::optional<std::string> endpoint = get_omniorb_variable(argc, argv, "endPoint");
        if(endpoint.has_value())
        {
            set_endpoint_specified(true);
            set_specified_ip(*endpoint);
        }
    }

    {
        const std::optional<std::string> endpoint_publish = get_omniorb_variable(argc, argv, "endPointPublish");
        if(endpoint_publish.has_value())
        {
            set_endpoint_publish_specified(true);
            set_endpoint_publish(*endpoint_publish);
        }
    }
}

#ifdef _TG_WINDOWS_
//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::build_argc_argv()
//
// description :
//        Build argc, argv UNIX like parameters from the Windows command line
//
//-------------------------------------------------------------------------------------------------------------------

void Util::build_argc_argv()
{
    //
    // Get command line
    //

    char *cmd_line = GetCommandLine();

    int cnt = 0;
    char *tmp;

    //
    // First, count how many args we have. If the user type two spaces between args, we will have too many pointers
    // allocates but it is not a problem
    //

    int cmd_line_size = strlen(cmd_line);
    for(int i = 0; i < cmd_line_size; i++)
    {
        if(cmd_line[i] == ' ')
        {
            cnt++;
        }
    }

    //
    // Allocate memory for argv
    //

    argv = new char *[cnt + 1];

    //
    // If only one args, no parsing is necessary
    //

    if(cnt == 0)
    {
        argv[0] = new char[cmd_line_size + 1];
        strcpy(argv[0], cmd_line);
        argc = 1;
    }
    else
    {
        //
        // Get program name
        //

        tmp = strtok(cmd_line, " ");
        argv[0] = new char[strlen(tmp) + 1];
        strcpy(argv[0], tmp);

        //
        // Get remaining args
        //

        int i = 1;
        while((tmp = strtok(nullptr, " ")) != nullptr)
        {
            argv[i] = new char[strlen(tmp) + 1];
            strcpy(argv[i], tmp);
            i++;
        }
        argc = i;
    }
}

HWND Util::get_console_window()
{
    return ds_window->get_output_buffer()->get_debug_window();
}

HWND Util::get_ds_main_window()
{
    return ds_window->get_win();
}

CoutBuf *Util::get_debug_object()
{
    return ds_window ? ds_window->get_output_buffer() : 0;
}

BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch(fdwCtrlType)
    {
        // Ignore logoff event!
    case CTRL_LOGOFF_EVENT:
        return TRUE;

        // Pass all other signals to the next signal handler
    default:
        return FALSE;
    }
}

void Util::install_cons_handler()
{
    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE))
    {
        print_err_message("WARNING: Can't install the console handler");
    }
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Util::ORBWin32Loop::run()
//
// description :
//        Start the ORB loop. This method is in a inner class because it is started using the a separate thread.
//        One thread is for the Windows event loop and the second thread is for the ORB loop.
//
//-------------------------------------------------------------------------------------------------------------------

void *Util::ORBWin32Loop::run_undetached(void *ptr)
{
    is_tango_library_thread = true;

    //
    // Create the DServer object
    //

    try
    {
        DServerClass::init();
    }
    catch(std::bad_alloc)
    {
        MessageBox((HWND) nullptr, "Memory error", "Device creation failed", MB_ICONSTOP);
        ::exit(-1);
    }
    catch(Tango::DevFailed &e)
    {
        std::string str(e.errors[0].desc.in());
        str = str + '\n';
        str = str + e.errors[0].origin.in();
        MessageBox((HWND) nullptr, str.c_str(), "Device creation failed", MB_ICONSTOP);
        ::exit(-1);
    }
    catch(CORBA::Exception &)
    {
        MessageBox((HWND) nullptr, "CORBA exception", "Device creation failed", MB_ICONSTOP);
        ::exit(-1);
    }

    //
    // Configure polling from polling properties
    //

    util->polling_configure();

    //
    // Delete DB cache (if there is one)
    //

    if(util->db_cache)
    {
        // extract sub device information before deleting cache!
        util->get_sub_dev_diag().get_sub_devices_from_cache();

        util->db_cache.reset();
    }

    util->set_svr_starting(false);

    //
    // Start the ORB
    //

    wait_for_go();

    util->get_orb()->run();

    return nullptr;
}

void Util::ORBWin32Loop::wait_for_go()
{
    omni_mutex_lock sync(util->mon);

    while(util->go == false)
    {
        util->mon.wait();
    }
}
#endif /* _TG_WINDOWS_ */

int TangoMonitor::wait(long nb_millis)
{
    unsigned long s, n;

    unsigned long nb_sec, nb_nanos;
    nb_sec = nb_millis / 1000;
    nb_nanos = (nb_millis - (nb_sec * 1000)) * 1000000;

    omni_thread::get_time(&s, &n, nb_sec, nb_nanos);
    return cond.timedwait(s, n);
}

void clear_att_dim(Tango::AttributeValue_3 &att_val)
{
    att_val.r_dim.dim_x = 0;
    att_val.r_dim.dim_y = 0;
    att_val.w_dim.dim_x = 0;
    att_val.w_dim.dim_y = 0;
}

void clear_att_dim(Tango::AttributeValue_4 &att_val)
{
    att_val.r_dim.dim_x = 0;
    att_val.r_dim.dim_y = 0;
    att_val.w_dim.dim_x = 0;
    att_val.w_dim.dim_y = 0;

    att_val.data_format = Tango::FMT_UNKNOWN;
}

void clear_att_dim(Tango::AttributeValue_5 &att_val)
{
    att_val.r_dim.dim_x = 0;
    att_val.r_dim.dim_y = 0;
    att_val.w_dim.dim_x = 0;
    att_val.w_dim.dim_y = 0;

    att_val.data_format = Tango::FMT_UNKNOWN;
    att_val.data_type = 0;
}

//
// A small function to convert Tango lib release string to a number
//
// It is defined as a function to be used within the Utils and ApiUtil
// classes (both client and server part)
//

long _convert_tango_lib_release()
{
    long ret;

    ret = (TANGO_VERSION_MAJOR * 100) + (TANGO_VERSION_MINOR * 10) + TANGO_VERSION_PATCH;

    return ret;
}

std::ostream &operator<<(std::ostream &o_str, const CmdArgType &type)
{
    o_str << data_type_to_string(type);
    return o_str;
}

std::ostream &operator<<(std::ostream &o_str, const AttrDataFormat &format)
{
    switch(format)
    {
    case Tango::FMT_UNKNOWN:
        o_str << "Unknown";
        break;

    case Tango::SCALAR:
        o_str << "Scalar";
        break;

    case Tango::SPECTRUM:
        o_str << "Spectrum";
        break;

    case Tango::IMAGE:
        o_str << "Image";
        break;

    default:
        o_str << "Undefined Format " << static_cast<std::underlying_type_t<AttrDataFormat>>(format);
        break;
    }
    return o_str;
}

std::ostream &operator<<(std::ostream &o_str, const AttrWriteType &writable)
{
    switch(writable)
    {
    case Tango::WRITE:
        o_str << "Write";
        break;

    case Tango::READ:
        o_str << "Read";
        break;

    case Tango::READ_WRITE:
        o_str << "Read/Write";
        break;

    case Tango::READ_WITH_WRITE:
        o_str << "Read with write";
        break;

    default:
        o_str << "Undefined WriteType " << static_cast<std::underlying_type_t<AttrWriteType>>(writable);
        break;
    }
    return o_str;
}

std::ostream &operator<<(std::ostream &o_str, const PipeWriteType &writable)
{
    switch(writable)
    {
    case Tango::PIPE_READ:
        o_str << "Read";
        break;

    case Tango::PIPE_READ_WRITE:
        o_str << "Read/Write";
        break;

    default:
        o_str << "Undefined PipeWriteType " << static_cast<std::underlying_type_t<PipeWriteType>>(writable);
        break;
    }
    return o_str;
}

std::ostream &operator<<(std::ostream &o_str, const DispLevel &level)
{
    switch(level)
    {
    case DL_UNKNOWN:
        o_str << "Unknown";
        break;

    case OPERATOR:
        o_str << "Operator";
        break;

    case EXPERT:
        o_str << "Expert";
        break;

    default:
        o_str << "Undefined Display Level " << static_cast<std::underlying_type_t<DispLevel>>(level);
        break;
    }
    return o_str;
}

std::ostream &operator<<(std::ostream &o_str, const FwdAttError &fae)
{
    switch(fae)
    {
    case FWD_WRONG_ATTR:
        o_str << "Attribute not found in root device";
        break;

    case FWD_CONF_LOOP:
        o_str << "Loop found in root attributes configuration";
        break;

    case FWD_WRONG_DEV:
        o_str << "Wrong root device";
        break;

    case FWD_MISSING_ROOT:
        o_str << "Missing or wrong root attribute definition";
        break;

    case FWD_ROOT_DEV_LOCAL_DEV:
        o_str << "Root device is local device";
        break;

    case FWD_WRONG_SYNTAX:
        o_str << "Wrong syntax in root attribute definition";
        break;

    case FWD_ROOT_DEV_NOT_STARTED:
        o_str << "Root device not started yet. Polling (if any) for this attribute not available";
        break;

    case FWD_TOO_OLD_LOCAL_DEVICE:
        o_str << "Local device too old (< IDL 5)";
        break;

    case FWD_TOO_OLD_ROOT_DEVICE:
        o_str << "Root device too old (< IDL 5)";
        break;

    case FWD_DOUBLE_USED:
    {
        o_str << "Root attribute already used in this device server process for attribute ";
        break;
    }

    default:
        o_str << "Undefined ForwardAttributeError" << static_cast<std::underlying_type_t<FwdAttError>>(fae);
        break;
    }
    return o_str;
}

} // namespace Tango
