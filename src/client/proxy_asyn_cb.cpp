//
// cpp     - C++ source code file for TANGO api class and asynchronous callback
//      related methods
//
// programmer     - Emmanuel Taurel (taurel@esrf.fr)
//
// original     - January 2003
//
// Copyright (C) :      2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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

#include <tango/client/Connection.h>
#include <tango/client/apiexcept.h>
#include <tango/client/ApiUtil.h>
#include <tango/client/DeviceProxy.h>
#include <tango/client/DeviceAttribute.h>
#include <tango/server/tango_config.h>

#include <thread>

namespace Tango
{

//--------------------------------------------------------------------------------------------------------------------
//
// helper method to user callback and printout error, if failed
//
//--------------------------------------------------------------------------------------------------------------------
template <typename Func>
static void catch_and_report_callback_failure(const char *method_name, const std::string &dev_name, Func &&callback)
{
    try
    {
        // invoke the real callback
        callback();
    }
    catch(const DevFailed &e)
    {
        TangoSys_OMemStream o;
        o << method_name << " callback method of " << dev_name << " got DevFailed exception: \n\n" << e.errors[0].desc;
        ApiUtil::instance()->print_error_message(o.str().c_str());
    }
    catch(const std::exception &e)
    {
        TangoSys_OMemStream o;
        o << method_name << " callback method of " << dev_name << " got std::exception: \n\n" << e.what();
        ApiUtil::instance()->print_error_message(o.str().c_str());
    }
    catch(...)
    {
        TangoSys_OMemStream o;
        o << method_name << " callback method of " << dev_name << " got unknown exception";
        ApiUtil::instance()->print_error_message(o.str().c_str());
    }
}

//-----------------------------------------------------------------------------
//
// method :         Connection::command_inout_asyn()
//
// description :     Send a command to a Tango device asynchrnously.
//            The client is not blocked until the command is executed
//
// argin(s) :        command : The command name
//            data_in : Command input data
//            cb : The callback object
//
//-----------------------------------------------------------------------------

void Connection::command_inout_asynch(const std::string &command, const DeviceData &data_in, CallBack &cb)
{
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
    // Create the request object
    //

    CORBA::ORB::RequestSeq req_seq;
    req_seq.length(1);

    if(version >= 4)
    {
        req_seq[0] = device_4->_request("command_inout_4");
    }
    else if(version >= 2)
    {
        req_seq[0] = device_2->_request("command_inout_2");
    }
    else
    {
        req_seq[0] = device->_request("command_inout");
    }

    req_seq[0]->add_in_arg() <<= command.c_str();
    req_seq[0]->add_in_arg() <<= data_in.any.in();

    if(version >= 4)
    {
        req_seq[0]->add_in_arg() <<= source;
        req_seq[0]->add_in_arg() <<= get_client_identification();
    }
    else if(version >= 2)
    {
        req_seq[0]->add_in_arg() <<= source;
    }

    req_seq[0]->set_return_type(CORBA::_tc_any);
    req_seq[0]->exceptions()->add(Tango::_tc_DevFailed);

    //
    // Send the request
    // and store it in the global asynchronous cb requests table
    //

    ApiUtil *au = ApiUtil::instance();
    add_asyn_cb_request(req_seq[0], &cb, this, TgRequest::CMD_INOUT);
    CORBA::ORB_var orb = au->get_orb();
    orb->send_multiple_requests_deferred(req_seq);
    if(au->get_asynch_cb_sub_model() == PUSH_CALLBACK)
    {
        au->get_pasyn_table()->signal();
    }
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
//            cb : The callback object
//
//-----------------------------------------------------------------------------

void Connection::command_inout_asynch(const std::string &command, CallBack &cb)
{
    DeviceData data_in;
    command_inout_asynch(command, data_in, cb);
}

//-----------------------------------------------------------------------------
//
// method :         Connection::get_asynch_replies()
//
// description :     Try to obtain data returned by a command asynchronously
//            requested. This method does not block if the reply is
//            not yet arrived. This method fires callback for already
//            arrived replies.
//
//-----------------------------------------------------------------------------

void Connection::get_asynch_replies()
{
    CORBA::ORB_var orb = ApiUtil::instance()->get_orb();

    //
    // First get all replies from ORB buffers
    //

    try
    {
        while(orb->poll_next_response())
        {
            CORBA::Request_ptr req;
            orb->get_next_response(req);

            //
            // Retrieve this request in the cb request map and mark it as "arrived" in both maps
            //

            TgRequest &tg_req = ApiUtil::instance()->get_pasyn_table()->get_request(req);

            tg_req.arrived = true;
            ApiUtil::instance()->get_pasyn_table()->mark_as_arrived(req);

            //
            // Is it a request for our device, process it ?
            //

            if(tg_req.dev == this)
            {
                switch(tg_req.req_type)
                {
                case TgRequest::CMD_INOUT:
                    Cb_Cmd_Request(req, tg_req.cb_ptr);
                    break;

                case TgRequest::READ_ATTR:
                    Cb_ReadAttr_Request(req, tg_req.cb_ptr);
                    break;

                case TgRequest::WRITE_ATTR:
                case TgRequest::WRITE_ATTR_SINGLE:
                    Cb_WriteAttr_Request(req, tg_req.cb_ptr);
                    break;
                }

                remove_asyn_cb_request(this, req);
            }
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
    // Now check all replies already there
    //

    TgRequest *tg_ptr;
    while((tg_ptr = ApiUtil::instance()->get_pasyn_table()->get_request(this)) != nullptr)
    {
        switch(tg_ptr->req_type)
        {
        case TgRequest::CMD_INOUT:
            Cb_Cmd_Request(tg_ptr->request, tg_ptr->cb_ptr);
            break;

        case TgRequest::READ_ATTR:
            Cb_ReadAttr_Request(tg_ptr->request, tg_ptr->cb_ptr);
            break;

        case TgRequest::WRITE_ATTR:
        case TgRequest::WRITE_ATTR_SINGLE:
            Cb_WriteAttr_Request(tg_ptr->request, tg_ptr->cb_ptr);
            break;
        }

        remove_asyn_cb_request(this, tg_ptr->request);
    }
}

//-----------------------------------------------------------------------------
//
// method :         Connection::Cb_Cmd_Request()
//
// description :     Fire callback for a command request
//
// argin(s) :        req : The CORBA request object
//            cb_ptr : The callback object
//
//-----------------------------------------------------------------------------

void Connection::Cb_Cmd_Request(CORBA::Request_ptr req, Tango::CallBack *cb_ptr)
{
    DeviceData data_out;
    Tango::DevErrorList errors(2);
    errors.length(0);

    CORBA::NVList_ptr req_arg = req->arguments();
    const char *cmd = nullptr;
    CORBA::NamedValue_ptr nv = req_arg->item(0);
    *(nv->value()) >>= cmd;

    //
    // Check if the reply is an exception
    //

    CORBA::Environment_ptr env = req->env();
    if(!CORBA::is_nil(env) && (env->exception() == nullptr))
    {
        //
        // Get received value
        //

        const CORBA::Any *received;
        CORBA::Any &dii_any = req->return_value();
        dii_any >>= received;
        CORBA::Any *server_any = new CORBA::Any(*received);

        data_out.any = server_any;
    }
    else
    {
        //
        // Retrieve exception
        //

        CORBA::Exception *ex_ptr = env->exception();

        //
        // Special treatement for timeout exception (TRANSIENT with specific minor code)
        //

        CORBA::TRANSIENT *tra;
        CORBA::UnknownUserException *unk_ex;
        CORBA::SystemException *sys_ex;
        bool to_except = false;

        if((tra = CORBA::TRANSIENT::_downcast(ex_ptr)) != nullptr)
        {
            if(tra->minor() == omni::TRANSIENT_CallTimedout)
            {
                to_except = true;
                char *tmp = Tango::string_dup(cmd);
                char cb_excep_mess[256];
                Tango::Except::print_CORBA_SystemException_r(tra, cb_excep_mess);

                TangoSys_OMemStream desc;
                desc << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
                desc << ", command " << tmp << std::ends;
                Tango::string_free(tmp);

                errors.length(2);
                errors[0].desc = Tango::string_dup(cb_excep_mess);
                errors[0].severity = Tango::ERR;
                errors[0].reason = Tango::string_dup(API_CorbaException);
                errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

                std::string st = desc.str();
                errors[1].desc = Tango::string_dup(st.c_str());
                errors[1].severity = Tango::ERR;
                errors[1].reason = Tango::string_dup(API_DeviceTimedOut);
                errors[1].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
            }
        }

        if((unk_ex = CORBA::UnknownUserException::_downcast(ex_ptr)) != nullptr)
        {
            //
            // It is a UserUnknownException exception. This means that the
            // server has sent a DevFailed exception
            //

            const Tango::DevFailed *serv_ex;
            unk_ex->exception() >>= serv_ex;
            errors = serv_ex->errors;

            char *tmp = Tango::string_dup(cmd);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout_asynch on device " << dev_name();
            desc << ", command " << tmp << std::ends;
            Tango::string_free(tmp);

            long nb_err = errors.length();
            errors.length(nb_err + 1);
            errors[nb_err].severity = Tango::ERR;

            std::string st = desc.str();
            errors[nb_err].desc = Tango::string_dup(st.c_str());
            errors[nb_err].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
            errors[nb_err].reason = Tango::string_dup(API_CommandFailed);
        }
        else if(((sys_ex = CORBA::SystemException::_downcast(ex_ptr)) != nullptr) && (!to_except))
        {
            set_connection_state(CONNECTION_NOTOK);

            //
            // Re-throw all CORBA system exceptions
            //

            char *tmp = Tango::string_dup(cmd);

            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(sys_ex, cb_excep_mess);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout_asynch on device " << dev_name();
            desc << ", command " << cmd << std::ends;
            Tango::string_free(tmp);

            errors.length(2);
            errors[0].desc = Tango::string_dup(cb_excep_mess);
            errors[0].severity = Tango::ERR;
            errors[0].reason = Tango::string_dup(API_CorbaException);
            errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string st = desc.str();
            errors[1].desc = Tango::string_dup(st.c_str());
            errors[1].severity = Tango::ERR;
            errors[1].reason = Tango::string_dup(API_CommunicationFailed);
            errors[1].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
        }
    }

    //
    // Fire Callback
    //

    std::string cmd_str(cmd);

    DeviceProxy *local_dev = static_cast<DeviceProxy *>(this);
    CmdDoneEvent *cb_data = new CmdDoneEvent(local_dev, cmd_str, data_out, errors);

    std::unique_ptr<CmdDoneEvent> auto_cb_data(cb_data);

    catch_and_report_callback_failure("cmd_ended", dev_name(), [&]() { cb_ptr->cmd_ended(auto_cb_data.get()); });
}

//-----------------------------------------------------------------------------
//
// method :         Connection::Cb_ReadAttr_Request()
//
// description :     Fire callback for a read attributes request
//
// argin(s) :        req : The CORBA request object
//            cb_ptr : The callback object
//
//-----------------------------------------------------------------------------

void Connection::Cb_ReadAttr_Request(CORBA::Request_ptr req, Tango::CallBack *cb_ptr)
{
    std::vector<DeviceAttribute> *dev_attr = nullptr;
    Tango::DevErrorList errors(2);
    errors.length(0);

    CORBA::NVList_ptr req_arg = req->arguments();
    const Tango::DevVarStringArray *names;
    CORBA::NamedValue_ptr nv = req_arg->item(0);
    *(nv->value()) >>= names;

    std::vector<std::string> attr_names;
    attr_names.resize(names->length());
    for(unsigned int i = 0; i < names->length(); i++)
    {
        attr_names[i] = (*names)[i];
    }

    //
    // Check if the reply is an exception
    //

    CORBA::Environment_ptr env = req->env();
    if(!CORBA::is_nil(env) && (env->exception() == nullptr))
    {
        //
        // Get received value
        //

        dev_attr = new(std::vector<DeviceAttribute>);

        const Tango::AttributeValueList *received;
        const Tango::AttributeValueList_3 *received_3;
        const Tango::AttributeValueList_4 *received_4;
        const Tango::AttributeValueList_5 *received_5;
        unsigned long nb_received;

        CORBA::Any &dii_any = req->return_value();

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
                // Add an error in the event error stack in case the attribute has an error
                //

                DevErrorList_var &err_list = (*dev_attr)[i].get_error_list();
                long nb_except = err_list.in().length();
                if(nb_except != 0)
                {
                    TangoSys_OMemStream desc;
                    desc << "Failed to read_attributes on device " << dev_name();
                    desc << ", attribute " << (*dev_attr)[i].name << std::ends;

                    const auto idx = errors.length();
                    errors.length(idx + 1);
                    errors[idx].reason = Tango::string_dup(API_AttributeFailed);
                    errors[idx].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
                    std::string st = desc.str();
                    errors[idx].desc = Tango::string_dup(st.c_str());
                    errors[idx].severity = Tango::ERR;
                }
            }
            else
            {
                ApiUtil::attr_to_device(&((*received)[i]), nullptr, version, &((*dev_attr)[i]));
            }
        }
    }
    else
    {
        //
        // The reply is an exception
        //

        CORBA::Exception *ex_ptr = env->exception();
        CORBA::UnknownUserException *unk_ex;
        CORBA::SystemException *sys_ex;
        bool to_except = false;

        //
        // Special treatement for timeout exception (TRANSIENT with specific minor code)
        //

        CORBA::TRANSIENT *tra;
        if((tra = CORBA::TRANSIENT::_downcast(ex_ptr)) != nullptr)
        {
            if(tra->minor() == omni::TRANSIENT_CallTimedout)
            {
                to_except = true;
                char cb_excep_mess[256];
                Tango::Except::print_CORBA_SystemException_r(tra, cb_excep_mess);

                TangoSys_OMemStream desc;
                desc << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
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

                errors.length(2);
                errors[0].desc = Tango::string_dup(cb_excep_mess);
                errors[0].severity = Tango::ERR;
                errors[0].reason = Tango::string_dup(API_CorbaException);
                errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

                std::string st = desc.str();
                errors[1].desc = Tango::string_dup(st.c_str());
                errors[1].severity = Tango::ERR;
                errors[1].reason = Tango::string_dup(API_DeviceTimedOut);
                errors[1].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
            }
        }

        if((unk_ex = CORBA::UnknownUserException::_downcast(ex_ptr)) != nullptr)
        {
            //
            // It is a UserUnknownException exception. This means that the
            // server has sent a DevFailed exception
            //

            const Tango::DevFailed *serv_ex;
            unk_ex->exception() >>= serv_ex;
            errors = serv_ex->errors;

            TangoSys_OMemStream desc;
            desc << "Failed to execute read_attribute_asynch on device " << dev_name();
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

            long nb_err = errors.length();
            errors.length(nb_err + 1);
            errors[nb_err].severity = Tango::ERR;

            std::string st = desc.str();
            errors[nb_err].desc = Tango::string_dup(st.c_str());
            errors[nb_err].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
            errors[nb_err].reason = Tango::string_dup(API_AttributeFailed);
        }
        else if(((sys_ex = CORBA::SystemException::_downcast(ex_ptr)) != nullptr) && (!to_except))
        {
            set_connection_state(CONNECTION_NOTOK);

            //
            // Re-throw all CORBA system exceptions
            //

            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(sys_ex, cb_excep_mess);

            TangoSys_OMemStream desc;
            desc << "Failed to execute read_attributes_asynch on device " << dev_name();
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

            errors.length(2);
            errors[0].desc = Tango::string_dup(cb_excep_mess);
            errors[0].severity = Tango::ERR;
            errors[0].reason = Tango::string_dup(API_CorbaException);
            errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string st = desc.str();
            errors[1].desc = Tango::string_dup(st.c_str());
            errors[1].severity = Tango::ERR;
            errors[1].reason = Tango::string_dup(API_CommunicationFailed);
            errors[1].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
        }
    }

    //
    // Fire Callback
    //

    DeviceProxy *local_dev = static_cast<DeviceProxy *>(this);
    AttrReadEvent *cb_data = new AttrReadEvent(local_dev, attr_names, dev_attr, errors);

    std::unique_ptr<AttrReadEvent> auto_cb_data(cb_data);

    catch_and_report_callback_failure("attr_read", dev_name(), [&]() { cb_ptr->attr_read(auto_cb_data.get()); });
}

//-----------------------------------------------------------------------------
//
// method :         Connection::Cb_WriteAttr_Request()
//
// description :     Fire callback for a write attribute request
//
// argin(s) :        req : The CORBA request object
//            cb_ptr : The callback object
//
//-----------------------------------------------------------------------------

void Connection::Cb_WriteAttr_Request(CORBA::Request_ptr req, Tango::CallBack *cb_ptr)
{
    Tango::NamedDevFailedList err_3;

    CORBA::NVList_ptr req_arg = req->arguments();
    const Tango::AttributeValueList *att;
    const Tango::AttributeValueList_4 *att_4;
    CORBA::NamedValue_ptr nv = req_arg->item(0);
    long nb_attr = 0;
    if(version >= 4)
    {
        if(*(nv->value()) >>= att_4)
        {
            nb_attr = att_4->length();
        }
    }
    else
    {
        if(*(nv->value()) >>= att)
        {
            nb_attr = att->length();
        }
    }

    //
    // Check if the reply is an exception
    //

    CORBA::Environment_ptr env = req->env();
    if(!CORBA::is_nil(env) && (env->exception() != nullptr))
    {
        //
        // The reply is an exception
        //

        CORBA::Exception *ex_ptr = env->exception();
        CORBA::UnknownUserException *unk_ex;
        CORBA::SystemException *sys_ex;
        bool to_except = false;

        //
        // Special treatement for timeout exception (TRANSIENT with specific minor code)
        //

        CORBA::TRANSIENT *tra;
        if((tra = CORBA::TRANSIENT::_downcast(ex_ptr)) != nullptr)
        {
            if(tra->minor() == omni::TRANSIENT_CallTimedout)
            {
                to_except = true;
                char cb_excep_mess[256];
                Tango::Except::print_CORBA_SystemException_r(tra, cb_excep_mess);

                TangoSys_OMemStream desc;
                desc << "Timeout (" << timeout << " mS) exceeded on device " << dev_name();
                if(nb_attr != 0)
                {
                    desc << "\nAttribute(s): ";
                    for(int i = 0; i < nb_attr; i++)
                    {
                        (version < 4) ? desc << (*att)[i].name : desc << (*att_4)[i].name;

                        if(i != (nb_attr - 1))
                        {
                            desc << ", ";
                        }
                    }
                }
                desc << std::ends;

                err_3.errors.length(2);
                err_3.errors[0].desc = Tango::string_dup(cb_excep_mess);
                err_3.errors[0].severity = Tango::ERR;
                err_3.errors[0].reason = Tango::string_dup(API_CorbaException);
                err_3.errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

                std::string st = desc.str();
                err_3.errors[1].desc = Tango::string_dup(st.c_str());
                err_3.errors[1].severity = Tango::ERR;
                err_3.errors[1].reason = Tango::string_dup(API_DeviceTimedOut);
                err_3.errors[1].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
            }
        }

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
            TangoSys_OMemStream desc;

            if(version < 3)
            {
                unk_ex->exception() >>= serv_ex;
                ex = *serv_ex;

                desc << "Failed to execute write_attributes_asynch on device " << dev_name();
                if(nb_attr != 0)
                {
                    desc << "\nAttribute(s): ";
                }
                for(int i = 0; i < nb_attr; i++)
                {
                    desc << (*att)[i].name.in();
                    if(i != (nb_attr - 1))
                    {
                        desc << ", ";
                    }
                }
                desc << std::ends;

                err_3.errors.length(1);
                err_3.errors[0].severity = Tango::ERR;
                std::string st = desc.str();
                err_3.errors[0].desc = Tango::string_dup(st.c_str());
                err_3.errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
                err_3.errors[0].reason = Tango::string_dup(API_AttributeFailed);

                err_3.err_list.resize(1);
                err_3.err_list[0].err_stack = serv_ex->errors;
                err_3.err_list[0].name = (*att)[0].name.in();
                err_3.err_list[0].idx_in_call = 0;
            }
            else
            {
                if(unk_ex->exception() >>= multi_serv_ex)
                {
                    m_ex = *multi_serv_ex;
                    err_3 = Tango::NamedDevFailedList(m_ex,
                                                      dev_name(),
                                                      (const char *) "Connection::Cb_WriteAttr_Request()",
                                                      (const char *) API_AttributeFailed);
                }
                else
                {
                    unk_ex->exception() >>= serv_ex;
                    ex = *serv_ex;

                    desc << "Failed to execute write_attributes_asynch on device " << dev_name();
                    if(nb_attr != 0)
                    {
                        desc << "\nAttribute(s): ";
                    }
                    for(int i = 0; i < nb_attr; i++)
                    {
                        (version < 4) ? desc << (*att)[i].name : desc << (*att_4)[i].name;

                        if(i != (nb_attr - 1))
                        {
                            desc << ", ";
                        }
                    }
                    desc << std::ends;

                    err_3.errors.length(1);
                    err_3.errors[0].severity = Tango::ERR;

                    std::string st = desc.str();
                    err_3.errors[0].desc = Tango::string_dup(st.c_str());
                    err_3.errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
                    err_3.errors[0].reason = Tango::string_dup(API_AttributeFailed);

                    err_3.err_list.resize(1);
                    err_3.err_list[0].err_stack = serv_ex->errors;
                    err_3.err_list[0].name = (*att)[0].name.in();
                    err_3.err_list[0].idx_in_call = 0;
                }
            }
        }
        else if(((sys_ex = CORBA::SystemException::_downcast(ex_ptr)) != nullptr) && (!to_except))
        {
            set_connection_state(CONNECTION_NOTOK);

            //
            // Re-throw all CORBA system exceptions
            //

            char cb_excep_mess[256];
            Tango::Except::print_CORBA_SystemException_r(sys_ex, cb_excep_mess);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes_asynch on device " << dev_name();
            if(nb_attr != 0)
            {
                desc << "\nAttribute(s): ";
            }
            for(int i = 0; i < nb_attr; i++)
            {
                (version < 4) ? desc << (*att)[i].name : desc << (*att_4)[i].name;

                if(i != (nb_attr - 1))
                {
                    desc << ", ";
                }
            }
            desc << std::ends;

            err_3.errors.length(2);
            err_3.errors[0].desc = Tango::string_dup(cb_excep_mess);
            err_3.errors[0].severity = Tango::ERR;
            err_3.errors[0].reason = Tango::string_dup(API_CorbaException);
            err_3.errors[0].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string st = desc.str();
            err_3.errors[1].desc = Tango::string_dup(st.c_str());
            err_3.errors[1].severity = Tango::ERR;
            err_3.errors[1].reason = Tango::string_dup(API_CommunicationFailed);
            err_3.errors[1].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);
        }
    }

    //
    // Fire Callback
    //

    std::vector<std::string> att_name;
    if(version >= 4)
    {
        for(int i = 0; i < nb_attr; i++)
        {
            att_name.emplace_back((*att_4)[i].name.in());
        }
    }
    else
    {
        for(int i = 0; i < nb_attr; i++)
        {
            att_name.emplace_back((*att)[i].name.in());
        }
    }

    DeviceProxy *local_dev = static_cast<DeviceProxy *>(this);
    AttrWrittenEvent *cb_data = new AttrWrittenEvent(local_dev, att_name, err_3);

    std::unique_ptr<AttrWrittenEvent> auto_cb_data(cb_data);

    catch_and_report_callback_failure("attr_written", dev_name(), [&]() { cb_ptr->attr_written(auto_cb_data.get()); });
}

//-----------------------------------------------------------------------------
//
// method :         Connection::get_asynch_replies()
//
// description :     Try to obtain data returned by a command asynchronously
//            requested. This method block for the specified timeout
//            if the reply is not yet arrived. This method fires
//            callback when the reply arived. If the timeout is
//            set to 0, the call wait undefinitely wait the reply.
//
// argin(s) :         call_timeout : The timeout in mS
//
//-----------------------------------------------------------------------------

void Connection::get_asynch_replies(long call_timeout)
{
    //
    // First check all replies already there
    //

    TgRequest *tg_ptr;
    while((tg_ptr = ApiUtil::instance()->get_pasyn_table()->get_request(this)) != nullptr)
    {
        switch(tg_ptr->req_type)
        {
        case TgRequest::CMD_INOUT:
            Cb_Cmd_Request(tg_ptr->request, tg_ptr->cb_ptr);
            break;

        case TgRequest::READ_ATTR:
            Cb_ReadAttr_Request(tg_ptr->request, tg_ptr->cb_ptr);
            break;

        case TgRequest::WRITE_ATTR:
        case TgRequest::WRITE_ATTR_SINGLE:
            Cb_WriteAttr_Request(tg_ptr->request, tg_ptr->cb_ptr);
            break;
        }

        remove_asyn_cb_request(this, tg_ptr->request);
    }

    //
    // If they are requests already sent but without being replied yet
    //

    if(get_pasyn_cb_ctr() != 0)
    {
        CORBA::ORB_var orb = ApiUtil::instance()->get_orb();
        CORBA::Request_ptr req;

        if(call_timeout != 0)
        {
            //
            // A timeout has been specified. Wait if there are still request without
            // replies but not more than the specified timeout. Leave method if the
            // timeout is not arrived but there is no more request without reply
            //

            long nb = call_timeout / 20;

            while((nb > 0) && (get_pasyn_cb_ctr() != 0))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                nb--;

                if(orb->poll_next_response())
                {
                    orb->get_next_response(req);

                    //
                    // Retrieve this request in the cb request map and mark it as "arrived" in both maps
                    //

                    TgRequest &tg_req = ApiUtil::instance()->get_pasyn_table()->get_request(req);

                    tg_req.arrived = true;
                    ApiUtil::instance()->get_pasyn_table()->mark_as_arrived(req);

                    //
                    // Is it a request for our device, process it ?
                    //

                    if(tg_req.dev == this)
                    {
                        switch(tg_req.req_type)
                        {
                        case TgRequest::CMD_INOUT:
                            Cb_Cmd_Request(req, tg_req.cb_ptr);
                            break;

                        case TgRequest::READ_ATTR:
                            Cb_ReadAttr_Request(req, tg_req.cb_ptr);
                            break;

                        case TgRequest::WRITE_ATTR:
                        case TgRequest::WRITE_ATTR_SINGLE:
                            Cb_WriteAttr_Request(req, tg_req.cb_ptr);
                            break;
                        }

                        remove_asyn_cb_request(this, req);
                    }
                }
            }

            //
            // Throw exception if the timeout has expired but there are still request
            // without replies
            //

            if((nb == 0) && (get_pasyn_cb_ctr() != 0))
            {
                TangoSys_OMemStream desc;
                desc << "Still some reply(ies) for asynchronous callback call(s) to be received" << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiAsynNotThereExcept, API_AsynReplyNotArrived, desc.str());
            }
        }
        else
        {
            //
            // If timeout is set to 0, this means wait until all the requests sent to this
            // device has sent their replies
            //

            while(get_pasyn_cb_ctr() != 0)
            {
                orb->get_next_response(req);

                //
                // Retrieve this request in the cb request map and mark it as "arrived" in both maps
                //

                TgRequest &tg_req = ApiUtil::instance()->get_pasyn_table()->get_request(req);

                tg_req.arrived = true;
                ApiUtil::instance()->get_pasyn_table()->mark_as_arrived(req);

                //
                // Is it a request for our device, process it ?
                //

                if(tg_req.dev == this)
                {
                    switch(tg_req.req_type)
                    {
                    case TgRequest::CMD_INOUT:
                        Cb_Cmd_Request(req, tg_req.cb_ptr);
                        break;

                    case TgRequest::READ_ATTR:
                        Cb_ReadAttr_Request(req, tg_req.cb_ptr);
                        break;

                    case TgRequest::WRITE_ATTR:
                    case TgRequest::WRITE_ATTR_SINGLE:
                        Cb_WriteAttr_Request(req, tg_req.cb_ptr);
                        break;
                    }

                    remove_asyn_cb_request(this, req);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::read_attributes_asynch()
//
// description :     Read Tango device attributes asynchrnously.
//            The client is not blocked until the attributes are read
//
// argin(s) :        attr_names : The attribute name(s)
//            cb : The callback object
//
//-----------------------------------------------------------------------------

void DeviceProxy::read_attributes_asynch(const std::vector<std::string> &attr_names, CallBack &cb)
{
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

    CORBA::ORB::RequestSeq req_seq;
    req_seq.length(1);

    Tango::DevVarStringArray names;
    long nb_names = attr_names.size();
    names.length(nb_names);
    for(int i = 0; i < nb_names; i++)
    {
        names[i] = attr_names[i].c_str();
    }

    if(version >= 5)
    {
        req_seq[0] = Connection::device_5->_request("read_attributes_5");
        req_seq[0]->add_in_arg() <<= names;
        req_seq[0]->add_in_arg() <<= source;
        req_seq[0]->add_in_arg() <<= get_client_identification();
        req_seq[0]->set_return_type(Tango::_tc_AttributeValueList_5);
    }
    else if(version == 4)
    {
        req_seq[0] = Connection::device_4->_request("read_attributes_4");
        req_seq[0]->add_in_arg() <<= names;
        req_seq[0]->add_in_arg() <<= source;
        req_seq[0]->add_in_arg() <<= get_client_identification();
        req_seq[0]->set_return_type(Tango::_tc_AttributeValueList_4);
    }
    else if(version == 3)
    {
        req_seq[0] = Connection::device_3->_request("read_attributes_3");
        req_seq[0]->add_in_arg() <<= names;
        req_seq[0]->add_in_arg() <<= source;
        req_seq[0]->set_return_type(Tango::_tc_AttributeValueList_3);
    }
    else if(version == 2)
    {
        req_seq[0] = device_2->_request("read_attributes_2");
        req_seq[0]->add_in_arg() <<= names;
        req_seq[0]->add_in_arg() <<= source;
        req_seq[0]->set_return_type(Tango::_tc_AttributeValueList);
    }
    else
    {
        req_seq[0] = device->_request("read_attributes");
        req_seq[0]->add_in_arg() <<= names;
        req_seq[0]->set_return_type(Tango::_tc_AttributeValueList);
    }
    req_seq[0]->exceptions()->add(Tango::_tc_DevFailed);

    //
    // Send the request
    // and store it in the global asynchronous cb requests table
    //

    ApiUtil *au = ApiUtil::instance();
    add_asyn_cb_request(req_seq[0], &cb, this, TgRequest::READ_ATTR);
    CORBA::ORB_var orb = au->get_orb();
    orb->send_multiple_requests_deferred(req_seq);
    if(au->get_asynch_cb_sub_model() == PUSH_CALLBACK)
    {
        au->get_pasyn_table()->signal();
    }
}

void DeviceProxy::read_attribute_asynch(const std::string &attr_name, CallBack &cb)
{
    std::vector<std::string> tmp_att_names(1, attr_name);
    read_attributes_asynch(tmp_att_names, cb);
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::write_attributes_asynch()
//
// description :     Write Tango device attributes asynchronously.
//            The client is not blocked until the attributes are written
//
// argin(s) :        attr_list : The attribute name(s) and value(s)
//            cb : The callback object
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attributes_asynch(const std::vector<DeviceAttribute> &attr_list, CallBack &cb)
{
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
    // Create the request object
    //

    CORBA::ORB::RequestSeq req_seq;
    req_seq.length(1);

    Tango::AttributeValueList att;
    Tango::AttributeValueList_4 att_4;

    long nb_attr = attr_list.size();
    if(version < 4)
    {
        att.length(nb_attr);
    }
    else
    {
        att_4.length(nb_attr);
    }

    for(int i = 0; i < nb_attr; i++)
    {
        if(version < 4)
        {
            ApiUtil::device_to_attr(attr_list[i], att[i], device_name);
        }
        else
        {
            ApiUtil::device_to_attr(attr_list[i], att_4[i]);
        }
    }

    if(version >= 4)
    {
        req_seq[0] = device_4->_request("write_attributes_4");
        req_seq[0]->add_in_arg() <<= att_4;
        req_seq[0]->add_in_arg() <<= get_client_identification();
        req_seq[0]->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else if(version == 3)
    {
        req_seq[0] = device->_request("write_attributes_3");
        req_seq[0]->add_in_arg() <<= att;
        req_seq[0]->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else
    {
        req_seq[0] = device->_request("write_attributes");
        req_seq[0]->add_in_arg() <<= att;
    }
    req_seq[0]->exceptions()->add(Tango::_tc_DevFailed);

    //
    // Send the request
    // and store it in the global asynchronous cb requests table
    //

    ApiUtil *au = ApiUtil::instance();
    add_asyn_cb_request(req_seq[0], &cb, this, TgRequest::WRITE_ATTR);
    CORBA::ORB_var orb = au->get_orb();
    orb->send_multiple_requests_deferred(req_seq);
    if(au->get_asynch_cb_sub_model() == PUSH_CALLBACK)
    {
        au->get_pasyn_table()->signal();
    }
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::write_attribute_asynch()
//
// description :     Write Tango device attribute asynchronously.
//            The client is not blocked until the attributes are written
//
//            WARNING : Obviously, this method could simply create
//            a vector of DeviceAttribute and call the previous
//            method (write_attributes_asynch (note the attributes))
//            using this vector. But the copy constructor of the
//            DeviceAttribute class, pass the memory to the newly
//            created DeviceAttribute object !!!!
//            Therefore, it is not possible to use this solution
//            in this case
//
// argin(s) :        attr_list : The attribute name(s) and value(s)
//            cb : The callback object
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attribute_asynch(const DeviceAttribute &attr, CallBack &cb)
{
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
    // Create the request object
    //

    CORBA::ORB::RequestSeq req_seq;
    req_seq.length(1);

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

    if(version >= 4)
    {
        req_seq[0] = device_4->_request("write_attributes_4");
        req_seq[0]->add_in_arg() <<= att_4;
        req_seq[0]->add_in_arg() <<= get_client_identification();
        req_seq[0]->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else if(version == 3)
    {
        req_seq[0] = device->_request("write_attributes_3");
        req_seq[0]->add_in_arg() <<= att;
        req_seq[0]->exceptions()->add(Tango::_tc_MultiDevFailed);
    }
    else
    {
        req_seq[0] = device->_request("write_attributes");
        req_seq[0]->add_in_arg() <<= att;
    }
    req_seq[0]->exceptions()->add(Tango::_tc_DevFailed);

    //
    // Send the request
    // and store it in the global asynchronous cb requests table
    //

    ApiUtil *au = ApiUtil::instance();
    add_asyn_cb_request(req_seq[0], &cb, this, TgRequest::WRITE_ATTR_SINGLE);
    CORBA::ORB_var orb = au->get_orb();
    orb->send_multiple_requests_deferred(req_seq);
    if(au->get_asynch_cb_sub_model() == PUSH_CALLBACK)
    {
        au->get_pasyn_table()->signal();
    }
}

} // namespace Tango
