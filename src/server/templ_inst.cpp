//+=================================================================================================================
//
// file :               templ_inst.cpp
//
// description :        C++ source code to instantiate template methods/class in order to have explicit instanciation
//                        This makes user life easier in term of getting bug fixes (no re-compilation needed)
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2013,2014,2015
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
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//-==================================================================================================================

//
// For template management we have several files:
// - A file with explicit template instanciation. This file (templ_inst.cpp)
// For class with templates:
// - A file with template methods/class definition (attribute_templ.h for Attribute class)
// - A file with template methods/class specialization definition (attribute_spec_templ.h for Attribute class)
//
// We did this in order to have explicit instanciation of templates method except for specialized templates
// for which we have instanciation following the inclusion model
// See C++ template book chapter 6
//

#include <tango/server/device.h>
#include <tango/common/utils/assert.h>
#include <tango/server/attribute_templ.h>
#include <tango/server/pollext_templ.h>
#include <tango/server/utils_templ.h>
#include <tango/server/w_attrsetval_templ.h>
#include <tango/client/devapi_pipe_templ.h>
#include <tango/server/pipe_templ.h>
#include <tango/server/w_pipe.h>
#include <tango/server/w_pipe_templ.h>

namespace Tango
{

//
// The DevEncoded and DevString data types are not there. DevString is handle by 2 (for const) classic methods
// forwarding the call to the same method using string data type as parameter. DevEncoded and string are
// managed as specialized templates
//

#define TEMPL_EXPL_CLASS_INST(CLASS)  \
    template class CLASS<DevBoolean>; \
    template class CLASS<DevUChar>;   \
    template class CLASS<DevShort>;   \
    template class CLASS<DevUShort>;  \
    template class CLASS<DevLong>;    \
    template class CLASS<DevULong>;   \
    template class CLASS<DevLong64>;  \
    template class CLASS<DevULong64>; \
    template class CLASS<DevFloat>;   \
    template class CLASS<DevDouble>;  \
    template class CLASS<DevState>;

// template class CLASS<DevString>;
// template class CLASS<DevEncoded>;

#define TEMPL_EXPL_CMD_CLASS_INST(CLASS)         \
    template class CLASS<DevBoolean>;            \
    template class CLASS<DevUChar>;              \
    template class CLASS<DevShort>;              \
    template class CLASS<DevUShort>;             \
    template class CLASS<DevLong>;               \
    template class CLASS<DevULong>;              \
    template class CLASS<DevLong64>;             \
    template class CLASS<DevULong64>;            \
    template class CLASS<DevFloat>;              \
    template class CLASS<DevDouble>;             \
    template class CLASS<DevState>;              \
    template class CLASS<DevString>;             \
    template class CLASS<DevEncoded>;            \
    template class CLASS<DevVarBooleanArray>;    \
    template class CLASS<DevVarCharArray>;       \
    template class CLASS<DevVarShortArray>;      \
    template class CLASS<DevVarUShortArray>;     \
    template class CLASS<DevVarLongArray>;       \
    template class CLASS<DevVarULongArray>;      \
    template class CLASS<DevVarLong64Array>;     \
    template class CLASS<DevVarULong64Array>;    \
    template class CLASS<DevVarFloatArray>;      \
    template class CLASS<DevVarDoubleArray>;     \
    template class CLASS<DevVarStringArray>;     \
    template class CLASS<DevVarLongStringArray>; \
    template class CLASS<DevVarDoubleStringArray>;

//+----------------------------------------------------------------------------
//
// Instantiate Attribute::XXX methods and associate template classes
//
//-----------------------------------------------------------------------------

TEMPL_EXPL_CLASS_INST(AttrProp)
TEMPL_EXPL_CLASS_INST(DoubleAttrProp)
TEMPL_EXPL_CLASS_INST(MultiAttrProp)

template void Attribute::check_hard_coded_properties(const AttributeConfig &);
template void Attribute::check_hard_coded_properties(const AttributeConfig_3 &);

template void Attribute::set_hard_coded_properties(const AttributeConfig &);
template void Attribute::set_hard_coded_properties(const AttributeConfig_3 &);

template void Attribute::Attribute_2_AttributeValue_base(AttributeValue_4 *, Tango::DeviceImpl *);
template void Attribute::Attribute_2_AttributeValue_base(AttributeValue_5 *, Tango::DeviceImpl *);

template void Attribute::AttrValUnion_fake_copy(const AttributeValue_5 *, AttributeValue_4 *);
template void Attribute::AttrValUnion_fake_copy(const AttributeValue_4 *, AttributeValue_5 *);

template void Attribute::AttrValUnion_2_Any(const AttributeValue_4 *, CORBA::Any &);
template void Attribute::AttrValUnion_2_Any(const AttributeValue_5 *, CORBA::Any &);

template void Attribute::_extract_value<Tango::DevVarLong64Array>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarDoubleArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarLongArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarULongArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarBooleanArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarStateArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarShortArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarCharArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarFloatArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarUShortArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarStringArray>(CORBA::Any &);
template void Attribute::_extract_value<Tango::DevVarULong64Array>(CORBA::Any &);

//+----------------------------------------------------------------------------
//
// Instantiate Util::fill_XXX_polling buffers methods and associate template classes
//
//-----------------------------------------------------------------------------

TEMPL_EXPL_CLASS_INST(AttrData)
TEMPL_EXPL_CLASS_INST(TimedAttrData)
TEMPL_EXPL_CLASS_INST(AttrHistoryStack)

template class AttrData<DevString>;
template class AttrData<DevEncoded>;

template class TimedAttrData<DevString>;
template class TimedAttrData<DevEncoded>;

template class AttrHistoryStack<DevString>;
template class AttrHistoryStack<DevEncoded>;

template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevBoolean> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevUChar> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevShort> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevUShort> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevLong> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevULong> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevLong64> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevULong64> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevFloat> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevDouble> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevState> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevString> &);
template void Util::fill_attr_polling_buffer(DeviceImpl *, std::string &, AttrHistoryStack<DevEncoded> &);

TEMPL_EXPL_CMD_CLASS_INST(TimedCmdData)
TEMPL_EXPL_CMD_CLASS_INST(CmdHistoryStack)

template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevShort> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevUShort> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevLong> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevULong> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevLong64> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevULong64> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevFloat> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevDouble> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevState> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevString> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevEncoded> &);

template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarBooleanArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarCharArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarShortArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarUShortArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarLongArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarULongArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarLong64Array> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarULong64Array> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarFloatArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarDoubleArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarStringArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarLongStringArray> &);
template void Util::fill_cmd_polling_buffer(DeviceImpl *, std::string &, CmdHistoryStack<DevVarDoubleStringArray> &);

//+----------------------------------------------------------------------------
//
// Instantiate DevicePipe helper functions
//
//-----------------------------------------------------------------------------

template DevicePipe &operator<<(DevicePipe &, DevBoolean &);
template DevicePipe &operator<<(DevicePipe &, short &);
template DevicePipe &operator<<(DevicePipe &, DevLong &);
template DevicePipe &operator<<(DevicePipe &, DevLong64 &);
template DevicePipe &operator<<(DevicePipe &, float &);
template DevicePipe &operator<<(DevicePipe &, double &);
template DevicePipe &operator<<(DevicePipe &, DevUChar &);
template DevicePipe &operator<<(DevicePipe &, DevUShort &);
template DevicePipe &operator<<(DevicePipe &, DevULong &);
template DevicePipe &operator<<(DevicePipe &, DevULong64 &);
template DevicePipe &operator<<(DevicePipe &, DevString &);
template DevicePipe &operator<<(DevicePipe &, DevState &);
template DevicePipe &operator<<(DevicePipe &, DevEncoded &);
template DevicePipe &operator<<(DevicePipe &, std::string &);

template DevicePipe &operator<<(DevicePipe &, DevicePipeBlob &);

template DevicePipe &operator<<(DevicePipe &, DataElement<DevBoolean> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<short> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevLong> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevLong64> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<float> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<double> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevUChar> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevUShort> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevULong> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevULong64> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevString> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevState> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevEncoded> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<const std::string> &);

template DevicePipe &operator<<(DevicePipe &, DataElement<DevicePipeBlob> &);

template DevicePipe &operator<<(DevicePipe &, std::vector<DevBoolean> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<short> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevLong> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevLong64> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<float> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<double> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevUChar> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevUShort> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevULong> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevULong64> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevString> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<DevState> &);
template DevicePipe &operator<<(DevicePipe &, std::vector<std::string> &);

template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevBoolean>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<short>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevLong>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevLong64>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<float>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<double>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevUChar>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevUShort>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevULong>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevULong64>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevString>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<DevState>> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<std::vector<std::string>> &);

template DevicePipe &operator<<(DevicePipe &, DevVarBooleanArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarShortArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarLongArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarLong64Array &);
template DevicePipe &operator<<(DevicePipe &, DevVarFloatArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarDoubleArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarUCharArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarUShortArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarULongArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarULong64Array &);
template DevicePipe &operator<<(DevicePipe &, DevVarStringArray &);
template DevicePipe &operator<<(DevicePipe &, DevVarStateArray &);

template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarBooleanArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarShortArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarLongArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarLong64Array &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarFloatArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarDoubleArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarUCharArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarUShortArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarULongArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarULong64Array &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarStringArray &> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarStateArray &> &);

template DevicePipe &operator<<(DevicePipe &, DevVarBooleanArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarShortArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarLongArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarLong64Array *);
template DevicePipe &operator<<(DevicePipe &, DevVarFloatArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarDoubleArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarUCharArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarUShortArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarULongArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarULong64Array *);
template DevicePipe &operator<<(DevicePipe &, DevVarStringArray *);
template DevicePipe &operator<<(DevicePipe &, DevVarStateArray *);

template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarBooleanArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarShortArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarLongArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarLong64Array *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarFloatArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarDoubleArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarUCharArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarUShortArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarULongArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarULong64Array *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarStringArray *> &);
template DevicePipe &operator<<(DevicePipe &, DataElement<DevVarStateArray *> &);

//-----------------------------------------------------------------------------------------------------

//
// Note that there is no instanciation for type DevString. For this type, a  non template method has been written
// Otherwise, due the partial specialization for pointer types, it was not possible to use extracttion into
// DevString type
//

template DevicePipe &operator>>(DevicePipe &, DevBoolean &);
template DevicePipe &operator>>(DevicePipe &, short &);
template DevicePipe &operator>>(DevicePipe &, DevLong &);
template DevicePipe &operator>>(DevicePipe &, DevLong64 &);
template DevicePipe &operator>>(DevicePipe &, float &);
template DevicePipe &operator>>(DevicePipe &, double &);
template DevicePipe &operator>>(DevicePipe &, DevUChar &);
template DevicePipe &operator>>(DevicePipe &, DevUShort &);
template DevicePipe &operator>>(DevicePipe &, DevULong &);
template DevicePipe &operator>>(DevicePipe &, DevULong64 &);
// Missing DevString (see above)
template DevicePipe &operator>>(DevicePipe &, DevState &);
template DevicePipe &operator>>(DevicePipe &, DevEncoded &);
template DevicePipe &operator>>(DevicePipe &, std::string &);

template DevicePipe &operator>>(DevicePipe &, DevicePipeBlob &);

template DevicePipe &operator>>(DevicePipe &, DataElement<DevBoolean> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<short> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevLong> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevLong64> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<float> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<double> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevUChar> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevUShort> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevULong> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevULong64> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevString> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevState> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevEncoded> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::string> &);

template DevicePipe &operator>>(DevicePipe &, DataElement<DevicePipeBlob> &);

template DevicePipe &operator>>(DevicePipe &, std::vector<DevBoolean> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<short> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<DevLong> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<DevLong64> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<float> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<double> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<DevUChar> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<DevUShort> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<DevULong> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<DevULong64> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<std::string> &);
template DevicePipe &operator>>(DevicePipe &, std::vector<DevState> &);
// template DevicePipe &operator>>(DevicePipe &,std::vector<DevEncoded> &);

template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevBoolean>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<short>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevLong>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevLong64>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<float>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<double>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevUChar>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevUShort>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevULong>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevULong64>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<std::string>> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<std::vector<DevState>> &);
// template DevicePipe &operator>>(DevicePipe &,DataElement<std::vector<DevEncoded> > &);

template DevicePipe &operator>>(DevicePipe &, DevVarBooleanArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarShortArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarLongArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarLong64Array *);
template DevicePipe &operator>>(DevicePipe &, DevVarFloatArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarDoubleArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarUCharArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarUShortArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarULongArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarULong64Array *);
template DevicePipe &operator>>(DevicePipe &, DevVarStringArray *);
template DevicePipe &operator>>(DevicePipe &, DevVarStateArray *);
// template DevicePipe &operator>>(DevicePipe &,DevVarEncodedArray *);

template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarBooleanArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarShortArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarLongArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarLong64Array *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarFloatArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarDoubleArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarUCharArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarUShortArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarULongArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarULong64Array *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarStringArray *> &);
template DevicePipe &operator>>(DevicePipe &, DataElement<DevVarStateArray *> &);
// template DevicePipe &operator>>(DevicePipe &,DataElement<DevVarEncodedArray *> &);

//+----------------------------------------------------------------------------
//
// Instantiate DevicePipeBlob helper functions
//
//-----------------------------------------------------------------------------

template DevicePipeBlob &operator<<(DevicePipeBlob &, DevBoolean &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, short &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevLong &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevLong64 &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, float &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, double &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevUChar &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevUShort &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevULong &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevULong64 &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevString &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevState &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevEncoded &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, const std::string &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::string &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DevicePipeBlob &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevBoolean> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<short> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevLong> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevLong64> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<float> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<double> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevUChar> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevUShort> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevULong> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevULong64> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevString> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevState> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevEncoded> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<const std::string> &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevicePipeBlob> &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevBoolean> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<short> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevLong> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevLong64> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<float> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<double> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevUChar> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevUShort> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevULong> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevULong64> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevString> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<DevState> &);
// template DevicePipeBlob &operator<<(DevicePipeBlob &,std::vector<DevEncoded> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, std::vector<std::string> &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevBoolean>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<short>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevLong>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevLong64>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<float>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<double>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevUChar>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevUShort>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevULong>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevULong64>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevString>> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<DevState>> &);
// template DevicePipeBlob &operator<<(DevicePipeBlob &,DataElement<std::vector<DevEncoded> > &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<std::vector<std::string>> &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarBooleanArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarShortArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarLongArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarLong64Array &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarFloatArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarDoubleArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarUCharArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarUShortArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarULongArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarULong64Array &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarStringArray &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarStateArray &);
// template DevicePipeBlob &operator<<(DevicePipeBlob &,DevVarEncodedArray &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarBooleanArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarShortArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarLongArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarLong64Array &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarFloatArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarDoubleArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarUCharArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarUShortArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarULongArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarULong64Array &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarStringArray &> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarStateArray &> &);
// template DevicePipeBlob &operator<<(DevicePipeBlob &,DataElement<DevVarEncodedArray &> &);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarBooleanArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarShortArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarLongArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarLong64Array *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarFloatArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarDoubleArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarUCharArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarUShortArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarULongArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarULong64Array *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarStringArray *);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DevVarStateArray *);
// template DevicePipeBlob &operator<<(DevicePipeBlob &,DevVarEncodedArray *);

template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarBooleanArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarShortArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarLongArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarLong64Array *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarFloatArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarDoubleArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarUCharArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarUShortArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarULongArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarULong64Array *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarStringArray *> &);
template DevicePipeBlob &operator<<(DevicePipeBlob &, DataElement<DevVarStateArray *> &);
// template DevicePipeBlob &operator<<(DevicePipeBlob &,DataElement<DevVarEncodedArray *> &);

//-------------------------------------------------------------------------------------------

template DevicePipeBlob &operator>>(DevicePipeBlob &, DevBoolean &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, short &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevLong &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevLong64 &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, float &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, double &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevUChar &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevUShort &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevULong &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevULong64 &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevString &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevState &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevEncoded &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::string &);

template DevicePipeBlob &operator>>(DevicePipeBlob &, DevicePipeBlob &);

template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevBoolean> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<short> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevLong> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevLong64> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<float> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<double> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevUChar> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevUShort> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevULong> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevULong64> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevString> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevState> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevEncoded> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::string> &);

template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevicePipeBlob> &);

template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevBoolean> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<short> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevLong> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevLong64> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<float> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<double> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevUChar> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevUShort> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevULong> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevULong64> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<std::string> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, std::vector<DevState> &);
// template DevicePipeBlob &operator>>(DevicePipeBlob &,std::vector<DevEncoded> &);

template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevBoolean>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<short>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevLong>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevLong64>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<float>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<double>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevUChar>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevUShort>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevULong>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevULong64>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<std::string>> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<std::vector<DevState>> &);
// template DevicePipeBlob &operator>>(DevicePipeBlob &,DataElement<std::vector<DevEncoded> > &);

template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarBooleanArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarShortArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarLongArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarLong64Array *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarFloatArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarDoubleArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarUCharArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarUShortArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarULongArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarULong64Array *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarStringArray *);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DevVarStateArray *);
// template DevicePipeBlob &operator>>(DevicePipeBlob &,DevVarEncodedArray *);

template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarBooleanArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarShortArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarLongArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarLong64Array *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarFloatArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarDoubleArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarUCharArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarUShortArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarULongArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarULong64Array *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarStringArray *> &);
template DevicePipeBlob &operator>>(DevicePipeBlob &, DataElement<DevVarStateArray *> &);
// template DevicePipeBlob &operator>>(DevicePipeBlob &,DataElement<DevVarEncodedArray *> &);

//+----------------------------------------------------------------------------
//
// Instantiate WPipe helper functions
//
//-----------------------------------------------------------------------------

template WPipe &operator>>(WPipe &, DevBoolean &);
template WPipe &operator>>(WPipe &, short &);
template WPipe &operator>>(WPipe &, DevLong &);
template WPipe &operator>>(WPipe &, DevLong64 &);
template WPipe &operator>>(WPipe &, float &);
template WPipe &operator>>(WPipe &, double &);
template WPipe &operator>>(WPipe &, DevUChar &);
template WPipe &operator>>(WPipe &, DevUShort &);
template WPipe &operator>>(WPipe &, DevULong &);
template WPipe &operator>>(WPipe &, DevULong64 &);
template WPipe &operator>>(WPipe &, DevString &);
template WPipe &operator>>(WPipe &, DevState &);
template WPipe &operator>>(WPipe &, DevEncoded &);
template WPipe &operator>>(WPipe &, std::string &);

template WPipe &operator>>(WPipe &, DevicePipeBlob &);

template WPipe &operator>>(WPipe &, DataElement<DevBoolean> &);
template WPipe &operator>>(WPipe &, DataElement<short> &);
template WPipe &operator>>(WPipe &, DataElement<DevLong> &);
template WPipe &operator>>(WPipe &, DataElement<DevLong64> &);
template WPipe &operator>>(WPipe &, DataElement<float> &);
template WPipe &operator>>(WPipe &, DataElement<double> &);
template WPipe &operator>>(WPipe &, DataElement<DevUChar> &);
template WPipe &operator>>(WPipe &, DataElement<DevUShort> &);
template WPipe &operator>>(WPipe &, DataElement<DevULong> &);
template WPipe &operator>>(WPipe &, DataElement<DevULong64> &);
template WPipe &operator>>(WPipe &, DataElement<DevString> &);
template WPipe &operator>>(WPipe &, DataElement<DevState> &);
template WPipe &operator>>(WPipe &, DataElement<DevEncoded> &);
template WPipe &operator>>(WPipe &, DataElement<std::string> &);

template WPipe &operator>>(WPipe &, DataElement<DevicePipeBlob> &);

template WPipe &operator>>(WPipe &, std::vector<DevBoolean> &);
template WPipe &operator>>(WPipe &, std::vector<short> &);
template WPipe &operator>>(WPipe &, std::vector<DevLong> &);
template WPipe &operator>>(WPipe &, std::vector<DevLong64> &);
template WPipe &operator>>(WPipe &, std::vector<float> &);
template WPipe &operator>>(WPipe &, std::vector<double> &);
template WPipe &operator>>(WPipe &, std::vector<DevUChar> &);
template WPipe &operator>>(WPipe &, std::vector<DevUShort> &);
template WPipe &operator>>(WPipe &, std::vector<DevULong> &);
template WPipe &operator>>(WPipe &, std::vector<DevULong64> &);
template WPipe &operator>>(WPipe &, std::vector<std::string> &);
template WPipe &operator>>(WPipe &, std::vector<DevState> &);
// template WPipe &operator>>(WPipe &,std::vector<DevEncoded> &);

template WPipe &operator>>(WPipe &, DataElement<std::vector<DevBoolean>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<short>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<DevLong>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<DevLong64>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<float>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<double>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<DevUChar>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<DevUShort>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<DevULong>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<DevULong64>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<std::string>> &);
template WPipe &operator>>(WPipe &, DataElement<std::vector<DevState>> &);
// template WPipe &operator>>(WPipe &,DataElement<std::vector<DevEncoded> > &);

template WPipe &operator>>(WPipe &, DevVarBooleanArray *);
template WPipe &operator>>(WPipe &, DevVarShortArray *);
template WPipe &operator>>(WPipe &, DevVarLongArray *);
template WPipe &operator>>(WPipe &, DevVarLong64Array *);
template WPipe &operator>>(WPipe &, DevVarFloatArray *);
template WPipe &operator>>(WPipe &, DevVarDoubleArray *);
template WPipe &operator>>(WPipe &, DevVarUCharArray *);
template WPipe &operator>>(WPipe &, DevVarUShortArray *);
template WPipe &operator>>(WPipe &, DevVarULongArray *);
template WPipe &operator>>(WPipe &, DevVarULong64Array *);
template WPipe &operator>>(WPipe &, DevVarStringArray *);
template WPipe &operator>>(WPipe &, DevVarStateArray *);
// template WPipe &operator>>(WPipe &,DevVarEncodedArray *);

template WPipe &operator>>(WPipe &, DataElement<DevVarBooleanArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarShortArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarLongArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarLong64Array *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarFloatArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarDoubleArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarUCharArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarUShortArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarULongArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarULong64Array *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarStringArray *> &);
template WPipe &operator>>(WPipe &, DataElement<DevVarStateArray *> &);
// template WPipe &operator>>(WPipe &,DataElement<DevVarEncodedArray *> &);

//+----------------------------------------------------------------------------
//
// Instantiate Pipe helper functions
//
//-----------------------------------------------------------------------------

template Pipe &operator<<(Pipe &, DevBoolean &);
template Pipe &operator<<(Pipe &, short &);
template Pipe &operator<<(Pipe &, DevLong &);
template Pipe &operator<<(Pipe &, DevLong64 &);
template Pipe &operator<<(Pipe &, float &);
template Pipe &operator<<(Pipe &, double &);
template Pipe &operator<<(Pipe &, DevUChar &);
template Pipe &operator<<(Pipe &, DevUShort &);
template Pipe &operator<<(Pipe &, DevULong &);
template Pipe &operator<<(Pipe &, DevULong64 &);
template Pipe &operator<<(Pipe &, DevString &);
template Pipe &operator<<(Pipe &, DevState &);
template Pipe &operator<<(Pipe &, DevEncoded &);
template Pipe &operator<<(Pipe &, const std::string &);

template Pipe &operator<<(Pipe &, DevicePipeBlob &);

template Pipe &operator<<(Pipe &, DataElement<DevBoolean> &);
template Pipe &operator<<(Pipe &, DataElement<short> &);
template Pipe &operator<<(Pipe &, DataElement<DevLong> &);
template Pipe &operator<<(Pipe &, DataElement<DevLong64> &);
template Pipe &operator<<(Pipe &, DataElement<float> &);
template Pipe &operator<<(Pipe &, DataElement<double> &);
template Pipe &operator<<(Pipe &, DataElement<DevUChar> &);
template Pipe &operator<<(Pipe &, DataElement<DevUShort> &);
template Pipe &operator<<(Pipe &, DataElement<DevULong> &);
template Pipe &operator<<(Pipe &, DataElement<DevULong64> &);
template Pipe &operator<<(Pipe &, DataElement<DevString> &);
template Pipe &operator<<(Pipe &, DataElement<DevState> &);
template Pipe &operator<<(Pipe &, DataElement<DevEncoded> &);
template Pipe &operator<<(Pipe &, DataElement<const std::string> &);

template Pipe &operator<<(Pipe &, DataElement<DevicePipeBlob> &);

template Pipe &operator<<(Pipe &, std::vector<DevBoolean> &);
template Pipe &operator<<(Pipe &, std::vector<short> &);
template Pipe &operator<<(Pipe &, std::vector<DevLong> &);
template Pipe &operator<<(Pipe &, std::vector<DevLong64> &);
template Pipe &operator<<(Pipe &, std::vector<float> &);
template Pipe &operator<<(Pipe &, std::vector<double> &);
template Pipe &operator<<(Pipe &, std::vector<DevUChar> &);
template Pipe &operator<<(Pipe &, std::vector<DevUShort> &);
template Pipe &operator<<(Pipe &, std::vector<DevULong> &);
template Pipe &operator<<(Pipe &, std::vector<DevULong64> &);
template Pipe &operator<<(Pipe &, std::vector<DevString> &);
template Pipe &operator<<(Pipe &, std::vector<DevState> &);
// template Pipe &operator<<(Pipe &,std::vector<DevEncoded> &);
template Pipe &operator<<(Pipe &, std::vector<std::string> &);

template Pipe &operator<<(Pipe &, DataElement<std::vector<DevBoolean>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<short>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevLong>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevLong64>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<float>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<double>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevUChar>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevUShort>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevULong>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevULong64>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevString>> &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<DevState>> &);
// template Pipe &operator<<(Pipe &,DataElement<std::vector<DevEncoded> > &);
template Pipe &operator<<(Pipe &, DataElement<std::vector<std::string>> &);

template Pipe &operator<<(Pipe &, DevVarBooleanArray &);
template Pipe &operator<<(Pipe &, DevVarShortArray &);
template Pipe &operator<<(Pipe &, DevVarLongArray &);
template Pipe &operator<<(Pipe &, DevVarLong64Array &);
template Pipe &operator<<(Pipe &, DevVarFloatArray &);
template Pipe &operator<<(Pipe &, DevVarDoubleArray &);
template Pipe &operator<<(Pipe &, DevVarUCharArray &);
template Pipe &operator<<(Pipe &, DevVarUShortArray &);
template Pipe &operator<<(Pipe &, DevVarULongArray &);
template Pipe &operator<<(Pipe &, DevVarULong64Array &);
template Pipe &operator<<(Pipe &, DevVarStringArray &);
template Pipe &operator<<(Pipe &, DevVarStateArray &);
// template Pipe &operator<<(Pipe &,DevVarEncodedArray &);

template Pipe &operator<<(Pipe &, DataElement<DevVarBooleanArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarShortArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarLongArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarLong64Array &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarFloatArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarDoubleArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarUCharArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarUShortArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarULongArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarULong64Array &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarStringArray &> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarStateArray &> &);
// template Pipe &operator<<(Pipe &,DataElement<DevVarEncodedArray &> &);

template Pipe &operator<<(Pipe &, DevVarBooleanArray *);
template Pipe &operator<<(Pipe &, DevVarShortArray *);
template Pipe &operator<<(Pipe &, DevVarLongArray *);
template Pipe &operator<<(Pipe &, DevVarLong64Array *);
template Pipe &operator<<(Pipe &, DevVarFloatArray *);
template Pipe &operator<<(Pipe &, DevVarDoubleArray *);
template Pipe &operator<<(Pipe &, DevVarUCharArray *);
template Pipe &operator<<(Pipe &, DevVarUShortArray *);
template Pipe &operator<<(Pipe &, DevVarULongArray *);
template Pipe &operator<<(Pipe &, DevVarULong64Array *);
template Pipe &operator<<(Pipe &, DevVarStringArray *);
template Pipe &operator<<(Pipe &, DevVarStateArray *);
// template Pipe &operator<<(Pipe &,DevVarEncodedArray *);

template Pipe &operator<<(Pipe &, DataElement<DevVarBooleanArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarShortArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarLongArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarLong64Array *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarFloatArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarDoubleArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarUCharArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarUShortArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarULongArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarULong64Array *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarStringArray *> &);
template Pipe &operator<<(Pipe &, DataElement<DevVarStateArray *> &);
// template Pipe &operator<<(Pipe &,DataElement<DevVarEncodedArray *> &);

//+----------------------------------------------------------------------------
//
// Instantiate DataElement helper functions
//
//-----------------------------------------------------------------------------

template std::ostream &operator<<(std::ostream &, const DataElement<DevBoolean> &);
template std::ostream &operator<<(std::ostream &, const DataElement<short> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevLong> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevLong64> &);
template std::ostream &operator<<(std::ostream &, const DataElement<float> &);
template std::ostream &operator<<(std::ostream &, const DataElement<double> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevUChar> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevUShort> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevULong> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevULong64> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevState> &);
// template std::ostream &operator<<(std::ostream &,DataElement<DevEncoded> &);
template std::ostream &operator<<(std::ostream &, const DataElement<const std::string> &);

template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevBoolean>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<short>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevLong>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevLong64>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<float>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<double>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevUChar>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevUShort>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevULong>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevULong64>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevString>> &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<DevState>> &);
// template std::ostream &operator<<(std::ostream &,DataElement<std::vector<DevEncoded> > &);
template std::ostream &operator<<(std::ostream &, const DataElement<std::vector<std::string>> &);

template std::ostream &operator<<(std::ostream &, const DataElement<DevVarBooleanArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarShortArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarLongArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarLong64Array *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarFloatArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarDoubleArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarUCharArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarUShortArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarULongArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarULong64Array *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarStringArray *> &);
template std::ostream &operator<<(std::ostream &, const DataElement<DevVarStateArray *> &);
// template std::ostream &operator<<(std::ostream &,DataElement<DevVarEncodedArray *> &);

} // namespace Tango
