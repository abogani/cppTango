#include <tango/tango.h>
#include <tango/internal/utils.h>

namespace Tango::detail
{

bool IDLVersionIsTooOld(int version, int desiredVersion)
{
    return version > detail::INVALID_IDL_VERSION && version < desiredVersion;
}

namespace
{
template <class T>
bool try_type(const CORBA::TypeCode_ptr type, std::string &result)
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
    CORBA::TypeCode_ptr type = any.type();

    std::string result;
    if(try_type<Tango::DevVarShortArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarUShortArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarLongArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarULongArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarLong64Array>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarULong64Array>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarDoubleArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarStringArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarUCharArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarFloatArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarBooleanArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarStateArray>(type, result))
    {
        goto end;
    }
    if(try_type<Tango::DevVarEncodedArray>(type, result))
    {
        goto end;
    }

    {
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
        result = oss.str();
    }

end:
    CORBA::release(type);
    return result;
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

} // namespace Tango::detail
