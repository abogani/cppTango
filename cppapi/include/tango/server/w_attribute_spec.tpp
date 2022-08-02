//+===================================================================================================================
//
// file :               w_attribute_spec.tpp
//
// description :        C++ source code for the WAttribute class template methods specialization
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2011,2012,2013,2014,2015
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
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
// $Revision: 17240 $
//
//-==================================================================================================================

#ifndef _WATTRIBUTE_SPEC_TPP
#define _WATTRIBUTE_SPEC_TPP

namespace Tango
{

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//		WAttribute::set_min_value()
//
// description :
//		Sets minimum value attribute property. Throws exception in case the data type of provided
//		property does not match the attribute data type
//
// args :
//		in :
// 			- new_min_value_str : The minimum value property to be set
//
//-------------------------------------------------------------------------------------------------------------------

template <>
inline void WAttribute::set_min_value(const Tango::DevEncoded &)
{
	std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
	TANGO_THROW_EXCEPTION(API_MethodArgument, err_msg.c_str());
}

template <>
inline void WAttribute::set_min_value(const std::string &new_min_value_str)
{
	if((data_type == Tango::DEV_STRING) ||
		(data_type == Tango::DEV_BOOLEAN) ||
		(data_type == Tango::DEV_STATE) ||
		(data_type == Tango::DEV_ENUM))
		throw_err_data_type("min_value",d_name,"WAttribute::set_min_value()");

	std::string min_value_str_tmp = new_min_value_str;
	std::string dev_name = d_name;

	Tango::DeviceClass *dev_class = get_att_device_class(d_name);
	Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
	Tango::Attr &att = mca->get_attr(name);

	std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
	std::vector<AttrProperty> &def_class_prop = att.get_class_properties();

	size_t nb_class = def_class_prop.size();
	size_t nb_user = def_user_prop.size();

	std::string usr_def_val;
	std::string class_def_val;
	bool user_defaults = false;
	bool class_defaults = false;

    user_defaults = prop_in_list("min_value",usr_def_val,nb_user,def_user_prop);

    class_defaults = prop_in_list("min_value",class_def_val,nb_class,def_class_prop);

	bool set_value = true;

    if (class_defaults)
    {
		if(TG_strcasecmp(new_min_value_str.c_str(),AlrmValueNotSpec) == 0)
		{
		    set_value = false;

		    avns_in_db("min_value",dev_name);
            avns_in_att(MIN_VALUE);
		}
		else if ((TG_strcasecmp(new_min_value_str.c_str(),NotANumber) == 0) ||
				(TG_strcasecmp(new_min_value_str.c_str(),class_def_val.c_str()) == 0))
        {
            min_value_str_tmp = class_def_val;
        }
		else if (strlen(new_min_value_str.c_str()) == 0)
		{
		    if (user_defaults)
		    {
                min_value_str_tmp = usr_def_val;
		    }
		    else
		    {
		        set_value = false;

                avns_in_db("min_value",dev_name);
                avns_in_att(MIN_VALUE);
		    }
		}
    }
	else if(user_defaults)
	{
		if(TG_strcasecmp(new_min_value_str.c_str(),AlrmValueNotSpec) == 0)
		{
			set_value = false;

            avns_in_db("min_value",dev_name);
            avns_in_att(MIN_VALUE);
		}
		else if ((TG_strcasecmp(new_min_value_str.c_str(),NotANumber) == 0) ||
				(TG_strcasecmp(new_min_value_str.c_str(),usr_def_val.c_str()) == 0) ||
				(strlen(new_min_value_str.c_str()) == 0))
			min_value_str_tmp = usr_def_val;
	}
	else
	{
		if ((TG_strcasecmp(new_min_value_str.c_str(),AlrmValueNotSpec) == 0) ||
				(TG_strcasecmp(new_min_value_str.c_str(),NotANumber) == 0) ||
				(strlen(new_min_value_str.c_str()) == 0))
		{
			set_value = false;

            avns_in_db("min_value",dev_name);
            avns_in_att(MIN_VALUE);
		}
	}

	if(set_value)
	{
		if ((data_type != Tango::DEV_STRING) &&
		    (data_type != Tango::DEV_BOOLEAN) &&
		    (data_type != Tango::DEV_STATE))
		{
			double db;
			float fl;

			TangoSys_MemStream str;
			str.precision(TANGO_FLOAT_PRECISION);
			str << min_value_str_tmp;
			switch (data_type)
			{
			case Tango::DEV_SHORT:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				set_min_value((DevShort)db);
				break;

			case Tango::DEV_LONG:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				set_min_value((DevLong)db);
				break;

			case Tango::DEV_LONG64:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				set_min_value((DevLong64)db);
				break;

			case Tango::DEV_DOUBLE:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				set_min_value(db);
				break;

			case Tango::DEV_FLOAT:
				if (!(str >> fl && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				set_min_value(fl);
				break;

			case Tango::DEV_USHORT:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				(db < 0.0) ? set_min_value((DevUShort)(-db)) : set_min_value((DevUShort)db);
				break;

			case Tango::DEV_UCHAR:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				(db < 0.0) ? set_min_value((DevUChar)(-db)) : set_min_value((DevUChar)db);
				break;

			case Tango::DEV_ULONG:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				(db < 0.0) ? set_min_value((DevULong)(-db)) : set_min_value((DevULong)db);
				break;

			case Tango::DEV_ULONG64:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				(db < 0.0) ? set_min_value((DevULong64)(-db)) : set_min_value((DevULong64)db);
				break;

			case Tango::DEV_ENCODED:
				if (!(str >> db && str.eof()))
					throw_err_format("min_value",dev_name,"WAttribute::set_min_value()");
				(db < 0.0) ? set_min_value((DevUChar)(-db)) : set_min_value((DevUChar)db);
				break;
			}
		}
		else
			throw_err_data_type("min_value",dev_name,"WAttribute::set_min_value()");
	}
}


//+------------------------------------------------------------------------------------------------------------------
//
// method :
//		WAttribute::set_max_value()
//
// description :
//		Sets maximum value attribute property. Throws exception in case the data type of provided
//		property does not match the attribute data type
//
// args :
//		in :
//			- new_max_value_str : The maximum value property to be set
//
//------------------------------------------------------------------------------------------------------------------

template <>
inline void WAttribute::set_max_value(const Tango::DevEncoded &)
{
	std::string err_msg = "Attribute properties cannot be set with Tango::DevEncoded data type";
	TANGO_THROW_EXCEPTION(API_MethodArgument, err_msg.c_str());
}

template <>
inline void WAttribute::set_max_value(const std::string &new_max_value_str)
{
	if((data_type == Tango::DEV_STRING) ||
		(data_type == Tango::DEV_BOOLEAN) ||
		(data_type == Tango::DEV_STATE))
		throw_err_data_type("max_value",d_name,"WAttribute::set_max_value()");

	std::string max_value_str_tmp = new_max_value_str;
	std::string dev_name = d_name;

	Tango::DeviceClass *dev_class = get_att_device_class(d_name);
	Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
	Tango::Attr &att = mca->get_attr(name);

	std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
	std::vector<AttrProperty> &def_class_prop = att.get_class_properties();

	size_t nb_class = def_class_prop.size();
	size_t nb_user = def_user_prop.size();

	std::string usr_def_val;
	std::string class_def_val;
	bool user_defaults = false;
	bool class_defaults = false;

    user_defaults = prop_in_list("max_value",usr_def_val,nb_user,def_user_prop);

    class_defaults = prop_in_list("max_value",class_def_val,nb_class,def_class_prop);

	bool set_value = true;

    if (class_defaults)
    {
		if(TG_strcasecmp(new_max_value_str.c_str(),AlrmValueNotSpec) == 0)
		{
		    set_value = false;

		    avns_in_db("max_value",dev_name);
            avns_in_att(MAX_VALUE);
		}
		else if ((TG_strcasecmp(new_max_value_str.c_str(),NotANumber) == 0) ||
				(TG_strcasecmp(new_max_value_str.c_str(),class_def_val.c_str()) == 0))
        {
            max_value_str_tmp = class_def_val;
        }
		else if (strlen(new_max_value_str.c_str()) == 0)
		{
		    if (user_defaults)
		    {
                max_value_str_tmp = usr_def_val;
		    }
		    else
		    {
		        set_value = false;

                avns_in_db("max_value",dev_name);
                avns_in_att(MAX_VALUE);
		    }
		}
    }
	else if(user_defaults)
	{
		if(TG_strcasecmp(new_max_value_str.c_str(),AlrmValueNotSpec) == 0)
		{
			set_value = false;

            avns_in_db("max_value",dev_name);
            avns_in_att(MAX_VALUE);
		}
		else if ((TG_strcasecmp(new_max_value_str.c_str(),NotANumber) == 0) ||
				(TG_strcasecmp(new_max_value_str.c_str(),usr_def_val.c_str()) == 0) ||
				(strlen(new_max_value_str.c_str()) == 0))
			max_value_str_tmp = usr_def_val;
	}
	else
	{
		if ((TG_strcasecmp(new_max_value_str.c_str(),AlrmValueNotSpec) == 0) ||
				(TG_strcasecmp(new_max_value_str.c_str(),NotANumber) == 0) ||
				(strlen(new_max_value_str.c_str()) == 0))
		{
			set_value = false;

            avns_in_db("max_value",dev_name);
            avns_in_att(MAX_VALUE);
		}
	}

	if(set_value)
	{
		if ((data_type != Tango::DEV_STRING) &&
		    (data_type != Tango::DEV_BOOLEAN) &&
		    (data_type != Tango::DEV_STATE) &&
			(data_type != Tango::DEV_ENUM))
		{
			double db;
			float fl;

			TangoSys_MemStream str;
			str.precision(TANGO_FLOAT_PRECISION);
			str << max_value_str_tmp;
			switch (data_type)
			{
			case Tango::DEV_SHORT:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				set_max_value((DevShort)db);
				break;

			case Tango::DEV_LONG:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				set_max_value((DevLong)db);
				break;

			case Tango::DEV_LONG64:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				set_max_value((DevLong64)db);
				break;

			case Tango::DEV_DOUBLE:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				set_max_value(db);
				break;

			case Tango::DEV_FLOAT:
				if (!(str >> fl && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				set_max_value(fl);
				break;

			case Tango::DEV_USHORT:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				(db < 0.0) ? set_max_value((DevUShort)(-db)) : set_max_value((DevUShort)db);
				break;

			case Tango::DEV_UCHAR:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				(db < 0.0) ? set_max_value((DevUChar)(-db)) : set_max_value((DevUChar)db);
				break;

			case Tango::DEV_ULONG:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				(db < 0.0) ? set_max_value((DevULong)(-db)) : set_max_value((DevULong)db);
				break;

			case Tango::DEV_ULONG64:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				(db < 0.0) ? set_max_value((DevULong64)(-db)) : set_max_value((DevULong64)db);
				break;

			case Tango::DEV_ENCODED:
				if (!(str >> db && str.eof()))
					throw_err_format("max_value",dev_name,"WAttribute::set_max_value()");
				(db < 0.0) ? set_max_value((DevUChar)(-db)) : set_max_value((DevUChar)db);
				break;
			}
		}
		else
			throw_err_data_type("max_value",dev_name,"WAttribute::set_max_value()");
	}
}

template<>
inline const Tango::DevShort*& WAttribute::get_write_value_ptr()
{
    return short_ptr;
}
template<>
inline Tango::DevVarShortArray& WAttribute::get_last_written_value()
{
    return short_array_val;
}
template<>
inline Tango::DevShort& WAttribute::get_write_value()
{
    return short_val;
}
template<>
inline Tango::DevShort& WAttribute::get_old_value()
{
    return old_short_val;
}
template<>
inline const Tango::DevUShort*& WAttribute::get_write_value_ptr()
{
    return ushort_ptr;
}
template<>
inline Tango::DevVarUShortArray& WAttribute::get_last_written_value()
{
    return ushort_array_val;
}
template<>
inline Tango::DevUShort& WAttribute::get_write_value()
{
    return ushort_val;
}
template<>
inline Tango::DevUShort& WAttribute::get_old_value()
{
    return old_ushort_val;
}
template<>
inline const Tango::DevLong*& WAttribute::get_write_value_ptr()
{
    return long_ptr;
}
template<>
inline Tango::DevVarLongArray& WAttribute::get_last_written_value()
{
    return long_array_val;
}
template<>
inline Tango::DevLong& WAttribute::get_write_value()
{
    return long_val;
}
template<>
inline Tango::DevLong& WAttribute::get_old_value()
{
    return old_long_val;
}
template<>
inline const Tango::DevULong*& WAttribute::get_write_value_ptr()
{
    return ulong_ptr;
}
template<>
inline Tango::DevVarULongArray& WAttribute::get_last_written_value()
{
    return ulong_array_val;
}
template<>
inline Tango::DevULong& WAttribute::get_write_value()
{
    return ulong_val;
}
template<>
inline Tango::DevULong& WAttribute::get_old_value()
{
    return old_ulong_val;
}
template<>
inline const Tango::DevLong64*& WAttribute::get_write_value_ptr()
{
    return long64_ptr;
}
template<>
inline Tango::DevVarLong64Array& WAttribute::get_last_written_value()
{
    return long64_array_val;
}
template<>
inline Tango::DevLong64& WAttribute::get_write_value()
{
    return long64_val;
}
template<>
inline Tango::DevLong64& WAttribute::get_old_value()
{
    return old_long64_val;
}
template<>
inline const Tango::DevULong64*& WAttribute::get_write_value_ptr()
{
    return ulong64_ptr;
}
template<>
inline Tango::DevVarULong64Array& WAttribute::get_last_written_value()
{
    return ulong64_array_val;
}
template<>
inline Tango::DevULong64& WAttribute::get_write_value()
{
    return ulong64_val;
}
template<>
inline Tango::DevULong64& WAttribute::get_old_value()
{
    return old_ulong64_val;
}
template<>
inline const Tango::DevDouble*& WAttribute::get_write_value_ptr()
{
    return double_ptr;
}
template<>
inline Tango::DevVarDoubleArray& WAttribute::get_last_written_value()
{
    return double_array_val;
}
template<>
inline Tango::DevDouble& WAttribute::get_write_value()
{
    return double_val;
}
template<>
inline Tango::DevDouble& WAttribute::get_old_value()
{
    return old_double_val;
}
template<>
inline const Tango::DevFloat*& WAttribute::get_write_value_ptr()
{
    return float_ptr;
}
template<>
inline Tango::DevVarFloatArray& WAttribute::get_last_written_value()
{
    return float_array_val;
}
template<>
inline Tango::DevFloat& WAttribute::get_write_value()
{
    return float_val;
}
template<>
inline Tango::DevFloat& WAttribute::get_old_value()
{
    return old_float_val;
}
template<>
inline const Tango::ConstDevString*& WAttribute::get_write_value_ptr()
{
    return str_ptr;
}
template<>
inline Tango::DevVarStringArray& WAttribute::get_last_written_value()
{
    return str_array_val;
}
template<>
inline Tango::DevString& WAttribute::get_write_value()
{
    return str_val;
}
template<>
inline Tango::DevString& WAttribute::get_old_value()
{
    return old_str_val;
}
template<>
inline const Tango::DevState*& WAttribute::get_write_value_ptr()
{
    return state_ptr;
}
template<>
inline Tango::DevVarStateArray& WAttribute::get_last_written_value()
{
    return state_array_val;
}
template<>
inline Tango::DevState& WAttribute::get_write_value()
{
    return dev_state_val;
}
template<>
inline Tango::DevState& WAttribute::get_old_value()
{
    return old_dev_state_val;
}
template<>
inline const Tango::DevBoolean*& WAttribute::get_write_value_ptr()
{
    return boolean_ptr;
}
template<>
inline Tango::DevVarBooleanArray& WAttribute::get_last_written_value()
{
    return boolean_array_val;
}
template<>
inline Tango::DevBoolean& WAttribute::get_write_value()
{
    return boolean_val;
}
template<>
inline Tango::DevBoolean& WAttribute::get_old_value()
{
    return old_boolean_val;
}
template<>
inline const Tango::DevEncoded*& WAttribute::get_write_value_ptr()
{
    return encoded_ptr;
}
template<>
inline Tango::DevEncoded& WAttribute::get_write_value()
{
    return encoded_val;
}
template<>
inline Tango::DevEncoded& WAttribute::get_old_value()
{
    return old_encoded_val;
}
template<>
inline const Tango::DevUChar*& WAttribute::get_write_value_ptr()
{
    return uchar_ptr;
}
template<>
inline Tango::DevVarCharArray& WAttribute::get_last_written_value()
{
    return uchar_array_val;
}
template<>
inline Tango::DevUChar& WAttribute::get_write_value()
{
    return uchar_val;
}
template<>
inline Tango::DevUChar& WAttribute::get_old_value()
{
    return old_uchar_val;
}

} // End of Tango namespace

#endif // _WATTRIBUTE_SPEC_TPP
