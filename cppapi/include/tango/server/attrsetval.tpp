//+===================================================================================================================
//
// file :               attsetval.tpp
//
// description :        C++ source code for the Attribute class template methods when they are not specialized and
//						related to attribute value setting
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2014,2015
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

#ifndef _ATTRSETVAL_TPP
#define _ATTRSETVAL_TPP

#include <type_traits>

namespace Tango
{

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//		Attribute::set_value
//
// description :
//		Set the attribute read value and quality. This method automatically set the date when it has been called
//
//		This method is overloaded several times for all the supported attribute data type. Nevertheless, one
//		template method is defined (this one) which will be called for all data types with no overload.
//		This is the case for enumeration data type
//
// argument :
// 		in :
//			- enum_ptr : The attribute read value
//			- x : The attribute x dimension (default is 1)
//			- y : The atttribute y dimension (default is 0)
//			- release : A flag set to true if memory must be de-allocated (default is false)
//
//-------------------------------------------------------------------------------------------------------------------

template <class T, typename std::enable_if<std::is_enum<T>::value && !std::is_same<T, Tango::DevState>::value, T>::type*>
inline void Attribute::set_value(T *enum_ptr,long x,long y,bool release)
{
//
// Throw exception if attribute data type is not correct
//

	if (data_type != Tango::DEV_ENUM)
	{
		delete_data_if_needed(enum_ptr,release);

		std::stringstream o;
		o << "Invalid data type for attribute " << name
                    <<  ". Expected: " << Tango::DEV_ENUM 
                    << " got " << (CmdArgType)data_type << std::ends;

		TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

	bool short_enum = std::is_same<short,typename std::underlying_type<T>::type>::value;
	bool uns_int_enum = std::is_same<unsigned int,typename std::underlying_type<T>::type>::value;

	if (short_enum == false && uns_int_enum == false)
	{
		delete_data_if_needed(enum_ptr,release);

		std::stringstream o;
		o << "Invalid enumeration type. Supported types are C++11 scoped enum with short as underlying data type or old enum."
                  << std::endl;

		TANGO_THROW_EXCEPTION(API_IncompatibleArgumentType, o.str());
	}

//
// Check if the input type is an enum and if it is from the valid type
//

	if (std::is_enum<T>::value == false)
	{
		delete_data_if_needed(enum_ptr,release);
		TANGO_THROW_EXCEPTION(API_IncompatibleArgumentType, "The input argument data type is not an enumeration");
	}

//
// Check if enum labels are defined
//

	if (enum_labels.size() == 0)
	{
		delete_data_if_needed(enum_ptr,release);

		std::stringstream o;
		o << "Attribute " << name << " data type is enum but no enum labels are defined!";

		TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

//
// Check enum type
//

	DeviceImpl *dev = get_att_device();
	Tango::DeviceClass *dev_class = dev->get_device_class();
	Tango::MultiClassAttribute *mca = dev_class->get_class_attr();
	Tango::Attr &att = mca->get_attr(name);

	if (att.same_type(typeid(T)) == false)
	{
		delete_data_if_needed(enum_ptr,release);

		std::stringstream o;
		o << "Invalid enumeration type. Requested enum type is " << att.get_enum_type();
		TANGO_THROW_EXCEPTION(API_IncompatibleArgumentType, o.str());
	}

//
// Check that data size is less than the given max
//

	if ((x > max_x) || (y > max_y))
	{
		delete_data_if_needed(enum_ptr,release);

		std::stringstream o;
		
                o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
                  << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

		TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

//
// Compute data size and set default quality to valid.
//

	dim_x = x;
	dim_y = y;
	set_data_size();
	quality = Tango::ATTR_VALID;

//
// Throw exception if pointer is null and data_size != 0
//

	if (data_size != 0)
	{
		CHECK_PTR(enum_ptr,name);
	}

//
// If the data is wanted from the DevState command, store it in a sequence. If the attribute  has an associated
// writable attribute, store data in a temporary buffer (the write value must be added before the data is sent
// back to the caller)
//

	if (data_size > enum_nb)
	{
		if (enum_nb != 0)
			delete [] loc_enum_ptr;
		loc_enum_ptr = new short [data_size];
		enum_nb = data_size;
	}

	short max_val = (short)enum_labels.size() - 1;
	for (std::uint32_t i = 0;i < data_size;i++)
	{
		loc_enum_ptr[i] = (short)enum_ptr[i];
		if (loc_enum_ptr[i] < 0 || loc_enum_ptr[i] > max_val)
		{
			delete_data_if_needed(enum_ptr,release);
			enum_nb = 0;

			std::stringstream o;
			o << "Wrong value for attribute " << name
			  << ". Element " << i << " (value = " << loc_enum_ptr[i] 
                          << ") is negative or above the limit defined by the enum (" << max_val << ").";

                        delete [] loc_enum_ptr;

			TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
		}
	}

	delete_data_if_needed(enum_ptr,release);

	if (date == false)
	{
		value.sh_seq = new Tango::DevVarShortArray(data_size,data_size,loc_enum_ptr,false);
	}
	else
	{
		if ((is_writ_associated() == true))
		{
			if (data_format == Tango::SCALAR)
			{
				tmp_sh[0] = *loc_enum_ptr;
			}
			else
			{
				value.sh_seq = new Tango::DevVarShortArray(data_size);
				value.sh_seq->length(data_size);
				::memcpy(value.sh_seq->get_buffer(false),loc_enum_ptr,data_size * sizeof(Tango::DevShort));
			}
		}
		else
		{
			if ((data_format == Tango::SCALAR) && (release == true))
			{
				value.sh_seq = new Tango::DevVarShortArray(data_size,data_size,loc_enum_ptr,false);
			}
			else
				value.sh_seq = new Tango::DevVarShortArray(data_size,data_size,loc_enum_ptr,release);
		}
	}
	value_flag = true;

//
// Reset alarm flags
//

	alarm.reset();

//
// Get time
//

	set_time();
}

template <class T, typename std::enable_if<!std::is_enum<T>::value || std::is_same<T, Tango::DevState>::value, T>::type*>
inline void Attribute::set_value(T* p_data, long x, long y, bool release)
{
    using ArrayType = typename tango_type_traits<T>::ArrayType;

//
// Throw exception if type is not correct
//

	if (data_type != Tango::tango_type_traits<T>::type_value())
	{
		delete_data_if_needed(p_data,release);

		std::stringstream o;

		o << "Invalid data type for attribute " << name
                    <<  ". Expected: " << Tango::tango_type_traits<T>::type_value()
                    << " got " << (CmdArgType)data_type << std::ends;

                TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

//
// Check that data size is less than the given max
//

	if ((x > max_x) || (y > max_y))
	{
		delete_data_if_needed(p_data,release);

		std::stringstream o;

                o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
                  << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

                TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}


//
// Compute data size and set default quality to valid.
//

	dim_x = x;
	dim_y = y;
	set_data_size();
	quality = Tango::ATTR_VALID;

//
// Throw exception if pointer is null and data_size != 0
//

	if (data_size != 0)
	{
		CHECK_PTR(p_data,name);
	}

//
// If the data is wanted from the DevState command, store it in a sequence.
// If the attribute  has an associated writable attribute, store data in a
// temporary buffer (the write value must be added before the data is sent
// back to the caller)
//

	if (date == false)
	{
                auto* tmp = get_value_storage<ArrayType>();
		*tmp = new ArrayType(data_size,data_size,p_data,release);
	}
	else
	{
		if ((is_writ_associated() == true))
		{
			if (data_format == Tango::SCALAR)
			{
				get_tmp_storage<T>()[0] = *p_data;
		                delete_data_if_needed(p_data,release);
			}
			else
			{
                                auto* tmp = get_value_storage<ArrayType>();
		                *tmp = new ArrayType(data_size);
				(*tmp)->length(data_size);
				::memcpy((*tmp)->get_buffer(false),p_data,data_size * sizeof(T));
				if (release == true)
					delete [] p_data;
			}
		}
		else
		{
			if ((data_format == Tango::SCALAR) && (release == true))
			{
				T *tmp_ptr = new T[1];
				*tmp_ptr = *p_data;
                                auto* tmp = get_value_storage<ArrayType>();
				*tmp = new ArrayType(data_size,data_size,tmp_ptr,release);
				if (is_fwd_att() == true)
					delete [] p_data;
				else
					delete p_data;
			}
			else
                        {
                                auto* tmp = get_value_storage<ArrayType>();
				*tmp = new ArrayType(data_size,data_size,p_data,release);
                        }
		}
	}
	value_flag = true;

//
// Reset alarm flags
//

	alarm.reset();

//
// Get time
//

	set_time();
}

template <class T>
inline void Attribute::set_value_date_quality(T *p_data,time_t t,Tango::AttrQuality qual,long x,long y,bool release)
{
	set_value(p_data,x,y,release);
	set_quality(qual,false);
	set_date(t);

	if (qual == Tango::ATTR_INVALID)
	{
		if (!((is_writ_associated() == true) && (data_format == Tango::SCALAR)))
			delete_seq();
	}
}

template <class T>
inline void Attribute::set_value_date_quality(T *p_data,const TangoTimestamp &t,Tango::AttrQuality qual,long x,long y,bool release)
{
	set_value(p_data,x,y,release);
	set_quality(qual,false);
	set_date(t);

	if (qual == Tango::ATTR_INVALID)
	{
		if (!((is_writ_associated() == true) && (data_format == Tango::SCALAR)))
			delete_seq();
	}
}

//+-------------------------------------------------------------------------
//
// method : 		Attribute::set_value
//
// description : 	Set the attribute read value and quality. This method
//			automatically set the date when it has been called
//			This method is overloaded several times for all the
//			supported attribute data type
//
// in :			p_data : The attribute read value
//			x : The attribute x dimension (default is 1)
//			y : The atttribute y dimension (default is 0)
//			release : A flag set to true if memory must be
//				  de-allocated (default is false)
//
//--------------------------------------------------------------------------
template<>
inline void Attribute::set_value(Tango::DevShort *p_data,long x,long y,bool release)
{

//
// Throw exception if type is not correct
//

	if (data_type != Tango::DEV_SHORT &&
		data_type != Tango::DEV_ENUM)
	{
                delete_data_if_needed(p_data,release);

		std::stringstream ss;
		ss << "Invalid data type for attribute " << name;

		TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
	}

//
// Check that data size is less than the given max
//

	if ((x > max_x) || (y > max_y))
	{
                delete_data_if_needed(p_data,release);

		std::stringstream ss;
		ss << "Data size for attribute " << name << " exceeds given limit";

		TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
	}

//
// Compute data size and set default quality to valid.
//

	dim_x = x;
	dim_y = y;
	set_data_size();
	quality = Tango::ATTR_VALID;

//
// Throw exception if pointer is null and data_size != 0
//

	if (data_size != 0)
	{
		CHECK_PTR(p_data,name);
	}

//
// For DevEnum, check that the enum labels are defined. Also check the enum value
//

	if (data_type == DEV_ENUM)
	{
		if (enum_labels.size() == 0)
		{
                        delete_data_if_needed(p_data,release);

			std::stringstream ss;
			ss << "Attribute " << name << " data type is enum but no enum labels are defined!";

			TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
		}

		int max_val = enum_labels.size() - 1;
		for (std::uint32_t i = 0;i < data_size;i++)
		{
			if (p_data[i] < 0 || p_data[i] > max_val)
			{
                                delete_data_if_needed(p_data,release);

				std::stringstream ss;
				ss << "Wrong value for attribute " << name;
				ss << ". Element " << i << " (value = " << p_data[i] << ") is negative or above the limit defined by the enum (" << max_val << ").";

				TANGO_THROW_EXCEPTION(API_AttrOptProp, ss.str());
			}
		}
	}

//
// If the data is wanted from the DevState command, store it in a sequence.
// If the attribute  has an associated writable attribute, store data in a
// temporary buffer (the write value must be added before the data is sent
// back to the caller)
//

	if (date == false)
	{
		value.sh_seq = new Tango::DevVarShortArray(data_size,data_size,p_data,release);
	}
	else
	{
		if ((is_writ_associated() == true))
		{
			if (data_format == Tango::SCALAR)
			{
				tmp_sh[0] = *p_data;
                                delete_data_if_needed(p_data,release);
			}
			else
			{
				value.sh_seq = new Tango::DevVarShortArray(data_size);
				value.sh_seq->length(data_size);
				::memcpy(value.sh_seq->get_buffer(false),p_data,data_size * sizeof(Tango::DevShort));
				if (release == true)
					delete [] p_data;
			}
		}
		else
		{
			if ((data_format == Tango::SCALAR) && (release == true))
			{
				Tango::DevShort *tmp_ptr = new Tango::DevShort[1];
				*tmp_ptr = *p_data;
				value.sh_seq = new Tango::DevVarShortArray(data_size,data_size,tmp_ptr,release);
				if (is_fwd_att() == true)
					delete [] p_data;
				else
					delete p_data;
			}
			else
				value.sh_seq = new Tango::DevVarShortArray(data_size,data_size,p_data,release);
		}
	}
	value_flag = true;

//
// Reset alarm flags
//

	alarm.reset();

//
// Get time
//

	set_time();
}

template<>
inline void Attribute::set_value(Tango::DevString *p_data,long x, long y,bool release)
{

//
// Throw exception if type is not correct
//

	if (data_type != Tango::DEV_STRING)
	{
                delete_data_if_needed(p_data,release);
		
                std::stringstream o;

		o << "Invalid data type for attribute " << name
                    <<  ". Expected: " << Tango::DEV_STRING 
                    << " got " << (CmdArgType)data_type << std::ends;

                TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

//
// Check that data size is less than the given max
//

	if ((x > max_x) || (y > max_y))
	{
                delete_data_if_needed(p_data,release);

		TangoSys_OMemStream o;

                o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
                  << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

                TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}


//
// Compute data size and set default quality to valid.
//

	dim_x = x;
	dim_y = y;
	set_data_size();
	quality = Tango::ATTR_VALID;

//
// Throw exception if pointer is null and data size != 0
//

	if (data_size != 0)
	{
		CHECK_PTR(p_data,name);
	}

//
// If the data is wanted from the DevState command, store it in a sequence.
// If the attribute  has an associated writable attribute, store data in a
// temporary buffer (the write value must be added before the data is sent
// back to the caller)
//

	if (date == false)
	{
		if (release == true)
		{
			char **strvec = Tango::DevVarStringArray::allocbuf(data_size);
			for (std::uint32_t i = 0;i < data_size;i++)
				strvec[i] = p_data[i];
			value.str_seq = new Tango::DevVarStringArray(data_size,data_size,strvec,release);
			delete [] p_data;
		}
		else
			value.str_seq = new Tango::DevVarStringArray(data_size,data_size,p_data,release);
	}
	else
	{
		if ((is_writ_associated() == true))
		{
			if (data_format == Tango::SCALAR)
			{
				tmp_str[0] = *p_data;
                                delete_data_if_needed(p_data,release);
				scalar_str_attr_release = release;
			}
			else
			{
				value.str_seq = new Tango::DevVarStringArray(data_size);
				value.str_seq->length(data_size);
				for (std::uint32_t k = 0;k < data_size;k++)
					(*value.str_seq)[k] = Tango::string_dup(p_data[k]);
				if (release == true)
				{
					if (is_fwd_att() == true)
						Tango::DevVarStringArray::freebuf(p_data);
					else
                                        {
                                            for (std::uint32_t k = 0;k < data_size;k++)
                                                delete [] p_data[k];
                                            delete [] p_data;
                                        }
				}
			}
		}
		else
		{
			if (release == true)
			{
				char **strvec = Tango::DevVarStringArray::allocbuf(data_size);
				if (is_fwd_att() == true)
				{
					for (std::uint32_t i = 0;i < data_size;i++)
						strvec[i] = Tango::string_dup(p_data[i]);
				}
				else
				{
					for (std::uint32_t i = 0;i < data_size;i++)
						strvec[i] = p_data[i];
				}
				value.str_seq = new Tango::DevVarStringArray(data_size,data_size,strvec,release);
				if (data_format == Tango::SCALAR)
				{
					if (is_fwd_att() == true)
						Tango::DevVarStringArray::freebuf(p_data);
					else
						delete p_data;
				}
				else
					delete [] p_data;
			}
			else
				value.str_seq = new Tango::DevVarStringArray(data_size,data_size,p_data,release);
		}
	}
	value_flag = true;

//
// Get time
//

	set_time();
}

template<>
inline void Attribute::set_value(Tango::DevEncoded *p_data,long x, long y,bool release)
{

//
// Throw exception if type is not correct
//

	if (data_type != Tango::DEV_ENCODED)
	{
                delete_data_if_needed(p_data,release);

		std::stringstream o;

		o << "Invalid data type for attribute " << name
                    <<  ". Expected: " << Tango::DEV_ENCODED 
                    << " got " << (CmdArgType)data_type << std::ends;

                TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

//
// Check that data size is less than the given max
//

	if ((x > max_x) || (y > max_y))
	{
                delete_data_if_needed(p_data,release);

		TangoSys_OMemStream o;

                o << "Data size for attribute " << name << " [" << x << ", " << y << "]"
                  << " exceeds given limit [" << max_x << ", " << max_y << "]" << std::ends;

                TANGO_THROW_EXCEPTION(API_AttrOptProp, o.str());
	}

//
// Compute data size and set default quality to valid.
//

	dim_x = x;
	dim_y = y;
	set_data_size();
	quality = Tango::ATTR_VALID;

//
// Throw exception if pointer is null and data size != 0
//

	if (data_size != 0)
	{
		CHECK_PTR(p_data,name);
	}

//
// If the data is wanted from the DevState command, store it in a sequence.
// If the attribute  has an associated writable attribute, store data in a
// temporary buffer (the write value must be added before the data is sent
// back to the caller)
//

	if (date == false)
	{
		value.enc_seq = new Tango::DevVarEncodedArray(data_size,data_size,p_data,release);
	}
	else
	{
		if ((is_writ_associated() == true))
		{
			tmp_enc[0] = *p_data;
                        delete_data_if_needed(p_data,release);
		}
		else
		{
			if (release == true)
			{
				DevEncoded *tmp_ptr = new DevEncoded[1];

				tmp_ptr->encoded_format = p_data->encoded_format;

				unsigned long nb_data = p_data->encoded_data.length();
				tmp_ptr->encoded_data.replace(nb_data,nb_data,p_data->encoded_data.get_buffer(true),true);
				p_data->encoded_data.replace(0,0,NULL,false);

				value.enc_seq = new Tango::DevVarEncodedArray(data_size,data_size,tmp_ptr,true);
				delete p_data;
			}
			else
				value.enc_seq = new Tango::DevVarEncodedArray(data_size,data_size,p_data,release);
		}
	}
	value_flag = true;

//
// Reset alarm flags
//

	alarm.reset();

//
// Get time
//

	set_time();
}
template<>
inline void Attribute::set_value_date_quality(Tango::DevEncoded *p_data,time_t t,
				    Tango::AttrQuality qual,
				    long x,long y,bool release)
{
	set_value(p_data,x,y,release);
	set_quality(qual,false);
	set_date(t);
}

template<>
inline void Attribute::set_value_date_quality(Tango::DevEncoded *p_data,const TangoTimestamp &t,
				    Tango::AttrQuality qual,
				    long x,long y,bool release)
{
	set_value(p_data,x,y,release);
	set_quality(qual,false);
	set_date(t);
}

} // End of Tango namespace
#endif // _ATTRSETVAL_TPP
