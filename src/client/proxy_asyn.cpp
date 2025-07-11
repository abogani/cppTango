//====================================================================================
//
// cpp     - C++ source code file for TANGO api Connection and DeviceProxy
//      classes (asynchronous polling related methods)
//
// programmer     - Emmanuel Taurel (taurel@esrf.fr)
//
// original     - August 2002
//
// Copyright (C) :      2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
//
//===================================================================================

#include <tango/client/Database.h>
#include <tango/internal/telemetry/telemetry_kernel_macros.h>

#define _dyn_attr

namespace Tango
{

//-----------------------------------------------------------------------------
//
// method :         Connection::command_inout_asyn()
//
// description :     Send a command to a Tango device asynchronously.
//            The client is not blocked until the command is executed
//
// argin(s) :        command : The command name
//            data_in : Command input data
//            faf : Fire And Forget flag
//
// return :        The asynchronous call identifier
//
//-----------------------------------------------------------------------------

long Connection::command_inout_asynch(const std::string &command, const DeviceData &data_in, bool faf)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}, {"tango.operation.argument", command}}));

    //
    // Reconnect to device in case it is needed
    //

    try
    {
        check_and_reconnect();
    }
    catch(Tango::ConnectionFailed &e)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to execute command_inout on device " << dev_name();
        desc << ", command " << command << std::ends;
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_CommandFailed, desc.str());
    }

    //
    // Throw exception if caller not allowed to do write action
    //

    if(access == ACCESS_READ)
    {
        int db_num;
        ApiUtil *au = ApiUtil::instance();

        if(get_from_env_var())
        {
            db_num = au->get_db_ind();
        }
        else
        {
            db_num = au->get_db_ind(get_db_host(), get_db_port_num());
        }

        std::vector<Database *> &v_d = au->get_db_vect();
        Database *db = v_d[db_num];

        //
        // If the command is not allowed, throw exception
        // Also throw exception if it was not possible to get the list
        // of allowed commands from the control access service
        //

        std::string d_name = dev_name();

        if(!db->is_command_allowed(d_name, command))
        {
            DevErrorList &e = db->get_access_except_errors();
            if(e.length() != 0)
            {
                DevFailed df(e);
                throw df;
            }

            TangoSys_OMemStream desc;
            desc << "Command_inout_asynch on device " << dev_name() << " for command ";
            desc << command << " is not authorized" << std::ends;

            TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
        }
    }

    //
    // Create the request object
    //

    CORBA::Request_ptr request;

    if(version >= 4)
    {
        request = device_4->_request("command_inout_4");
    }
    else if(version >= 2)
    {
        request = device_2->_request("command_inout_2");
    }
    else
    {
        request = device->_request("command_inout");
    }

    request->add_in_arg() <<= command.c_str();
    request->add_in_arg() <<= data_in.any.in();

    if(version >= 4)
    {
        request->add_in_arg() <<= source;
        request->add_in_arg() <<= get_client_identification();
    }
    else if(version >= 2)
    {
        request->add_in_arg() <<= source;
    }

    request->set_return_type(CORBA::_tc_any);
    request->exceptions()->add(Tango::_tc_DevFailed);

    //
    // If its a fire and forget (faf) call, send it and forget. Otherwise, send it
    // and store the request in the global asynchronous polling requests table
    //

    long id = 0;

    if(!faf)
    {
        id = add_asyn_request(request, TgRequest::CMD_INOUT);
        request->send_deferred();
    }
    else
    {
        request->send_oneway();
        CORBA::release(request);
    }

    return id;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         Connection::command_inout_asyn()
//
// description :     Send a command to a Tango device asynchrnously.
//            The client is not blocked until the command is executed
//            These methods are only user helper methods which call
//            the official command_inout_asyn() method
//
// argin(s) :        command : The command name
//            data_in : Command input data
//            faf : Fire And Forget flag
//
// return :        The asynchronous call identifier
//
//-----------------------------------------------------------------------------

long Connection::command_inout_asynch(const std::string &command, bool faf)
{
    DeviceData data_in;
    return command_inout_asynch(command, data_in, faf);
}

//-----------------------------------------------------------------------------
//
// method :         Connection::command_inout_reply()
//
// description :     Try to obtain data returned by a command asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. An exception is thrown in this case.
//
// argin(s) :        id : The asynchrnous call identifier
//
// return :        The command output data
//
//-----------------------------------------------------------------------------

DeviceData Connection::command_inout_reply(long id)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    DeviceData data_out;
    //
    // Retrieve request object
    //

    TgRequest &req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if(req.req_type != TgRequest::CMD_INOUT)
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Reply arrived ? Throw exception if not yet arrived
    //

    if(!req.request->poll_response())
    {
        TangoSys_OMemStream desc;
        desc << "Device " << dev_name();
        desc << ": Reply for asynchronous call (id = " << id;
        desc << ") is not yet arrived" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
    }

    //
    // Check if the reply is an exception
    // Due to a compatibility pb between omniORB 4.1 and omniORB 4.1 (at least 4.2.0),
    // we have to also handle CORBA::Request instance throwing exception in its env() and
    // other methods. This was not the case in omniORB 4.1!
    //

    CORBA::Environment_ptr env = nullptr;
    try
    {
        env = req.request->env();
    }
    catch(CORBA::TRANSIENT &tra)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
        if(tra.minor() == omni::TRANSIENT_CallTimedout)
        {
            omni420_timeout(id, cb_excep_mess);
        }
        else
        {
            set_connection_state(CONNECTION_NOTOK);
            return omni420_except(id, cb_excep_mess, req);
        }
    }
    catch(CORBA::SystemException &ex)
    {
        set_connection_state(CONNECTION_NOTOK);
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
        return omni420_except(id, cb_excep_mess, req);
    }

    if(!CORBA::is_nil(env) && (env->exception() == nullptr))
    {
        //
        // Get received value
        //

        const CORBA::Any *received = nullptr;
        CORBA::Any &dii_any = req.request->return_value();
        dii_any >>= received;
        CORBA::Any *server_any = new CORBA::Any(*received);

        data_out.any = server_any;
    }
    else
    {
        //
        // Retrieve exception and re-throw it.
        //

        CORBA::Exception *ex_ptr = env->exception();

        //
        // Special treatment for timeout exception (TRANSIENT with specific minor code)
        //

        CORBA::TRANSIENT *tra;
        if((tra = CORBA::TRANSIENT::_downcast(ex_ptr)) != nullptr)
        {
            if(tra->minor() == omni::TRANSIENT_CallTimedout)
            {
                bool need_reconnect = false;
                if(ext->has_alt_adr)
                {
                    try
                    {
                        Device_var dev = Device::_duplicate(device);
                        dev->ping();
                    }
                    catch(CORBA::TRANSIENT &trans_ping)
                    {
                        if(trans_ping.minor() == omni::TRANSIENT_ConnectFailed ||
                           trans_ping.minor() == omni::TRANSIENT_CallTimedout)
                        {
                            need_reconnect = true;
                        }
                    }
                    catch(...)
                    {
                    }
                }

                char cb_excep_mess[256];
                Tango::Except::print_CORBA_SystemException_r(tra, cb_excep_mess);

                if(!need_reconnect)
                {
                    CORBA::NVList_ptr req_arg = req.request->arguments();
                    const char *cmd = nullptr;
                    CORBA::NamedValue_ptr nv = req_arg->item(0);
                    *(nv->value()) >>= cmd;
                    char *tmp = Tango::string_dup(cmd);

                    TangoSys_OMemStream desc;
                    desc << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
                    desc << ", command " << tmp << std::ends;
                    Tango::string_free(tmp);

                    remove_asyn_request(id);

                    TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_DeviceTimedOut, desc.str());
                }
                else
                {
                    set_connection_state(CONNECTION_NOTOK);
                    remove_asyn_request(id);

                    std::stringstream ss;
                    ss << "Failed to execute command_inout_asynch on device " << dev_name();

                    TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
                }
            }
        }

        CORBA::UnknownUserException *unk_ex;
        if((unk_ex = CORBA::UnknownUserException::_downcast(ex_ptr)) != nullptr)
        {
            //
            // It is a UserUnknownException exception. This means that the
            // server has sent a DevFailed exception
            //

            const Tango::DevFailed *serv_ex;
            unk_ex->exception() >>= serv_ex;
            Tango::DevFailed ex(*serv_ex);

            CORBA::NVList_ptr req_arg = req.request->arguments();
            const char *cmd = nullptr;
            CORBA::NamedValue_ptr nv = req_arg->item(0);
            *(nv->value()) >>= cmd;
            char *tmp = Tango::string_dup(cmd);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout_asynch on device " << dev_name();
            desc << ", command " << tmp << std::ends;
            Tango::string_free(tmp);

            remove_asyn_request(id);

            TANGO_RETHROW_EXCEPTION(ex, API_CommandFailed, desc.str());
        }

        CORBA::SystemException *sys_ex;
        if((sys_ex = CORBA::SystemException::_downcast(ex_ptr)) != nullptr)
        {
            set_connection_state(CONNECTION_NOTOK);

            //
            // Re-throw nearly all CORBA system exceptions
            //

            CORBA::NVList_ptr req_arg = req.request->arguments();
            const char *cmd = nullptr;
            CORBA::NamedValue_ptr nv = req_arg->item(0);
            *(nv->value()) >>= cmd;
            char *tmp = Tango::string_dup(cmd);

            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(sys_ex, cb_excep_mess);

            //
            // Check if the exception was a connection exception
            // If so, execute the command synchronously (tries to reconnect)
            // If successful just return, otherwise throw the first exception
            //

            std::string ex(cb_excep_mess);
            std::string::size_type pos_con = ex.find("TRANSIENT_ConnectFailed");
            std::string::size_type pos_one = ex.find("EXIST_NoMatch");
            if(pos_con != std::string::npos || pos_one != std::string::npos)
            {
                try
                {
                    DeviceData dd_out = redo_synch_cmd(req);

                    //
                    // Remove request from request global table.
                    //

                    Tango::string_free(tmp);

                    remove_asyn_request(id);

                    return dd_out;
                }
                catch(Tango::DevFailed &)
                {
                }
            }

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout_asynch on device " << dev_name();
            desc << ", command " << tmp << std::ends;
            Tango::string_free(tmp);

            remove_asyn_request(id);

            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
        }
    }

    //
    // Remove request from request global table.
    //

    remove_asyn_request(id);

    return data_out;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         Connection::command_inout_reply()
//
// description :     Try to obtain data returned by a command asynchronously
//            requested. This method blocks if the reply is
//            not yet arrived for a max of timeout mS.
//            An exception is thrown in case of reply not yet arrived.
//
// argin(s) :        call_timeout : The max time to block the calling process
//            if timeout is 0, this means wait until the reply is
//            there
//            id : The asynchrnous call identifier
//
// return :        The command output data
//
//-----------------------------------------------------------------------------

DeviceData Connection::command_inout_reply(long id, long call_timeout)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Retrieve request object
    //

    TgRequest &req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if(req.req_type != TgRequest::CMD_INOUT)
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Use CORBA get_response call if the call timeout is specified as 0
    // (If the response is not already there).
    // Otherwise, uses a loop. Switch process to sleeping state for 20 ms
    // between each test to check if the replies has arrived.
    //

    if(call_timeout == 0)
    {
        if(!req.request->poll_response())
        {
            try
            {
                req.request->get_response();
            }
            catch(...)
            {
            }
        }
    }
    else
    {
        long nb = call_timeout / 20;
        int i;

        for(i = 0; i < nb; i++)
        {
            if(req.request->poll_response())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if(i == nb)
        {
            if(!req.request->poll_response())
            {
                TangoSys_OMemStream desc;
                desc << "Device " << dev_name();
                desc << ": Reply for asynchronous call (id = " << id;
                desc << ") is not yet arrived" << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
            }
        }
    }

    //
    // Check if the reply is an exception
    // Due to a compatibility pb between omniORB 4.1 and omniORB 4.1 (at least 4.2.0),
    // we have to also handle CORBA::Request instance throwing exception in its env() and
    // other methods. This was not the case in omniORB 4.1!
    //

    DeviceData data_out;
    CORBA::Environment_ptr env = nullptr;

    try
    {
        env = req.request->env();
    }
    catch(CORBA::TRANSIENT &tra)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
        if(tra.minor() == omni::TRANSIENT_CallTimedout)
        {
            omni420_timeout(id, cb_excep_mess);
        }
        else
        {
            set_connection_state(CONNECTION_NOTOK);
            return omni420_except(id, cb_excep_mess, req);
        }
    }
    catch(CORBA::SystemException &ex)
    {
        set_connection_state(CONNECTION_NOTOK);
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
        return omni420_except(id, cb_excep_mess, req);
    }

    if(!CORBA::is_nil(env) && (env->exception() == nullptr))
    {
        //
        // It's not an exception, therefore get received value
        //

        const CORBA::Any *received;
        CORBA::Any &dii_any = req.request->return_value();
        dii_any >>= received;
        CORBA::Any *server_any = new CORBA::Any(*received);

        data_out.any = server_any;
    }
    else
    {
        //
        // Retrieve exception and re-throw it.
        //

        CORBA::Exception *ex_ptr = env->exception();

        //
        // Special treatement for timeout exception (TRANSIENT with specific minor code)
        //

        CORBA::TRANSIENT *tra;
        if((tra = CORBA::TRANSIENT::_downcast(ex_ptr)) != nullptr)
        {
            if(tra->minor() == omni::TRANSIENT_CallTimedout)
            {
                bool need_reconnect = false;
                if(ext->has_alt_adr)
                {
                    try
                    {
                        Device_var dev = Device::_duplicate(device);
                        dev->ping();
                    }
                    catch(CORBA::TRANSIENT &trans_ping)
                    {
                        if(trans_ping.minor() == omni::TRANSIENT_ConnectFailed ||
                           trans_ping.minor() == omni::TRANSIENT_CallTimedout)
                        {
                            need_reconnect = true;
                        }
                    }
                    catch(...)
                    {
                    }
                }

                char cb_excep_mess[256];
                Tango::Except::print_CORBA_SystemException_r(tra, cb_excep_mess);

                if(!need_reconnect)
                {
                    CORBA::NVList_ptr req_arg = req.request->arguments();
                    const char *cmd = nullptr;
                    CORBA::NamedValue_ptr nv = req_arg->item(0);
                    *(nv->value()) >>= cmd;
                    char *tmp = Tango::string_dup(cmd);

                    TangoSys_OMemStream desc;
                    desc << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
                    desc << ", command " << tmp << std::ends;
                    Tango::string_free(tmp);

                    remove_asyn_request(id);

                    TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_DeviceTimedOut, desc.str());
                }
                else
                {
                    set_connection_state(CONNECTION_NOTOK);
                    remove_asyn_request(id);

                    std::stringstream ss;
                    ss << "Failed to execute command_inout_asynch on device " << dev_name();

                    TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
                }
            }
        }

        CORBA::UnknownUserException *unk_ex;
        if((unk_ex = CORBA::UnknownUserException::_downcast(ex_ptr)) != nullptr)
        {
            //
            // It is a UserUnknownException exception. This means that the
            // server has sent a DevFailed exception
            //

            const Tango::DevFailed *serv_ex;
            unk_ex->exception() >>= serv_ex;
            Tango::DevFailed ex(*serv_ex);

            CORBA::NVList_ptr req_arg = req.request->arguments();
            const char *cmd = nullptr;
            CORBA::NamedValue_ptr nv = req_arg->item(0);
            *(nv->value()) >>= cmd;
            char *tmp = Tango::string_dup(cmd);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout_asynch on device " << dev_name();
            desc << ", command " << tmp << std::ends;
            Tango::string_free(tmp);

            remove_asyn_request(id);
            ;

            TANGO_RETHROW_EXCEPTION(ex, API_CommandFailed, desc.str());
        }

        CORBA::SystemException *sys_ex;
        if((sys_ex = CORBA::SystemException::_downcast(ex_ptr)) != nullptr)
        {
            set_connection_state(CONNECTION_NOTOK);

            //
            // Re-throw nearly all CORBA system exceptions
            //

            CORBA::NVList_ptr req_arg = req.request->arguments();
            const char *cmd = nullptr;
            CORBA::NamedValue_ptr nv = req_arg->item(0);
            *(nv->value()) >>= cmd;
            char *tmp = Tango::string_dup(cmd);

            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(sys_ex, cb_excep_mess);

            //
            // Check if the exception was a connection exception
            // If so, execute the command synchronously (tries to reconnect)
            // If successful just return, otherwise throw the first exception
            //

            std::string ex(cb_excep_mess);
            std::string::size_type pos_con = ex.find("TRANSIENT_ConnectFailed");
            std::string::size_type pos_one = ex.find("EXIST_NoMatch");
            if(pos_con != std::string::npos || pos_one != std::string::npos)
            {
                try
                {
                    DeviceData dd_out = redo_synch_cmd(req);

                    //
                    // Remove request from request global table.
                    //

                    Tango::string_free(tmp);

                    remove_asyn_request(id);

                    return dd_out;
                }
                catch(Tango::DevFailed &)
                {
                }
            }

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout_asynch on device " << dev_name();
            desc << ", command " << tmp << std::ends;
            Tango::string_free(tmp);

            remove_asyn_request(id);

            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
        }
    }

    //
    // Remove request from request global table.
    //

    remove_asyn_request(id);

    return data_out;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::read_attributes_asynch()
//
// description :     Read Tango device attributes asynchrnously.
//            The client is not blocked until the attributes are read
//
// argin(s) :        attr_names : The attribute name(s)
//
// return :        The asynchronous call identifier
//
//-----------------------------------------------------------------------------

long DeviceProxy::read_attributes_asynch(const std::vector<std::string> &attr_names)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Reconnect to device in case it is needed
    //

    try
    {
        check_and_reconnect();
    }
    catch(Tango::ConnectionFailed &e)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to execute read_attributes_asynch on device " << dev_name() << std::ends;
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_CommandFailed, desc.str());
    }

    //
    // Check that the caller did not give two times the same attribute
    //

    same_att_name(attr_names, "DeviceProxy::read_attributes_asynch");

    //
    // Create the request object
    //

    Tango::DevVarStringArray names;
    long nb_names = attr_names.size();
    names.length(nb_names);
    for(int i = 0; i < nb_names; i++)
    {
        names[i] = attr_names[i].c_str();
    }

    CORBA::Request_ptr request;
    if(version >= 5)
    {
        request = Connection::device_5->_request("read_attributes_5");
        request->add_in_arg() <<= names;
        request->add_in_arg() <<= source;
        request->add_in_arg() <<= get_client_identification();
        request->set_return_type(Tango::_tc_AttributeValueList_5);
    }
    else if(version == 4)
    {
        request = Connection::device_4->_request("read_attributes_4");
        request->add_in_arg() <<= names;
        request->add_in_arg() <<= source;
        request->add_in_arg() <<= get_client_identification();
        request->set_return_type(Tango::_tc_AttributeValueList_4);
    }
    else if(version == 3)
    {
        request = Connection::device_3->_request("read_attributes_3");
        request->add_in_arg() <<= names;
        request->add_in_arg() <<= source;
        request->set_return_type(Tango::_tc_AttributeValueList_3);
    }
    else if(version == 2)
    {
        request = device_2->_request("read_attributes_2");
        request->add_in_arg() <<= names;
        request->add_in_arg() <<= source;
        request->set_return_type(Tango::_tc_AttributeValueList);
    }
    else
    {
        request = device->_request("read_attributes");
        request->add_in_arg() <<= names;
        request->set_return_type(Tango::_tc_AttributeValueList);
    }
    request->exceptions()->add(Tango::_tc_DevFailed);

    //
    // Send it and store the request in the global asynchronous polling requests
    // table
    //

    long id = 0;

    id = add_asyn_request(request, TgRequest::READ_ATTR);
    request->send_deferred();

    return id;

    TANGO_TELEMETRY_TRACE_END();
}

long DeviceProxy::read_attribute_asynch(const std::string &name)
{
    std::vector<std::string> tmp_names(1, name);
    return read_attributes_asynch(tmp_names);
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::read_attributes_reply()
//
// description :     Try to obtain data returned by attributes asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. An exception is thrown in this case.
//
// argin(s) :        id : The asynchrnous call identifier
//
// return :        The attributes data
//
//-----------------------------------------------------------------------------

std::vector<DeviceAttribute> *DeviceProxy::read_attributes_reply(long id)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Retrieve request object
    //

    TgRequest &req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if(req.req_type != TgRequest::READ_ATTR)
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Reply arrived ? Throw exception is not yet arrived
    //

    if(!req.request->poll_response())
    {
        TangoSys_OMemStream desc;
        desc << "Device " << dev_name();
        desc << ": Reply for asynchronous call (id = " << id;
        desc << ") is not yet arrived" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
    }
    else
    {
        //
        // Check if the reply is an exception
        // Due to a compatibility pb between omniORB 4.1 and omniORB 4.1 (at least 4.2.0),
        // we have to also handle CORBA::Request instance throwing exception in its env() and
        // other methods. This was not the case in omniORB 4.1!
        //

        CORBA::Environment_ptr env = nullptr;
        try
        {
            env = req.request->env();
        }
        catch(CORBA::TRANSIENT &tra)
        {
            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
            if(tra.minor() == omni::TRANSIENT_CallTimedout)
            {
                omni420_timeout_attr(id, cb_excep_mess, MULTIPLE);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                omni420_except_attr(id, cb_excep_mess, MULTIPLE);

                std::vector<DeviceAttribute> *a_ptr = redo_synch_reads_call(req);
                remove_asyn_request(id);
                return a_ptr;
            }
        }
        catch(CORBA::SystemException &ex)
        {
            set_connection_state(CONNECTION_NOTOK);
            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
            omni420_except_attr(id, cb_excep_mess, MULTIPLE);

            std::vector<DeviceAttribute> *a_ptr = redo_synch_reads_call(req);
            remove_asyn_request(id);
            return a_ptr;
        }

        if(!CORBA::is_nil(env) && (env->exception() != nullptr))
        {
            read_attr_except(req.request, id, MULTIPLE);

            //
            // If we arrive here, this means the exception was there due to a server
            // shutdown but it is now back into operation
            // Try to redo the call synchronously
            //

            std::vector<DeviceAttribute> *a_ptr;
            a_ptr = redo_synch_reads_call(req);

            //
            // Remove request from request global table.
            //

            remove_asyn_request(id);

            return a_ptr;
        }

        auto *dev_attr = new(std::vector<DeviceAttribute>);

        //
        // Get received value
        //

        const Tango::AttributeValueList *received;
        const Tango::AttributeValueList_3 *received_3;
        const Tango::AttributeValueList_4 *received_4;
        const Tango::AttributeValueList_5 *received_5;

        CORBA::Any &dii_any = req.request->return_value();
        unsigned long nb_received;

        switch(version)
        {
        case 6:
        case 5:
            dii_any >>= received_5;
            nb_received = received_5->length();
            break;

        case 4:
            dii_any >>= received_4;
            nb_received = received_4->length();
            break;

        case 3:
            dii_any >>= received_3;
            nb_received = received_3->length();
            break;

        default:
            dii_any >>= received;
            nb_received = received->length();
            break;
        }

        dev_attr->resize(nb_received);

        for(unsigned long i = 0; i < nb_received; i++)
        {
            if(version >= 3)
            {
                if(version >= 5)
                {
                    ApiUtil::attr_to_device(&((*received_5)[i]), version, &((*dev_attr)[i]));
                }
                else if(version == 4)
                {
                    ApiUtil::attr_to_device(&((*received_4)[i]), version, &((*dev_attr)[i]));
                }
                else
                {
                    ApiUtil::attr_to_device(nullptr, &((*received_3)[i]), version, &((*dev_attr)[i]));
                }

                //
                // Add an error in the error stack in case there is one
                //

                DevErrorList_var &err_list = (*dev_attr)[i].get_error_list();
                long nb_except = err_list.in().length();
                if(nb_except != 0)
                {
                    TangoSys_OMemStream desc;
                    desc << "Failed to read_attributes on device " << device_name;
                    desc << ", attribute " << (*dev_attr)[i].name << std::ends;

                    err_list.inout().length(nb_except + 1);
                    err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
                    err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

                    std::string st = desc.str();
                    err_list[nb_except].desc = Tango::string_dup(st.c_str());
                    err_list[nb_except].severity = Tango::ERR;
                }
            }
            else
            {
                ApiUtil::attr_to_device(&((*received)[i]), nullptr, version, &((*dev_attr)[i]));
            }
        }

        //
        // Remove request from request global table.
        //

        remove_asyn_request(id);

        return dev_attr;
    }

    return nullptr;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::read_attribute_reply()
//
// description :     Try to obtain data returned by an attribute asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. An exception is thrown in this case.
//
// argin(s) :        id : The asynchronous call identifier
//
// return :        The attribute data
//
//-----------------------------------------------------------------------------

DeviceAttribute *DeviceProxy::read_attribute_reply(long id)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Retrieve request object
    //

    TgRequest &req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if(req.req_type != TgRequest::READ_ATTR)
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Reply arrived ? Throw exception is not yet arrived
    //

    if(!req.request->poll_response())
    {
        TangoSys_OMemStream desc;
        desc << "Device " << dev_name();
        desc << ": Reply for asynchronous call (id = " << id;
        desc << ") is not yet arrived" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
    }
    else
    {
        //
        // Check if the reply is an exception
        // Due to a compatibility pb between omniORB 4.1 and omniORB 4.1 (at least 4.2.0),
        // we have to also handle CORBA::Request instance throwing exception in its env() and
        // other methods. This was not the case in omniORB 4.1!
        //

        CORBA::Environment_ptr env = nullptr;
        try
        {
            env = req.request->env();
        }
        catch(CORBA::TRANSIENT &tra)
        {
            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
            if(tra.minor() == omni::TRANSIENT_CallTimedout)
            {
                omni420_timeout_attr(id, cb_excep_mess, SIMPLE);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                omni420_except_attr(id, cb_excep_mess, SIMPLE);

                DeviceAttribute *a_ptr = redo_synch_read_call(req);
                remove_asyn_request(id);
                return a_ptr;
            }
        }
        catch(CORBA::SystemException &ex)
        {
            set_connection_state(CONNECTION_NOTOK);
            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
            omni420_except_attr(id, cb_excep_mess, SIMPLE);

            DeviceAttribute *a_ptr = redo_synch_read_call(req);
            remove_asyn_request(id);
            return a_ptr;
        }

        if(!CORBA::is_nil(env) && (env->exception() != nullptr))
        {
            read_attr_except(req.request, id, SIMPLE);

            //
            // If we arrive here, this means the exception was there due to a server
            // shutdown but it is now back into operation
            // Try to redo the call synchronously
            //

            DeviceAttribute *a_ptr;
            a_ptr = redo_synch_read_call(req);

            //
            // Remove request from request global table.
            //

            remove_asyn_request(id);

            return a_ptr;
        }

        DeviceAttribute *dev_attr = new DeviceAttribute;

        //
        // Get received value
        //

        const Tango::AttributeValueList *received;
        const Tango::AttributeValueList_3 *received_3;
        const Tango::AttributeValueList_4 *received_4;
        const Tango::AttributeValueList_5 *received_5;

        CORBA::Any &dii_any = req.request->return_value();

        switch(version)
        {
        case 6:
        case 5:
            dii_any >>= received_5;
            break;

        case 4:
            dii_any >>= received_4;
            break;

        case 3:
            dii_any >>= received_3;
            break;

        default:
            dii_any >>= received;
            break;
        }

        if(version >= 3)
        {
            if(version >= 5)
            {
                ApiUtil::attr_to_device(&((*received_5)[0]), version, dev_attr);
            }
            else if(version == 4)
            {
                ApiUtil::attr_to_device(&((*received_4)[0]), version, dev_attr);
            }
            else
            {
                ApiUtil::attr_to_device(nullptr, &((*received_3)[0]), version, dev_attr);
            }

            //
            // Add an error in the error stack in case there is one
            //

            DevErrorList_var &err_list = dev_attr->get_error_list();
            long nb_except = err_list.in().length();
            if(nb_except != 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to read_attributes on device " << device_name;
                desc << ", attribute " << dev_attr->name << std::ends;

                err_list.inout().length(nb_except + 1);
                err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
                err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

                std::string st = desc.str();
                err_list[nb_except].desc = Tango::string_dup(st.c_str());

                err_list[nb_except].severity = Tango::ERR;
            }
        }
        else
        {
            ApiUtil::attr_to_device(&((*received)[0]), nullptr, version, dev_attr);
        }

        //
        // Remove request from request global table.
        //

        remove_asyn_request(id);

        return dev_attr;
    }
    return nullptr;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::read_attributes_reply()
//
// description :     Try to obtain data returned by a read attributes asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. An exception is thrown in this case.
//
// argin(s) :        call_timeout : The max time to block the calling process
//            if timeout is 0, this means wait until the reply is
//            there
//            id : The asynchrnous call identifier
//
// return :        The command output data
//
//-----------------------------------------------------------------------------

std::vector<DeviceAttribute> *DeviceProxy::read_attributes_reply(long id, long call_timeout)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Retrieve request object
    //

    TgRequest req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if(req.req_type != TgRequest::READ_ATTR)
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Use CORBA get_response call if the call timeout is specified as 0
    // (If the response is not already there).
    // Otherwise, uses a loop. Switch process to sleeping state for 20 ms
    // between each test to check if the replies has arrived.
    //

    if(call_timeout == 0)
    {
        if(!req.request->poll_response())
        {
            try
            {
                req.request->get_response();
            }
            catch(...)
            {
            }
        }
    }
    else
    {
        long nb = call_timeout / 20;
        int i;

        for(i = 0; i < nb; i++)
        {
            if(req.request->poll_response())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if(i == nb)
        {
            if(!req.request->poll_response())
            {
                TangoSys_OMemStream desc;
                desc << "Device " << device_name;
                desc << ": Reply for asynchronous call (id = " << id;
                desc << ") is not yet arrived" << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
            }
        }
    }

    //
    // Check if the reply is an exception
    // Due to a compatibility pb between omniORB 4.1 and omniORB 4.1 (at least 4.2.0),
    // we have to also handle CORBA::Request instance throwing exception in its env() and
    // other methods. This was not the case in omniORB 4.1!
    //

    CORBA::Environment_ptr env = nullptr;
    try
    {
        env = req.request->env();
    }
    catch(CORBA::TRANSIENT &tra)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
        if(tra.minor() == omni::TRANSIENT_CallTimedout)
        {
            omni420_timeout_attr(id, cb_excep_mess, MULTIPLE);
        }
        else
        {
            set_connection_state(CONNECTION_NOTOK);
            omni420_except_attr(id, cb_excep_mess, MULTIPLE);

            std::vector<DeviceAttribute> *a_ptr = redo_synch_reads_call(req);
            remove_asyn_request(id);
            return a_ptr;
        }
    }
    catch(CORBA::SystemException &ex)
    {
        set_connection_state(CONNECTION_NOTOK);
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
        omni420_except_attr(id, cb_excep_mess, MULTIPLE);

        std::vector<DeviceAttribute> *a_ptr = redo_synch_reads_call(req);
        remove_asyn_request(id);
        return a_ptr;
    }

    if(!CORBA::is_nil(env) && (env->exception() != nullptr))
    {
        read_attr_except(req.request, id, MULTIPLE);

        //
        // If we arrive here, this means the exception was there due to a server
        // shutdown but it is now back into operation
        // Try to redo the call synchronously
        //

        std::vector<DeviceAttribute> *a_ptr;
        a_ptr = redo_synch_reads_call(req);

        //
        // Remove request from request global table.
        //

        remove_asyn_request(id);

        return a_ptr;
    }

    auto *dev_attr = new(std::vector<DeviceAttribute>);

    //
    // Get received value
    //

    const Tango::AttributeValueList *received;
    const Tango::AttributeValueList_3 *received_3;
    const Tango::AttributeValueList_4 *received_4;
    const Tango::AttributeValueList_5 *received_5;
    unsigned long nb_received;

    CORBA::Any &dii_any = req.request->return_value();

    switch(version)
    {
    case 6:
    case 5:
        dii_any >>= received_5;
        nb_received = received_5->length();
        break;

    case 4:
        dii_any >>= received_4;
        nb_received = received_4->length();
        break;

    case 3:
        dii_any >>= received_3;
        nb_received = received_3->length();
        break;

    default:
        dii_any >>= received;
        nb_received = received->length();
        break;
    }

    dev_attr->resize(nb_received);

    for(unsigned long i = 0; i < nb_received; i++)
    {
        if(version >= 3)
        {
            if(version >= 5)
            {
                ApiUtil::attr_to_device(&((*received_5)[i]), version, &((*dev_attr)[i]));
            }
            else if(version == 4)
            {
                ApiUtil::attr_to_device(&((*received_4)[i]), version, &((*dev_attr)[i]));
            }
            else
            {
                ApiUtil::attr_to_device(nullptr, &((*received_3)[i]), version, &((*dev_attr)[i]));
            }

            //
            // Add an error in the error stack in case there is one
            //

            DevErrorList_var &err_list = (*dev_attr)[i].get_error_list();
            long nb_except = err_list.in().length();
            if(nb_except != 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to read_attributes on device " << device_name;
                desc << ", attribute " << (*dev_attr)[i].name << std::ends;

                err_list.inout().length(nb_except + 1);
                err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
                err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

                std::string st = desc.str();
                err_list[nb_except].desc = Tango::string_dup(st.c_str());
                err_list[nb_except].severity = Tango::ERR;
            }
        }
        else
        {
            ApiUtil::attr_to_device(&((*received)[i]), nullptr, version, &((*dev_attr)[i]));
        }
    }

    //
    // Remove request from request global table.
    //

    remove_asyn_request(id);

    return dev_attr;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::read_attribute_reply()
//
// description :     Try to obtain data returned by a read attribute asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. An exception is thrown in this case.
//            This call is for one attribute only
//
// argin(s) :        call_timeout : The max time to block the calling process
//            if timeout is 0, this means wait until the reply is
//            there
//            id : The asynchrnous call identifier
//
// return :        The command output data
//
//-----------------------------------------------------------------------------

DeviceAttribute *DeviceProxy::read_attribute_reply(long id, long call_timeout)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Retrieve request object
    //

    TgRequest &req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if(req.req_type != TgRequest::READ_ATTR)
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Use CORBA get_response call if the call timeout is specified as 0
    // (If the response is not already there).
    // Otherwise, uses a loop. Switch process to sleeping state for 20 ms
    // between each test to check if the replies has arrived.
    //

    if(call_timeout == 0)
    {
        if(!req.request->poll_response())
        {
            try
            {
                req.request->get_response();
            }
            catch(...)
            {
            }
        }
    }
    else
    {
        long nb = call_timeout / 20;
        int i;

        for(i = 0; i < nb; i++)
        {
            if(req.request->poll_response())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if(i == nb)
        {
            if(!req.request->poll_response())
            {
                TangoSys_OMemStream desc;
                desc << "Device " << device_name;
                desc << ": Reply for asynchronous call (id = " << id;
                desc << ") is not yet arrived" << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
            }
        }
    }

    //
    // Check if the reply is an exception
    // Due to a compatibility pb between omniORB 4.1 and omniORB 4.2 (at least 4.2.0),
    // we have to also handle CORBA::Request instance throwing exception in its env() and
    // other methods. This was not the case in omniORB 4.1!
    //

    CORBA::Environment_ptr env = nullptr;
    try
    {
        env = req.request->env();
    }
    catch(CORBA::TRANSIENT &tra)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
        if(tra.minor() == omni::TRANSIENT_CallTimedout)
        {
            omni420_timeout_attr(id, cb_excep_mess, SIMPLE);
        }
        else
        {
            set_connection_state(CONNECTION_NOTOK);
            omni420_except_attr(id, cb_excep_mess, SIMPLE);

            DeviceAttribute *a_ptr = redo_synch_read_call(req);
            remove_asyn_request(id);
            return a_ptr;
        }
    }
    catch(CORBA::SystemException &ex)
    {
        set_connection_state(CONNECTION_NOTOK);
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
        omni420_except_attr(id, cb_excep_mess, SIMPLE);

        DeviceAttribute *a_ptr = redo_synch_read_call(req);
        remove_asyn_request(id);
        return a_ptr;
    }

    if(!CORBA::is_nil(env) && (env->exception() != nullptr))
    {
        read_attr_except(req.request, id, SIMPLE);

        //
        // If we arrive here, this means the exception was there due to a server
        // shutdown but it is now back into operation
        // Try to redo the call synchronously
        //

        DeviceAttribute *a_ptr;
        a_ptr = redo_synch_read_call(req);

        //
        // Remove request from request global table.
        //

        remove_asyn_request(id);

        return a_ptr;
    }

    DeviceAttribute *dev_attr = new DeviceAttribute;

    //
    // Get received value
    //

    const Tango::AttributeValueList *received;
    const Tango::AttributeValueList_3 *received_3;
    const Tango::AttributeValueList_4 *received_4;
    const Tango::AttributeValueList_5 *received_5;

    CORBA::Any &dii_any = req.request->return_value();

    switch(version)
    {
    case 6:
    case 5:
        dii_any >>= received_5;
        break;

    case 4:
        dii_any >>= received_4;
        break;

    case 3:
        dii_any >>= received_3;
        break;

    default:
        dii_any >>= received;
        break;
    }

    if(version >= 3)
    {
        if(version >= 5)
        {
            ApiUtil::attr_to_device(&((*received_5)[0]), version, dev_attr);
        }
        else if(version == 4)
        {
            ApiUtil::attr_to_device(&((*received_4)[0]), version, dev_attr);
        }
        else
        {
            ApiUtil::attr_to_device(nullptr, &((*received_3)[0]), version, dev_attr);
        }

        //
        // Add an error in the error stack in case there is one
        //

        DevErrorList_var &err_list = dev_attr->get_error_list();
        long nb_except = err_list.in().length();
        if(nb_except != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attributes on device " << device_name;
            desc << ", attribute " << dev_attr->name << std::ends;

            err_list.inout().length(nb_except + 1);
            err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
            err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string st = desc.str();
            err_list[nb_except].desc = Tango::string_dup(st.c_str());
            err_list[nb_except].severity = Tango::ERR;
        }
    }
    else
    {
        ApiUtil::attr_to_device(&((*received)[0]), nullptr, version, dev_attr);
    }

    //
    // Remove request from request global table.
    //

    remove_asyn_request(id);

    return dev_attr;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::read_attr_except()
//
// description :     Analyse the exception returned by the DII call and
//            send a Tango::DevFailed exception to the caller
//
// argin(s) :        req : The CORBA request object
//            id : The asynchrnous call identifier
//            type : The request type
//
//-----------------------------------------------------------------------------

void DeviceProxy::read_attr_except(CORBA::Request_ptr req, long id, read_attr_type type)
{
    CORBA::Environment_ptr env = req->env();
    CORBA::Exception *ex_ptr = env->exception();

    //
    // Special treatement for timeout exception (TRANSIENT with specific minor code)
    //

    CORBA::TRANSIENT *tra;
    if((tra = CORBA::TRANSIENT::_downcast(ex_ptr)) != nullptr)
    {
        if(tra->minor() == omni::TRANSIENT_CallTimedout)
        {
            bool need_reconnect = false;
            if(ext->has_alt_adr)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(CORBA::TRANSIENT &trans_ping)
                {
                    if(trans_ping.minor() == omni::TRANSIENT_ConnectFailed ||
                       trans_ping.minor() == omni::TRANSIENT_CallTimedout)
                    {
                        need_reconnect = true;
                    }
                }
                catch(...)
                {
                }
            }

            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(tra, cb_excep_mess);

            std::string meth;
            if(type == SIMPLE)
            {
                meth = "DeviceProxy::read_attribute_replay()";
            }
            else
            {
                meth = "DeviceProxy::read_attributes_reply()";
            }

            if(!need_reconnect)
            {
                CORBA::NVList_ptr req_arg = req->arguments();
                const Tango::DevVarStringArray *names;
                CORBA::NamedValue_ptr nv = req_arg->item(0);
                *(nv->value()) >>= names;

                TangoSys_OMemStream desc;
                desc << "Timeout (" << timeout << " mS) exceeded on device " << device_name;
                desc << "\nAttribute(s): ";
                for(unsigned int i = 0; i < names->length(); i++)
                {
                    desc << (*names)[i];
                    if(i != (names->length() - 1))
                    {
                        desc << ", ";
                    }
                }
                desc << std::ends;

                remove_asyn_request(id);
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_DeviceTimedOut, desc.str());
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                remove_asyn_request(id);

                std::stringstream ss;
                ss << "Failed to execute read_attribute_asynch on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
            }
        }
    }

    CORBA::UnknownUserException *unk_ex;
    if((unk_ex = CORBA::UnknownUserException::_downcast(ex_ptr)) != nullptr)
    {
        //
        // It is a UserUnknownException exception. This means that the
        // server has sent a DevFailed exception
        //

        const Tango::DevFailed *serv_ex;
        unk_ex->exception() >>= serv_ex;
        Tango::DevFailed ex(*serv_ex);

        CORBA::NVList_ptr req_arg = req->arguments();
        const Tango::DevVarStringArray *names;
        CORBA::NamedValue_ptr nv = req_arg->item(0);
        *(nv->value()) >>= names;

        TangoSys_OMemStream desc;
        desc << "Failed to execute read_attribute_asynch on device " << device_name;
        desc << "\nAttribute(s): ";
        for(unsigned int i = 0; i < names->length(); i++)
        {
            desc << (*names)[i];
            if(i != (names->length() - 1))
            {
                desc << ", ";
            }
        }
        desc << std::ends;

        remove_asyn_request(id);

        if(type == SIMPLE)
        {
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
        else
        {
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
    }

    CORBA::SystemException *sys_ex;
    if((sys_ex = CORBA::SystemException::_downcast(ex_ptr)) != nullptr)
    {
        set_connection_state(CONNECTION_NOTOK);

        //
        // Re-throw nearly (but not all) all CORBA system exceptions
        //

        CORBA::NVList_ptr req_arg = req->arguments();
        const Tango::DevVarStringArray *names;
        CORBA::NamedValue_ptr nv = req_arg->item(0);
        *(nv->value()) >>= names;

        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(sys_ex, cb_excep_mess);

        //
        // Check if the exception was a connection exception
        // In this case, try to ping the device.
        // If successfull, just returns otherwise, throw the first exception
        //

        std::string ex(cb_excep_mess);
        std::string::size_type pos = ex.find("TRANSIENT_ConnectFailed");
        std::string::size_type pos_one = ex.find("EXIST_NoMatch");
        if(pos != std::string::npos || pos_one != std::string::npos)
        {
            try
            {
                ping();
                return;
            }
            catch(Tango::DevFailed &)
            {
            }
        }

        TangoSys_OMemStream desc;
        desc << "Failed to execute read_attributes_asynch on device " << device_name;
        desc << "\nAttribute(s): ";
        for(unsigned int i = 0; i < names->length(); i++)
        {
            desc << (*names)[i];
            if(i != (names->length() - 1))
            {
                desc << ", ";
            }
        }
        desc << std::ends;

        remove_asyn_request(id);

        if(type == SIMPLE)
        {
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
        }
        else
        {
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
        }
    }
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::write_attributes_asynch()
//
// description :     Write Tango device attributes asynchronously.
//            The client is not blocked until the attributes are written
//
// argin(s) :        attr_list : The attribute name(s) and value(s)
//
// return :        The asynchronous call identifier
//
//-----------------------------------------------------------------------------

long DeviceProxy::write_attributes_asynch(const std::vector<DeviceAttribute> &attr_list)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Reconnect to device in case it is needed
    //

    try
    {
        check_and_reconnect();
    }
    catch(Tango::ConnectionFailed &e)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to execute write_attributes_asynch on device " << dev_name() << std::ends;
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_CommandFailed, desc.str());
    }

    //
    // Throw exception if caller not allowed to write_attribute
    //

    if(access == ACCESS_READ)
    {
        TangoSys_OMemStream desc;
        desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

        TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
    }

    //
    // Create the request object
    //

    Tango::AttributeValueList att;
    Tango::AttributeValueList_4 att_4;

    long nb_attr = attr_list.size();
    if(version >= 4)
    {
        att_4.length(nb_attr);
    }
    else
    {
        att.length(nb_attr);
    }

    for(int i = 0; i < nb_attr; i++)
    {
        if(version >= 4)
        {
            ApiUtil::device_to_attr(attr_list[i], att_4[i]);
        }
        else
        {
            ApiUtil::device_to_attr(attr_list[i], att[i], device_name);
        }
    }

    CORBA::Request_ptr request;
    if(version >= 4)
    {
        request = device_4->_request("write_attributes_4");
        request->add_in_arg() <<= att_4;
        request->add_in_arg() <<= get_client_identification();
        request->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else if(version == 3)
    {
        request = device->_request("write_attributes_3");
        request->add_in_arg() <<= att;
        request->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else
    {
        request = device->_request("write_attributes");
        request->add_in_arg() <<= att;
    }
    request->exceptions()->add(Tango::_tc_DevFailed);

    //
    // Send the request
    //

    long id = 0;
    id = add_asyn_request(request, TgRequest::WRITE_ATTR);

    request->send_deferred();

    return id;

    TANGO_TELEMETRY_TRACE_END();
}

long DeviceProxy::write_attribute_asynch(const DeviceAttribute &attr)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}, {"tango.operation.argument", attr.name}}));

    //
    // Reconnect to device in case it is needed
    //

    try
    {
        check_and_reconnect();
    }
    catch(Tango::ConnectionFailed &e)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to execute write_attributes_asynch on device " << dev_name() << std::ends;
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_CommandFailed, desc.str());
    }

    //
    // Throw exception if caller not allowed to write_attribute
    //

    if(access == ACCESS_READ)
    {
        TangoSys_OMemStream desc;
        desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

        TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
    }

    //
    // Create the request object
    //

    Tango::AttributeValueList att;
    Tango::AttributeValueList_4 att_4;

    if(version < 4)
    {
        att.length(1);
        ApiUtil::device_to_attr(attr, att[0], device_name);
    }
    else
    {
        att_4.length(1);
        ApiUtil::device_to_attr(attr, att_4[0]);
    }

    CORBA::Request_ptr request;

    if(version >= 4)
    {
        request = device_4->_request("write_attributes_4");
        request->add_in_arg() <<= att_4;
        request->add_in_arg() <<= get_client_identification();
        request->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else if(version == 3)
    {
        request = device->_request("write_attributes_3");
        request->add_in_arg() <<= att;
        request->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else
    {
        request = device->_request("write_attributes");
        request->add_in_arg() <<= att;
    }
    request->exceptions()->add(Tango::_tc_DevFailed);

    //
    // Send the request
    //

    long id = 0;
    id = add_asyn_request(request, TgRequest::WRITE_ATTR_SINGLE);

    request->send_deferred();

    return id;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::write_attributes_reply()
//
// description :     Try to obtain data returned by a write attribute asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. An exception is thrown in this case.
//
// argin(s) :        call_timeout : The max time to block the calling process
//            if timeout is 0, this means wait until the reply is
//            there
//            id : The asynchrnous call identifier
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attributes_reply(long id, long call_timeout)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Retrieve request object
    //

    TgRequest &req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if((req.req_type == TgRequest::CMD_INOUT) || (req.req_type == TgRequest::READ_ATTR))
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Use CORBA get_response call if the call timeout is specified as 0
    // (If the response is not already there).
    // Otherwise, uses a loop. Switch process to sleeping state for 20 ms
    // between each test to check if the replies has arrived.
    //

    if(call_timeout == 0)
    {
        if(!req.request->poll_response())
        {
            try
            {
                req.request->get_response();
            }
            catch(...)
            {
            }
        }
    }
    else
    {
        long nb = call_timeout / 20;
        int i;

        for(i = 0; i < nb; i++)
        {
            if(req.request->poll_response())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if(i == nb)
        {
            if(!req.request->poll_response())
            {
                TangoSys_OMemStream desc;
                desc << "Device " << device_name;
                desc << ": Reply for asynchronous call (id = " << id;
                desc << ") is not yet arrived" << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
            }
        }
    }

    //
    // Check if the reply is an exception
    // Due to a compatibility pb between omniORB 4.1 and omniORB 4.1 (at least 4.2.0),
    // we have to also handle CORBA::Request instance throwing exception in its env() and
    // other methods. This was not the case in omniORB 4.1!
    //

    CORBA::Environment_ptr env = nullptr;
    try
    {
        env = req.request->env();
    }
    catch(CORBA::TRANSIENT &tra)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
        if(tra.minor() == omni::TRANSIENT_CallTimedout)
        {
            omni420_timeout_wattr(id, cb_excep_mess);
        }
        else
        {
            set_connection_state(CONNECTION_NOTOK);
            omni420_except_wattr(id, cb_excep_mess);
            redo_synch_write_call(req);
        }
    }
    catch(CORBA::SystemException &ex)
    {
        set_connection_state(CONNECTION_NOTOK);
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
        omni420_except_wattr(id, cb_excep_mess);
        redo_synch_write_call(req);
    }

    if(!CORBA::is_nil(env) && (env->exception() != nullptr))
    {
        write_attr_except(req.request, id, req.req_type);

        //
        // If we arrive here, this means the exception was there due to a server
        // shutdown but it is now back into operation
        // Try to redo the call synchronously
        //

        redo_synch_write_call(req);
    }

    //
    // Remove request from request global table.
    //

    remove_asyn_request(id);

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::write_attributes_reply()
//
// description :     Try to obtain data returned by a read attribute asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. An exception is thrown in this case.
//
// argin(s) :        id : The asynchrnous call identifier
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attributes_reply(long id)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Retrieve request object
    //

    TgRequest &req = ApiUtil::instance()->get_pasyn_table()->get_request(id);

    //
    // Check request type
    //

    if((req.req_type == TgRequest::CMD_INOUT) || (req.req_type == TgRequest::READ_ATTR))
    {
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynExcept, API_BadAsynReqType, "Incompatible request type");
    }

    //
    // Reply arrived ? Throw exception is not yet arrived
    //

    if(!req.request->poll_response())
    {
        TangoSys_OMemStream desc;
        desc << "Device " << dev_name();
        desc << ": Reply for asynchronous call (id = " << id;
        desc << ") is not yet arrived" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
    }
    else
    {
        //
        // Check if the reply is an exception
        // Due to a compatibility pb between omniORB 4.1 and omniORB 4.1 (at least 4.2.0),
        // we have to also handle CORBA::Request instance throwing exception in its env() and
        // other methods. This was not the case in omniORB 4.1!
        //

        CORBA::Environment_ptr env = nullptr;
        try
        {
            env = req.request->env();
        }
        catch(CORBA::TRANSIENT &tra)
        {
            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(&tra, cb_excep_mess);
            if(tra.minor() == omni::TRANSIENT_CallTimedout)
            {
                omni420_timeout_wattr(id, cb_excep_mess);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                omni420_except_wattr(id, cb_excep_mess);
                redo_synch_write_call(req);
            }
        }
        catch(CORBA::SystemException &ex)
        {
            set_connection_state(CONNECTION_NOTOK);
            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(&ex, cb_excep_mess);
            omni420_except_wattr(id, cb_excep_mess);
            redo_synch_write_call(req);
        }

        if(!CORBA::is_nil(env) && (env->exception() != nullptr))
        {
            write_attr_except(req.request, id, req.req_type);

            //
            // If we arrive here, this means the exception was there due to a server
            // shutdown but it is now back into operation
            // Try to redo the call synchronously
            //

            redo_synch_write_call(req);
        }
    }

    //
    // Remove request from request global table.
    //

    remove_asyn_request(id);

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::write_attr_except()
//
// description :     Analyse the exception returned by the DII call and
//            send a Tango::DevFailed exception to the caller
//
// argin(s) :        req : The CORBA request object
//            id : The asynchrnous call identifier
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attr_except(CORBA::Request_ptr req, long id, TgRequest::ReqType req_type)
{
    CORBA::Environment_ptr env = req->env();
    CORBA::Exception *ex_ptr = env->exception();

    //
    // Special treatement for timeout exception (TRANSIENT with specific minor code)
    //

    CORBA::TRANSIENT *tra;
    if((tra = CORBA::TRANSIENT::_downcast(ex_ptr)) != nullptr)
    {
        if(tra->minor() == omni::TRANSIENT_CallTimedout)
        {
            bool need_reconnect = false;
            if(ext->has_alt_adr)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(CORBA::TRANSIENT &trans_ping)
                {
                    if(trans_ping.minor() == omni::TRANSIENT_ConnectFailed ||
                       trans_ping.minor() == omni::TRANSIENT_CallTimedout)
                    {
                        need_reconnect = true;
                    }
                }
                catch(...)
                {
                }
            }

            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(tra, cb_excep_mess);

            if(!need_reconnect)
            {
                CORBA::NVList_ptr req_arg = req->arguments();
                const Tango::AttributeValueList *att;
                const Tango::AttributeValueList_4 *att_4;
                unsigned int nb_att = 0;
                CORBA::NamedValue_ptr nv = req_arg->item(0);

                if(version < 4)
                {
                    if(*(nv->value()) >>= att)
                    {
                        nb_att = att->length();
                    }
                }
                else
                {
                    if(*(nv->value()) >>= att_4)
                    {
                        nb_att = att_4->length();
                    }
                }

                TangoSys_OMemStream desc;
                desc << "Timeout (" << timeout << " mS) exceeded on device " << device_name;
                if(nb_att != 0)
                {
                    desc << "\nAttribute(s): ";
                    for(unsigned int i = 0; i < nb_att; i++)
                    {
                        (version < 4) ? desc << (*att)[i].name : desc << (*att_4)[i].name;

                        if(i != (nb_att - 1))
                        {
                            desc << ", ";
                        }
                    }
                    desc << std::ends;
                }

                remove_asyn_request(id);
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_DeviceTimedOut, desc.str());
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                remove_asyn_request(id);

                std::stringstream ss;
                ss << "Failed to execute write_attribute_asynch on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
            }
        }
    }

    CORBA::UnknownUserException *unk_ex;
    if((unk_ex = CORBA::UnknownUserException::_downcast(ex_ptr)) != nullptr)
    {
        //
        // It is a UserUnknownException exception. This means that the
        // server has sent a DevFailed exception
        //

        const Tango::DevFailed *serv_ex = nullptr;
        Tango::DevFailed ex;
        const Tango::MultiDevFailed *multi_serv_ex = nullptr;
        Tango::MultiDevFailed m_ex;

        if(version < 3)
        {
            unk_ex->exception() >>= serv_ex;
            ex = *serv_ex;
        }
        else
        {
            if(unk_ex->exception() >>= multi_serv_ex)
            {
                m_ex = *multi_serv_ex;
            }
            else
            {
                unk_ex->exception() >>= serv_ex;
                ex = *serv_ex;
            }
        }

        CORBA::NVList_ptr req_arg = req->arguments();
        const Tango::AttributeValueList *att;
        const Tango::AttributeValueList_4 *att_4;
        unsigned int nb_att = 0;
        CORBA::NamedValue_ptr nv = req_arg->item(0);

        if(version < 4)
        {
            if(*(nv->value()) >>= att)
            {
                nb_att = att->length();
            }
        }
        else
        {
            if(*(nv->value()) >>= att_4)
            {
                nb_att = att_4->length();
            }
        }

        TangoSys_OMemStream desc;
        desc << "Failed to execute write_attributes_asynch on device " << device_name;
        if(nb_att != 0)
        {
            desc << "\nAttribute(s): ";
            for(unsigned int i = 0; i < nb_att; i++)
            {
                if(version < 4)
                {
                    desc << (*att)[i].name;
                }
                else
                {
                    desc << (*att_4)[i].name;
                }

                if(i != (nb_att - 1))
                {
                    desc << ", ";
                }
            }
        }
        desc << std::ends;

        remove_asyn_request(id);

        if(version < 3)
        {
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
        else
        {
            if(serv_ex != nullptr)
            {
                TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
            }

            if(req_type == TgRequest::WRITE_ATTR)
            {
                throw Tango::NamedDevFailedList(
                    m_ex, device_name, "DeviceProxy::write_attributes_reply()", API_AttributeFailed);
            }
            else
            {
                //
                // Transfer this exception into a DevFailed exception
                //

                Tango::DevFailed ex(m_ex.errors[0].err_list);
                TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
            }
        }
    }

    CORBA::SystemException *sys_ex;
    if((sys_ex = CORBA::SystemException::_downcast(ex_ptr)) != nullptr)
    {
        set_connection_state(CONNECTION_NOTOK);

        //
        // Re-throw nearly all CORBA system exceptions
        //

        CORBA::NVList_ptr req_arg = req->arguments();
        const Tango::AttributeValueList *att;
        const Tango::AttributeValueList *att_4;
        unsigned int nb_att = 0;
        CORBA::NamedValue_ptr nv = req_arg->item(0);
        if(version < 4)
        {
            if(*(nv->value()) >>= att)
            {
                nb_att = att->length();
            }
        }
        else
        {
            if(*(nv->value()) >>= att_4)
            {
                nb_att = att_4->length();
            }
        }

        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(sys_ex, cb_excep_mess);

        //
        // Check if the exception was a connection exception
        // In this case, try to ping the device.
        // If successfull, just returns otherwise, throw the first exception
        //

        std::string ex(cb_excep_mess);
        std::string::size_type pos = ex.find("TRANSIENT_ConnectFailed");
        std::string::size_type pos_one = ex.find("EXIST_NoMatch");
        if(pos != std::string::npos || pos_one != std::string::npos)
        {
            try
            {
                ping();
                return;
            }
            catch(Tango::DevFailed &)
            {
            }
        }

        TangoSys_OMemStream desc;
        desc << "Failed to execute write_attributes_asynch on device " << device_name;
        if(nb_att != 0)
        {
            desc << "\nAttribute(s): ";
            for(unsigned int i = 0; i < nb_att; i++)
            {
                if(version < 4)
                {
                    desc << (*att)[i].name;
                }
                else
                {
                    desc << (*att_4)[i].name;
                }

                if(i != (nb_att - 1))
                {
                    desc << ", ";
                }
            }
        }
        desc << std::ends;

        remove_asyn_request(id);

        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
    }
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::retrieve_read_args()
//
// description :     Retrieve the read_attribute asynchrouns call from the
//                    Tango request object which itself contains the CORBA
//                    request object
//
// argin(s) :        req : The Tango request object
//                    att_list : reference to a string vector whihc will be filled
//                               in with attribute(s) name
//
//-----------------------------------------------------------------------------

void DeviceProxy::retrieve_read_args(const TgRequest &req, std::vector<std::string> &att_list)
{
    att_list.clear();

    //
    // Retrieve which attribute was read
    //

    const Tango::DevVarStringArray *att_names = nullptr;
    try
    {
        CORBA::NVList_ptr args_ptr = req.request->arguments();
        CORBA::NamedValue_ptr arg_ptr = args_ptr->item(0);
        CORBA::Any *arg_val = arg_ptr->value();
        (*arg_val) >>= att_names;

        for(unsigned loop = 0; loop < att_names->length(); loop++)
        {
            att_list.emplace_back((*att_names)[loop]);
        }
    }
    catch(CORBA::SystemException &e)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&e, cb_excep_mess);

        TangoSys_OMemStream desc;
        desc << "Failed to redo the call synchronously on device " << device_name;
        if(att_names != nullptr)
        {
            desc << "\nAttribute(s): ";
            for(unsigned int i = 0; i < att_names->length(); i++)
            {
                desc << (*att_names)[i];
                if(i != (att_names->length() - 1))
                {
                    desc << ", ";
                }
            }
        }
        desc << std::ends;

        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
    }
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::redo_synch_read_call()
//
// description :     Redo a synchronous read_attribute() call
//                    This is needed for re-connection with asynchronous call
//
// argin(s) :        req : The Tango request object
//
//-----------------------------------------------------------------------------

DeviceAttribute *DeviceProxy::redo_synch_read_call(TgRequest &req)
{
    //
    // Retrieve which attribute was read
    //

    std::vector<std::string> att_list;
    retrieve_read_args(req, att_list);

    //
    // Redo the read_attribute but synchronously
    //

    DeviceAttribute attrib = read_attribute(att_list[0]);
    DeviceAttribute *attr_ptr = new DeviceAttribute(attrib);
    return attr_ptr;
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::redo_synch_reads_call()
//
// description :     Redo a read-attributes() call but synchronously
//                    This is needed for re-connection with asynchronous call
//
// argin(s) :        req : The Tango request object
//
//-----------------------------------------------------------------------------

std::vector<DeviceAttribute> *DeviceProxy::redo_synch_reads_call(TgRequest &req)
{
    //
    // Retrieve which attributes was read
    //

    std::vector<std::string> att_list;
    retrieve_read_args(req, att_list);

    //
    // Redo the read_attributes but synchronously
    //

    return read_attributes(att_list);
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::redo_synch_write_call()
//
// description :     Redo a read-attributes() call but synchronously
//                    This is needed for re-connection with asynchronous call
//
// argin(s) :        req : The Tango request object
//
//-----------------------------------------------------------------------------

void DeviceProxy::redo_synch_write_call(TgRequest &req)
{
    //
    // Retrieve which attributes was written
    //

    const Tango::AttributeValueList *att;
    const Tango::AttributeValueList_4 *att_4;

    try
    {
        CORBA::NVList_ptr args_ptr = req.request->arguments();
        CORBA::NamedValue_ptr arg_ptr = args_ptr->item(0);
        CORBA::Any *arg_val = arg_ptr->value();
        if(version < 4)
        {
            (*arg_val) >>= att;
        }
        else
        {
            (*arg_val) >>= att_4;
        }
    }
    catch(CORBA::SystemException &e)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&e, cb_excep_mess);

        TangoSys_OMemStream desc;
        desc << "Failed to redo the call synchronously on device " << device_name << std::ends;

        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
    }

    //
    // Redo the write_attributes but synchronously
    //

    if(version < 4)
    {
        write_attribute(*att);
    }
    else
    {
        write_attribute(*att_4);
    }
}

//-----------------------------------------------------------------------------
//
// method :         Connection::redo_synch_cmd()
//
// description :     Redo a command_inout() call but synchronously
//                    This is needed for re-connection with asynchronous call
//
// argin(s) :        req : The Tango request object
//
//-----------------------------------------------------------------------------

DeviceData Connection::redo_synch_cmd(const TgRequest &req)
{
    const char *cmd_name = nullptr;
    const CORBA::Any *a_ptr;

    try
    {
        CORBA::NVList_ptr args_ptr = req.request->arguments();
        CORBA::NamedValue_ptr arg_ptr = args_ptr->item(0);

        *(arg_ptr->value()) >>= cmd_name;

        arg_ptr = args_ptr->item(1);
        *(arg_ptr->value()) >>= a_ptr;
    }
    catch(CORBA::SystemException &e)
    {
        char cb_excep_mess[256];
        Tango::Except::print_CORBA_SystemException_r(&e, cb_excep_mess);

        TangoSys_OMemStream desc;
        desc << "Failed to redo the call synchronously on device " << dev_name() << std::ends;

        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, desc.str());
    }

    //
    // Redo the call synchronously
    //

    DeviceData dd;
    CORBA::Any *tmp_any_ptr = new CORBA::Any(*a_ptr);
    dd.any = tmp_any_ptr;

    return command_inout(cmd_name, dd);
}

//-----------------------------------------------------------------------------
//
// method :         Connection::cancel_asynch_request()
//
// description :     Cancel a running asynchronous request
//                    This is a client side call. Obviously, the call cannot be aborted
//                    while it is running in the device.
//
// argin(s) :        id : The asynchronous call identifier
//
//-----------------------------------------------------------------------------

void Connection::cancel_asynch_request(long id)
{
    omni_mutex_lock guard(asyn_mutex);
    ApiUtil::instance()->get_pasyn_table()->mark_as_cancelled(id);
    pasyn_ctr--;
}

//-----------------------------------------------------------------------------
//
// method :         Connection::cancel_all_polling_asynch_request()
//
// description :     Cancel all running asynchronous request
//                    This is a client side call. Obviously, the calls cannot be aborted
//                    while it is running in the device.
//
// argin(s) :        id : The asynchronous call identifier
//
//-----------------------------------------------------------------------------

void Connection::cancel_all_polling_asynch_request()
{
    omni_mutex_lock guard(asyn_mutex);
    ApiUtil::instance()->get_pasyn_table()->mark_all_polling_as_cancelled();
    pasyn_ctr = 0;
}

//-----------------------------------------------------------------------------
//
// method :         Connection::omni420_xxx
//                  DeviceProxy::omni420_xxx
//
// description :     These methods are there due to the compatibility pb with omniORB
//                  4.2.0 about CORBA::Request class methods throwing exceptions while
//                  it was not the case in omniORB 4.1!
//
//-----------------------------------------------------------------------------

void Connection::omni420_timeout(int id, char *cb_excep_mess)
{
    bool need_reconnect = false;
    if(ext->has_alt_adr)
    {
        try
        {
            Device_var dev = Device::_duplicate(device);
            dev->ping();
        }
        catch(CORBA::TRANSIENT &trans_ping)
        {
            if(trans_ping.minor() == omni::TRANSIENT_ConnectFailed ||
               trans_ping.minor() == omni::TRANSIENT_CallTimedout)
            {
                need_reconnect = true;
            }
        }
        catch(...)
        {
        }
    }

    std::stringstream ss;
    remove_asyn_request(id);

    if(!need_reconnect)
    {
        ss << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_DeviceTimedOut, ss.str());
    }
    else
    {
        set_connection_state(CONNECTION_NOTOK);

        ss << "Failed to execute command_inout_asynch on device " << dev_name();
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
    }
}

DeviceData Connection::omni420_except(int id, char *cb_excep_mess, TgRequest &req)
{
    //
    // Check if the exception was a connection exception
    // If so, execute the command synchronously (tries to reconnect)
    // If successful just return, otherwise throw the first exception
    //

    std::string ex(cb_excep_mess);
    std::string::size_type pos = ex.find("TRANSIENT_ConnectFailed");
    if(pos != std::string::npos)
    {
        try
        {
            DeviceData dd_out = redo_synch_cmd(req);

            //
            // Remove request from request global table and return
            //

            remove_asyn_request(id);
            return dd_out;
        }
        catch(Tango::DevFailed &)
        {
        }
    }

    std::stringstream ss;
    ss << "Failed to execute command_inout_asynch on device " << dev_name();

    remove_asyn_request(id);

    TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());

    DeviceData dummy;
    return dummy;
}

void DeviceProxy::omni420_timeout_attr(int id, char *cb_excep_mess, read_attr_type type)
{
    bool need_reconnect = false;
    if(ext->has_alt_adr)
    {
        try
        {
            Device_var dev = Device::_duplicate(device);
            dev->ping();
        }
        catch(CORBA::TRANSIENT &trans_ping)
        {
            if(trans_ping.minor() == omni::TRANSIENT_ConnectFailed ||
               trans_ping.minor() == omni::TRANSIENT_CallTimedout)
            {
                need_reconnect = true;
            }
        }
        catch(...)
        {
        }
    }

    std::stringstream ss;
    remove_asyn_request(id);

    std::string meth;
    if(type == SIMPLE)
    {
        meth = "DeviceProxy::read_attribute_reply()";
    }
    else
    {
        meth = "DeviceProxy::read_attributes_reply()";
    }

    if(!need_reconnect)
    {
        ss << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_DeviceTimedOut, ss.str());
    }
    else
    {
        set_connection_state(CONNECTION_NOTOK);

        ss << "Failed to execute command_inout_asynch on device " << dev_name();
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
    }
}

void DeviceProxy::omni420_except_attr(int id, char *cb_excep_mess, read_attr_type type)
{
    std::string ex(cb_excep_mess);
    std::string::size_type pos = ex.find("TRANSIENT_ConnectFailed");
    if(pos != std::string::npos)
    {
        try
        {
            ping();
            return;
        }
        catch(Tango::DevFailed &)
        {
        }
    }

    std::stringstream ss;
    ss << "Failed to execute read_attributes_asynch on device " << device_name;

    remove_asyn_request(id);

    std::string meth;
    if(type == SIMPLE)
    {
        meth = "DeviceProxy::read_attribute_reply()";
    }
    else
    {
        meth = "DeviceProxy::read_attributes_reply()";
    }

    TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
}

void DeviceProxy::omni420_timeout_wattr(int id, char *cb_excep_mess)
{
    bool need_reconnect = false;
    if(ext->has_alt_adr)
    {
        try
        {
            Device_var dev = Device::_duplicate(device);
            dev->ping();
        }
        catch(CORBA::TRANSIENT &trans_ping)
        {
            if(trans_ping.minor() == omni::TRANSIENT_ConnectFailed ||
               trans_ping.minor() == omni::TRANSIENT_CallTimedout)
            {
                need_reconnect = true;
            }
        }
        catch(...)
        {
        }
    }

    std::stringstream ss;
    remove_asyn_request(id);

    if(!need_reconnect)
    {
        ss << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_DeviceTimedOut, ss.str());
    }
    else
    {
        set_connection_state(CONNECTION_NOTOK);

        ss << "Failed to execute write_attribute_asynch on device " << dev_name();
        TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
    }
}

void DeviceProxy::omni420_except_wattr(int id, char *cb_excep_mess)
{
    std::string ex(cb_excep_mess);
    std::string::size_type pos = ex.find("TRANSIENT_ConnectFailed");
    if(pos != std::string::npos)
    {
        try
        {
            ping();
            return;
        }
        catch(Tango::DevFailed &)
        {
        }
    }

    std::stringstream ss;
    ss << "Failed to execute write_attributes_asynch on device " << device_name;

    remove_asyn_request(id);

    TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, cb_excep_mess, API_CommunicationFailed, ss.str());
}

} // namespace Tango
