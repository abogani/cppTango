//===================================================================================================================
//
// devapi_attr.cpp     - C++ source code file for TANGO devapi class DeviceAttribute
//
// programmer(s)     - Andy Gotz (goetz@esrf.fr)
//
// original         - February 2002
//
// Copyright (C) :      2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU
// Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>
//
//
//====================================================================================================================

#include <tango/common/tango_const.h>
#include <tango/server/seqvec.h>
#include <tango/server/tango_clock.h>
#include <tango/client/DeviceAttribute.h>
#include <tango/client/apiexcept.h>
#include <tango/internal/utils.h>

#include <iomanip>
#include <memory>

using namespace CORBA;

namespace Tango
{

//-----------------------------------------------------------------------------
//
// DeviceAttributeExt::assignment operator
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttributeExt &
    DeviceAttribute::DeviceAttributeExt::operator=(TANGO_UNUSED(const DeviceAttribute::DeviceAttributeExt &rval))
{
    ext_state = rval.ext_state;
    return *this;
}

void DeviceAttribute::DeviceAttributeExt::deep_copy(TANGO_UNUSED(const DeviceAttribute::DeviceAttributeExt &rval))
{
    ext_state = rval.ext_state;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - default constructor to create DeviceAttribute
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute() :
    ext(new DeviceAttributeExt)
{
    name = "Name not set";
    dim_x = 0;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    time.tv_sec = 0;
    time.tv_usec = 0;
    time.tv_nsec = 0;
    quality = Tango::ATTR_INVALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DATA_TYPE_UNKNOWN;
    d_state = {};
    d_state_filled = false;
    exceptions_flags.set(failed_flag);
    exceptions_flags.set(isempty_flag);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - copy constructor to create DeviceAttribute
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const DeviceAttribute &source) :
    ext(nullptr)
{
    name = source.name;
    exceptions_flags = source.exceptions_flags;
    dim_x = source.dim_x;
    dim_y = source.dim_y;
    w_dim_x = source.w_dim_x;
    w_dim_y = source.w_dim_y;
    quality = source.quality;
    data_format = source.data_format;
    data_type = source.data_type;
    time = source.time;
    err_list = source.err_list;

    LongSeq = source.LongSeq;
    ShortSeq = source.ShortSeq;
    DoubleSeq = source.DoubleSeq;
    StringSeq = source.StringSeq;
    FloatSeq = source.FloatSeq;
    BooleanSeq = source.BooleanSeq;
    UShortSeq = source.UShortSeq;
    UCharSeq = source.UCharSeq;
    Long64Seq = source.Long64Seq;
    ULongSeq = source.ULongSeq;
    ULong64Seq = source.ULong64Seq;
    StateSeq = source.StateSeq;
    EncodedSeq = source.EncodedSeq;

    d_state = source.d_state;
    d_state_filled = source.d_state_filled;

    if(source.ext != nullptr)
    {
        ext = std::make_unique<DeviceAttributeExt>();
        *(ext) = *(source.ext);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - move constructor to create DeviceAttribute
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(DeviceAttribute &&source) :
    ext(nullptr)
{
    name = std::move(source.name);
    exceptions_flags = source.exceptions_flags;
    dim_x = source.dim_x;
    dim_y = source.dim_y;
    w_dim_x = source.w_dim_x;
    w_dim_y = source.w_dim_y;
    quality = source.quality;
    data_format = source.data_format;
    data_type = source.data_type;
    time = source.time;
    err_list = source.err_list;

    if(source.LongSeq.operator->() != nullptr)
    {
        LongSeq = source.LongSeq._retn();
    }
    if(source.ShortSeq.operator->() != nullptr)
    {
        ShortSeq = source.ShortSeq._retn();
    }
    if(source.DoubleSeq.operator->() != nullptr)
    {
        DoubleSeq = source.DoubleSeq._retn();
    }
    if(source.StringSeq.operator->() != nullptr)
    {
        StringSeq = source.StringSeq._retn();
    }
    if(source.FloatSeq.operator->() != nullptr)
    {
        FloatSeq = source.FloatSeq._retn();
    }
    if(source.BooleanSeq.operator->() != nullptr)
    {
        BooleanSeq = source.BooleanSeq._retn();
    }
    if(source.UShortSeq.operator->() != nullptr)
    {
        UShortSeq = source.UShortSeq._retn();
    }
    if(source.UCharSeq.operator->() != nullptr)
    {
        UCharSeq = source.UCharSeq._retn();
    }
    if(source.Long64Seq.operator->() != nullptr)
    {
        Long64Seq = source.Long64Seq._retn();
    }
    if(source.ULongSeq.operator->() != nullptr)
    {
        ULongSeq = source.ULongSeq._retn();
    }
    if(source.ULong64Seq.operator->() != nullptr)
    {
        ULong64Seq = source.ULong64Seq._retn();
    }
    if(source.StateSeq.operator->() != nullptr)
    {
        StateSeq = source.StateSeq._retn();
    }
    if(source.EncodedSeq.operator->() != nullptr)
    {
        EncodedSeq = source.EncodedSeq._retn();
    }

    d_state = source.d_state;
    d_state_filled = source.d_state_filled;

    if(source.ext != nullptr)
    {
        ext = std::move(source.ext);
    }
}

void DeviceAttribute::deep_copy(const DeviceAttribute &source)
{
    name = source.name;
    exceptions_flags = source.exceptions_flags;
    dim_x = source.dim_x;
    dim_y = source.dim_y;
    w_dim_x = source.w_dim_x;
    w_dim_y = source.w_dim_y;
    quality = source.quality;
    data_format = source.data_format;
    data_type = source.data_type;
    time = source.time;
    err_list = source.err_list;

    LongSeq = source.LongSeq;
    ShortSeq = source.ShortSeq;
    DoubleSeq = source.DoubleSeq;
    StringSeq = source.StringSeq;
    FloatSeq = source.FloatSeq;
    BooleanSeq = source.BooleanSeq;
    UShortSeq = source.UShortSeq;
    UCharSeq = source.UCharSeq;
    Long64Seq = source.Long64Seq;
    ULongSeq = source.ULongSeq;
    ULong64Seq = source.ULong64Seq;
    StateSeq = source.StateSeq;
    EncodedSeq = source.EncodedSeq;

    d_state = source.d_state;
    d_state_filled = source.d_state_filled;

    if(source.ext != nullptr)
    {
        ext = std::make_unique<DeviceAttributeExt>();
        ext->deep_copy(*(source.ext));
    }
    else
    {
        ext.reset();
    }
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::get_x_dimension - Get attribute data transfer dimension
//
//-----------------------------------------------------------------------------

AttributeDimension DeviceAttribute::get_r_dimension()
{
    AttributeDimension d;
    d.dim_x = dim_x;
    d.dim_y = dim_y;

    return d;
}

long DeviceAttribute::get_nb_read()
{
    if(dim_y == 0)
    {
        return dim_x;
    }
    else
    {
        return dim_x * dim_y;
    }
}

AttributeDimension DeviceAttribute::get_w_dimension()
{
    AttributeDimension d;
    d.dim_x = w_dim_x;
    d.dim_y = w_dim_y;

    return d;
}

long DeviceAttribute::get_nb_written()
{
    if(w_dim_y == 0)
    {
        return w_dim_x;
    }
    else
    {
        return w_dim_x * w_dim_y;
    }
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator=() - assignement operator
//
//-----------------------------------------------------------------------------

DeviceAttribute &DeviceAttribute::operator=(const DeviceAttribute &rval)
{
    if(this != &rval)
    {
        name = rval.name;
        exceptions_flags = rval.exceptions_flags;
        dim_x = rval.dim_x;
        dim_y = rval.dim_y;
        w_dim_x = rval.w_dim_x;
        w_dim_y = rval.w_dim_y;
        quality = rval.quality;
        data_format = rval.data_format;
        data_type = rval.data_type;
        time = rval.time;
        err_list = rval.err_list;

        LongSeq = rval.LongSeq;
        ShortSeq = rval.ShortSeq;
        DoubleSeq = rval.DoubleSeq;
        StringSeq = rval.StringSeq;
        FloatSeq = rval.FloatSeq;
        BooleanSeq = rval.BooleanSeq;
        UShortSeq = rval.UShortSeq;
        UCharSeq = rval.UCharSeq;
        Long64Seq = rval.Long64Seq;
        ULongSeq = rval.ULongSeq;
        ULong64Seq = rval.ULong64Seq;
        StateSeq = rval.StateSeq;
        EncodedSeq = rval.EncodedSeq;

        d_state = rval.d_state;
        d_state_filled = rval.d_state_filled;

        if(rval.ext != nullptr)
        {
            ext = std::make_unique<DeviceAttributeExt>();
            *(ext) = *(rval.ext);
        }
        else
        {
            ext.reset();
        }
    }

    return *this;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator=() - move assignement operator
//
//-----------------------------------------------------------------------------

DeviceAttribute &DeviceAttribute::operator=(DeviceAttribute &&rval)
{
    name = std::move(rval.name);
    exceptions_flags = rval.exceptions_flags;
    dim_x = rval.dim_x;
    dim_y = rval.dim_y;
    w_dim_x = rval.w_dim_x;
    w_dim_y = rval.w_dim_y;
    quality = rval.quality;
    data_format = rval.data_format;
    data_type = rval.data_type;
    time = rval.time;
    err_list = rval.err_list;

    if(rval.LongSeq.operator->() != nullptr)
    {
        LongSeq = rval.LongSeq._retn();
    }
    else
    {
        LongSeq = nullptr;
    }

    if(rval.ShortSeq.operator->() != nullptr)
    {
        ShortSeq = rval.ShortSeq._retn();
    }
    else
    {
        ShortSeq = nullptr;
    }

    if(rval.DoubleSeq.operator->() != nullptr)
    {
        DoubleSeq = rval.DoubleSeq._retn();
    }
    else
    {
        DoubleSeq = nullptr;
    }

    if(rval.StringSeq.operator->() != nullptr)
    {
        StringSeq = rval.StringSeq._retn();
    }
    else
    {
        StringSeq = nullptr;
    }

    if(rval.FloatSeq.operator->() != nullptr)
    {
        FloatSeq = rval.FloatSeq._retn();
    }
    else
    {
        FloatSeq = nullptr;
    }

    if(rval.BooleanSeq.operator->() != nullptr)
    {
        BooleanSeq = rval.BooleanSeq._retn();
    }
    else
    {
        BooleanSeq = nullptr;
    }

    if(rval.UShortSeq.operator->() != nullptr)
    {
        UShortSeq = rval.UShortSeq._retn();
    }
    else
    {
        UShortSeq = nullptr;
    }

    if(rval.UCharSeq.operator->() != nullptr)
    {
        UCharSeq = rval.UCharSeq._retn();
    }
    else
    {
        UCharSeq = nullptr;
    }

    if(rval.Long64Seq.operator->() != nullptr)
    {
        Long64Seq = rval.Long64Seq._retn();
    }
    else
    {
        Long64Seq = nullptr;
    }

    if(rval.ULongSeq.operator->() != nullptr)
    {
        ULongSeq = rval.ULongSeq._retn();
    }
    else
    {
        ULongSeq = nullptr;
    }

    if(rval.ULong64Seq.operator->() != nullptr)
    {
        ULong64Seq = rval.ULong64Seq._retn();
    }
    else
    {
        ULong64Seq = nullptr;
    }

    if(rval.StateSeq.operator->() != nullptr)
    {
        StateSeq = rval.StateSeq._retn();
    }
    else
    {
        StateSeq = nullptr;
    }

    if(rval.EncodedSeq.operator->() != nullptr)
    {
        EncodedSeq = rval.EncodedSeq._retn();
    }
    else
    {
        EncodedSeq = nullptr;
    }

    d_state = rval.d_state;
    d_state_filled = rval.d_state_filled;

    if(rval.ext != nullptr)
    {
        ext = std::move(rval.ext);
    }
    else
    {
        ext.reset();
    }

    return *this;
}

void DeviceAttribute::init_common_class_members(const char *new_name, int x_dim = 1, int y_dim = 0)
{
    name = new_name;
    dim_x = x_dim;
    dim_y = y_dim;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    d_state = {};
    d_state_filled = false;
    exceptions_flags.set(failed_flag);
    exceptions_flags.set(isempty_flag);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for short
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, short datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    ShortSeq = new(DevVarShortArray);
    ShortSeq->length(1);
    ShortSeq[0] = datum;
    data_type = DEV_SHORT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, short datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    ShortSeq = new(DevVarShortArray);
    ShortSeq->length(1);
    ShortSeq[0] = datum;
    data_type = DEV_SHORT;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for DevLong
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, DevLong datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    LongSeq = new(DevVarLongArray);
    LongSeq->length(1);
    LongSeq[0] = datum;
    data_type = DEV_LONG;
}

DeviceAttribute::DeviceAttribute(const char *new_name, DevLong datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    LongSeq = new(DevVarLongArray);
    LongSeq->length(1);
    LongSeq[0] = datum;
    data_type = DEV_LONG;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for DevLong64
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, DevLong64 datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    Long64Seq = new(DevVarLong64Array);
    Long64Seq->length(1);
    Long64Seq[0] = datum;
    data_type = DEV_LONG64;
}

DeviceAttribute::DeviceAttribute(const char *new_name, DevLong64 datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    Long64Seq = new(DevVarLong64Array);
    Long64Seq->length(1);
    Long64Seq[0] = datum;
    data_type = DEV_LONG64;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for double
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, double datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    DoubleSeq = new(DevVarDoubleArray);
    DoubleSeq->length(1);
    DoubleSeq[0] = datum;
    data_type = DEV_DOUBLE;
}

DeviceAttribute::DeviceAttribute(const char *new_name, double datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    DoubleSeq = new(DevVarDoubleArray);
    DoubleSeq->length(1);
    DoubleSeq[0] = datum;
    data_type = DEV_DOUBLE;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for string
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::string &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    StringSeq = new(DevVarStringArray);
    StringSeq->length(1);
    StringSeq[0] = string_dup(datum.c_str());
    data_type = DEV_STRING;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::string &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    StringSeq = new(DevVarStringArray);
    StringSeq->length(1);
    StringSeq[0] = string_dup(datum.c_str());
    data_type = DEV_STRING;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const char *datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    StringSeq = new(DevVarStringArray);
    StringSeq->length(1);
    StringSeq[0] = string_dup(datum);
    data_type = DEV_STRING;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const char *datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    StringSeq = new(DevVarStringArray);
    StringSeq->length(1);
    StringSeq[0] = string_dup(datum);
    data_type = DEV_STRING;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for float
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, float datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    FloatSeq = new(DevVarFloatArray);
    FloatSeq->length(1);
    FloatSeq[0] = datum;
    data_type = DEV_FLOAT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, float datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    FloatSeq = new(DevVarFloatArray);
    FloatSeq->length(1);
    FloatSeq[0] = datum;
    data_type = DEV_FLOAT;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for boolean
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, bool datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    BooleanSeq = new(DevVarBooleanArray);
    BooleanSeq->length(1);
    BooleanSeq[0] = datum;
    data_type = DEV_BOOLEAN;
}

DeviceAttribute::DeviceAttribute(const char *new_name, bool datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    BooleanSeq = new(DevVarBooleanArray);
    BooleanSeq->length(1);
    BooleanSeq[0] = datum;
    data_type = DEV_BOOLEAN;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for unsigned short (DevUShort)
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, unsigned short datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    UShortSeq = new(DevVarUShortArray);
    UShortSeq->length(1);
    UShortSeq[0] = datum;
    data_type = DEV_USHORT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, unsigned short datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    UShortSeq = new(DevVarUShortArray);
    UShortSeq->length(1);
    UShortSeq[0] = datum;
    data_type = DEV_USHORT;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for unsigned char (DevUChar)
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, unsigned char datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    UCharSeq = new(DevVarCharArray);
    UCharSeq->length(1);
    UCharSeq[0] = datum;
    data_type = DEV_UCHAR;
}

DeviceAttribute::DeviceAttribute(const char *new_name, unsigned char datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    UCharSeq = new(DevVarCharArray);
    UCharSeq->length(1);
    UCharSeq[0] = datum;
    data_type = DEV_UCHAR;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for DevULong
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, DevULong datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    ULongSeq = new(DevVarULongArray);
    ULongSeq->length(1);
    ULongSeq[0] = datum;
    data_type = DEV_ULONG;
}

DeviceAttribute::DeviceAttribute(const char *new_name, DevULong datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    ULongSeq = new(DevVarULongArray);
    ULongSeq->length(1);
    ULongSeq[0] = datum;
    data_type = DEV_ULONG;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for DevULong64
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, DevULong64 datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    ULong64Seq = new(DevVarULong64Array);
    ULong64Seq->length(1);
    ULong64Seq[0] = datum;
    data_type = DEV_ULONG64;
}

DeviceAttribute::DeviceAttribute(const char *new_name, DevULong64 datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    ULong64Seq = new(DevVarULong64Array);
    ULong64Seq->length(1);
    ULong64Seq[0] = datum;
    data_type = DEV_ULONG64;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for DevState
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, DevState datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    StateSeq = new(DevVarStateArray);
    StateSeq->length(1);
    StateSeq[0] = datum;
    data_type = DEV_STATE;
}

DeviceAttribute::DeviceAttribute(const char *new_name, DevState datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    StateSeq = new(DevVarStateArray);
    StateSeq->length(1);
    StateSeq[0] = datum;
    data_type = DEV_STATE;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for DevEncoded
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const DevEncoded &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str());
    EncodedSeq = new(DevVarEncodedArray);
    EncodedSeq->length(1);
    EncodedSeq[0] = datum;
    data_type = DEV_ENCODED;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const DevEncoded &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name);
    EncodedSeq = new(DevVarEncodedArray);
    EncodedSeq->length(1);
    EncodedSeq[0] = datum;
    data_type = DEV_ENCODED;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of short
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<short> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    ShortSeq = new(DevVarShortArray);
    ShortSeq.inout() << datum;
    data_type = DEV_SHORT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<short> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    ShortSeq = new(DevVarShortArray);
    ShortSeq.inout() << datum;
    data_type = DEV_SHORT;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<short> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    ShortSeq = new(DevVarShortArray);
    ShortSeq.inout() << datum;
    data_type = DEV_SHORT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<short> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    ShortSeq = new(DevVarShortArray);
    ShortSeq.inout() << datum;
    data_type = DEV_SHORT;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of DevLong
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevLong> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    LongSeq = new(DevVarLongArray);
    LongSeq.inout() << datum;
    data_type = DEV_LONG;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevLong> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    LongSeq = new(DevVarLongArray);
    LongSeq.inout() << datum;
    data_type = DEV_LONG;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevLong> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    LongSeq = new(DevVarLongArray);
    LongSeq.inout() << datum;
    data_type = DEV_LONG;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevLong> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    LongSeq = new(DevVarLongArray);
    LongSeq.inout() << datum;
    data_type = DEV_LONG;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of DevLong64
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevLong64> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    Long64Seq = new(DevVarLong64Array);
    Long64Seq.inout() << datum;
    data_type = DEV_LONG64;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevLong64> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    Long64Seq = new(DevVarLong64Array);
    Long64Seq.inout() << datum;
    data_type = DEV_LONG64;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevLong64> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    Long64Seq = new(DevVarLong64Array);
    Long64Seq.inout() << datum;
    data_type = DEV_LONG64;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevLong64> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    Long64Seq = new(DevVarLong64Array);
    Long64Seq.inout() << datum;
    data_type = DEV_LONG64;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of double
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<double> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    DoubleSeq = new(DevVarDoubleArray);
    DoubleSeq.inout() << datum;
    data_type = DEV_DOUBLE;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<double> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    DoubleSeq = new(DevVarDoubleArray);
    DoubleSeq.inout() << datum;
    data_type = DEV_DOUBLE;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<double> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    DoubleSeq = new(DevVarDoubleArray);
    DoubleSeq.inout() << datum;
    data_type = DEV_DOUBLE;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<double> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    DoubleSeq = new(DevVarDoubleArray);
    DoubleSeq.inout() << datum;
    data_type = DEV_DOUBLE;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of string
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<std::string> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    StringSeq = new(DevVarStringArray);
    StringSeq.inout() << datum;
    data_type = DEV_STRING;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<std::string> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    StringSeq = new(DevVarStringArray);
    StringSeq.inout() << datum;
    data_type = DEV_STRING;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<std::string> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    StringSeq = new(DevVarStringArray);
    StringSeq.inout() << datum;
    data_type = DEV_STRING;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<std::string> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    StringSeq = new(DevVarStringArray);
    StringSeq.inout() << datum;
    data_type = DEV_STRING;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of float
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<float> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    FloatSeq = new(DevVarFloatArray);
    FloatSeq.inout() << datum;
    data_type = DEV_FLOAT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<float> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    FloatSeq = new(DevVarFloatArray);
    FloatSeq.inout() << datum;
    data_type = DEV_FLOAT;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<float> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    FloatSeq = new(DevVarFloatArray);
    FloatSeq.inout() << datum;
    data_type = DEV_FLOAT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<float> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    FloatSeq = new(DevVarFloatArray);
    FloatSeq.inout() << datum;
    data_type = DEV_FLOAT;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of bool
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<bool> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    BooleanSeq = new(DevVarBooleanArray);
    BooleanSeq.inout() << datum;
    data_type = DEV_BOOLEAN;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<bool> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    BooleanSeq = new(DevVarBooleanArray);
    BooleanSeq.inout() << datum;
    data_type = DEV_BOOLEAN;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<bool> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    BooleanSeq = new(DevVarBooleanArray);
    BooleanSeq.inout() << datum;
    data_type = DEV_BOOLEAN;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<bool> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    BooleanSeq = new(DevVarBooleanArray);
    BooleanSeq.inout() << datum;
    data_type = DEV_BOOLEAN;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of unsigned short
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<unsigned short> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    UShortSeq = new(DevVarUShortArray);
    UShortSeq.inout() << datum;
    data_type = DEV_USHORT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<unsigned short> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    UShortSeq = new(DevVarUShortArray);
    UShortSeq.inout() << datum;
    data_type = DEV_USHORT;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<unsigned short> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    UShortSeq = new(DevVarUShortArray);
    UShortSeq.inout() << datum;
    data_type = DEV_USHORT;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<unsigned short> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    UShortSeq = new(DevVarUShortArray);
    UShortSeq.inout() << datum;
    data_type = DEV_USHORT;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of unsigned char
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<unsigned char> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    UCharSeq = new(DevVarCharArray);
    UCharSeq.inout() << datum;
    data_type = DEV_UCHAR;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<unsigned char> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    UCharSeq = new(DevVarCharArray);
    UCharSeq.inout() << datum;
    data_type = DEV_UCHAR;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<unsigned char> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    UCharSeq = new(DevVarCharArray);
    UCharSeq.inout() << datum;
    data_type = DEV_UCHAR;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<unsigned char> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    UCharSeq = new(DevVarCharArray);
    UCharSeq.inout() << datum;
    data_type = DEV_UCHAR;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of DevULong
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevULong> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    ULongSeq = new(DevVarULongArray);
    ULongSeq.inout() << datum;
    data_type = DEV_ULONG;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevULong> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    ULongSeq = new(DevVarULongArray);
    ULongSeq.inout() << datum;
    data_type = DEV_ULONG;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevULong> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    ULongSeq = new(DevVarULongArray);
    ULongSeq.inout() << datum;
    data_type = DEV_ULONG;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevULong> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    ULongSeq = new(DevVarULongArray);
    ULongSeq.inout() << datum;
    data_type = DEV_ULONG;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of DevULong64
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevULong64> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    ULong64Seq = new(DevVarULong64Array);
    ULong64Seq.inout() << datum;
    data_type = DEV_ULONG64;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevULong64> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    ULong64Seq = new(DevVarULong64Array);
    ULong64Seq.inout() << datum;
    data_type = DEV_ULONG64;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevULong64> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    ULong64Seq = new(DevVarULong64Array);
    ULong64Seq.inout() << datum;
    data_type = DEV_ULONG64;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevULong64> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    ULong64Seq = new(DevVarULong64Array);
    ULong64Seq.inout() << datum;
    data_type = DEV_ULONG64;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::DeviceAttribute() - constructor for vector of DevState
//
//-----------------------------------------------------------------------------

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevState> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), datum.size());
    StateSeq = new(DevVarStateArray);
    StateSeq.inout() << datum;
    data_type = DEV_STATE;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevState> &datum) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, datum.size());
    StateSeq = new(DevVarStateArray);
    StateSeq.inout() << datum;
    data_type = DEV_STATE;
}

DeviceAttribute::DeviceAttribute(const std::string &new_name, const std::vector<DevState> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name.c_str(), x, y);
    StateSeq = new(DevVarStateArray);
    StateSeq.inout() << datum;
    data_type = DEV_STATE;
}

DeviceAttribute::DeviceAttribute(const char *new_name, const std::vector<DevState> &datum, int x, int y) :
    ext(new DeviceAttributeExt)
{
    init_common_class_members(new_name, x, y);
    StateSeq = new(DevVarStateArray);
    StateSeq.inout() << datum;
    data_type = DEV_STATE;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::~DeviceAttribute() - destructor to destroy DeviceAttribute
//
//-----------------------------------------------------------------------------

DeviceAttribute::~DeviceAttribute() { }

//-----------------------------------------------------------------------------
//
// DeviceAttribute::is_empty() - returns true or false if datum is empty
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::is_empty() const
{
    ext->ext_state.reset(isempty_flag);

    if(LongSeq.operator->() != nullptr)
    {
        if(LongSeq->length() != 0)
        {
            return false;
        }
    }

    if(ShortSeq.operator->() != nullptr)
    {
        if(ShortSeq->length() != 0)
        {
            return false;
        }
    }

    if(DoubleSeq.operator->() != nullptr)
    {
        if(DoubleSeq->length() != 0)
        {
            return false;
        }
    }

    if(StringSeq.operator->() != nullptr)
    {
        if(StringSeq->length() != 0)
        {
            return false;
        }
    }

    if(FloatSeq.operator->() != nullptr)
    {
        if(FloatSeq->length() != 0)
        {
            return false;
        }
    }

    if(BooleanSeq.operator->() != nullptr)
    {
        if(BooleanSeq->length() != 0)
        {
            return false;
        }
    }

    if(UShortSeq.operator->() != nullptr)
    {
        if(UShortSeq->length() != 0)
        {
            return false;
        }
    }

    if(UCharSeq.operator->() != nullptr)
    {
        if(UCharSeq->length() != 0)
        {
            return false;
        }
    }

    if(Long64Seq.operator->() != nullptr)
    {
        if(Long64Seq->length() != 0)
        {
            return false;
        }
    }

    if(ULongSeq.operator->() != nullptr)
    {
        if(ULongSeq->length() != 0)
        {
            return false;
        }
    }

    if(ULong64Seq.operator->() != nullptr)
    {
        if(ULong64Seq->length() != 0)
        {
            return false;
        }
    }

    if(StateSeq.operator->() != nullptr)
    {
        if(StateSeq->length() != 0)
        {
            return false;
        }
    }

    if(EncodedSeq.operator->() != nullptr)
    {
        if(EncodedSeq->length() != 0)
        {
            return false;
        }
    }

    if(d_state_filled)
    {
        return false;
    }

    ext->ext_state.set(isempty_flag);

    if(exceptions_flags.test(isempty_flag))
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            ApiDataExcept, API_EmptyDeviceAttribute, "cannot extract, no data in DeviceAttribute object ");
    }
    return true;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::get_type() - returns attribute data type
//
//-----------------------------------------------------------------------------

int DeviceAttribute::get_type() const
{
    return data_type;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::get_data_format() - returns attribute data format
//
//-----------------------------------------------------------------------------

AttrDataFormat DeviceAttribute::get_data_format()
{
    if(exceptions_flags.test(unknown_format_flag) && (data_format == Tango::FMT_UNKNOWN))
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            ApiDataExcept,
            API_EmptyDeviceAttribute,
            "Cannot returned data_type from DeviceAttribute object: Not initialised yet or too old device (< V7)");
    }
    return data_format;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(short &) - extract a short from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(short &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ShortSeq.operator->() != nullptr)
    {
        if(ShortSeq->length() != 0)
        {
            datum = ShortSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(short) - insert a short into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(short datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    if(data_type != DEV_ENUM)
    {
        data_type = DEV_SHORT;
    }

    DevVarShortArray *short_vararr = new(DevVarShortArray);
    short_vararr->length(1);
    (*short_vararr)[0] = datum;
    ShortSeq = short_vararr;

    del_mem(Tango::DEV_SHORT);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevLong &) - extract a DevLong from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevLong &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(LongSeq.operator->() != nullptr)
    {
        if(LongSeq->length() != 0)
        {
            datum = LongSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevLong) - insert a DevLong into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevLong datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG;

    DevVarLongArray *long_vararr = new(DevVarLongArray);
    long_vararr->length(1);
    (*long_vararr)[0] = datum;
    LongSeq = long_vararr;

    del_mem(Tango::DEV_LONG);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevLong64 &) - extract a DevLong64 from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevLong64 &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(Long64Seq.operator->() != nullptr)
    {
        if(Long64Seq->length() != 0)
        {
            datum = Long64Seq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevLong64) - insert a DevLong64 into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevLong64 datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG64;

    DevVarLong64Array *long_vararr = new(DevVarLong64Array);
    long_vararr->length(1);
    (*long_vararr)[0] = datum;
    Long64Seq = long_vararr;

    del_mem(Tango::DEV_LONG64);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(double &) - extract a double from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(double &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(DoubleSeq.operator->() != nullptr)
    {
        if(DoubleSeq->length() != 0)
        {
            datum = DoubleSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(double) - insert a double into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(double datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_DOUBLE;

    DevVarDoubleArray *double_vararr = new(DevVarDoubleArray);
    double_vararr->length(1);
    (*double_vararr)[0] = datum;
    DoubleSeq = double_vararr;

    del_mem(Tango::DEV_DOUBLE);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::string &) - extract a string from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::string &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(StringSeq.operator->() != nullptr)
    {
        if(StringSeq->length() != 0)
        {
            datum = StringSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::string &) - insert a string into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::string &datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STRING;

    DevVarStringArray *string_vararr = new(DevVarStringArray);
    string_vararr->length(1);
    (*string_vararr)[0] = string_dup(datum.c_str());
    StringSeq = string_vararr;

    del_mem(Tango::DEV_STRING);
}

void DeviceAttribute::operator<<(DevString datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STRING;

    DevVarStringArray *string_vararr = new(DevVarStringArray);
    string_vararr->length(1);
    (*string_vararr)[0] = string_dup(datum);
    StringSeq = string_vararr;

    del_mem(Tango::DEV_STRING);
}

void DeviceAttribute::operator<<(const char *datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STRING;

    DevVarStringArray *string_vararr = new(DevVarStringArray);
    string_vararr->length(1);
    (*string_vararr)[0] = string_dup(datum);
    StringSeq = string_vararr;

    del_mem(Tango::DEV_STRING);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(float &) - extract a float from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(float &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(FloatSeq.operator->() != nullptr)
    {
        if(FloatSeq->length() != 0)
        {
            datum = FloatSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(float) - insert a float into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(float datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_FLOAT;

    DevVarFloatArray *float_vararr = new(DevVarFloatArray);
    float_vararr->length(1);
    (*float_vararr)[0] = datum;
    FloatSeq = float_vararr;

    del_mem(Tango::DEV_FLOAT);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(bool &) - extract a boolean from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(bool &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(BooleanSeq.operator->() != nullptr)
    {
        if(BooleanSeq->length() != 0)
        {
            datum = BooleanSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(bool) - insert a boolean into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(bool datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_BOOLEAN;

    DevVarBooleanArray *bool_vararr = new(DevVarBooleanArray);
    bool_vararr->length(1);
    (*bool_vararr)[0] = datum;
    BooleanSeq = bool_vararr;

    del_mem(Tango::DEV_BOOLEAN);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(unsigned short &) - extract a unsigned short from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(unsigned short &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UShortSeq.operator->() != nullptr)
    {
        if(UShortSeq->length() != 0)
        {
            datum = UShortSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(unsigned short) - insert a unsigned short into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(unsigned short datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_USHORT;

    DevVarUShortArray *ush_vararr = new(DevVarUShortArray);
    ush_vararr->length(1);
    (*ush_vararr)[0] = datum;
    UShortSeq = ush_vararr;

    del_mem(Tango::DEV_USHORT);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(unsigned char &) - extract a unsigned char from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(unsigned char &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UCharSeq.operator->() != nullptr)
    {
        if(UCharSeq->length() != 0)
        {
            datum = UCharSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(unsigned char) - insert a unsigned char into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(unsigned char datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_UCHAR;

    DevVarCharArray *uch_vararr = new(DevVarCharArray);
    uch_vararr->length(1);
    (*uch_vararr)[0] = datum;
    UCharSeq = uch_vararr;

    del_mem(Tango::DEV_UCHAR);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevULong &) - extract a DevULong from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevULong &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULongSeq.operator->() != nullptr)
    {
        if(ULongSeq->length() != 0)
        {
            datum = ULongSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevULong) - insert a DevULong into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevULong datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG;

    DevVarULongArray *long_vararr = new(DevVarULongArray);
    long_vararr->length(1);
    (*long_vararr)[0] = datum;
    ULongSeq = long_vararr;

    del_mem(Tango::DEV_ULONG);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevULong64 &) - extract a DevULong64 from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevULong64 &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULong64Seq.operator->() != nullptr)
    {
        if(ULong64Seq->length() != 0)
        {
            datum = ULong64Seq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevULong64) - insert a DevULong64 into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevULong64 datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG64;

    DevVarULong64Array *long_vararr = new(DevVarULong64Array);
    long_vararr->length(1);
    (*long_vararr)[0] = datum;
    ULong64Seq = long_vararr;

    del_mem(Tango::DEV_ULONG64);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevState &) - extract a DevState from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevState &datum)
{
    // check for available data

    bool ret = check_for_data();

    if(!ret)
    {
        return false;
    }

    if(d_state_filled)
    {
        datum = d_state;

        return ret;
    }

    if(StateSeq.operator->() != nullptr)
    {
        if(StateSeq->length() != 0)
        {
            datum = StateSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevState) - insert a DevState into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevState datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STATE;

    DevVarStateArray *state_vararr = new(DevVarStateArray);
    state_vararr->length(1);
    (*state_vararr)[0] = datum;
    StateSeq = state_vararr;

    del_mem(Tango::DEV_STATE);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevEncoded &) - extract a DevEncoded from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevEncoded &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(EncodedSeq.operator->() != nullptr)
    {
        if(EncodedSeq->length() != 0)
        {
            datum = EncodedSeq[0];
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevEncoded&) - insert a DevEncoded into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevEncoded &datum)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ENCODED;

    DevVarEncodedArray *enc_vararr = new(DevVarEncodedArray);
    enc_vararr->length(1);
    (*enc_vararr)[0] = datum;
    EncodedSeq = enc_vararr;

    del_mem(Tango::DEV_ENCODED);
}

void DeviceAttribute::insert(char *&str, unsigned char *&ptr, unsigned int size)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ENCODED;

    DevVarEncodedArray *enc_vararr = new(DevVarEncodedArray);
    enc_vararr->length(1);
    (*enc_vararr)[0].encoded_format = Tango::string_dup(str);
    (*enc_vararr)[0].encoded_data.replace(size, size, (CORBA::Octet *) ptr);
    EncodedSeq = enc_vararr;

    del_mem(Tango::DEV_ENCODED);
}

void DeviceAttribute::insert(const char *str, unsigned char *ptr, unsigned int size)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;

    DevVarEncodedArray *enc_vararr = new(DevVarEncodedArray);
    enc_vararr->length(1);
    (*enc_vararr)[0].encoded_format = Tango::string_dup(str);
    (*enc_vararr)[0].encoded_data.replace(size, size, (CORBA::Octet *) ptr);
    EncodedSeq = enc_vararr;

    del_mem(Tango::DEV_ENCODED);
}

void DeviceAttribute::insert(const std::string &str, std::vector<unsigned char> &array)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ENCODED;

    DevVarEncodedArray *enc_vararr = new(DevVarEncodedArray);
    enc_vararr->length(1);
    (*enc_vararr)[0].encoded_format = Tango::string_dup(str.c_str());
    (*enc_vararr)[0].encoded_data << array;
    EncodedSeq = enc_vararr;

    del_mem(Tango::DEV_ENCODED);
}

void DeviceAttribute::insert(std::string &str, std::vector<unsigned char> &array)
{
    const std::string &tmp_str = str;
    insert(tmp_str, array);
}

void DeviceAttribute::insert(const char *str, DevVarCharArray *array)
{
    dim_x = 1;
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ENCODED;

    DevVarEncodedArray *enc_vararr = new(DevVarEncodedArray);
    enc_vararr->length(1);
    (*enc_vararr)[0].encoded_format = Tango::string_dup(str);
    (*enc_vararr)[0].encoded_data.replace(array->length(), array->length(), array->get_buffer());
    EncodedSeq = enc_vararr;

    del_mem(Tango::DEV_ENCODED);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<std::string>) -
// insert a vector of string into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<std::string> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STRING;

    if(StringSeq.operator->() == nullptr)
    {
        DevVarStringArray *str_vararr = new(DevVarStringArray);
        StringSeq = str_vararr;
    }
    StringSeq.inout() << datum;

    del_mem(Tango::DEV_STRING);
}

void DeviceAttribute::insert(const std::vector<std::string> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<std::string> &) - extract a vector<string> from DeviceData
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<std::string> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(StringSeq.operator->() != nullptr)
    {
        if(StringSeq->length() != 0)
        {
            datum.resize(StringSeq->length());

            for(unsigned int i = 0; i < StringSeq->length(); i++)
            {
                datum[i] = StringSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<short>) -
// insert a vector of short into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<short> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;

    if(data_type != DEV_ENUM)
    {
        data_type = DEV_SHORT;
    }

    if(ShortSeq.operator->() == nullptr)
    {
        DevVarShortArray *short_vararr = new(DevVarShortArray);
        ShortSeq = short_vararr;
    }
    ShortSeq.inout() << datum;

    del_mem(Tango::DEV_SHORT);
}

void DeviceAttribute::insert(std::vector<short> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<short> &) - extract a vector<short> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<short> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ShortSeq.operator->() != nullptr)
    {
        if(ShortSeq->length() != 0)
        {
            datum.resize(ShortSeq->length());

            for(unsigned int i = 0; i < ShortSeq->length(); i++)
            {
                datum[i] = ShortSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<DevLong>) -
// insert a vector of DevLong into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<DevLong> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG;

    if(LongSeq.operator->() == nullptr)
    {
        DevVarLongArray *long_vararr = new(DevVarLongArray);
        LongSeq = long_vararr;
    }
    LongSeq.inout() << datum;

    del_mem(Tango::DEV_LONG);
}

void DeviceAttribute::insert(const std::vector<DevLong> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<DevLong> &) - extract a vector<DevLong> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<DevLong> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(LongSeq.operator->() != nullptr)
    {
        if(LongSeq->length() != 0)
        {
            datum.resize(LongSeq->length());

            for(unsigned int i = 0; i < LongSeq->length(); i++)
            {
                datum[i] = LongSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<DevLong64>) -
// insert a vector of DevLong64 into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<DevLong64> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG64;

    if(Long64Seq.operator->() == nullptr)
    {
        DevVarLong64Array *long_vararr = new(DevVarLong64Array);
        Long64Seq = long_vararr;
    }
    Long64Seq.inout() << datum;

    del_mem(Tango::DEV_LONG64);
}

void DeviceAttribute::insert(const std::vector<DevLong64> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<DevLong64> &) - extract a vector<DevLong64> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<DevLong64> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(Long64Seq.operator->() != nullptr)
    {
        if(Long64Seq->length() != 0)
        {
            datum.resize(Long64Seq->length());

            for(unsigned int i = 0; i < Long64Seq->length(); i++)
            {
                datum[i] = Long64Seq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<double>) -
// insert a vector of double into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<double> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_DOUBLE;

    if(DoubleSeq.operator->() == nullptr)
    {
        DevVarDoubleArray *double_vararr = new(DevVarDoubleArray);
        DoubleSeq = double_vararr;
    }
    DoubleSeq.inout() << datum;

    del_mem(Tango::DEV_DOUBLE);
}

void DeviceAttribute::insert(const std::vector<double> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<double> &) - extract a vector<double> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<double> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(DoubleSeq.operator->() != nullptr)
    {
        if(DoubleSeq->length() != 0)
        {
            datum.resize(DoubleSeq->length());

            for(unsigned int i = 0; i < DoubleSeq->length(); i++)
            {
                datum[i] = DoubleSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<float>) -
// insert a vector of float into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<float> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_FLOAT;

    if(FloatSeq.operator->() == nullptr)
    {
        DevVarFloatArray *float_vararr = new(DevVarFloatArray);
        FloatSeq = float_vararr;
    }
    FloatSeq.inout() << datum;

    del_mem(Tango::DEV_FLOAT);
}

void DeviceAttribute::insert(const std::vector<float> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<float> &) - extract a vector<float> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<float> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(FloatSeq.operator->() != nullptr)
    {
        if(FloatSeq->length() != 0)
        {
            datum.resize(FloatSeq->length());

            for(unsigned int i = 0; i < FloatSeq->length(); i++)
            {
                datum[i] = FloatSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<bool>) -
// insert a vector of boolean into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<bool> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_BOOLEAN;

    if(BooleanSeq.operator->() == nullptr)
    {
        DevVarBooleanArray *bool_vararr = new(DevVarBooleanArray);
        BooleanSeq = bool_vararr;
    }
    BooleanSeq.inout() << datum;

    del_mem(Tango::DEV_BOOLEAN);
}

void DeviceAttribute::insert(const std::vector<bool> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(vector<bool> &) - extract a vector<bool> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<bool> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(BooleanSeq.operator->() != nullptr)
    {
        if(BooleanSeq->length() != 0)
        {
            datum.resize(BooleanSeq->length());

            for(unsigned int i = 0; i < BooleanSeq->length(); i++)
            {
                datum[i] = BooleanSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<unsigned short>) -
// insert a vector of unsigned short into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<unsigned short> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_USHORT;

    if(UShortSeq.operator->() == nullptr)
    {
        DevVarUShortArray *ushort_vararr = new(DevVarUShortArray);
        UShortSeq = ushort_vararr;
    }
    UShortSeq.inout() << datum;

    del_mem(Tango::DEV_USHORT);
}

void DeviceAttribute::insert(const std::vector<unsigned short> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<unsigned short> &) - extract a vector<unsigned short> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<unsigned short> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UShortSeq.operator->() != nullptr)
    {
        if(UShortSeq->length() != 0)
        {
            datum.resize(UShortSeq->length());

            for(unsigned int i = 0; i < UShortSeq->length(); i++)
            {
                datum[i] = UShortSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<unsigned char>) -
// insert a vector of unsigned char into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<unsigned char> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_UCHAR;

    if(UCharSeq.operator->() == nullptr)
    {
        DevVarUCharArray *uchar_vararr = new(DevVarUCharArray);
        UCharSeq = uchar_vararr;
    }
    UCharSeq.inout() << datum;

    del_mem(Tango::DEV_UCHAR);
}

void DeviceAttribute::insert(const std::vector<unsigned char> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<unsigned char> &) - extract a vector<unsigne char> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<unsigned char> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UCharSeq.operator->() != nullptr)
    {
        if(UCharSeq->length() != 0)
        {
            datum.resize(UCharSeq->length());

            for(unsigned int i = 0; i < UCharSeq->length(); i++)
            {
                datum[i] = UCharSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<DevULong>) -
// insert a vector of DevULong into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<DevULong> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG;

    if(ULongSeq.operator->() == nullptr)
    {
        DevVarULongArray *long_vararr = new(DevVarULongArray);
        ULongSeq = long_vararr;
    }
    ULongSeq.inout() << datum;

    del_mem(Tango::DEV_ULONG);
}

void DeviceAttribute::insert(const std::vector<DevULong> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<DevULong> &) - extract a vector<DevULong> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<DevULong> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULongSeq.operator->() != nullptr)
    {
        if(ULongSeq->length() != 0)
        {
            datum.resize(ULongSeq->length());

            for(unsigned int i = 0; i < ULongSeq->length(); i++)
            {
                datum[i] = ULongSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<DevULong64>) -
// insert a vector of DevULong64 into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<DevULong64> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG64;

    if(ULong64Seq.operator->() == nullptr)
    {
        DevVarULong64Array *long_vararr = new(DevVarULong64Array);
        ULong64Seq = long_vararr;
    }
    ULong64Seq.inout() << datum;

    del_mem(Tango::DEV_ULONG64);
}

void DeviceAttribute::insert(const std::vector<DevULong64> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<DevULong64> &) - extract a vector<DevULong64> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<DevULong64> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULong64Seq.operator->() != nullptr)
    {
        if(ULong64Seq->length() != 0)
        {
            datum.resize(ULong64Seq->length());

            for(unsigned int i = 0; i < ULong64Seq->length(); i++)
            {
                datum[i] = ULong64Seq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const std::vector<DevState>) -
// insert a vector of DevState into DeviceAttribute
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const std::vector<DevState> &datum)
{
    dim_x = datum.size();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STATE;

    if(StateSeq.operator->() == nullptr)
    {
        DevVarStateArray *long_vararr = new(DevVarStateArray);
        StateSeq = long_vararr;
    }
    StateSeq.inout() << datum;

    del_mem(Tango::DEV_STATE);
}

void DeviceAttribute::insert(const std::vector<DevState> &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(std::vector<DevState> &) - extract a vector<DevState> from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(std::vector<DevState> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(d_state_filled)
    {
        datum.resize(1);
        datum[0] = d_state;
        return true;
    }

    if(StateSeq.operator->() != nullptr)
    {
        if(StateSeq->length() != 0)
        {
            datum.resize(StateSeq->length());

            for(unsigned int i = 0; i < StateSeq->length(); i++)
            {
                datum[i] = StateSeq[i];
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarShortArray *) - extract a sequence of
// short from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarShortArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ShortSeq.operator->() != nullptr)
    {
        if(ShortSeq->length() != 0)
        {
            datum = ShortSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarLongArray *) - extract a sequence of
// long from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarLongArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(LongSeq.operator->() != nullptr)
    {
        if(LongSeq->length() != 0)
        {
            datum = LongSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarDoubleArray *) - extract a sequence of
// double from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarDoubleArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(DoubleSeq.operator->() != nullptr)
    {
        if(DoubleSeq->length() != 0)
        {
            datum = DoubleSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarStringArray *) - extract a sequence of
// string from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarStringArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(StringSeq.operator->() != nullptr)
    {
        if(StringSeq->length() != 0)
        {
            datum = StringSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarFloatArray *) - extract a sequence of
// double from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarFloatArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(FloatSeq.operator->() != nullptr)
    {
        if(FloatSeq->length() != 0)
        {
            datum = FloatSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarBooleanArray *) - extract a sequence of
// bool from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarBooleanArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(BooleanSeq.operator->() != nullptr)
    {
        if(BooleanSeq->length() != 0)
        {
            datum = BooleanSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarUShortArray *) - extract a sequence of
// unsigned short from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarUShortArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UShortSeq.operator->() != nullptr)
    {
        if(UShortSeq->length() != 0)
        {
            datum = UShortSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarCharArray *) - extract a sequence of
// unsigned char from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarCharArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UCharSeq.operator->() != nullptr)
    {
        if(UCharSeq->length() != 0)
        {
            datum = UCharSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarLong64Array *) - extract a sequence of
// DevLong64 from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarLong64Array *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(Long64Seq.operator->() != nullptr)
    {
        if(Long64Seq->length() != 0)
        {
            datum = Long64Seq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarULongArray *) - extract a sequence of
// DevULong from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarULongArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULongSeq.operator->() != nullptr)
    {
        if(ULongSeq->length() != 0)
        {
            datum = ULongSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarULong64Array *) - extract a sequence of
// DevUULong64 from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarULong64Array *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULong64Seq.operator->() != nullptr)
    {
        if(ULong64Seq->length() != 0)
        {
            datum = ULong64Seq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarStateArray *) - extract a sequence of
// DevState from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarStateArray *&datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(d_state_filled)
    {
        Tango::DevState *tmp_ptr = new Tango::DevState[1];
        *tmp_ptr = d_state;
        datum = new DevVarStateArray(1, 1, tmp_ptr, true);
        return true;
    }

    if(StateSeq.operator->() != nullptr)
    {
        if(StateSeq->length() != 0)
        {
            datum = StateSeq._retn();
        }
        else
        {
            return false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator >>(DevVarEncodedArray *) - extract a sequence of
// long from DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::operator>>(DevVarEncodedArray *&datum)
{
    bool ret = true;

    if(err_list.operator->() != nullptr)
    {
        if(err_list.in().length() != 0)
        {
            if(exceptions_flags.test(failed_flag))
            {
                throw DevFailed(err_list.in());
            }
            else
            {
                return false;
            }
        }
    }

    if(is_empty())
    {
        return false;
    }

    if(EncodedSeq.operator->() != nullptr)
    {
        if(EncodedSeq->length() != 0)
        {
            datum = EncodedSeq._retn();
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        ret = false;

        if(exceptions_flags.test(wrongtype_flag))
        {
            TANGO_THROW_DETAILED_EXCEPTION(
                ApiDataExcept,
                API_IncompatibleAttrArgumentType,
                "Cannot extract, data in DeviceAttribute object is not an array of DevEncoded");
        }
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarShortArray &) -
// insert a DevVarShortArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarShortArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_SHORT;

    ShortSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        ShortSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_SHORT);
}

void DeviceAttribute::insert(const DevVarShortArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarShortArray *) -
// insert a DevVarShortArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarShortArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_SHORT;

    ShortSeq = datum;

    del_mem(Tango::DEV_SHORT);
}

void DeviceAttribute::insert(DevVarShortArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarLongArray &) -
// insert a DevVarLongArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarLongArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG;

    LongSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        LongSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_LONG);
}

void DeviceAttribute::insert(const DevVarLongArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarLongArray *) -
// insert a DevVarLongArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarLongArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG;

    LongSeq = datum;

    del_mem(Tango::DEV_LONG);
}

void DeviceAttribute::insert(DevVarLongArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarDoubleArray &) -
// insert a DevVarDoubleArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarDoubleArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_DOUBLE;

    DoubleSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        DoubleSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_DOUBLE);
}

void DeviceAttribute::insert(const DevVarDoubleArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarDoubleArray *) -
// insert a DevVarDoubleArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarDoubleArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_DOUBLE;

    DoubleSeq = datum;

    del_mem(Tango::DEV_DOUBLE);
}

void DeviceAttribute::insert(DevVarDoubleArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarStringArray &) -
// insert a DevVarStringArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarStringArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STRING;

    StringSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        StringSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_STRING);
}

void DeviceAttribute::insert(const DevVarStringArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarStringArray *) -
// insert a DevVarStringArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarStringArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STRING;

    StringSeq = datum;

    del_mem(Tango::DEV_STRING);
}

void DeviceAttribute::insert(DevVarStringArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarFloatArray &) -
// insert a DevVarFloatArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarFloatArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_FLOAT;

    FloatSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        FloatSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_FLOAT);
}

void DeviceAttribute::insert(const DevVarFloatArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarFloatArray *) -
// insert a DevVarFloatArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarFloatArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_FLOAT;

    FloatSeq = datum;

    del_mem(Tango::DEV_FLOAT);
}

void DeviceAttribute::insert(DevVarFloatArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarBooleanArray &) -
// insert a DevVarBooleanArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarBooleanArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_BOOLEAN;

    BooleanSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        BooleanSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_BOOLEAN);
}

void DeviceAttribute::insert(const DevVarBooleanArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarBooleanArray *) -
// insert a DevVarBooleanArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarBooleanArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_BOOLEAN;

    BooleanSeq = datum;

    del_mem(Tango::DEV_BOOLEAN);
}

void DeviceAttribute::insert(DevVarBooleanArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarUShortArray &) -
// insert a DevVarUShortArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarUShortArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_USHORT;

    UShortSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        UShortSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_USHORT);
}

void DeviceAttribute::insert(const DevVarUShortArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarUShortArray *) -
// insert a DevVarUShortArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarUShortArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_USHORT;

    UShortSeq = datum;

    del_mem(Tango::DEV_USHORT);
}

void DeviceAttribute::insert(DevVarUShortArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarCharArray &) -
// insert a DevVarCharArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarCharArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_UCHAR;

    UCharSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        UCharSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_UCHAR);
}

void DeviceAttribute::insert(const DevVarCharArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarCharArray *) -
// insert a DevVarCharArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarCharArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_UCHAR;

    UCharSeq = datum;

    del_mem(Tango::DEV_UCHAR);
}

void DeviceAttribute::insert(DevVarCharArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarLong64Array &) -
// insert a DevVarLong64Array by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarLong64Array &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG64;

    Long64Seq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        Long64Seq[i] = datum[i];
    }

    del_mem(Tango::DEV_LONG64);
}

void DeviceAttribute::insert(const DevVarLong64Array &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarLong64Array *) -
// insert a DevVarLong64Array by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarLong64Array *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_LONG64;

    Long64Seq = datum;

    del_mem(Tango::DEV_LONG64);
}

void DeviceAttribute::insert(DevVarLong64Array *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarULongArray &) -
// insert a DevVarULongArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarULongArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG;

    ULongSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        ULongSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_ULONG);
}

void DeviceAttribute::insert(const DevVarULongArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarULongArray *) -
// insert a DevVarULongArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarULongArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG;

    ULongSeq = datum;

    del_mem(Tango::DEV_ULONG);
}

void DeviceAttribute::insert(DevVarULongArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarULong64Array &) -
// insert a DevVarULong64Array by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarULong64Array &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG64;

    ULong64Seq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        ULong64Seq[i] = datum[i];
    }

    del_mem(Tango::DEV_ULONG64);
}

void DeviceAttribute::insert(const DevVarULong64Array &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarULong64Array *) -
// insert a DevVarULong64Array by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarULong64Array *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_ULONG64;

    ULong64Seq = datum;

    del_mem(Tango::DEV_ULONG64);
}

void DeviceAttribute::insert(DevVarULong64Array *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(const DevVarStateArray &) -
// insert a DevVarStateArray by reference into the DeviceAttribute.
// This inserter copy the data
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(const DevVarStateArray &datum)
{
    dim_x = datum.length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STATE;

    StateSeq->length(datum.length());
    for(unsigned int i = 0; i < datum.length(); i++)
    {
        StateSeq[i] = datum[i];
    }

    del_mem(Tango::DEV_STATE);
}

void DeviceAttribute::insert(const DevVarStateArray &datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::operator <<(DevVarStateArray *) -
// insert a DevVarStateArray by pointer into the DeviceAttribute.
// This inserter takes onwership of the pointed to memory
//
//-----------------------------------------------------------------------------

void DeviceAttribute::operator<<(DevVarStateArray *datum)
{
    dim_x = datum->length();
    dim_y = 0;
    w_dim_x = 0;
    w_dim_y = 0;
    quality = Tango::ATTR_VALID;
    data_format = Tango::FMT_UNKNOWN;
    data_type = Tango::DEV_STATE;

    StateSeq = datum;

    del_mem(Tango::DEV_STATE);
}

void DeviceAttribute::insert(DevVarStateArray *datum, int x, int y)
{
    *this << datum;
    dim_x = x;
    dim_y = y;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract(char *&,unsigned char *&)
//
// - extract the read value as pointers from the DeviceAttribute
// for the DevEncoded data type
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract(char *&str, unsigned char *&data_ptr, unsigned int &data_size)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(EncodedSeq.operator->() != nullptr)
    {
        if(EncodedSeq->length() != 0)
        {
            str = Tango::string_dup(EncodedSeq[0].encoded_format.in());
            data_size = EncodedSeq[0].encoded_data.length();
            data_ptr = EncodedSeq[0].encoded_data.get_buffer(true);
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

bool DeviceAttribute::extract(const char *&str, unsigned char *&data_ptr, unsigned int &data_size)
{
    return extract(const_cast<char *&>(str), data_ptr, data_size);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract(std::string &,std::vector<unsigned char> &)
//
// - extract the read value as pointers from the DeviceAttribute
// for the DevEncoded data type
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract(std::string &str, std::vector<unsigned char> &dat)
{
    return extract_read(str, dat);
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::string &,std::vector<unsigned char> &)
//
// - extract the read value as a string, vector<unsigned char> from the DeviceAttribute
// for the DevEncoded data type
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::string &datum_str, std::vector<unsigned char> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(EncodedSeq.operator->() != nullptr)
    {
        if(EncodedSeq->length() != 0)
        {
            datum_str = EncodedSeq[0].encoded_format;

            unsigned long length = EncodedSeq[0].encoded_data.length();
            datum.resize(length);

            for(unsigned long i = 0; i < length; i++)
            {
                datum[i] = EncodedSeq[0].encoded_data[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::string &,std::vector<unsigned char> &)
//
// - extract the set value as a string,vector<unsigned char> from the DeviceAttribute
// when the data type is DevEncoded
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::string &datum_str, std::vector<unsigned char> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(EncodedSeq.operator->() != nullptr)
    {
        if(EncodedSeq->length() == 2)
        {
            datum_str = EncodedSeq[1].encoded_format;

            unsigned long length = EncodedSeq[1].encoded_data.length();
            datum.resize(length);

            for(unsigned long i = 0; i < length; i++)
            {
                datum[i] = EncodedSeq[1].encoded_data[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<std::string> &)
//
// - extract the read value as a vector<string> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<std::string> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(StringSeq.operator->() != nullptr)
    {
        if(StringSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = StringSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<std::string> &)
//
// - extract the set value as a vector<string> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<std::string> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(StringSeq.operator->() != nullptr)
    {
        if(StringSeq->length() != 0)
        {
            // check the size of the setpoint values
            int read_length = check_set_value_size(StringSeq->length());

            // copy the set point values to the vector
            datum.resize(StringSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < StringSeq->length(); i++, k++)
            {
                datum[k] = StringSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<short> &)
//
// - extract the read value as a vector<short> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<short> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(ShortSeq.operator->() != nullptr)
    {
        if(ShortSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = ShortSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<short> &)
//
// - extract the set value as a vector<short> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<short> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ShortSeq.operator->() != nullptr)
    {
        if(ShortSeq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(ShortSeq->length());

            // copy the set point values to the vector
            datum.resize(ShortSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < ShortSeq->length(); i++, k++)
            {
                datum[k] = ShortSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<DevLong> &)
//
// - extract the read value as a vector<DevLong> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<DevLong> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(LongSeq.operator->() != nullptr)
    {
        if(LongSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = LongSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<DevLong> &)
//
// - extract the set value as a vector<DevLong> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<DevLong> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(LongSeq.operator->() != nullptr)
    {
        if(LongSeq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(LongSeq->length());

            // copy the set point values to the vector
            datum.resize(LongSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < LongSeq->length(); i++, k++)
            {
                datum[k] = LongSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<double> &)
//
// - extract the read value as a vector<double> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<double> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(DoubleSeq.operator->() != nullptr)
    {
        if(DoubleSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = DoubleSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<double> &)
//
// - extract the set value as a vector<double> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<double> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(DoubleSeq.operator->() != nullptr)
    {
        if(DoubleSeq->length() != 0)
        {
            // check the size of the setpoint values
            int read_length = check_set_value_size(DoubleSeq->length());

            // copy the set point values to the vector
            datum.resize(DoubleSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < DoubleSeq->length(); i++, k++)
            {
                datum[k] = DoubleSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<float> &)
//
// - extract the read value as a vector<float> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<float> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(FloatSeq.operator->() != nullptr)
    {
        if(FloatSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = FloatSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<float> &)
//
// - extract the set value as a vector<float> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<float> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(FloatSeq.operator->() != nullptr)
    {
        if(FloatSeq->length() != 0)
        {
            // check the size of the setpoint values
            int read_length = check_set_value_size(FloatSeq->length());

            // copy the set point values to the vector
            datum.resize(FloatSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < FloatSeq->length(); i++, k++)
            {
                datum[k] = FloatSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<bool> &)
//
// - extract the read value as a vector<bool> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<bool> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(BooleanSeq.operator->() != nullptr)
    {
        if(BooleanSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = BooleanSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<bool> &)
//
// - extract the set value as a vector<bool> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<bool> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(BooleanSeq.operator->() != nullptr)
    {
        if(BooleanSeq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(BooleanSeq->length());

            // copy the set point values to the vector
            datum.resize(BooleanSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < BooleanSeq->length(); i++, k++)
            {
                datum[k] = BooleanSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<unsigned short> &)
//
// - extract the read value as a vector<unsigned short> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<unsigned short> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(UShortSeq.operator->() != nullptr)
    {
        if(UShortSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = UShortSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<unsigned short> &)
//
// - extract the set value as a vector<unsigned short> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<unsigned short> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UShortSeq.operator->() != nullptr)
    {
        if(UShortSeq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(UShortSeq->length());

            // copy the set point values to the vector
            datum.resize(UShortSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < UShortSeq->length(); i++, k++)
            {
                datum[k] = UShortSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<unsigned char> &)
//
// - extract the read value as a vector<unsigned char> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<unsigned char> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(UCharSeq.operator->() != nullptr)
    {
        if(UCharSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = UCharSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<unsigned char> &)
//
// - extract the set value as a vector<unsigned char> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<unsigned char> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(UCharSeq.operator->() != nullptr)
    {
        if(UCharSeq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(UCharSeq->length());

            // copy the set point values to the vector
            datum.resize(UCharSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < UCharSeq->length(); i++, k++)
            {
                datum[k] = UCharSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<DevLong64> &)
//
// - extract the read value as a vector<DevLong64> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<DevLong64> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(Long64Seq.operator->() != nullptr)
    {
        if(Long64Seq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = Long64Seq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<DevLong64> &)
//
// - extract the set value as a vector<DevLong64> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<DevLong64> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(Long64Seq.operator->() != nullptr)
    {
        if(Long64Seq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(Long64Seq->length());

            // copy the set point values to the vector
            datum.resize(Long64Seq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < Long64Seq->length(); i++, k++)
            {
                datum[k] = Long64Seq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<DevULong64> &)
//
// - extract the read value as a vector<DevULong64> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<DevULong64> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(ULong64Seq.operator->() != nullptr)
    {
        if(ULong64Seq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = ULong64Seq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<DevULong64> &)
//
// - extract the set value as a vector<DevULong64> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<DevULong64> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULong64Seq.operator->() != nullptr)
    {
        if(ULong64Seq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(ULong64Seq->length());

            // copy the set point values to the vector
            datum.resize(ULong64Seq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < ULong64Seq->length(); i++, k++)
            {
                datum[k] = ULong64Seq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<DevULong> &)
//
// - extract the read value as a vector<DevULong> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<DevULong> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(ULongSeq.operator->() != nullptr)
    {
        if(ULongSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = ULongSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<DevULong> &)
//
// - extract the set value as a vector<DevULong> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<DevULong> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(ULongSeq.operator->() != nullptr)
    {
        if(ULongSeq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(ULongSeq->length());

            // copy the set point values to the vector
            datum.resize(ULongSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < ULongSeq->length(); i++, k++)
            {
                datum[k] = ULongSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_read(std::vector<DevState> &)
//
// - extract the read value as a vector<DevState> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_read(std::vector<DevState> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    // copy the read value to the vector

    if(StateSeq.operator->() != nullptr)
    {
        if(StateSeq->length() != 0)
        {
            long length = get_nb_read();
            datum.resize(length);

            for(long i = 0; i < length; i++)
            {
                datum[i] = StateSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceAttribute::extract_set(std::vector<DevState> &)
//
// - extract the set value as a vector<DevState> from the DeviceAttribute
//
//-----------------------------------------------------------------------------

bool DeviceAttribute::extract_set(std::vector<DevState> &datum)
{
    // check for available data

    bool ret = check_for_data();
    if(!ret)
    {
        return false;
    }

    if(StateSeq.operator->() != nullptr)
    {
        if(StateSeq->length() != 0)
        {
            // check the size of the setpoint values
            long read_length = check_set_value_size(StateSeq->length());

            // copy the set point values to the vector
            datum.resize(StateSeq->length() - read_length);
            unsigned int k = 0;
            for(unsigned int i = read_length; i < StateSeq->length(); i++, k++)
            {
                datum[k] = StateSeq[i];
            }
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        // check the wrongtype_flag
        ret = check_wrong_type_exception();
    }
    return ret;
}

//+-------------------------------------------------------------------------
//
// method name :     check_for data
//
// description :     Checks whether attribute data is available.
//                In case of missing data, an exception will be
//                thrown when the failed exception flag is set.
//                Otherwise the method returns false.
//
// ret : false when no attribute data is available.
//
//--------------------------------------------------------------------------
bool DeviceAttribute::check_for_data()
{
    ext->ext_state.reset();

    if(err_list.operator->() != nullptr)
    {
        if(err_list.in().length() != 0)
        {
            ext->ext_state.set(failed_flag);

            if(exceptions_flags.test(failed_flag))
            {
                throw DevFailed(err_list.in());
            }
            else
            {
                return false;
            }
        }
    }

    if(is_empty())
    {
        return false;
    }

    return true;
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method name :
//        check_wrong_type_exception
//
// description :
//        Checks whether the wrongtype exception flag is set and throws an exception in this case.
//      Otherwise the method will return false.
//
// return :
//         False when no exception was thrown.
//
//-------------------------------------------------------------------------------------------------------------------
bool DeviceAttribute::check_wrong_type_exception()
{
    ext->ext_state.set(wrongtype_flag);

    if(exceptions_flags.test(wrongtype_flag))
    {
        TANGO_THROW_DETAILED_EXCEPTION(
            ApiDataExcept,
            API_IncompatibleAttrArgumentType,
            "Cannot extract, data type in DeviceAttribute object is not coherent with the type "
            "provided to extraction method");
    }

    return false;
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method name :
//        check_set_value_size
//
// description :
//        checks wether set value data is available and calculates the index of the first set value element.
//
// arg(s) :
//        in:
//            - seq_length : the length of the attribute data array
//
// return :
//        The index of the first set value element.
//
//----------------------------------------------------------------------------------------------------------------
int DeviceAttribute::check_set_value_size(int seq_length)
{
    //
    // check if the attribute data contains a set value
    //

    if(get_nb_written() == 0)
    {
        // no set point available

        TANGO_THROW_DETAILED_EXCEPTION(ApiDataExcept,
                                       API_NoSetValueAvailable,
                                       "Cannot extract, data from the DeviceAttribute object. No set value available");
    }

    //
    // For Tango::WRITE attributes, the read and set value are identical! In this case the number of set values is the
    // same as the number of data elements in the returned sequence.
    //

    if(get_nb_written() == seq_length)
    {
        return 0;
    }
    else
    {
        return get_nb_read();
    }
}

//+----------------------------------------------------------------------------------------------------------------
//
// method name :
//        DeviceAttribute::del_mem
//
// description :
//        Delete already allocated memory except for the data just inserted
//
// arg(s) :
//        in :
//            - data_type : The data type just inserted
//
//----------------------------------------------------------------------------------------------------------------

void DeviceAttribute::del_mem(int _data_type)
{
    if((_data_type != Tango::DEV_STRING) && (StringSeq.operator->() != nullptr))
    {
        delete StringSeq._retn();
    }
    if((_data_type != Tango::DEV_LONG) && (LongSeq.operator->() != nullptr))
    {
        delete LongSeq._retn();
    }
    if((_data_type != Tango::DEV_SHORT) && (ShortSeq.operator->() != nullptr))
    {
        delete ShortSeq._retn();
    }
    if((_data_type != Tango::DEV_DOUBLE) && (DoubleSeq.operator->() != nullptr))
    {
        delete DoubleSeq._retn();
    }
    if((_data_type != Tango::DEV_FLOAT) && (FloatSeq.operator->() != nullptr))
    {
        delete FloatSeq._retn();
    }
    if((_data_type != Tango::DEV_BOOLEAN) && (BooleanSeq.operator->() != nullptr))
    {
        delete BooleanSeq._retn();
    }
    if((_data_type != Tango::DEV_USHORT) && (UShortSeq.operator->() != nullptr))
    {
        delete UShortSeq._retn();
    }
    if((_data_type != Tango::DEV_UCHAR) && (UCharSeq.operator->() != nullptr))
    {
        delete UCharSeq._retn();
    }
    if((_data_type != Tango::DEV_LONG64) && (Long64Seq.operator->() != nullptr))
    {
        delete Long64Seq._retn();
    }
    if((_data_type != Tango::DEV_ULONG) && (ULongSeq.operator->() != nullptr))
    {
        delete ULongSeq._retn();
    }
    if((_data_type != Tango::DEV_ULONG64) && (ULong64Seq.operator->() != nullptr))
    {
        delete ULong64Seq._retn();
    }
    if((_data_type != Tango::DEV_STATE) && (StateSeq.operator->() != nullptr))
    {
        delete StateSeq._retn();
    }
    if((_data_type != Tango::DEV_ENCODED) && (EncodedSeq.operator->() != nullptr))
    {
        delete EncodedSeq._retn();
    }
}

//+-----------------------------------------------------------------------------------------------------------------
//
// function name :
//         operator overloading :     <<
//
// description :
//        Friend function to ease printing instance of the DeviceAttribute class
//
//-------------------------------------------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &o_str, const DeviceAttribute &da)
{
    if(da.has_failed())
    {
        o_str << "Exception stored in DeviceAttribute object" << std::endl;
        Except::print_error_stack(da.err_list, o_str);
    }
    else
    {
        if(da.is_empty_noexcept())
        {
            o_str << "No data in DeviceAttribute object";
            return o_str;
        }

        //
        // Print date
        //

        if(da.time.tv_sec != 0)
        {
            std::tm tm = Tango_localtime(da.time.tv_sec);
            o_str << std::put_time(&tm, "%c") << " (" << da.time.tv_sec << "," << da.time.tv_usec << " sec) : ";
        }

        //
        // print attribute name
        //

        o_str << da.name;

        //
        // print dim_x and dim_y
        //

        o_str << " (dim_x = " << da.dim_x << ", dim_y = " << da.dim_y << ", ";
        o_str << "w_dim_x = " << da.w_dim_x << ", w_dim_y = " << da.w_dim_y << ", ";

        //
        // Print quality
        //

        o_str << "Data quality factor = " << da.quality << ", ";

        //
        // Print data format
        //

        o_str << "Data format = " << da.data_format << ", ";

        //
        // Print data type
        //

        o_str << "Data type = " << (CmdArgType) da.get_type() << ")" << std::endl;

        //
        // Print data (if valid)
        //

        if(da.quality != Tango::ATTR_INVALID)
        {
            detail::stringify_attribute_data(o_str, da);
        }
    }

    return o_str;
}

bool DeviceAttribute::is_empty_noexcept() const
{
    try
    {
        return is_empty();
    }
    catch(Tango::WrongData &)
    {
        return true;
    }
}

} // namespace Tango
