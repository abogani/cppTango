//====================================================================================================================
//
// file :               attrsetval.cpp
//
// description :        C++ source code for the Attribute class set_value() method family.
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2013,2014,2015
//						European Synchrotron Radiation Facility
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
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//====================================================================================================================

#include <tango/tango.h>
#include <tango/server/attribute.h>
#include <tango/server/classattribute.h>
#include <tango/server/eventsupplier.h>

#include <functional>
#include <algorithm>

#ifdef _TG_WINDOWS_
#include <sys/types.h>
#endif /* _TG_WINDOWS_ */

namespace Tango
{

void Attribute::set_value(Tango::DevString *p_data_str,Tango::DevUChar *p_data,long size,bool release)
{
	if (p_data_str == NULL || p_data == NULL)
	{
		TangoSys_OMemStream o;
		o << "Data pointer for attribute " << name << " is NULL!" << std::ends;
		TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

	if (release == false)
	{
		enc_help.encoded_format = Tango::string_dup(*p_data_str);
		enc_help.encoded_data.replace(size,size,p_data,false);

		set_value(&enc_help);
	}
	else
	{
		DevEncoded *enc_ptr = new DevEncoded;
		enc_ptr->encoded_format = Tango::string_dup(*p_data_str);
		delete [] *p_data_str;
		enc_ptr->encoded_data.replace(size,size,p_data,true);

		set_value(enc_ptr,1,0,true);
	}
}

void Attribute::set_value(Tango::EncodedAttribute *attr)
{
	CHECK_PTR(attr,name);

	Tango::DevString *f    = attr->get_format();
	Tango::DevUChar  *d    = attr->get_data();
	long              size = attr->get_size();

	if( *f==NULL )
	{
		TangoSys_OMemStream o;
		o << "DevEncoded format for attribute " << name << " not specified" << std::ends;
		TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

	if( size==0 || !d )
	{
		TangoSys_OMemStream o;
		o << "DevEncoded data for attribute " << name << " not specified" << std::ends;
		TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

	set_value(f,d,size,false);

	if (attr->get_exclusion() == true)
	{
		set_user_attr_mutex(attr->get_mutex());
	}
}

//---------------------------------------------------------------------------

void Attribute::set_value_date_quality(Tango::DevString *p_data_str,Tango::DevUChar *p_data,long size,time_t t,
				    Tango::AttrQuality qual,
				    bool release)
{
	set_value(p_data_str,p_data,size,release);
	set_quality(qual,false);
	set_date(t);
}

void Attribute::set_value_date_quality(Tango::DevString *p_data_str,Tango::DevUChar *p_data,long size,const TangoTimestamp &t,
				    Tango::AttrQuality qual,
				    bool release)
{
	set_value(p_data_str,p_data,size,release);
	set_quality(qual,false);
	set_date(t);
}
} // End of Tango namespace
