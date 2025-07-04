//+=============================================================================
//
// file :              LogStream.cpp
//
// description :  A TLS helper class
//
// project :        TANGO
//
// author(s) :      N.Leclercq - SOLEIL
//
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
//-=============================================================================

#include <tango/server/logstream.h>
#include <tango/common/log4tango/LoggerStream.h>

#include <tango/common/tango_const.h>
#include <tango/server/tango_config.h>
#include <tango/server/attribute.h>
#include <tango/server/attrmanip.h>
#include <tango/server/dserver.h>
#include <tango/server/device.h>
#include <tango/client/DeviceProxy.h>

namespace Tango
{

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevFailed &e)
{
    // split exception stack into several logs [use a tag to identify the exception]
    static unsigned long exception_tag = 0;
    unsigned long num_errors = e.errors.length();
    for(unsigned long i = 0; i < num_errors; i++)
    {
        TangoSys_OMemStream msg;
        msg << "[Ex:" << exception_tag << "-Err:" << i << "] "
            << "Rsn: " << e.errors[i].reason.in() << " "
            << "Dsc: " << e.errors[i].desc.in() << " "
            << "Org: " << e.errors[i].origin.in();
        ls << msg.str();
        if(i != num_errors - 1)
        {
            ls << std::endl;
        }
    }
    exception_tag++;
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarCharArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i];
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarShortArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i];
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarLongArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i];
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarFloatArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i];
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarDoubleArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i];
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarUShortArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i];
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarULongArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i];
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const DevVarStringArray &v)
{
    long nb_elt = v.length();
    for(long i = 0; i < nb_elt; i++)
    {
        ls << "Element number [" << i << "]: " << v[i].in();
        if(i < (nb_elt - 1))
        {
            ls << std::endl;
        }
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const Attribute &a)
{
    Tango::AttributeConfig conf;
    (const_cast<Attribute &>(a)).get_properties(conf);

    ls << "Attribute name: " << conf.name.in() << std::endl;
    ls << "Attribute data_type: " << (CmdArgType) conf.data_type << std::endl;
    ls << "Attribute data_format: ";

    switch(conf.data_format)
    {
    case Tango::FMT_UNKNOWN:
        break;

    case Tango::SCALAR:
        ls << "scalar" << std::endl;
        break;

    case Tango::SPECTRUM:
        ls << "spectrum, max_dim_x: " << conf.max_dim_x << std::endl;
        break;

    case Tango::IMAGE:
        ls << "image, max_dim_x: " << conf.max_dim_x << ", max_dim_y: " << conf.max_dim_y << std::endl;
        break;
    }

    switch(conf.writable)
    {
    case WRITE:
    case READ_WITH_WRITE:
    case READ_WRITE:
        ls << "Attribute is writable" << std::endl;
        break;
    case READ:
    case WT_UNKNOWN:
    default:
        ls << "Attribute is not writable" << std::endl;
        break;
    }
    ls << "Attribute label: " << conf.label.in() << std::endl;
    ls << "Attribute description: " << conf.description.in() << std::endl;
    ls << "Attribute unit: " << conf.unit.in() << std::endl;
    ls << "Attribute standard unit: " << conf.standard_unit.in() << std::endl;
    ls << "Attribute display unit: " << conf.display_unit.in() << std::endl;
    ls << "Attribute format: " << conf.format.in() << std::endl;
    ls << "Attribute min alarm: " << conf.min_alarm.in() << std::endl;
    ls << "Attribute max alarm: " << conf.max_alarm.in() << std::endl;
    ls << "Attribute min value: " << conf.min_value.in() << std::endl;
    ls << "Attribute max value: " << conf.max_value.in() << std::endl;
    ls << "Attribute writable_attr_name: " << conf.writable_attr_name.in() << std::endl;
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const AttrProperty &ap)
{
    AttrProperty &ap_ = const_cast<AttrProperty &>(ap);
    ls << "Attr.Property: name:" << ap_.get_name() << " - value:" << ap_.get_value() << std::endl;
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const Attr &a)
{
    std::vector<AttrProperty> v = (const_cast<Attr &>(a)).get_class_properties();
    unsigned int n = v.size();
    if(n != 0u)
    {
        for(unsigned i = 0; i < n; i++)
        {
            ls << "Attr: " << const_cast<Attr &>(a).get_name() << " Property: name:" << v[i].get_name()
               << " - value:" << v[i].get_value();
            if(i + 2 <= n)
            {
                ls << std::endl;
            }
        }
    }
    else
    {
        ls << "Attr. " << const_cast<Attr &>(a).get_name() << " has no class properties";
    }
    return ls;
}

//+----------------------------------------------------------------------------
//
// method : LoggerStream::operator<<
//
//-----------------------------------------------------------------------------
log4tango::LoggerStream &operator<<(log4tango::LoggerStream &ls, const AttrManip &m)
{
    ls << m.to_string();
    return ls;
}

} // namespace Tango
