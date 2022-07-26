//+===================================================================================================================
//
// file :               w_attribute.tpp
//
// description :        C++ source code for the WAttribute class template methods
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
//-===================================================================================================================

#ifndef _WATTRIBUTE_TPP
#define _WATTRIBUTE_TPP

namespace Tango
{

namespace
{
    template<class T>
    const T& get_value(const AttrValUnion&);

    template<>
    inline const Tango::DevVarDoubleArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.double_att_value();
    }

    template<>
    inline const Tango::DevVarFloatArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.float_att_value();
    }

    template<>
    inline const Tango::DevVarLongArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.long_att_value();
    }

    template<>
    inline const Tango::DevVarULongArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.ulong_att_value();
    }

    template<>
    inline const Tango::DevVarLong64Array& get_value(const AttrValUnion& att_union)
    {
        return att_union.long64_att_value();
    }

    template<>
    inline const Tango::DevVarULong64Array& get_value(const AttrValUnion& att_union)
    {
        return att_union.ulong64_att_value();
    }

    template<>
    inline const Tango::DevVarShortArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.short_att_value();
    }

    template<>
    inline const Tango::DevVarUShortArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.ushort_att_value();
    }

    template<>
    inline const Tango::DevVarBooleanArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.bool_att_value();
    }

    template<>
    inline const Tango::DevVarCharArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.uchar_att_value();
    }

    template<>
    inline const Tango::DevVarStringArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.string_att_value();
    }

    template<>
    inline const Tango::DevVarStateArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.state_att_value();
    }

    template<>
    inline const Tango::DevVarEncodedArray& get_value(const AttrValUnion& att_union)
    {
        return att_union.encoded_att_value();
    }
    
    template<typename T
        , typename std::enable_if<!(std::is_same<T, Tango::DevDouble>::value || std::is_same<T, Tango::DevFloat>::value), T>::type* = nullptr>
    void check_nan(const std::string&, const T&, const size_t)
    {}
    
    template<typename T
        , typename std::enable_if<(std::is_same<T, Tango::DevDouble>::value || std::is_same<T, Tango::DevFloat>::value), T>::type* = nullptr>
    void check_nan(const std::string&, const T&, const size_t);

    template<typename T
        , typename std::enable_if<(std::is_same<T, Tango::DevDouble>::value || std::is_same<T, Tango::DevFloat>::value), T>::type*>
    void check_nan(const std::string& name, const T& val, const size_t i)
    {
        if (std::isfinite(val) == 0)
        {
            std::stringstream o;

            o << "Set value for attribute " << name
              << " is a NaN or INF value (at least element " << i << ")" << std::ends;

            TANGO_THROW_EXCEPTION(API_WAttrOutsideLimit, o.str());
        }
    }

    void _throw_incompatible_exception(long data_type)
    {
        std::stringstream o;

        o << "Incompatible attribute type, expected type is : " << data_type << " (even for single value)"
          << std::ends;
        Except::throw_exception((const char *) API_IncompatibleAttrDataType,
                                o.str(),
                                (const char *) "WAttribute::check_written_value()");
    }
    
    template<class T>
    bool _check_for_nan(Tango::Util* ) {return false;}
    
    template<>
    inline bool _check_for_nan<Tango::DevDouble>(Tango::Util* tg)
    {
        return !tg->is_wattr_nan_allowed();
    }

    template<>
    inline bool _check_for_nan<Tango::DevFloat>(Tango::Util* tg)
    {
        return !tg->is_wattr_nan_allowed();
    }


}
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
// 			- new_min_value : The minimum value property to be set
//
//-----------------------------------------------------------------------------------------------------------------

template <typename T>
void WAttribute::set_min_value(const T &new_min_value)
{

//
// Check type validity
//

	if((data_type == Tango::DEV_STRING) ||
		(data_type == Tango::DEV_BOOLEAN) ||
		(data_type == Tango::DEV_STATE))
		throw_err_data_type("min_value",d_name,"WAttribute::set_min_value()");

	if (!(data_type == DEV_ENCODED && ranges_type2const<T>::enu == DEV_UCHAR) &&
		(data_type != ranges_type2const<T>::enu))
	{
		std::string err_msg = "Attribute (" + name + ") data type does not match the type provided : " + ranges_type2const<T>::str;
		TANGO_THROW_EXCEPTION(API_IncompatibleAttrDataType, err_msg.c_str());
	}

//
//	Check coherence with max_value
//

	if(check_max_value)
	{
		T max_value_tmp;
		memcpy((void *) &max_value_tmp, (const void *) &max_value, sizeof(T));
		if(new_min_value >= max_value_tmp)
			throw_incoherent_val_err("min_value","max_value",d_name,"WAttribute::set_min_value()");
	}

//
// Store new min value as a string
//

	TangoSys_MemStream str;
	str.precision(TANGO_FLOAT_PRECISION);
	if(ranges_type2const<T>::enu == Tango::DEV_UCHAR)
		str << (short)new_min_value; // to represent the numeric value
	else
		str << new_min_value;
	std::string min_value_tmp_str = str.str();

//
// Get the monitor protecting device att config
// If the server is in its starting phase, give a NULL ptr to the AutoLock object
//

	Tango::Util *tg = Tango::Util::instance();
	Tango::TangoMonitor *mon_ptr = NULL;
	if (!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
		mon_ptr = &(get_att_device()->get_att_conf_monitor());
	AutoTangoMonitor sync1(mon_ptr);

//
// Store the new value locally
//

	Attr_CheckVal old_min_value;
	memcpy((void *)&old_min_value, (void *)&min_value, sizeof(T));
	memcpy((void *)&min_value, (void *)&new_min_value, sizeof(T));

//
// Then, update database
//

	Tango::DeviceClass *dev_class = get_att_device_class(d_name);
	Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
	Tango::Attr &att = mca->get_attr(name);
	std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
	size_t nb_user = def_user_prop.size();

	std::string usr_def_val;
	bool user_defaults = false;
	if (nb_user != 0)
	{
		size_t i;
		for (i = 0;i < nb_user;i++)
		{
			if (def_user_prop[i].get_name() == "min_value")
				break;
		}
		if (i != nb_user) // user defaults defined
		{
			user_defaults = true;
			usr_def_val = def_user_prop[i].get_value();
		}
	}


	if (Tango::Util::_UseDb == true)
	{
		if(user_defaults && min_value_tmp_str == usr_def_val)
		{
			DbDatum attr_dd(name), prop_dd("min_value");
			DbData db_data;
			db_data.push_back(attr_dd);
			db_data.push_back(prop_dd);

			bool retry = true;
			while (retry == true)
			{
				try
				{
					tg->get_database()->delete_device_attribute_property(d_name,db_data);
					retry = false;
				}
				catch (CORBA::COMM_FAILURE &)
				{
					tg->get_database()->reconnect(true);
				}
			}
		}
		else
		{
			try
			{
				upd_att_prop_db(min_value,"min_value");
			}
			catch (Tango::DevFailed &)
			{
				memcpy((void *)&min_value, (void *)&old_min_value, sizeof(T));
				throw;
			}
		}
	}

//
// Set the min_value flag
//

	check_min_value = true;

//
// Store new value as a string
//

	min_value_str = min_value_tmp_str;

//
// Push a att conf event
//

	if (!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
		get_att_device()->push_att_conf_event(this);

//
// Delete device startup exception related to min_value if there is any
//

	delete_startup_exception("min_value",d_name);
}

//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//		WAttribute::get_min_value()
//
// description :
//		Gets attribute's minimum value and assigns it to the variable provided as a parameter
//		Throws exception in case the data type of provided parameter does not match the attribute data type
//		or if minimum value is not defined
//
// args :
//		in :
// 			- min_val : The variable to be assigned the attribute's minimum value
//
//------------------------------------------------------------------------------------------------------------------

template <typename T>
void WAttribute::get_min_value(T &min_val)
{
	if (!(data_type == DEV_ENCODED && ranges_type2const<T>::enu == DEV_UCHAR) &&
		(data_type != ranges_type2const<T>::enu))
	{
		std::string err_msg = "Attribute (" + name + ") data type does not match the type provided : " + ranges_type2const<T>::str;
		TANGO_THROW_EXCEPTION(API_IncompatibleAttrDataType, err_msg.c_str());
	}

	if (check_min_value == false)
	{
		TANGO_THROW_EXCEPTION(API_AttrNotAllowed, "Minimum value not defined for this attribute");
	}

	memcpy((void *)&min_val,(void *)&min_value,sizeof(T));
}


//+-----------------------------------------------------------------------------------------------------------------
//
// method :
//		WAttribute::set_max_value()
//
// description :
//		Sets maximum value attribute property
//		Throws exception in case the data type of provided property does not match the attribute data type
//
// args :
//		in :
//			- new_max_value : The maximum value property to be set
//
//-------------------------------------------------------------------------------------------------------------------

template <typename T>
void WAttribute::set_max_value(const T &new_max_value)
{

//
// Check type validity
//

	if((data_type == Tango::DEV_STRING) ||
		(data_type == Tango::DEV_BOOLEAN) ||
		(data_type == Tango::DEV_STATE))
		throw_err_data_type("max_value",d_name,"WAttribute::set_max_value()");

	if (!(data_type == DEV_ENCODED && ranges_type2const<T>::enu == DEV_UCHAR) &&
		(data_type != ranges_type2const<T>::enu))
	{
		std::string err_msg = "Attribute (" + name + ") data type does not match the type provided : " + ranges_type2const<T>::str;
		TANGO_THROW_EXCEPTION(API_IncompatibleAttrDataType, err_msg.c_str());
	}

//
//	Check coherence with max_value
//

	if(check_min_value)
	{
		T min_value_tmp;
		memcpy((void *) &min_value_tmp, (const void *) &min_value, sizeof(T));
		if(new_max_value <= min_value_tmp)
			throw_incoherent_val_err("min_value","max_value",d_name,"WAttribute::set_max_value()");
	}

//
// Store new max value as a string
//

	TangoSys_MemStream str;
	str.precision(TANGO_FLOAT_PRECISION);
	if(ranges_type2const<T>::enu == Tango::DEV_UCHAR)
		str << (short)new_max_value; // to represent the numeric value
	else
		str << new_max_value;
	std::string max_value_tmp_str = str.str();

//
// Get the monitor protecting device att config
// If the server is in its starting phase, give a NULL ptr to the AutoLock object
//

	Tango::Util *tg = Tango::Util::instance();
	Tango::TangoMonitor *mon_ptr = NULL;
	if (!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
		mon_ptr = &(get_att_device()->get_att_conf_monitor());
	AutoTangoMonitor sync1(mon_ptr);

//
// Store the new value locally
//

	Attr_CheckVal old_max_value;
	memcpy((void *)&old_max_value, (void *)&max_value, sizeof(T));
	memcpy((void *)&max_value, (void *)&new_max_value, sizeof(T));

//
// Then, update database
//

	Tango::DeviceClass *dev_class = get_att_device_class(d_name);
	Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
	Tango::Attr &att = mca->get_attr(name);
	std::vector<AttrProperty> &def_user_prop = att.get_user_default_properties();
	size_t nb_user = def_user_prop.size();

	std::string usr_def_val;
	bool user_defaults = false;
	if (nb_user != 0)
	{
		size_t i;
		for (i = 0;i < nb_user;i++)
		{
			if (def_user_prop[i].get_name() == "max_value")
				break;
		}
		if (i != nb_user) // user defaults defined
		{
			user_defaults = true;
			usr_def_val = def_user_prop[i].get_value();
		}
	}


	if (Tango::Util::_UseDb == true)
	{
		if(user_defaults && max_value_tmp_str == usr_def_val)
		{
			DbDatum attr_dd(name), prop_dd("max_value");
			DbData db_data;
			db_data.push_back(attr_dd);
			db_data.push_back(prop_dd);

			bool retry = true;
			while (retry == true)
			{
				try
				{
					tg->get_database()->delete_device_attribute_property(d_name,db_data);
					retry = false;
				}
				catch (CORBA::COMM_FAILURE &)
				{
					tg->get_database()->reconnect(true);
				}
			}
		}
		else
		{
			try
			{
				upd_att_prop_db(max_value,"max_value");
			}
			catch (Tango::DevFailed &)
			{
				memcpy((void *)&max_value, (void *)&old_max_value, sizeof(T));
				throw;
			}
		}
	}

//
// Set the max_value flag
//

	check_max_value = true;

//
// Store new value as a string
//

	max_value_str = max_value_tmp_str;

//
// Push a att conf event
//

	if (!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
		get_att_device()->push_att_conf_event(this);

//
// Delete device startup exception related to max_value if there is any
//

	delete_startup_exception("max_value",d_name);
}

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//		WAttribute::get_max_value()
//
// description :
//		Gets attribute's maximum value and assigns it to the variable provided as a parameter
//		Throws exception in case the data type of provided parameter does not match the attribute data type
//		or if maximum value is not defined
//
// args :
//		in :
// 			- max_val : The variable to be assigned the attribute's maximum value
//
//------------------------------------------------------------------------------------------------------------------

template <typename T>
void WAttribute::get_max_value(T &max_val)
{
	if (!(data_type == DEV_ENCODED && ranges_type2const<T>::enu == DEV_UCHAR) &&
		(data_type != ranges_type2const<T>::enu))
	{
		std::string err_msg = "Attribute (" + name + ") data type does not match the type provided : " + ranges_type2const<T>::str;
		TANGO_THROW_EXCEPTION(API_IncompatibleAttrDataType, err_msg.c_str());
	}

	if (check_max_value == false)
	{
		TANGO_THROW_EXCEPTION(API_AttrNotAllowed, "Maximum value not defined for this attribute");
	}

	memcpy((void *)&max_val,(void *)&max_value,sizeof(T));
}


//+------------------------------------------------------------------------------------------------------------------
//
// method :
//		WAttribute::check_data_limits()
//
// description :
//		Check if the data received from client is valid.
//      It will check for nan value if needed, if itis not below the min (if one defined) or above the max
//      (if one defined), and for enum if it is in the accepted range.
//       This method throws exception in case of threshold violation.
//
// args :
//		in :
//          - nb_data : Data number
//          - seq : The received data
//          - min : The min allowed value
//          - max : The max allowed value
//
//------------------------------------------------------------------------------------------------------------------

template<class T>
void WAttribute::check_data_limits(const size_t nb_data
    , const typename tango_type_traits<T>::ArrayType& seq
    , Attr_CheckVal& min
    , Attr_CheckVal& max)
{

    const T& min_value = min.get_value<T>();
    const T& max_value = max.get_value<T>();
//
// If the server is in its starting phase, gives a NULL ptr
// to the AutoLock object
//

    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if (!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        mon_ptr = &(get_att_device()->get_att_conf_monitor());
    }

    AutoTangoMonitor sync1(mon_ptr);

    bool check_for_nan = _check_for_nan<T>(tg);

    if (check_for_nan || check_min_value || check_max_value)
    {
        for (size_t i = 0; i < nb_data; ++i)
        {
            if(check_for_nan)
            {
                check_nan(name, seq[i], i);
            }
            if(check_min_value)
            {
                if (seq[i] < min_value)
                {
                    std::stringstream o;

                    o << "Set value for attribute " << name
                      << " is below the minimum authorized (at least element " << i << ")" << std::ends;
                    TANGO_THROW_EXCEPTION(API_WAttrOutsideLimit, o.str());
                }
            }
            if(check_max_value)
            {
                if (seq[i] > max_value)
                {
                    std::stringstream o;

                    o << "Set value for attribute " << name
                      << " is above the maximum authorized (at least element " << i << ")" << std::ends;
                    TANGO_THROW_EXCEPTION(API_WAttrOutsideLimit, o.str());
                }
            }
        }
    }

    check_enum<T>(seq, nb_data);
}

template<>
void WAttribute::check_data_limits<Tango::DevEncoded>(const size_t nb_data
    , const Tango::DevVarEncodedArray& seq
    , Attr_CheckVal& min
    , Attr_CheckVal& max)
{
    const Tango::DevUChar& min_value = min.uch;
    const Tango::DevUChar& max_value = max.uch;
//
// If the server is in its starting phase, gives a NULL ptr
// to the AutoLock object
//

    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if (tg->is_svr_starting() == false && tg->is_device_restarting(d_name) == false)
    {
        mon_ptr = &(get_att_device()->get_att_conf_monitor());
    }

    AutoTangoMonitor sync1(mon_ptr);

    if (check_min_value || check_max_value)
    {
        for (size_t i = 0; i < nb_data; ++i)
        {
            size_t nb_data_elt = seq[i].encoded_data.length();
            for(size_t j=0; j < nb_data_elt; ++j)
            {
                if(check_min_value)
                {
                    if (seq[i].encoded_data[j] < min_value)
                    {
                        std::stringstream o;

                        o << "Set value for attribute " << name
                          << " is below the minimum authorized (at least element " << i << ")" << std::ends;
                        TANGO_THROW_EXCEPTION(API_WAttrOutsideLimit, o.str());
                    }
                }
                if(check_max_value)
                {
                    if (seq[i].encoded_data[j] > max_value)
                    {
                        std::stringstream o;

                        o << "Set value for attribute " << name
                          << " is above the maximum authorized (at least element " << i << ")" << std::ends;
                        TANGO_THROW_EXCEPTION(API_WAttrOutsideLimit, o.str());
                    }
                }
            }
        }
    }

}

template<>
void WAttribute::check_data_limits<Tango::DevBoolean>(const size_t
        , const Tango::DevVarBooleanArray&
        , Attr_CheckVal&
        , Attr_CheckVal&)
{
}

template<>
void WAttribute::check_data_limits<Tango::DevString>(const size_t
        , const Tango::DevVarStringArray&
        , Attr_CheckVal&
    , Attr_CheckVal&)
{
}

template<class T>
void WAttribute::_copy_data(const CORBA::Any& any)
{
    const typename tango_type_traits<T>::ArrayType* ptr;
    any >>= ptr;
    get_last_written_value<typename tango_type_traits<T>::ArrayType>() = *ptr;
}

template<class T>
void WAttribute::_copy_data(const Tango::AttrValUnion &the_union)
{
    get_last_written_value<typename tango_type_traits<T>::ArrayType>()= get_value<typename tango_type_traits<T>::ArrayType>(the_union);
}

//+-------------------------------------------------------------------------
//
// method : 		WAttribute::copy_data
//
// description : 	Copy data into the attribute object in order to return
//			them in case of a read on this attribute
//
// in :			any : Reference to the CORBA Any object
//
//--------------------------------------------------------------------------
template<class T>
void WAttribute::_copy_any_data(const T& data)
{
    switch (data_type)
    {
        case Tango::DEV_SHORT :
        case Tango::DEV_ENUM :
            _copy_data<Tango::DevShort>(data);
            break;

        case Tango::DEV_LONG :
            _copy_data<Tango::DevLong>(data);
            break;

        case Tango::DEV_LONG64 :
            _copy_data<Tango::DevLong64>(data);
            break;

        case Tango::DEV_DOUBLE :
            _copy_data<Tango::DevDouble>(data);
            break;

        case Tango::DEV_STRING :
            _copy_data<Tango::DevString>(data);
            break;

        case Tango::DEV_FLOAT :
            _copy_data<Tango::DevFloat>(data);
            break;

        case Tango::DEV_BOOLEAN :
            _copy_data<Tango::DevBoolean>(data);
            break;

        case Tango::DEV_USHORT :
            _copy_data<Tango::DevUShort>(data);
            break;

        case Tango::DEV_UCHAR :
            _copy_data<Tango::DevUChar>(data);
            break;

        case Tango::DEV_ULONG :
            _copy_data<Tango::DevULong>(data);
            break;

        case Tango::DEV_ULONG64 :
            _copy_data<Tango::DevULong64>(data);
            break;

        case Tango::DEV_STATE :
            _copy_data<Tango::DevState>(data);
            break;
        case Tango::DEV_ENCODED:
            // do nothing
            break;
        default:
            TANGO_THROW_ON_DEFAULT(data_type);
    }
}
template<class T>
void WAttribute::_update_value(const T& seq)
{
    get_old_value<typename get_type<typename T::Base_T_seq>::type>() = get_write_value<typename get_type<typename T::Base_T_seq>::type>();
    get_write_value<typename get_type<typename T::Base_T_seq>::type>() = seq[0];
}

template<>
void WAttribute::_update_value(const Tango::DevVarStringArray& seq)
{
    Tango::string_free(old_str_val);
    old_str_val = Tango::string_dup(str_val);
    Tango::string_free(str_val);
    
    str_val = Tango::string_dup(seq[0]);
}

template<class T>
void WAttribute::_update_any_written_value(const T& any, std::size_t x, std::size_t y)
{
    switch (data_type)
    {
        case Tango::DEV_SHORT :
        case Tango::DEV_ENUM :
            _update_written_value<Tango::DevShort>(any, x, y);
            break;

        case Tango::DEV_LONG :
            _update_written_value<Tango::DevLong>(any, x, y);
            break;

        case Tango::DEV_LONG64 :
            _update_written_value<Tango::DevLong64>(any, x, y);
            break;

        case Tango::DEV_DOUBLE :
            _update_written_value<Tango::DevDouble>(any, x, y);
            break;

        case Tango::DEV_STRING :
            _update_written_value<Tango::DevString>(any, x, y);
            break;

        case Tango::DEV_FLOAT :
            _update_written_value<Tango::DevFloat>(any, x, y);
            break;

        case Tango::DEV_USHORT :
            _update_written_value<Tango::DevUShort>(any, x, y);
            break;

        case Tango::DEV_UCHAR :
            _update_written_value<Tango::DevUChar>(any, x, y);
            break;

        case Tango::DEV_ULONG :
            _update_written_value<Tango::DevULong>(any, x, y);
            break;

        case Tango::DEV_ULONG64 :
            _update_written_value<Tango::DevULong64>(any, x, y);
            break;

        case Tango::DEV_STATE :
            _update_written_value<Tango::DevState>(any, x, y);
            break;

        case Tango::DEV_BOOLEAN :
            _update_written_value<Tango::DevBoolean>(any, x, y);
            break;

        case Tango::DEV_ENCODED :
            _update_written_value<Tango::DevEncoded>(any, x, y);
            break;
        
        default:
            TANGO_THROW_ON_DEFAULT(data_type);
    }
}

template<class T>
void WAttribute::_update_written_value(const CORBA::Any& any, std::size_t x, std::size_t y)
{
//
// Check data type inside the any and data number
//

    const typename tango_type_traits<T>::ArrayType *ptr;

    if ((any >>= ptr) == false)
    {
        _throw_incompatible_exception(data_type);
    }

    update_internal_sequence<T>(*ptr, x, y);
}


template<class T>
void WAttribute::_update_written_value(const Tango::AttrValUnion &att_union, std::size_t x, std::size_t y)
{
//
// Check data type inside the union
//
    if (att_union._d() != tango_type_traits<T>::att_type_value())
    {
        _throw_incompatible_exception(data_type);
    }
    
    const typename tango_type_traits<T>::ArrayType& seq = get_value<typename tango_type_traits<T>::ArrayType>(att_union);

    update_internal_sequence<T>(seq, x, y);
}

template<class T>
void WAttribute::update_internal_sequence(const typename tango_type_traits<T>::ArrayType& seq, size_t x, size_t y)
{
    size_t nb_data;

    nb_data = seq.length();
    check_length(nb_data, x, y);

//
// Check the incoming value
//

    check_data_limits<T>(nb_data, seq, min_value, max_value);

    using BufferType = decltype(seq.get_buffer());
    get_write_value_ptr<BufferType>() = seq.get_buffer();
    
    if (data_format == Tango::SCALAR)
    {
        _update_value(seq);
        w_dim_x = 1;
        w_dim_y = 0;
    }
    else
    {
        w_dim_x = x;
        w_dim_y = y;
    }
}

template<class T>
void WAttribute::check_enum(const typename tango_type_traits<T>::ArrayType&, const size_t)
{
}

template<>
void WAttribute::check_enum<Tango::DevShort>(const Tango::DevVarShortArray& seq, const size_t nb_data)
{
//
// If the attribute is enumerated, check the input value compared to the enum labels
//
    if (data_type == DEV_ENUM)
    {
        std::size_t max_val = enum_labels.size();
        for (std::size_t i = 0; i < nb_data; i++)
        {
            if (seq[i] < 0 || static_cast<std::size_t>(seq[i]) >= max_val)
            {
                std::stringstream o;
                o << "Set value for attribute " << name
                  << " is negative or above the maximun authorized (" << max_val << ") for at least element "
                  << i;

                TANGO_THROW_EXCEPTION(API_WAttrOutsideLimit, o.str());
            }
        }
    }

}

} // End of Tango namespace
#endif // _WATTRIBUTE_TPP
