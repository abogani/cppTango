#ifndef CATCH_UTILS_STRINGMAKERS_H
#define CATCH_UTILS_STRINGMAKERS_H

#include <tango/tango.h>

#include <iostream>

#include <catch2/catch_tostring.hpp>

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
struct StringMaker<Tango::DeviceAttribute *>
{
    static std::string convert(Tango::DeviceAttribute *da);
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

} // namespace Catch

#endif // CATCH_UTILS_STRINGMAKERS_H
