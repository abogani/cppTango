//
// devapi_datahist.cpp     - C++ source code file for TANGO devapi class
//              DeviceDataHistory and DeviceAttributeHistory
//
// programmer(s)     - Emmanuel Taurel (taurel@esrf.fr)
//
// original         - June 2002
//
// Copyright (C) :      2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
//
//
//

#include <tango/client/devapi.h>
#include <tango/server/seqvec.h>
#include <tango/server/tango_clock.h>
#include <iomanip>
#include <memory>

#include <tango/internal/utils.h>

using namespace CORBA;

namespace Tango
{

//-----------------------------------------------------------------------------
//
// DeviceDataHistory::DeviceDataHistory() - constructors to create DeviceDataHistory
//
//-----------------------------------------------------------------------------

DeviceDataHistory::DeviceDataHistory() :

    ext_hist(nullptr)
{
    fail = false;
    err = new DevErrorList();
    seq_ptr = nullptr;
    ref_ctr_ptr = nullptr;
}

DeviceDataHistory::DeviceDataHistory(int n, int *ref, DevCmdHistoryList *ptr) :
    ext_hist(nullptr)
{
    ref_ctr_ptr = ref;
    seq_ptr = ptr;

    (*ref_ctr_ptr)++;

    any = &((*ptr)[n].value);
    fail = (*ptr)[n].cmd_failed;
    time = (*ptr)[n].time;
    err = &((*ptr)[n].errors);
}

DeviceDataHistory::DeviceDataHistory(const DeviceDataHistory &source) :
    DeviceData(source),
    ext_hist(nullptr)
{
    fail = source.fail;
    time = source.time;
    err = const_cast<DeviceDataHistory &>(source).err._retn();

    seq_ptr = source.seq_ptr;
    ref_ctr_ptr = source.ref_ctr_ptr;
    if(ref_ctr_ptr != nullptr)
    {
        (*ref_ctr_ptr)++;
    }

    if(source.ext_hist != nullptr)
    {
        ext_hist = std::make_unique<DeviceDataHistoryExt>();
        *(ext_hist) = *(source.ext_hist);
    }
}

DeviceDataHistory::DeviceDataHistory(DeviceDataHistory &&source) :
    DeviceData(std::move(source)),
    ext_hist(nullptr)
{
    fail = source.fail;
    time = source.time;
    err = source.err._retn();

    seq_ptr = source.seq_ptr;
    ref_ctr_ptr = source.ref_ctr_ptr;

    if(source.ext_hist != nullptr)
    {
        ext_hist = std::move(source.ext_hist);
    }
    else
    {
        ext_hist.reset();
    }
}

//-----------------------------------------------------------------------------
//
// DeviceDataHistory::~DeviceDataHistory() - Destructor
//
//-----------------------------------------------------------------------------

DeviceDataHistory::~DeviceDataHistory()
{
    if(seq_ptr != nullptr)
    {
        any._retn();
        err._retn();

        (*ref_ctr_ptr)--;
        if(*ref_ctr_ptr == 0)
        {
            delete seq_ptr;
            delete ref_ctr_ptr;
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceDataHistory::operator=() - assignement operator
//
//-----------------------------------------------------------------------------

DeviceDataHistory &DeviceDataHistory::operator=(const DeviceDataHistory &rval)
{
    if(this != &rval)
    {
        //
        // Assignement of DeviceData class members first
        //

        this->DeviceData::operator=(rval);

        //
        // Then, assignement of DeviceDataHistory members
        //

        fail = rval.fail;
        time = rval.time;
        err = rval.err;

        if(ref_ctr_ptr != nullptr)
        {
            (*ref_ctr_ptr)--;
            if(*ref_ctr_ptr == 0)
            {
                delete seq_ptr;
                delete ref_ctr_ptr;
            }
        }

        seq_ptr = rval.seq_ptr;
        ref_ctr_ptr = rval.ref_ctr_ptr;
        (*ref_ctr_ptr)++;

        if(rval.ext_hist != nullptr)
        {
            ext_hist = std::make_unique<DeviceDataHistoryExt>();
            *(ext_hist) = *(rval.ext_hist);
        }
        else
        {
            ext_hist.reset();
        }
    }

    return *this;
}

//-----------------------------------------------------------------------------
//
// DeviceDataHistory::operator=() - move assignement operator
//
//-----------------------------------------------------------------------------

DeviceDataHistory &DeviceDataHistory::operator=(DeviceDataHistory &&rval)
{
    //
    // Assignement of DeviceData class members first
    //

    this->DeviceData::operator=(std::move(rval));

    //
    // Then, assignement of DeviceDataHistory members
    //

    fail = rval.fail;
    time = rval.time;
    err = rval.err._retn();

    //
    // Decrement old ctr
    //
    if(ref_ctr_ptr != nullptr)
    {
        (*ref_ctr_ptr)--;
        if(*ref_ctr_ptr == 0)
        {
            delete seq_ptr;
            delete ref_ctr_ptr;
        }
    }

    //
    // Copy ctr (but don't increment it) and ptr
    //

    seq_ptr = rval.seq_ptr;
    ref_ctr_ptr = rval.ref_ctr_ptr;

    //
    // Extension class
    //

    if(rval.ext_hist != nullptr)
    {
        ext_hist = std::move(rval.ext_hist);
    }
    else
    {
        ext_hist.reset();
    }

    return *this;
}

//+-------------------------------------------------------------------------
//
// operator overloading :     <<
//
// description :     Friend function to ease printing instance of the
//            DeviceDataHistory class
//
//--------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &o_str, const DeviceDataHistory &dh)
{
    //
    // First, print date
    //

    std::tm tm = Tango_localtime(dh.time.tv_sec);
    o_str << std::put_time(&tm, "%c") << " (" << dh.time.tv_sec << "," << std::setw(6) << std::setfill('0')
          << dh.time.tv_usec << " sec) : ";

    //
    // Print data or error stack
    //

    if(dh.fail)
    {
        unsigned int nb_err = dh.err.in().length();
        for(unsigned long i = 0; i < nb_err; i++)
        {
            o_str << "Tango error stack" << std::endl;
            o_str << "Severity = " << (dh.err.in())[i].severity << std::endl;
            o_str << "Error reason = " << (dh.err.in())[i].reason.in() << std::endl;
            o_str << "Desc : " << (dh.err.in())[i].desc.in() << std::endl;
            o_str << "Origin : " << (dh.err.in())[i].origin.in();
            if(i != nb_err - 1)
            {
                o_str << std::endl;
            }
        }
    }
    else
    {
        o_str << static_cast<const DeviceData &>(dh);
    }

    return o_str;
}

//-----------------------------------------------------------------------------
//
// DeviceAttributeHistory::DeviceAttributeHistory() - constructors to create DeviceAttributeHistory
//
//-----------------------------------------------------------------------------

DeviceAttributeHistory::DeviceAttributeHistory() :

    ext_hist(nullptr)
{
    fail = false;
    err_list = new DevErrorList();
}

DeviceAttributeHistory::DeviceAttributeHistory(int n, DevAttrHistoryList_var &seq) :
    ext_hist(nullptr)
{
    fail = seq[n].attr_failed;

    err_list = new DevErrorList(seq[n].errors);
    time = seq[n].value.time;
    quality = seq[n].value.quality;
    dim_x = seq[n].value.dim_x;
    dim_y = seq[n].value.dim_y;
    name = seq[n].value.name;

    const DevVarLongArray *tmp_seq_lo;
    CORBA::Long *tmp_lo;
    const DevVarLong64Array *tmp_seq_lolo;
    CORBA::LongLong *tmp_lolo;
    const DevVarShortArray *tmp_seq_sh;
    CORBA::Short *tmp_sh;
    const DevVarDoubleArray *tmp_seq_db;
    CORBA::Double *tmp_db;
    const DevVarStringArray *tmp_seq_str;
    char **tmp_str;
    const DevVarFloatArray *tmp_seq_fl;
    CORBA::Float *tmp_fl;
    const DevVarBooleanArray *tmp_seq_boo;
    CORBA::Boolean *tmp_boo;
    const DevVarUShortArray *tmp_seq_ush;
    CORBA::UShort *tmp_ush;
    const DevVarCharArray *tmp_seq_uch;
    CORBA::Octet *tmp_uch;
    const DevVarULongArray *tmp_seq_ulo;
    CORBA::ULong *tmp_ulo;
    const DevVarULong64Array *tmp_seq_ulolo;
    CORBA::ULongLong *tmp_ulolo;
    const DevVarStateArray *tmp_seq_state;
    Tango::DevState *tmp_state;

    CORBA::ULong max, len;

    if((!fail) && (quality != Tango::ATTR_INVALID))
    {
        CORBA::TypeCode_var ty = seq[n].value.value.type();
        CORBA::TypeCode_var ty_alias = ty->content_type();
        CORBA::TypeCode_var ty_seq = ty_alias->content_type();
        switch(ty_seq->kind())
        {
        case tk_long:
            seq[n].value.value >>= tmp_seq_lo;
            max = tmp_seq_lo->maximum();
            len = tmp_seq_lo->length();
            tmp_lo = (const_cast<DevVarLongArray *>(tmp_seq_lo))->get_buffer((CORBA::Boolean) true);
            LongSeq = new DevVarLongArray(max, len, tmp_lo, true);
            break;

        case tk_longlong:
            seq[n].value.value >>= tmp_seq_lolo;
            max = tmp_seq_lolo->maximum();
            len = tmp_seq_lolo->length();
            tmp_lolo = (const_cast<DevVarLong64Array *>(tmp_seq_lolo))->get_buffer((CORBA::Boolean) true);
            Long64Seq = new DevVarLong64Array(max, len, tmp_lolo, true);
            break;

        case tk_short:
            seq[n].value.value >>= tmp_seq_sh;
            max = tmp_seq_sh->maximum();
            len = tmp_seq_sh->length();
            tmp_sh = (const_cast<DevVarShortArray *>(tmp_seq_sh))->get_buffer((CORBA::Boolean) true);
            ShortSeq = new DevVarShortArray(max, len, tmp_sh, true);
            break;

        case tk_double:
            seq[n].value.value >>= tmp_seq_db;
            max = tmp_seq_db->maximum();
            len = tmp_seq_db->length();
            tmp_db = (const_cast<DevVarDoubleArray *>(tmp_seq_db))->get_buffer((CORBA::Boolean) true);
            DoubleSeq = new DevVarDoubleArray(max, len, tmp_db, true);
            break;

        case tk_string:
            seq[n].value.value >>= tmp_seq_str;
            max = tmp_seq_str->maximum();
            len = tmp_seq_str->length();
            tmp_str = (const_cast<DevVarStringArray *>(tmp_seq_str))->get_buffer((CORBA::Boolean) true);
            StringSeq = new DevVarStringArray(max, len, tmp_str, true);
            break;

        case tk_float:
            seq[n].value.value >>= tmp_seq_fl;
            max = tmp_seq_fl->maximum();
            len = tmp_seq_fl->length();
            tmp_fl = (const_cast<DevVarFloatArray *>(tmp_seq_fl))->get_buffer((CORBA::Boolean) true);
            FloatSeq = new DevVarFloatArray(max, len, tmp_fl, true);
            break;

        case tk_boolean:
            seq[n].value.value >>= tmp_seq_boo;
            max = tmp_seq_boo->maximum();
            len = tmp_seq_boo->length();
            tmp_boo = (const_cast<DevVarBooleanArray *>(tmp_seq_boo))->get_buffer((CORBA::Boolean) true);
            BooleanSeq = new DevVarBooleanArray(max, len, tmp_boo, true);
            break;

        case tk_ushort:
            seq[n].value.value >>= tmp_seq_ush;
            max = tmp_seq_ush->maximum();
            len = tmp_seq_ush->length();
            tmp_ush = (const_cast<DevVarUShortArray *>(tmp_seq_ush))->get_buffer((CORBA::Boolean) true);
            UShortSeq = new DevVarUShortArray(max, len, tmp_ush, true);
            break;

        case tk_octet:
            seq[n].value.value >>= tmp_seq_uch;
            max = tmp_seq_uch->maximum();
            len = tmp_seq_uch->length();
            tmp_uch = (const_cast<DevVarCharArray *>(tmp_seq_uch))->get_buffer((CORBA::Boolean) true);
            UCharSeq = new DevVarCharArray(max, len, tmp_uch, true);
            break;

        case tk_ulong:
            seq[n].value.value >>= tmp_seq_ulo;
            max = tmp_seq_ulo->maximum();
            len = tmp_seq_ulo->length();
            tmp_ulo = (const_cast<DevVarULongArray *>(tmp_seq_ulo))->get_buffer((CORBA::Boolean) true);
            ULongSeq = new DevVarULongArray(max, len, tmp_ulo, true);
            break;

        case tk_ulonglong:
            seq[n].value.value >>= tmp_seq_ulolo;
            max = tmp_seq_ulolo->maximum();
            len = tmp_seq_ulolo->length();
            tmp_ulolo = (const_cast<DevVarULong64Array *>(tmp_seq_ulolo))->get_buffer((CORBA::Boolean) true);
            ULong64Seq = new DevVarULong64Array(max, len, tmp_ulolo, true);
            break;

        case tk_enum:
            seq[n].value.value >>= tmp_seq_state;
            max = tmp_seq_state->maximum();
            len = tmp_seq_state->length();
            tmp_state = (const_cast<DevVarStateArray *>(tmp_seq_state))->get_buffer((CORBA::Boolean) true);
            StateSeq = new DevVarStateArray(max, len, tmp_state, true);
            break;

        default:
            TangoSys_OMemStream desc;
            desc << "'seq[" << n << "].value.value' with unexpected sequence kind '" << ty_seq->kind() << "'.";
            TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
        }
    }
}

DeviceAttributeHistory::DeviceAttributeHistory(int n, DevAttrHistoryList_3_var &seq) :
    ext_hist(nullptr)
{
    fail = seq[n].attr_failed;

    err_list = new DevErrorList(seq[n].value.err_list);
    time = seq[n].value.time;
    quality = seq[n].value.quality;
    dim_x = seq[n].value.r_dim.dim_x;
    dim_y = seq[n].value.r_dim.dim_y;
    w_dim_x = seq[n].value.w_dim.dim_x;
    w_dim_y = seq[n].value.w_dim.dim_y;
    name = seq[n].value.name;

    const DevVarLongArray *tmp_seq_lo;
    CORBA::Long *tmp_lo;
    const DevVarLong64Array *tmp_seq_lolo;
    CORBA::LongLong *tmp_lolo;
    const DevVarShortArray *tmp_seq_sh;
    CORBA::Short *tmp_sh;
    const DevVarDoubleArray *tmp_seq_db;
    CORBA::Double *tmp_db;
    const DevVarStringArray *tmp_seq_str;
    char **tmp_str;
    const DevVarFloatArray *tmp_seq_fl;
    CORBA::Float *tmp_fl;
    const DevVarBooleanArray *tmp_seq_boo;
    CORBA::Boolean *tmp_boo;
    const DevVarUShortArray *tmp_seq_ush;
    CORBA::UShort *tmp_ush;
    const DevVarCharArray *tmp_seq_uch;
    CORBA::Octet *tmp_uch;
    const DevVarULongArray *tmp_seq_ulo;
    CORBA::ULong *tmp_ulo;
    const DevVarULong64Array *tmp_seq_ulolo;
    CORBA::ULongLong *tmp_ulolo;
    const DevVarStateArray *tmp_seq_state;
    Tango::DevState *tmp_state;

    CORBA::ULong max, len;

    if((!fail) && (quality != Tango::ATTR_INVALID))
    {
        CORBA::TypeCode_var ty = seq[n].value.value.type();
        CORBA::TypeCode_var ty_alias = ty->content_type();
        CORBA::TypeCode_var ty_seq = ty_alias->content_type();
        switch(ty_seq->kind())
        {
        case tk_long:
            seq[n].value.value >>= tmp_seq_lo;
            max = tmp_seq_lo->maximum();
            len = tmp_seq_lo->length();
            tmp_lo = (const_cast<DevVarLongArray *>(tmp_seq_lo))->get_buffer((CORBA::Boolean) true);
            LongSeq = new DevVarLongArray(max, len, tmp_lo, true);
            break;

        case tk_longlong:
            seq[n].value.value >>= tmp_seq_lolo;
            max = tmp_seq_lolo->maximum();
            len = tmp_seq_lolo->length();
            tmp_lolo = (const_cast<DevVarLong64Array *>(tmp_seq_lolo))->get_buffer((CORBA::Boolean) true);
            Long64Seq = new DevVarLong64Array(max, len, tmp_lolo, true);
            break;

        case tk_short:
            seq[n].value.value >>= tmp_seq_sh;
            max = tmp_seq_sh->maximum();
            len = tmp_seq_sh->length();
            tmp_sh = (const_cast<DevVarShortArray *>(tmp_seq_sh))->get_buffer((CORBA::Boolean) true);
            ShortSeq = new DevVarShortArray(max, len, tmp_sh, true);
            break;

        case tk_double:
            seq[n].value.value >>= tmp_seq_db;
            max = tmp_seq_db->maximum();
            len = tmp_seq_db->length();
            tmp_db = (const_cast<DevVarDoubleArray *>(tmp_seq_db))->get_buffer((CORBA::Boolean) true);
            DoubleSeq = new DevVarDoubleArray(max, len, tmp_db, true);
            break;

        case tk_string:
            seq[n].value.value >>= tmp_seq_str;
            max = tmp_seq_str->maximum();
            len = tmp_seq_str->length();
            tmp_str = (const_cast<DevVarStringArray *>(tmp_seq_str))->get_buffer((CORBA::Boolean) true);
            StringSeq = new DevVarStringArray(max, len, tmp_str, true);
            break;

        case tk_float:
            seq[n].value.value >>= tmp_seq_fl;
            max = tmp_seq_fl->maximum();
            len = tmp_seq_fl->length();
            tmp_fl = (const_cast<DevVarFloatArray *>(tmp_seq_fl))->get_buffer((CORBA::Boolean) true);
            FloatSeq = new DevVarFloatArray(max, len, tmp_fl, true);
            break;

        case tk_boolean:
            seq[n].value.value >>= tmp_seq_boo;
            max = tmp_seq_boo->maximum();
            len = tmp_seq_boo->length();
            tmp_boo = (const_cast<DevVarBooleanArray *>(tmp_seq_boo))->get_buffer((CORBA::Boolean) true);
            BooleanSeq = new DevVarBooleanArray(max, len, tmp_boo, true);
            break;

        case tk_ushort:
            seq[n].value.value >>= tmp_seq_ush;
            max = tmp_seq_ush->maximum();
            len = tmp_seq_ush->length();
            tmp_ush = (const_cast<DevVarUShortArray *>(tmp_seq_ush))->get_buffer((CORBA::Boolean) true);
            UShortSeq = new DevVarUShortArray(max, len, tmp_ush, true);
            break;

        case tk_octet:
            seq[n].value.value >>= tmp_seq_uch;
            max = tmp_seq_uch->maximum();
            len = tmp_seq_uch->length();
            tmp_uch = (const_cast<DevVarCharArray *>(tmp_seq_uch))->get_buffer((CORBA::Boolean) true);
            UCharSeq = new DevVarCharArray(max, len, tmp_uch, true);
            break;

        case tk_ulong:
            seq[n].value.value >>= tmp_seq_ulo;
            max = tmp_seq_ulo->maximum();
            len = tmp_seq_ulo->length();
            tmp_ulo = (const_cast<DevVarULongArray *>(tmp_seq_ulo))->get_buffer((CORBA::Boolean) true);
            ULongSeq = new DevVarULongArray(max, len, tmp_ulo, true);
            break;

        case tk_ulonglong:
            seq[n].value.value >>= tmp_seq_ulolo;
            max = tmp_seq_ulolo->maximum();
            len = tmp_seq_ulolo->length();
            tmp_ulolo = (const_cast<DevVarULong64Array *>(tmp_seq_ulolo))->get_buffer((CORBA::Boolean) true);
            ULong64Seq = new DevVarULong64Array(max, len, tmp_ulolo, true);
            break;

        case tk_enum:
            seq[n].value.value >>= tmp_seq_state;
            max = tmp_seq_state->maximum();
            len = tmp_seq_state->length();
            tmp_state = (const_cast<DevVarStateArray *>(tmp_seq_state))->get_buffer((CORBA::Boolean) true);
            StateSeq = new DevVarStateArray(max, len, tmp_state, true);
            break;

        default:
            TangoSys_OMemStream desc;
            desc << "'seq[" << n << "].value.value' with unexpected sequence kind '" << ty_seq->kind() << "'.";
            TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, desc.str().c_str());
        }
    }
}

DeviceAttributeHistory::DeviceAttributeHistory(const DeviceAttributeHistory &source) :
    DeviceAttribute(source),
    ext_hist(nullptr)
{
    fail = source.fail;

    if(source.ext_hist != nullptr)
    {
        ext_hist = std::make_unique<DeviceAttributeHistoryExt>();
        *(ext_hist) = *(source.ext_hist);
    }
}

DeviceAttributeHistory::DeviceAttributeHistory(DeviceAttributeHistory &&source) :
    DeviceAttribute(std::move(source)),
    ext_hist(nullptr)
{
    fail = source.fail;

    if(source.ext_hist != nullptr)
    {
        ext_hist = std::move(source.ext_hist);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceAttributeHistory::~DeviceAttributeHistory() - Destructor
//
//-----------------------------------------------------------------------------

DeviceAttributeHistory::~DeviceAttributeHistory() { }

//-----------------------------------------------------------------------------
//
// DeviceAttributeHistory::operator=() - assignement operator
//
//-----------------------------------------------------------------------------

DeviceAttributeHistory &DeviceAttributeHistory::operator=(const DeviceAttributeHistory &rval)
{
    if(this != &rval)
    {
        //
        // First, assignement of DeviceAttribute class members
        //

        this->DeviceAttribute::operator=(rval);

        //
        // Then, assignement of DeviceAttributeHistory members
        //

        fail = rval.fail;

        if(rval.ext_hist != nullptr)
        {
            ext_hist = std::make_unique<DeviceAttributeHistoryExt>();
            *(ext_hist) = *(rval.ext_hist);
        }
        else
        {
            ext_hist.reset();
        }
    }

    return *this;
}

DeviceAttributeHistory &DeviceAttributeHistory::operator=(DeviceAttributeHistory &&rval)
{
    //
    // First, assignement of DeviceAttribute class members
    //

    this->DeviceAttribute::operator=(std::move(rval));

    //
    // Then, assignement of DeviceAttributeHistory members
    //

    fail = rval.fail;

    if(rval.ext_hist != nullptr)
    {
        ext_hist = std::move(rval.ext_hist);
    }
    else
    {
        ext_hist.reset();
    }

    return *this;
}

//+-------------------------------------------------------------------------
//
// operator overloading :     <<
//
// description :     Friend function to ease printing instance of the
//            DeviceAttributeHistory class
//
//--------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &o_str, const DeviceAttributeHistory &dah)
{
    //
    // Print date
    //

    if(dah.time.tv_sec != 0)
    {
        std::tm tm = Tango_localtime(dah.time.tv_sec);
        o_str << std::put_time(&tm, "%c") << " (" << dah.time.tv_sec << "," << std::setw(6) << std::setfill('0')
              << dah.time.tv_usec << " sec) : ";
    }

    //
    // print attribute name
    //

    o_str << dah.name;

    //
    // print dim_x and dim_y
    //

    o_str << " (dim_x = " << dah.dim_x << ", dim_y = " << dah.dim_y << ", ";

    //
    // print write dim_x and dim_y
    //

    o_str << "w_dim_x = " << dah.w_dim_x << ", w_dim_y = " << dah.w_dim_y << ", ";

    //
    // Print quality
    //

    o_str << "Data quality factor = " << dah.quality << ")" << std::endl;

    //
    // Print data (if valid) or error stack
    //

    if(dah.fail)
    {
        unsigned int nb_err = dah.err_list.in().length();
        for(unsigned long i = 0; i < nb_err; i++)
        {
            o_str << "Tango error stack" << std::endl;
            o_str << "Severity = " << dah.err_list.in()[i].severity << std::endl;
            o_str << "Error reason = " << dah.err_list.in()[i].reason.in() << std::endl;
            o_str << "Desc : " << dah.err_list.in()[i].desc.in() << std::endl;
            o_str << "Origin : " << dah.err_list.in()[i].origin.in();
            if(i != nb_err - 1)
            {
                o_str << std::endl;
            }
        }
    }
    else
    {
        if(dah.quality != Tango::ATTR_INVALID)
        {
            if(dah.is_empty_noexcept())
            {
                o_str << "No data in DeviceData object";
            }
            else
            {
                if(dah.LongSeq.operator->() != nullptr)
                {
                    o_str << *(dah.LongSeq.operator->());
                }
                else if(dah.ShortSeq.operator->() != nullptr)
                {
                    o_str << *(dah.ShortSeq.operator->());
                }
                else if(dah.DoubleSeq.operator->() != nullptr)
                {
                    o_str << *(dah.DoubleSeq.operator->());
                }
                else if(dah.FloatSeq.operator->() != nullptr)
                {
                    o_str << *(dah.FloatSeq.operator->());
                }
                else if(dah.BooleanSeq.operator->() != nullptr)
                {
                    o_str << *(dah.BooleanSeq.operator->());
                }
                else if(dah.UShortSeq.operator->() != nullptr)
                {
                    o_str << *(dah.UShortSeq.operator->());
                }
                else if(dah.UCharSeq.operator->() != nullptr)
                {
                    o_str << *(dah.UCharSeq.operator->());
                }
                else if(dah.Long64Seq.operator->() != nullptr)
                {
                    o_str << *(dah.Long64Seq.operator->());
                }
                else if(dah.ULongSeq.operator->() != nullptr)
                {
                    o_str << *(dah.ULongSeq.operator->());
                }
                else if(dah.ULong64Seq.operator->() != nullptr)
                {
                    o_str << *(dah.ULong64Seq.operator->());
                }
                else if(dah.StateSeq.operator->() != nullptr)
                {
                    o_str << *(dah.StateSeq.operator->());
                }
                else if(dah.EncodedSeq.operator->() != nullptr)
                {
                    o_str << *(dah.EncodedSeq.operator->());
                }
                else
                {
                    o_str << *(dah.StringSeq.operator->());
                }
            }
        }
    }

    return o_str;
}

} // namespace Tango
