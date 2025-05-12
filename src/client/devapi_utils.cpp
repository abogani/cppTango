//+==================================================================================================================
// devapi_utils.cpp     - C++ source code file for TANGO device api
//
// programmer(s)    - Emmanuel Taurel(taurel@esrf.fr)
//
// original         - November 2007
//
// Copyright (C) :      2007,2008,2009,2010,2011,2012,2013,2014,2015
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Lesser Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//+==================================================================================================================

#include <tango/client/devapi.h>

using namespace CORBA;

namespace Tango
{

//-------------------------------------------------------------------------------------------------------------------
//
// Some operator method definition to make Python binding development easier
//
//-------------------------------------------------------------------------------------------------------------------

bool _DevCommandInfo::operator==(const _DevCommandInfo &dci)
{
    return cmd_tag == dci.cmd_tag && cmd_name == dci.cmd_name && in_type == dci.in_type && out_type == dci.out_type;
}

bool _CommandInfo::operator==(const _CommandInfo &ci)
{
    return _DevCommandInfo::operator==(ci) && disp_level == ci.disp_level;
}

std::ostream &operator<<(std::ostream &o_str, const _CommandInfo &ci)
{
    o_str << "Command name = " << ci.cmd_name << std::endl;

    o_str << "Command input parameter data type = Tango::" << data_type_to_string(ci.in_type) << std::endl;
    if(!ci.in_type_desc.empty())
    {
        o_str << "Command input parameter description = " << ci.in_type_desc << std::endl;
    }

    o_str << "Command output parameter data type = Tango::" << data_type_to_string(ci.out_type) << std::endl;
    if(!ci.out_type_desc.empty())
    {
        o_str << "Command output parameter description = " << ci.out_type_desc;
    }

    return o_str;
}

bool _DeviceAttributeConfig::operator==(const _DeviceAttributeConfig &dac)
{
    return name == dac.name && writable == dac.writable && data_format == dac.data_format &&
           data_type == dac.data_type && max_dim_x == dac.max_dim_x && max_dim_y == dac.max_dim_y &&
           description == dac.description && label == dac.label && unit == dac.unit &&
           standard_unit == dac.standard_unit && display_unit == dac.display_unit && format == dac.format &&
           min_value == dac.min_value && max_value == dac.max_value && min_alarm == dac.min_alarm &&
           max_alarm == dac.max_alarm && writable_attr_name == dac.writable_attr_name && extensions == dac.extensions;
}

bool _AttributeInfo::operator==(const _AttributeInfo &ai)
{
    return DeviceAttributeConfig::operator==(ai) && disp_level == ai.disp_level;
}

bool _AttributeAlarmInfo::operator==(const _AttributeAlarmInfo &aai)
{
    return min_alarm == aai.min_alarm && max_alarm == aai.max_alarm && min_warning == aai.min_warning &&
           max_warning == aai.max_warning && delta_t == aai.delta_t && delta_val == aai.delta_val &&
           extensions == aai.extensions;
}

bool _AttributeEventInfo::operator==(const _AttributeEventInfo &aei)
{
    return ch_event.rel_change == aei.ch_event.rel_change && ch_event.abs_change == aei.ch_event.abs_change &&
           ch_event.extensions == aei.ch_event.extensions && per_event.period == aei.per_event.period &&
           per_event.extensions == aei.per_event.extensions &&
           arch_event.archive_abs_change == aei.arch_event.archive_abs_change &&
           arch_event.archive_rel_change == aei.arch_event.archive_rel_change &&
           arch_event.archive_period == aei.arch_event.archive_period &&
           arch_event.extensions == aei.arch_event.extensions;
}

bool _AttributeInfoEx::operator==(const _AttributeInfoEx &aie)
{
    return AttributeInfo::operator==(aie) && alarms.AttributeAlarmInfo::operator==(aie.alarms) &&
           events.AttributeEventInfo::operator==(aie.events) && sys_extensions == aie.sys_extensions &&
           root_attr_name == aie.root_attr_name && memorized == aie.memorized && enum_labels == aie.enum_labels;
}

} // namespace Tango
