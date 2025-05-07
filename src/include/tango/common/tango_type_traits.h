#ifndef _TANGO_TYPE_TRAITS_H
#define _TANGO_TYPE_TRAITS_H

#include <tango/common/tango_const.h>

#include <string>

namespace Tango
{

template <class T>
struct tango_type_traits;

template <>
struct tango_type_traits<Tango::DevShort>
{
    using ArrayType = Tango::DevVarShortArray;
    using Type = Tango::DevShort;

    static constexpr auto type_value()
    {
        return DEV_SHORT;
    }

    static constexpr auto att_type_value()
    {
        return ATT_SHORT;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevShort;
    }
};

template <>
struct tango_type_traits<Tango::DevUShort>
{
    using ArrayType = Tango::DevVarUShortArray;
    using Type = Tango::DevUShort;

    static constexpr auto type_value()
    {
        return DEV_USHORT;
    }

    static constexpr auto att_type_value()
    {
        return ATT_USHORT;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevUShort;
    }
};

template <>
struct tango_type_traits<Tango::DevLong>
{
    using ArrayType = Tango::DevVarLongArray;
    using Type = Tango::DevLong;

    static constexpr auto type_value()
    {
        return DEV_LONG;
    }

    static constexpr auto att_type_value()
    {
        return ATT_LONG;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevLong;
    }
};

template <>
struct tango_type_traits<Tango::DevULong>
{
    using ArrayType = Tango::DevVarULongArray;
    using Type = Tango::DevULong;

    static constexpr auto type_value()
    {
        return DEV_ULONG;
    }

    static constexpr auto att_type_value()
    {
        return ATT_ULONG;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarULongArray;
    }
};

template <>
struct tango_type_traits<Tango::DevLong64>
{
    using ArrayType = Tango::DevVarLong64Array;
    using Type = Tango::DevLong64;

    static constexpr auto type_value()
    {
        return DEV_LONG64;
    }

    static constexpr auto att_type_value()
    {
        return ATT_LONG64;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevLong64;
    }
};

template <>
struct tango_type_traits<Tango::DevULong64>
{
    using ArrayType = Tango::DevVarULong64Array;
    using Type = Tango::DevULong64;

    static constexpr auto type_value()
    {
        return DEV_ULONG64;
    }

    static constexpr auto att_type_value()
    {
        return ATT_ULONG64;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevULong64;
    }
};

template <>
struct tango_type_traits<Tango::DevDouble>
{
    using ArrayType = Tango::DevVarDoubleArray;
    using Type = Tango::DevDouble;

    static constexpr auto type_value()
    {
        return DEV_DOUBLE;
    }

    static constexpr auto att_type_value()
    {
        return ATT_DOUBLE;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevDouble;
    }
};

template <>
struct tango_type_traits<Tango::DevString>
{
    using ArrayType = Tango::DevVarStringArray;
    using Type = Tango::DevString;

    static constexpr auto type_value()
    {
        return DEV_STRING;
    }

    static constexpr auto att_type_value()
    {
        return ATT_STRING;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevString;
    }
};

template <>
struct tango_type_traits<std::string> : tango_type_traits<Tango::DevString>
{
};

template <>
struct tango_type_traits<Tango::DevBoolean>
{
    using ArrayType = Tango::DevVarBooleanArray;
    using Type = Tango::DevBoolean;

    static constexpr auto type_value()
    {
        return DEV_BOOLEAN;
    }

    static constexpr auto att_type_value()
    {
        return ATT_BOOL;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevBoolean;
    }
};

template <>
struct tango_type_traits<Tango::DevFloat>
{
    using ArrayType = Tango::DevVarFloatArray;
    using Type = Tango::DevFloat;

    static constexpr auto type_value()
    {
        return DEV_FLOAT;
    }

    static constexpr auto att_type_value()
    {
        return ATT_FLOAT;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevFloat;
    }
};

template <>
struct tango_type_traits<Tango::DevUChar>
{
    using ArrayType = Tango::DevVarUCharArray;
    using Type = Tango::DevUChar;

    static constexpr auto type_value()
    {
        return DEV_UCHAR;
    }

    static constexpr auto att_type_value()
    {
        return ATT_UCHAR;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevUChar;
    }
};

template <>
struct tango_type_traits<Tango::DevState>
{
    using ArrayType = Tango::DevVarStateArray;
    using Type = Tango::DevState;

    static constexpr auto type_value()
    {
        return DEV_STATE;
    }

    static constexpr auto att_type_value()
    {
        return ATT_STATE;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevState;
    }
};

template <>
struct tango_type_traits<Tango::DevEncoded>
{
    using ArrayType = Tango::DevVarEncodedArray;
    using Type = Tango::DevEncoded;

    static constexpr auto type_value()
    {
        return DEV_ENCODED;
    }

    static constexpr auto att_type_value()
    {
        return ATT_ENCODED;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevEncoded;
    }
};

template <>
struct tango_type_traits<Tango::DevVarShortArray>
{
    using ArrayType = Tango::DevVarShortArray;
    using Type = Tango::DevVarShortArray;

    static constexpr auto type_value()
    {
        return DEVVAR_SHORTARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_SHORT;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarShortArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarUShortArray>
{
    using ArrayType = Tango::DevVarUShortArray;
    using Type = Tango::DevVarUShortArray;

    static constexpr auto type_value()
    {
        return DEVVAR_USHORTARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_USHORT;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarUShortArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarLongArray>
{
    using ArrayType = Tango::DevVarLongArray;
    using Type = Tango::DevVarLongArray;

    static constexpr auto type_value()
    {
        return DEVVAR_LONGARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_LONG;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarLongArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarULongArray>
{
    using ArrayType = Tango::DevVarULongArray;
    using Type = Tango::DevVarULongArray;

    static constexpr auto type_value()
    {
        return DEVVAR_ULONGARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_ULONG;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarULongArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarLong64Array>
{
    using ArrayType = Tango::DevVarLong64Array;
    using Type = Tango::DevVarLong64Array;

    static constexpr auto type_value()
    {
        return DEVVAR_LONG64ARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_LONG64;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarLong64Array;
    }
};

template <>
struct tango_type_traits<Tango::DevVarULong64Array>
{
    using ArrayType = Tango::DevVarULong64Array;
    using Type = Tango::DevVarULong64Array;

    static constexpr auto type_value()
    {
        return DEVVAR_ULONG64ARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_ULONG64;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarULong64Array;
    }
};

template <>
struct tango_type_traits<Tango::DevVarCharArray>
{
    using ArrayType = Tango::DevVarCharArray;
    using Type = Tango::DevVarCharArray;

    static constexpr auto type_value()
    {
        return DEVVAR_CHARARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_UCHAR;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarCharArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarFloatArray>
{
    using ArrayType = Tango::DevVarFloatArray;
    using Type = Tango::DevVarFloatArray;

    static constexpr auto type_value()
    {
        return DEVVAR_FLOATARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_FLOAT;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarFloatArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarDoubleArray>
{
    using ArrayType = Tango::DevVarDoubleArray;
    using Type = Tango::DevVarDoubleArray;

    static constexpr auto type_value()
    {
        return DEVVAR_DOUBLEARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_DOUBLE;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarDoubleArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarStringArray>
{
    using ArrayType = Tango::DevVarStringArray;
    using Type = Tango::DevVarStringArray;

    static constexpr auto type_value()
    {
        return DEVVAR_STRINGARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_STRING;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarStringArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarBooleanArray>
{
    using ArrayType = Tango::DevVarBooleanArray;
    using Type = Tango::DevVarBooleanArray;

    static constexpr auto type_value()
    {
        return DEVVAR_BOOLEANARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_BOOL;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarBooleanArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarStateArray>
{
    using ArrayType = Tango::DevVarStateArray;
    using Type = Tango::DevVarStateArray;

    static constexpr auto type_value()
    {
        return DEVVAR_STATEARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_STATE;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarStateArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarEncodedArray>
{
    using ArrayType = Tango::DevVarEncodedArray;
    using Type = Tango::DevVarEncodedArray;

    static constexpr auto type_value()
    {
        return DEVVAR_ENCODEDARRAY;
    }

    static constexpr auto att_type_value()
    {
        return ATT_ENCODED;
    }

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarEncodedArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarLongStringArray>
{
    using ArrayType = Tango::DevVarLongStringArray;

    static constexpr auto type_value()
    {
        return DEVVAR_LONGSTRINGARRAY;
    }

    // no att_type_value member as it is commands only

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarLongStringArray;
    }
};

template <>
struct tango_type_traits<Tango::DevVarDoubleStringArray>
{
    using ArrayType = Tango::DevVarDoubleStringArray;

    static constexpr auto type_value()
    {
        return DEVVAR_DOUBLESTRINGARRAY;
    }

    // no att_type_value member as it is commands only

    static auto corba_type_code()
    {
        return Tango::_tc_DevVarDoubleStringArray;
    }
};

} // namespace Tango
#endif /* TANGO_TYPE_TRAITS_H */
