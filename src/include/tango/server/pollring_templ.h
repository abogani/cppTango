//+==================================================================================================================
//
// file :               PollRing_templ.h
//
// description :        C++ source code for the PollRing class template methods
//
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2014,2015
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

#ifndef _POLLRING_TPP
#define _POLLRING_TPP

namespace Tango
{

template <typename T>
void PollRing::get_attr_history(long n, T *ptr, long type)
{
    long i;

    //
    // Set index to read ring and to initialised returned sequence
    // In the returned sequence , indice 0 is the oldest data
    //

    long index = insert_elt;
    if(index == 0)
    {
        index = max_elt;
    }
    index--;

    long seq_index = n - 1;

    //
    // Compute the size of the global sequence and the error numbers
    //

    long seq_size = 0;
    long error_nb = 0;

    bool idl_version_5_or_later;
    idl_version_5_or_later = ring[index].attr_value_4 == nullptr;

    for(i = 0; i < n; i++)
    {
        if(ring[index].except == nullptr)
        {
            if(!idl_version_5_or_later)
            {
                int r_dim_x = (*ring[index].attr_value_4)[0].r_dim.dim_x;
                int r_dim_y = (*ring[index].attr_value_4)[0].r_dim.dim_y;
                int w_dim_x = (*ring[index].attr_value_4)[0].w_dim.dim_x;
                int w_dim_y = (*ring[index].attr_value_4)[0].w_dim.dim_y;

                int data_length;
                (r_dim_y == 0) ? data_length = r_dim_x : data_length = r_dim_x * r_dim_y;
                (w_dim_y == 0) ? data_length += w_dim_x : data_length += (w_dim_x * w_dim_y);

                seq_size = seq_size + data_length;
            }
            else
            {
                int r_dim_x = (*ring[index].attr_value_5)[0].r_dim.dim_x;
                int r_dim_y = (*ring[index].attr_value_5)[0].r_dim.dim_y;
                int w_dim_x = (*ring[index].attr_value_5)[0].w_dim.dim_x;
                int w_dim_y = (*ring[index].attr_value_5)[0].w_dim.dim_y;

                int data_length;
                (r_dim_y == 0) ? data_length = r_dim_x : data_length = r_dim_x * r_dim_y;
                (w_dim_y == 0) ? data_length += w_dim_x : data_length += (w_dim_x * w_dim_y);

                seq_size = seq_size + data_length;
            }
        }
        else
        {
            error_nb++;
        }

        if(index == 0)
        {
            index = max_elt;
        }
        index--;
    }

    //
    // Now, we need to build the data transfer structure
    //

    AttrQuality last_quality;
    int quals_length = 1;
    AttributeDim last_dim_read;
    int read_dims_length = 1;
    AttributeDim last_dim_write;
    int write_dims_length = 1;
    DevErrorList last_err_list;
    int errors_length = 1;

    last_dim_read.dim_x = -1;
    last_dim_read.dim_y = -1;

    last_dim_write.dim_x = -1;
    last_dim_write.dim_y = -1;

    unsigned int ind_in_seq = 0;

    Tango::DevVarDoubleArray *new_tmp_db = nullptr;
    Tango::DevVarShortArray *new_tmp_sh = nullptr;
    Tango::DevVarLongArray *new_tmp_lg = nullptr;
    Tango::DevVarLong64Array *new_tmp_lg64 = nullptr;
    Tango::DevVarStringArray *new_tmp_str = nullptr;
    Tango::DevVarFloatArray *new_tmp_fl = nullptr;
    Tango::DevVarBooleanArray *new_tmp_boo = nullptr;
    Tango::DevVarUShortArray *new_tmp_ush = nullptr;
    Tango::DevVarCharArray *new_tmp_uch = nullptr;
    Tango::DevVarULongArray *new_tmp_ulg = nullptr;
    Tango::DevVarULong64Array *new_tmp_ulg64 = nullptr;
    Tango::DevVarStateArray *new_tmp_state = nullptr;
    Tango::DevVarEncodedArray *new_tmp_enc = nullptr;

    //
    // Read buffer
    //

    bool previous_no_data = true;
    bool no_data = true;
    last_quality = Tango::ATTR_VALID;

    index = insert_elt;
    if(index == 0)
    {
        index = max_elt;
    }
    index--;

    for(i = 0; i < n; i++)
    {
        previous_no_data = no_data;
        no_data = false;

        //
        // Copy date in output structure
        // In no error case, we take date from the attr_valie_X stucture and here the date is not biased
        //

        if(ring[index].except == nullptr)
        {
            if(!idl_version_5_or_later)
            {
                ptr->dates[seq_index].tv_sec = (*ring[index].attr_value_4)[0].time.tv_sec;
                ptr->dates[seq_index].tv_usec = (*ring[index].attr_value_4)[0].time.tv_usec;
            }
            else
            {
                ptr->dates[seq_index].tv_sec = (*ring[index].attr_value_5)[0].time.tv_sec;
                ptr->dates[seq_index].tv_usec = (*ring[index].attr_value_5)[0].time.tv_usec;
            }
        }
        else
        {
            ptr->dates[seq_index] = make_TimeVal(ring[index].when);
        }
        ptr->dates[seq_index].tv_nsec = 0;

        //
        // Copy element only if they are different than the previous one
        // First, for quality factor
        //

        if(ring[index].except == nullptr)
        {
            AttrQuality qu;
            if(!idl_version_5_or_later)
            {
                qu = (*ring[index].attr_value_4)[0].quality;
            }
            else
            {
                qu = (*ring[index].attr_value_5)[0].quality;
            }

            if((quals_length == 1) || (qu != last_quality))
            {
                if(!idl_version_5_or_later)
                {
                    last_quality = (*ring[index].attr_value_4)[0].quality;
                }
                else
                {
                    last_quality = (*ring[index].attr_value_5)[0].quality;
                }

                ptr->quals.length(quals_length);
                ptr->quals[quals_length - 1] = last_quality;
                ptr->quals_array.length(quals_length);
                ptr->quals_array[quals_length - 1].start = n - (i + 1);
                ptr->quals_array[quals_length - 1].nb_elt = 1;
                quals_length++;
                if(last_quality == Tango::ATTR_INVALID)
                {
                    previous_no_data = no_data;
                    no_data = true;
                }
            }
            else
            {
                ptr->quals_array[quals_length - 2].nb_elt++;
                if(last_quality == Tango::ATTR_INVALID)
                {
                    previous_no_data = no_data;
                    no_data = true;
                }
            }

            //
            // The read dimension
            //

            bool check = false;

            if(!idl_version_5_or_later)
            {
                if(((*ring[index].attr_value_4)[0].r_dim.dim_x == last_dim_read.dim_x) &&
                   ((*ring[index].attr_value_4)[0].r_dim.dim_y == last_dim_read.dim_y))
                {
                    check = true;
                }
            }
            else
            {
                if(((*ring[index].attr_value_5)[0].r_dim.dim_x == last_dim_read.dim_x) &&
                   ((*ring[index].attr_value_5)[0].r_dim.dim_y == last_dim_read.dim_y))
                {
                    check = true;
                }
            }

            if(check)
            {
                ptr->r_dims_array[read_dims_length - 2].nb_elt++;
            }
            else
            {
                if(!idl_version_5_or_later)
                {
                    last_dim_read = (*ring[index].attr_value_4)[0].r_dim;
                }
                else
                {
                    last_dim_read = (*ring[index].attr_value_5)[0].r_dim;
                }
                ptr->r_dims.length(read_dims_length);
                ptr->r_dims[read_dims_length - 1] = last_dim_read;
                ptr->r_dims_array.length(read_dims_length);
                ptr->r_dims_array[read_dims_length - 1].start = n - (i + 1);
                ptr->r_dims_array[read_dims_length - 1].nb_elt = 1;
                read_dims_length++;
            }

            //
            // The write dimension
            //

            check = false;

            if(!idl_version_5_or_later)
            {
                if(((*ring[index].attr_value_4)[0].w_dim.dim_x == last_dim_write.dim_x) &&
                   ((*ring[index].attr_value_4)[0].w_dim.dim_y == last_dim_write.dim_y))
                {
                    check = true;
                }
            }
            else
            {
                if(((*ring[index].attr_value_5)[0].w_dim.dim_x == last_dim_write.dim_x) &&
                   ((*ring[index].attr_value_5)[0].w_dim.dim_y == last_dim_write.dim_y))
                {
                    check = true;
                }
            }

            if(check)
            {
                ptr->w_dims_array[write_dims_length - 2].nb_elt++;
            }
            else
            {
                if(!idl_version_5_or_later)
                {
                    last_dim_write = (*ring[index].attr_value_4)[0].w_dim;
                }
                else
                {
                    last_dim_write = (*ring[index].attr_value_5)[0].w_dim;
                }
                ptr->w_dims.length(write_dims_length);
                ptr->w_dims[write_dims_length - 1] = last_dim_write;
                ptr->w_dims_array.length(write_dims_length);
                ptr->w_dims_array[write_dims_length - 1].start = n - (i + 1);
                ptr->w_dims_array[write_dims_length - 1].nb_elt = 1;
                write_dims_length++;
            }
        }
        else
        {
            no_data = true;
        }

        //
        // Error treatement
        //

        if(ring[index].except != nullptr)
        {
            bool new_err = false;

            if(previous_no_data)
            {
                if(ring[index].except->errors.length() != last_err_list.length())
                {
                    new_err = true;
                }
                else
                {
                    for(unsigned int k = 0; k < last_err_list.length(); k++)
                    {
                        if(::strcmp(ring[index].except->errors[k].reason.in(), last_err_list[k].reason.in()) != 0)
                        {
                            new_err = true;
                            break;
                        }
                        if(::strcmp(ring[index].except->errors[k].desc.in(), last_err_list[k].desc.in()) != 0)
                        {
                            new_err = true;
                            break;
                        }
                        if(::strcmp(ring[index].except->errors[k].origin.in(), last_err_list[k].origin.in()) != 0)
                        {
                            new_err = true;
                            break;
                        }
                        if(ring[index].except->errors[k].severity != last_err_list[k].severity)
                        {
                            new_err = true;
                            break;
                        }
                    }
                }
            }
            else
            {
                new_err = true;
            }

            if(new_err)
            {
                if(ptr->errors.length() == 0)
                {
                    ptr->errors.length(error_nb);
                }
                if(ptr->errors_array.length() == 0)
                {
                    ptr->errors_array.length(error_nb);
                }
                last_err_list = ring[index].except->errors;
                ptr->errors[errors_length - 1] = last_err_list;
                ptr->errors_array[errors_length - 1].start = n - (i + 1);
                ptr->errors_array[errors_length - 1].nb_elt = 1;
                errors_length++;
                previous_no_data = no_data;
                no_data = true;
            }
            else
            {
                ptr->errors_array[errors_length - 2].nb_elt++;
                previous_no_data = no_data;
                no_data = true;
            }

            //
            // Due to compatibility with the old release, the polling thread stores the error
            // got when reading the attribute as an exception (same behaviour that what we had before IDL3)
            // This means that we have to manually add entry for the management of quality factor (set to INVALID),
            // r_dim and w_dim (set to 0)
            //

            if((quals_length == 1) || (last_quality != Tango::ATTR_INVALID))
            {
                last_quality = Tango::ATTR_INVALID;
                ptr->quals.length(quals_length);
                ptr->quals[quals_length - 1] = last_quality;
                ptr->quals_array.length(quals_length);
                ptr->quals_array[quals_length - 1].start = n - (i + 1);
                ptr->quals_array[quals_length - 1].nb_elt = 1;
                quals_length++;
            }
            else
            {
                ptr->quals_array[quals_length - 2].nb_elt++;
            }

            if((last_dim_read.dim_x == 0) && (last_dim_read.dim_y == 0))
            {
                ptr->r_dims_array[read_dims_length - 2].nb_elt++;
            }
            else
            {
                last_dim_read.dim_x = last_dim_read.dim_y = 0;
                ptr->r_dims.length(read_dims_length);
                ptr->r_dims[read_dims_length - 1] = last_dim_read;
                ptr->r_dims_array.length(read_dims_length);
                ptr->r_dims_array[read_dims_length - 1].start = n - (i + 1);
                ptr->r_dims_array[read_dims_length - 1].nb_elt = 1;
                read_dims_length++;
            }

            if((last_dim_write.dim_x == 0) && (last_dim_write.dim_y == 0))
            {
                ptr->w_dims_array[write_dims_length - 2].nb_elt++;
            }
            else
            {
                last_dim_write.dim_x = last_dim_write.dim_y = 0;
                ptr->w_dims.length(write_dims_length);
                ptr->w_dims[write_dims_length - 1] = last_dim_write;
                ptr->w_dims_array.length(write_dims_length);
                ptr->w_dims_array[write_dims_length - 1].start = n - (i + 1);
                ptr->w_dims_array[write_dims_length - 1].nb_elt = 1;
                write_dims_length++;
            }
        }

        //
        // Now, the data themselves
        //

        if(!no_data)
        {
            //
            // Trick: The state when read as an attribute is not store within the Any as a sequence
            // To cover this case, we use the "type" data set to DEV_VOID when we are dealing with
            // the state as an attribute
            //

            AttrValUnion *union_ptr;
            if(!idl_version_5_or_later)
            {
                union_ptr = &((*ring[index].attr_value_4)[0].value);
            }
            else
            {
                union_ptr = &((*ring[index].attr_value_5)[0].value);
            }

            switch(type)
            {
            case Tango::DEV_SHORT:
            case Tango::DEV_ENUM:
            {
                DevVarShortArray &tmp_seq = union_ptr->short_att_value();
                if(new_tmp_sh == nullptr)
                {
                    new_tmp_sh = new DevVarShortArray();
                    new_tmp_sh->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_sh, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_DOUBLE:
            {
                DevVarDoubleArray &tmp_seq = union_ptr->double_att_value();
                if(new_tmp_db == nullptr)
                {
                    new_tmp_db = new DevVarDoubleArray();
                    new_tmp_db->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_db, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_LONG:
            {
                DevVarLongArray &tmp_seq = union_ptr->long_att_value();
                if(new_tmp_lg == nullptr)
                {
                    new_tmp_lg = new DevVarLongArray();
                    new_tmp_lg->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_lg, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_LONG64:
            {
                DevVarLong64Array &tmp_seq = union_ptr->long64_att_value();
                if(new_tmp_lg64 == nullptr)
                {
                    new_tmp_lg64 = new DevVarLong64Array();
                    new_tmp_lg64->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_lg64, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_STRING:
            {
                DevVarStringArray &tmp_seq = union_ptr->string_att_value();
                if(new_tmp_str == nullptr)
                {
                    new_tmp_str = new DevVarStringArray();
                    new_tmp_str->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_str, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_FLOAT:
            {
                DevVarFloatArray &tmp_seq = union_ptr->float_att_value();
                if(new_tmp_fl == nullptr)
                {
                    new_tmp_fl = new DevVarFloatArray();
                    new_tmp_fl->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_fl, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_BOOLEAN:
            {
                DevVarBooleanArray &tmp_seq = union_ptr->bool_att_value();
                if(new_tmp_boo == nullptr)
                {
                    new_tmp_boo = new DevVarBooleanArray();
                    new_tmp_boo->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_boo, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_USHORT:
            {
                DevVarUShortArray &tmp_seq = union_ptr->ushort_att_value();
                if(new_tmp_ush == nullptr)
                {
                    new_tmp_ush = new DevVarUShortArray();
                    new_tmp_ush->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_ush, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_UCHAR:
            {
                DevVarCharArray &tmp_seq = union_ptr->uchar_att_value();
                if(new_tmp_uch == nullptr)
                {
                    new_tmp_uch = new DevVarUCharArray();
                    new_tmp_uch->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_uch, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_ULONG:
            {
                DevVarULongArray &tmp_seq = union_ptr->ulong_att_value();
                if(new_tmp_ulg == nullptr)
                {
                    new_tmp_ulg = new DevVarULongArray();
                    new_tmp_ulg->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_ulg, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_ULONG64:
            {
                DevVarULong64Array &tmp_seq = union_ptr->ulong64_att_value();
                if(new_tmp_ulg64 == nullptr)
                {
                    new_tmp_ulg64 = new DevVarULong64Array();
                    new_tmp_ulg64->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_ulg64, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_STATE:
            {
                DevVarStateArray &tmp_seq = union_ptr->state_att_value();
                if(new_tmp_state == nullptr)
                {
                    new_tmp_state = new DevVarStateArray();
                    new_tmp_state->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_state, tmp_seq, ind_in_seq);
                break;
            }

            case Tango::DEV_VOID:
            {
                DevState tmp_state = union_ptr->dev_state_att();
                if(new_tmp_state == nullptr)
                {
                    new_tmp_state = new DevVarStateArray();
                    new_tmp_state->length(seq_size);
                }
                ADD_SIMPLE_DATA_TO_GLOBAL_SEQ(new_tmp_state, tmp_state, ind_in_seq);
                break;
            }

            case Tango::DEV_ENCODED:
            {
                DevVarEncodedArray &tmp_seq = union_ptr->encoded_att_value();
                if(new_tmp_enc == nullptr)
                {
                    new_tmp_enc = new DevVarEncodedArray();
                    new_tmp_enc->length(seq_size);
                }
                ADD_ELT_DATA_TO_GLOBAL_SEQ_BY_PTR_REF(new_tmp_enc, tmp_seq, ind_in_seq);
                break;
            }
            }
        }

        //
        // If it is the last point, insert the global sequence into the Any
        //

        if(i == (n - 1))
        {
            if(errors_length != (error_nb + 1))
            {
                ptr->errors.length(errors_length - 1);
                ptr->errors_array.length(errors_length - 1);
            }

            switch(type)
            {
            case Tango::DEV_SHORT:
            case Tango::DEV_ENUM:
                if(new_tmp_sh != nullptr)
                {
                    ptr->value <<= new_tmp_sh;
                }
                break;

            case Tango::DEV_DOUBLE:
                if(new_tmp_db != nullptr)
                {
                    ptr->value <<= new_tmp_db;
                }
                break;

            case Tango::DEV_LONG:
                if(new_tmp_lg != nullptr)
                {
                    ptr->value <<= new_tmp_lg;
                }
                break;

            case Tango::DEV_LONG64:
                if(new_tmp_lg64 != nullptr)
                {
                    ptr->value <<= new_tmp_lg64;
                }
                break;

            case Tango::DEV_STRING:
                if(new_tmp_str != nullptr)
                {
                    ptr->value <<= new_tmp_str;
                }
                break;

            case Tango::DEV_FLOAT:
                if(new_tmp_fl != nullptr)
                {
                    ptr->value <<= new_tmp_fl;
                }
                break;

            case Tango::DEV_BOOLEAN:
                if(new_tmp_boo != nullptr)
                {
                    ptr->value <<= new_tmp_boo;
                }
                break;

            case Tango::DEV_USHORT:
                if(new_tmp_ush != nullptr)
                {
                    ptr->value <<= new_tmp_ush;
                }
                break;

            case Tango::DEV_UCHAR:
                if(new_tmp_uch != nullptr)
                {
                    ptr->value <<= new_tmp_uch;
                }
                break;

            case Tango::DEV_ULONG:
                if(new_tmp_ulg != nullptr)
                {
                    ptr->value <<= new_tmp_ulg;
                }
                break;

            case Tango::DEV_ULONG64:
                if(new_tmp_ulg64 != nullptr)
                {
                    ptr->value <<= new_tmp_ulg64;
                }
                break;

            case Tango::DEV_ENCODED:
                if(new_tmp_enc != nullptr)
                {
                    ptr->value <<= new_tmp_enc;
                }
                break;

            case Tango::DEV_STATE:
            case Tango::DEV_VOID:
                if(new_tmp_state != nullptr)
                {
                    ptr->value <<= new_tmp_state;
                }
                break;
            }
        }

        //
        // Manage indexes
        //

        if(index == 0)
        {
            index = max_elt;
        }
        index--;
        seq_index--;
    }
}

} // namespace Tango

#endif
