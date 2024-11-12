#ifndef CATCH_UTILS_STRINGMAKERS_H
#define CATCH_UTILS_STRINGMAKERS_H

#include <tango/tango.h>

#include <iostream>

#include <catch2/catch_tostring.hpp>

#include "callback_mock_helpers.h"
#include "test_server.h"

#include <tango/internal/stl_corba_helpers.h>

CATCH_REGISTER_ENUM(Tango::AttrMemorizedType,
                    Tango::AttrMemorizedType::NOT_KNOWN,
                    Tango::AttrMemorizedType::NONE,
                    Tango::AttrMemorizedType::MEMORIZED,
                    Tango::AttrMemorizedType::MEMORIZED_WRITE_INIT)

CATCH_REGISTER_ENUM(Tango::AttrWriteType,
                    Tango::AttrWriteType::READ,
                    Tango::AttrWriteType::READ_WITH_WRITE,
                    Tango::AttrWriteType::READ_WRITE,
                    Tango::AttrWriteType::WT_UNKNOWN)

CATCH_REGISTER_ENUM(Tango::AttrDataFormat,
                    Tango::AttrDataFormat::SCALAR,
                    Tango::AttrDataFormat::SPECTRUM,
                    Tango::AttrDataFormat::IMAGE,
                    Tango::AttrDataFormat::FMT_UNKNOWN)

CATCH_REGISTER_ENUM(Tango::DispLevel,
                    Tango::DispLevel::OPERATOR,
                    Tango::DispLevel::EXPERT,
                    Tango::DispLevel::DL_UNKNOWN)

namespace TangoTest::detail
{

/// separator between entries
const std::string sep{", "};

/// opening curly brace
const std::string opc{"{ "};

// closing curly brace
const std::string clc{" }"};

} // namespace TangoTest::detail

namespace Catch
{
template <>
struct StringMaker<Tango::DeviceInfo>
{
    static std::string convert(Tango::DeviceInfo const &info);
};

template <>
struct StringMaker<Tango::DeviceProxy *>
{
    static std::string convert(Tango::DeviceProxy *dev);
};

template <>
struct StringMaker<Tango::DeviceProxy>
{
    static std::string convert(const Tango::DeviceProxy &dev);
};

template <>
struct StringMaker<Tango::DeviceAttribute *>
{
    static std::string convert(Tango::DeviceAttribute *da);
};

template <>
struct StringMaker<Tango::DeviceAttribute>
{
    static std::string convert(const Tango::DeviceAttribute &da);
};

template <>
struct StringMaker<Tango::DeviceData>
{
    static std::string convert(const Tango::DeviceData &dd);
};

template <>
struct StringMaker<Tango::TimeVal>
{
    static std::string convert(Tango::TimeVal const &value);
};

template <>
struct StringMaker<Tango::EventData>
{
    static std::string convert(Tango::EventData const &value);
};

template <>
struct StringMaker<Tango::DataReadyEventData>
{
    static std::string convert(Tango::DataReadyEventData const &value);
};

template <>
struct StringMaker<TangoTest::AttrReadEventCopyable>
{
    static std::string convert(TangoTest::AttrReadEventCopyable const &value);
};

template <>
struct StringMaker<Tango::DevIntrChangeEventData>
{
    static std::string convert(Tango::DevIntrChangeEventData const &value);
};

template <>
struct StringMaker<Tango::CommandInfo>
{
    static std::string convert(Tango::CommandInfo const &value);
};

template <>
struct StringMaker<Tango::AttributeAlarmInfo>
{
    static std::string convert(Tango::AttributeAlarmInfo const &value);
};

template <>
struct StringMaker<Tango::ChangeEventInfo>
{
    static std::string convert(Tango::ChangeEventInfo const &value);
};

template <>
struct StringMaker<Tango::PeriodicEventInfo>
{
    static std::string convert(Tango::PeriodicEventInfo const &value);
};

template <>
struct StringMaker<Tango::ArchiveEventInfo>
{
    static std::string convert(Tango::ArchiveEventInfo const &value);
};

template <>
struct StringMaker<Tango::AttributeEventInfo>
{
    static std::string convert(Tango::AttributeEventInfo const &value);
};

template <>
struct StringMaker<Tango::AttributeInfoEx>
{
    static std::string convert(Tango::AttributeInfoEx const &value);
};

template <>
struct StringMaker<CORBA::Any>
{
    static std::string convert(CORBA::Any const &any);
};

template <>
struct StringMaker<Tango::DevError>
{
    static std::string convert(Tango::DevError const &err);
};

/// Generic output routine for CORBA sequences
template <typename T>
struct StringMaker<T, std::enable_if_t<Tango::detail::is_corba_seq_v<T>>>
{
    static std::string convert(T const &seq)
    {
        using TangoTest::detail::clc;
        using TangoTest::detail::opc;
        using TangoTest::detail::sep;

        auto length = size(seq);

        std::ostringstream os;

        if(length == 0)
        {
            os << opc;
            os << clc;
            return os.str();
        }

        using ElementType = Tango::detail::corba_ut_from_seq_t<T>;

        TANGO_ASSERT(length > 0);
        const auto *end_it = end(seq);

        os << opc;
        for(const auto *it = begin(seq); it < end_it; it++)
        {
            os << StringMaker<ElementType>::convert(*it);

            if(std::distance(it, end_it) > 1)
            {
                os << sep;
            }
        }
        os << clc;

        return os.str();
    }
};

// No generic output routine for CORBA var classes
// as this is available via implicit conversion

template <>
struct StringMaker<TangoTest::ExitStatus>
{
    static std::string convert(TangoTest::ExitStatus const &status);
};

} // namespace Catch

#endif // CATCH_UTILS_STRINGMAKERS_H
