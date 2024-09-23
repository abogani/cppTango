#include <tango/tango.h>

#include <tango/internal/utils.h>
#include <tango/common/utils/type_info.h>

#include "stringmakers.h"

namespace
{
using TangoTest::detail::clc;
using TangoTest::detail::opc;
using TangoTest::detail::sep;

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

    return Catch::StringMaker<Tango::DeviceProxy>::convert(*dev);
}

std::string StringMaker<Tango::DeviceProxy>::convert(const Tango::DeviceProxy &dev)
{
    return Catch::StringMaker<Tango::DeviceInfo>::convert(const_cast<Tango::DeviceProxy &>(dev).info());
}

std::string StringMaker<Tango::DeviceAttribute *>::convert(Tango::DeviceAttribute *da)
{
    if(da == nullptr)
    {
        return Catch::StringMaker<std::nullptr_t>::convert(nullptr);
    }

    return Catch::StringMaker<Tango::DeviceAttribute>::convert(*da);
}

std::string StringMaker<Tango::DeviceData>::convert(const Tango::DeviceData &dd)
{
    return StringMaker<CORBA::Any>::convert(dd.any);
}

std::string StringMaker<Tango::DeviceAttribute>::convert(const Tango::DeviceAttribute &da)
{
    std::ostringstream os;

    os << opc;

    os << "error: ";

    if(da.has_failed())
    {
        os << StringMaker<Tango::DevErrorList>::convert(da.err_list);
        // no return as we want to output all elements for debug purposes
    }
    else
    {
        os << opc;
        os << clc;
    }
    os << sep;
    os << "time: " << StringMaker<Tango::TimeVal>::convert(da.time);
    os << sep;
    if(da.name == "Name not set")
    {
        os << "name: \"\"";
    }
    else
    {
        os << "name: " << StringMaker<const std::string &>::convert(da.name);
    }
    os << sep;

    os << "dim: [" << da.dim_x << ", " << da.dim_y << "]";
    os << sep;
    os << "w_dim: [" << da.w_dim_x << ", " << da.w_dim_y << "]";
    os << sep;
    os << "quality: " << da.quality;
    os << sep;
    os << "data_format: " << da.data_format;
    os << sep;
    os << "data_type: " << (Tango::CmdArgType) da.get_type();
    os << sep;
    os << "value: ";
    os << opc;
    Tango::detail::stringify_attribute_data(os, da);
    os << clc;

    os << clc;
    return os.str();
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
    os << "errors: " << StringMaker<Tango::DevErrorList>::convert(value.errors);
    os << clc;

    return os.str();
}

std::string StringMaker<TangoTest::AttrReadEventCopyable>::convert(TangoTest::AttrReadEventCopyable const &value)
{
    std::ostringstream os;

    os << opc;
    os << "attr_names: " << Catch::StringMaker<std::vector<std::string>>::convert(value.attr_names);
    os << sep;
    os << "argout: " << Catch::StringMaker<std::vector<Tango::DeviceAttribute>>::convert(value.argout);
    os << sep;
    os << "err: " << std::boolalpha << value.err;
    os << sep;
    os << "errors: " << Catch::StringMaker<Tango::DevErrorList>::convert(value.errors);
    os << clc;

    return os.str();
}

std::string StringMaker<CORBA::Any>::convert(CORBA::Any const &any)
{
    std::ostringstream os;

    os << opc;
    os << "value: ";
    Tango::detail::stringify_any(os, any);
    os << sep;
    os << "type: " << Tango::detail::corba_any_to_type_name(any);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::DevError>::convert(Tango::DevError const &err)
{
    std::ostringstream os;

    os << opc;
    os << "reason: " << StringMaker<const char *>::convert(err.reason.in());
    os << sep;
    os << "severity: " << StringMaker<Tango::ErrSeverity>::convert(err.severity);
    os << sep;
    os << "desc: " << StringMaker<const char *>::convert(err.desc.in());
    os << sep;
    os << "origin: " << StringMaker<const char *>::convert(err.origin.in());
    os << clc;

    return os.str();
}

} // namespace Catch
