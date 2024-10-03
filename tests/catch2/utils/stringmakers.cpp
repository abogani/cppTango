#include <tango/tango.h>

#include "stringmakers.h"

namespace
{

/// separator between entries
const std::string sep{", "};

/// opening curly brace
const std::string opc{"{ "};

// closing curly brace
const std::string clc{" }"};

} // namespace

namespace Catch
{

std::string StringMaker<Tango::DeviceInfo>::convert(Tango::DeviceInfo const &info)
{
    std::ostringstream os;

    os << opc;
    os << "dev_class: " << info.dev_class;
    os << sep;
    os << "server_id: " << info.server_id;
    os << sep;
    os << "server_host: " << info.server_host;
    os << sep;
    os << "server_version: " << info.server_version;
    os << sep;
    os << "doc_url: " << info.doc_url;
    os << sep;
    os << "dev_type: " << info.dev_type;
    os << sep;
    os << "version_info: " << Catch::StringMaker<std::map<std::string, std::string>>::convert(info.version_info);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::DeviceProxy *>::convert(Tango::DeviceProxy *dev)
{
    if(dev == nullptr)
    {
        return Catch::StringMaker<std::nullptr_t>::convert(nullptr);
    }

    return Catch::StringMaker<Tango::DeviceInfo>::convert(dev->info());
}

std::string StringMaker<Tango::DeviceAttribute *>::convert(Tango::DeviceAttribute *da)
{
    if(da == nullptr)
    {
        return Catch::StringMaker<std::nullptr_t>::convert(nullptr);
    }

    return Catch::StringMaker<Tango::DeviceAttribute>::convert(*da);
}

std::string StringMaker<Tango::TimeVal>::convert(Tango::TimeVal const &value)
{
    std::ostringstream os;

    os << opc;
    os << "tv_sec: " << value.tv_sec;
    os << sep;
    os << "tv_usec:" << value.tv_usec;
    os << sep;
    os << "tv_nsec:" << value.tv_nsec;
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::EventData>::convert(Tango::EventData const &value)
{
    std::ostringstream os;

    os << opc;
    os << "reception_date: " << StringMaker<Tango::TimeVal>::convert(value.reception_date);
    os << sep;
    os << "device: " << StringMaker<Tango::DeviceProxy *>::convert(value.device);
    os << sep;
    os << "attr_name: " << value.attr_name;
    os << sep;
    os << "event: " << value.event;
    os << sep;
    os << "attr_value: " << opc << StringMaker<Tango::DeviceAttribute *>::convert(value.attr_value) << clc;
    os << sep;
    os << "err: " << std::boolalpha << value.err;
    os << sep;
    os << "errors: " << opc << StringMaker<Tango::DevErrorList>::convert(value.errors) << clc;
    os << clc;

    return os.str();
}

} // namespace Catch
