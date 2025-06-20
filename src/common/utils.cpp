#include <tango/common/tango_type_traits.h>
#include <tango/internal/utils.h>

#include <tango/common/tango_const.h>
#include <tango/common/utils/type_info.h>
#include <tango/common/utils/assert.h>
#include <tango/server/except.h>
#include <tango/server/seqvec.h>
#include <tango/client/DeviceAttribute.h>
#include <tango/client/ApiUtil.h>
#include <tango/client/Database.h>
#include <tango/client/DeviceProxy.h>

#include <algorithm>

namespace
{
const char *const EVENT_COMPAT = "idl";
const char *const EVENT_COMPAT_IDL5 = "idl5_";
const int EVENT_COMPAT_IDL5_SIZE = 5; // strlen of previous string
} // anonymous namespace

namespace Tango::detail
{

bool IDLVersionIsTooOld(int version, int desiredVersion)
{
    return version > detail::INVALID_IDL_VERSION && version < desiredVersion;
}

namespace
{
template <class T>
bool try_type(const CORBA::TypeCode_ptr &type, std::string &result)
{
    if(type->equivalent(tango_type_traits<T>::corba_type_code()))
    {
        result = data_type_to_string(tango_type_traits<T>::type_value());
        return true;
    }

    return false;
}
} // namespace

std::string corba_any_to_type_name(const CORBA::Any &any)
{
    CORBA::TypeCode_var type = any.type();

    std::string result;
    if(try_type<Tango::DevVarShortArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarUShortArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarLongArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarULongArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarLong64Array>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarULong64Array>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarDoubleArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarStringArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarUCharArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarFloatArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarBooleanArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarStateArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevVarEncodedArray>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevShort>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevUShort>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevLong>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevULong>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevLong64>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevULong64>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevDouble>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevString>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevUChar>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevFloat>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevBoolean>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevState>(type, result))
    {
        return result;
    }
    if(try_type<Tango::DevEncoded>(type, result))
    {
        return result;
    }

    TangoSys_OMemStream oss;
    oss << "UnknownCorbaAny<kind=" << type->kind();

    // `TypeCode`s for basic data types do not have a `name()`, as they can
    // be distinguished with only their `kind()`.

    try
    {
        const char *name = type->name();
        oss << ",name=" << name;
    }
    catch(const CORBA::TypeCode::BadKind &)
    {
        /* ignore */
    }

    oss << ">" << std::ends;
    return oss.str();
}

std::string attr_union_dtype_to_type_name(Tango::AttributeDataType d)
{
    switch(d)
    {
    case Tango::ATT_BOOL:
        return data_type_to_string(DEVVAR_BOOLEANARRAY);
    case Tango::ATT_SHORT:
        return data_type_to_string(DEVVAR_SHORTARRAY);
    case Tango::ATT_LONG:
        return data_type_to_string(DEVVAR_LONGARRAY);
    case Tango::ATT_LONG64:
        return data_type_to_string(DEVVAR_LONG64ARRAY);
    case Tango::ATT_FLOAT:
        return data_type_to_string(DEVVAR_FLOATARRAY);
    case Tango::ATT_DOUBLE:
        return data_type_to_string(DEVVAR_DOUBLEARRAY);
    case Tango::ATT_UCHAR:
        return data_type_to_string(DEVVAR_CHARARRAY);
    case Tango::ATT_USHORT:
        return data_type_to_string(DEVVAR_USHORTARRAY);
    case Tango::ATT_ULONG:
        return data_type_to_string(DEVVAR_ULONGARRAY);
    case Tango::ATT_ULONG64:
        return data_type_to_string(DEVVAR_ULONG64ARRAY);
    case Tango::ATT_STRING:
        return data_type_to_string(DEVVAR_STRINGARRAY);
    case Tango::ATT_STATE:
        return data_type_to_string(DEVVAR_STATEARRAY);
    case Tango::ATT_ENCODED:
        return data_type_to_string(DEVVAR_ENCODEDARRAY);
    case Tango::DEVICE_STATE: /* fallthrough */
    case Tango::ATT_NO_DATA:  /* fallthrough */
    default:
    {
        TangoSys_OMemStream oss;
        oss << "UnknownAttrValUnion<dtype=" << d << ">";
        return oss.str();
    }
    }
}

std::string to_lower(std::string str)
{
    std::transform(std::begin(str), std::end(str), std::begin(str), ::tolower);
    return str;
}

std::string to_upper(std::string str)
{
    std::transform(std::begin(str), std::end(str), std::begin(str), ::toupper);
    return str;
}

std::optional<bool> to_boolean(std::string_view str)
{
    if(str == "on" || str == "true" || str == "1")
    {
        return true;
    }
    else if(str == "off" || str == "false" || str == "0")
    {
        return false;
    }

    return {};
}

bool get_boolean_env_var(const char *env_var, bool default_value)
{
    std::string contents;
    int ret = ApiUtil::instance()->get_env_var(env_var, contents);

    if(ret != 0)
    {
        return default_value;
    }

    auto result = to_boolean(to_lower(contents));

    if(!result.has_value())
    {
        std::stringstream sstr;
        sstr << "Environment variable: " << env_var << ", with contents " << contents
             << ", can not be parsed as boolean.";
        TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, sstr.str());
    }

    return result.value();
}

void stringify_any(std::ostream &os, const CORBA::Any &any)
{
    CORBA::TypeCode_var tc_al;
    CORBA::TypeCode_var tc_seq;
    CORBA::TypeCode_var tc_field;

    CORBA::TypeCode_var tc = any.type();

    if(tc->equal(CORBA::_tc_null))
    {
        os << "empty";
        return;
    }

    switch(tc->kind())
    {
    case CORBA::tk_boolean:
        bool bo_tmp;
        any >>= CORBA::Any::to_boolean(bo_tmp);
        if(bo_tmp)
        {
            os << "true";
        }
        else
        {
            os << "false";
        }
        break;

    case CORBA::tk_short:
        short tmp;
        any >>= tmp;
        os << tmp;
        break;

    case CORBA::tk_long:
        Tango::DevLong l_tmp;
        any >>= l_tmp;
        os << l_tmp;
        break;

    case CORBA::tk_longlong:
        Tango::DevLong64 ll_tmp;
        any >>= ll_tmp;
        os << ll_tmp;
        break;

    case CORBA::tk_float:
        float f_tmp;
        any >>= f_tmp;
        os << f_tmp;
        break;

    case CORBA::tk_double:
        double db_tmp;
        any >>= db_tmp;
        os << db_tmp;
        break;

    case CORBA::tk_ushort:
        unsigned short us_tmp;
        any >>= us_tmp;
        os << us_tmp;
        break;

    case CORBA::tk_ulong:
        Tango::DevULong ul_tmp;
        any >>= ul_tmp;
        os << ul_tmp;
        break;

    case CORBA::tk_ulonglong:
        Tango::DevULong64 ull_tmp;
        any >>= ull_tmp;
        os << ull_tmp;
        break;

    case CORBA::tk_string:
        const char *str_tmp;
        any >>= str_tmp;
        os << str_tmp;
        break;

    case CORBA::tk_alias:
        tc_al = tc->content_type();
        tc_seq = tc_al->content_type();
        switch(tc_seq->kind())
        {
        case CORBA::tk_octet:
            Tango::DevVarCharArray *ch_arr;
            any >>= ch_arr;
            os << *ch_arr;
            break;

        case CORBA::tk_boolean:
            Tango::DevVarBooleanArray *bl_arr;
            any >>= bl_arr;
            os << *bl_arr;
            break;

        case CORBA::tk_short:
            Tango::DevVarShortArray *sh_arr;
            any >>= sh_arr;
            os << *sh_arr;
            break;

        case CORBA::tk_long:
            Tango::DevVarLongArray *lg_arr;
            any >>= lg_arr;
            os << *lg_arr;
            break;

        case CORBA::tk_longlong:
            Tango::DevVarLong64Array *llg_arr;
            any >>= llg_arr;
            os << *llg_arr;
            break;

        case CORBA::tk_float:
            Tango::DevVarFloatArray *fl_arr;
            any >>= fl_arr;
            os << *fl_arr;
            break;

        case CORBA::tk_double:
            Tango::DevVarDoubleArray *db_arr;
            any >>= db_arr;
            os << *db_arr;
            break;

        case CORBA::tk_ushort:
            Tango::DevVarUShortArray *us_arr;
            any >>= us_arr;
            os << *us_arr;
            break;

        case CORBA::tk_ulong:
            Tango::DevVarULongArray *ul_arr;
            any >>= ul_arr;
            os << *ul_arr;
            break;

        case CORBA::tk_ulonglong:
            Tango::DevVarULong64Array *ull_arr;
            any >>= ull_arr;
            os << *ull_arr;
            break;

        case CORBA::tk_string:
            Tango::DevVarStringArray *str_arr;
            any >>= str_arr;
            os << *str_arr;
            break;

        default:
            TangoSys_OMemStream desc;
            desc << "'any' with unexpected sequence kind '" << tc_seq->kind() << "'.";
            TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
        }
        break;

    case CORBA::tk_struct:
        tc_field = tc->member_type(0);
        tc_al = tc_field->content_type();
        switch(tc_al->kind())
        {
        case CORBA::tk_sequence:
            tc_seq = tc_al->content_type();
            switch(tc_seq->kind())
            {
            case CORBA::tk_long:
                Tango::DevVarLongStringArray *lgstr_arr;
                any >>= lgstr_arr;
                os << lgstr_arr->lvalue << std::endl;
                os << lgstr_arr->svalue;
                break;

            case CORBA::tk_double:
                Tango::DevVarDoubleStringArray *dbstr_arr;
                any >>= dbstr_arr;
                os << dbstr_arr->dvalue << std::endl;
                os << dbstr_arr->svalue;
                break;

            default:
                TangoSys_OMemStream desc;
                desc << "'any' with unexpected struct field sequence kind '" << tc_seq->kind() << "'.";
                TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
            }
            break;

        case CORBA::tk_string:
            Tango::DevEncoded *enc;
            any >>= enc;
            os << "Encoding string: " << enc->encoded_format << std::endl;
            {
                long nb_data_elt = enc->encoded_data.length();
                for(long i = 0; i < nb_data_elt; i++)
                {
                    os << "Data element number [" << i << "] = " << (int) enc->encoded_data[i];
                    if(i < (nb_data_elt - 1))
                    {
                        os << '\n';
                    }
                }
            }
            break;

        default:
            TangoSys_OMemStream desc;
            desc << "'any' with unexpected struct field alias kind '" << tc_al->kind() << "'.";
            TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
        }
        break;

    case CORBA::tk_enum:
        Tango::DevState tmp_state;
        any >>= tmp_state;
        os << Tango::DevStateName[tmp_state];
        break;

    default:
        TangoSys_OMemStream desc;
        desc << "'any' with unexpected kind '" << tc->kind() << "'.";
        TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
    }
}

void stringify_attribute_data(std::ostream &os, const DeviceAttribute &da)
{
    if(da.LongSeq.operator->() != nullptr)
    {
        os << *(da.LongSeq.operator->());
    }
    else if(da.Long64Seq.operator->() != nullptr)
    {
        os << *(da.Long64Seq.operator->());
    }
    else if(da.ShortSeq.operator->() != nullptr)
    {
        os << *(da.ShortSeq.operator->());
    }
    else if(da.DoubleSeq.operator->() != nullptr)
    {
        os << *(da.DoubleSeq.operator->());
    }
    else if(da.FloatSeq.operator->() != nullptr)
    {
        os << *(da.FloatSeq.operator->());
    }
    else if(da.BooleanSeq.operator->() != nullptr)
    {
        os << *(da.BooleanSeq.operator->());
    }
    else if(da.UShortSeq.operator->() != nullptr)
    {
        os << *(da.UShortSeq.operator->());
    }
    else if(da.UCharSeq.operator->() != nullptr)
    {
        os << *(da.UCharSeq.operator->());
    }
    else if(da.StringSeq.operator->() != nullptr)
    {
        os << *(da.StringSeq.operator->());
    }
    else if(da.ULongSeq.operator->() != nullptr)
    {
        os << *(da.ULongSeq.operator->());
    }
    else if(da.ULong64Seq.operator->() != nullptr)
    {
        os << *(da.ULong64Seq.operator->());
    }
    else if(da.StateSeq.operator->() != nullptr)
    {
        os << *(da.StateSeq.operator->());
    }
    else if(da.EncodedSeq.operator->() != nullptr)
    {
        os << *(da.EncodedSeq.operator->());
    }
    else
    {
        os << DevStateName[da.d_state];
    }
}

std::vector<std::string> get_databases_from_control_system(Database *db)
{
    std::vector<std::string> vs;

    try
    {
        DeviceData dd;
        dd = db->command_inout("DbGetCSDbServerList");
        dd >> vs;
    }
    catch(...)
    {
    }

    return vs;
}

std::vector<std::string> gather_fqdn_prefixes_from_env(Database *db)
{
    std::vector<std::string> env_var_fqdn_prefix;
    std::string prefix = "tango://" + db->get_db_host() + ':' + db->get_db_port() + '/';
    env_var_fqdn_prefix.push_back(prefix);

    if(db->is_multi_tango_host())
    {
        std::vector<std::string> &tango_hosts = db->get_multi_host();
        std::vector<std::string> &tango_ports = db->get_multi_port();
        for(unsigned int i = 1; i < tango_hosts.size(); i++)
        {
            std::string prefix = "tango://" + tango_hosts[i] + ':' + tango_ports[i] + '/';
            env_var_fqdn_prefix.push_back(prefix);
        }
    }

    for(size_t loop = 0; loop < env_var_fqdn_prefix.size(); ++loop)
    {
        std::transform(env_var_fqdn_prefix[loop].begin(),
                       env_var_fqdn_prefix[loop].end(),
                       env_var_fqdn_prefix[loop].begin(),
                       ::tolower);
    }

    return env_var_fqdn_prefix;
}

void append_fqdn_host_prefixes_from_db(const std::vector<std::string> &vs, std::vector<std::string> &prefixes)
{
    //
    // Several Db servers for one TANGO_HOST case
    //

    std::vector<std::string>::iterator pos;

    for(unsigned int i = 0; i < vs.size(); i++)
    {
        pos = find_if(prefixes.begin(),
                      prefixes.end(),
                      [&](std::string str) -> bool { return str.find(vs[i]) != std::string::npos; });

        if(pos == prefixes.end())
        {
            std::string prefix = "tango://" + vs[i] + '/';
            prefixes.push_back(prefix);
        }
    }
}

std::string build_device_trl(DeviceProxy *device, const std::vector<std::string> &prefixes)
{
    std::string local_device_name = device->dev_name();

    if(!device->get_from_env_var())
    {
        std::string prot("tango://");
        if(!device->is_dbase_used())
        {
            std::string &ho = device->get_dev_host();
            if(ho.find('.') == std::string::npos)
            {
                Connection::get_fqdn(ho);
            }
            prot = prot + ho + ':' + device->get_dev_port() + '/';
        }
        else
        {
            prot = prot + device->get_db_host() + ':' + device->get_db_port() + '/';
        }
        local_device_name.insert(0, prot);
        if(!device->is_dbase_used())
        {
            local_device_name = local_device_name + MODIFIER_DBASE_NO;
        }
    }
    else
    {
        local_device_name.insert(0, prefixes[0]);
    }

    std::transform(local_device_name.begin(), local_device_name.end(), local_device_name.begin(), ::tolower);

    return local_device_name;
}

std::string add_idl_prefix(std::string event_name)
{
    return EVENT_COMPAT_IDL5 + event_name;
}

std::string remove_idl_prefix(std::string event_name)
{
    auto pos = event_name.find(EVENT_COMPAT);
    if(pos == std::string::npos)
    {
        return event_name;
    }

    event_name.erase(0, EVENT_COMPAT_IDL5_SIZE);

    return event_name;
}

std::optional<int> extract_idl_version_from_event_name(const std::string &event_name)
{
    std::string::size_type pos = event_name.find(EVENT_COMPAT);
    if(pos == std::string::npos)
    {
        return {};
    }

    int client_release;

    // FIXME prefer parse_as from !1450 once available
    std::string client_lib_str = event_name.substr(pos + 3, 1);
    std::stringstream ss;
    ss << client_lib_str;
    ss >> client_release;

    return client_release;
}

std::string insert_idl_for_compat(std::string event_name)
{
    std::string::size_type pos = event_name.rfind('.');
    TANGO_ASSERT(pos != std::string::npos);

    event_name.insert(pos + 1, EVENT_COMPAT_IDL5);

    return event_name;
}

} // namespace Tango::detail

namespace Tango
{

std::ostream &operator<<(std::ostream &os, const AttrQuality &quality)
{
    switch(quality)
    {
    case Tango::ATTR_VALID:
        os << "VALID";
        break;
    case Tango::ATTR_INVALID:
        os << "INVALID";
        break;
    case Tango::ATTR_ALARM:
        os << "ALARM";
        break;
    case Tango::ATTR_CHANGING:
        os << "CHANGING";
        break;
    case Tango::ATTR_WARNING:
        os << "WARNING";
        break;
    default:
        TANGO_ASSERT_ON_DEFAULT(quality);
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const ErrSeverity &error_severity)
{
    switch(error_severity)
    {
    case Tango::WARN:
        os << "WARNING";
        break;
    case Tango::ERR:
        os << "ERROR";
        break;
    case Tango::PANIC:
        os << "PANIC";
        break;
    default:
        // backwards compatibility
        os << "Unknown";
    }

    return os;
}

} // namespace Tango
