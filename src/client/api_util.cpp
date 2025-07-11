//+==================================================================================================================
//
// C++ source code file for TANGO api class ApiUtil
//
// programmer     - Emmanuel Taurel (taurel@esrf.fr)
//
// Copyright (C) :      2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Lesser Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
// original     - May 2002
//
//
//
//+==================================================================================================================

#include <tango/client/eventconsumer.h>
#include <tango/client/Database.h>
#include <tango/client/DeviceAttribute.h>
#include <thread>
#include <iomanip>

#ifndef _TG_WINDOWS_
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <net/if.h>
  #include <sys/ioctl.h>
  #include <netdb.h>
  #include <csignal>
  #include <ifaddrs.h>
  #include <netinet/in.h> // FreeBSD
#else
  #include <ws2tcpip.h>
  #include <process.h>
#endif

#include <tango/common/pointer_with_lock.h>

namespace
{

void killproc()
{
    ::exit(-1);
}

void _t_handler(TANGO_UNUSED(int signum))
{
    std::thread t(killproc);
    t.detach();
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

//-----------------------------------------------------------------------------------------------------------------
//
// method :
//        attr_to_device_base()
//
// description :
//
//
//-------------------------------------------------------------------------------------------------------------------

template <typename T>
inline void attr_to_device_base(const T *attr_value, Tango::DeviceAttribute *dev_attr)
{
    CORBA::Long *tmp_lo;
    CORBA::Short *tmp_sh;
    CORBA::Double *tmp_db;
    char **tmp_str;
    CORBA::Float *tmp_fl;
    CORBA::Boolean *tmp_boo;
    CORBA::UShort *tmp_ush;
    CORBA::Octet *tmp_uch;
    CORBA::LongLong *tmp_lolo;
    CORBA::ULong *tmp_ulo;
    CORBA::ULongLong *tmp_ulolo;
    Tango::DevState *tmp_state;
    Tango::DevEncoded *tmp_enc;

    CORBA::ULong max, len;

    dev_attr->name = attr_value->name;
    dev_attr->quality = attr_value->quality;
    dev_attr->data_format = attr_value->data_format;
    dev_attr->time = attr_value->time;
    dev_attr->dim_x = attr_value->r_dim.dim_x;
    dev_attr->dim_y = attr_value->r_dim.dim_y;
    dev_attr->set_w_dim_x(attr_value->w_dim.dim_x);
    dev_attr->set_w_dim_y(attr_value->w_dim.dim_y);
    dev_attr->err_list = new Tango::DevErrorList(attr_value->err_list);

    if(dev_attr->quality != Tango::ATTR_INVALID)
    {
        switch(attr_value->value._d())
        {
        case Tango::ATT_BOOL:
        {
            const Tango::DevVarBooleanArray &tmp_seq = attr_value->value.bool_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_boo = (const_cast<Tango::DevVarBooleanArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->BooleanSeq = new Tango::DevVarBooleanArray(max, len, tmp_boo, true);
            }
            else
            {
                tmp_boo = const_cast<CORBA::Boolean *>(tmp_seq.get_buffer());
                dev_attr->BooleanSeq = new Tango::DevVarBooleanArray(max, len, tmp_boo, false);
            }
            dev_attr->data_type = Tango::DEV_BOOLEAN;
        }
        break;

        case Tango::ATT_SHORT:
        {
            const Tango::DevVarShortArray &tmp_seq = attr_value->value.short_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_sh = (const_cast<Tango::DevVarShortArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->ShortSeq = new Tango::DevVarShortArray(max, len, tmp_sh, true);
            }
            else
            {
                tmp_sh = const_cast<CORBA::Short *>(tmp_seq.get_buffer());
                dev_attr->ShortSeq = new Tango::DevVarShortArray(max, len, tmp_sh, false);
            }
            dev_attr->data_type = Tango::DEV_SHORT;
        }
        break;

        case Tango::ATT_LONG:
        {
            const Tango::DevVarLongArray &tmp_seq = attr_value->value.long_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_lo = (const_cast<Tango::DevVarLongArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->LongSeq = new Tango::DevVarLongArray(max, len, tmp_lo, true);
            }
            else
            {
                tmp_lo = const_cast<CORBA::Long *>(tmp_seq.get_buffer());
                dev_attr->LongSeq = new Tango::DevVarLongArray(max, len, tmp_lo, false);
            }
            dev_attr->data_type = Tango::DEV_LONG;
        }
        break;

        case Tango::ATT_LONG64:
        {
            const Tango::DevVarLong64Array &tmp_seq = attr_value->value.long64_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_lolo = (const_cast<Tango::DevVarLong64Array &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->Long64Seq = new Tango::DevVarLong64Array(max, len, tmp_lolo, true);
            }
            else
            {
                tmp_lolo = const_cast<CORBA::LongLong *>(tmp_seq.get_buffer());
                dev_attr->Long64Seq = new Tango::DevVarLong64Array(max, len, tmp_lolo, false);
            }
            dev_attr->data_type = Tango::DEV_LONG64;
        }
        break;

        case Tango::ATT_FLOAT:
        {
            const Tango::DevVarFloatArray &tmp_seq = attr_value->value.float_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_fl = (const_cast<Tango::DevVarFloatArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->FloatSeq = new Tango::DevVarFloatArray(max, len, tmp_fl, true);
            }
            else
            {
                tmp_fl = const_cast<CORBA::Float *>(tmp_seq.get_buffer());
                dev_attr->FloatSeq = new Tango::DevVarFloatArray(max, len, tmp_fl, false);
            }
            dev_attr->data_type = Tango::DEV_FLOAT;
        }
        break;

        case Tango::ATT_DOUBLE:
        {
            const Tango::DevVarDoubleArray &tmp_seq = attr_value->value.double_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_db = (const_cast<Tango::DevVarDoubleArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->DoubleSeq = new Tango::DevVarDoubleArray(max, len, tmp_db, true);
            }
            else
            {
                tmp_db = const_cast<CORBA::Double *>(tmp_seq.get_buffer());
                dev_attr->DoubleSeq = new Tango::DevVarDoubleArray(max, len, tmp_db, false);
            }
            dev_attr->data_type = Tango::DEV_DOUBLE;
        }
        break;

        case Tango::ATT_UCHAR:
        {
            const Tango::DevVarCharArray &tmp_seq = attr_value->value.uchar_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_uch = (const_cast<Tango::DevVarCharArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->UCharSeq = new Tango::DevVarCharArray(max, len, tmp_uch, true);
            }
            else
            {
                tmp_uch = const_cast<CORBA::Octet *>(tmp_seq.get_buffer());
                dev_attr->UCharSeq = new Tango::DevVarCharArray(max, len, tmp_uch, false);
            }
            dev_attr->data_type = Tango::DEV_UCHAR;
        }
        break;

        case Tango::ATT_USHORT:
        {
            const Tango::DevVarUShortArray &tmp_seq = attr_value->value.ushort_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_ush = (const_cast<Tango::DevVarUShortArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->UShortSeq = new Tango::DevVarUShortArray(max, len, tmp_ush, true);
            }
            else
            {
                tmp_ush = const_cast<CORBA::UShort *>(tmp_seq.get_buffer());
                dev_attr->UShortSeq = new Tango::DevVarUShortArray(max, len, tmp_ush, false);
            }
            dev_attr->data_type = Tango::DEV_USHORT;
        }
        break;

        case Tango::ATT_ULONG:
        {
            const Tango::DevVarULongArray &tmp_seq = attr_value->value.ulong_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_ulo = (const_cast<Tango::DevVarULongArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->ULongSeq = new Tango::DevVarULongArray(max, len, tmp_ulo, true);
            }
            else
            {
                tmp_ulo = const_cast<CORBA::ULong *>(tmp_seq.get_buffer());
                dev_attr->ULongSeq = new Tango::DevVarULongArray(max, len, tmp_ulo, false);
            }
            dev_attr->data_type = Tango::DEV_ULONG;
        }
        break;

        case Tango::ATT_ULONG64:
        {
            const Tango::DevVarULong64Array &tmp_seq = attr_value->value.ulong64_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_ulolo = (const_cast<Tango::DevVarULong64Array &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->ULong64Seq = new Tango::DevVarULong64Array(max, len, tmp_ulolo, true);
            }
            else
            {
                tmp_ulolo = const_cast<CORBA::ULongLong *>(tmp_seq.get_buffer());
                dev_attr->ULong64Seq = new Tango::DevVarULong64Array(max, len, tmp_ulolo, false);
            }
            dev_attr->data_type = Tango::DEV_ULONG64;
        }
        break;

        case Tango::ATT_STRING:
        {
            const Tango::DevVarStringArray &tmp_seq = attr_value->value.string_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_str = (const_cast<Tango::DevVarStringArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->StringSeq = new Tango::DevVarStringArray(max, len, tmp_str, true);
            }
            else
            {
                tmp_str = const_cast<char **>(tmp_seq.get_buffer());
                dev_attr->StringSeq = new Tango::DevVarStringArray(max, len, tmp_str, false);
            }
            dev_attr->data_type = Tango::DEV_STRING;
        }
        break;

        case Tango::ATT_STATE:
        {
            const Tango::DevVarStateArray &tmp_seq = attr_value->value.state_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_state = (const_cast<Tango::DevVarStateArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->StateSeq = new Tango::DevVarStateArray(max, len, tmp_state, true);
            }
            else
            {
                tmp_state = const_cast<Tango::DevState *>(tmp_seq.get_buffer());
                dev_attr->StateSeq = new Tango::DevVarStateArray(max, len, tmp_state, false);
            }
            dev_attr->data_type = Tango::DEV_STATE;
        }
        break;

        case Tango::DEVICE_STATE:
        {
            dev_attr->d_state = attr_value->value.dev_state_att();
            dev_attr->d_state_filled = true;
            dev_attr->data_type = Tango::DEV_STATE;
        }
        break;

        case Tango::ATT_ENCODED:
        {
            const Tango::DevVarEncodedArray &tmp_seq = attr_value->value.encoded_att_value();
            max = tmp_seq.maximum();
            len = tmp_seq.length();
            if(tmp_seq.release())
            {
                tmp_enc = (const_cast<Tango::DevVarEncodedArray &>(tmp_seq)).get_buffer((CORBA::Boolean) true);
                dev_attr->EncodedSeq = new Tango::DevVarEncodedArray(max, len, tmp_enc, true);
            }
            else
            {
                tmp_enc = const_cast<Tango::DevEncoded *>(tmp_seq.get_buffer());
                dev_attr->EncodedSeq = new Tango::DevVarEncodedArray(max, len, tmp_enc, false);
            }
            dev_attr->data_type = Tango::DEV_ENCODED;
        }
        break;

        case Tango::ATT_NO_DATA:
        {
            dev_attr->data_type = Tango::DATA_TYPE_UNKNOWN;
        }
        break;

        default:
            dev_attr->data_type = Tango::DATA_TYPE_UNKNOWN;
        }
    }
}
} // anonymous namespace

namespace Tango
{

ApiUtil *ApiUtil::_instance = nullptr;

omni_mutex ApiUtil::inst_mutex;

ApiUtil *ApiUtil::instance()
{
    omni_mutex_lock lo(inst_mutex);

    if(_instance == nullptr)
    {
        _instance = new ApiUtil();
    }
    return _instance;
}

void ApiUtil::cleanup()
{
    if(_instance != nullptr)
    {
        delete _instance;
        _instance = nullptr;
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::ApiUtil()
//
// description :
//        Constructor of the ApiUtil class.
//
//------------------------------------------------------------------------------------------------------------------

ApiUtil::ApiUtil() :

    ext(new ApiUtilExt),

    cl_pid(0)

{
    _orb = CORBA::ORB::_nil();

    //
    // Check if it is created from a device server
    //

    in_serv = Util::_constructed;

    //
    // Create the Asynchronous polling request Id generator
    //

    UniqIdent *ui = new UniqIdent();

    //
    // Create the table used to store asynchronous polling requests
    //

    asyn_p_table = new AsynReq(ui);

    //
    // Set the asynchronous callback mode to "ON_CALL"
    //

    auto_cb = PULL_CALLBACK;
    cb_thread_ptr = nullptr;

    //
    // Get the process PID
    //

#ifdef _TG_WINDOWS_
    cl_pid = _getpid();
#else
    cl_pid = getpid();
#endif

    //
    // Check if the user has defined his own connection timeout
    //

    std::string var;
    if(get_env_var("TANGOconnectTimeout", var) == 0)
    {
        int user_to = -1;
        std::istringstream iss(var);
        iss >> user_to;
        if(iss)
        {
            user_connect_timeout = user_to;
        }
    }

    //
    // Check if the user has defined his own subscriber hwm (fpr zmq event tuning)
    //

    var.clear();
    if(get_env_var("TANGO_EVENT_BUFFER_HWM", var) == 0)
    {
        int sub_hwm = -1;
        std::istringstream iss(var);
        iss >> sub_hwm;
        if(iss)
        {
            user_sub_hwm = sub_hwm;
        }
    }
}

//+----------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::~ApiUtil()
//
// description :
//        Destructor of the ApiUtil class.
//
//------------------------------------------------------------------------------------------------------------------

ApiUtil::~ApiUtil()
{
    //
    // Release Asyn stuff
    //

    delete asyn_p_table;

    if(cb_thread_ptr != nullptr)
    {
        cb_thread_cmd.stop_thread();
        cb_thread_ptr->join(nullptr);
    }

    //
    // Kill any remaining locking threads
    //

    clean_locking_threads();

    //
    // Release event stuff (in case it is necessary)
    //

    bool event_was_used = false;

    if(ext != nullptr)
    {
        if((get_notifd_event_consumer() != nullptr) || (get_zmq_event_consumer() != nullptr))
        {
            event_was_used = true;
            leavefunc();
        }
    }

    //
    // Delete the database object
    //

    for(unsigned int i = 0; i < db_vect.size(); i++)
    {
        delete db_vect[i];
    }

    //
    // Properly shutdown the ORB
    //

    if((!in_serv) && (!CORBA::is_nil(_orb)))
    {
        if(!event_was_used)
        {
            try
            {
                _orb->destroy();
            }
            catch(...)
            {
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::set_sig_handler()
//
// description :
//        Install a signal handler for SIGINT and SIGTERM but only if nothing is already installed
//
//-------------------------------------------------------------------------------------------------------------------

void ApiUtil::set_sig_handler()
{
#ifndef _TG_WINDOWS_
    if(!in_serv)
    {
        struct sigaction sa, old_action;

        sa.sa_handler = _t_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if(sigaction(SIGTERM, nullptr, &old_action) != -1)
        {
            if(old_action.sa_handler == nullptr)
            {
                sigaction(SIGTERM, &sa, nullptr);
            }
        }

        if(sigaction(SIGINT, nullptr, &old_action) != -1)
        {
            if(old_action.sa_handler == nullptr)
            {
                sigaction(SIGINT, &sa, nullptr);
            }
        }
    }
#endif
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::create_orb()
//
// description :
//        Create the CORBA orb object
//
//-------------------------------------------------------------------------------------------------------------------

void ApiUtil::create_orb()
{
    int _argc;
    char **_argv;

    //
    // pass dummy arguments to init() because we don't have access to argc and argv
    //

    _argc = 1;
    _argv = (char **) malloc(sizeof(char *));
    _argv[0] = (char *) "dummy";

    //
    // Get user signal handler for SIGPIPE (ORB_init call install a SIG_IGN for SIGPIPE. This could be annoying in case
    // the user uses SIGPIPE)
    //

#ifndef _TG_WINDOWS_
    struct sigaction sa;
    sa.sa_handler = nullptr;

    if(sigaction(SIGPIPE, nullptr, &sa) == -1)
    {
        sa.sa_handler = nullptr;
    }
#endif

    // Init the ORB

    const char *options[][2] = {{"clientCallTimeOutPeriod", CLNT_TIMEOUT_STR},
                                {"verifyObjectExistsAndType", "0"},
                                {"maxGIOPConnectionPerServer", MAX_GIOP_PER_SERVER},
                                {"giopMaxMsgSize", MAX_TRANSFER_SIZE},
                                {"throwTransientOnTimeOut", "1"},
                                {"exceptionIdInAny", "0"},
                                {nullptr, nullptr}};

    _orb = CORBA::ORB_init(_argc, _argv, "omniORB4", options);

    free(_argv);

    //
    // Restore SIGPIPE handler
    //

#ifndef _TG_WINDOWS_
    if(sa.sa_handler != nullptr)
    {
        struct sigaction sb;

        sb = sa;

        if(sigaction(SIGPIPE, &sb, nullptr) == -1)
        {
            std::cerr << "Can re-install user signal handler for SIGPIPE!" << std::endl;
        }
    }
#endif
}

//---------------------------------------------------------------------------------------------------------------------
//
// method :
//         ApiUtil::get_db_ind()
//
// description :
//        Retrieve a Tango database object created from the TANGO_HOST environment variable
//
//--------------------------------------------------------------------------------------------------------------------

int ApiUtil::get_db_ind()
{
    omni_mutex_lock lo(the_mutex);

    for(unsigned int i = 0; i < db_vect.size(); i++)
    {
        if(db_vect[i]->get_from_env_var())
        {
            return i;
        }
    }

    //
    // The database object has not been found, create it
    //

    db_vect.push_back(new Database());

    return (db_vect.size() - 1);
}

int ApiUtil::get_db_ind(const std::string &host, int port)
{
    omni_mutex_lock lo(the_mutex);

    for(unsigned int i = 0; i < db_vect.size(); i++)
    {
        if(db_vect[i]->get_db_port_num() == port)
        {
            if(db_vect[i]->get_db_host() == host)
            {
                return i;
            }
        }
    }

    //
    // The database object has not been found, create it
    //

    db_vect.push_back(new Database(host, port));

    return (db_vect.size() - 1);
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::get_asynch_replies()
//
// description :
//        Try to obtain data returned by a command asynchronously requested. This method does not block if the reply is
//        not yet arrived. Fire callback for replies already arrived
//
//------------------------------------------------------------------------------------------------------------------

void ApiUtil::get_asynch_replies()
{
    //
    // First get all replies from ORB buffers
    //

    try
    {
        while(_orb->poll_next_response())
        {
            CORBA::Request_ptr req;
            _orb->get_next_response(req);

            //
            // Retrieve this request in the cb request map and mark it as "arrived" in both maps
            //

            TgRequest &tg_req = asyn_p_table->get_request(req);

            tg_req.arrived = true;
            asyn_p_table->mark_as_arrived(req);

            //
            // Process request
            //

            process_request(tg_req.dev, tg_req, req);
        }
    }
    catch(CORBA::BAD_INV_ORDER &e)
    {
        if(e.minor() != omni::BAD_INV_ORDER_RequestNotSentYet)
        {
            throw;
        }
    }

    //
    // For all replies already there
    //

    std::multimap<Connection *, TgRequest>::iterator pos;
    std::multimap<Connection *, TgRequest> &req_table = asyn_p_table->get_cb_dev_table();

    for(pos = req_table.begin(); pos != req_table.end(); ++pos)
    {
        if(pos->second.arrived)
        {
            process_request(pos->first, pos->second, pos->second.request);
        }
    }
}

void ApiUtil::process_request(Connection *connection, const TgRequest &tg_req, CORBA::Request_ptr &req)
{
    switch(tg_req.req_type)
    {
    case TgRequest::CMD_INOUT:
        connection->Cb_Cmd_Request(req, tg_req.cb_ptr);
        break;

    case TgRequest::READ_ATTR:
        connection->Cb_ReadAttr_Request(req, tg_req.cb_ptr);
        break;

    case TgRequest::WRITE_ATTR:
    case TgRequest::WRITE_ATTR_SINGLE:
        connection->Cb_WriteAttr_Request(req, tg_req.cb_ptr);
        break;
    }
    connection->dec_asynch_counter(CALL_BACK);
    asyn_p_table->remove_request(connection, req);
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::get_asynch_replies()
//
// description :
//        Try to obtain data returned by a command asynchronously requested. This method does not block if the reply is
//        not yet arrived. Fire callback for replies already arrived
//
// arg(s) :
//        in :
//            - call_timeout : The timeout value in mS
//
//-----------------------------------------------------------------------------

void ApiUtil::get_asynch_replies(long call_timeout)
{
    //
    // For all replies already there
    //

    std::multimap<Connection *, TgRequest>::iterator pos;
    std::multimap<Connection *, TgRequest> &req_table = asyn_p_table->get_cb_dev_table();

    for(pos = req_table.begin(); pos != req_table.end(); ++pos)
    {
        if(pos->second.arrived)
        {
            switch(pos->second.req_type)
            {
            case TgRequest::CMD_INOUT:
                pos->first->Cb_Cmd_Request(pos->second.request, pos->second.cb_ptr);
                break;

            case TgRequest::READ_ATTR:
                pos->first->Cb_ReadAttr_Request(pos->second.request, pos->second.cb_ptr);
                break;

            case TgRequest::WRITE_ATTR:
            case TgRequest::WRITE_ATTR_SINGLE:
                pos->first->Cb_WriteAttr_Request(pos->second.request, pos->second.cb_ptr);
                break;
            }
            pos->first->dec_asynch_counter(CALL_BACK);
            asyn_p_table->remove_request(pos->first, pos->second.request);
        }
    }

    //
    // If they are requests already sent but without being replied yet
    //

    if(asyn_p_table->get_cb_request_nb() != 0)
    {
        CORBA::Request_ptr req;

        if(call_timeout != 0)
        {
            //
            // A timeout has been specified. Wait if there are still request without replies but not more than the
            // specified timeout. Leave method if the timeout is not arrived but there is no more request without reply
            //

            long nb = call_timeout / 20;

            while((nb > 0) && (asyn_p_table->get_cb_request_nb() != 0))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                nb--;

                try
                {
                    if(_orb->poll_next_response())
                    {
                        _orb->get_next_response(req);

                        //
                        // Retrieve this request in the cb request map and mark it as "arrived" in both maps
                        //

                        TgRequest &tg_req = asyn_p_table->get_request(req);

                        tg_req.arrived = true;
                        asyn_p_table->mark_as_arrived(req);

                        //
                        // Is it a request for our device, process it ?
                        //

                        switch(tg_req.req_type)
                        {
                        case TgRequest::CMD_INOUT:
                            tg_req.dev->Cb_Cmd_Request(req, tg_req.cb_ptr);
                            break;

                        case TgRequest::READ_ATTR:
                            tg_req.dev->Cb_ReadAttr_Request(req, tg_req.cb_ptr);
                            break;

                        case TgRequest::WRITE_ATTR:
                        case TgRequest::WRITE_ATTR_SINGLE:
                            tg_req.dev->Cb_WriteAttr_Request(req, tg_req.cb_ptr);
                            break;
                        }

                        tg_req.dev->dec_asynch_counter(CALL_BACK);
                        asyn_p_table->remove_request(tg_req.dev, req);
                    }
                }
                catch(CORBA::BAD_INV_ORDER &e)
                {
                    if(e.minor() != omni::BAD_INV_ORDER_RequestNotSentYet)
                    {
                        throw;
                    }
                }
            }

            //
            // Throw exception if the timeout has expired but there are still request without replies
            //

            if((nb == 0) && (asyn_p_table->get_cb_request_nb() != 0))
            {
                TangoSys_OMemStream desc;
                desc << "Still some reply(ies) for asynchronous callback call(s) to be received" << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
            }
        }
        else
        {
            //
            // If timeout is set to 0, this means wait until all the requests sent to this device has sent their replies
            //

            while(asyn_p_table->get_cb_request_nb() != 0)
            {
                try
                {
                    _orb->get_next_response(req);

                    //
                    // Retrieve this request in the cb request map and mark it as "arrived" in both maps
                    //

                    TgRequest &tg_req = asyn_p_table->get_request(req);

                    tg_req.arrived = true;
                    asyn_p_table->mark_as_arrived(req);

                    //
                    // Process the reply
                    //

                    switch(tg_req.req_type)
                    {
                    case TgRequest::CMD_INOUT:
                        tg_req.dev->Cb_Cmd_Request(req, tg_req.cb_ptr);
                        break;

                    case TgRequest::READ_ATTR:
                        tg_req.dev->Cb_ReadAttr_Request(req, tg_req.cb_ptr);
                        break;

                    case TgRequest::WRITE_ATTR:
                    case TgRequest::WRITE_ATTR_SINGLE:
                        tg_req.dev->Cb_WriteAttr_Request(req, tg_req.cb_ptr);
                        break;
                    }

                    tg_req.dev->dec_asynch_counter(CALL_BACK);
                    asyn_p_table->remove_request(tg_req.dev, req);
                }
                catch(CORBA::BAD_INV_ORDER &e)
                {
                    if(e.minor() != omni::BAD_INV_ORDER_RequestNotSentYet)
                    {
                        throw;
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::set_asynch_cb_sub_model()
//
// description :
//        Set the callback automatic mode (Fired by dedicated call or automatically fired by a separate thread)
//
// arg(s) :
//        in :
//            - mode : The new callback mode
//
//--------------------------------------------------------------------------------------------------------------------

void ApiUtil::set_asynch_cb_sub_model(cb_sub_model mode)
{
    if(auto_cb == PULL_CALLBACK)
    {
        if(mode == PUSH_CALLBACK)
        {
            //
            // In this case, delete the old object in case it is needed, create a new thread and start it
            //

            delete cb_thread_ptr;
            cb_thread_ptr = nullptr;

            cb_thread_cmd.start_thread();

            cb_thread_ptr = new CallBackThread(cb_thread_cmd, asyn_p_table);
            cb_thread_ptr->start();
            auto_cb = PUSH_CALLBACK;
        }
    }
    else
    {
        if(mode == PULL_CALLBACK)
        {
            //
            // Ask the thread to stop and to exit
            //

            cb_thread_cmd.stop_thread();
            auto_cb = PULL_CALLBACK;
            asyn_p_table->signal();
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::create_xxx_event_consumer()
//
// description :
//        Create the event consumer. This will automatically start a new thread which is waiting in a CORBA::run()
//        indefintely for events. It will then trigger the events.
//
//----------------------------------------------------------------------------------------------------------------

PointerWithLock<EventConsumer> ApiUtil::create_notifd_event_consumer()
{
    if(!is_notifd_event_consumer_created())
    {
        WriterLock lock{notifd_rw_lock};
        notifd_event_consumer = new NotifdEventConsumer(this);
    }

    return get_notifd_event_consumer();
}

PointerWithLock<EventConsumer> ApiUtil::create_zmq_event_consumer()
{
    if(!is_zmq_event_consumer_created())
    {
        WriterLock lock{zmq_rw_lock};
        zmq_event_consumer = new ZmqEventConsumer(this);
    }

    return get_zmq_event_consumer();
}

PointerWithLock<EventConsumer> ApiUtil::get_notifd_event_consumer()
{
    return PointerWithLock<EventConsumer>{notifd_event_consumer, notifd_rw_lock};
}

PointerWithLock<EventConsumer> ApiUtil::get_zmq_event_consumer()
{
    return PointerWithLock<EventConsumer>{zmq_event_consumer, zmq_rw_lock};
}

bool ApiUtil::is_notifd_event_consumer_created()
{
    return get_notifd_event_consumer() != nullptr;
}

bool ApiUtil::is_zmq_event_consumer_created()
{
    return get_zmq_event_consumer() != nullptr;
}

// The caller is required to hold the read lock on the event consumer behind ptr
PointerWithLock<ZmqEventConsumer> ApiUtil::get_zmq_event_consumer_derived(EventConsumer *ptr)
{
    if(ptr == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_InvalidArgs, "Can't handle nullptr");
    }

    {
        ReaderLock lock{zmq_rw_lock};

        if(zmq_event_consumer != nullptr && dynamic_cast<EventConsumer *>(zmq_event_consumer) == ptr)
        {
            return PointerWithLock<ZmqEventConsumer>(zmq_event_consumer, zmq_rw_lock);
        }
    }

    TANGO_THROW_EXCEPTION(API_InvalidArgs, "Could not find event consumer for ptr");
}

// The caller is required to hold the read lock on the event consumer behind ptr
PointerWithLock<EventConsumer> ApiUtil::get_locked_event_consumer(EventConsumer *ptr)
{
    if(ptr == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_InvalidArgs, "Can't handle nullptr");
    }

    {
        ReaderLock lock{zmq_rw_lock};

        if(zmq_event_consumer != nullptr && dynamic_cast<EventConsumer *>(zmq_event_consumer) == ptr)
        {
            return PointerWithLock<EventConsumer>(zmq_event_consumer, zmq_rw_lock);
        }
    }

    {
        ReaderLock lock{notifd_rw_lock};

        if(notifd_event_consumer != nullptr && dynamic_cast<EventConsumer *>(notifd_event_consumer) == ptr)
        {
            return PointerWithLock<EventConsumer>(notifd_event_consumer, notifd_rw_lock);
        }
    }

    TANGO_THROW_EXCEPTION(API_InvalidArgs, "Could not find event consumer for ptr");
}

void ApiUtil::shutdown_event_consumers()
{
    {
        WriterLock lock{notifd_rw_lock};

        if(notifd_event_consumer != nullptr)
        {
            notifd_event_consumer->shutdown();

            //
            // Shut-down the notifd ORB and wait for the thread to exit
            //

            int *rv;
            notifd_event_consumer->orb_->shutdown(true);
            notifd_event_consumer->join((void **) &rv);
            notifd_event_consumer = nullptr;
        }
    }

    {
        WriterLock lock{zmq_rw_lock};

        if(zmq_event_consumer != nullptr)
        {
            int *rv;
            zmq_event_consumer->shutdown();
            zmq_event_consumer->join((void **) &rv);
            zmq_event_consumer = nullptr;
        }
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::clean_locking_threads()
//
// description :
//        Ask all remaining locking threads to exit
//
// args :
//        in :
//            - clean :
//
//------------------------------------------------------------------------------------------------------------------

void ApiUtil::clean_locking_threads(bool clean)
{
    omni_mutex_lock oml(lock_th_map);

    if(!lock_threads.empty())
    {
        std::map<std::string, LockingThread>::iterator pos;
        for(pos = lock_threads.begin(); pos != lock_threads.end(); ++pos)
        {
            if(pos->second.shared->suicide)
            {
                delete pos->second.shared;
                delete pos->second.mon;
            }
            else
            {
                {
                    omni_mutex_lock sync(*(pos->second.mon));

                    pos->second.shared->cmd_pending = true;
                    if(clean)
                    {
                        pos->second.shared->cmd_code = LOCK_UNLOCK_ALL_EXIT;
                    }
                    else
                    {
                        pos->second.shared->cmd_code = LOCK_EXIT;
                    }

                    pos->second.mon->signal();

                    TANGO_LOG_DEBUG << "Cmd sent to locking thread" << std::endl;

                    if(pos->second.shared->cmd_pending)
                    {
                        pos->second.mon->wait(DEFAULT_TIMEOUT);
                    }
                }
                delete pos->second.shared;
                delete pos->second.mon;
            }
        }

        if(!clean)
        {
            lock_threads.clear();
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::attr_to_device()
//
// description :
//
//
//-------------------------------------------------------------------------------------------------------------------

void ApiUtil::attr_to_device(const AttributeValue *attr_value,
                             const AttributeValue_3 *attr_value_3,
                             long vers,
                             DeviceAttribute *dev_attr)
{
    const DevVarLongArray *tmp_seq_lo;
    CORBA::Long *tmp_lo;
    const DevVarShortArray *tmp_seq_sh;
    CORBA::Short *tmp_sh;
    const DevVarDoubleArray *tmp_seq_db;
    CORBA::Double *tmp_db;
    const DevVarStringArray *tmp_seq_str;
    char **tmp_str;
    const DevVarFloatArray *tmp_seq_fl;
    CORBA::Float *tmp_fl;
    const DevVarBooleanArray *tmp_seq_boo;
    CORBA::Boolean *tmp_boo;
    const DevVarUShortArray *tmp_seq_ush;
    CORBA::UShort *tmp_ush;
    const DevVarCharArray *tmp_seq_uch;
    CORBA::Octet *tmp_uch;
    const DevVarLong64Array *tmp_seq_64;
    CORBA::LongLong *tmp_lolo;
    const DevVarULongArray *tmp_seq_ulo;
    CORBA::ULong *tmp_ulo;
    const DevVarULong64Array *tmp_seq_u64;
    CORBA::ULongLong *tmp_ulolo;
    const DevVarStateArray *tmp_seq_state;
    Tango::DevState *tmp_state;

    CORBA::ULong max, len;

    if(vers == 3)
    {
        dev_attr->name = attr_value_3->name;
        dev_attr->quality = attr_value_3->quality;
        dev_attr->time = attr_value_3->time;
        dev_attr->dim_x = attr_value_3->r_dim.dim_x;
        dev_attr->dim_y = attr_value_3->r_dim.dim_y;
        dev_attr->set_w_dim_x(attr_value_3->w_dim.dim_x);
        dev_attr->set_w_dim_y(attr_value_3->w_dim.dim_y);
        dev_attr->err_list = new DevErrorList(attr_value_3->err_list);
    }
    else
    {
        dev_attr->name = attr_value->name;
        dev_attr->quality = attr_value->quality;
        dev_attr->time = attr_value->time;
        dev_attr->dim_x = attr_value->dim_x;
        dev_attr->dim_y = attr_value->dim_y;
    }

    if(dev_attr->quality != Tango::ATTR_INVALID)
    {
        CORBA::TypeCode_var ty;
        if(vers == 3)
        {
            ty = attr_value_3->value.type();
        }
        else
        {
            ty = attr_value->value.type();
        }

        if(ty->kind() == CORBA::tk_enum)
        {
            dev_attr->data_type = Tango::DEV_STATE;
            attr_value_3->value >>= dev_attr->d_state;
            dev_attr->d_state_filled = true;
            return;
        }

        CORBA::TypeCode_var ty_alias = ty->content_type();
        CORBA::TypeCode_var ty_seq = ty_alias->content_type();
        switch(ty_seq->kind())
        {
        case CORBA::tk_long:
            dev_attr->data_type = Tango::DEV_LONG;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_lo;
            }
            else
            {
                attr_value->value >>= tmp_seq_lo;
            }
            max = tmp_seq_lo->maximum();
            len = tmp_seq_lo->length();
            if(tmp_seq_lo->release())
            {
                tmp_lo = (const_cast<DevVarLongArray *>(tmp_seq_lo))->get_buffer((CORBA::Boolean) true);
                dev_attr->LongSeq = new DevVarLongArray(max, len, tmp_lo, true);
            }
            else
            {
                tmp_lo = const_cast<CORBA::Long *>(tmp_seq_lo->get_buffer());
                dev_attr->LongSeq = new DevVarLongArray(max, len, tmp_lo, false);
            }
            break;

        case CORBA::tk_longlong:
            dev_attr->data_type = Tango::DEV_LONG64;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_64;
            }
            else
            {
                attr_value->value >>= tmp_seq_64;
            }
            max = tmp_seq_64->maximum();
            len = tmp_seq_64->length();
            if(tmp_seq_64->release())
            {
                tmp_lolo = (const_cast<DevVarLong64Array *>(tmp_seq_64))->get_buffer((CORBA::Boolean) true);
                dev_attr->Long64Seq = new DevVarLong64Array(max, len, tmp_lolo, true);
            }
            else
            {
                tmp_lolo = const_cast<CORBA::LongLong *>(tmp_seq_64->get_buffer());
                dev_attr->Long64Seq = new DevVarLong64Array(max, len, tmp_lolo, false);
            }
            break;

        case CORBA::tk_short:
            dev_attr->data_type = Tango::DEV_SHORT;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_sh;
            }
            else
            {
                attr_value->value >>= tmp_seq_sh;
            }
            max = tmp_seq_sh->maximum();
            len = tmp_seq_sh->length();
            if(tmp_seq_sh->release())
            {
                tmp_sh = (const_cast<DevVarShortArray *>(tmp_seq_sh))->get_buffer((CORBA::Boolean) true);
                dev_attr->ShortSeq = new DevVarShortArray(max, len, tmp_sh, true);
            }
            else
            {
                tmp_sh = const_cast<CORBA::Short *>(tmp_seq_sh->get_buffer());
                dev_attr->ShortSeq = new DevVarShortArray(max, len, tmp_sh, false);
            }
            break;

        case CORBA::tk_double:
            dev_attr->data_type = Tango::DEV_DOUBLE;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_db;
            }
            else
            {
                attr_value->value >>= tmp_seq_db;
            }
            max = tmp_seq_db->maximum();
            len = tmp_seq_db->length();
            if(tmp_seq_db->release())
            {
                tmp_db = (const_cast<DevVarDoubleArray *>(tmp_seq_db))->get_buffer((CORBA::Boolean) true);
                dev_attr->DoubleSeq = new DevVarDoubleArray(max, len, tmp_db, true);
            }
            else
            {
                tmp_db = const_cast<CORBA::Double *>(tmp_seq_db->get_buffer());
                dev_attr->DoubleSeq = new DevVarDoubleArray(max, len, tmp_db, false);
            }
            break;

        case CORBA::tk_string:
            dev_attr->data_type = Tango::DEV_STRING;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_str;
            }
            else
            {
                attr_value->value >>= tmp_seq_str;
            }
            max = tmp_seq_str->maximum();
            len = tmp_seq_str->length();
            if(tmp_seq_str->release())
            {
                tmp_str = (const_cast<DevVarStringArray *>(tmp_seq_str))->get_buffer((CORBA::Boolean) true);
                dev_attr->StringSeq = new DevVarStringArray(max, len, tmp_str, true);
            }
            else
            {
                tmp_str = const_cast<char **>(tmp_seq_str->get_buffer());
                dev_attr->StringSeq = new DevVarStringArray(max, len, tmp_str, false);
            }
            break;

        case CORBA::tk_float:
            dev_attr->data_type = Tango::DEV_FLOAT;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_fl;
            }
            else
            {
                attr_value->value >>= tmp_seq_fl;
            }
            max = tmp_seq_fl->maximum();
            len = tmp_seq_fl->length();
            if(tmp_seq_fl->release())
            {
                tmp_fl = (const_cast<DevVarFloatArray *>(tmp_seq_fl))->get_buffer((CORBA::Boolean) true);
                dev_attr->FloatSeq = new DevVarFloatArray(max, len, tmp_fl, true);
            }
            else
            {
                tmp_fl = const_cast<CORBA::Float *>(tmp_seq_fl->get_buffer());
                dev_attr->FloatSeq = new DevVarFloatArray(max, len, tmp_fl, false);
            }
            break;

        case CORBA::tk_boolean:
            dev_attr->data_type = Tango::DEV_BOOLEAN;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_boo;
            }
            else
            {
                attr_value->value >>= tmp_seq_boo;
            }
            max = tmp_seq_boo->maximum();
            len = tmp_seq_boo->length();
            if(tmp_seq_boo->release())
            {
                tmp_boo = (const_cast<DevVarBooleanArray *>(tmp_seq_boo))->get_buffer((CORBA::Boolean) true);
                dev_attr->BooleanSeq = new DevVarBooleanArray(max, len, tmp_boo, true);
            }
            else
            {
                tmp_boo = const_cast<CORBA::Boolean *>(tmp_seq_boo->get_buffer());
                dev_attr->BooleanSeq = new DevVarBooleanArray(max, len, tmp_boo, false);
            }
            break;

        case CORBA::tk_ushort:
            dev_attr->data_type = Tango::DEV_USHORT;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_ush;
            }
            else
            {
                attr_value->value >>= tmp_seq_ush;
            }
            max = tmp_seq_ush->maximum();
            len = tmp_seq_ush->length();
            if(tmp_seq_ush->release())
            {
                tmp_ush = (const_cast<DevVarUShortArray *>(tmp_seq_ush))->get_buffer((CORBA::Boolean) true);
                dev_attr->UShortSeq = new DevVarUShortArray(max, len, tmp_ush, true);
            }
            else
            {
                tmp_ush = const_cast<CORBA::UShort *>(tmp_seq_ush->get_buffer());
                dev_attr->UShortSeq = new DevVarUShortArray(max, len, tmp_ush, false);
            }
            break;

        case CORBA::tk_octet:
            dev_attr->data_type = Tango::DEV_UCHAR;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_uch;
            }
            else
            {
                attr_value->value >>= tmp_seq_uch;
            }
            max = tmp_seq_uch->maximum();
            len = tmp_seq_uch->length();
            if(tmp_seq_uch->release())
            {
                tmp_uch = (const_cast<DevVarCharArray *>(tmp_seq_uch))->get_buffer((CORBA::Boolean) true);
                dev_attr->UCharSeq = new DevVarCharArray(max, len, tmp_uch, true);
            }
            else
            {
                tmp_uch = const_cast<CORBA::Octet *>(tmp_seq_uch->get_buffer());
                dev_attr->UCharSeq = new DevVarCharArray(max, len, tmp_uch, false);
            }
            break;

        case CORBA::tk_ulong:
            dev_attr->data_type = Tango::DEV_ULONG;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_ulo;
            }
            else
            {
                attr_value->value >>= tmp_seq_ulo;
            }
            max = tmp_seq_ulo->maximum();
            len = tmp_seq_ulo->length();
            if(tmp_seq_ulo->release())
            {
                tmp_ulo = (const_cast<DevVarULongArray *>(tmp_seq_ulo))->get_buffer((CORBA::Boolean) true);
                dev_attr->ULongSeq = new DevVarULongArray(max, len, tmp_ulo, true);
            }
            else
            {
                tmp_ulo = const_cast<CORBA::ULong *>(tmp_seq_ulo->get_buffer());
                dev_attr->ULongSeq = new DevVarULongArray(max, len, tmp_ulo, false);
            }
            break;

        case CORBA::tk_ulonglong:
            dev_attr->data_type = Tango::DEV_ULONG64;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_u64;
            }
            else
            {
                attr_value->value >>= tmp_seq_u64;
            }
            max = tmp_seq_u64->maximum();
            len = tmp_seq_u64->length();
            if(tmp_seq_u64->release())
            {
                tmp_ulolo = (const_cast<DevVarULong64Array *>(tmp_seq_u64))->get_buffer((CORBA::Boolean) true);
                dev_attr->ULong64Seq = new DevVarULong64Array(max, len, tmp_ulolo, true);
            }
            else
            {
                tmp_ulolo = const_cast<CORBA::ULongLong *>(tmp_seq_u64->get_buffer());
                dev_attr->ULong64Seq = new DevVarULong64Array(max, len, tmp_ulolo, false);
            }
            break;

        case CORBA::tk_enum:
            dev_attr->data_type = Tango::DEV_STATE;
            if(vers == 3)
            {
                attr_value_3->value >>= tmp_seq_state;
            }
            else
            {
                attr_value->value >>= tmp_seq_state;
            }
            max = tmp_seq_state->maximum();
            len = tmp_seq_state->length();
            if(tmp_seq_state->release())
            {
                tmp_state = (const_cast<DevVarStateArray *>(tmp_seq_state))->get_buffer((CORBA::Boolean) true);
                dev_attr->StateSeq = new DevVarStateArray(max, len, tmp_state, true);
            }
            else
            {
                tmp_state = const_cast<Tango::DevState *>(tmp_seq_state->get_buffer());
                dev_attr->StateSeq = new DevVarStateArray(max, len, tmp_state, false);
            }
            break;

        default:
            dev_attr->data_type = Tango::DATA_TYPE_UNKNOWN;
            TangoSys_OMemStream desc;
            desc << (vers == 3 ? "'attr_value_3" : "'attr_value") << "->value' with unexpected sequence kind '"
                 << ty_seq->kind() << "'.";
            TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
        }
    }
}

void ApiUtil::attr_to_device(const AttributeValue_4 *attr_value_4, TANGO_UNUSED(long vers), DeviceAttribute *dev_attr)
{
    attr_to_device_base(attr_value_4, dev_attr);
}

void ApiUtil::attr_to_device(const AttributeValue_5 *attr_value_5, TANGO_UNUSED(long vers), DeviceAttribute *dev_attr)
{
    attr_to_device_base(attr_value_5, dev_attr);
    dev_attr->data_type = attr_value_5->data_type;
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::device_to_attr()
//
// description :
//        initialize one AttributeValue instance from a DeviceAttribute one
//
// arg(s) :
//        in :
//            - dev_attr : The DeviceAttribute instance taken as source
//        out :
//            - att : The AttributeValue used as destination
//
//---------------------------------------------------------------------------------------------------------------

void ApiUtil::device_to_attr(const DeviceAttribute &dev_attr, AttributeValue_4 &att)
{
    att.name = dev_attr.name.c_str();
    att.quality = dev_attr.quality;
    att.time = dev_attr.time;
    att.w_dim.dim_x = dev_attr.dim_x;
    att.w_dim.dim_y = dev_attr.dim_y;
    att.data_format = Tango::FMT_UNKNOWN;

    if(dev_attr.LongSeq.operator->() != nullptr)
    {
        att.value.long_att_value(dev_attr.LongSeq.in());
    }
    else if(dev_attr.ShortSeq.operator->() != nullptr)
    {
        att.value.short_att_value(dev_attr.ShortSeq.in());
    }
    else if(dev_attr.DoubleSeq.operator->() != nullptr)
    {
        att.value.double_att_value(dev_attr.DoubleSeq.in());
    }
    else if(dev_attr.StringSeq.operator->() != nullptr)
    {
        att.value.string_att_value(dev_attr.StringSeq.in());
    }
    else if(dev_attr.FloatSeq.operator->() != nullptr)
    {
        att.value.float_att_value(dev_attr.FloatSeq.in());
    }
    else if(dev_attr.BooleanSeq.operator->() != nullptr)
    {
        att.value.bool_att_value(dev_attr.BooleanSeq.in());
    }
    else if(dev_attr.UShortSeq.operator->() != nullptr)
    {
        att.value.ushort_att_value(dev_attr.UShortSeq.in());
    }
    else if(dev_attr.UCharSeq.operator->() != nullptr)
    {
        att.value.uchar_att_value(dev_attr.UCharSeq.in());
    }
    else if(dev_attr.Long64Seq.operator->() != nullptr)
    {
        att.value.long64_att_value(dev_attr.Long64Seq.in());
    }
    else if(dev_attr.ULongSeq.operator->() != nullptr)
    {
        att.value.ulong_att_value(dev_attr.ULongSeq.in());
    }
    else if(dev_attr.ULong64Seq.operator->() != nullptr)
    {
        att.value.ulong64_att_value(dev_attr.ULong64Seq.in());
    }
    else if(dev_attr.StateSeq.operator->() != nullptr)
    {
        att.value.state_att_value(dev_attr.StateSeq.in());
    }
    else if(dev_attr.EncodedSeq.operator->() != nullptr)
    {
        att.value.encoded_att_value(dev_attr.EncodedSeq.in());
    }
}

void ApiUtil::device_to_attr(const DeviceAttribute &dev_attr, AttributeValue &att, const std::string &d_name)
{
    att.name = dev_attr.name.c_str();
    att.quality = dev_attr.quality;
    att.time = dev_attr.time;
    att.dim_x = dev_attr.dim_x;
    att.dim_y = dev_attr.dim_y;

    if(dev_attr.LongSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.LongSeq.in();
    }
    else if(dev_attr.ShortSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.ShortSeq.in();
    }
    else if(dev_attr.DoubleSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.DoubleSeq.in();
    }
    else if(dev_attr.StringSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.StringSeq.in();
    }
    else if(dev_attr.FloatSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.FloatSeq.in();
    }
    else if(dev_attr.BooleanSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.BooleanSeq.in();
    }
    else if(dev_attr.UShortSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.UShortSeq.in();
    }
    else if(dev_attr.UCharSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.UCharSeq.in();
    }
    else if(dev_attr.Long64Seq.operator->() != nullptr)
    {
        att.value <<= dev_attr.Long64Seq.in();
    }
    else if(dev_attr.ULongSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.ULongSeq.in();
    }
    else if(dev_attr.ULong64Seq.operator->() != nullptr)
    {
        att.value <<= dev_attr.ULong64Seq.in();
    }
    else if(dev_attr.StateSeq.operator->() != nullptr)
    {
        att.value <<= dev_attr.StateSeq.in();
    }
    else if(dev_attr.EncodedSeq.operator->() != nullptr)
    {
        TangoSys_OMemStream desc;
        desc << "Device " << d_name;
        desc << " does not support DevEncoded data type" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, desc.str());
    }
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::AttributeInfoEx_to_AttributeConfig()
//
// description :
//        Initialize one AttributeConfig instance from a AttributeInfoEx one
//
// arg(s) :
//        in :
//            - aie : The AttributeInfoEx instance taken as source
//        out :
//            - att_conf_5 : The AttributeConfig used as destination
//
//------------------------------------------------------------------------------------------------------------------

void ApiUtil::AttributeInfoEx_to_AttributeConfig(const AttributeInfoEx *aie, AttributeConfig_5 *att_conf_5)
{
    att_conf_5->name = aie->name.c_str();
    att_conf_5->writable = aie->writable;
    att_conf_5->data_format = aie->data_format;
    att_conf_5->data_type = aie->data_type;
    att_conf_5->max_dim_x = aie->max_dim_x;
    att_conf_5->max_dim_y = aie->max_dim_y;
    att_conf_5->description = aie->description.c_str();
    att_conf_5->label = aie->label.c_str();
    att_conf_5->unit = aie->unit.c_str();
    att_conf_5->standard_unit = aie->standard_unit.c_str();
    att_conf_5->display_unit = aie->display_unit.c_str();
    att_conf_5->format = aie->format.c_str();
    att_conf_5->min_value = aie->min_value.c_str();
    att_conf_5->max_value = aie->max_value.c_str();
    att_conf_5->writable_attr_name = aie->writable_attr_name.c_str();
    att_conf_5->level = aie->disp_level;
    att_conf_5->root_attr_name = aie->root_attr_name.c_str();
    switch(aie->memorized)
    {
    case NOT_KNOWN:
    case NONE:
        att_conf_5->memorized = false;
        att_conf_5->mem_init = false;
        break;

    case MEMORIZED:
        att_conf_5->memorized = true;
        att_conf_5->mem_init = false;
        break;

    case MEMORIZED_WRITE_INIT:
        att_conf_5->memorized = true;
        att_conf_5->mem_init = true;
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(aie->memorized);
    }
    att_conf_5->enum_labels.length(aie->enum_labels.size());
    for(size_t j = 0; j < aie->enum_labels.size(); j++)
    {
        att_conf_5->enum_labels[j] = string_dup(aie->enum_labels[j].c_str());
    }
    att_conf_5->extensions.length(aie->extensions.size());
    for(size_t j = 0; j < aie->extensions.size(); j++)
    {
        att_conf_5->extensions[j] = string_dup(aie->extensions[j].c_str());
    }
    for(size_t j = 0; j < aie->sys_extensions.size(); j++)
    {
        att_conf_5->sys_extensions[j] = string_dup(aie->sys_extensions[j].c_str());
    }

    att_conf_5->att_alarm.min_alarm = aie->alarms.min_alarm.c_str();
    att_conf_5->att_alarm.max_alarm = aie->alarms.max_alarm.c_str();
    att_conf_5->att_alarm.min_warning = aie->alarms.min_warning.c_str();
    att_conf_5->att_alarm.max_warning = aie->alarms.max_warning.c_str();
    att_conf_5->att_alarm.delta_t = aie->alarms.delta_t.c_str();
    att_conf_5->att_alarm.delta_val = aie->alarms.delta_val.c_str();
    for(size_t j = 0; j < aie->alarms.extensions.size(); j++)
    {
        att_conf_5->att_alarm.extensions[j] = string_dup(aie->alarms.extensions[j].c_str());
    }

    att_conf_5->event_prop.ch_event.rel_change = aie->events.ch_event.rel_change.c_str();
    att_conf_5->event_prop.ch_event.abs_change = aie->events.ch_event.abs_change.c_str();
    for(size_t j = 0; j < aie->events.ch_event.extensions.size(); j++)
    {
        att_conf_5->event_prop.ch_event.extensions[j] = string_dup(aie->events.ch_event.extensions[j].c_str());
    }

    att_conf_5->event_prop.per_event.period = aie->events.per_event.period.c_str();
    for(size_t j = 0; j < aie->events.per_event.extensions.size(); j++)
    {
        att_conf_5->event_prop.per_event.extensions[j] = string_dup(aie->events.per_event.extensions[j].c_str());
    }

    att_conf_5->event_prop.arch_event.rel_change = aie->events.arch_event.archive_rel_change.c_str();
    att_conf_5->event_prop.arch_event.abs_change = aie->events.arch_event.archive_abs_change.c_str();
    att_conf_5->event_prop.arch_event.period = aie->events.arch_event.archive_period.c_str();
    for(size_t j = 0; j < aie->events.ch_event.extensions.size(); j++)
    {
        att_conf_5->event_prop.arch_event.extensions[j] = string_dup(aie->events.arch_event.extensions[j].c_str());
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::get_env_var()
//
// description :
//        Get environment variable
//
// arg(s) :
//        in :
//            - env_var_name : The environment variable name
//        out :
//            - env_var : Reference to the string initialised with the env. variable value (if found)
//
// return :
//        0 if the env. variable is found and -1 otherwise
//
//----------------------------------------------------------------------------------------------------------------

int ApiUtil::get_env_var(const char *env_var_name, std::string &env_var)
{
    DummyDeviceProxy d;
    return d.get_env_var(env_var_name, env_var);
}

//---------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::get_ip_from_if()
//
// description :
//        Get host IP address from its network interface
//
// arg(s) :
//        out :
//            - ip_adr_list : Host IP address
//
//----------------------------------------------------------------------------------------------------------------

void ApiUtil::get_ip_from_if(std::vector<std::string> &ip_adr_list)
{
    omni_mutex_lock oml(lock_th_map);

    if(host_ip_adrs.empty())
    {
#ifndef _TG_WINDOWS_
        struct ifaddrs *ifaddr, *ifa;
        int family, s;
        char host[NI_MAXHOST];

        if(getifaddrs(&ifaddr) == -1)
        {
            std::cerr << "ApiUtil::get_ip_from_if: getifaddrs() failed: " << strerror(errno) << std::endl;

            TANGO_THROW_EXCEPTION(API_SystemCallFailed, strerror(errno));
        }

        //
        // Walk through linked list, maintaining head pointer so we can free list later. The ifa_addr field points to a
        // structure containing the interface address. (The sa_family subfield should be consulted to determine the
        // format of the address structure.)
        //

        for(ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if(ifa->ifa_addr != nullptr)
            {
                family = ifa->ifa_addr->sa_family;

                //
                // Only get IP V4 addresses
                //

                /*              if(family == AF_INET || family == AF_INET6)
                                {
                                    s = getnameinfo(ifa->ifa_addr,(family == AF_INET) ? sizeof(struct sockaddr_in) :
                   sizeof(struct sockaddr_in6), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);*/

                if(family == AF_INET)
                {
                    s = getnameinfo(
                        ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);

                    if(s != 0)
                    {
                        std::cerr << "ApiUtil::get_ip_from_if: getnameinfo() failed: " << gai_strerror(s);
                        std::cerr << "ApiUtil::get_ip_from_if: not getting info from remaining ifaddrs";

                        freeifaddrs(ifaddr);

                        TANGO_THROW_EXCEPTION(API_SystemCallFailed, gai_strerror(s));
                    }
                    else
                    {
                        host_ip_adrs.emplace_back(host);
                    }
                }
            }
        }

        freeifaddrs(ifaddr);
#else

        //
        // Get address from interfaces
        //

        int sock = socket(AF_INET, SOCK_STREAM, 0);

        if(sock == INVALID_SOCKET)
        {
            int err = WSAGetLastError();
            TangoSys_OMemStream desc;
            desc << "Unable to create socket! Error = " << err;
            TANGO_THROW_EXCEPTION(API_SystemCallFailed, desc.str().c_str());
        }

        INTERFACE_INFO info[64]; // Assume max 64 interfaces
        DWORD retlen;

        if(WSAIoctl(sock,
                    SIO_GET_INTERFACE_LIST,
                    nullptr,
                    0,
                    (LPVOID) &info,
                    sizeof(info),
                    (LPDWORD) &retlen,
                    nullptr,
                    nullptr) == SOCKET_ERROR)
        {
            closesocket(sock);

            int err = WSAGetLastError();
            std::cerr << "Warning: WSAIoctl failed" << std::endl;
            std::cerr << "Unable to obtain the list of all interface addresses. Error = " << err << std::endl;

            TangoSys_OMemStream desc;
            desc << "Can't retrieve list of all interfaces addresses (WSAIoctl)! Error = " << err << std::ends;

            TANGO_THROW_EXCEPTION(API_SystemCallFailed, desc.str().c_str());
        }
        closesocket(sock);

        //
        // Converts addresses to string. Only for running interfaces
        //

        int numAddresses = retlen / sizeof(INTERFACE_INFO);
        for(int i = 0; i < numAddresses; i++)
        {
            if(info[i].iiFlags & IFF_UP)
            {
                if(info[i].iiAddress.Address.sa_family == AF_INET || info[i].iiAddress.Address.sa_family == AF_INET6)
                {
                    struct sockaddr *addr = (struct sockaddr *) &info[i].iiAddress.AddressIn;
                    char dest[NI_MAXHOST];
                    socklen_t addrlen = sizeof(sockaddr);

                    int result = getnameinfo(addr, addrlen, dest, sizeof(dest), 0, 0, NI_NUMERICHOST);
                    if(result != 0)
                    {
                        std::cerr << "Warning: getnameinfo failed" << std::endl;
                        std::cerr << "Unable to convert IP address to string (getnameinfo)." << std::endl;

                        TANGO_THROW_EXCEPTION(API_SystemCallFailed,
                                              "Can't convert IP address to string (getnameinfo)!");
                    }
                    std::string tmp_str(dest);
                    if(tmp_str != "0.0.0.0" && tmp_str != "0:0:0:0:0:0:0:0" && tmp_str != "::")
                    {
                        host_ip_adrs.push_back(tmp_str);
                    }
                }
            }
        }
#endif
    }

    //
    // Copy host IP addresses into caller vector
    //

    ip_adr_list.clear();
    copy(host_ip_adrs.begin(), host_ip_adrs.end(), back_inserter(ip_adr_list));
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//        ApiUtil::print_error_message
//
// description :
//        Print error message on stderr but first print date
//
// argument :
//        in :
//            - mess : The user error message
//
//---------------------------------------------------------------------------------------------------------------------

void ApiUtil::print_error_message(const char *mess)
{
    const std::tm tm = Tango_localtime(Tango::get_current_system_datetime());
    std::cerr << std::put_time(&tm, "%c") << ": " << mess << std::endl;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// function
//         operator overloading :     <<
//
// description :
//        Friend function to ease printing instance of the AttributeInfo class
//
//-----------------------------------------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &o_str, const AttributeInfo &p)
{
    //
    // Print all these properties
    //

    o_str << "Attribute name = " << p.name << std::endl;
    o_str << "Attribute data_type = " << (CmdArgType) p.data_type << std::endl;

    o_str << "Attribute data_format = ";
    switch(p.data_format)
    {
    case Tango::FMT_UNKNOWN:
        break;

    case Tango::SCALAR:
        o_str << "scalar" << std::endl;
        break;

    case Tango::SPECTRUM:
        o_str << "spectrum, max_dim_x = " << p.max_dim_x << std::endl;
        break;

    case Tango::IMAGE:
        o_str << "image, max_dim_x = " << p.max_dim_x << ", max_dim_y = " << p.max_dim_y << std::endl;
        break;
    }

    if((p.writable == Tango::WRITE) || (p.writable == Tango::READ_WRITE))
    {
        o_str << "Attribute is writable" << std::endl;
    }
    else
    {
        o_str << "Attribute is not writable" << std::endl;
    }
    o_str << "Attribute label = " << p.label << std::endl;
    o_str << "Attribute description = " << p.description << std::endl;
    o_str << "Attribute unit = " << p.unit;
    o_str << ", standard unit = " << p.standard_unit;
    o_str << ", display unit = " << p.display_unit << std::endl;
    o_str << "Attribute format = " << p.format << std::endl;
    o_str << "Attribute min alarm = " << p.min_alarm << std::endl;
    o_str << "Attribute max alarm = " << p.max_alarm << std::endl;
    o_str << "Attribute min value = " << p.min_value << std::endl;
    o_str << "Attribute max value = " << p.max_value << std::endl;
    o_str << "Attribute writable_attr_name = " << p.writable_attr_name;

    return o_str;
}

//+----------------------------------------------------------------------------------------------------------------
//
// function :
//         operator overloading :     =
//
// description :
//        Assignement operator for the AttributeInfoEx class from a AttributeConfig_2 pointer
//
//----------------------------------------------------------------------------------------------------------------

AttributeInfoEx &AttributeInfoEx::operator=(const AttributeConfig_2 *att_2)
{
    name = att_2->name;
    writable = att_2->writable;
    data_format = att_2->data_format;
    data_type = att_2->data_type;
    max_dim_x = att_2->max_dim_x;
    max_dim_y = att_2->max_dim_y;
    description = att_2->description;
    label = att_2->label;
    unit = att_2->unit;
    standard_unit = att_2->standard_unit;
    display_unit = att_2->display_unit;
    format = att_2->format;
    min_value = att_2->min_value;
    max_value = att_2->max_value;
    min_alarm = att_2->min_alarm;
    max_alarm = att_2->max_alarm;
    writable_attr_name = att_2->writable_attr_name;
    extensions.resize(att_2->extensions.length());
    for(unsigned int j = 0; j < att_2->extensions.length(); j++)
    {
        extensions[j] = att_2->extensions[j];
    }
    disp_level = att_2->level;

    return *this;
}

//+---------------------------------------------------------------------------------------------------------------
//
// function :
//         operator overloading :     =
//
// description :
//        Assignement operator for the AttributeInfoEx class from a AttributeConfig_3 pointer
//
//-----------------------------------------------------------------------------------------------------------------

AttributeInfoEx &AttributeInfoEx::operator=(const AttributeConfig_3 *att_3)
{
    name = att_3->name;
    writable = att_3->writable;
    data_format = att_3->data_format;
    data_type = att_3->data_type;
    max_dim_x = att_3->max_dim_x;
    max_dim_y = att_3->max_dim_y;
    description = att_3->description;
    label = att_3->label;
    unit = att_3->unit;
    standard_unit = att_3->standard_unit;
    display_unit = att_3->display_unit;
    format = att_3->format;
    min_value = att_3->min_value;
    max_value = att_3->max_value;
    min_alarm = att_3->att_alarm.min_alarm;
    max_alarm = att_3->att_alarm.max_alarm;
    writable_attr_name = att_3->writable_attr_name;
    extensions.resize(att_3->sys_extensions.length());
    for(unsigned int j = 0; j < att_3->sys_extensions.length(); j++)
    {
        extensions[j] = att_3->sys_extensions[j];
    }
    disp_level = att_3->level;

    alarms.min_alarm = att_3->att_alarm.min_alarm;
    alarms.max_alarm = att_3->att_alarm.max_alarm;
    alarms.min_warning = att_3->att_alarm.min_warning;
    alarms.max_warning = att_3->att_alarm.max_warning;
    alarms.delta_t = att_3->att_alarm.delta_t;
    alarms.delta_val = att_3->att_alarm.delta_val;
    alarms.extensions.resize(att_3->att_alarm.extensions.length());
    for(unsigned int j = 0; j < att_3->att_alarm.extensions.length(); j++)
    {
        alarms.extensions[j] = att_3->att_alarm.extensions[j];
    }

    events.ch_event.abs_change = att_3->event_prop.ch_event.abs_change;
    events.ch_event.rel_change = att_3->event_prop.ch_event.rel_change;
    events.ch_event.extensions.resize(att_3->event_prop.ch_event.extensions.length());
    for(unsigned int j = 0; j < att_3->event_prop.ch_event.extensions.length(); j++)
    {
        events.ch_event.extensions[j] = att_3->event_prop.ch_event.extensions[j];
    }

    events.per_event.period = att_3->event_prop.per_event.period;
    events.per_event.extensions.resize(att_3->event_prop.per_event.extensions.length());
    for(unsigned int j = 0; j < att_3->event_prop.per_event.extensions.length(); j++)
    {
        events.per_event.extensions[j] = att_3->event_prop.per_event.extensions[j];
    }

    events.arch_event.archive_abs_change = att_3->event_prop.arch_event.abs_change;
    events.arch_event.archive_rel_change = att_3->event_prop.arch_event.rel_change;
    events.arch_event.archive_period = att_3->event_prop.arch_event.period;
    events.arch_event.extensions.resize(att_3->event_prop.arch_event.extensions.length());
    for(unsigned int j = 0; j < att_3->event_prop.arch_event.extensions.length(); j++)
    {
        events.arch_event.extensions[j] = att_3->event_prop.arch_event.extensions[j];
    }

    return *this;
}

//+---------------------------------------------------------------------------------------------------------------
//
// function :
//         operator overloading :     =
//
// description :
//        Assignement operator for the AttributeInfoEx class from a AttributeConfig_5 pointer
//
//-----------------------------------------------------------------------------------------------------------------

AttributeInfoEx &AttributeInfoEx::operator=(const AttributeConfig_5 *att_5)
{
    name = att_5->name;
    writable = att_5->writable;
    data_format = att_5->data_format;
    data_type = att_5->data_type;
    max_dim_x = att_5->max_dim_x;
    max_dim_y = att_5->max_dim_y;
    description = att_5->description;
    label = att_5->label;
    unit = att_5->unit;
    standard_unit = att_5->standard_unit;
    display_unit = att_5->display_unit;
    format = att_5->format;
    min_value = att_5->min_value;
    max_value = att_5->max_value;
    min_alarm = att_5->att_alarm.min_alarm;
    max_alarm = att_5->att_alarm.max_alarm;
    writable_attr_name = att_5->writable_attr_name;
    extensions.resize(att_5->sys_extensions.length());
    for(unsigned int j = 0; j < att_5->sys_extensions.length(); j++)
    {
        extensions[j] = att_5->sys_extensions[j];
    }
    disp_level = att_5->level;
    root_attr_name = att_5->root_attr_name;
    if(!att_5->memorized)
    {
        memorized = NONE;
    }
    else
    {
        if(!att_5->mem_init)
        {
            memorized = MEMORIZED;
        }
        else
        {
            memorized = MEMORIZED_WRITE_INIT;
        }
    }
    enum_labels.clear();
    for(unsigned int j = 0; j < att_5->enum_labels.length(); j++)
    {
        enum_labels.emplace_back(att_5->enum_labels[j].in());
    }

    alarms.min_alarm = att_5->att_alarm.min_alarm;
    alarms.max_alarm = att_5->att_alarm.max_alarm;
    alarms.min_warning = att_5->att_alarm.min_warning;
    alarms.max_warning = att_5->att_alarm.max_warning;
    alarms.delta_t = att_5->att_alarm.delta_t;
    alarms.delta_val = att_5->att_alarm.delta_val;
    alarms.extensions.resize(att_5->att_alarm.extensions.length());
    for(unsigned int j = 0; j < att_5->att_alarm.extensions.length(); j++)
    {
        alarms.extensions[j] = att_5->att_alarm.extensions[j];
    }

    events.ch_event.abs_change = att_5->event_prop.ch_event.abs_change;
    events.ch_event.rel_change = att_5->event_prop.ch_event.rel_change;
    events.ch_event.extensions.resize(att_5->event_prop.ch_event.extensions.length());
    for(unsigned int j = 0; j < att_5->event_prop.ch_event.extensions.length(); j++)
    {
        events.ch_event.extensions[j] = att_5->event_prop.ch_event.extensions[j];
    }

    events.per_event.period = att_5->event_prop.per_event.period;
    events.per_event.extensions.resize(att_5->event_prop.per_event.extensions.length());
    for(unsigned int j = 0; j < att_5->event_prop.per_event.extensions.length(); j++)
    {
        events.per_event.extensions[j] = att_5->event_prop.per_event.extensions[j];
    }

    events.arch_event.archive_abs_change = att_5->event_prop.arch_event.abs_change;
    events.arch_event.archive_rel_change = att_5->event_prop.arch_event.rel_change;
    events.arch_event.archive_period = att_5->event_prop.arch_event.period;
    events.arch_event.extensions.resize(att_5->event_prop.arch_event.extensions.length());
    for(unsigned int j = 0; j < att_5->event_prop.arch_event.extensions.length(); j++)
    {
        events.arch_event.extensions[j] = att_5->event_prop.arch_event.extensions[j];
    }

    return *this;
}

//+----------------------------------------------------------------------------------------------------------------
//
// function :
//         operator overloading :     <<
//
// description :
//        Friend function to ease printing instance of the AttributeInfo class
//
//-----------------------------------------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &o_str, const AttributeInfoEx &p)
{
    //
    // Print all these properties
    //

    o_str << "Attribute name = " << p.name << std::endl;
    o_str << "Attribute data_type = " << (CmdArgType) p.data_type << std::endl;

    if(p.data_type == Tango::DEV_ENUM)
    {
        for(size_t loop = 0; loop < p.enum_labels.size(); loop++)
        {
            o_str << "\tEnumeration label = " << p.enum_labels[loop] << std::endl;
        }
    }

    o_str << "Attribute data_format = " << p.data_format;
    switch(p.data_format)
    {
    case Tango::SPECTRUM:
        o_str << ", max_dim_x = " << p.max_dim_x << std::endl;
        break;

    case Tango::IMAGE:
        o_str << ", max_dim_x = " << p.max_dim_x << ", max_dim_y = " << p.max_dim_y << std::endl;
        break;
    case Tango::SCALAR:
        // do nothing
        break;
    default:
        TANGO_ASSERT_ON_DEFAULT(p.data_format);
    }

    o_str << "Attribute writable type = " << p.writable << std::endl;

    if((p.writable == Tango::WRITE) || (p.writable == Tango::READ_WRITE))
    {
        switch(p.memorized)
        {
        case NOT_KNOWN:
            o_str << "Device/Appli too old to send/receive attribute memorisation data" << std::endl;
            break;

        case NONE:
            o_str << "Attribute is not memorized" << std::endl;
            break;

        case MEMORIZED:
            o_str << "Attribute is memorized" << std::endl;
            break;

        case MEMORIZED_WRITE_INIT:
            o_str << "Attribute is memorized and the memorized value is written at initialisation" << std::endl;
            break;

        default:
            TANGO_ASSERT_ON_DEFAULT(p.memorized);
        }
    }

    o_str << "Attribute display level = " << p.disp_level << std::endl;

    o_str << "Attribute writable_attr_name = " << p.writable_attr_name << std::endl;
    if(!p.root_attr_name.empty())
    {
        o_str << "Root attribute name = " << p.root_attr_name << std::endl;
    }
    o_str << "Attribute label = " << p.label << std::endl;
    o_str << "Attribute description = " << p.description << std::endl;
    o_str << "Attribute unit = " << p.unit;
    o_str << ", standard unit = " << p.standard_unit;
    o_str << ", display unit = " << p.display_unit << std::endl;
    o_str << "Attribute format = " << p.format << std::endl;
    o_str << "Attribute min value = " << p.min_value << std::endl;
    o_str << "Attribute max value = " << p.max_value << std::endl;

    unsigned int i;
    for(i = 0; i < p.extensions.size(); i++)
    {
        o_str << "Attribute extensions " << i + 1 << " = " << p.extensions[i] << std::endl;
    }

    o_str << "Attribute alarm : min alarm = ";
    p.alarms.min_alarm.empty() ? o_str << "Not specified" : o_str << p.alarms.min_alarm;
    o_str << std::endl;

    o_str << "Attribute alarm : max alarm = ";
    p.alarms.max_alarm.empty() ? o_str << "Not specified" : o_str << p.alarms.max_alarm;
    o_str << std::endl;

    o_str << "Attribute warning alarm : min warning = ";
    p.alarms.min_warning.empty() ? o_str << "Not specified" : o_str << p.alarms.min_warning;
    o_str << std::endl;

    o_str << "Attribute warning alarm : max warning = ";
    p.alarms.max_warning.empty() ? o_str << "Not specified" : o_str << p.alarms.max_warning;
    o_str << std::endl;

    o_str << "Attribute rds alarm : delta time = ";
    p.alarms.delta_t.empty() ? o_str << "Not specified" : o_str << p.alarms.delta_t;
    o_str << std::endl;

    o_str << "Attribute rds alarm : delta value = ";
    p.alarms.delta_val.empty() ? o_str << "Not specified" : o_str << p.alarms.delta_val;
    o_str << std::endl;

    for(i = 0; i < p.alarms.extensions.size(); i++)
    {
        o_str << "Attribute alarm extensions " << i + 1 << " = " << p.alarms.extensions[i] << std::endl;
    }

    o_str << "Attribute event : change event absolute change = ";
    p.events.ch_event.abs_change.empty() ? o_str << "Not specified" : o_str << p.events.ch_event.abs_change;
    o_str << std::endl;

    o_str << "Attribute event : change event relative change = ";
    p.events.ch_event.rel_change.empty() ? o_str << "Not specified" : o_str << p.events.ch_event.rel_change;
    o_str << std::endl;

    for(i = 0; i < p.events.ch_event.extensions.size(); i++)
    {
        o_str << "Attribute alarm : change event extensions " << i + 1 << " = " << p.events.ch_event.extensions[i]
              << std::endl;
    }

    o_str << "Attribute event : periodic event period = ";
    p.events.per_event.period.empty() ? o_str << "Not specified" : o_str << p.events.per_event.period;
    o_str << std::endl;

    for(i = 0; i < p.events.per_event.extensions.size(); i++)
    {
        o_str << "Attribute alarm : periodic event extensions " << i + 1 << " = " << p.events.per_event.extensions[i]
              << std::endl;
    }

    o_str << "Attribute event : archive event absolute change = ";
    p.events.arch_event.archive_abs_change.empty() ? o_str << "Not specified"
                                                   : o_str << p.events.arch_event.archive_abs_change;
    o_str << std::endl;

    o_str << "Attribute event : archive event relative change = ";
    p.events.arch_event.archive_rel_change.empty() ? o_str << "Not specified"
                                                   : o_str << p.events.arch_event.archive_rel_change;
    o_str << std::endl;

    o_str << "Attribute event : archive event period = ";
    p.events.arch_event.archive_period.empty() ? o_str << "Not specified" : o_str << p.events.arch_event.archive_period;
    o_str << std::endl;

    for(i = 0; i < p.events.arch_event.extensions.size(); i++)
    {
        if(i == 0)
        {
            o_str << std::endl;
        }
        o_str << "Attribute alarm : archive event extensions " << i + 1 << " = " << p.events.arch_event.extensions[i];
        if(i != p.events.arch_event.extensions.size() - 1)
        {
            o_str << std::endl;
        }
    }

    return o_str;
}

//+----------------------------------------------------------------------------------------------------------------
//
// function :
//         operator overloading :     <<
//
// description :
//        Friend function to ease printing instance of the PipeInfo class
//
//-----------------------------------------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &o_str, const PipeInfo &p)
{
    //
    // Print all these properties
    //

    o_str << "Pipe name = " << p.name << std::endl;
    o_str << "Pipe label = " << p.label << std::endl;
    o_str << "Pipe description = " << p.description << std::endl;

    o_str << "Pipe writable type = ";
    if(p.writable == PIPE_READ)
    {
        o_str << "READ" << std::endl;
    }
    else
    {
        o_str << "READ_WRITE" << std::endl;
    }

    o_str << "Pipe display level = " << p.disp_level << std::endl;

    unsigned int i;
    for(i = 0; i < p.extensions.size(); i++)
    {
        if(i == 0)
        {
            o_str << std::endl;
        }
        o_str << "Pipe extensions " << i + 1 << " = " << p.extensions[i] << std::endl;
    }

    return o_str;
}

} // namespace Tango
