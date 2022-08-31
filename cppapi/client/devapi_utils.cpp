//+==================================================================================================================
// devapi_utils.cpp 	- C++ source code file for TANGO device api
//
// programmer(s)	- Emmanuel Taurel(taurel@esrf.fr)
//
// original 		- November 2007
//
// Copyright (C) :      2007,2008,2009,2010,2011,2012,2013,2014,2015
//						European Synchrotron Radiation Facility
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

#include <tango/tango.h>


using namespace CORBA;

namespace Tango
{

//------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::from_hist4_2_DataHistory()
//
// description :
// 		Convert the command history as returned by a IDL 4 device to the classical DeviceDataHistory format
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceProxy::from_hist4_2_DataHistory(const DevCmdHistory_4_var &hist_4,std::vector<DeviceDataHistory> *ddh)
{

//
// Check received data validity
//

	if ((hist_4->dims.length() != hist_4->dims_array.length()) ||
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

	for (loop = 0;loop < h_depth;loop++)
	{
		(*ddh)[loop].set_date(hist_4->dates[loop]);
	}

//
// Copy errors
//

	for (loop = 0;loop < hist_4->errors.length();loop++)
	{
		int nb_elt = hist_4->errors_array[loop].nb_elt;
		int start = hist_4->errors_array[loop].start;

		for (k = 0;k < nb_elt;k++)
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

	for (loop = 0;loop < hist_4->dims.length();loop++)
	{
		int nb_elt = hist_4->dims_array[loop].nb_elt;
		int start = hist_4->dims_array[loop].start;

		for (k = 0;k < nb_elt;k++)
		{
			ad[start - k].dim_x = hist_4->dims[loop].dim_x;
			ad[start - k].dim_y = hist_4->dims[loop].dim_y;
		}
	}

//
// Get data ptr and data size
//

	switch (hist_4->cmd_type)
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
	if (ci.in_type_desc.empty() == false)
		o_str << "Command input parameter description = " << ci.in_type_desc << std::endl;

	o_str << "Command output parameter data type = Tango::" << data_type_to_string(ci.out_type) << std::endl;
	if (ci.out_type_desc.empty() == false)
		o_str << "Command output parameter description = " << ci.out_type_desc;

	return o_str;
}


bool _DeviceAttributeConfig::operator==(const _DeviceAttributeConfig &dac)
{
	  return name == dac.name &&
	         writable == dac.writable &&
	         data_format == dac.data_format &&
	         data_type == dac.data_type &&
	         max_dim_x == dac.max_dim_x &&
	         max_dim_y == dac.max_dim_y &&
	         description == dac.description &&
	         label == dac.label &&
	         unit == dac.unit &&
	         standard_unit == dac.standard_unit &&
	         display_unit == dac.display_unit &&
	         format == dac.format &&
	         min_value == dac.min_value &&
	         max_value == dac.max_value &&
	         min_alarm == dac.min_alarm &&
	         max_alarm == dac.max_alarm &&
	         writable_attr_name == dac.writable_attr_name &&
	         extensions == dac.extensions;
}

bool _AttributeInfo::operator==(const _AttributeInfo &ai)
{
	return DeviceAttributeConfig::operator==(ai) && disp_level == ai.disp_level;
}

bool _AttributeAlarmInfo::operator==(const _AttributeAlarmInfo &aai)
{
	return min_alarm == aai.min_alarm &&
		   max_alarm == aai.max_alarm &&
		   min_warning == aai.min_warning &&
		   max_warning == aai.max_warning &&
		   delta_t == aai.delta_t &&
		   delta_val == aai.delta_val &&
		   extensions == aai.extensions;
}

bool _AttributeEventInfo::operator==(const _AttributeEventInfo &aei)
{
	return ch_event.rel_change == aei.ch_event.rel_change &&
		   ch_event.abs_change == aei.ch_event.abs_change &&
		   ch_event.extensions == aei.ch_event.extensions &&
		   per_event.period == aei.per_event.period &&
		   per_event.extensions == aei.per_event.extensions &&
		   arch_event.archive_abs_change == aei.arch_event.archive_abs_change &&
		   arch_event.archive_rel_change == aei.arch_event.archive_rel_change &&
		   arch_event.archive_period == aei.arch_event.archive_period &&
		   arch_event.extensions == aei.arch_event.extensions;
}

bool _AttributeInfoEx::operator==(const _AttributeInfoEx &aie)
{
	return AttributeInfo::operator==(aie) &&
		   alarms.AttributeAlarmInfo::operator==(aie.alarms) &&
		   events.AttributeEventInfo::operator==(aie.events) &&
		   sys_extensions == aie.sys_extensions &&
		   root_attr_name == aie.root_attr_name &&
		   memorized == aie.memorized &&
		   enum_labels == aie.enum_labels;
}


} // End of namespace
