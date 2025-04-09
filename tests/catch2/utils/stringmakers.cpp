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
    os << "data_type: " << Tango::data_type_to_string(da.data_type);
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

std::string StringMaker<Tango::FwdEventData>::convert(Tango::FwdEventData const &value)
{
    std::ostringstream os;

    auto non_const_value = const_cast<Tango::FwdEventData &>(value);

    os << opc;
    os << "base class:" << StringMaker<Tango::EventData>::convert(dynamic_cast<Tango::EventData const &>(value));
    os << sep;
    os << "av_5: " << StringMaker<const Tango::AttributeValue_5 *>::convert(non_const_value.get_av_5());
    os << sep;
    os << "event_data: " << std::hex << non_const_value.get_zmq_mess_ptr();
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttrConfEventData>::convert(Tango::AttrConfEventData const &value)
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
    os << "attr_conf: " << opc << StringMaker<Tango::AttributeInfoEx *>::convert(value.attr_conf) << clc;
    os << sep;
    os << "err: " << std::boolalpha << value.err;
    os << sep;
    os << "errors: " << StringMaker<Tango::DevErrorList>::convert(value.errors);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::FwdAttrConfEventData>::convert(Tango::FwdAttrConfEventData const &value)
{
    std::ostringstream os;

    auto non_const_value = const_cast<Tango::FwdAttrConfEventData &>(value);

    os << opc;
    os << "base class:"
       << StringMaker<Tango::AttrConfEventData>::convert(dynamic_cast<Tango::AttrConfEventData const &>(value));
    os << sep;
    os << "av_5: " << StringMaker<Tango::AttributeConfig_5 *>::convert(non_const_value.get_fwd_attr_conf());
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::PipeEventData>::convert(Tango::PipeEventData const &value)
{
    std::ostringstream os;

    os << opc;
    os << "reception_date: " << StringMaker<Tango::TimeVal>::convert(value.reception_date);
    os << sep;
    os << "device: " << StringMaker<Tango::DeviceProxy *>::convert(value.device);
    os << sep;
    os << "pipe_name: " << value.pipe_name;
    os << sep;
    os << "event: " << value.event;
    os << sep;
    // As pipes are about to removed, we don't create a stringmaker for Tango::DevicePipe
    os << "pipe_value: " << opc << std::hex << value.pipe_value;
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

std::string StringMaker<Tango::DevIntrChangeEventData>::convert(Tango::DevIntrChangeEventData const &value)
{
    std::ostringstream os;

    os << opc;
    os << "reception_date: " << StringMaker<Tango::TimeVal>::convert(value.reception_date);
    os << sep;
    os << "device: " << StringMaker<Tango::DeviceProxy *>::convert(value.device);
    os << sep;
    os << "event: " << value.event;
    os << sep;
    os << "device_name: " << value.device_name;
    os << sep;
    os << "cmd_list: " << StringMaker<Tango::CommandInfoList>::convert(value.cmd_list);
    os << sep;
    os << "att_list: " << StringMaker<Tango::AttributeInfoListEx>::convert(value.att_list);
    os << sep;
    os << "dev_started: " << std::boolalpha << value.dev_started;
    os << sep;
    os << "err: " << std::boolalpha << value.err;
    os << sep;
    os << "errors: " << StringMaker<Tango::DevErrorList>::convert(value.errors);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::CommandInfo>::convert(Tango::CommandInfo const &value)
{
    std::ostringstream os;

    os << opc;
    os << "disp_level: " << StringMaker<Tango::DispLevel>::convert(value.disp_level); // CommandInfo addition
    os << sep;
    os << "cmd_name: " << value.cmd_name; // DevCommandInfo from here on
    os << sep;
    os << "cmd_tag: " << value.cmd_tag;
    os << sep;
    os << "in_type: " << Tango::data_type_to_string(value.in_type);
    os << sep;
    os << "out_type: " << Tango::data_type_to_string(value.out_type);
    os << sep;
    os << "in_type_desc: " << value.in_type_desc;
    os << sep;
    os << "out_type_desc: " << value.out_type_desc;
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttributeAlarmInfo>::convert(Tango::AttributeAlarmInfo const &value)
{
    std::ostringstream os;

    os << opc;
    os << "min_alarm: " << value.min_alarm;
    os << sep;
    os << "max_alarm: " << value.max_alarm;
    os << sep;
    os << "min_warning: " << value.min_warning;
    os << sep;
    os << "max_warning: " << value.max_warning;
    os << sep;
    os << "delta_t: " << value.delta_t;
    os << sep;
    os << "delta_val: " << value.delta_val;
    os << sep;
    os << "extensions: " << StringMaker<std::vector<std::string>>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::ChangeEventInfo>::convert(Tango::ChangeEventInfo const &value)
{
    std::ostringstream os;

    os << opc;
    os << "rel_change: " << value.rel_change;
    os << sep;
    os << "abs_change: " << value.abs_change;
    os << sep;
    os << "extensions: " << StringMaker<std::vector<std::string>>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::PeriodicEventInfo>::convert(Tango::PeriodicEventInfo const &value)
{
    std::ostringstream os;

    os << opc;
    os << "period: " << value.period;
    os << sep;
    os << "extensions: " << StringMaker<std::vector<std::string>>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::ArchiveEventInfo>::convert(Tango::ArchiveEventInfo const &value)
{
    std::ostringstream os;

    os << opc;
    os << "archive_rel_change: " << value.archive_rel_change;
    os << sep;
    os << "archive_abs_change: " << value.archive_abs_change;
    os << sep;
    os << "archive_period: " << value.archive_period;
    os << sep;
    os << "extensions: " << StringMaker<std::vector<std::string>>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttributeEventInfo>::convert(Tango::AttributeEventInfo const &value)
{
    std::ostringstream os;

    os << opc;
    os << "ch_event: " << StringMaker<Tango::ChangeEventInfo>::convert(value.ch_event);
    os << sep;
    os << "per_event: " << StringMaker<Tango::PeriodicEventInfo>::convert(value.per_event);
    os << sep;
    os << "arch_event: " << StringMaker<Tango::ArchiveEventInfo>::convert(value.arch_event);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttributeInfoEx>::convert(Tango::AttributeInfoEx const &value)
{
    std::ostringstream os;

    os << opc;
    // AttributeInfoEx additions
    os << "root_attr_name: " << value.root_attr_name;
    os << sep;
    os << "memorized: " << StringMaker<Tango::AttrMemorizedType>::convert(value.memorized);
    os << sep;
    os << "enum_labels: " << StringMaker<std::vector<std::string>>::convert(value.enum_labels);
    os << sep;
    os << "alarms: " << StringMaker<Tango::AttributeAlarmInfo>::convert(value.alarms);
    os << sep;
    os << "events: " << StringMaker<Tango::AttributeEventInfo>::convert(value.events);
    os << sep;
    os << "sys_extensions: " << StringMaker<std::vector<std::string>>::convert(value.sys_extensions);
    os << sep;
    os << "disp_level: " << StringMaker<Tango::DispLevel>::convert(value.disp_level); // AttributeInfo addition
    os << sep;
    os << "name: " << value.name; // DeviceAttributeConfig additions
    os << sep;
    os << "writable: " << StringMaker<Tango::AttrWriteType>::convert(value.writable);
    os << sep;
    os << "data_format: " << StringMaker<Tango::AttrDataFormat>::convert(value.data_format);
    os << sep;
    os << "data_type: " << Tango::data_type_to_string(value.data_type);
    os << sep;
    os << "max_dim_x: " << value.max_dim_x;
    os << sep;
    os << "max_dim_y: " << value.max_dim_y;
    os << sep;
    os << "description: " << value.description;
    os << sep;
    os << "label: " << value.label;
    os << sep;
    os << "unit: " << value.unit;
    os << sep;
    os << "standard_unit: " << value.standard_unit;
    os << sep;
    os << "display_unit: " << value.display_unit;
    os << sep;
    os << "format: " << value.format;
    os << sep;
    os << "min_value: " << value.min_value;
    os << sep;
    os << "max_value: " << value.max_value;
    os << sep;
    os << "min_alarm: " << value.min_alarm;
    os << sep;
    os << "max_alarm: " << value.max_alarm;
    os << sep;
    os << "writable_attr_name: " << value.writable_attr_name;
    os << sep;
    os << "extensions: " << StringMaker<std::vector<std::string>>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::DataReadyEventData>::convert(Tango::DataReadyEventData const &value)
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
    os << "attr_data_type: " << Tango::data_type_to_string(value.attr_data_type);
    os << sep;
    os << "ctr: " << value.ctr;
    os << sep;
    os << "err: " << std::boolalpha << value.err;
    os << sep;
    os << "errors: " << StringMaker<Tango::DevErrorList>::convert(value.errors);
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

std::string StringMaker<TangoTest::ExitStatus>::convert(TangoTest::ExitStatus const &status)
{
    using Kind = TangoTest::ExitStatus::Kind;

    std::ostringstream os;
    os << opc;
    os << "kind: ";
    switch(status.kind)
    {
    case Kind::Normal:
    {
        os << "Normal";
        os << sep;
        os << "code: " << status.code;
        break;
    }
    case Kind::Aborted:
    {
        os << "Aborted";
        os << sep;
        os << "signal: " << status.signal;
        break;
    }
    case Kind::AbortedNoSignal:
    {
        os << "AbortedNoSignal";
        break;
    }
    };
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttributeAlarm>::convert(const Tango::AttributeAlarm &value)
{
    std::ostringstream os;

    os << opc;
    os << "min_alarm: " << value.min_alarm;
    os << sep;
    os << "max_alarm: " << value.max_alarm;
    os << sep;
    os << "min_warning: " << value.min_warning;
    os << sep;
    os << "max_warning: " << value.max_warning;
    os << sep;
    os << "delta_t: " << value.delta_t;
    os << sep;
    os << "delta_val: " << value.delta_val;
    os << sep;
    os << "extensions: " << StringMaker<Tango::DevVarStringArray>::convert(value.extensions);
    os << sep;
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::EventProperties>::convert(const Tango::EventProperties &value)
{
    std::ostringstream os;

    os << opc;
    os << "ch_event: " << StringMaker<Tango::ChangeEventProp>::convert(value.ch_event);
    os << sep;
    os << "per_event: " << StringMaker<Tango::PeriodicEventProp>::convert(value.per_event);
    os << sep;
    os << "arch_event: " << StringMaker<Tango::ArchiveEventProp>::convert(value.arch_event);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::ChangeEventProp>::convert(const Tango::ChangeEventProp &value)
{
    std::ostringstream os;

    os << opc;
    os << "rel_change: " << value.rel_change;
    os << sep;
    os << "abs_change: " << value.abs_change;
    os << sep;
    os << "extensions: " << StringMaker<Tango::DevVarStringArray>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::PeriodicEventProp>::convert(const Tango::PeriodicEventProp &value)
{
    std::ostringstream os;

    os << opc;
    os << "period: " << value.period;
    os << sep;
    os << "extensions: " << StringMaker<Tango::DevVarStringArray>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::ArchiveEventProp>::convert(const Tango::ArchiveEventProp &value)
{
    std::ostringstream os;

    os << opc;
    os << "rel_change: " << value.rel_change;
    os << sep;
    os << "abs_change: " << value.abs_change;
    os << sep;
    os << "period: " << value.period;
    os << sep;
    os << "extensions: " << StringMaker<Tango::DevVarStringArray>::convert(value.extensions);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttributeDim>::convert(const Tango::AttributeDim &value)
{
    std::ostringstream os;

    os << opc;
    os << "dim_x: " << value.dim_x;
    os << sep;
    os << "dim_y: " << value.dim_y;
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttributeValue_5 *>::convert(const Tango::AttributeValue_5 *attr_val)
{
    if(attr_val == nullptr)
    {
        return Catch::StringMaker<std::nullptr_t>::convert(nullptr);
    }

    return Catch::StringMaker<Tango::AttributeValue_5>::convert(*attr_val);
}

std::string StringMaker<Tango::AttributeValue_5>::convert(const Tango::AttributeValue_5 &value)
{
    std::ostringstream os;

    os << opc;
    os << "value: (not yet supported)";
    os << sep;
    os << "quality: " << StringMaker<Tango::AttrQuality>::convert(value.quality);
    os << sep;
    os << "data_format: " << StringMaker<Tango::AttrDataFormat>::convert(value.data_format);
    os << sep;
    os << "data_type: " << Tango::data_type_to_string(value.data_type);
    os << sep;
    os << "time: " << StringMaker<Tango::TimeVal>::convert(value.time);
    os << sep;
    os << "name: " << value.name;
    os << sep;
    os << "r_dim: " << StringMaker<Tango::AttributeDim>::convert(value.r_dim);
    os << sep;
    os << "w_dim: " << StringMaker<Tango::AttributeDim>::convert(value.w_dim);
    os << sep;
    os << "err_list: " << StringMaker<Tango::DevErrorList>::convert(value.err_list);
    os << clc;

    return os.str();
}

std::string StringMaker<Tango::AttributeConfig_5 *>::convert(const Tango::AttributeConfig_5 *attr_conf)
{
    if(attr_conf == nullptr)
    {
        return Catch::StringMaker<std::nullptr_t>::convert(nullptr);
    }

    return Catch::StringMaker<Tango::AttributeConfig_5>::convert(*attr_conf);
}

std::string StringMaker<Tango::AttributeConfig_5>::convert(Tango::AttributeConfig_5 const &value)
{
    std::ostringstream os;

    os << opc;
    os << "name: " << value.name;
    os << sep;
    os << "writable: " << StringMaker<Tango::AttrWriteType>::convert(value.writable);
    os << sep;
    os << "data_format: " << StringMaker<Tango::AttrDataFormat>::convert(value.data_format);
    os << sep;
    os << "data_type: " << Tango::data_type_to_string(value.data_type);
    os << sep;
    os << "memorized: " << std::boolalpha << value.memorized;
    os << sep;
    os << "mem_init: " << std::boolalpha << value.mem_init;
    os << sep;
    os << "max_dim_x: " << value.max_dim_x;
    os << sep;
    os << "max_dim_y: " << value.max_dim_y;
    os << sep;
    os << "description: " << value.description;
    os << sep;
    os << "label: " << value.label;
    os << sep;
    os << "unit: " << value.unit;
    os << sep;
    os << "standard_unit: " << value.standard_unit;
    os << sep;
    os << "display_unit: " << value.display_unit;
    os << sep;
    os << "format: " << value.format;
    os << sep;
    os << "min_value: " << value.min_value;
    os << sep;
    os << "max_value: " << value.max_value;
    os << sep;
    os << "writable_attr_name: " << value.writable_attr_name;
    os << sep;
    os << "level: " << StringMaker<Tango::DispLevel>::convert(value.level);
    os << sep;
    os << "root_attr_name: " << value.root_attr_name;
    os << sep;
    os << "enum_labels: " << StringMaker<Tango::DevVarStringArray>::convert(value.enum_labels);
    os << sep;
    os << "att_alarm: " << StringMaker<Tango::AttributeAlarm>::convert(value.att_alarm);
    os << sep;
    os << "event_prop: " << StringMaker<Tango::EventProperties>::convert(value.event_prop);
    os << sep;
    os << "extensions: " << StringMaker<Tango::DevVarStringArray>::convert(value.extensions);
    os << sep;
    os << "sys_extensions: " << StringMaker<Tango::DevVarStringArray>::convert(value.sys_extensions);
    os << sep;
    os << clc;

    return os.str();
}

} // namespace Catch
