
//+==================================================================================================================
// devapi_utils.cpp 	- C++ source code file for TANGO device api
//
// programmer(s)	- Emmanuel Taurel(taurel@esrf.fr)
//
// original 		- March 2014
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
// GNU General Lesser Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
// $Revision: 25126 $
//
//+==================================================================================================================


using namespace CORBA;

namespace Tango
{

    namespace
    {
        template<class T, class U, typename std::enable_if<std::is_same<T, U>::value, T>::type* = nullptr>
        CORBA::Any* create_any(const U*, const size_t, const size_t);
        template<class T, class U, typename std::enable_if<!std::is_same<T, U>::value, T>::type* = nullptr>
        CORBA::Any* create_any(const U*, const size_t, const size_t);

        template<class T, class U, typename std::enable_if<std::is_same<T, U>::value, T>::type*>
        CORBA::Any* create_any(const U* tmp, const size_t base, const size_t data_length)
        {
            CORBA::Any *any_ptr = new CORBA::Any();
            
            const auto* c_seq_buff = tmp->get_buffer();
            auto* seq_buff = const_cast<typename std::remove_const<typename std::remove_pointer<decltype(c_seq_buff)>::type>::type *>(c_seq_buff);

            T tmp_data = T(data_length, data_length, &(seq_buff[base - data_length]), false);
            
            (*any_ptr) <<= tmp_data;

            return any_ptr;
        }

        template<class T, class U, typename std::enable_if<!std::is_same<T, U>::value, T>::type*>
        CORBA::Any* create_any(const U* tmp, const size_t base, const size_t)
        {
            CORBA::Any *any_ptr = new CORBA::Any();
            (*any_ptr) <<= (*tmp)[base - 1];
            return any_ptr;
        }
        
        template<>
        CORBA::Any* create_any<DevBoolean>(const DevVarBooleanArray* tmp, const size_t base, const size_t)
        {
            CORBA::Any *any_ptr = new CORBA::Any();
            (*any_ptr) <<= CORBA::Any::from_boolean((*tmp)[base - 1]);
            return any_ptr;
        }

        template<>
        CORBA::Any* create_any<DevString>(const DevVarStringArray* tmp, const size_t base, const size_t)
        {
            CORBA::Any *any_ptr = new CORBA::Any();
            Tango::ConstDevString tmp_data = (*tmp)[base - 1].in();
            (*any_ptr) <<= tmp_data;
            return any_ptr;
        }

        template<>
        CORBA::Any* create_any<DevVarStringArray>(const DevVarStringArray* tmp, const size_t base, const size_t data_length)
        {
            CORBA::Any *any_ptr = new CORBA::Any();
            const Tango::ConstDevString *c_seq_buff = tmp->get_buffer();
            char **seq_buff = const_cast<char **>(c_seq_buff);
            Tango::DevVarStringArray tmp_data = DevVarStringArray(data_length,data_length,&(seq_buff[base - data_length]),false);
            
            (*any_ptr) <<= tmp_data;
            return any_ptr;
        }
    }

//------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::from_hist_2_AttHistory()
//
// description :
// 		Convert the attribute history as returned by a IDL 4 device to the classical DeviceAttributeHistory format
//
//-------------------------------------------------------------------------------------------------------------------

template <typename T>
void DeviceProxy::from_hist_2_AttHistory(const T &hist,std::vector<DeviceAttributeHistory> *ddh)
{

//
// Check received data validity
//

	if ((hist->quals.length() != hist->quals_array.length()) ||
		(hist->r_dims.length() != hist->r_dims_array.length()) ||
		(hist->w_dims.length() != hist->w_dims_array.length()) ||
		(hist->errors.length() != hist->errors_array.length()))
	{
		TANGO_THROW_EXCEPTION(API_WrongHistoryDataBuffer, "Data buffer received from server is not valid !");
	}

//
// Get history depth
//

	unsigned int h_depth = hist->dates.length();

//
// Copy date and name in each history list element
//

	unsigned int loop;
	for (loop = 0;loop < h_depth;loop++)
	{
		(*ddh)[loop].time = hist->dates[loop];
		(*ddh)[loop].name = hist->name.in();
	}

//
// Copy the attribute quality factor
//

	int k;

	for (loop = 0;loop < hist->quals.length();loop++)
	{
		int nb_elt = hist->quals_array[loop].nb_elt;
		int start = hist->quals_array[loop].start;

		for (k = 0;k < nb_elt;k++)
			(*ddh)[start - k].quality = hist->quals[loop];
	}

//
// Copy read dimension
//

	for (loop = 0;loop < hist->r_dims.length();loop++)
	{
		int nb_elt = hist->r_dims_array[loop].nb_elt;
		int start = hist->r_dims_array[loop].start;

		for (k = 0;k < nb_elt;k++)
		{
			(*ddh)[start - k].dim_x = hist->r_dims[loop].dim_x;
			(*ddh)[start - k].dim_y = hist->r_dims[loop].dim_y;
		}
	}

//
// Copy write dimension
//

	for (loop = 0;loop < hist->w_dims.length();loop++)
	{
		int nb_elt = hist->w_dims_array[loop].nb_elt;
		int start = hist->w_dims_array[loop].start;

		for (k = 0;k < nb_elt;k++)
		{
			(*ddh)[start - k].set_w_dim_x(hist->w_dims[loop].dim_x);
			(*ddh)[start - k].set_w_dim_y(hist->w_dims[loop].dim_y);
		}
	}

//
// Copy errors
//

	for (loop = 0;loop < hist->errors.length();loop++)
	{
		int nb_elt = hist->errors_array[loop].nb_elt;
		int start = hist->errors_array[loop].start;

		for (k = 0;k < nb_elt;k++)
		{
			(*ddh)[start - k].failed(true);
			DevErrorList &err_list = (*ddh)[start - k].get_error_list();
			err_list.length(hist->errors[loop].length());
			for (unsigned int g = 0;g < hist->errors[loop].length();g++)
			{
				err_list[g] = (hist->errors[loop])[g];
			}
		}
	}

//
// Get data type and data ptr
//

	CORBA::TypeCode_var ty = hist->value.type();
	if (ty->kind() != tk_null)
	{
		CORBA::TypeCode_var ty_alias = ty->content_type();
		CORBA::TypeCode_var ty_seq = ty_alias->content_type();

		switch (ty_seq->kind())
		{
			case tk_long:
                        extract_value<Tango::DevVarLongArray>(hist->value, *ddh);
			break;

			case tk_longlong:
                        extract_value<Tango::DevVarLong64Array>(hist->value, *ddh);
			break;

			case tk_short:
                        extract_value<Tango::DevVarShortArray>(hist->value, *ddh);
			break;

			case tk_double:
                        extract_value<Tango::DevVarDoubleArray>(hist->value, *ddh);
			break;

			case tk_string:
                        extract_value<Tango::DevVarStringArray>(hist->value, *ddh);
			break;

			case tk_float:
                        extract_value<Tango::DevVarFloatArray>(hist->value, *ddh);
			break;

			case tk_boolean:
                        extract_value<Tango::DevVarBooleanArray>(hist->value, *ddh);
			break;

			case tk_ushort:
                        extract_value<Tango::DevVarUShortArray>(hist->value, *ddh);
			break;

			case tk_octet:
                        extract_value<Tango::DevVarCharArray>(hist->value, *ddh);
			break;

			case tk_ulong:
                        extract_value<Tango::DevVarULongArray>(hist->value, *ddh);
			break;

			case tk_ulonglong:
                        extract_value<Tango::DevVarULong64Array>(hist->value, *ddh);
			break;

			case tk_enum:
                        extract_value<Tango::DevVarStateArray>(hist->value, *ddh);
			break;

			case tk_struct:
                        extract_value<Tango::DevVarEncodedArray>(hist->value, *ddh);
			break;

			default:
			break;
		}
	}
}

template<class T>
void DeviceProxy::extract_value(CORBA::Any& value, std::vector<DeviceAttributeHistory>& ddh)
{
    const T* tmp;
    
    value >>= tmp;
    size_t seq_size = tmp->length();

//
// Copy data
//

    size_t base = seq_size;

    for (auto& hist : ddh)
    {

        if (hist.failed() || hist.quality == Tango::ATTR_INVALID)
            continue;

//
// Get the data length for this record
//

        int r_dim_x = hist.dim_x;
        int r_dim_y = hist.dim_y;
        int w_dim_x = hist.get_written_dim_x();
        int w_dim_y = hist.get_written_dim_y();

        int data_length;
        (r_dim_y == 0) ? data_length = r_dim_x : data_length = r_dim_x * r_dim_y;
        (w_dim_y == 0) ? data_length += w_dim_x : data_length += (w_dim_x * w_dim_y);

//
// Real copy now
//

        hist.update_internal_sequence(tmp, base - data_length, data_length);
        
        base -= data_length;
    }
}

template<class T>
void DeviceProxy::extract_value(CORBA::Any& value, std::vector<DeviceDataHistory>& ddh, const Tango::AttributeDimList& ad)
{
    const typename tango_type_traits<T>::ArrayType* tmp;
    
    value >>= tmp;
    size_t seq_size = tmp->length();

//
// Copy data
//


    size_t base = seq_size;

    size_t loop = 0;

    for (auto& hist : ddh)
    {

//
// Get the data length for this record
//

        size_t data_length = ad[loop++].dim_x;

        if (hist.failed())
            continue;

//
// Real copy now
//
        CORBA::Any *any_ptr = create_any<T>(tmp, base, data_length);
        
        hist.any = any_ptr;

        base -= data_length;

    }
}

template<>
void DeviceProxy::extract_value<Tango::DevVarDoubleStringArray>(CORBA::Any& value, std::vector<DeviceDataHistory>& ddh, const Tango::AttributeDimList& ad)
{
    const Tango::DevVarDoubleStringArray *tmp;
    
    value >>= tmp;
    size_t seq_size_str = tmp->svalue.length();
    size_t seq_size_num = tmp->dvalue.length();

//
// Copy data
//

    size_t base_str = seq_size_str;
    size_t base_num = seq_size_num;

    size_t loop = 0;

    for (auto& hist : ddh)
    {

        if (hist.failed())
            continue;

//
// Get the data length for this record
//

        size_t data_length = ad[loop].dim_x;
        size_t data_num_length = ad[loop].dim_y;
        ++loop;
//
// Real copy now
//
        Tango::DevVarDoubleStringArray* dvdsa = new Tango::DevVarDoubleStringArray();
        dvdsa->svalue.length(data_length);
        dvdsa->dvalue.length(data_num_length);

        for (size_t i = 0; i < data_length; ++i)
       	    dvdsa->svalue[i] = tmp->svalue[(base_str - data_length) + i];
        for (size_t i = 0; i < data_num_length; ++i)
            dvdsa->dvalue[i] = tmp->dvalue[(base_num - data_num_length) + i];

        CORBA::Any *any_ptr = new CORBA::Any();
        (*any_ptr) <<= dvdsa;
        hist.any = any_ptr;

        base_str -=  data_length;
        base_num -=  data_num_length;
    }
}

template<>
void DeviceProxy::extract_value<Tango::DevVarLongStringArray>(CORBA::Any& value, std::vector<DeviceDataHistory>& ddh, const Tango::AttributeDimList& ad)
{
    const Tango::DevVarLongStringArray *tmp;
    
    value >>= tmp;
    size_t seq_size_str = tmp->svalue.length();
    size_t seq_size_num = tmp->lvalue.length();

//
// Copy data
//

    size_t base_str = seq_size_str;
    size_t base_num = seq_size_num;

    size_t loop = 0;

    for (auto& hist : ddh)
    {

        if (hist.failed())
            continue;

//
// Get the data length for this record
//

        size_t data_length = ad[loop].dim_x;
        size_t data_num_length = ad[loop].dim_y;
        ++loop;
//
// Real copy now
//
        Tango::DevVarLongStringArray* dvdsa = new Tango::DevVarLongStringArray();
        dvdsa->svalue.length(data_length);
        dvdsa->lvalue.length(data_num_length);

        for (size_t i = 0; i < data_length; ++i)
       	    dvdsa->svalue[i] = tmp->svalue[(base_str - data_length) + i];
        for (size_t i = 0; i < data_num_length;++i)
            dvdsa->lvalue[i] = tmp->lvalue[(base_num - data_num_length) + i];

        CORBA::Any *any_ptr = new CORBA::Any();
        (*any_ptr) <<= dvdsa;
        hist.any = any_ptr;

        base_str -=  data_length;
        base_num -=  data_num_length;
    }
}

} // End of namespace
