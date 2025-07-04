//===================================================================================================================
//
// devapi_base.cpp     - C++ source code file for TANGO device api
//
// programmer(s)    - Andy Gotz (goetz@esrf.fr)
//
// original         - March 2001
//
// Copyright (C) :      2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//===================================================================================================================

#include <tango/client/eventconsumer.h>
#include <tango/client/Database.h>
#include <tango/client/DbDevice.h>
#include <tango/server/seqvec.h>
#include <tango/server/device.h>
#include <tango/internal/net.h>
#include <tango/internal/utils.h>

#ifdef _TG_WINDOWS_
  #include <process.h>
  #include <ws2tcpip.h>
#else
  #include <netdb.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <pwd.h>
#endif /* _TG_WINDOWS_ */

#include <ctime>
#include <csignal>
#include <algorithm>

#include <chrono>
#include <memory>

#include <tango/internal/telemetry/telemetry_kernel_macros.h>
#include <tango/common/pointer_with_lock.h>

using namespace CORBA;

namespace Tango
{

namespace
{
constexpr auto RECONNECTION_DELAY = std::chrono::seconds(1);

PointerWithLock<EventConsumer> get_event_system_for_event_id(int event_id)
{
    ApiUtil *au = ApiUtil::instance();

    if(EventConsumer::get_event_system_for_event_id(event_id) == ZMQ)
    {
        auto zmq_consumer = au->get_zmq_event_consumer();
        if(zmq_consumer == nullptr)
        {
            TangoSys_OMemStream desc;
            desc << "Could not find event consumer object, \n";
            desc << "probably no event subscription was done before!";
            desc << std::ends;
            TANGO_THROW_EXCEPTION(API_EventConsumer, desc.str());
        }

        return au->get_zmq_event_consumer();
    }

    auto notifd_consumer = au->get_notifd_event_consumer();
    if(notifd_consumer == nullptr)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << std::ends;
        TANGO_THROW_EXCEPTION(API_EventConsumer, desc.str());
    }

    return au->get_notifd_event_consumer();
}

template <class T, class U, std::enable_if_t<std::is_same_v<T, U>, T> * = nullptr>
CORBA::Any *create_any(const U *, size_t, size_t);
template <class T, class U, std::enable_if_t<!std::is_same_v<T, U>, T> * = nullptr>
CORBA::Any *create_any(const U *, size_t, size_t);

template <class T, class U, std::enable_if_t<std::is_same_v<T, U>, T> *>
CORBA::Any *create_any(const U *tmp, const size_t base, const size_t data_length)
{
    CORBA::Any *any_ptr = new CORBA::Any();

    const auto *c_seq_buff = tmp->get_buffer();
    auto *seq_buff = const_cast<std::remove_const_t<std::remove_pointer_t<decltype(c_seq_buff)>> *>(c_seq_buff);

    T tmp_data = T(data_length, data_length, &(seq_buff[base - data_length]), false);

    (*any_ptr) <<= tmp_data;

    return any_ptr;
}

template <class T, class U, std::enable_if_t<!std::is_same_v<T, U>, T> *>
CORBA::Any *create_any(const U *tmp, const size_t base, const size_t)
{
    CORBA::Any *any_ptr = new CORBA::Any();
    (*any_ptr) <<= (*tmp)[base - 1];
    return any_ptr;
}

template <>
CORBA::Any *create_any<DevBoolean>(const DevVarBooleanArray *tmp, const size_t base, const size_t)
{
    CORBA::Any *any_ptr = new CORBA::Any();
    (*any_ptr) <<= CORBA::Any::from_boolean((*tmp)[base - 1]);
    return any_ptr;
}

template <>
CORBA::Any *create_any<DevString>(const DevVarStringArray *tmp, const size_t base, const size_t)
{
    CORBA::Any *any_ptr = new CORBA::Any();
    Tango::ConstDevString tmp_data = (*tmp)[base - 1].in();
    (*any_ptr) <<= tmp_data;
    return any_ptr;
}

template <>
CORBA::Any *create_any<DevVarStringArray>(const DevVarStringArray *tmp, const size_t base, const size_t data_length)
{
    CORBA::Any *any_ptr = new CORBA::Any();
    const Tango::ConstDevString *c_seq_buff = tmp->get_buffer();
    char **seq_buff = const_cast<char **>(c_seq_buff);
    Tango::DevVarStringArray tmp_data =
        DevVarStringArray(data_length, data_length, &(seq_buff[base - data_length]), false);

    (*any_ptr) <<= tmp_data;
    return any_ptr;
}

template <class T>
void extract_value(CORBA::Any &value, std::vector<DeviceAttributeHistory> &ddh)
{
    const T *tmp;

    value >>= tmp;
    size_t seq_size = tmp->length();

    //
    // Copy data
    //

    size_t base = seq_size;

    for(auto &hist : ddh)
    {
        if(hist.failed() || hist.quality == Tango::ATTR_INVALID)
        {
            continue;
        }

        //
        // Get the data length for this record
        //

        int r_dim_x = hist.dim_x;
        int r_dim_y = hist.dim_y;
        int w_dim_x = hist.get_written_dim_x();
        int w_dim_y = hist.get_written_dim_y();

        int data_length;
        (r_dim_y == 0) ? data_length = r_dim_x : data_length = r_dim_x * r_dim_y;
        (w_dim_y == 0) ? data_length += w_dim_x : data_length += (w_dim_x * w_dim_y);

        //
        // Real copy now
        //

        hist.update_internal_sequence(tmp, base - data_length, data_length);

        base -= data_length;
    }
}

template <class T>
void extract_value(CORBA::Any &value, std::vector<DeviceDataHistory> &ddh, const Tango::AttributeDimList &ad)
{
    const typename tango_type_traits<T>::ArrayType *tmp;

    value >>= tmp;
    size_t seq_size = tmp->length();

    //
    // Copy data
    //

    size_t base = seq_size;

    size_t loop = 0;

    for(auto &hist : ddh)
    {
        //
        // Get the data length for this record
        //

        size_t data_length = ad[loop++].dim_x;

        if(hist.failed())
        {
            continue;
        }

        //
        // Real copy now
        //
        CORBA::Any *any_ptr = create_any<T>(tmp, base, data_length);

        hist.any = any_ptr;

        base -= data_length;
    }
}

template <>
void extract_value<Tango::DevVarDoubleStringArray>(CORBA::Any &value,
                                                   std::vector<DeviceDataHistory> &ddh,
                                                   const Tango::AttributeDimList &ad)
{
    const Tango::DevVarDoubleStringArray *tmp;

    value >>= tmp;
    size_t seq_size_str = tmp->svalue.length();
    size_t seq_size_num = tmp->dvalue.length();

    //
    // Copy data
    //

    size_t base_str = seq_size_str;
    size_t base_num = seq_size_num;

    size_t loop = 0;

    for(auto &hist : ddh)
    {
        if(hist.failed())
        {
            continue;
        }

        //
        // Get the data length for this record
        //

        size_t data_length = ad[loop].dim_x;
        size_t data_num_length = ad[loop].dim_y;
        ++loop;
        //
        // Real copy now
        //
        auto *dvdsa = new Tango::DevVarDoubleStringArray();
        dvdsa->svalue.length(data_length);
        dvdsa->dvalue.length(data_num_length);

        for(size_t i = 0; i < data_length; ++i)
        {
            dvdsa->svalue[i] = tmp->svalue[(base_str - data_length) + i];
        }
        for(size_t i = 0; i < data_num_length; ++i)
        {
            dvdsa->dvalue[i] = tmp->dvalue[(base_num - data_num_length) + i];
        }

        CORBA::Any *any_ptr = new CORBA::Any();
        (*any_ptr) <<= dvdsa;
        hist.any = any_ptr;

        base_str -= data_length;
        base_num -= data_num_length;
    }
}

template <>
void extract_value<Tango::DevVarLongStringArray>(CORBA::Any &value,
                                                 std::vector<DeviceDataHistory> &ddh,
                                                 const Tango::AttributeDimList &ad)
{
    const Tango::DevVarLongStringArray *tmp;

    value >>= tmp;
    size_t seq_size_str = tmp->svalue.length();
    size_t seq_size_num = tmp->lvalue.length();

    //
    // Copy data
    //

    size_t base_str = seq_size_str;
    size_t base_num = seq_size_num;

    size_t loop = 0;

    for(auto &hist : ddh)
    {
        if(hist.failed())
        {
            continue;
        }

        //
        // Get the data length for this record
        //

        size_t data_length = ad[loop].dim_x;
        size_t data_num_length = ad[loop].dim_y;
        ++loop;
        //
        // Real copy now
        //
        auto *dvdsa = new Tango::DevVarLongStringArray();
        dvdsa->svalue.length(data_length);
        dvdsa->lvalue.length(data_num_length);

        for(size_t i = 0; i < data_length; ++i)
        {
            dvdsa->svalue[i] = tmp->svalue[(base_str - data_length) + i];
        }
        for(size_t i = 0; i < data_num_length; ++i)
        {
            dvdsa->lvalue[i] = tmp->lvalue[(base_num - data_num_length) + i];
        }

        CORBA::Any *any_ptr = new CORBA::Any();
        (*any_ptr) <<= dvdsa;
        hist.any = any_ptr;

        base_str -= data_length;
        base_num -= data_num_length;
    }
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//         from_hist_2_AttHistory()
//
// description :
//         Convert the attribute history as returned by a IDL 4 device to the classical DeviceAttributeHistory format
//
//-------------------------------------------------------------------------------------------------------------------

template <typename T>
void from_hist_2_AttHistory(const T &hist, std::vector<DeviceAttributeHistory> *ddh)
{
    //
    // Check received data validity
    //

    if((hist->quals.length() != hist->quals_array.length()) || (hist->r_dims.length() != hist->r_dims_array.length()) ||
       (hist->w_dims.length() != hist->w_dims_array.length()) || (hist->errors.length() != hist->errors_array.length()))
    {
        TANGO_THROW_EXCEPTION(API_WrongHistoryDataBuffer, "Data buffer received from server is not valid !");
    }

    //
    // Get history depth
    //

    unsigned int h_depth = hist->dates.length();

    //
    // Copy date and name in each history list element
    //

    unsigned int loop;
    for(loop = 0; loop < h_depth; loop++)
    {
        (*ddh)[loop].time = hist->dates[loop];
        (*ddh)[loop].name = hist->name.in();
    }

    //
    // Copy the attribute quality factor
    //

    int k;

    for(loop = 0; loop < hist->quals.length(); loop++)
    {
        int nb_elt = hist->quals_array[loop].nb_elt;
        int start = hist->quals_array[loop].start;

        for(k = 0; k < nb_elt; k++)
        {
            (*ddh)[start - k].quality = hist->quals[loop];
        }
    }

    //
    // Copy read dimension
    //

    for(loop = 0; loop < hist->r_dims.length(); loop++)
    {
        int nb_elt = hist->r_dims_array[loop].nb_elt;
        int start = hist->r_dims_array[loop].start;

        for(k = 0; k < nb_elt; k++)
        {
            (*ddh)[start - k].dim_x = hist->r_dims[loop].dim_x;
            (*ddh)[start - k].dim_y = hist->r_dims[loop].dim_y;
        }
    }

    //
    // Copy write dimension
    //

    for(loop = 0; loop < hist->w_dims.length(); loop++)
    {
        int nb_elt = hist->w_dims_array[loop].nb_elt;
        int start = hist->w_dims_array[loop].start;

        for(k = 0; k < nb_elt; k++)
        {
            (*ddh)[start - k].set_w_dim_x(hist->w_dims[loop].dim_x);
            (*ddh)[start - k].set_w_dim_y(hist->w_dims[loop].dim_y);
        }
    }

    //
    // Copy errors
    //

    for(loop = 0; loop < hist->errors.length(); loop++)
    {
        int nb_elt = hist->errors_array[loop].nb_elt;
        int start = hist->errors_array[loop].start;

        for(k = 0; k < nb_elt; k++)
        {
            (*ddh)[start - k].failed(true);
            DevErrorList &err_list = (*ddh)[start - k].get_error_list();
            err_list.length(hist->errors[loop].length());
            for(unsigned int g = 0; g < hist->errors[loop].length(); g++)
            {
                err_list[g] = (hist->errors[loop])[g];
            }
        }
    }

    //
    // Get data type and data ptr
    //

    CORBA::TypeCode_var ty = hist->value.type();
    if(ty->kind() != tk_null)
    {
        CORBA::TypeCode_var ty_alias = ty->content_type();
        CORBA::TypeCode_var ty_seq = ty_alias->content_type();

        switch(ty_seq->kind())
        {
        case tk_long:
            extract_value<Tango::DevVarLongArray>(hist->value, *ddh);
            break;

        case tk_longlong:
            extract_value<Tango::DevVarLong64Array>(hist->value, *ddh);
            break;

        case tk_short:
            extract_value<Tango::DevVarShortArray>(hist->value, *ddh);
            break;

        case tk_double:
            extract_value<Tango::DevVarDoubleArray>(hist->value, *ddh);
            break;

        case tk_string:
            extract_value<Tango::DevVarStringArray>(hist->value, *ddh);
            break;

        case tk_float:
            extract_value<Tango::DevVarFloatArray>(hist->value, *ddh);
            break;

        case tk_boolean:
            extract_value<Tango::DevVarBooleanArray>(hist->value, *ddh);
            break;

        case tk_ushort:
            extract_value<Tango::DevVarUShortArray>(hist->value, *ddh);
            break;

        case tk_octet:
            extract_value<Tango::DevVarCharArray>(hist->value, *ddh);
            break;

        case tk_ulong:
            extract_value<Tango::DevVarULongArray>(hist->value, *ddh);
            break;

        case tk_ulonglong:
            extract_value<Tango::DevVarULong64Array>(hist->value, *ddh);
            break;

        case tk_enum:
            extract_value<Tango::DevVarStateArray>(hist->value, *ddh);
            break;

        case tk_struct:
            extract_value<Tango::DevVarEncodedArray>(hist->value, *ddh);
            break;

        default:
            TangoSys_OMemStream desc;
            desc << "'hist.value' with unexpected sequence kind '" << ty_seq->kind() << "'";
            TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
        }
    }
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//         from_hist4_2_DataHistory()
//
// description :
//         Convert the command history as returned by a IDL 4 device to the classical DeviceDataHistory format
//
//-------------------------------------------------------------------------------------------------------------------

void from_hist4_2_DataHistory(const Tango::DevCmdHistory_4_var &hist_4, std::vector<Tango::DeviceDataHistory> *ddh)
{
    //
    // Check received data validity
    //

    if((hist_4->dims.length() != hist_4->dims_array.length()) ||
       (hist_4->errors.length() != hist_4->errors_array.length()))
    {
        TANGO_THROW_EXCEPTION(API_WrongHistoryDataBuffer, "Data buffer received from server is not valid !");
    }

    //
    // Get history depth
    //

    unsigned int h_depth = hist_4->dates.length();

    //
    // Copy date in each history list element
    //

    unsigned int loop;
    int k;

    for(loop = 0; loop < h_depth; loop++)
    {
        (*ddh)[loop].set_date(hist_4->dates[loop]);
    }

    //
    // Copy errors
    //

    for(loop = 0; loop < hist_4->errors.length(); loop++)
    {
        int nb_elt = hist_4->errors_array[loop].nb_elt;
        int start = hist_4->errors_array[loop].start;

        for(k = 0; k < nb_elt; k++)
        {
            (*ddh)[start - k].failed(true);
            DevErrorList_var del(&hist_4->errors[loop]);
            (*ddh)[start - k].errors(del);
            del._retn();
        }
    }

    //
    // Create a temporary sequence with record dimension
    //

    Tango::AttributeDimList ad(h_depth);
    ad.length(h_depth);

    for(loop = 0; loop < hist_4->dims.length(); loop++)
    {
        int nb_elt = hist_4->dims_array[loop].nb_elt;
        int start = hist_4->dims_array[loop].start;

        for(k = 0; k < nb_elt; k++)
        {
            ad[start - k].dim_x = hist_4->dims[loop].dim_x;
            ad[start - k].dim_y = hist_4->dims[loop].dim_y;
        }
    }

    //
    // Get data ptr and data size
    //

    switch(hist_4->cmd_type)
    {
    case DEV_LONG:
        extract_value<Tango::DevLong>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_LONGARRAY:
        extract_value<Tango::DevVarLongArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_LONG64:
        extract_value<Tango::DevLong64>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_LONG64ARRAY:
        extract_value<Tango::DevVarLong64Array>(hist_4->value, *ddh, ad);
        break;
    case DEV_SHORT:
        extract_value<Tango::DevShort>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_SHORTARRAY:
        extract_value<Tango::DevVarShortArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_DOUBLE:
        extract_value<Tango::DevDouble>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_DOUBLEARRAY:
        extract_value<Tango::DevVarDoubleArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_STRING:
        extract_value<Tango::DevString>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_STRINGARRAY:
        extract_value<Tango::DevVarStringArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_FLOAT:
        extract_value<Tango::DevFloat>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_FLOATARRAY:
        extract_value<Tango::DevVarFloatArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_BOOLEAN:
        extract_value<Tango::DevBoolean>(hist_4->value, *ddh, ad);
        break;
    case DEV_USHORT:
        extract_value<Tango::DevUShort>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_USHORTARRAY:
        extract_value<Tango::DevVarUShortArray>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_CHARARRAY:
        extract_value<Tango::DevVarCharArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_ULONG:
        extract_value<Tango::DevULong>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_ULONGARRAY:
        extract_value<Tango::DevVarULongArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_ULONG64:
        extract_value<Tango::DevULong64>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_ULONG64ARRAY:
        extract_value<Tango::DevVarULong64Array>(hist_4->value, *ddh, ad);
        break;
    case DEV_STATE:
        extract_value<Tango::DevState>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_LONGSTRINGARRAY:
        extract_value<Tango::DevVarLongStringArray>(hist_4->value, *ddh, ad);
        break;
    case DEVVAR_DOUBLESTRINGARRAY:
        extract_value<Tango::DevVarDoubleStringArray>(hist_4->value, *ddh, ad);
        break;
    case DEV_ENCODED:
        extract_value<Tango::DevEncoded>(hist_4->value, *ddh, ad);
        break;
    }
}
} // namespace

//-----------------------------------------------------------------------------
//
// ConnectionExt class methods:
//
// - assignement operator
//
//-----------------------------------------------------------------------------

Connection::ConnectionExt &Connection::ConnectionExt::operator=(TANGO_UNUSED(const Connection::ConnectionExt &rval))
{
    return *this;
}

//-----------------------------------------------------------------------------
//
// Connection::Connection() - constructor to manage a connection to a device
//
//-----------------------------------------------------------------------------

Connection::Connection(CORBA::ORB_var orb_in) :
    pasyn_ctr(0),
    pasyn_cb_ctr(0),
    timeout(CLNT_TIMEOUT),
    connection_state(CONNECTION_NOTOK),
    version(detail::INVALID_IDL_VERSION),
    server_version(detail::INVALID_IDL_VERSION),
    source(Tango::CACHE_DEV),
    ext(new ConnectionExt()),
    tr_reco(true),
    user_connect_timeout(-1),
    tango_host_localhost(false)

{
    //
    // Some default init for access control
    //

    check_acc = true;
    access = ACCESS_READ;

    //
    // If the proxy is created from inside a device server, use the server orb
    //

    ApiUtil *au = ApiUtil::instance();
    if((CORBA::is_nil(orb_in)) && (au->is_orb_nil()))
    {
        if(au->in_server())
        {
            ApiUtil::instance()->set_orb(Util::instance()->get_orb());
        }
        else
        {
            ApiUtil::instance()->create_orb();
        }
    }
    else
    {
        if(!CORBA::is_nil(orb_in))
        {
            au->set_orb(orb_in);
        }
    }

    //
    // Get user connect timeout if one is defined
    //

    int ucto = au->get_user_connect_timeout();
    if(ucto != -1)
    {
        user_connect_timeout = ucto;
    }
}

Connection::Connection(bool dummy) :
    ext(nullptr),
    tr_reco(true),

    user_connect_timeout(-1),
    tango_host_localhost(false)
{
    if(dummy)
    {
        ext = std::make_unique<ConnectionExt>();
    }
}

//-----------------------------------------------------------------------------
//
// Connection::~Connection() - destructor to destroy connection to TANGO device
//
//-----------------------------------------------------------------------------

Connection::~Connection() { }

//-----------------------------------------------------------------------------
//
// Connection::Connection() - copy constructor
//
//-----------------------------------------------------------------------------

Connection::Connection(const Connection &sou) :
    ext(nullptr)
{
    dbase_used = sou.dbase_used;
    from_env_var = sou.from_env_var;
    host = sou.host;
    port = sou.port;
    port_num = sou.port_num;

    db_host = sou.db_host;
    db_port = sou.db_port;
    db_port_num = sou.db_port_num;

    ior = sou.ior;
    pasyn_ctr = sou.pasyn_ctr;
    pasyn_cb_ctr = sou.pasyn_cb_ctr;

    device = sou.device;
    if(sou.version >= 2)
    {
        device_2 = sou.device_2;
    }

    timeout = sou.timeout;
    connection_state = sou.connection_state;
    version = sou.version;
    server_version = sou.server_version;
    source = sou.source;

    check_acc = sou.check_acc;
    access = sou.access;

    tr_reco = sou.tr_reco;
    device_3 = sou.device_3;

    prev_failed_t0 = sou.prev_failed_t0;

    device_4 = sou.device_4;

    user_connect_timeout = sou.user_connect_timeout;
    tango_host_localhost = sou.tango_host_localhost;

    device_5 = sou.device_5;

    device_6 = sou.device_6;

    if(sou.ext != nullptr)
    {
        ext = std::make_unique<ConnectionExt>();
        *(ext) = *(sou.ext);
    }
}

//-----------------------------------------------------------------------------
//
// Connection::operator=() - assignement operator
//
//-----------------------------------------------------------------------------

Connection &Connection::operator=(const Connection &rval)
{
    if(this == &rval)
    {
        return *this;
    }

    dbase_used = rval.dbase_used;
    from_env_var = rval.from_env_var;
    host = rval.host;
    port = rval.port;
    port_num = rval.port_num;

    db_host = rval.db_host;
    db_port = rval.db_port;
    db_port_num = rval.db_port_num;

    ior = rval.ior;
    pasyn_ctr = rval.pasyn_ctr;
    pasyn_cb_ctr = rval.pasyn_cb_ctr;

    device = rval.device;
    if(rval.version >= 2)
    {
        device_2 = rval.device_2;
    }

    timeout = rval.timeout;
    connection_state = rval.connection_state;
    version = rval.version;
    server_version = rval.server_version;
    source = rval.source;

    check_acc = rval.check_acc;
    access = rval.access;

    tr_reco = rval.tr_reco;
    device_3 = rval.device_3;

    prev_failed_t0 = rval.prev_failed_t0;

    device_4 = rval.device_4;

    user_connect_timeout = rval.user_connect_timeout;
    tango_host_localhost = rval.tango_host_localhost;

    device_5 = rval.device_5;

    device_6 = rval.device_6;

    if(rval.ext != nullptr)
    {
        ext = std::make_unique<ConnectionExt>();
        *(ext) = *(rval.ext);
    }
    else
    {
        ext.reset(nullptr);
    }

    return *this;
}

//-----------------------------------------------------------------------------
//
// Connection::check_and_reconnect() methods family
//
// Check if a re-connection is needed and if true, try to reconnect.
// These meethods also manage a TangoMonitor object for thread safety.
// Some of them set parameters while the object is locked in order to pass
// them to the caller in thread safe way.
//
//-----------------------------------------------------------------------------

void Connection::check_and_reconnect()
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
    }
    if(local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if(connection_state != CONNECTION_OK)
        {
            reconnect(dbase_used);
        }
    }
}

void Connection::check_and_reconnect(Tango::DevSource &sou)
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
        sou = source;
    }
    if(local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if(connection_state != CONNECTION_OK)
        {
            reconnect(dbase_used);
        }
    }
}

void Connection::check_and_reconnect(Tango::AccessControlType &act)
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
        act = access;
    }
    if(local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if(connection_state != CONNECTION_OK)
        {
            reconnect(dbase_used);
            act = access;
        }
    }
}

void Connection::check_and_reconnect(Tango::DevSource &sou, Tango::AccessControlType &act)
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
        act = access;
        sou = source;
    }
    if(local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if(connection_state != CONNECTION_OK)
        {
            reconnect(dbase_used);
            act = access;
        }
    }
}

void Connection::set_connection_state(int con)
{
    WriterLock guard(con_to_mon);
    connection_state = con;
}

Tango::DevSource Connection::get_source()
{
    ReaderLock guard(con_to_mon);
    return source;
}

void Connection::set_source(Tango::DevSource sou)
{
    WriterLock guard(con_to_mon);
    source = sou;
}

//-----------------------------------------------------------------------------
//
// Connection::connect() - method to create connection to a TANGO device
//        using its stringified CORBA reference i.e. IOR or corbaloc
//
//-----------------------------------------------------------------------------

void Connection::connect(const std::string &corba_name)
{
    bool retry = true;
    long db_retries = DB_START_PHASE_RETRIES;
    bool connect_to_db = false;

    while(retry)
    {
        try
        {
            Object_var obj;
            obj = ApiUtil::instance()->get_orb()->string_to_object(corba_name.c_str());

            //
            // Narrow CORBA string name to CORBA object
            // First, try as a Device_5, then as a Device_4, then as .... and finally as a Device
            //
            // But we have want to know if the connection to the device is OK or not.
            // The _narrow() call does not necessary generates a remote call. It all depends on the object IDL type
            // stored in the IOR. If in the IOR, the IDL is the same than the one on which the narrow is done (Device_5
            // on both side for instance), then the _narrow call will not generate any remote call and therefore, we
            // don't know if the connection is OK or NOT. This is the reason of the _non_existent() call. In case the
            // IDl in the IOR and in the narrow() call are different, then the _narrow() call try to execute a remote
            // _is_a() call and therefore tries to connect to the device. IN this case, the _non_existent() call is
            // useless. But because we don want to analyse the IOR ourself, we always call _non_existent() Reset the
            // connection timeout only after the _non_existent call.
            //

            if(corba_name.find(DbObjName) != std::string::npos)
            {
                connect_to_db = true;
            }

            if(!connect_to_db)
            {
                if(user_connect_timeout != -1)
                {
                    omniORB::setClientConnectTimeout(user_connect_timeout);
                }
                else
                {
                    omniORB::setClientConnectTimeout(NARROW_CLNT_TIMEOUT);
                }
            }

            device_6 = Device_6::_narrow(obj);

            if(CORBA::is_nil(device_6))
            {
                device_5 = Device_5::_narrow(obj);

                if(CORBA::is_nil(device_5))
                {
                    device_4 = Device_4::_narrow(obj);

                    if(CORBA::is_nil(device_4))
                    {
                        device_3 = Device_3::_narrow(obj);

                        if(CORBA::is_nil(device_3))
                        {
                            device_2 = Device_2::_narrow(obj);
                            if(CORBA::is_nil(device_2))
                            {
                                device = Device::_narrow(obj);
                                if(CORBA::is_nil(device))
                                {
                                    std::cerr << "Can't build connection to object " << corba_name << std::endl;
                                    connection_state = CONNECTION_NOTOK;

                                    TangoSys_OMemStream desc;
                                    desc << "Failed to connect to device " << dev_name();
                                    desc << " (device nil after _narrowing)" << std::ends;
                                    TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_CantConnectToDevice, desc.str());
                                }
                                else
                                {
                                    device->_non_existent();
                                    version = 1;
                                }
                            }
                            else
                            {
                                device_2->_non_existent();
                                version = 2;
                                device = Device_2::_duplicate(device_2);
                            }
                        }
                        else
                        {
                            device_3->_non_existent();
                            version = 3;
                            device_2 = Device_3::_duplicate(device_3);
                            device = Device_3::_duplicate(device_3);
                        }
                    }
                    else
                    {
                        device_4->_non_existent();
                        version = 4;
                        device_3 = Device_4::_duplicate(device_4);
                        device_2 = Device_4::_duplicate(device_4);
                        device = Device_4::_duplicate(device_4);
                    }
                }
                else
                {
                    device_5->_non_existent();
                    version = 5;
                    device_4 = Device_5::_duplicate(device_5);
                    device_3 = Device_5::_duplicate(device_5);
                    device_2 = Device_5::_duplicate(device_5);
                    device = Device_5::_duplicate(device_5);
                }
            }
            else
            {
                device_6->_non_existent();
                version = 6;
                device_5 = Device_6::_duplicate(device_6);
                device_4 = Device_6::_duplicate(device_6);
                device_3 = Device_6::_duplicate(device_6);
                device_2 = Device_6::_duplicate(device_6);
                device = Device_6::_duplicate(device_6);
            }

            //
            // Warning! Some non standard code (omniORB specific).
            // Set a flag if the object is running on a host with several net addresses. This is used during
            // re-connection algo.
            //

            if(corba_name[0] == 'I' && corba_name[1] == 'O' && corba_name[2] == 'R')
            {
                IOP::IOR ior;
                toIOR(corba_name.c_str(), ior);
                IIOP::ProfileBody pBody;
                IIOP::unmarshalProfile(ior.profiles[0], pBody);

                CORBA::ULong total = pBody.components.length();

                for(CORBA::ULong index = 0; index < total; index++)
                {
                    IOP::TaggedComponent &c = pBody.components[index];
                    if(c.tag == 3)
                    {
                        ext->has_alt_adr = true;
                        break;
                    }
                    else
                    {
                        ext->has_alt_adr = false;
                    }
                }
            }

            //            if (connect_to_db == false)
            //                omniORB::setClientConnectTimeout(0);
            retry = false;

            //
            // Mark the connection as OK and set timeout to its value
            // (The default is 3 seconds)
            //

            connection_state = CONNECTION_OK;
            if(timeout != CLNT_TIMEOUT)
            {
                set_timeout_millis(timeout);
            }
        }
        catch(CORBA::SystemException &ce)
        {
            //            if (connect_to_db == false)
            //                omniORB::setClientConnectTimeout(0);

            TangoSys_OMemStream desc;
            TangoSys_MemStream reason;
            bool db_connect = false;

            desc << "Failed to connect to ";

            std::string::size_type pos = corba_name.find(':');
            if(pos == std::string::npos)
            {
                desc << "device " << dev_name() << std::ends;
                reason << API_CantConnectToDevice << std::ends;
            }
            else
            {
                std::string prot = corba_name.substr(0, pos);
                if(prot == "corbaloc")
                {
                    std::string::size_type tmp = corba_name.find('/');
                    if(tmp != std::string::npos)
                    {
                        std::string dev = corba_name.substr(tmp + 1);
                        if(dev == "database")
                        {
                            desc << "database on host ";
                            desc << db_host << " with port ";
                            desc << db_port << std::ends;
                            reason << API_CantConnectToDatabase << std::ends;
                            db_retries--;
                            if(db_retries != 0)
                            {
                                db_connect = true;
                            }
                        }
                        else
                        {
                            desc << "device " << dev_name() << std::ends;
                            if(CORBA::OBJECT_NOT_EXIST::_downcast(&ce) != nullptr)
                            {
                                reason << API_DeviceNotDefined << std::ends;
                            }
                            else if(CORBA::TRANSIENT::_downcast(&ce) != nullptr)
                            {
                                reason << API_ServerNotRunning << std::ends;
                            }
                            else
                            {
                                reason << API_CantConnectToDevice << std::ends;
                            }
                        }
                    }
                    else
                    {
                        desc << "device " << dev_name() << std::ends;
                        reason << API_CantConnectToDevice << std::ends;
                    }
                }
                else
                {
                    desc << "device " << dev_name() << std::ends;
                    reason << API_CantConnectToDevice << std::ends;
                }
            }

            if(!db_connect)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, ce, reason.str(), desc.str());
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Connection::toIOR() - Convert string IOR to omniORB IOR object
// omniORB specific code !!!
//
//-----------------------------------------------------------------------------

void Connection::toIOR(const char *iorstr, IOP::IOR &ior)
{
    size_t s = (iorstr != nullptr ? strlen(iorstr) : 0);
    if(s < 4)
    {
        throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);
    }
    const char *p = iorstr;
    if(p[0] != 'I' || p[1] != 'O' || p[2] != 'R' || p[3] != ':')
    {
        throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);
    }

    s = (s - 4) / 2; // how many octets are there in the string
    p += 4;

    cdrMemoryStream buf((CORBA::ULong) s, false);

    for(int i = 0; i < (int) s; i++)
    {
        int j = i * 2;
        CORBA::Octet v;

        if(p[j] >= '0' && p[j] <= '9')
        {
            v = ((p[j] - '0') << 4);
        }
        else if(p[j] >= 'a' && p[j] <= 'f')
        {
            v = ((p[j] - 'a' + 10) << 4);
        }
        else if(p[j] >= 'A' && p[j] <= 'F')
        {
            v = ((p[j] - 'A' + 10) << 4);
        }
        else
        {
            throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);
        }

        if(p[j + 1] >= '0' && p[j + 1] <= '9')
        {
            v += (p[j + 1] - '0');
        }
        else if(p[j + 1] >= 'a' && p[j + 1] <= 'f')
        {
            v += (p[j + 1] - 'a' + 10);
        }
        else if(p[j + 1] >= 'A' && p[j + 1] <= 'F')
        {
            v += (p[j + 1] - 'A' + 10);
        }
        else
        {
            throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);
        }

        buf.marshalOctet(v);
    }

    buf.rewindInputPtr();
    CORBA::Boolean b = buf.unmarshalBoolean();
    buf.setByteSwapFlag(b);

    ior.type_id = IOP::IOR::unmarshaltype_id(buf);
    ior.profiles <<= buf;
}

//-----------------------------------------------------------------------------
//
// Connection::reconnect() - reconnect to a CORBA object
//
//-----------------------------------------------------------------------------

void Connection::reconnect(bool db_used)
{
    auto now = std::chrono::steady_clock::now();

    if(connection_state != CONNECTION_OK)
    {
        // Do not reconnect if to soon
        if(prev_failed_t0.has_value() && (now - *prev_failed_t0) < RECONNECTION_DELAY)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to connect to device " << dev_name() << std::endl;
            desc << "The connection request was delayed." << std::endl;
            desc << "The last connection request was done less than "
                 << std::chrono::milliseconds(RECONNECTION_DELAY).count() << " ms ago" << std::ends;

            TANGO_THROW_EXCEPTION(API_CantConnectToDevice, desc.str());
        }
    }

    try
    {
        std::string corba_name;
        if(connection_state != CONNECTION_OK)
        {
            if(db_used)
            {
                corba_name = get_corba_name(check_acc);
                if(!check_acc)
                {
                    ApiUtil *au = ApiUtil::instance();
                    int db_num;
                    if(get_from_env_var())
                    {
                        db_num = au->get_db_ind();
                    }
                    else
                    {
                        db_num = au->get_db_ind(get_db_host(), get_db_port_num());
                    }
                    (au->get_db_vect())[db_num]->clear_access_except_errors();
                }
            }
            else
            {
                corba_name = build_corba_name();
            }

            connect(corba_name);
        }

        //
        // Try to ping the device. With omniORB, it is possible that the first
        // real access to the device is done when a call to one of the interface
        // operation is done. Do it now.
        //

        if(connection_state == CONNECTION_OK)
        {
            try
            {
                //                if (user_connect_timeout != -1)
                //                    omniORB::setClientConnectTimeout(user_connect_timeout);
                //                else
                //                    omniORB::setClientConnectTimeout(NARROW_CLNT_TIMEOUT);

                //
                // Impl. change for Tango-10 + IDLv6 - see cppTango #1193
                //
                // we now call 'info' instead of 'ping' so that we can obtain the so called
                // 'server_version' - i.e., the ultimate version supported by the server in
                // which the device we are connected to is running.
                //
                // in some particular cases, this allows us to offer new features to devices
                // simply recompiled against the latest version of tango without modifing
                // their inheritance schema.
                //
                // the telemetry service is an example of such a case - devices inheriting
                // from Device_4Impl or Device5_impl will be able to propagate the trace
                // context as far as they are running within a server where idl 6 or above is
                // supported.
                //
                std::unique_ptr<Tango::DevInfo> device_info(device->info());
                server_version = device_info->server_version;

                //                omniORB::setClientConnectTimeout(0);

                prev_failed_t0 = std::nullopt;

                //
                // If the device is the database, call its post-reconnection method
                //
                // TODO: Implement this with a virtual method in Connection and Database
                // class. Doing it now, will break compatibility (one more virtual method)
                //

                if(corba_name.find("database") != std::string::npos)
                {
                    static_cast<Database *>(this)->post_reconnection();
                }
            }
            catch(CORBA::SystemException &ce)
            {
                //                omniORB::setClientConnectTimeout(0);
                connection_state = CONNECTION_NOTOK;

                TangoSys_OMemStream desc;
                desc << "Failed to connect to device " << dev_name() << std::ends;

                TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, ce, API_CantConnectToDevice, desc.str());
            }
        }
    }
    catch(DevFailed &)
    {
        prev_failed_t0 = now;

        throw;
    }
}

//+----------------------------------------------------------------------------------------------------------------
//
// function:
//
// 		ClntIdent Connection::get_client_identification() const
//
// description:
//
//      Returns a ClntIdent initialized according to the IDL version of the device (peer) and the ultimate
//      IDL version supported by the server in which the peer is running.
//
//      Added for the telemetry service introduced in IDLv6.
//
//      Any device with IDL version >= 4 running in a server in which the ultimate version of the IDL is >= 6,
//      will benefit from the telemetry features offered at the kernel level (low level profiling). This
//      will limit the discontinuity in the tracing information.
//
//      See ClntIdent in tango.idl v6 for details.
//
//-----------------------------------------------------------------------------------------------------------------
ClntIdent Connection::get_client_identification() const
{
    // the client identification struct to be returned
    ClntIdent ci;

    // the pid of the cpp server (acting as a client) or the pure cpp client within which this code is executed
    TangoSys_Pid pid = ApiUtil::instance()->get_client_pid();

    if(version >= 4 && server_version >= 6)
    {
        // IDLv6 case
        CppClntIdent_6 ci_v6;
        ci_v6.cpp_clnt = pid;
        auto trace_context{W3CTraceContextV0()};
#if defined(TANGO_USE_TELEMETRY)
        // populate W3C headers of the IDLv6 data structure for trace context propagation
        std::string trace_parent;
        std::string trace_state;
        Tango::telemetry::Interface::get_trace_context(trace_parent, trace_state);
        trace_context.trace_parent = Tango::string_dup(trace_parent.c_str());
        trace_context.trace_state = Tango::string_dup(trace_state.c_str());
#endif
        ci_v6.trace_context.data(trace_context);
        ci.cpp_clnt_6(ci_v6);
    }
    else
    {
        // pre-IDLv6 case (the pid is the only info set for a cpp client)
        ci.cpp_clnt(pid);
    }

    return ci;
}

//-----------------------------------------------------------------------------
//
// Connection::is_connected() - returns true if connection is in the OK state
//
//-----------------------------------------------------------------------------

bool Connection::is_connected()
{
    bool connected = true;
    ReaderLock guard(con_to_mon);
    if(connection_state != CONNECTION_OK)
    {
        connected = false;
    }
    return connected;
}

//-----------------------------------------------------------------------------
//
// Connection::get_env_var() - Get an environment variable
//
// This method get an environment variable value from different source.
// which are (orderd by piority)
//
// 1 - A real environement variable
// 2 - A file ".tangorc" in the user home directory
// 3 - A file "/etc/tangorc"
//
// in :    - env_var_name : The environment variable name
//
// out : - env_var : The string initialised with the env. variable value
//
// This method returns 0 of the env. variable is found. Otherwise, it returns -1
//
//-----------------------------------------------------------------------------

int Connection::get_env_var(const char *env_var_name, std::string &env_var)
{
    int ret = -1;
    char *env_c_str;

    //
    // try to get it as a classical env. variable
    //

    env_c_str = getenv(env_var_name);

    if(env_c_str == nullptr)
    {
#ifndef _TG_WINDOWS_
        uid_t user_id = geteuid();

        struct passwd pw;
        struct passwd *pw_ptr;
        char buffer[1024];

        if(getpwuid_r(user_id, &pw, buffer, sizeof(buffer), &pw_ptr) != 0)
        {
            return ret;
        }

        if(pw_ptr == nullptr)
        {
            return ret;
        }

        //
        // Try to get it from the user home dir file
        //

        std::string home_file(pw.pw_dir);
        home_file = home_file + "/" + USER_ENV_VAR_FILE;

        int local_ret;
        std::string local_env_var;
        local_ret = get_env_var_from_file(home_file, env_var_name, local_env_var);

        if(local_ret == 0)
        {
            env_var = local_env_var;
            ret = 0;
        }
        else
        {
            //
            // Try to get it from a host defined file
            //

            home_file = TANGO_RC_FILE;
            local_ret = get_env_var_from_file(home_file, env_var_name, local_env_var);
            if(local_ret == 0)
            {
                env_var = local_env_var;
                ret = 0;
            }
        }
#else
        char *env_tango_root;

        env_tango_root = getenv(WindowsEnvVariable);
        if(env_tango_root != nullptr)
        {
            std::string home_file(env_tango_root);
            home_file = home_file + "/" + WINDOWS_ENV_VAR_FILE;

            int local_ret;
            std::string local_env_var;
            local_ret = get_env_var_from_file(home_file, env_var_name, local_env_var);

            if(local_ret == 0)
            {
                env_var = local_env_var;
                ret = 0;
            }
        }

#endif
    }
    else
    {
        env_var = env_c_str;
        ret = 0;
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// Connection::get_env_var_from_file() - Get an environment variable from a file
//
// in :    - env_var : The environment variable name
//        - f_name : The file name
//
// out : - ret_env_var : The string initialised with the env. variable value
//
// This method returns 0 of the env. variable is found. Otherwise, it returns -1
//
//-----------------------------------------------------------------------------

int Connection::get_env_var_from_file(const std::string &f_name, const char *env_var, std::string &ret_env_var)
{
    std::ifstream inFile;
    std::string file_line;
    std::string var(env_var);
    int ret = -1;

    inFile.open(f_name.c_str());
    if(!inFile)
    {
        return ret;
    }

    std::transform(var.begin(), var.end(), var.begin(), ::tolower);

    std::string::size_type pos_env, pos_comment;

    while(!inFile.eof())
    {
        getline(inFile, file_line);
        std::transform(file_line.begin(), file_line.end(), file_line.begin(), ::tolower);

        if((pos_env = file_line.find(var)) != std::string::npos)
        {
            pos_comment = file_line.find('#');
            if((pos_comment != std::string::npos) && (pos_comment < pos_env))
            {
                continue;
            }

            std::string::size_type pos;
            if((pos = file_line.find('=')) != std::string::npos)
            {
                std::string tg_host = file_line.substr(pos + 1);
                std::string::iterator end_pos = remove(tg_host.begin(), tg_host.end(), ' ');
                tg_host.erase(end_pos, tg_host.end());

                ret_env_var = tg_host;
                ret = 0;
                break;
            }
        }
    }

    inFile.close();
    return ret;
}

//-------------------------------------------------------------------------------------------------------------------
//
// method:
//        Connection::get_fqdn()
//
// description:
//         This method gets the host fully qualified domain name (from DNS) and modified the passed string accordingly
//
// argument:
//         in/out :
//            - the_host: The original host name
//
//------------------------------------------------------------------------------------------------------------------

void Connection::get_fqdn(std::string &the_host)
{
    //
    // If the host name we received is the name of the host we are running on,
    // set a flag
    //

    char buffer[Tango::detail::TANGO_MAX_HOSTNAME_LEN];
    bool local_host = false;

    if(gethostname(buffer, Tango::detail::TANGO_MAX_HOSTNAME_LEN) == 0)
    {
        if(::strcmp(buffer, the_host.c_str()) == 0)
        {
            local_host = true;
        }
    }

    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *info;
    struct addrinfo *ptr;
    char tmp_host[512];
    bool host_found = false;
    std::vector<std::string> ip_list;

    //
    // If we are running on local host, get IP address(es) from NIC board
    //

    if(local_host)
    {
        ApiUtil *au = ApiUtil::instance();
        au->get_ip_from_if(ip_list);
        hints.ai_flags |= AI_NUMERICHOST;
    }
    else
    {
        ip_list.push_back(the_host);
    }

    //
    // Try to get FQDN
    //

    size_t i;
    for(i = 0; i < ip_list.size() && !host_found; i++)
    {
        int result = getaddrinfo(ip_list[i].c_str(), nullptr, &hints, &info);

        if(result == 0)
        {
            ptr = info;
            int nb_loop = 0;
            std::string myhost;
            std::string::size_type pos;

            while(ptr != nullptr)
            {
                if(getnameinfo(ptr->ai_addr, ptr->ai_addrlen, tmp_host, 512, nullptr, 0, NI_NAMEREQD) == 0)
                {
                    nb_loop++;
                    myhost = tmp_host;
                    pos = myhost.find('.');
                    if(pos != std::string::npos)
                    {
                        std::string canon = myhost.substr(0, pos);
                        if(canon == the_host)
                        {
                            the_host = myhost;
                            host_found = true;
                            break;
                        }
                    }
                }
                ptr = ptr->ai_next;
            }
            freeaddrinfo(info);

            if(!host_found && nb_loop == 1 && i == (ip_list.size() - 1))
            {
                the_host = myhost;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Connection::get_timeout_millis() - public method to get timeout on a TANGO device
//
//-----------------------------------------------------------------------------

int Connection::get_timeout_millis()
{
    ReaderLock guard(con_to_mon);
    return timeout;
}

//-----------------------------------------------------------------------------
//
// Connection::set_timeout_millis() - public method to set timeout on a TANGO device
//
//-----------------------------------------------------------------------------

void Connection::set_timeout_millis(int millisecs)
{
    WriterLock guard(con_to_mon);

    timeout = millisecs;

    try
    {
        if(connection_state != CONNECTION_OK)
        {
            reconnect(dbase_used);
        }

        omniORB::setClientCallTimeout(device, millisecs);
    }
    catch(Tango::DevFailed &)
    {
    }
}

//-----------------------------------------------------------------------------
//
// Connection::command_inout() - public method to execute a command on a TANGO device
//
//-----------------------------------------------------------------------------

DeviceData Connection::command_inout(const std::string &command)
{
    DeviceData data_in;

    return (command_inout(command, data_in));
}

//-----------------------------------------------------------------------------
//
// Connection::command_inout() - public method to execute a command on a TANGO device
//
//-----------------------------------------------------------------------------

DeviceData Connection::command_inout(const std::string &command, const DeviceData &data_in)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}, {"tango.operation.argument", command}}));

    //
    // We are using a pointer to an Any as the return value of the command_inout
    // call. This is because the assignament to the Any_var any in the
    // DeviceData object in faster in this case (no copy).
    // Don't forget that the any_var in the DeviceData takes ownership of the
    // memory allocated
    //

    DeviceData data_out;
    int ctr = 0;
    DevSource local_source;
    AccessControlType local_act;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source, local_act);

            //
            // Manage control access in case the access right
            // is READ_ONLY. We need to check if the command is a
            // "READ" command or not
            //

            if(local_act == ACCESS_READ)
            {
                ApiUtil *au = ApiUtil::instance();

                std::vector<Database *> &v_d = au->get_db_vect();
                Database *db;
                if(v_d.empty())
                {
                    db = static_cast<Database *>(this);
                }
                else
                {
                    int db_num;

                    if(get_from_env_var())
                    {
                        db_num = au->get_db_ind();
                    }
                    else
                    {
                        db_num = au->get_db_ind(get_db_host(), get_db_port_num());
                    }
                    db = v_d[db_num];
                    /*                    if (db->is_control_access_checked() == false)
                                            db = static_cast<Database *>(this);*/
                }

                //
                // If the command is not allowed, throw exception
                // Also throw exception if it was not possible to get the list
                // of allowed commands from the control access service
                //
                // The ping rule is simply to send to the client correct
                // error message in case of re-connection
                //

                std::string d_name = dev_name();

                if(!db->is_command_allowed(d_name, command))
                {
                    try
                    {
                        Device_var dev = Device::_duplicate(device);
                        dev->ping();
                    }
                    catch(...)
                    {
                        set_connection_state(CONNECTION_NOTOK);
                        throw;
                    }

                    DevErrorList &e = db->get_access_except_errors();
                    /*                    if (e.length() != 0)
                                        {
                                            DevFailed df(e);
                                            throw df;
                                        }*/

                    TangoSys_OMemStream desc;
                    if(e.length() == 0)
                    {
                        desc << "Command " << command << " on device " << dev_name() << " is not authorized"
                             << std::ends;
                    }
                    else
                    {
                        desc << "Command " << command << " on device " << dev_name()
                             << " is not authorized because an error occurs while talking to the Controlled Access "
                                "Service"
                             << std::ends;
                        std::string ex(e[0].desc);
                        if(ex.find("defined") != std::string::npos)
                        {
                            desc << "\n" << ex;
                        }
                        desc << std::ends;
                    }

                    TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
                }
            }

            //
            // Now, try to execute the command
            //

            CORBA::Any *received;
            if(version >= 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                received =
                    dev->command_inout_4(command.c_str(), data_in.any, local_source, get_client_identification());
            }
            else if(version >= 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                received = dev->command_inout_2(command.c_str(), data_in.any, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                received = dev->command_inout(command.c_str(), data_in.any);
            }

            ctr = 2;
            data_out.any = received;
        }
        catch(Tango::ConnectionFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_CommandFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_CommandFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT_CMD(trans);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(one);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(comm);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return data_out;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// Connection::command_inout() - public method to execute a command on a TANGO device
//                 using low level CORBA types
//
//-----------------------------------------------------------------------------

CORBA::Any_var Connection::command_inout(const std::string &command, const CORBA::Any &any)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}, {"tango.operation.argument", command}}));

    int ctr = 0;
    Tango::DevSource local_source;
    Tango::AccessControlType local_act;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source, local_act);

            //
            // Manage control access in case the access right
            // is READ_ONLY. We need to check if the command is a
            // "READ" command or not
            //

            if(local_act == ACCESS_READ)
            {
                ApiUtil *au = ApiUtil::instance();

                std::vector<Database *> &v_d = au->get_db_vect();
                Database *db;
                if(v_d.empty())
                {
                    db = static_cast<Database *>(this);
                }
                else
                {
                    int db_num;

                    if(get_from_env_var())
                    {
                        db_num = au->get_db_ind();
                    }
                    else
                    {
                        db_num = au->get_db_ind(get_db_host(), get_db_port_num());
                    }
                    db = v_d[db_num];
                    /*                    if (db->is_control_access_checked() == false)
                                            db = static_cast<Database *>(this);*/
                }

                //
                // If the command is not allowed, throw exception
                // Also throw exception if it was not possible to get the list
                // of allowed commands from the control access service
                //
                // The ping rule is simply to send to the client correct
                // error message in case of re-connection
                //

                std::string d_name = dev_name();
                if(!db->is_command_allowed(d_name, command))
                {
                    try
                    {
                        Device_var dev = Device::_duplicate(device);
                        dev->ping();
                    }
                    catch(...)
                    {
                        set_connection_state(CONNECTION_NOTOK);
                        throw;
                    }

                    DevErrorList &e = db->get_access_except_errors();
                    /*                    if (e.length() != 0)
                                        {
                                            DevFailed df(e);
                                            throw df;
                                        }*/

                    TangoSys_OMemStream desc;
                    if(e.length() == 0)
                    {
                        desc << "Command " << command << " on device " << dev_name() << " is not authorized"
                             << std::ends;
                    }
                    else
                    {
                        desc << "Command " << command << " on device " << dev_name()
                             << " is not authorized because an error occurs while talking to the Controlled Access "
                                "Service"
                             << std::ends;
                        std::string ex(e[0].desc);
                        if(ex.find("defined") != std::string::npos)
                        {
                            desc << "\n" << ex;
                        }
                        desc << std::ends;
                    }

                    TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
                }
            }

            if(version >= 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                return (dev->command_inout_4(command.c_str(), any, local_source, get_client_identification()));
            }
            else if(version >= 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                return (dev->command_inout_2(command.c_str(), any, local_source));
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                return (dev->command_inout(command.c_str(), any));
            }
            ctr = 2;
        }
        catch(Tango::ConnectionFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_CommandFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_CommandFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT_CMD(trans);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(one);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(comm);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    //
    // Just to make VC++ quiet (will never reach this code !)
    //

    CORBA::Any_var tmp;
    return tmp;

    TANGO_TELEMETRY_TRACE_END();
}

long Connection::add_asyn_request(CORBA::Request_ptr req, TgRequest::ReqType req_type)
{
    omni_mutex_lock guard(asyn_mutex);
    long id = ApiUtil::instance()->get_pasyn_table()->store_request(req, req_type);
    pasyn_ctr++;
    return id;
}

void Connection::remove_asyn_request(long id)
{
    omni_mutex_lock guard(asyn_mutex);

    ApiUtil::instance()->get_pasyn_table()->remove_request(id);
    pasyn_ctr--;
}

void Connection::add_asyn_cb_request(CORBA::Request_ptr req, CallBack *cb, Connection *con, TgRequest::ReqType req_type)
{
    omni_mutex_lock guard(asyn_mutex);
    ApiUtil::instance()->get_pasyn_table()->store_request(req, cb, con, req_type);
    pasyn_cb_ctr++;
}

void Connection::remove_asyn_cb_request(Connection *con, CORBA::Request_ptr req)
{
    omni_mutex_lock guard(asyn_mutex);
    ApiUtil::instance()->get_pasyn_table()->remove_request(con, req);
    pasyn_cb_ctr--;
}

long Connection::get_pasyn_cb_ctr()
{
    long ret;
    asyn_mutex.lock();
    ret = pasyn_cb_ctr;
    asyn_mutex.unlock();
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::DeviceProxy() - constructor for device proxy object
//
//-----------------------------------------------------------------------------

DeviceProxy::DeviceProxy(const std::string &name, CORBA::ORB_var orb) :
    Connection(orb),
    db_dev(nullptr),
    is_alias(false),
    adm_device(nullptr),
    lock_ctr(0),
    ext_proxy(new DeviceProxyExt())
{
    real_constructor(name, true);
}

DeviceProxy::DeviceProxy(const char *na, CORBA::ORB_var orb) :
    Connection(orb),
    db_dev(nullptr),
    is_alias(false),
    adm_device(nullptr),
    lock_ctr(0),
    ext_proxy(new DeviceProxyExt())
{
    std::string name(na);
    real_constructor(name, true);
}

DeviceProxy::DeviceProxy(const std::string &name, bool need_check_acc, CORBA::ORB_var orb) :
    Connection(orb),
    db_dev(nullptr),
    is_alias(false),
    adm_device(nullptr),
    lock_ctr(0),
    ext_proxy(new DeviceProxyExt())
{
    real_constructor(name, need_check_acc);
}

DeviceProxy::DeviceProxy(const char *na, bool need_check_acc, CORBA::ORB_var orb) :
    Connection(orb),
    db_dev(nullptr),
    is_alias(false),
    adm_device(nullptr),
    lock_ctr(0),
    ext_proxy(new DeviceProxyExt())
{
    std::string name(na);
    real_constructor(name, need_check_acc);
}

void DeviceProxy::real_constructor(const std::string &name, bool need_check_acc)
{
#if defined(TANGO_USE_TELEMETRY)
    // start a 'client' span and create a scope so that the RPCs related to the construction of
    // the device proxy will traced under a specific scope and will enhance readability on backend
    // side - we use the current telemetry interface or the default one if none.
    // by default, the traces generated in this scope are silently ignored (see Tango::telemetry::SilentKernelScope)
    auto silent_kernel_scope = TANGO_TELEMETRY_SILENT_KERNEL_SCOPE;
    auto span = TANGO_TELEMETRY_SPAN("Tango::DeviceProxy::DeviceProxy", {{"tango.operation.argument", name}});
    auto scope = TANGO_TELEMETRY_SCOPE(span);
#endif

    //
    // Parse device name
    //

    parse_name(name);
    std::string corba_name;
    bool exported = true;

    if(dbase_used)
    {
        try
        {
            if(from_env_var)
            {
                ApiUtil *ui = ApiUtil::instance();
                db_dev = new DbDevice(device_name);
                int ind = ui->get_db_ind();
                db_host = (ui->get_db_vect())[ind]->get_db_host();
                db_port = (ui->get_db_vect())[ind]->get_db_port();
                db_port_num = (ui->get_db_vect())[ind]->get_db_port_num();
            }
            else
            {
                db_dev = new DbDevice(device_name, db_host, db_port);
                if(ext_proxy->nethost_alias)
                {
                    Database *tmp_db = db_dev->get_dbase();
                    const std::string &orig = tmp_db->get_orig_tango_host();
                    if(orig.empty())
                    {
                        std::string orig_tg_host = ext_proxy->orig_tango_host;
                        if(orig_tg_host.find('.') == std::string::npos)
                        {
                            get_fqdn(orig_tg_host);
                        }
                        tmp_db->set_orig_tango_host(ext_proxy->orig_tango_host);
                    }
                }
            }
        }
        catch(Tango::DevFailed &e)
        {
            if(strcmp(e.errors[0].reason.in(), API_TangoHostNotSet) == 0)
            {
                std::cerr << e.errors[0].desc.in() << std::endl;
            }
            throw;
        }

        try
        {
            corba_name = get_corba_name(need_check_acc);
        }
        catch(Tango::DevFailed &dfe)
        {
            if(strcmp(dfe.errors[0].reason, DB_DeviceNotDefined) == 0)
            {
                delete db_dev;
                TangoSys_OMemStream desc;
                desc << "Can't connect to device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, dfe, API_DeviceNotDefined, desc.str());
            }
            else if(strcmp(dfe.errors[0].reason, API_DeviceNotExported) == 0)
            {
                exported = false;
            }
        }
    }
    else
    {
        //
        // If we are not using the database, give write access
        //

        access = ACCESS_WRITE;
    }

    //
    // Implement stateless new() i.e. even if connect fails continue
    // If the DeviceProxy was created using device alias, ask for the real
    // device name.
    //

    try
    {
        if(exported)
        {
            // we now use reconnect instead of connect
            // it allows us to know more about the device we are talking to
            // see Connection::reconnect for details
            reconnect(dbase_used);

            if(is_alias)
            {
                CORBA::String_var real_name = device->name();
                device_name = real_name.in();
                std::transform(device_name.begin(), device_name.end(), device_name.begin(), ::tolower);
                db_dev->set_name(device_name);
            }
        }
    }
    catch(Tango::ConnectionFailed &dfe)
    {
        set_connection_state(CONNECTION_NOTOK);
        if(!dbase_used)
        {
            if(strcmp(dfe.errors[1].reason, API_DeviceNotDefined) == 0)
            {
                throw;
            }
        }
    }
    catch(CORBA::SystemException &)
    {
        set_connection_state(CONNECTION_NOTOK);
        if(!dbase_used)
        {
            throw;
        }
    }

    //
    // get the name of the asscociated device when connecting
    // inside a device server
    //

    try
    {
        ApiUtil *ui = ApiUtil::instance();
        if(ui->in_server())
        {
            Tango::Util *tg = Tango::Util::instance(false);
            tg->get_sub_dev_diag().register_sub_device(tg->get_sub_dev_diag().get_associated_device(), name);
        }
    }
    catch(Tango::DevFailed &e)
    {
        if(::strcmp(e.errors[0].reason.in(), API_UtilSingletonNotCreated) != 0)
        {
            throw;
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::DeviceProxy() - copy constructor
//
//-----------------------------------------------------------------------------

DeviceProxy::DeviceProxy(const DeviceProxy &sou) :
    Connection(sou),
    adm_device(nullptr),
    ext_proxy(nullptr)
{
    //
    // Copy DeviceProxy members
    //

    device_name = sou.device_name;
    alias_name = sou.alias_name;
    is_alias = sou.is_alias;
    adm_dev_name = sou.adm_dev_name;
    lock_ctr = sou.lock_ctr;

    if(dbase_used)
    {
        if(from_env_var)
        {
            ApiUtil *ui = ApiUtil::instance();
            if(ui->in_server())
            {
                db_dev = new DbDevice(device_name, Tango::Util::instance()->get_database());
            }
            else
            {
                db_dev = new DbDevice(device_name);
            }
        }
        else
        {
            db_dev = new DbDevice(device_name, db_host, db_port);
        }
    }

    //
    // Copy extension class
    //

    if(sou.ext_proxy != nullptr)
    {
        ext_proxy = std::make_unique<DeviceProxyExt>();
        *(ext_proxy) = *(sou.ext_proxy);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::DeviceProxy() - assignement operator
//
//-----------------------------------------------------------------------------

DeviceProxy &DeviceProxy::operator=(const DeviceProxy &rval)
{
    if(this != &rval)
    {
        this->Connection::operator=(rval);

        //
        // Now DeviceProxy members
        //

        device_name = rval.device_name;
        alias_name = rval.alias_name;
        is_alias = rval.is_alias;
        adm_dev_name = rval.adm_dev_name;
        lock_ctr = rval.lock_ctr;
        lock_valid = rval.lock_valid;

        delete db_dev;
        if(dbase_used)
        {
            if(from_env_var)
            {
                ApiUtil *ui = ApiUtil::instance();
                if(ui->in_server())
                {
                    db_dev = new DbDevice(device_name, Tango::Util::instance()->get_database());
                }
                else
                {
                    db_dev = new DbDevice(device_name);
                }
            }
            else
            {
                db_dev = new DbDevice(device_name, db_host, db_port);
            }
        }

        adm_device.reset(nullptr);

        if(rval.ext_proxy != nullptr)
        {
            ext_proxy = std::make_unique<DeviceProxyExt>();
            *(ext_proxy) = *(rval.ext_proxy);
        }
        else
        {
            ext_proxy.reset();
        }
    }

    return *this;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::parse_name() - Parse device name according to Tango device
//                   name syntax
//
// in :    - full_name : The device name
//
//-----------------------------------------------------------------------------

void DeviceProxy::parse_name(const std::string &full_name)
{
    std::string name_wo_prot;
    std::string name_wo_db_mod;
    std::string object_name;

    //
    // Error of the string is empty
    //

    if(full_name.empty())
    {
        TangoSys_OMemStream desc;
        desc << "The given name is an empty string!!! " << full_name << std::endl;
        desc << "Device name syntax is domain/family/member" << std::ends;

        TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
    }

    //
    // Device name in lower case letters
    //

    std::string full_name_low(full_name);
    std::transform(full_name_low.begin(), full_name_low.end(), full_name_low.begin(), ::tolower);

    //
    // Try to find protocol specification in device name and analyse it
    //

    std::string::size_type pos = full_name_low.find(PROT_SEP);
    if(pos == std::string::npos)
    {
        if(full_name_low.size() > 2)
        {
            if((full_name_low[0] == '/') && (full_name_low[1] == '/'))
            {
                name_wo_prot = full_name_low.substr(2);
            }
            else
            {
                name_wo_prot = full_name_low;
            }
        }
        else
        {
            name_wo_prot = full_name_low;
        }
    }
    else
    {
        std::string protocol = full_name_low.substr(0, pos);

        if(protocol == TANGO_PROTOCOL)
        {
            name_wo_prot = full_name_low.substr(pos + 3);
        }
        else
        {
            TangoSys_OMemStream desc;
            desc << protocol;
            desc << " protocol is an unsupported protocol" << std::ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_UnsupportedProtocol, desc.str());
        }
    }

    //
    // Try to find database database modifier and analyse it
    //

    pos = name_wo_prot.find(MODIFIER);
    if(pos != std::string::npos)
    {
        std::string mod = name_wo_prot.substr(pos + 1);

        if(mod == DBASE_YES)
        {
            std::string::size_type len = name_wo_prot.size();
            name_wo_db_mod = name_wo_prot.substr(0, len - (len - pos));
            dbase_used = true;
        }
        else if(mod == DBASE_NO)
        {
            std::string::size_type len = name_wo_prot.size();
            name_wo_db_mod = name_wo_prot.substr(0, len - (len - pos));
            dbase_used = false;
        }
        else
        {
            // cerr << mod << " is a non supported database modifier!" << std::endl;

            TangoSys_OMemStream desc;
            desc << mod;
            desc << " modifier is an unsupported db modifier" << std::ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_UnsupportedDBaseModifier, desc.str());
        }
    }
    else
    {
        name_wo_db_mod = name_wo_prot;
        dbase_used = true;
    }

    if(!dbase_used)
    {
        //
        // Extract host name and port number
        //

        pos = name_wo_db_mod.find(HOST_SEP);
        if(pos == std::string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Host and port not correctly defined in device name " << full_name << std::ends;

            TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
        }

        host = name_wo_db_mod.substr(0, pos);
        std::string::size_type tmp = name_wo_db_mod.find(PORT_SEP);
        if(tmp == std::string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Host and port not correctly defined in device name " << full_name << std::ends;

            TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
        }
        port = name_wo_db_mod.substr(pos + 1, tmp - pos - 1);
        TangoSys_MemStream s;
        s << port << std::ends;
        s >> port_num;
        device_name = name_wo_db_mod.substr(tmp + 1);

        //
        // Check device name syntax (domain/family/member). Alias are forbidden without
        // using the db
        //

        tmp = device_name.find(DEV_NAME_FIELD_SEP);
        if(tmp == std::string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::endl;
            desc << "Rem: Alias are forbidden when not using a database" << std::ends;

            TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
        }
        std::string::size_type prev_sep = tmp;
        tmp = device_name.find(DEV_NAME_FIELD_SEP, tmp + 1);
        if((tmp == std::string::npos) || (tmp == prev_sep + 1))
        {
            TangoSys_OMemStream desc;
            desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::endl;
            desc << "Rem: Alias are forbidden when not using a database" << std::ends;

            TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
        }
        prev_sep = tmp;
        tmp = device_name.find(DEV_NAME_FIELD_SEP, tmp + 1);
        if(tmp != std::string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::endl;
            desc << "Rem: Alias are forbidden when not using a database" << std::ends;

            TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
        }

        db_host = db_port = NOT_USED;
        db_port_num = 0;
        from_env_var = false;
    }
    else
    {
        //
        // Search if host and port are specified
        //

        pos = name_wo_db_mod.find(PORT_SEP);
        if(pos == std::string::npos)
        {
            //
            // It could be an alias name, check its syntax
            //

            pos = name_wo_db_mod.find(HOST_SEP);
            if(pos != std::string::npos)
            {
                TangoSys_OMemStream desc;
                desc << "Wrong alias name syntax in " << full_name << " (: is not allowed in alias name)" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
            }

            pos = name_wo_db_mod.find(RES_SEP);
            if(pos != std::string::npos)
            {
                TangoSys_OMemStream desc;
                desc << "Wrong alias name syntax in " << full_name << " (-> is not allowed in alias name)" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
            }

            //
            // Alias name syntax OK
            //

            alias_name = device_name = name_wo_db_mod;
            is_alias = true;
            from_env_var = true;
            port_num = 0;
            host = FROM_IOR;
            port = FROM_IOR;
        }
        else
        {
            std::string bef_sep = name_wo_db_mod.substr(0, pos);
            std::string::size_type tmp = bef_sep.find(HOST_SEP);
            if(tmp == std::string::npos)
            {
                //
                // There is at least one / in dev name but it is not a TANGO_HOST definition.
                // A correct dev name must have 2 /. Check this. An alias cannot have any /
                //

                if(pos == 0)
                {
                    TangoSys_OMemStream desc;
                    desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::ends;

                    TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
                }

                std::string::size_type prev_sep = pos;
                pos = name_wo_db_mod.find(DEV_NAME_FIELD_SEP, pos + 1);
                if((pos == std::string::npos) || (pos == prev_sep + 1))
                {
                    TangoSys_OMemStream desc;
                    desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::ends;

                    TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
                }

                prev_sep = pos;
                pos = name_wo_db_mod.find(DEV_NAME_FIELD_SEP, prev_sep + 1);
                if(pos != std::string::npos)
                {
                    TangoSys_OMemStream desc;
                    desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::ends;

                    TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
                }

                device_name = name_wo_db_mod;
                from_env_var = true;
                port_num = 0;
                port = FROM_IOR;
                host = FROM_IOR;
            }
            else
            {
                std::string tmp_host(bef_sep.substr(0, tmp));
                std::string safe_tmp_host(tmp_host);

                if(tmp_host.find('.') == std::string::npos)
                {
                    get_fqdn(tmp_host);
                }

                std::string::size_type pos2 = tmp_host.find('.');
                bool alias_used = false;
                std::string fq;
                if(pos2 != std::string::npos)
                {
                    std::string h_name = tmp_host.substr(0, pos2);
                    fq = tmp_host.substr(pos2);
                    if(h_name != tmp_host)
                    {
                        alias_used = true;
                    }
                }

                if(alias_used)
                {
                    ext_proxy->nethost_alias = true;
                    ext_proxy->orig_tango_host = safe_tmp_host;
                    if(safe_tmp_host.find('.') == std::string::npos)
                    {
                        ext_proxy->orig_tango_host = ext_proxy->orig_tango_host + fq;
                    }
                }
                else
                {
                    ext_proxy->nethost_alias = false;
                }

                db_host = tmp_host;
                db_port = bef_sep.substr(tmp + 1);
                TangoSys_MemStream s;
                s << db_port << std::ends;
                s >> db_port_num;
                object_name = name_wo_db_mod.substr(pos + 1);

                //
                // We should now check if the object name is a device name or an alias
                //

                pos = object_name.find(DEV_NAME_FIELD_SEP);
                if(pos == std::string::npos)
                {
                    //
                    // It is an alias. Check its syntax
                    //

                    pos = object_name.find(HOST_SEP);
                    if(pos != std::string::npos)
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong alias name syntax in " << full_name << " (: is not allowed in alias name)"
                             << std::ends;

                        TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
                    }

                    pos = object_name.find(RES_SEP);
                    if(pos != std::string::npos)
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong alias name syntax in " << full_name << " (-> is not allowed in alias name)"
                             << std::ends;

                        TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
                    }
                    alias_name = device_name = object_name;
                    is_alias = true;

                    //
                    // Alias name syntax OK, but is it really an alias defined in db ?
                    //
                }
                else
                {
                    //
                    // It's a device name. Check its syntax.
                    // There is at least one / in dev name but it is not a TANGO_HOST definition.
                    // A correct dev name must have 2 /. Check this. An alias cannot have any /
                    //

                    std::string::size_type prev_sep = pos;
                    pos = object_name.find(DEV_NAME_FIELD_SEP, pos + 1);
                    if((pos == std::string::npos) || (pos == prev_sep + 1))
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::ends;

                        TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
                    }

                    prev_sep = pos;
                    pos = object_name.find(DEV_NAME_FIELD_SEP, prev_sep + 1);
                    if(pos != std::string::npos)
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong device name syntax (domain/family/member) in " << full_name << std::ends;

                        TANGO_THROW_DETAILED_EXCEPTION(ApiWrongNameExcept, API_WrongDeviceNameSyntax, desc.str());
                    }

                    device_name = object_name;
                }
                from_env_var = false;
                port_num = 0;
                port = FROM_IOR;
                host = FROM_IOR;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_corba_name() - return IOR for device from database
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::get_corba_name(bool need_check_acc)
{
    //
    // If we are in a server, try a local import
    // (in case the device is embedded in the same process)
    //

    std::string local_ior;
    if(ApiUtil::instance()->in_server())
    {
        local_import(local_ior);
    }

    //
    // If we are not in a server or if the device is not in the same process,
    // ask the database
    //

    DbDevImportInfo import_info;

    if(local_ior.size() == 0)
    {
        import_info = db_dev->import_device();

        if(import_info.exported != 1)
        {
            connection_state = CONNECTION_NOTOK;

            TangoSys_OMemStream desc;
            desc << "Device " << device_name << " is not exported (hint: try starting the device server)" << std::ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_DeviceNotExported, desc.str());
        }
    }

    //
    // Get device access right
    //

    if(need_check_acc)
    {
        access = db_dev->check_access_control();
    }
    else
    {
        check_acc = false;
    }

    if(local_ior.size() != 0)
    {
        return local_ior;
    }
    else
    {
        return import_info.ior;
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::build_corba_name() - build corba name for non database device
//                     server. In this case, corba name uses
//                     the "corbaloc" naming schema
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::build_corba_name()
{
    std::string db_corbaloc = "corbaloc:iiop:";
    db_corbaloc = db_corbaloc + host + ":" + port;
    db_corbaloc = db_corbaloc + "/" + device_name;

    return db_corbaloc;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::reconnect() - Call the reconnection method and in case
//                  the device has been created from its alias,
//                      get its real name.
//
//-----------------------------------------------------------------------------

void DeviceProxy::reconnect(bool db_used)
{
    Connection::reconnect(db_used);

    if(connection_state == CONNECTION_OK)
    {
        if(is_alias)
        {
            CORBA::String_var real_name = device->name();
            device_name = real_name.in();
            std::transform(device_name.begin(), device_name.end(), device_name.begin(), ::tolower);
            db_dev->set_name(device_name);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::import_info() - return import info for device from database
//
//-----------------------------------------------------------------------------

DbDevImportInfo DeviceProxy::import_info()
{
    DbDevImportInfo import_info;

    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        import_info = db_dev->import_device();
    }

    return (import_info);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::~DeviceProxy() - destructor to destroy proxy to TANGO device
//
//-----------------------------------------------------------------------------

DeviceProxy::~DeviceProxy()
{
    if(dbase_used)
    {
        delete db_dev;
    }

    //
    // If the device has some subscribed event, unsubscribe them
    //
    unsubscribe_all_events();

    //
    // If the device is locked, unlock it whatever the lock counter is
    //

    if(!ApiUtil::_is_instance_null())
    {
        if(lock_ctr > 0)
        {
            try
            {
                unlock(true);
            }
            catch(...)
            {
            }
        }
    }

    //
    // Delete memory
    //
}

void DeviceProxy::unsubscribe_all_events()
{
    if(ApiUtil *api = ApiUtil::instance())
    {
        auto zmq_event_consumer = api->get_zmq_event_consumer();
        if(zmq_event_consumer != nullptr)
        {
            std::vector<int> event_ids;
            zmq_event_consumer->get_subscribed_event_ids(this, event_ids);

            for(auto event_id = event_ids.begin(); event_id != event_ids.end(); ++event_id)
            {
                try
                {
                    unsubscribe_event(*event_id);
                }
                catch(CORBA::Exception &e)
                {
                    Tango::Except::print_exception(e);
                }
                catch(...)
                {
                    std::cerr << "DeviceProxy::unsubscribe_all_events(): "
                                 "Unknown exception thrown from unsubscribe_event() for "
                                 "device \""
                              << name() << "\" and event_id=" << *event_id << std::endl;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::s) - ping TANGO device and return time elapsed in microseconds
//
//-----------------------------------------------------------------------------

int DeviceProxy::ping()
{
    auto before = std::chrono::steady_clock::now();

    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            dev->ping();
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "ping", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "ping", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute ping on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "ping", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute ping on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute ping on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    auto after = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::name() - return TANGO device name as string
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::name()
{
    std::string na;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var n = dev->name();
            ctr = 2;
            na = n;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "name", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute name() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute name() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute name() on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return (na);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::alias() - return TANGO device alias (if any)
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::alias()
{
    if(alias_name.size() == 0)
    {
        Database *db = this->get_device_db();
        if(db != nullptr)
        {
            db->get_alias(device_name, alias_name);
        }
        else
        {
            TANGO_THROW_EXCEPTION(DB_AliasNotDefined, "No alias found for your device");
        }
    }

    return alias_name;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::state() - return TANGO state of device
//
//-----------------------------------------------------------------------------

DevState DeviceProxy::state()
{
    DevState sta = Tango::UNKNOWN;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            sta = dev->state();
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &transp)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(transp, "DeviceProxy", "state", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "state", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute state() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "state", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute state() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute state() on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return sta;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::status() - return TANGO status of device
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::status()
{
    std::string status_str;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var st = dev->status();
            ctr = 2;
            status_str = st;
        }
        catch(CORBA::TRANSIENT &transp)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(transp, "DeviceProxy", "status", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "status", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute status() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "status", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute status() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute status() on device (CORBA exception)" << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return (status_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::adm_name() - return TANGO admin name of device
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::adm_name()
{
    std::string adm_name_str;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var st = dev->adm_name();
            ctr = 2;
            adm_name_str = st;

            if(!dbase_used)
            {
                std::string prot("tango://");
                if(host.find('.') == std::string::npos)
                {
                    Connection::get_fqdn(host);
                }
                prot = prot + host + ':' + port + '/';
                adm_name_str.insert(0, prot);
                adm_name_str.append(MODIFIER_DBASE_NO);
            }
            else if(!from_env_var)
            {
                std::string prot("tango://");
                prot = prot + db_host + ':' + db_port + '/';
                adm_name_str.insert(0, prot);
            }
        }
        catch(CORBA::TRANSIENT &transp)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(transp, "DeviceProxy", "adm_name", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "adm_name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute adm_name() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "adm_name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute adm_name() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute adm_name() on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return (adm_name_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::description() - return TANGO device description as string
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::description()
{
    std::string description_str;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var st = dev->description();
            ctr = 2;
            description_str = st;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "description", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "description", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute description() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "description", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute description() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute description() on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return (description_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::black_box() - return the list of the last n commands exectued on
//        this TANGO device
//
//-----------------------------------------------------------------------------

std::vector<std::string> *DeviceProxy::black_box(int last_n_commands)
{
    DevVarStringArray_var last_commands;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            last_commands = dev->black_box(last_n_commands);
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "black_box", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "black_box", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute black_box on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "black_box", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute black_box on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute black_box on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    auto *last_commands_vector = new(std::vector<std::string>);
    last_commands_vector->resize(last_commands->length());

    for(unsigned int i = 0; i < last_commands->length(); i++)
    {
        (*last_commands_vector)[i] = last_commands[i];
    }

    return (last_commands_vector);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::info() - return information about this device
//
//-----------------------------------------------------------------------------

DeviceInfo const &DeviceProxy::info()
{
    DevInfo_var dev_info;
    DevInfo_3_var dev_info_3;
    DevInfo_6_var dev_info_6;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if(version >= 6)
            {
                Device_6_var dev = Device_6::_duplicate(device_6);
                dev_info_6 = dev->info_6();

                _info.dev_class = dev_info_6->dev_class;
                _info.server_id = dev_info_6->server_id;
                _info.server_host = dev_info_6->server_host;
                _info.server_version = dev_info_6->server_version;
                _info.doc_url = dev_info_6->doc_url;
                _info.dev_type = dev_info_6->dev_type;

                for(CORBA::ULong i = 0; i < dev_info_6->version_info.length(); ++i)
                {
                    Tango::DevInfoVersion version_info = dev_info_6->version_info[i];
                    _info.version_info.insert(std::make_pair(version_info.key, version_info.value));
                }
            }
            else if(version >= 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev_info_3 = dev->info_3();

                _info.dev_class = dev_info_3->dev_class;
                _info.server_id = dev_info_3->server_id;
                _info.server_host = dev_info_3->server_host;
                _info.server_version = dev_info_3->server_version;
                _info.doc_url = dev_info_3->doc_url;
                _info.dev_type = dev_info_3->dev_type;
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev_info = dev->info();

                _info.dev_class = dev_info->dev_class;
                _info.server_id = dev_info->server_id;
                _info.server_host = dev_info->server_host;
                _info.server_version = dev_info->server_version;
                _info.doc_url = dev_info->doc_url;
                _info.dev_type = NotSet;
            }
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "info", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "info", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute info() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "info", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute info() on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute info() on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return (_info);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::command_query() - return the description for the specified
//        command implemented for this TANGO device
//
//-----------------------------------------------------------------------------

CommandInfo DeviceProxy::command_query(std::string cmd)
{
    CommandInfo command_info;
    DevCmdInfo_var cmd_info;
    DevCmdInfo_2_var cmd_info_2;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if(version == 1)
            {
                Device_var dev = Device::_duplicate(device);
                cmd_info = dev->command_query(cmd.c_str());

                command_info.cmd_name = cmd_info->cmd_name;
                command_info.cmd_tag = cmd_info->cmd_tag;
                command_info.in_type = cmd_info->in_type;
                command_info.out_type = cmd_info->out_type;
                command_info.in_type_desc = cmd_info->in_type_desc;
                command_info.out_type_desc = cmd_info->out_type_desc;
                command_info.disp_level = Tango::OPERATOR;
            }
            else
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                cmd_info_2 = dev->command_query_2(cmd.c_str());

                command_info.cmd_name = cmd_info_2->cmd_name;
                command_info.cmd_tag = cmd_info_2->cmd_tag;
                command_info.in_type = cmd_info_2->in_type;
                command_info.out_type = cmd_info_2->out_type;
                command_info.in_type_desc = cmd_info_2->in_type_desc;
                command_info.out_type_desc = cmd_info_2->out_type_desc;
                command_info.disp_level = cmd_info_2->level;
            }
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "command_query", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "command_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_query on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "command_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_query on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_query on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return (command_info);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_command_config() - return the command info for a set of commands
//
//-----------------------------------------------------------------------------

CommandInfoList *DeviceProxy::get_command_config(std::vector<std::string> &cmd_names)
{
    CommandInfoList *all_cmds = command_list_query();

    //
    // Leave method if the user requires config for all commands
    //

    if(cmd_names.size() == 1 && cmd_names[0] == AllCmd)
    {
        return all_cmds;
    }

    //
    // Return only the required commands config
    //

    CommandInfoList *ret_cmds = new CommandInfoList;
    std::vector<std::string>::iterator ite;
    for(ite = cmd_names.begin(); ite != cmd_names.end(); ++ite)
    {
        std::string w_str(*ite);

        std::transform(w_str.begin(), w_str.end(), w_str.begin(), ::tolower);

        std::vector<CommandInfo>::iterator pos;
        for(pos = all_cmds->begin(); pos != all_cmds->end(); ++pos)
        {
            std::string lower_cmd(pos->cmd_name);
            std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
            if(w_str == lower_cmd)
            {
                ret_cmds->push_back(*pos);
                break;
            }
        }
    }

    delete all_cmds;

    return ret_cmds;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::command_list_query() - return the list of commands implemented for this TANGO device
//
//-----------------------------------------------------------------------------

CommandInfoList *DeviceProxy::command_list_query()
{
    CommandInfoList *command_info_list = nullptr;
    DevCmdInfoList_var cmd_info_list;
    DevCmdInfoList_2_var cmd_info_list_2;
    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if(version == 1)
            {
                Device_var dev = Device::_duplicate(device);
                cmd_info_list = dev->command_list_query();

                command_info_list = new CommandInfoList(cmd_info_list->length());
                //                command_info_list->resize(cmd_info_list->length());

                for(unsigned int i = 0; i < cmd_info_list->length(); i++)
                {
                    (*command_info_list)[i].cmd_name = cmd_info_list[i].cmd_name;
                    (*command_info_list)[i].cmd_tag = cmd_info_list[i].cmd_tag;
                    (*command_info_list)[i].in_type = cmd_info_list[i].in_type;
                    (*command_info_list)[i].out_type = cmd_info_list[i].out_type;
                    (*command_info_list)[i].in_type_desc = cmd_info_list[i].in_type_desc;
                    (*command_info_list)[i].out_type_desc = cmd_info_list[i].out_type_desc;
                    (*command_info_list)[i].disp_level = Tango::OPERATOR;
                }
            }
            else
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                cmd_info_list_2 = dev->command_list_query_2();

                command_info_list = new CommandInfoList(cmd_info_list_2->length());
                //                command_info_list->resize(cmd_info_list_2->length());

                for(unsigned int i = 0; i < cmd_info_list_2->length(); i++)
                {
                    (*command_info_list)[i].cmd_name = cmd_info_list_2[i].cmd_name;
                    (*command_info_list)[i].cmd_tag = cmd_info_list_2[i].cmd_tag;
                    (*command_info_list)[i].in_type = cmd_info_list_2[i].in_type;
                    (*command_info_list)[i].out_type = cmd_info_list_2[i].out_type;
                    (*command_info_list)[i].in_type_desc = cmd_info_list_2[i].in_type_desc;
                    (*command_info_list)[i].out_type_desc = cmd_info_list_2[i].out_type_desc;
                    (*command_info_list)[i].disp_level = cmd_info_list_2[i].level;
                }
            }
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "command_list_query", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "command_list_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_list_query on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "command_list_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_list_query on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_list_query on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    return (command_info_list);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_command_list() - return the list of commands implemented for this TANGO device (only names)
//
//-----------------------------------------------------------------------------

std::vector<std::string> *DeviceProxy::get_command_list()
{
    CommandInfoList *all_cmd_config;

    all_cmd_config = command_list_query();

    auto *cmd_list = new std::vector<std::string>;
    cmd_list->resize(all_cmd_config->size());
    for(unsigned int i = 0; i < all_cmd_config->size(); i++)
    {
        (*cmd_list)[i] = (*all_cmd_config)[i].cmd_name;
    }
    delete all_cmd_config;

    return cmd_list;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property() - get a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property(const std::string &property_name, DbData &db_data)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        db_data.resize(1);
        db_data[0] = DbDatum(property_name);

        db_dev->get_property(db_data);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property() - get a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property(const std::vector<std::string> &property_names, DbData &db_data)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        db_data.resize(property_names.size());
        for(unsigned int i = 0; i < property_names.size(); i++)
        {
            db_data[i] = DbDatum(property_names[i]);
        }

        db_dev->get_property(db_data);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property() - get a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property(DbData &db_data)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        db_dev->get_property(db_data);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::put_property() - put a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::put_property(const DbData &db_data)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        db_dev->put_property(db_data);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::delete_property() - delete a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::delete_property(const std::string &property_name)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        DbData db_data;

        db_data.emplace_back(property_name);

        db_dev->delete_property(db_data);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::delete_property() - delete a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::delete_property(const std::vector<std::string> &property_names)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        DbData db_data;

        for(unsigned int i = 0; i < property_names.size(); i++)
        {
            db_data.emplace_back(property_names[i]);
        }

        db_dev->delete_property(db_data);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::delete_property() - delete a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::delete_property(const DbData &db_data)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        db_dev->delete_property(db_data);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property_list() - get a list of property names from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property_list(const std::string &wildcard, std::vector<std::string> &prop_list)
{
    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }
    else
    {
        int num = 0;
        num = count(wildcard.begin(), wildcard.end(), '*');

        if(num > 1)
        {
            TANGO_THROW_DETAILED_EXCEPTION(
                ApiWrongNameExcept, API_WrongWildcardUsage, "Only one wildcard character (*) allowed!");
        }
        db_dev->get_property_list(wildcard, prop_list);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_config() - return a list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoList *DeviceProxy::get_attribute_config(const std::vector<std::string> &attr_string_list)
{
    AttributeConfigList_var attr_config_list;
    AttributeConfigList_2_var attr_config_list_2;
    AttributeInfoList *dev_attr_config = new AttributeInfoList();
    DevVarStringArray attr_list;
    int ctr = 0;

    attr_list.length(attr_string_list.size());
    for(unsigned int i = 0; i < attr_string_list.size(); i++)
    {
        if(attr_string_list[i] == AllAttr)
        {
            if(version >= 3)
            {
                attr_list[i] = string_dup(AllAttr_3);
            }
            else
            {
                attr_list[i] = string_dup(AllAttr);
            }
        }
        else if(attr_string_list[i] == AllAttr_3)
        {
            if(version < 3)
            {
                attr_list[i] = string_dup(AllAttr);
            }
            else
            {
                attr_list[i] = string_dup(AllAttr_3);
            }
        }
        else
        {
            attr_list[i] = string_dup(attr_string_list[i].c_str());
        }
    }

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if(version == 1)
            {
                Device_var dev = Device::_duplicate(device);
                attr_config_list = dev->get_attribute_config(attr_list);

                dev_attr_config->resize(attr_config_list->length());

                for(unsigned int i = 0; i < attr_config_list->length(); i++)
                {
                    (*dev_attr_config)[i].name = attr_config_list[i].name;
                    (*dev_attr_config)[i].writable = attr_config_list[i].writable;
                    (*dev_attr_config)[i].data_format = attr_config_list[i].data_format;
                    (*dev_attr_config)[i].data_type = attr_config_list[i].data_type;
                    (*dev_attr_config)[i].max_dim_x = attr_config_list[i].max_dim_x;
                    (*dev_attr_config)[i].max_dim_y = attr_config_list[i].max_dim_y;
                    (*dev_attr_config)[i].description = attr_config_list[i].description;
                    (*dev_attr_config)[i].label = attr_config_list[i].label;
                    (*dev_attr_config)[i].unit = attr_config_list[i].unit;
                    (*dev_attr_config)[i].standard_unit = attr_config_list[i].standard_unit;
                    (*dev_attr_config)[i].display_unit = attr_config_list[i].display_unit;
                    (*dev_attr_config)[i].format = attr_config_list[i].format;
                    (*dev_attr_config)[i].min_value = attr_config_list[i].min_value;
                    (*dev_attr_config)[i].max_value = attr_config_list[i].max_value;
                    (*dev_attr_config)[i].min_alarm = attr_config_list[i].min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list[i].max_alarm;
                    (*dev_attr_config)[i].writable_attr_name = attr_config_list[i].writable_attr_name;
                    (*dev_attr_config)[i].extensions.resize(attr_config_list[i].extensions.length());
                    for(unsigned int j = 0; j < attr_config_list[i].extensions.length(); j++)
                    {
                        (*dev_attr_config)[i].extensions[j] = attr_config_list[i].extensions[j];
                    }
                    (*dev_attr_config)[i].disp_level = Tango::OPERATOR;
                }
            }
            else
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_config_list_2 = dev->get_attribute_config_2(attr_list);

                dev_attr_config->resize(attr_config_list_2->length());

                for(unsigned int i = 0; i < attr_config_list_2->length(); i++)
                {
                    (*dev_attr_config)[i].name = attr_config_list_2[i].name;
                    (*dev_attr_config)[i].writable = attr_config_list_2[i].writable;
                    (*dev_attr_config)[i].data_format = attr_config_list_2[i].data_format;
                    (*dev_attr_config)[i].data_type = attr_config_list_2[i].data_type;
                    (*dev_attr_config)[i].max_dim_x = attr_config_list_2[i].max_dim_x;
                    (*dev_attr_config)[i].max_dim_y = attr_config_list_2[i].max_dim_y;
                    (*dev_attr_config)[i].description = attr_config_list_2[i].description;
                    (*dev_attr_config)[i].label = attr_config_list_2[i].label;
                    (*dev_attr_config)[i].unit = attr_config_list_2[i].unit;
                    (*dev_attr_config)[i].standard_unit = attr_config_list_2[i].standard_unit;
                    (*dev_attr_config)[i].display_unit = attr_config_list_2[i].display_unit;
                    (*dev_attr_config)[i].format = attr_config_list_2[i].format;
                    (*dev_attr_config)[i].min_value = attr_config_list_2[i].min_value;
                    (*dev_attr_config)[i].max_value = attr_config_list_2[i].max_value;
                    (*dev_attr_config)[i].min_alarm = attr_config_list_2[i].min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list_2[i].max_alarm;
                    (*dev_attr_config)[i].writable_attr_name = attr_config_list_2[i].writable_attr_name;
                    (*dev_attr_config)[i].extensions.resize(attr_config_list_2[i].extensions.length());
                    for(unsigned int j = 0; j < attr_config_list_2[i].extensions.length(); j++)
                    {
                        (*dev_attr_config)[i].extensions[j] = attr_config_list_2[i].extensions[j];
                    }
                    (*dev_attr_config)[i].disp_level = attr_config_list_2[i].level;
                }
            }
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "get_attribute_config", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute get_attribute_config on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
        catch(Tango::DevFailed &)
        {
            delete dev_attr_config;
            throw;
        }
    }

    return (dev_attr_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_config_ex() - return a list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoListEx *DeviceProxy::get_attribute_config_ex(const std::vector<std::string> &attr_string_list)
{
    AttributeConfigList_var attr_config_list;
    AttributeConfigList_2_var attr_config_list_2;
    AttributeConfigList_3_var attr_config_list_3;
    AttributeConfigList_5_var attr_config_list_5;
    AttributeInfoListEx *dev_attr_config = new AttributeInfoListEx();
    DevVarStringArray attr_list;
    int ctr = 0;

    attr_list.length(attr_string_list.size());
    for(unsigned int i = 0; i < attr_string_list.size(); i++)
    {
        if(attr_string_list[i] == AllAttr)
        {
            if(version >= 3)
            {
                attr_list[i] = string_dup(AllAttr_3);
            }
            else
            {
                attr_list[i] = string_dup(AllAttr);
            }
        }
        else if(attr_string_list[i] == AllAttr_3)
        {
            if(version < 3)
            {
                attr_list[i] = string_dup(AllAttr);
            }
            else
            {
                attr_list[i] = string_dup(AllAttr_3);
            }
        }
        else
        {
            attr_list[i] = string_dup(attr_string_list[i].c_str());
        }
    }

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            switch(version)
            {
            case 1:
            {
                Device_var dev = Device::_duplicate(device);
                attr_config_list = dev->get_attribute_config(attr_list);
                dev_attr_config->resize(attr_config_list->length());

                for(size_t i = 0; i < attr_config_list->length(); i++)
                {
                    COPY_BASE_CONFIG((*dev_attr_config), attr_config_list)
                    (*dev_attr_config)[i].min_alarm = attr_config_list[i].min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list[i].max_alarm;
                    (*dev_attr_config)[i].disp_level = Tango::OPERATOR;
                }
            }
            break;

            case 2:
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_config_list_2 = dev->get_attribute_config_2(attr_list);
                dev_attr_config->resize(attr_config_list_2->length());

                for(size_t i = 0; i < attr_config_list_2->length(); i++)
                {
                    COPY_BASE_CONFIG((*dev_attr_config), attr_config_list_2)
                    (*dev_attr_config)[i].min_alarm = attr_config_list_2[i].min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list_2[i].max_alarm;
                    (*dev_attr_config)[i].disp_level = attr_config_list_2[i].level;
                }

                get_remaining_param(dev_attr_config);
            }
            break;

            case 3:
            case 4:
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                attr_config_list_3 = dev->get_attribute_config_3(attr_list);
                dev_attr_config->resize(attr_config_list_3->length());

                for(size_t i = 0; i < attr_config_list_3->length(); i++)
                {
                    COPY_BASE_CONFIG((*dev_attr_config), attr_config_list_3)

                    for(size_t j = 0; j < attr_config_list_3[i].sys_extensions.length(); j++)
                    {
                        (*dev_attr_config)[i].sys_extensions[j] = attr_config_list_3[i].sys_extensions[j];
                    }
                    (*dev_attr_config)[i].min_alarm = attr_config_list_3[i].att_alarm.min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list_3[i].att_alarm.max_alarm;
                    (*dev_attr_config)[i].disp_level = attr_config_list_3[i].level;
                    (*dev_attr_config)[i].memorized = NOT_KNOWN;

                    COPY_ALARM_CONFIG((*dev_attr_config), attr_config_list_3)

                    COPY_EVENT_CONFIG((*dev_attr_config), attr_config_list_3)
                }
            }
            break;

            case 5:
            case 6:
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_config_list_5 = dev->get_attribute_config_5(attr_list);
                dev_attr_config->resize(attr_config_list_5->length());

                for(size_t i = 0; i < attr_config_list_5->length(); i++)
                {
                    COPY_BASE_CONFIG((*dev_attr_config), attr_config_list_5)

                    for(size_t j = 0; j < attr_config_list_5[i].sys_extensions.length(); j++)
                    {
                        (*dev_attr_config)[i].sys_extensions[j] = attr_config_list_5[i].sys_extensions[j];
                    }
                    (*dev_attr_config)[i].disp_level = attr_config_list_5[i].level;
                    (*dev_attr_config)[i].min_alarm = attr_config_list_5[i].att_alarm.min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list_5[i].att_alarm.max_alarm;
                    (*dev_attr_config)[i].root_attr_name = attr_config_list_5[i].root_attr_name;
                    if(!attr_config_list_5[i].memorized)
                    {
                        (*dev_attr_config)[i].memorized = NONE;
                    }
                    else
                    {
                        if(!attr_config_list_5[i].mem_init)
                        {
                            (*dev_attr_config)[i].memorized = MEMORIZED;
                        }
                        else
                        {
                            (*dev_attr_config)[i].memorized = MEMORIZED_WRITE_INIT;
                        }
                    }
                    if(attr_config_list_5[i].data_type == DEV_ENUM)
                    {
                        for(size_t loop = 0; loop < attr_config_list_5[i].enum_labels.length(); loop++)
                        {
                            (*dev_attr_config)[i].enum_labels.emplace_back(
                                attr_config_list_5[i].enum_labels[loop].in());
                        }
                    }
                    COPY_ALARM_CONFIG((*dev_attr_config), attr_config_list_5)

                    COPY_EVENT_CONFIG((*dev_attr_config), attr_config_list_5)
                }
            }
            break;

            default:
                TANGO_ASSERT_ON_DEFAULT(version);
            }

            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "get_attribute_config", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute get_attribute_config on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
        catch(Tango::DevFailed &)
        {
            delete dev_attr_config;
            throw;
        }
    }

    return (dev_attr_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_remaining_param()
//
// For device implementing device_2, get attribute config param from db
// instead of getting them from device. The wanted parameters are the
// warning alarm parameters, the RDS parameters and the event param.
// This method is called only for device_2 device
//
// In : dev_attr_config : ptr to attribute config vector returned
//              to caller
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_remaining_param(AttributeInfoListEx *dev_attr_config)
{
    //
    // Give a default value to all param.
    //

    for(unsigned int loop = 0; loop < dev_attr_config->size(); loop++)
    {
        (*dev_attr_config)[loop].alarms.min_alarm = (*dev_attr_config)[loop].min_alarm;
        (*dev_attr_config)[loop].alarms.max_alarm = (*dev_attr_config)[loop].max_alarm;
        (*dev_attr_config)[loop].alarms.min_warning = AlrmValueNotSpec;
        (*dev_attr_config)[loop].alarms.max_warning = AlrmValueNotSpec;
        (*dev_attr_config)[loop].alarms.delta_t = AlrmValueNotSpec;
        (*dev_attr_config)[loop].alarms.delta_val = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.ch_event.abs_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.ch_event.rel_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.per_event.period = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.arch_event.archive_abs_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.arch_event.archive_rel_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.arch_event.archive_period = AlrmValueNotSpec;
    }

    //
    // If device does not use db, simply retruns
    //

    if(!dbase_used)
    {
        return;
    }
    else
    {
        //
        // First get device class (if not already done)
        //

        if(_info.dev_class.empty())
        {
            this->info();
        }

        //
        // Get class attribute properties
        //

        DbData db_data_class, db_data_device;
        unsigned int i, k;
        int j;
        for(i = 0; i < dev_attr_config->size(); i++)
        {
            db_data_class.emplace_back((*dev_attr_config)[i].name);
            db_data_device.emplace_back((*dev_attr_config)[i].name);
        }
        db_dev->get_dbase()->get_class_attribute_property(_info.dev_class, db_data_class);

        //
        // Now get device attribute properties
        //

        db_dev->get_attribute_property(db_data_device);

        //
        // Init remaining parameters from them retrieve at class level
        //

        for(i = 0; i < db_data_class.size(); i++)
        {
            long nb_prop;

            std::string &att_name = db_data_class[i].name;
            db_data_class[i] >> nb_prop;
            i++;

            for(j = 0; j < nb_prop; j++)
            {
                //
                // Extract prop value
                //

                std::string prop_value;
                std::string &prop_name = db_data_class[i].name;
                if(db_data_class[i].size() != 1)
                {
                    std::vector<std::string> tmp;
                    db_data_class[i] >> tmp;
                    prop_value = tmp[0] + ", " + tmp[1];
                }
                else
                {
                    db_data_class[i] >> prop_value;
                }
                i++;

                //
                // Store prop value in attribute config vector
                //

                for(k = 0; k < dev_attr_config->size(); k++)
                {
                    if((*dev_attr_config)[k].name == att_name)
                    {
                        if(prop_name == "min_warning")
                        {
                            (*dev_attr_config)[k].alarms.min_warning = prop_value;
                        }
                        else if(prop_name == "max_warning")
                        {
                            (*dev_attr_config)[k].alarms.max_warning = prop_value;
                        }
                        else if(prop_name == "delta_t")
                        {
                            (*dev_attr_config)[k].alarms.delta_t = prop_value;
                        }
                        else if(prop_name == "delta_val")
                        {
                            (*dev_attr_config)[k].alarms.delta_val = prop_value;
                        }
                        else if(prop_name == "abs_change")
                        {
                            (*dev_attr_config)[k].events.ch_event.abs_change = prop_value;
                        }
                        else if(prop_name == "rel_change")
                        {
                            (*dev_attr_config)[k].events.ch_event.rel_change = prop_value;
                        }
                        else if(prop_name == "period")
                        {
                            (*dev_attr_config)[k].events.per_event.period = prop_value;
                        }
                        else if(prop_name == "archive_abs_change")
                        {
                            (*dev_attr_config)[k].events.arch_event.archive_abs_change = prop_value;
                        }
                        else if(prop_name == "archive_rel_change")
                        {
                            (*dev_attr_config)[k].events.arch_event.archive_rel_change = prop_value;
                        }
                        else if(prop_name == "archive_period")
                        {
                            (*dev_attr_config)[k].events.arch_event.archive_period = prop_value;
                        }
                    }
                }
            }
        }

        //
        // Init remaining parameters from them retrieve at device level
        //

        for(i = 0; i < db_data_device.size(); i++)
        {
            long nb_prop;

            std::string &att_name = db_data_device[i].name;
            db_data_device[i] >> nb_prop;
            i++;

            for(j = 0; j < nb_prop; j++)
            {
                //
                // Extract prop value
                //

                std::string prop_value;
                std::string &prop_name = db_data_device[i].name;
                if(db_data_device[i].size() != 1)
                {
                    std::vector<std::string> tmp;
                    db_data_device[i] >> tmp;
                    prop_value = tmp[0] + ", " + tmp[1];
                }
                else
                {
                    db_data_device[i] >> prop_value;
                }
                i++;

                //
                // Store prop value in attribute config vector
                //

                for(k = 0; k < dev_attr_config->size(); k++)
                {
                    if((*dev_attr_config)[k].name == att_name)
                    {
                        if(prop_name == "min_warning")
                        {
                            (*dev_attr_config)[k].alarms.min_warning = prop_value;
                        }
                        else if(prop_name == "max_warning")
                        {
                            (*dev_attr_config)[k].alarms.max_warning = prop_value;
                        }
                        else if(prop_name == "delta_t")
                        {
                            (*dev_attr_config)[k].alarms.delta_t = prop_value;
                        }
                        else if(prop_name == "delta_val")
                        {
                            (*dev_attr_config)[k].alarms.delta_val = prop_value;
                        }
                        else if(prop_name == "abs_change")
                        {
                            (*dev_attr_config)[k].events.ch_event.abs_change = prop_value;
                        }
                        else if(prop_name == "rel_change")
                        {
                            (*dev_attr_config)[k].events.ch_event.rel_change = prop_value;
                        }
                        else if(prop_name == "period")
                        {
                            (*dev_attr_config)[k].events.per_event.period = prop_value;
                        }
                        else if(prop_name == "archive_abs_change")
                        {
                            (*dev_attr_config)[k].events.arch_event.archive_abs_change = prop_value;
                        }
                        else if(prop_name == "archive_rel_change")
                        {
                            (*dev_attr_config)[k].events.arch_event.archive_rel_change = prop_value;
                        }
                        else if(prop_name == "archive_period")
                        {
                            (*dev_attr_config)[k].events.arch_event.archive_period = prop_value;
                        }
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_config() - return a single attribute config
//
//-----------------------------------------------------------------------------

AttributeInfoEx DeviceProxy::get_attribute_config(const std::string &attr_string)
{
    std::vector<std::string> attr_string_list;
    AttributeInfoListEx *dev_attr_config_list;
    AttributeInfoEx dev_attr_config;

    attr_string_list.push_back(attr_string);
    dev_attr_config_list = get_attribute_config_ex(attr_string_list);

    dev_attr_config = (*dev_attr_config_list)[0];
    delete(dev_attr_config_list);

    return (dev_attr_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::set_attribute_config() - set config for a list of attributes
//
//-----------------------------------------------------------------------------

void DeviceProxy::set_attribute_config(const AttributeInfoList &dev_attr_list)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    AttributeConfigList attr_config_list;
    DevVarStringArray attr_list;
    int ctr = 0;

    attr_config_list.length(dev_attr_list.size());

    for(unsigned int i = 0; i < attr_config_list.length(); i++)
    {
        attr_config_list[i].name = dev_attr_list[i].name.c_str();
        attr_config_list[i].writable = dev_attr_list[i].writable;
        attr_config_list[i].data_format = dev_attr_list[i].data_format;
        attr_config_list[i].data_type = dev_attr_list[i].data_type;
        attr_config_list[i].max_dim_x = dev_attr_list[i].max_dim_x;
        attr_config_list[i].max_dim_y = dev_attr_list[i].max_dim_y;
        attr_config_list[i].description = dev_attr_list[i].description.c_str();
        attr_config_list[i].label = dev_attr_list[i].label.c_str();
        attr_config_list[i].unit = dev_attr_list[i].unit.c_str();
        attr_config_list[i].standard_unit = dev_attr_list[i].standard_unit.c_str();
        attr_config_list[i].display_unit = dev_attr_list[i].display_unit.c_str();
        attr_config_list[i].format = dev_attr_list[i].format.c_str();
        attr_config_list[i].min_value = dev_attr_list[i].min_value.c_str();
        attr_config_list[i].max_value = dev_attr_list[i].max_value.c_str();
        attr_config_list[i].min_alarm = dev_attr_list[i].min_alarm.c_str();
        attr_config_list[i].max_alarm = dev_attr_list[i].max_alarm.c_str();
        attr_config_list[i].writable_attr_name = dev_attr_list[i].writable_attr_name.c_str();
        attr_config_list[i].extensions.length(dev_attr_list[i].extensions.size());
        for(unsigned int j = 0; j < dev_attr_list[i].extensions.size(); j++)
        {
            attr_config_list[i].extensions[j] = string_dup(dev_attr_list[i].extensions[j].c_str());
        }
    }

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            dev->set_attribute_config(attr_config_list);
            ctr = 2;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;

                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                throw;
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "set_attribute_config", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    TANGO_TELEMETRY_TRACE_END();
}

void DeviceProxy::set_attribute_config(const AttributeInfoListEx &dev_attr_list)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    AttributeConfigList attr_config_list;
    AttributeConfigList_3 attr_config_list_3;
    AttributeConfigList_5 attr_config_list_5;
    DevVarStringArray attr_list;
    int ctr = 0;
    unsigned int i, j;

    if(version >= 5)
    {
        attr_config_list_5.length(dev_attr_list.size());

        for(i = 0; i < attr_config_list_5.length(); i++)
        {
            ApiUtil::AttributeInfoEx_to_AttributeConfig(&dev_attr_list[i], &attr_config_list_5[i]);
        }
    }
    else if(version >= 3)
    {
        attr_config_list_3.length(dev_attr_list.size());

        for(i = 0; i < attr_config_list_3.length(); i++)
        {
            attr_config_list_3[i].name = dev_attr_list[i].name.c_str();
            attr_config_list_3[i].writable = dev_attr_list[i].writable;
            attr_config_list_3[i].data_format = dev_attr_list[i].data_format;
            attr_config_list_3[i].data_type = dev_attr_list[i].data_type;
            attr_config_list_3[i].max_dim_x = dev_attr_list[i].max_dim_x;
            attr_config_list_3[i].max_dim_y = dev_attr_list[i].max_dim_y;
            attr_config_list_3[i].description = dev_attr_list[i].description.c_str();
            attr_config_list_3[i].label = dev_attr_list[i].label.c_str();
            attr_config_list_3[i].unit = dev_attr_list[i].unit.c_str();
            attr_config_list_3[i].standard_unit = dev_attr_list[i].standard_unit.c_str();
            attr_config_list_3[i].display_unit = dev_attr_list[i].display_unit.c_str();
            attr_config_list_3[i].format = dev_attr_list[i].format.c_str();
            attr_config_list_3[i].min_value = dev_attr_list[i].min_value.c_str();
            attr_config_list_3[i].max_value = dev_attr_list[i].max_value.c_str();
            attr_config_list_3[i].writable_attr_name = dev_attr_list[i].writable_attr_name.c_str();
            attr_config_list_3[i].level = dev_attr_list[i].disp_level;
            attr_config_list_3[i].extensions.length(dev_attr_list[i].extensions.size());
            for(j = 0; j < dev_attr_list[i].extensions.size(); j++)
            {
                attr_config_list_3[i].extensions[j] = string_dup(dev_attr_list[i].extensions[j].c_str());
            }
            for(j = 0; j < dev_attr_list[i].sys_extensions.size(); j++)
            {
                attr_config_list_3[i].sys_extensions[j] = string_dup(dev_attr_list[i].sys_extensions[j].c_str());
            }

            attr_config_list_3[i].att_alarm.min_alarm = dev_attr_list[i].alarms.min_alarm.c_str();
            attr_config_list_3[i].att_alarm.max_alarm = dev_attr_list[i].alarms.max_alarm.c_str();
            attr_config_list_3[i].att_alarm.min_warning = dev_attr_list[i].alarms.min_warning.c_str();
            attr_config_list_3[i].att_alarm.max_warning = dev_attr_list[i].alarms.max_warning.c_str();
            attr_config_list_3[i].att_alarm.delta_t = dev_attr_list[i].alarms.delta_t.c_str();
            attr_config_list_3[i].att_alarm.delta_val = dev_attr_list[i].alarms.delta_val.c_str();
            for(j = 0; j < dev_attr_list[i].alarms.extensions.size(); j++)
            {
                attr_config_list_3[i].att_alarm.extensions[j] =
                    string_dup(dev_attr_list[i].alarms.extensions[j].c_str());
            }

            attr_config_list_3[i].event_prop.ch_event.rel_change = dev_attr_list[i].events.ch_event.rel_change.c_str();
            attr_config_list_3[i].event_prop.ch_event.abs_change = dev_attr_list[i].events.ch_event.abs_change.c_str();
            for(j = 0; j < dev_attr_list[i].events.ch_event.extensions.size(); j++)
            {
                attr_config_list_3[i].event_prop.ch_event.extensions[j] =
                    string_dup(dev_attr_list[i].events.ch_event.extensions[j].c_str());
            }

            attr_config_list_3[i].event_prop.per_event.period = dev_attr_list[i].events.per_event.period.c_str();
            for(j = 0; j < dev_attr_list[i].events.per_event.extensions.size(); j++)
            {
                attr_config_list_3[i].event_prop.per_event.extensions[j] =
                    string_dup(dev_attr_list[i].events.per_event.extensions[j].c_str());
            }

            attr_config_list_3[i].event_prop.arch_event.rel_change =
                dev_attr_list[i].events.arch_event.archive_rel_change.c_str();
            attr_config_list_3[i].event_prop.arch_event.abs_change =
                dev_attr_list[i].events.arch_event.archive_abs_change.c_str();
            attr_config_list_3[i].event_prop.arch_event.period =
                dev_attr_list[i].events.arch_event.archive_period.c_str();
            for(j = 0; j < dev_attr_list[i].events.ch_event.extensions.size(); j++)
            {
                attr_config_list_3[i].event_prop.arch_event.extensions[j] =
                    string_dup(dev_attr_list[i].events.arch_event.extensions[j].c_str());
            }
        }
    }
    else
    {
        attr_config_list.length(dev_attr_list.size());

        for(i = 0; i < attr_config_list.length(); i++)
        {
            attr_config_list[i].name = dev_attr_list[i].name.c_str();
            attr_config_list[i].writable = dev_attr_list[i].writable;
            attr_config_list[i].data_format = dev_attr_list[i].data_format;
            attr_config_list[i].data_type = dev_attr_list[i].data_type;
            attr_config_list[i].max_dim_x = dev_attr_list[i].max_dim_x;
            attr_config_list[i].max_dim_y = dev_attr_list[i].max_dim_y;
            attr_config_list[i].description = dev_attr_list[i].description.c_str();
            attr_config_list[i].label = dev_attr_list[i].label.c_str();
            attr_config_list[i].unit = dev_attr_list[i].unit.c_str();
            attr_config_list[i].standard_unit = dev_attr_list[i].standard_unit.c_str();
            attr_config_list[i].display_unit = dev_attr_list[i].display_unit.c_str();
            attr_config_list[i].format = dev_attr_list[i].format.c_str();
            attr_config_list[i].min_value = dev_attr_list[i].min_value.c_str();
            attr_config_list[i].max_value = dev_attr_list[i].max_value.c_str();
            attr_config_list[i].min_alarm = dev_attr_list[i].min_alarm.c_str();
            attr_config_list[i].max_alarm = dev_attr_list[i].max_alarm.c_str();
            attr_config_list[i].writable_attr_name = dev_attr_list[i].writable_attr_name.c_str();
            attr_config_list[i].extensions.length(dev_attr_list[i].extensions.size());
            for(j = 0; j < dev_attr_list[i].extensions.size(); j++)
            {
                attr_config_list_3[i].extensions[j] = string_dup(dev_attr_list[i].extensions[j].c_str());
            }
        }
    }

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if(version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                dev->set_attribute_config_5(attr_config_list_5, get_client_identification());
            }
            else if(version == 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                dev->set_attribute_config_4(attr_config_list_3, get_client_identification());
            }
            else if(version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->set_attribute_config_3(attr_config_list_3);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                device->set_attribute_config(attr_config_list);
            }
            ctr = 2;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;

                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                throw;
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "set_attribute_config", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute set_attribute_config on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_pipe_config() - return a list of pipe config
//
//-----------------------------------------------------------------------------

PipeInfoList *DeviceProxy::get_pipe_config(const std::vector<std::string> &pipe_string_list)
{
    PipeConfigList_var pipe_config_list_5;
    PipeInfoList *dev_pipe_config = new PipeInfoList();
    DevVarStringArray pipe_list;
    int ctr = 0;

    //
    // Error if device does not support IDL 5
    //

    if(detail::IDLVersionIsTooOld(version, 5))
    {
        std::stringstream ss;
        ss << "Device " << device_name << " too old to use get_pipe_config() call. Please upgrade to Tango 9/IDL5";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, ss.str());
    }

    //
    // Prepare sent parameters
    //

    pipe_list.length(pipe_string_list.size());
    for(unsigned int i = 0; i < pipe_string_list.size(); i++)
    {
        if(pipe_string_list[i] == AllPipe)
        {
            pipe_list[i] = string_dup(AllPipe);
        }
        else
        {
            pipe_list[i] = string_dup(pipe_string_list[i].c_str());
        }
    }

    //
    // Call device
    //

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_5_var dev = Device_5::_duplicate(device_5);
            pipe_config_list_5 = dev->get_pipe_config_5(pipe_list);
            dev_pipe_config->resize(pipe_config_list_5->length());

            for(size_t i = 0; i < pipe_config_list_5->length(); i++)
            {
                (*dev_pipe_config)[i].disp_level = pipe_config_list_5[i].level;
                (*dev_pipe_config)[i].name = pipe_config_list_5[i].name;
                (*dev_pipe_config)[i].description = pipe_config_list_5[i].description;
                (*dev_pipe_config)[i].label = pipe_config_list_5[i].label;
                (*dev_pipe_config)[i].writable = pipe_config_list_5[i].writable;
                for(size_t j = 0; j < pipe_config_list_5[i].extensions.length(); j++)
                {
                    (*dev_pipe_config)[i].extensions[j] = pipe_config_list_5[i].extensions[j];
                }
            }

            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "get_pipe_config", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "get_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_pipe_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "get_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_pipe_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute get_pipe_config on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
        catch(Tango::DevFailed &)
        {
            delete dev_pipe_config;
            throw;
        }
    }

    return (dev_pipe_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_pipe_config() - return a pipe config
//
//-----------------------------------------------------------------------------

PipeInfo DeviceProxy::get_pipe_config(const std::string &pipe_name)
{
    std::vector<std::string> pipe_string_list;
    PipeInfoList *dev_pipe_config_list;
    PipeInfo dev_pipe_config;

    pipe_string_list.push_back(pipe_name);
    dev_pipe_config_list = get_pipe_config(pipe_string_list);

    dev_pipe_config = (*dev_pipe_config_list)[0];
    delete(dev_pipe_config_list);

    return (dev_pipe_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::set_pipe_config() - set config for a list of pipes
//
//-----------------------------------------------------------------------------

void DeviceProxy::set_pipe_config(const PipeInfoList &dev_pipe_list)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // Error if device does not support IDL 5
    //

    if(detail::IDLVersionIsTooOld(version, 5))
    {
        std::stringstream ss;
        ss << "Device " << device_name << " too old to use set_pipe_config() call. Please upgrade to Tango 9/IDL5";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, ss.str());
    }

    PipeConfigList pipe_config_list;
    int ctr = 0;

    pipe_config_list.length(dev_pipe_list.size());

    for(unsigned int i = 0; i < pipe_config_list.length(); i++)
    {
        pipe_config_list[i].name = dev_pipe_list[i].name.c_str();
        pipe_config_list[i].writable = dev_pipe_list[i].writable;
        pipe_config_list[i].description = dev_pipe_list[i].description.c_str();
        pipe_config_list[i].label = dev_pipe_list[i].label.c_str();
        pipe_config_list[i].level = dev_pipe_list[i].disp_level;
        pipe_config_list[i].extensions.length(dev_pipe_list[i].extensions.size());
        for(unsigned int j = 0; j < dev_pipe_list[i].extensions.size(); j++)
        {
            pipe_config_list[i].extensions[j] = string_dup(dev_pipe_list[i].extensions[j].c_str());
        }
    }

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_5_var dev = Device_5::_duplicate(device_5);
            dev->set_pipe_config_5(pipe_config_list, get_client_identification());

            ctr = 2;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_pipe_config on device " << device_name << std::ends;

                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                throw;
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "set_pipe_config", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "set_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_pipe_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "set_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_pipe_config on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute set_pipe_config on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_pipe_list() - get list of pipes
//
//-----------------------------------------------------------------------------

std::vector<std::string> *DeviceProxy::get_pipe_list()
{
    std::vector<std::string> all_pipe;
    PipeInfoList *all_pipe_config;

    all_pipe.emplace_back(AllPipe);
    all_pipe_config = get_pipe_config(all_pipe);

    auto *pipe_list = new std::vector<std::string>;
    pipe_list->resize(all_pipe_config->size());
    for(unsigned int i = 0; i < all_pipe_config->size(); i++)
    {
        (*pipe_list)[i] = (*all_pipe_config)[i].name;
    }
    delete all_pipe_config;

    return pipe_list;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::read_pipe() - read a single pipe
//
//-----------------------------------------------------------------------------

DevicePipe DeviceProxy::read_pipe(const std::string &pipe_name)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}, {"tango.operation.argument", pipe_name}}));

    DevPipeData_var pipe_value_5;
    DevicePipe dev_pipe;
    int ctr = 0;

    //
    // Error if device does not support IDL 5
    //

    if(detail::IDLVersionIsTooOld(version, 5))
    {
        std::stringstream ss;
        ss << "Device " << device_name << " too old to use read_pipe() call. Please upgrade to Tango 9/IDL5";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, ss.str());
    }

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_5_var dev = Device_5::_duplicate(device_5);
            pipe_value_5 = dev->read_pipe_5(pipe_name.c_str(), get_client_identification());

            ctr = 2;
        }
        catch(Tango::ConnectionFailed &e)
        {
            std::stringstream desc;
            desc << "Failed to read_pipe on device " << device_name << ", pipe " << pipe_name;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_PipeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            std::stringstream desc;
            desc << "Failed to read_pipe on device " << device_name << ", pipe " << pipe_name;
            TANGO_RETHROW_EXCEPTION(e, API_PipeFailed, desc.str());
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "read_pipe", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                std::stringstream desc;
                desc << "Failed to read_pipe on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                std::stringstream desc;
                desc << "Failed to read_pipe on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            std::stringstream desc;
            desc << "Failed to read_pipe on device " << device_name;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    //
    // Pass received data to the caller.
    // For thw data elt sequence, we create a new one with size and buffer from the original one.
    // This is required because the whole object received by the call will be deleted at the end of this method
    //

    dev_pipe.set_name(pipe_value_5->name.in());
    dev_pipe.set_time(pipe_value_5->time);

    CORBA::ULong max, len;
    max = pipe_value_5->data_blob.blob_data.maximum();
    len = pipe_value_5->data_blob.blob_data.length();
    DevPipeDataElt *buf = pipe_value_5->data_blob.blob_data.get_buffer((CORBA::Boolean) true);
    auto *dvpdea = new DevVarPipeDataEltArray(max, len, buf, true);

    dev_pipe.get_root_blob().reset_extract_ctr();
    dev_pipe.get_root_blob().reset_insert_ctr();
    dev_pipe.get_root_blob().set_name(pipe_value_5->data_blob.name.in());
    delete dev_pipe.get_root_blob().get_extract_data();
    dev_pipe.get_root_blob().set_extract_data(dvpdea);
    dev_pipe.get_root_blob().set_extract_delete(true);

    return dev_pipe;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_pipe() - write a single pipe
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_pipe(DevicePipe &dev_pipe)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", dev_pipe.get_name()}}));

    DevPipeData pipe_value_5;
    int ctr = 0;

    //
    // Error if device does not support IDL 5
    //

    if(detail::IDLVersionIsTooOld(version, 5))
    {
        std::stringstream ss;
        ss << "Device " << device_name << " too old to use write_pipe() call. Please upgrade to Tango 9/IDL5";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, ss.str());
    }

    //
    // Prepare data sent to device
    //

    pipe_value_5.name = dev_pipe.get_name().c_str();
    const std::string &bl_name = dev_pipe.get_root_blob().get_name();
    if(bl_name.size() != 0)
    {
        pipe_value_5.data_blob.name = bl_name.c_str();
    }

    DevVarPipeDataEltArray *tmp_ptr = dev_pipe.get_root_blob().get_insert_data();
    if(tmp_ptr == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_PipeNoDataElement, "No data in pipe!");
    }

    CORBA::ULong max, len;
    max = tmp_ptr->maximum();
    len = tmp_ptr->length();
    pipe_value_5.data_blob.blob_data.replace(max, len, tmp_ptr->get_buffer((CORBA::Boolean) true), true);

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_5_var dev = Device_5::_duplicate(device_5);
            dev->write_pipe_5(pipe_value_5, get_client_identification());

            ctr = 2;
        }
        catch(Tango::ConnectionFailed &e)
        {
            dev_pipe.get_root_blob().reset_insert_ctr();
            delete tmp_ptr;

            std::stringstream desc;
            desc << "Failed to write_pipe on device " << device_name << ", pipe " << dev_pipe.get_name();
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_PipeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            dev_pipe.get_root_blob().reset_insert_ctr();
            delete tmp_ptr;

            std::stringstream desc;
            desc << "Failed to write_pipe on device " << device_name << ", pipe " << dev_pipe.get_name();
            TANGO_RETHROW_EXCEPTION(e, API_PipeFailed, desc.str());
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_pipe", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_pipe", this);
            }
            else
            {
                dev_pipe.get_root_blob().reset_insert_ctr();
                delete tmp_ptr;

                set_connection_state(CONNECTION_NOTOK);
                std::stringstream desc;
                desc << "Failed to write_pipe on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_pipe", this);
            }
            else
            {
                dev_pipe.get_root_blob().reset_insert_ctr();
                delete tmp_ptr;

                set_connection_state(CONNECTION_NOTOK);
                std::stringstream desc;
                desc << "Failed to write_pipe on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            dev_pipe.get_root_blob().reset_insert_ctr();
            delete tmp_ptr;

            set_connection_state(CONNECTION_NOTOK);
            std::stringstream desc;
            desc << "Failed to write_pipe on device " << device_name;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    dev_pipe.get_root_blob().reset_insert_ctr();
    delete tmp_ptr;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_read_pipe() - write then read a single pipe
//
//-----------------------------------------------------------------------------

DevicePipe DeviceProxy::write_read_pipe(DevicePipe &pipe_data)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", pipe_data.get_name()}}));

    DevPipeData pipe_value_5;
    DevPipeData_var r_pipe_value_5;
    DevicePipe r_dev_pipe;
    int ctr = 0;

    //
    // Error if device does not support IDL 5
    //

    if(detail::IDLVersionIsTooOld(version, 5))
    {
        std::stringstream ss;
        ss << "Device " << device_name << " too old to use write_read_pipe() call. Please upgrade to Tango 9/IDL5";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, ss.str());
    }

    //
    // Prepare data sent to device
    //

    pipe_value_5.name = pipe_data.get_name().c_str();
    const std::string &bl_name = pipe_data.get_root_blob().get_name();
    if(bl_name.size() != 0)
    {
        pipe_value_5.data_blob.name = bl_name.c_str();
    }

    DevVarPipeDataEltArray *tmp_ptr = pipe_data.get_root_blob().get_insert_data();
    CORBA::ULong max, len;
    max = tmp_ptr->maximum();
    len = tmp_ptr->length();
    pipe_value_5.data_blob.blob_data.replace(max, len, tmp_ptr->get_buffer((CORBA::Boolean) true), true);

    delete tmp_ptr;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_5_var dev = Device_5::_duplicate(device_5);
            r_pipe_value_5 = dev->write_read_pipe_5(pipe_value_5, get_client_identification());

            ctr = 2;
        }
        catch(Tango::ConnectionFailed &e)
        {
            std::stringstream desc;
            desc << "Failed to write_read_pipe on device " << device_name << ", pipe " << pipe_data.get_name();
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_PipeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            std::stringstream desc;
            desc << "Failed to write_pipe on device " << device_name << ", pipe " << pipe_data.get_name();
            TANGO_RETHROW_EXCEPTION(e, API_PipeFailed, desc.str());
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_read_pipe", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                std::stringstream desc;
                desc << "Failed to write_read_pipe on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                std::stringstream desc;
                desc << "Failed to write_read_pipe on device " << device_name;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            std::stringstream desc;
            desc << "Failed to write_read_pipe on device " << device_name;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    //
    // Pass received data to the caller.
    // For thw data elt sequence, we create a new one with size and buffer from the original one.
    // This is required because the whole object received by the call will be deleted at the end of this method
    //

    r_dev_pipe.set_name(r_pipe_value_5->name.in());
    r_dev_pipe.set_time(r_pipe_value_5->time);

    max = r_pipe_value_5->data_blob.blob_data.maximum();
    len = r_pipe_value_5->data_blob.blob_data.length();
    DevPipeDataElt *buf = r_pipe_value_5->data_blob.blob_data.get_buffer((CORBA::Boolean) true);
    auto *dvpdea = new DevVarPipeDataEltArray(max, len, buf, true);

    r_dev_pipe.get_root_blob().reset_extract_ctr();
    r_dev_pipe.get_root_blob().reset_insert_ctr();
    r_dev_pipe.get_root_blob().set_name(r_pipe_value_5->data_blob.name.in());
    r_dev_pipe.get_root_blob().set_extract_data(dvpdea);
    r_dev_pipe.get_root_blob().set_extract_delete(true);

    return r_dev_pipe;

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::read_attributes() - Read attributes
//
//-----------------------------------------------------------------------------

std::vector<DeviceAttribute> *DeviceProxy::read_attributes(const std::vector<std::string> &attr_string_list)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    AttributeValueList_var attr_value_list;
    AttributeValueList_3_var attr_value_list_3;
    AttributeValueList_4_var attr_value_list_4;
    AttributeValueList_5_var attr_value_list_5;
    DevVarStringArray attr_list;

    //
    // Check that the caller did not give two times the same attribute
    //

    same_att_name(attr_string_list, "Deviceproxy::read_attributes()");
    unsigned long i;

    attr_list.length(attr_string_list.size());
    for(i = 0; i < attr_string_list.size(); i++)
    {
        attr_list[i] = string_dup(attr_string_list[i].c_str());
    }

    int ctr = 0;
    Tango::DevSource local_source;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            if(version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->read_attributes_5(attr_list, local_source, get_client_identification());
            }
            else if(version == 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->read_attributes_4(attr_list, local_source, get_client_identification());
            }
            else if(version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                attr_value_list_3 = dev->read_attributes_3(attr_list, local_source);
            }
            else if(version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_value_list = dev->read_attributes_2(attr_list, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                attr_value_list = dev->read_attributes(attr_list);
            }

            ctr = 2;
        }
        catch(Tango::ConnectionFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attributes on device " << device_name;
            desc << ", attributes ";
            int nb_attr = attr_string_list.size();
            for(int i = 0; i < nb_attr; i++)
            {
                desc << attr_string_list[i];
                if(i != nb_attr - 1)
                {
                    desc << ", ";
                }
            }
            desc << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiConnExcept, e, API_AttributeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attributes on device " << device_name;
            desc << ", attributes ";
            int nb_attr = attr_string_list.size();
            for(int i = 0; i < nb_attr; i++)
            {
                desc << attr_string_list[i];
                if(i != nb_attr - 1)
                {
                    desc << ", ";
                }
            }
            desc << std::ends;
            TANGO_RETHROW_EXCEPTION(e, API_AttributeFailed, desc.str());
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "read_attributes", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute read_attributes on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute read_attributes on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            TangoSys_OMemStream desc;
            desc << "Failed to execute read_attributes on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    unsigned long nb_received;
    if(version >= 5)
    {
        nb_received = attr_value_list_5->length();
    }
    else if(version == 4)
    {
        nb_received = attr_value_list_4->length();
    }
    else if(version == 3)
    {
        nb_received = attr_value_list_3->length();
    }
    else
    {
        nb_received = attr_value_list->length();
    }

    auto *dev_attr = new(std::vector<DeviceAttribute>);
    dev_attr->resize(nb_received);

    for(i = 0; i < nb_received; i++)
    {
        if(version >= 3)
        {
            if(version >= 5)
            {
                ApiUtil::attr_to_device(&(attr_value_list_5[i]), version, &(*dev_attr)[i]);
            }
            else if(version == 4)
            {
                ApiUtil::attr_to_device(&(attr_value_list_4[i]), version, &(*dev_attr)[i]);
            }
            else
            {
                ApiUtil::attr_to_device(nullptr, &(attr_value_list_3[i]), version, &(*dev_attr)[i]);
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
            ApiUtil::attr_to_device(&(attr_value_list[i]), nullptr, version, &(*dev_attr)[i]);
        }
    }

    return (dev_attr);

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::read_attribute() - return a single attribute
//
//-----------------------------------------------------------------------------

DeviceAttribute DeviceProxy::read_attribute(const std::string &attr_string)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}, {"tango.operation.argument", attr_string}}));

    AttributeValueList_var attr_value_list;
    AttributeValueList_3_var attr_value_list_3;
    AttributeValueList_4_var attr_value_list_4;
    AttributeValueList_5_var attr_value_list_5;
    DeviceAttribute dev_attr;
    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    attr_list.length(1);
    attr_list[0] = CORBA::string_dup(attr_string.c_str());

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            if(version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->read_attributes_5(attr_list, local_source, get_client_identification());
            }
            else if(version == 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->read_attributes_4(attr_list, local_source, get_client_identification());
            }
            else if(version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                attr_value_list_3 = dev->read_attributes_3(attr_list, local_source);
            }
            else if(version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_value_list = dev->read_attributes_2(attr_list, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                attr_value_list = dev->read_attributes(attr_list);
            }
            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_string, this)
    }

    if(version >= 3)
    {
        if(version >= 5)
        {
            ApiUtil::attr_to_device(&(attr_value_list_5[0]), version, &dev_attr);
        }
        else if(version == 4)
        {
            ApiUtil::attr_to_device(&(attr_value_list_4[0]), version, &dev_attr);
        }
        else
        {
            ApiUtil::attr_to_device(nullptr, &(attr_value_list_3[0]), version, &dev_attr);
        }

        //
        // Add an error in the error stack in case there is one
        //

        DevErrorList_var &err_list = dev_attr.get_error_list();
        long nb_except = err_list.in().length();
        if(nb_except != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attribute on device " << device_name;
            desc << ", attribute " << dev_attr.name << std::ends;

            err_list.inout().length(nb_except + 1);
            err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
            err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string st = desc.str();
            err_list[nb_except].desc = Tango::string_dup(st.c_str());
            err_list[nb_except].severity = Tango::ERR;
            dev_attr.data_type = Tango::DATA_TYPE_UNKNOWN;
        }
    }
    else
    {
        ApiUtil::attr_to_device(&(attr_value_list[0]), nullptr, version, &dev_attr);
    }

    return (dev_attr);

    TANGO_TELEMETRY_TRACE_END();
}

void DeviceProxy::read_attribute(const char *attr_str, DeviceAttribute &dev_attr)
{
    TANGO_TELEMETRY_TRACE_BEGIN((Tango::telemetry::Attributes{{"tango.operation.target", dev_name()},
                                                              {"tango.operation.argument", std::string(attr_str)}}));

    AttributeValueList *attr_value_list = nullptr;
    AttributeValueList_3 *attr_value_list_3 = nullptr;
    AttributeValueList_4 *attr_value_list_4 = nullptr;
    AttributeValueList_5 *attr_value_list_5 = nullptr;
    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    attr_list.length(1);
    attr_list[0] = CORBA::string_dup(attr_str);

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            if(version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->read_attributes_5(attr_list, local_source, get_client_identification());
            }
            else if(version == 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->read_attributes_4(attr_list, local_source, get_client_identification());
            }
            else if(version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                attr_value_list_3 = dev->read_attributes_3(attr_list, local_source);
            }
            else if(version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_value_list = dev->read_attributes_2(attr_list, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                attr_value_list = dev->read_attributes(attr_list);
            }
            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_str, this)
    }

    if(version >= 3)
    {
        if(version >= 5)
        {
            ApiUtil::attr_to_device(&((*attr_value_list_5)[0]), version, &dev_attr);
            delete attr_value_list_5;
        }
        else if(version == 4)
        {
            ApiUtil::attr_to_device(&((*attr_value_list_4)[0]), version, &dev_attr);
            delete attr_value_list_4;
        }
        else
        {
            ApiUtil::attr_to_device(nullptr, &((*attr_value_list_3)[0]), version, &dev_attr);
            delete attr_value_list_3;
        }

        //
        // Add an error in the error stack in case there is one
        //

        DevErrorList_var &err_list = dev_attr.get_error_list();
        long nb_except = err_list.in().length();
        if(nb_except != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attribute on device " << device_name;
            desc << ", attribute " << dev_attr.name << std::ends;

            err_list.inout().length(nb_except + 1);
            err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
            err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string st = desc.str();
            err_list[nb_except].desc = Tango::string_dup(st.c_str());
            err_list[nb_except].severity = Tango::ERR;
            dev_attr.data_type = Tango::DATA_TYPE_UNKNOWN;
        }
    }
    else
    {
        ApiUtil::attr_to_device(&((*attr_value_list)[0]), nullptr, version, &dev_attr);
        delete attr_value_list;
    }

    TANGO_TELEMETRY_TRACE_END();
}

void DeviceProxy::read_attribute(const std::string &attr_str, AttributeValue_4 *&av_4)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", std::string(attr_str)}}));

    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    if(detail::IDLVersionIsTooOld(version, 4))
    {
        std::stringstream ss;
        ss << "Device " << dev_name()
           << " is too old to support this call. Please, update to IDL 4 (Tango 7.x or more)";
        TANGO_THROW_EXCEPTION(API_NotSupported, ss.str());
    }

    attr_list.length(1);
    attr_list[0] = CORBA::string_dup(attr_str.c_str());

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            Device_4_var dev = Device_4::_duplicate(device_4);
            AttributeValueList_4 *attr_value_list_4 =
                dev->read_attributes_4(attr_list, local_source, get_client_identification());

            av_4 = attr_value_list_4->get_buffer(true);
            delete attr_value_list_4;

            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_str, this)
    }

    //
    // Add an error in the error stack in case there is one
    //

    long nb_except = av_4->err_list.length();
    if(nb_except != 0)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to read_attribute on device " << device_name;
        desc << ", attribute " << attr_str << std::ends;

        av_4->err_list.length(nb_except + 1);
        av_4->err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
        av_4->err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

        std::string st = desc.str();
        av_4->err_list[nb_except].desc = Tango::string_dup(st.c_str());
        av_4->err_list[nb_except].severity = Tango::ERR;
    }

    TANGO_TELEMETRY_TRACE_END();
}

void DeviceProxy::read_attribute(const std::string &attr_str, AttributeValue_5 *&av_5)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", std::string(attr_str)}}));

    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    if(detail::IDLVersionIsTooOld(version, 5))
    {
        std::stringstream ss;
        ss << "Device " << dev_name()
           << " is too old to support this call. Please, update to IDL 5 (Tango 9.x or more)";
        TANGO_THROW_EXCEPTION(API_NotSupported, ss.str());
    }

    attr_list.length(1);
    attr_list[0] = CORBA::string_dup(attr_str.c_str());

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            Device_5_var dev = Device_5::_duplicate(device_5);
            AttributeValueList_5 *attr_value_list_5 =
                dev->read_attributes_5(attr_list, local_source, get_client_identification());

            av_5 = attr_value_list_5->get_buffer(true);
            delete attr_value_list_5;

            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_str, this)
    }

    //
    // Add an error in the error stack in case there is one
    //

    long nb_except = av_5->err_list.length();
    if(nb_except != 0)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to read_attribute on device " << device_name;
        desc << ", attribute " << attr_str << std::ends;

        av_5->err_list.length(nb_except + 1);
        av_5->err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
        av_5->err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

        std::string st = desc.str();
        av_5->err_list[nb_except].desc = Tango::string_dup(st.c_str());
        av_5->err_list[nb_except].severity = Tango::ERR;
    }

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_attributes() - write a list of attributes
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attributes(const std::vector<DeviceAttribute> &attr_list)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    AttributeValueList attr_value_list;
    AttributeValueList_4 attr_value_list_4;

    Tango::AccessControlType local_act;

    if(version == detail::INVALID_IDL_VERSION)
    {
        check_and_reconnect(local_act);
    }

    if(version >= 4)
    {
        attr_value_list_4.length(attr_list.size());
    }
    else
    {
        attr_value_list.length(attr_list.size());
    }

    for(unsigned int i = 0; i < attr_list.size(); i++)
    {
        if(version >= 4)
        {
            attr_value_list_4[i].name = attr_list[i].name.c_str();
            attr_value_list_4[i].quality = attr_list[i].quality;
            attr_value_list_4[i].data_format = attr_list[i].data_format;
            attr_value_list_4[i].time = attr_list[i].time;
            attr_value_list_4[i].w_dim.dim_x = attr_list[i].dim_x;
            attr_value_list_4[i].w_dim.dim_y = attr_list[i].dim_y;
        }
        else
        {
            attr_value_list[i].name = attr_list[i].name.c_str();
            attr_value_list[i].quality = attr_list[i].quality;
            attr_value_list[i].time = attr_list[i].time;
            attr_value_list[i].dim_x = attr_list[i].dim_x;
            attr_value_list[i].dim_y = attr_list[i].dim_y;
        }

        if(attr_list[i].LongSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.long_att_value(attr_list[i].LongSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].LongSeq.in();
            }
            continue;
        }
        if(attr_list[i].Long64Seq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.long64_att_value(attr_list[i].Long64Seq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].Long64Seq.in();
            }
            continue;
        }
        if(attr_list[i].ShortSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.short_att_value(attr_list[i].ShortSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].ShortSeq.in();
            }
            continue;
        }
        if(attr_list[i].DoubleSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.double_att_value(attr_list[i].DoubleSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].DoubleSeq.in();
            }
            continue;
        }
        if(attr_list[i].StringSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.string_att_value(attr_list[i].StringSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].StringSeq.in();
            }
            continue;
        }
        if(attr_list[i].FloatSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.float_att_value(attr_list[i].FloatSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].FloatSeq.in();
            }
            continue;
        }
        if(attr_list[i].BooleanSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.bool_att_value(attr_list[i].BooleanSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].BooleanSeq.in();
            }
            continue;
        }
        if(attr_list[i].UShortSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.ushort_att_value(attr_list[i].UShortSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].UShortSeq.in();
            }
            continue;
        }
        if(attr_list[i].UCharSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.uchar_att_value(attr_list[i].UCharSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].UCharSeq.in();
            }
            continue;
        }
        if(attr_list[i].ULongSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.ulong_att_value(attr_list[i].ULongSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].ULongSeq.in();
            }
            continue;
        }
        if(attr_list[i].ULong64Seq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.ulong64_att_value(attr_list[i].ULong64Seq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].ULong64Seq.in();
            }
            continue;
        }
        if(attr_list[i].StateSeq.operator->() != nullptr)
        {
            if(version >= 4)
            {
                attr_value_list_4[i].value.state_att_value(attr_list[i].StateSeq.in());
            }
            else
            {
                attr_value_list[i].value <<= attr_list[i].StateSeq.in();
            }
            continue;
        }
    }

    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

            //
            // Throw exception if caller not allowed to write_attribute
            //

            if(local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
            }

            //
            // Now, write the attribute(s)
            //

            if(version >= 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                dev->write_attributes_4(attr_value_list_4, get_client_identification());
            }
            else if(version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->write_attributes_3(attr_value_list);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev->write_attributes(attr_value_list);
            }
            ctr = 2;
        }
        catch(Tango::MultiDevFailed &e)
        {
            throw Tango::NamedDevFailedList(
                e, device_name, (const char *) "DeviceProxy::write_attributes", (const char *) API_AttributeFailed);
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attributes on device " << device_name;
            desc << ", attributes ";
            int nb_attr = attr_value_list.length();
            for(int i = 0; i < nb_attr; i++)
            {
                desc << attr_value_list[i].name.in();
                if(i != nb_attr - 1)
                {
                    desc << ", ";
                }
            }
            desc << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_AttributeFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attributes", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_attribute() - write a single attribute
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attribute(const DeviceAttribute &dev_attr)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", dev_attr.name}}));

    AttributeValueList attr_value_list;
    AttributeValueList_4 attr_value_list_4;
    Tango::AccessControlType local_act;

    if(version == detail::INVALID_IDL_VERSION)
    {
        check_and_reconnect(local_act);
    }

    if(version >= 4)
    {
        attr_value_list_4.length(1);

        attr_value_list_4[0].name = dev_attr.name.c_str();
        attr_value_list_4[0].quality = dev_attr.quality;
        attr_value_list_4[0].data_format = dev_attr.data_format;
        attr_value_list_4[0].time = dev_attr.time;
        attr_value_list_4[0].w_dim.dim_x = dev_attr.dim_x;
        attr_value_list_4[0].w_dim.dim_y = dev_attr.dim_y;

        if(dev_attr.LongSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.long_att_value(dev_attr.LongSeq.in());
        }
        else if(dev_attr.Long64Seq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.long64_att_value(dev_attr.Long64Seq.in());
        }
        else if(dev_attr.ShortSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.short_att_value(dev_attr.ShortSeq.in());
        }
        else if(dev_attr.DoubleSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.double_att_value(dev_attr.DoubleSeq.in());
        }
        else if(dev_attr.StringSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.string_att_value(dev_attr.StringSeq.in());
        }
        else if(dev_attr.FloatSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.float_att_value(dev_attr.FloatSeq.in());
        }
        else if(dev_attr.BooleanSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.bool_att_value(dev_attr.BooleanSeq.in());
        }
        else if(dev_attr.UShortSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.ushort_att_value(dev_attr.UShortSeq.in());
        }
        else if(dev_attr.UCharSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.uchar_att_value(dev_attr.UCharSeq.in());
        }
        else if(dev_attr.ULongSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.ulong_att_value(dev_attr.ULongSeq.in());
        }
        else if(dev_attr.ULong64Seq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.ulong64_att_value(dev_attr.ULong64Seq.in());
        }
        else if(dev_attr.StateSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.state_att_value(dev_attr.StateSeq.in());
        }
        else if(dev_attr.EncodedSeq.operator->() != nullptr)
        {
            attr_value_list_4[0].value.encoded_att_value(dev_attr.EncodedSeq.in());
        }
    }
    else
    {
        attr_value_list.length(1);

        attr_value_list[0].name = dev_attr.name.c_str();
        attr_value_list[0].quality = dev_attr.quality;
        attr_value_list[0].time = dev_attr.time;
        attr_value_list[0].dim_x = dev_attr.dim_x;
        attr_value_list[0].dim_y = dev_attr.dim_y;

        if(dev_attr.LongSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.LongSeq.in();
        }
        else if(dev_attr.Long64Seq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.Long64Seq.in();
        }
        else if(dev_attr.ShortSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.ShortSeq.in();
        }
        else if(dev_attr.DoubleSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.DoubleSeq.in();
        }
        else if(dev_attr.StringSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.StringSeq.in();
        }
        else if(dev_attr.FloatSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.FloatSeq.in();
        }
        else if(dev_attr.BooleanSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.BooleanSeq.in();
        }
        else if(dev_attr.UShortSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.UShortSeq.in();
        }
        else if(dev_attr.UCharSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.UCharSeq.in();
        }
        else if(dev_attr.ULongSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.ULongSeq.in();
        }
        else if(dev_attr.ULong64Seq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.ULong64Seq.in();
        }
        else if(dev_attr.StateSeq.operator->() != nullptr)
        {
            attr_value_list[0].value <<= dev_attr.StateSeq.in();
        }
    }

    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

            //
            // Throw exception if caller not allowed to write_attribute
            //

            if(local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
            }

            //
            // Now, write the attribute(s)
            //

            if(version >= 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                dev->write_attributes_4(attr_value_list_4, get_client_identification());
            }
            else if(version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->write_attributes_3(attr_value_list);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev->write_attributes(attr_value_list);
            }
            ctr = 2;
        }
        catch(Tango::MultiDevFailed &e)
        {
            //
            // Transfer this exception into a DevFailed exception
            //

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << dev_attr.name;
            desc << std::ends;
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << dev_attr.name;
            desc << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_AttributeFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attribute()", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_attribute() - write attribute(s) using the CORBA data type
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attribute(const AttributeValueList &attr_val)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", attr_val[0].name.in()}}));

    int ctr = 0;
    Tango::AccessControlType local_act;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

            //
            // Throw exception if caller not allowed to write_attribute
            //

            if(local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
            }

            //
            // Now, write the attribute(s)
            //

            if(version >= 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->write_attributes_3(attr_val);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev->write_attributes(attr_val);
            }
            ctr = 2;
        }
        catch(Tango::MultiDevFailed &e)
        {
            //
            // Transfer this exception into a DevFailed exception
            //

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << std::ends;
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_AttributeFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attribute()", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    TANGO_TELEMETRY_TRACE_END();
}

void DeviceProxy::write_attribute(const AttributeValueList_4 &attr_val)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", attr_val[0].name.in()}}));

    Tango::AccessControlType local_act;

    if(version == detail::INVALID_IDL_VERSION)
    {
        check_and_reconnect(local_act);
    }

    //
    // Check that the device supports IDL V4
    //

    if(detail::IDLVersionIsTooOld(version, 4))
    {
        TangoSys_OMemStream desc;
        desc << "Failed to write_attribute on device " << device_name;
        desc << ", attribute ";
        desc << attr_val[0].name.in();
        desc << ". The device does not support thi stype of data (Bad IDL release)";
        desc << std::ends;
        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, desc.str());
    }

    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

            //
            // Throw exception if caller not allowed to write_attribute
            //

            if(local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
            }

            //
            // Now, write the attribute(s)
            //

            Device_4_var dev = Device_4::_duplicate(device_4);
            dev->write_attributes_4(attr_val, get_client_identification());

            ctr = 2;
        }
        catch(Tango::MultiDevFailed &e)
        {
            //
            // Transfer this exception into a DevFailed exception
            //

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << std::ends;
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_AttributeFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attribute()", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_list() - get list of attributes
//
//-----------------------------------------------------------------------------

std::vector<std::string> *DeviceProxy::get_attribute_list()
{
    std::vector<std::string> all_attr;
    AttributeInfoListEx *all_attr_config;

    all_attr.emplace_back(AllAttr_3);
    all_attr_config = get_attribute_config_ex(all_attr);

    auto *attr_list = new std::vector<std::string>;
    attr_list->resize(all_attr_config->size());
    for(unsigned int i = 0; i < all_attr_config->size(); i++)
    {
        (*attr_list)[i] = (*all_attr_config)[i].name;
    }
    delete all_attr_config;

    return attr_list;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::attribute_list_query() - get list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoList *DeviceProxy::attribute_list_query()
{
    std::vector<std::string> all_attr;
    AttributeInfoList *all_attr_config;

    all_attr.emplace_back(AllAttr_3);
    all_attr_config = get_attribute_config(all_attr);

    return all_attr_config;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::attribute_list_query_ex() - get list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoListEx *DeviceProxy::attribute_list_query_ex()
{
    std::vector<std::string> all_attr;
    AttributeInfoListEx *all_attr_config;

    all_attr.emplace_back(AllAttr_3);
    all_attr_config = get_attribute_config_ex(all_attr);

    return all_attr_config;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::command_history() - get command history (only for polled command)
//
//-----------------------------------------------------------------------------

std::vector<DeviceDataHistory> *DeviceProxy::command_history(const std::string &cmd_name, int depth)
{
    if(version == 1)
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support command_history feature" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, desc.str());
    }

    DevCmdHistoryList *hist = nullptr;
    DevCmdHistory_4_var hist_4 = nullptr;

    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if(version <= 3)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                hist = dev->command_inout_history_2(cmd_name.c_str(), depth);
            }
            else
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                hist_4 = dev->command_inout_history_4(cmd_name.c_str(), depth);
            }
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "command_history", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "command_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Command_history failed on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "command_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Command_history failed on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Command_history failed on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    auto *ddh = new std::vector<DeviceDataHistory>;

    if(version <= 3)
    {
        int *ctr_ptr = new int;
        *ctr_ptr = 0;

        ddh->reserve(hist->length());

        for(unsigned int i = 0; i < hist->length(); i++)
        {
            ddh->emplace_back(i, ctr_ptr, hist);
        }
    }
    else
    {
        ddh->reserve(hist_4->dates.length());
        for(unsigned int i = 0; i < hist_4->dates.length(); i++)
        {
            ddh->emplace_back();
        }
        from_hist4_2_DataHistory(hist_4, ddh);
    }

    return ddh;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::attribute_history() - get attribute history
//                      (only for polled attribute)
//
//-----------------------------------------------------------------------------

std::vector<DeviceAttributeHistory> *DeviceProxy::attribute_history(const std::string &cmd_name, int depth)
{
    if(version == 1)
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support attribute_history feature" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, desc.str());
    }

    DevAttrHistoryList_var hist;
    DevAttrHistoryList_3_var hist_3;
    DevAttrHistory_4_var hist_4;
    DevAttrHistory_5_var hist_5;

    int ctr = 0;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if(version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                hist = device_2->read_attribute_history_2(cmd_name.c_str(), depth);
            }
            else
            {
                if(version == 3)
                {
                    Device_3_var dev = Device_3::_duplicate(device_3);
                    hist_3 = dev->read_attribute_history_3(cmd_name.c_str(), depth);
                }
                else if(version == 4)
                {
                    Device_4_var dev = Device_4::_duplicate(device_4);
                    hist_4 = dev->read_attribute_history_4(cmd_name.c_str(), depth);
                }
                else
                {
                    Device_5_var dev = Device_5::_duplicate(device_5);
                    hist_5 = dev->read_attribute_history_5(cmd_name.c_str(), depth);
                }
            }
            ctr = 2;
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "attribute_history", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "attribute_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Attribute_history failed on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "attribute_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Attribute_history failed on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            TangoSys_OMemStream desc;
            desc << "Attribute_history failed on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    auto *ddh = new std::vector<DeviceAttributeHistory>;

    if(version > 4)
    {
        ddh->reserve(hist_5->dates.length());
        for(unsigned int i = 0; i < hist_5->dates.length(); i++)
        {
            ddh->emplace_back();
        }
        from_hist_2_AttHistory(hist_5, ddh);
        for(unsigned int i = 0; i < hist_5->dates.length(); i++)
        {
            (*ddh)[i].data_type = hist_5->data_type;
        }
    }
    else if(version == 4)
    {
        ddh->reserve(hist_4->dates.length());
        for(unsigned int i = 0; i < hist_4->dates.length(); i++)
        {
            ddh->emplace_back();
        }
        from_hist_2_AttHistory(hist_4, ddh);
    }
    else if(version == 3)
    {
        ddh->reserve(hist_3->length());
        for(unsigned int i = 0; i < hist_3->length(); i++)
        {
            ddh->emplace_back(i, hist_3);
        }
    }
    else
    {
        ddh->reserve(hist->length());
        for(unsigned int i = 0; i < hist->length(); i++)
        {
            ddh->emplace_back(i, hist);
        }
    }

    return ddh;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::polling_status() - get device polling status
//-----------------------------------------------------------------------------

std::vector<std::string> *DeviceProxy::polling_status()
{
    DeviceData dout, din;
    std::string cmd("DevPollStatus");
    din.any <<= device_name.c_str();

    auto &admin_device = get_admin_device();
    //
    // In case of connection failed error, do a re-try
    //

    try
    {
        dout = admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        dout = admin_device.command_inout(cmd, din);
    }

    const DevVarStringArray *out_str;
    dout >> out_str;

    auto *poll_stat = new std::vector<std::string>;
    poll_stat->reserve(out_str->length());

    for(unsigned int i = 0; i < out_str->length(); i++)
    {
        std::string str = (*out_str)[i].in();
        poll_stat->push_back(str);
    }
    return poll_stat;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_polled() - return true if the object "obj_name" is polled.
//                  In this case, the upd string is initialised with
//                  the polling period.
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_polled(polled_object obj, const std::string &obj_name, std::string &upd)
{
    bool ret = false;
    std::vector<std::string> *poll_str;

    poll_str = polling_status();
    if(poll_str->empty())
    {
        delete poll_str;
        return ret;
    }

    std::string loc_obj_name(obj_name);
    std::transform(loc_obj_name.begin(), loc_obj_name.end(), loc_obj_name.begin(), ::tolower);

    for(unsigned int i = 0; i < poll_str->size(); i++)
    {
        std::string &tmp_str = (*poll_str)[i];
        std::string::size_type pos, end;
        pos = tmp_str.find(' ');
        pos++;
        end = tmp_str.find(' ', pos + 1);
        std::string obj_type = tmp_str.substr(pos, end - pos);
        if(obj_type == "command")
        {
            if(obj == Attr)
            {
                continue;
            }
        }
        else if(obj_type == "attribute")
        {
            if(obj == Cmd)
            {
                if((loc_obj_name != "state") && (loc_obj_name != "status"))
                {
                    continue;
                }
            }
        }

        pos = tmp_str.find('=');
        pos = pos + 2;
        end = tmp_str.find(". S", pos + 1);
        if(end == std::string::npos)
        {
            end = tmp_str.find('\n', pos + 1);
        }
        std::string name = tmp_str.substr(pos, end - pos);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if(name == loc_obj_name)
        {
            //
            // Now that it's found, search for its polling period
            //

            pos = tmp_str.find("triggered", end);
            if(pos != std::string::npos)
            {
                ret = true;
                upd = "0";
                break;
            }
            else
            {
                pos = tmp_str.find('=', end);
                pos = pos + 2;
                end = tmp_str.find('\n', pos + 1);
                std::string per = tmp_str.substr(pos, end - pos);
                upd = per;
                ret = true;
                break;
            }
        }
    }

    delete poll_str;

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_command_poll_period() - Return command polling period
//                        (in mS)
//
//-----------------------------------------------------------------------------

int DeviceProxy::get_command_poll_period(const std::string &cmd_name)
{
    std::string poll_per;
    bool poll = is_polled(Cmd, cmd_name, poll_per);

    int ret;
    if(poll)
    {
        TangoSys_MemStream stream;

        stream << poll_per << std::ends;
        stream >> ret;
    }
    else
    {
        ret = 0;
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_poll_period() - Return attribute polling period
//                        (in mS)
//
//-----------------------------------------------------------------------------

int DeviceProxy::get_attribute_poll_period(const std::string &attr_name)
{
    std::string poll_per;
    bool poll = is_polled(Attr, attr_name, poll_per);

    int ret;
    if(poll)
    {
        TangoSys_MemStream stream;

        stream << poll_per << std::ends;
        stream >> ret;
    }
    else
    {
        ret = 0;
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::poll_command() - If object is already polled, just update its
//                 polling period. If object is not polled, add
//                 it to the list of polled objects
//
//-----------------------------------------------------------------------------

void DeviceProxy::poll_command(const std::string &cmd_name, int period)
{
    std::string poll_per;
    bool poll = is_polled(Cmd, cmd_name, poll_per);
    auto &admin_device = get_admin_device();

    DevVarLongStringArray in;
    in.lvalue.length(1);
    in.svalue.length(3);

    in.svalue[0] = CORBA::string_dup(device_name.c_str());
    in.svalue[1] = CORBA::string_dup("command");
    in.svalue[2] = CORBA::string_dup(cmd_name.c_str());
    in.lvalue[0] = period;

    if(poll)
    {
        //
        // If object is polled and the polling period is the same, simply retruns
        //
        TangoSys_MemStream stream;
        int per;

        stream << poll_per << std::ends;
        stream >> per;

        if((per == period) || (per == 0))
        {
            return;
        }
        else
        {
            //
            // If object is polled, this is an update of the polling period
            //

            DeviceData din;
            std::string cmd("UpdObjPollingPeriod");
            din.any <<= in;

            try
            {
                admin_device.command_inout(cmd, din);
            }
            catch(Tango::CommunicationFailed &)
            {
                admin_device.command_inout(cmd, din);
            }
        }
    }
    else
    {
        //
        // This a AddObjPolling command
        //

        DeviceData din;
        std::string cmd("AddObjPolling");
        din.any <<= in;

        try
        {
            admin_device.command_inout(cmd, din);
        }
        catch(Tango::CommunicationFailed &)
        {
            admin_device.command_inout(cmd, din);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::poll_attribute() - If object is already polled, just update its
//                 polling period. If object is not polled, add
//                 it to the list of polled objects
//
//-----------------------------------------------------------------------------

void DeviceProxy::poll_attribute(const std::string &attr_name, int period)
{
    std::string poll_per;
    bool poll = is_polled(Attr, attr_name, poll_per);
    auto &admin_device = get_admin_device();

    DevVarLongStringArray in;
    in.lvalue.length(1);
    in.svalue.length(3);

    in.svalue[0] = CORBA::string_dup(device_name.c_str());
    in.svalue[1] = CORBA::string_dup("attribute");
    in.svalue[2] = CORBA::string_dup(attr_name.c_str());
    in.lvalue[0] = period;

    if(poll)
    {
        //
        // If object is polled and the polling period is the same, simply returns
        //

        TangoSys_MemStream stream;
        int per;

        stream << poll_per << std::ends;
        stream >> per;

        if((per == period) || (per == 0))
        {
            return;
        }
        else
        {
            //
            // If object is polled, this is an update of the polling period
            //

            DeviceData din;
            std::string cmd("UpdObjPollingPeriod");
            din.any <<= in;

            try
            {
                admin_device.command_inout(cmd, din);
            }
            catch(Tango::CommunicationFailed &)
            {
                admin_device.command_inout(cmd, din);
            }
        }
    }
    else
    {
        //
        // This a AddObjPolling command
        //

        DeviceData din;
        std::string cmd("AddObjPolling");
        din.any <<= in;

        try
        {
            admin_device.command_inout(cmd, din);
        }
        catch(Tango::CommunicationFailed &)
        {
            admin_device.command_inout(cmd, din);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_cmd_polled() - return true if the command is polled
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_command_polled(const std::string &cmd_name)
{
    std::string upd;
    return is_polled(Cmd, cmd_name, upd);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_attribute_polled() - return true if the attribute is polled
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_attribute_polled(const std::string &attr_name)
{
    std::string upd;
    return is_polled(Attr, attr_name, upd);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::stop_poll_command() - Stop polling a command
//
//-----------------------------------------------------------------------------

void DeviceProxy::stop_poll_command(const std::string &cmd_name)
{
    auto &admin_device = get_admin_device();
    DevVarStringArray in;
    in.length(3);

    in[0] = CORBA::string_dup(device_name.c_str());
    in[1] = CORBA::string_dup("command");
    in[2] = CORBA::string_dup(cmd_name.c_str());

    DeviceData din;
    std::string cmd("RemObjPolling");
    din.any <<= in;

    try
    {
        admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        admin_device.command_inout(cmd, din);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::stop_poll_attribute() - Stop polling attribute
//
//-----------------------------------------------------------------------------

void DeviceProxy::stop_poll_attribute(const std::string &attr_name)
{
    auto &admin_device = get_admin_device();
    DevVarStringArray in;
    in.length(3);

    in[0] = CORBA::string_dup(device_name.c_str());
    in[1] = CORBA::string_dup("attribute");
    in[2] = CORBA::string_dup(attr_name.c_str());

    DeviceData din;
    std::string cmd("RemObjPolling");
    din.any <<= in;

    try
    {
        admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        admin_device.command_inout(cmd, din);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::add_logging_target - Add a logging target
//
//-----------------------------------------------------------------------------
void DeviceProxy::add_logging_target(const std::string &target_type_name)
{
    auto &admin_device = get_admin_device();
    DevVarStringArray in(2);
    in.length(2);

    in[0] = CORBA::string_dup(device_name.c_str());
    in[1] = CORBA::string_dup(target_type_name.c_str());

    DeviceData din;
    std::string cmd("AddLoggingTarget");
    din.any <<= in;

    try
    {
        admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        admin_device.command_inout(cmd, din);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::remove_logging_target - Remove a logging target
//
//-----------------------------------------------------------------------------
void DeviceProxy::remove_logging_target(const std::string &target_type_name)
{
    auto &admin_device = get_admin_device();
    DevVarStringArray in(2);
    in.length(2);

    in[0] = CORBA::string_dup(device_name.c_str());
    in[1] = CORBA::string_dup(target_type_name.c_str());

    DeviceData din;
    std::string cmd("RemoveLoggingTarget");
    din.any <<= in;

    try
    {
        admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        admin_device.command_inout(cmd, din);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_logging_target - Returns the logging target list
//
//-----------------------------------------------------------------------------
std::vector<std::string> DeviceProxy::get_logging_target()
{
    auto &admin_device = get_admin_device();
    DeviceData din;
    din << device_name;

    std::string cmd("GetLoggingTarget");

    DeviceData dout;
    DevVarStringArray_var logging_targets;
    try
    {
        dout = admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        dout = admin_device.command_inout(cmd, din);
    }

    std::vector<std::string> logging_targets_vec;

    dout >> logging_targets_vec;

    return logging_targets_vec;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_logging_level - Returns the current logging level
//
//-----------------------------------------------------------------------------

int DeviceProxy::get_logging_level()
{
    std::string cmd("GetLoggingLevel");
    auto &admin_device = get_admin_device();

    DevVarStringArray in;
    in.length(1);
    in[0] = CORBA::string_dup(device_name.c_str());

    DeviceData din;
    din.any <<= in;

    DeviceData dout;
    try
    {
        dout = admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        dout = admin_device.command_inout(cmd, din);
    }

    long level;
    if(!(dout >> level))
    {
        const Tango::DevVarLongStringArray *lsarr;
        dout >> lsarr;

        std::string devnm = dev_name();
        std::transform(devnm.begin(), devnm.end(), devnm.begin(), ::tolower);

        for(unsigned int i = 0; i < lsarr->svalue.length(); i++)
        {
            std::string nm(lsarr->svalue[i]);
            std::transform(nm.begin(), nm.end(), nm.begin(), ::tolower);

            if(devnm == nm)
            {
                level = lsarr->lvalue[i];
                break;
            }
        }
    }

    return (int) level;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::set_logging_level - Set the logging level
//
//-----------------------------------------------------------------------------

void DeviceProxy::set_logging_level(int level)
{
    std::string cmd("SetLoggingLevel");
    auto &admin_device = get_admin_device();

    DevVarLongStringArray in;
    in.lvalue.length(1);
    in.lvalue[0] = level;
    in.svalue.length(1);
    in.svalue[0] = CORBA::string_dup(device_name.c_str());

    DeviceData din;
    din.any <<= in;

    try
    {
        admin_device.command_inout(cmd, din);
    }
    catch(Tango::CommunicationFailed &)
    {
        admin_device.command_inout(cmd, din);
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//         DeviceProxy::subscribe_event
//
// description :
//        Subscribe to an event - Old interface for compatibility
//
//-------------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(const std::string &attr_name,
                                 EventType event,
                                 CallBack *callback,
                                 const std::vector<std::string> &filters)
{
    return subscribe_event(attr_name, event, callback, filters, false);
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//         DeviceProxy::subscribe_event
//
// description :
//        Subscribe to an event- Adds the statless flag for stateless event subscription.
//
//-------------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(const std::string &attr_name,
                                 EventType event,
                                 CallBack *callback,
                                 const std::vector<std::string> &filters,
                                 bool stateless)
{
    ApiUtil *au = ApiUtil::instance();

    //
    // First, try using zmq. If it fails with the error "Command Not Found", try using notifd
    //

    int ret;
    try
    {
        auto zmq_consumer = au->create_zmq_event_consumer();
        ret = zmq_consumer->subscribe_event(this, attr_name, event, callback, filters, stateless);
    }
    catch(DevFailed &e)
    {
        std::string reason(e.errors[0].reason.in());
        if(reason == API_CommandNotFound)
        {
            auto notifd_consumer = au->create_notifd_event_consumer();

            ret = notifd_consumer->subscribe_event(this, attr_name, event, callback, filters, stateless);
        }
        else
        {
            throw;
        }
    }

    return ret;
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//         DeviceProxy::subscribe_event
//
// description :
//        Subscribe to an event with the usage of the event queue for data reception. Adds the statless flag for
//        stateless event subscription.
//
//-----------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(const std::string &attr_name,
                                 EventType event,
                                 int event_queue_size,
                                 const std::vector<std::string> &filters,
                                 bool stateless)
{
    ApiUtil *au = ApiUtil::instance();

    //
    // First, try using zmq. If it fails with the error "Command Not Found", try using notifd
    //

    int ret;
    try
    {
        auto zmq_consumer = au->create_zmq_event_consumer();
        ret = zmq_consumer->subscribe_event(this, attr_name, event, event_queue_size, filters, stateless);
    }
    catch(DevFailed &e)
    {
        std::string reason(e.errors[0].reason.in());
        if(reason == API_CommandNotFound)
        {
            auto notifd_consumer = au->create_notifd_event_consumer();
            ret = notifd_consumer->subscribe_event(this, attr_name, event, event_queue_size, filters, stateless);
        }
        else
        {
            throw;
        }
    }

    return ret;
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
//         DeviceProxy::subscribe_event
//
// description :
//        Subscribe to a device event- Add the statless flag for stateless event subscription.
//
//-------------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(EventType event, CallBack *callback, bool stateless)
{
    if(detail::IDLVersionIsTooOld(version, MIN_IDL_DEV_INTR))
    {
        std::stringstream ss;
        ss << "Device " << dev_name() << " does not support device interface change event\n";
        ss << "Available since Tango release 9 AND for device inheriting from IDL release 5 (Device_5Impl)";

        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, ss.str());
    }

    ApiUtil *api_ptr = ApiUtil::instance();
    if(api_ptr->get_zmq_event_consumer() == nullptr)
    {
        api_ptr->create_zmq_event_consumer();
    }

    int ret;
    ret = api_ptr->get_zmq_event_consumer()->subscribe_event(this, event, callback, stateless);

    return ret;
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
//         DeviceProxy::subscribe_event
//
// description :
//        Subscribe to an event with the usage of the event queue for data reception. Adds the statless flag for
//        stateless event subscription.
//
//-----------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(EventType event, int event_queue_size, bool stateless)
{
    if(detail::IDLVersionIsTooOld(version, MIN_IDL_DEV_INTR))
    {
        std::stringstream ss;
        ss << "Device " << dev_name() << " does not support device interface change event\n";
        ss << "Available since Tango release 9 AND for device inheriting from IDL release 5 (Device_5Impl)";

        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, ss.str());
    }

    ApiUtil *au = ApiUtil::instance();
    auto zmq_consumer = au->create_zmq_event_consumer();

    return zmq_consumer->subscribe_event(this, event, event_queue_size, stateless);
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
//         DeviceProxy::unsubscribe_event
//
// description :
//        Unsubscribe to an event
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceProxy::unsubscribe_event(int event_id)
{
    auto es = get_event_system_for_event_id(event_id);
    es->unsubscribe_event(event_id);
}

//-----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_events()
//
// description :  Return a vector with all events stored in the event queue.
//                Events are kept in the buffer since the last extraction
//                with get_events().
//                After returning the event data, the event queue gets
//                emptied!
//
// argument : in  : event_id   : The event identifier
// argument : out : event_list : A reference to an event data list to be filled
//-----------------------------------------------------------------------------
void DeviceProxy::get_events(int event_id, EventDataList &event_list)
{
    auto es = get_event_system_for_event_id(event_id);
    es->get_events(event_id, event_list);
}

//-----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_events()
//
// description :  Return a vector with all attribute configuration events
//                stored in the event queue.
//                Events are kept in the buffer since the last extraction
//                with get_events().
//                After returning the event data, the event queue gets
//                emptied!
//
// argument : in  : event_id   : The event identifier
// argument : out : event_list : A reference to an event data list to be filled
//-----------------------------------------------------------------------------
void DeviceProxy::get_events(int event_id, AttrConfEventDataList &event_list)
{
    auto es = get_event_system_for_event_id(event_id);
    es->get_events(event_id, event_list);
}

void DeviceProxy::get_events(int event_id, DataReadyEventDataList &event_list)
{
    auto es = get_event_system_for_event_id(event_id);
    es->get_events(event_id, event_list);
}

void DeviceProxy::get_events(int event_id, DevIntrChangeEventDataList &event_list)
{
    auto es = get_event_system_for_event_id(event_id);
    es->get_events(event_id, event_list);
}

void DeviceProxy::get_events(int event_id, PipeEventDataList &event_list)
{
    auto es = get_event_system_for_event_id(event_id);
    es->get_events(event_id, event_list);
}

//-----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_events()
//
// description :  Call the callback method for all events stored
//                in the event queue.
//                Events are kept in the buffer since the last extraction
//                with get_events().
//                After returning the event data, the event queue gets
//                emptied!
//
// argument : in  : event_id   : The event identifier
// argument : out : cb : The callback object pointer
//-----------------------------------------------------------------------------
void DeviceProxy::get_events(int event_id, CallBack *cb)
{
    auto es = get_event_system_for_event_id(event_id);
    es->get_events(event_id, cb);
}

//+----------------------------------------------------------------------------
//
// method :       DeviceProxy::event_queue_size()
//
// description :  Returns the number of events stored in the event queue
//
// argument : in : event_id   : The event identifier
//
//-----------------------------------------------------------------------------
int DeviceProxy::event_queue_size(int event_id)
{
    auto es = get_event_system_for_event_id(event_id);
    return es->event_queue_size(event_id);
}

//+----------------------------------------------------------------------------
//
// method :       DeviceProxy::is_event_queue_empty()
//
// description :  Returns true when the event queue is empty
//
// argument : in : event_id   : The event identifier
//
//-----------------------------------------------------------------------------
bool DeviceProxy::is_event_queue_empty(int event_id)
{
    auto es = get_event_system_for_event_id(event_id);
    return es->is_event_queue_empty(event_id);
}

//+----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_last_event_date()
//
// description :  Get the time stamp of the last inserted event
//
// argument : in : event_id   : The event identifier
//
//-----------------------------------------------------------------------------
TimeVal DeviceProxy::get_last_event_date(int event_id)
{
    auto es = get_event_system_for_event_id(event_id);
    return es->get_last_event_date(event_id);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_device_db - get database
//
//-----------------------------------------------------------------------------

Database *DeviceProxy::get_device_db()
{
    if((db_port_num != 0) && (db_dev != nullptr))
    {
        return db_dev->get_dbase();
    }
    else
    {
        return (Database *) nullptr;
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_admin_device - get a device proxy to the admin device
//
//-----------------------------------------------------------------------------
DeviceProxy &DeviceProxy::get_admin_device()
{
    omni_mutex_lock guard(adm_dev_mutex);
    if(adm_device == nullptr)
    {
        adm_dev_name = adm_name();
        adm_device = std::make_unique<Tango::DeviceProxy>(adm_dev_name);
    }
    return *adm_device;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_adm_device - get a device proxy to the admin device
//
//-----------------------------------------------------------------------------
DeviceProxy *DeviceProxy::get_adm_device()
{
    auto &admin_device = get_admin_device();
    return new DeviceProxy(admin_device);
}

//-----------------------------------------------------------------------------
//
// clean_lock - Litle function installed in the list of function(s) to be called
// at exit time. It will clean all locking thread(s) and unlock locked device(s)
//
//-----------------------------------------------------------------------------

static void clean_lock()
{
    if(!ApiUtil::_is_instance_null())
    {
        ApiUtil *au = ApiUtil::instance();
        au->clean_locking_threads();
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::lock - Lock the device
//
//-----------------------------------------------------------------------------

void DeviceProxy::lock(int lock_validity)
{
    auto &admin_device = get_admin_device();
    //
    // Feature unavailable for device without database
    //

    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Feature not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }

    //
    // Some checks on lock validity
    //

    if(lock_validity < MIN_LOCK_VALIDITY)
    {
        TangoSys_OMemStream desc;
        desc << "Lock validity can not be lower than " << MIN_LOCK_VALIDITY << " seconds" << std::ends;

        TANGO_THROW_EXCEPTION(API_MethodArgument, desc.str());
    }

    {
        omni_mutex_lock guard(lock_mutex);
        if(lock_ctr != 0)
        {
            if(lock_validity != lock_valid)
            {
                TangoSys_OMemStream desc;

                desc << "Device " << device_name << " is already locked with another lock validity (";
                desc << lock_valid << " sec)" << std::ends;

                TANGO_THROW_EXCEPTION(API_MethodArgument, desc.str());
            }
        }
    }

    //
    // Check if the function to be executed atexit is already installed
    //

    Tango::ApiUtil *au = ApiUtil::instance();
    if(!au->is_lock_exit_installed())
    {
        atexit(clean_lock);
        au->set_sig_handler();
        au->set_lock_exit_installed(true);
    }

    //
    // Send command to admin device
    //

    std::string cmd("LockDevice");
    DeviceData din;
    DevVarLongStringArray sent_data;
    sent_data.svalue.length(1);
    sent_data.svalue[0] = CORBA::string_dup(device_name.c_str());
    sent_data.lvalue.length(1);
    sent_data.lvalue[0] = lock_validity;
    din << sent_data;

    admin_device.command_inout(cmd, din);

    //
    // Increment locking counter
    //

    {
        omni_mutex_lock guard(lock_mutex);

        lock_ctr++;
        lock_valid = lock_validity;
    }

    //
    // Try to find the device's server admin device locking thread
    // in the ApiUtil map.
    // If the thread is not there, start one.
    // If the thread is there, ask it to add the device to its list
    // of locked devices
    //

    {
        omni_mutex_lock oml(au->lock_th_map);

        auto pos = au->lock_threads.find(adm_dev_name);
        if(pos == au->lock_threads.end())
        {
            create_locking_thread(au, std::chrono::seconds(lock_validity));
        }
        else
        {
            bool local_suicide;
            {
                omni_mutex_lock sync(*(pos->second.mon));
                local_suicide = pos->second.shared->suicide;
            }

            if(local_suicide)
            {
                delete pos->second.shared;
                delete pos->second.mon;
                au->lock_threads.erase(pos);

                create_locking_thread(au, std::chrono::seconds(lock_validity));
            }
            else
            {
                int interupted;

                omni_mutex_lock sync(*(pos->second.mon));
                if(pos->second.shared->cmd_pending)
                {
                    interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                    if((pos->second.shared->cmd_pending) && (interupted == 0))
                    {
                        TANGO_LOG_DEBUG << "TIME OUT" << std::endl;
                        TANGO_THROW_EXCEPTION(API_CommandTimedOut, "Locking thread blocked !!!");
                    }
                }
                pos->second.shared->cmd_pending = true;
                pos->second.shared->cmd_code = LOCK_ADD_DEV;
                pos->second.shared->dev_name = device_name;
                {
                    omni_mutex_lock guard(lock_mutex);
                    pos->second.shared->lock_validity = std::chrono::seconds(lock_valid);
                }

                pos->second.mon->signal();

                TANGO_LOG_DEBUG << "Cmd sent to locking thread" << std::endl;

                while(pos->second.shared->cmd_pending)
                {
                    interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                    if((pos->second.shared->cmd_pending) && (interupted == 0))
                    {
                        TANGO_LOG_DEBUG << "TIME OUT" << std::endl;
                        TANGO_THROW_EXCEPTION(API_CommandTimedOut, "Locking thread blocked !!!");
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::unlock - Unlock the device
//
//-----------------------------------------------------------------------------

void DeviceProxy::unlock(bool force)
{
    auto &admin_device = get_admin_device();
    //
    // Feature unavailable for device without database
    //

    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Feature not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }

    //
    // Send command to admin device
    //

    std::string cmd("UnLockDevice");
    DeviceData din, dout;
    DevVarLongStringArray sent_data;
    sent_data.svalue.length(1);
    sent_data.svalue[0] = CORBA::string_dup(device_name.c_str());
    sent_data.lvalue.length(1);
    if(force)
    {
        sent_data.lvalue[0] = 1;
    }
    else
    {
        sent_data.lvalue[0] = 0;
    }
    din << sent_data;

    //
    // Send request to the DS admin device
    //

    dout = admin_device.command_inout(cmd, din);

    //
    // Decrement locking counter or replace it by the device global counter
    // returned by the server
    //

    Tango::DevLong glob_ctr;

    dout >> glob_ctr;
    int local_lock_ctr;

    {
        omni_mutex_lock guard(lock_mutex);

        lock_ctr--;
        if(glob_ctr != lock_ctr)
        {
            lock_ctr = glob_ctr;
        }
        local_lock_ctr = lock_ctr;
    }

    //
    // Try to find the device's server admin device locking thread
    // in the ApiUtil map.
    // Ask the thread to remove the device to its list of locked devices
    //

    if((local_lock_ctr == 0) || (force))
    {
        Tango::ApiUtil *au = Tango::ApiUtil::instance();

        {
            omni_mutex_lock oml(au->lock_th_map);
            auto pos = au->lock_threads.find(adm_dev_name);
            if(pos == au->lock_threads.end())
            {
                //                TangoSys_OMemStream o;

                //                o << "Can't find the locking thread for device " << device_name << " and admin device
                //                " << adm_dev_name << ends; TANGO_THROW_EXCEPTION(API_CantFindLockingThread, o.str());
            }
            else
            {
                if(pos->second.shared->suicide)
                {
                    delete pos->second.shared;
                    delete pos->second.mon;
                    au->lock_threads.erase(pos);
                }
                else
                {
                    int interupted;

                    omni_mutex_lock sync(*(pos->second.mon));
                    if(pos->second.shared->cmd_pending)
                    {
                        interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                        if((pos->second.shared->cmd_pending) && (interupted == 0))
                        {
                            TANGO_LOG_DEBUG << "TIME OUT" << std::endl;
                            TANGO_THROW_EXCEPTION(API_CommandTimedOut, "Locking thread blocked !!!");
                        }
                    }
                    pos->second.shared->cmd_pending = true;
                    pos->second.shared->cmd_code = LOCK_REM_DEV;
                    pos->second.shared->dev_name = device_name;

                    pos->second.mon->signal();

                    TANGO_LOG_DEBUG << "Cmd sent to locking thread" << std::endl;

                    while(pos->second.shared->cmd_pending)
                    {
                        interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                        if((pos->second.shared->cmd_pending) && (interupted == 0))
                        {
                            TANGO_LOG_DEBUG << "TIME OUT" << std::endl;
                            TANGO_THROW_EXCEPTION(API_CommandTimedOut, "Locking thread blocked !!!");
                        }
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::create_locking_thread - Create and start a locking thread
//
//-----------------------------------------------------------------------------

void DeviceProxy::create_locking_thread(ApiUtil *au, std::chrono::seconds dl)
{
    LockingThread lt;
    lt.mon = nullptr;
    lt.l_thread = nullptr;
    lt.shared = nullptr;

    std::pair<std::map<std::string, LockingThread>::iterator, bool> status;
    status = au->lock_threads.insert(std::make_pair(adm_dev_name, lt));
    if(!status.second)
    {
        TangoSys_OMemStream o;
        o << "Can't create the locking thread for device " << device_name << " and admin device " << adm_dev_name
          << std::ends;
        TANGO_THROW_EXCEPTION(API_CantCreateLockingThread, o.str());
    }
    else
    {
        std::map<std::string, LockingThread>::iterator pos;

        pos = status.first;
        pos->second.mon = new TangoMonitor(adm_dev_name.c_str());
        pos->second.shared = new LockThCmd;
        pos->second.shared->cmd_pending = false;
        pos->second.shared->suicide = false;
        pos->second.l_thread = new LockThread(*pos->second.shared, *pos->second.mon, get_adm_device(), device_name, dl);

        pos->second.l_thread->start();
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::locking_status - Return a device locking status as a string
//
//-----------------------------------------------------------------------------

std::string DeviceProxy::locking_status()
{
    std::vector<std::string> v_str;
    std::vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    std::string str(v_str[0]);
    return str;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_locked - Check if a device is locked
//
// Returns true if locked, false otherwise
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_locked()
{
    std::vector<std::string> v_str;
    std::vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    return v_l[0] > 0;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_locked_by_me - Check if a device is locked by the caller
//
// Returns true if the caller is the locker, false otherwise
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_locked_by_me()
{
    std::vector<std::string> v_str;
    std::vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    bool ret = false;

    if(v_l[0] == 0)
    {
        ret = false;
    }
    else
    {
#ifndef _TG_WINDOWS_
        if(getpid() != v_l[1])
#else
        if(_getpid() != v_l[1])
#endif
        {
            ret = false;
        }
        else if((v_l[2] != 0) || (v_l[3] != 0) || (v_l[4] != 0) || (v_l[5] != 0))
        {
            ret = false;
        }
        else
        {
            std::string full_ip_str;
            get_locker_host(v_str[1], full_ip_str);

            //
            // If the call is local, as the PID is already the good one, the caller is the locker
            //

            if(full_ip_str == TG_LOCAL_HOST)
            {
                ret = true;
            }
            else
            {
                //
                // Get the host address(es) and check if it is the same than the one sent by the server
                //

                ApiUtil *au = ApiUtil::instance();
                std::vector<std::string> adrs;

                au->get_ip_from_if(adrs);

                for(unsigned int nb_adrs = 0; nb_adrs < adrs.size(); nb_adrs++)
                {
                    if(adrs[nb_adrs] == full_ip_str)
                    {
                        ret = true;
                        break;
                    }
                }
            }
        }
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_locker - Get some info on the device locker if the device
// is locked
// This method returns true if the device is effectively locked.
// Otherwise, it returns false
//
//-----------------------------------------------------------------------------

bool DeviceProxy::get_locker(LockerInfo &lock_info)
{
    std::vector<std::string> v_str;
    std::vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    if(v_l[0] == 0)
    {
        return false;
    }
    else
    {
        //
        // If the PID info coming from server is not 0, the locker is CPP
        // Otherwise, it is Java
        //

        if(v_l[1] != 0)
        {
            lock_info.ll = Tango::CPP; // TODO: what about the Tango::CPP_6 case
            lock_info.li.LockerPid = v_l[1];

            lock_info.locker_class = "Not defined";
        }
        else
        {
            lock_info.ll = Tango::JAVA;
            for(int loop = 0; loop < 4; loop++)
            {
                lock_info.li.UUID[loop] = v_l[2 + loop];
            }

            std::string full_ip;
            get_locker_host(v_str[1], full_ip);

            lock_info.locker_class = v_str[2];
        }

        //
        // Add locker host name
        //

        std::string full_ip;
        get_locker_host(v_str[1], full_ip);

        //
        // Convert locker IP address to its name
        //

        if(full_ip != TG_LOCAL_HOST)
        {
            struct sockaddr_in si;
            si.sin_family = AF_INET;
            si.sin_port = 0;
#ifdef _TG_WINDOWS_
            int slen = sizeof(si);
            WSAStringToAddress((char *) full_ip.c_str(), AF_INET, nullptr, (SOCKADDR *) &si, &slen);
#else
            inet_pton(AF_INET, full_ip.c_str(), &si.sin_addr);
#endif

            char host_os[512];

            int res = getnameinfo((const sockaddr *) &si, sizeof(si), host_os, 512, nullptr, 0, 0);

            if(res == 0)
            {
                lock_info.locker_host = host_os;
            }
            else
            {
                lock_info.locker_host = full_ip;
            }
        }
        else
        {
            char h_name[Tango::detail::TANGO_MAX_HOSTNAME_LEN];
            gethostname(h_name, Tango::detail::TANGO_MAX_HOSTNAME_LEN);

            lock_info.locker_host = h_name;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::ask_locking_status - Get the device locking status
//
//-----------------------------------------------------------------------------

void DeviceProxy::ask_locking_status(std::vector<std::string> &v_str, std::vector<DevLong> &v_l)
{
    auto &admin_device = get_admin_device();
    //
    // Feature unavailable for device without database
    //

    if(!dbase_used)
    {
        TangoSys_OMemStream desc;
        desc << "Feature not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        TANGO_THROW_DETAILED_EXCEPTION(ApiNonDbExcept, API_NonDatabaseDevice, desc.str());
    }

    //
    // Send command to admin device
    //

    std::string cmd("DevLockStatus");
    DeviceData din, dout;
    din.any <<= device_name.c_str();

    dout = admin_device.command_inout(cmd, din);

    //
    // Extract data and return data to caller
    //

    dout.extract(v_l, v_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_locker_host - Isolate from the host string as it is returned
// by omniORB, only the host IP address
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_locker_host(const std::string &f_addr, std::string &ip_addr)
{
    //
    // The hostname is returned in the following format:
    // "giop:tcp:160.103.5.157:32989" or "giop:tcp:[::ffff:160.103.5.157]:32989
    // or "giop:unix:/tmp..."
    // We need to isolate the IP address
    //

    bool ipv6 = false;

    if(f_addr.find('[') != std::string::npos)
    {
        ipv6 = true;
    }

    if(f_addr.find(":unix:") != std::string::npos)
    {
        ip_addr = TG_LOCAL_HOST;
    }
    else
    {
        std::string::size_type pos;
        if((pos = f_addr.find(':')) == std::string::npos)
        {
            TANGO_THROW_EXCEPTION(API_WrongLockingStatus, "Locker IP address returned by server is unvalid");
        }
        pos++;
        if((pos = f_addr.find(':', pos)) == std::string::npos)
        {
            TANGO_THROW_EXCEPTION(API_WrongLockingStatus, "Locker IP address returned by server is unvalid");
        }
        pos++;

        if(ipv6)
        {
            pos = pos + 3;
            if((pos = f_addr.find(':', pos)) == std::string::npos)
            {
                TANGO_THROW_EXCEPTION(API_WrongLockingStatus, "Locker IP address returned by server is unvalid");
            }
            pos++;
            std::string ip_str = f_addr.substr(pos);
            if((pos = ip_str.find(']')) == std::string::npos)
            {
                TANGO_THROW_EXCEPTION(API_WrongLockingStatus, "Locker IP address returned by server is unvalid");
            }
            ip_addr = ip_str.substr(0, pos);
        }
        else
        {
            std::string ip_str = f_addr.substr(pos);
            if((pos = ip_str.find(':')) == std::string::npos)
            {
                TANGO_THROW_EXCEPTION(API_WrongLockingStatus, "Locker IP address returned by server is unvalid");
            }
            ip_addr = ip_str.substr(0, pos);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_read_attribute() - write then read a single attribute
//
//-----------------------------------------------------------------------------

DeviceAttribute DeviceProxy::write_read_attribute(const DeviceAttribute &dev_attr)
{
    TANGO_TELEMETRY_TRACE_BEGIN(
        ({{"tango.operation.target", dev_name()}, {"tango.operation.argument", dev_attr.name}}));

    //
    // This call is available only for Devices implemented IDL V4
    //

    if(detail::IDLVersionIsTooOld(version, 4))
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support write_read_attribute feature" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, desc.str());
    }

    //
    // Data into the AttributeValue object
    //

    AttributeValueList_4 attr_value_list;
    attr_value_list.length(1);

    attr_value_list[0].name = dev_attr.name.c_str();
    attr_value_list[0].quality = dev_attr.quality;
    attr_value_list[0].data_format = dev_attr.data_format;
    attr_value_list[0].time = dev_attr.time;
    attr_value_list[0].w_dim.dim_x = dev_attr.dim_x;
    attr_value_list[0].w_dim.dim_y = dev_attr.dim_y;

    if(dev_attr.LongSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.long_att_value(dev_attr.LongSeq.in());
    }
    else if(dev_attr.Long64Seq.operator->() != nullptr)
    {
        attr_value_list[0].value.long64_att_value(dev_attr.Long64Seq.in());
    }
    else if(dev_attr.ShortSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.short_att_value(dev_attr.ShortSeq.in());
    }
    else if(dev_attr.DoubleSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.double_att_value(dev_attr.DoubleSeq.in());
    }
    else if(dev_attr.StringSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.string_att_value(dev_attr.StringSeq.in());
    }
    else if(dev_attr.FloatSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.float_att_value(dev_attr.FloatSeq.in());
    }
    else if(dev_attr.BooleanSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.bool_att_value(dev_attr.BooleanSeq.in());
    }
    else if(dev_attr.UShortSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.ushort_att_value(dev_attr.UShortSeq.in());
    }
    else if(dev_attr.UCharSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.uchar_att_value(dev_attr.UCharSeq.in());
    }
    else if(dev_attr.ULongSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.ulong_att_value(dev_attr.ULongSeq.in());
    }
    else if(dev_attr.ULong64Seq.operator->() != nullptr)
    {
        attr_value_list[0].value.ulong64_att_value(dev_attr.ULong64Seq.in());
    }
    else if(dev_attr.StateSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.state_att_value(dev_attr.StateSeq.in());
    }
    else if(dev_attr.EncodedSeq.operator->() != nullptr)
    {
        attr_value_list[0].value.encoded_att_value(dev_attr.EncodedSeq.in());
    }

    Tango::DevVarStringArray dvsa;
    dvsa.length(1);
    dvsa[0] = CORBA::string_dup(dev_attr.name.c_str());

    int ctr = 0;
    AttributeValueList_4_var attr_value_list_4;
    AttributeValueList_5_var attr_value_list_5;
    Tango::AccessControlType local_act;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

            //
            // Throw exception if caller not allowed to write_attribute
            //

            if(local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
            }

            //
            // Now, call the server
            //

            if(version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->write_read_attributes_5(attr_value_list, dvsa, get_client_identification());
            }
            else
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->write_read_attributes_4(attr_value_list, get_client_identification());
            }

            ctr = 2;
        }
        catch(Tango::MultiDevFailed &e)
        {
            //
            // Transfer this exception into a DevFailed exception
            //

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << std::ends;
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_AttributeFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_read_attribute()", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_read_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_read_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_read_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    //
    // Init the returned DeviceAttribute instance
    //

    DeviceAttribute ret_dev_attr;
    if(version >= 5)
    {
        ApiUtil::attr_to_device(&(attr_value_list_5[0]), version, &ret_dev_attr);
    }
    else
    {
        ApiUtil::attr_to_device(&(attr_value_list_4[0]), version, &ret_dev_attr);
    }

    //
    // Add an error in the error stack in case there is one
    //

    DevErrorList_var &err_list = ret_dev_attr.get_error_list();
    long nb_except = err_list.in().length();
    if(nb_except != 0)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to write_read_attribute on device " << device_name;
        desc << ", attribute " << dev_attr.name << std::ends;

        err_list.inout().length(nb_except + 1);
        err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
        err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

        std::string st = desc.str();
        err_list[nb_except].desc = Tango::string_dup(st.c_str());
        err_list[nb_except].severity = Tango::ERR;
    }

    return (ret_dev_attr);

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_read_attributes() - write then read a single attribute
//
//-----------------------------------------------------------------------------

std::vector<DeviceAttribute> *DeviceProxy::write_read_attributes(const std::vector<DeviceAttribute> &attr_list,
                                                                 const std::vector<std::string> &r_names)
{
    TANGO_TELEMETRY_TRACE_BEGIN(({{"tango.operation.target", dev_name()}}));

    //
    // This call is available only for Devices implemented IDL V5
    //

    if(detail::IDLVersionIsTooOld(version, 5))
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support write_read_attributes feature" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, desc.str());
    }

    //
    // Data into the AttributeValue object
    //

    AttributeValueList_4 attr_value_list;
    attr_value_list.length(attr_list.size());

    for(unsigned int i = 0; i < attr_list.size(); i++)
    {
        attr_value_list[i].name = attr_list[i].name.c_str();
        attr_value_list[i].quality = attr_list[i].quality;
        attr_value_list[i].data_format = attr_list[i].data_format;
        attr_value_list[i].time = attr_list[i].time;
        attr_value_list[i].w_dim.dim_x = attr_list[i].dim_x;
        attr_value_list[i].w_dim.dim_y = attr_list[i].dim_y;

        if(attr_list[i].LongSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.long_att_value(attr_list[i].LongSeq.in());
        }
        else if(attr_list[i].Long64Seq.operator->() != nullptr)
        {
            attr_value_list[i].value.long64_att_value(attr_list[i].Long64Seq.in());
        }
        else if(attr_list[i].ShortSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.short_att_value(attr_list[i].ShortSeq.in());
        }
        else if(attr_list[i].DoubleSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.double_att_value(attr_list[i].DoubleSeq.in());
        }
        else if(attr_list[i].StringSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.string_att_value(attr_list[i].StringSeq.in());
        }
        else if(attr_list[i].FloatSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.float_att_value(attr_list[i].FloatSeq.in());
        }
        else if(attr_list[i].BooleanSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.bool_att_value(attr_list[i].BooleanSeq.in());
        }
        else if(attr_list[i].UShortSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.ushort_att_value(attr_list[i].UShortSeq.in());
        }
        else if(attr_list[i].UCharSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.uchar_att_value(attr_list[i].UCharSeq.in());
        }
        else if(attr_list[i].ULongSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.ulong_att_value(attr_list[i].ULongSeq.in());
        }
        else if(attr_list[i].ULong64Seq.operator->() != nullptr)
        {
            attr_value_list[i].value.ulong64_att_value(attr_list[i].ULong64Seq.in());
        }
        else if(attr_list[i].StateSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.state_att_value(attr_list[i].StateSeq.in());
        }
        else if(attr_list[i].EncodedSeq.operator->() != nullptr)
        {
            attr_value_list[i].value.encoded_att_value(attr_list[i].EncodedSeq.in());
        }
    }

    //
    // Create remaining parameter
    //

    Tango::DevVarStringArray dvsa;
    dvsa << r_names;

    //
    // Call device
    //

    int ctr = 0;
    AttributeValueList_5_var attr_value_list_5;
    Tango::AccessControlType local_act;

    while(ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

            //
            // Throw exception if caller not allowed to write_attribute
            //

            if(local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch(...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << std::ends;

                TANGO_THROW_DETAILED_EXCEPTION(NotAllowedExcept, API_ReadOnlyMode, desc.str());
            }

            //
            // Now, call the server
            //

            Device_5_var dev = Device_5::_duplicate(device_5);
            attr_value_list_5 = dev->write_read_attributes_5(attr_value_list, dvsa, get_client_identification());

            ctr = 2;
        }
        catch(Tango::MultiDevFailed &e)
        {
            //
            // Transfer this exception into a DevFailed exception
            //

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attributes on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << std::ends;
            TANGO_RETHROW_EXCEPTION(ex, API_AttributeFailed, desc.str());
        }
        catch(Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attributes on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << std::ends;

            if(::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TANGO_RETHROW_DETAILED_EXCEPTION(DeviceUnlockedExcept, e, DEVICE_UNLOCKED_REASON, desc.str());
            }
            else
            {
                TANGO_RETHROW_EXCEPTION(e, API_AttributeFailed, desc.str());
            }
        }
        catch(CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_read_attributes()", this);
        }
        catch(CORBA::OBJECT_NOT_EXIST &one)
        {
            if(one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_read_attributes on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, one, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::COMM_FAILURE &comm)
        {
            if(comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attributes on device " << device_name << std::ends;
                TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, comm, API_CommunicationFailed, desc.str());
            }
        }
        catch(CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_read_attributes on device " << device_name << std::ends;
            TANGO_RETHROW_DETAILED_EXCEPTION(ApiCommExcept, ce, API_CommunicationFailed, desc.str());
        }
    }

    //
    // Init the returned DeviceAttribute vector

    unsigned long nb_received;
    nb_received = attr_value_list_5->length();

    auto *dev_attr = new(std::vector<DeviceAttribute>);
    dev_attr->resize(nb_received);

    for(unsigned int i = 0; i < nb_received; i++)
    {
        ApiUtil::attr_to_device(&(attr_value_list_5[i]), 5, &(*dev_attr)[i]);

        //
        // Add an error in the error stack in case there is one
        //

        DevErrorList_var &err_list = (*dev_attr)[i].get_error_list();
        long nb_except = err_list.in().length();
        if(nb_except != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attribute on device " << device_name;
            desc << ", attribute " << (*dev_attr)[i].name << std::ends;

            err_list.inout().length(nb_except + 1);
            err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
            err_list[nb_except].origin = Tango::string_dup(TANGO_EXCEPTION_ORIGIN);

            std::string st = desc.str();
            err_list[nb_except].desc = Tango::string_dup(st.c_str());
            err_list[nb_except].severity = Tango::ERR;
        }
    }

    return (dev_attr);

    TANGO_TELEMETRY_TRACE_END();
}

//-----------------------------------------------------------------------------
//
// method :         DeviceProxy::same_att_name()
//
// description :     Check if in the attribute name list there is not several
//                    times the same attribute. Throw exception in case of
//
// argin(s) :        attr_list : The attribute name(s) list
//                    met_name : The calling method name (for exception)
//
//-----------------------------------------------------------------------------

void DeviceProxy::same_att_name(const std::vector<std::string> &attr_list, const char *met_name)
{
    if(attr_list.size() > 1)
    {
        unsigned int i;
        std::vector<std::string> same_att = attr_list;

        for(i = 0; i < same_att.size(); ++i)
        {
            std::transform(same_att[i].begin(), same_att[i].end(), same_att[i].begin(), ::tolower);
        }
        sort(same_att.begin(), same_att.end());
        std::vector<std::string> same_att_lower = same_att;

        auto pos = unique(same_att.begin(), same_att.end());

        int duplicate_att;
        duplicate_att = distance(attr_list.begin(), attr_list.end()) - distance(same_att.begin(), pos);

        if(duplicate_att != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Several times the same attribute in required attributes list: ";
            int ctr = 0;
            for(i = 0; i < same_att_lower.size() - 1; i++)
            {
                if(same_att_lower[i] == same_att_lower[i + 1])
                {
                    ctr++;
                    desc << same_att_lower[i];
                    if(ctr < duplicate_att)
                    {
                        desc << ", ";
                    }
                }
            }
            desc << std::ends;
            ApiConnExcept::throw_exception(API_AttributeFailed, desc.str(), met_name);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::local_import() - If the device is embedded within the same
// process, re-create its IOR and returns it. This save one DB call.
//
//-----------------------------------------------------------------------------

void DeviceProxy::local_import(std::string &local_ior)
{
    Tango::Util *tg = nullptr;

    //
    // In case of controlled access used, this method is called while the
    // Util object is still in its construction case.
    // Catch this exception and return from this method in this case
    //

    try
    {
        tg = Tango::Util::instance(false);
    }
    catch(Tango::DevFailed &e)
    {
        std::string reas(e.errors[0].reason);
        if(reas == API_UtilSingletonNotCreated)
        {
            return;
        }
    }

    const std::vector<Tango::DeviceClass *> *cl_list_ptr = tg->get_class_list();
    for(unsigned int loop = 0; loop < cl_list_ptr->size(); loop++)
    {
        Tango::DeviceClass *cl_ptr = (*cl_list_ptr)[loop];
        std::vector<Tango::DeviceImpl *> dev_list = cl_ptr->get_device_list();
        for(unsigned int lo = 0; lo < dev_list.size(); lo++)
        {
            if(dev_list[lo]->get_name_lower() == device_name)
            {
                if(Tango::Util::instance()->use_db())
                {
                    Database *db = tg->get_database();
                    if(db->get_db_host() != get_db_host())
                    {
                        return;
                    }
                }

                Tango::Device_var d_var = dev_list[lo]->get_d_var();
                CORBA::ORB_var orb_var = tg->get_orb();

                char *s = orb_var->object_to_string(d_var);
                local_ior = s;

                Tango::string_free(s);

                return;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method:
//        DeviceProxy::get_tango_lib_version()
//
// description:
//        Returns the Tango lib version number used by the remote device
//
// return:
//        The device Tango lib version as a 3 or 4 digits number
//        Possible return value are: 100,200,500,520,700,800,810,...
//
//---------------------------------------------------------------------------------------------------------------------

int DeviceProxy::get_tango_lib_version()
{
    int ret = 0;

    auto &admin_device = get_admin_device();
    //
    // Get admin device IDL release and command list
    //

    int admin_idl_vers = admin_device.get_idl_version();
    Tango::CommandInfoList *cmd_list;
    cmd_list = admin_device.command_list_query();

    switch(admin_idl_vers)
    {
    case 1:
        ret = 100;
        break;

    case 2:
        ret = 200;
        break;

    case 3:
    {
        //
        // IDL 3 is for Tango 5 and 6. Unfortunately, there is no way from the client side to determmine if it is
        // Tango 5 or 6. The beast we can do is to get the info that it is Tango 5.2 (or above)
        //

        auto pos = find_if((*cmd_list).begin(),
                           (*cmd_list).end(),
                           [](Tango::CommandInfo &cc) -> bool { return cc.cmd_name == "QueryWizardClassProperty"; });
        if(pos != (*cmd_list).end())
        {
            ret = 520;
        }
        else
        {
            ret = 500;
        }
        break;
    }

    case 4:
    {
        //
        // IDL 4 is for Tango 7 and 8.
        //

        bool ecs = false;
        bool zesc = false;

        for(const auto &cmd : *cmd_list)
        {
            if(cmd.cmd_name == "EventConfirmSubscription")
            {
                ecs = true;
                break;
            }

            if(cmd.cmd_name == "ZmqEventSubscriptionChange")
            {
                zesc = true;
            }
        }
        if(ecs)
        {
            ret = 810;
        }
        else if(zesc)
        {
            ret = 800;
        }
        else
        {
            ret = 700;
        }

        break;
    }

    case 5:
        ret = 902;
        break;
    case 6:
        ret = 1000;
        break;

    default:
        TANGO_ASSERT_ON_DEFAULT(admin_idl_vers);
    }

    delete cmd_list;

    return ret;
}

inline int DeviceProxy::subscribe_event(const std::string &attr_name, EventType event, CallBack *callback)
{
    std::vector<std::string> filt;
    return subscribe_event(attr_name, event, callback, filt, false);
}

inline int
    DeviceProxy::subscribe_event(const std::string &attr_name, EventType event, CallBack *callback, bool stateless)
{
    std::vector<std::string> filt;
    return subscribe_event(attr_name, event, callback, filt, stateless);
}

inline int
    DeviceProxy::subscribe_event(const std::string &attr_name, EventType event, int event_queue_size, bool stateless)
{
    std::vector<std::string> filt;
    return subscribe_event(attr_name, event, event_queue_size, filt, stateless);
}

} // namespace Tango
