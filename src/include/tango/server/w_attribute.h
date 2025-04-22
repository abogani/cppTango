//=============================================================================
//
// file :        w_attribute.h
//
// description :    Include file for the WAttribute classes.
//
// project :        TANGO
//
// author(s) :        A.Gotz + E.Taurel
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
//
//=============================================================================

#ifndef _WATTRIBUTE_H
#define _WATTRIBUTE_H

#include <tango/tango.h>
#include <tango/common/tango_type_traits.h>
#include <tango/server/attrdesc.h>
#include <functional>
#include <time.h>

namespace Tango
{

//=============================================================================
//
//            The WAttribute class
//
//
// description :    This class inherits the Attribute class. There is one
//            instance of this class for each writable attribute
//
//=============================================================================

/**
 * This class represents a writable attribute. It inherits from the Attribute
 * class and only add what is specific to writable attribute.
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

class WAttribute : public Attribute
{
  public:
    /**@name Constructors
     * Miscellaneous constructors */
    //@{
    /**
     * Create a new Writable Attribute object.
     *
     * @param prop_list The attribute properties list. Each property is an object
     * of the AttrProperty class
     * @param tmp_attr The temporary attribute object built from user parameters
     * @param dev_name The device name
     * @param idx The index of the related Attr object in the MultiClassAttribute
     *            vector of Attr object
     */
    WAttribute(std::vector<AttrProperty> &prop_list, Attr &tmp_attr, const std::string &dev_name, long idx);
    //@}

    /**@name Destructor
     * Only one desctructor is defined for this class
     */
    //@{
    /**
     * The WAttribute desctructor.
     */
    ~WAttribute() override;

    //@}

    /**@name Attribute configuration methods
     * Miscellaneous methods dealing with attribute min and max value property
     */
    //@{
    /**
     * Check if the attribute has a minimum value.
     *
     * @return A boolean set to true if the attribute has a minimum value
     * defined
     */
    bool is_min_value()
    {
        return check_min_value;
    }

    /**
     * Set attribute minimum value
     *
     * @param min_value Reference to a variable which represents the new min value
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T> || std::is_same_v<T, std::string>> * = nullptr>
    void set_min_value(const T &min_value);

    /**
     * Set attribute minimum value
     *
     * @param min_value The new min value
     */
    void set_min_value(char *min_value);

    /**
     * Set attribute minimum value
     *
     * @param min_value The new min value
     */
    void set_min_value(const char *min_value);
    /**
     * Gets attribute minimum value or throws an exception if the
     * attribute does not have a minimum value
     *
     * @param min_value Reference to a variable which value will be set to the attribute's
     * minimum value
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void get_min_value(T &min_value);

    /**
     * Check if the attribute has a maximum value.
     *
     * @return check_max_value A boolean set to true if the attribute has a maximum value
     * defined
     */
    bool is_max_value()
    {
        return check_max_value;
    }

    /**
     * Set attribute maximum value
     *
     * @param max_value Reference to a variable which represents the new max value
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T> || std::is_same_v<T, std::string>> * = nullptr>
    void set_max_value(const T &max_value);

    /**
     * Set attribute maximum value
     *
     * @param max_value The new max value
     */
    void set_max_value(char *max_value);
    /**
     * Set attribute maximum value
     *
     * @param max_value The new max value
     */
    void set_max_value(const char *max_value);
    /**
     * Get attribute maximum value or throws an exception if the
     * attribute does not have a maximum value
     *
     * @param max_value Reference to a variable which value will be set to the attribute's
     * maximum value
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void get_max_value(T &max_value);
    //@}

    /**@name Get new value for attribute
     * Miscellaneous method to retrieve from the WAttribute object the new value
     * for the attribute.
     */
    //@{
    /**
     * Retrieve the new value length (data number) for writable attribute.
     *
     * @return  The new value data length
     */
    long get_write_value_length();

    /**
     * Retrieve the date of the last attribute writing. This is set only
     * if the attribute has a read different than set alarm. Otherwise,
     * date is set to 0.
     *
     * @return  The written date
     */
    struct timeval &get_write_date()
    {
        return write_date;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevShort.
     *
     * @param val A reference to a Tango::DevShort data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevShort &val)
    {
        val = short_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevShort and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer wich will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevShort *&ptr)
    {
        ptr = short_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevLong.
     *
     * @param val A reference to a Tango::DevLong data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevLong &val)
    {
        val = long_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevLong and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevLong *&ptr)
    {
        ptr = long_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevLong64.
     *
     * @param val A reference to a Tango::DevLong64 data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevLong64 &val)
    {
        val = long64_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevLong64 and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevLong64 *&ptr)
    {
        ptr = long64_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevFloat.
     *
     * @param val A reference to a Tango::DevFloat data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevFloat &val)
    {
        val = float_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevFloat and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevFloat *&ptr)
    {
        ptr = float_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevDouble.
     *
     * @param val A reference to a Tango::DevDouble data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevDouble &val)
    {
        val = double_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevDouble and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevDouble *&ptr)
    {
        ptr = double_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevString.
     *
     * @param val A reference to a Tango::DevString data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevString &val)
    {
        val = str_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevString and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::ConstDevString *&ptr)
    {
        ptr = str_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevBoolean.
     *
     * @param val A reference to a Tango::DevBoolean data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevBoolean &val)
    {
        val = boolean_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevBoolean and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevBoolean *&ptr)
    {
        ptr = boolean_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevUShort.
     *
     * @param val A reference to a Tango::DevUShort data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevUShort &val)
    {
        val = ushort_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevUShort and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevUShort *&ptr)
    {
        ptr = ushort_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevUChar.
     *
     * @param val A reference to a Tango::DevUChar data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevUChar &val)
    {
        val = uchar_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevUChar and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevUChar *&ptr)
    {
        ptr = uchar_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevULong.
     *
     * @param val A reference to a Tango::DevULong data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevULong &val)
    {
        val = ulong_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevULong and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevULong *&ptr)
    {
        ptr = ulong_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevULong64.
     *
     * @param val A reference to a Tango::DevULong64 data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevULong64 &val)
    {
        val = ulong64_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevLong64 and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevULong64 *&ptr)
    {
        ptr = ulong64_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevState.
     *
     * @param val A reference to a Tango::DevState data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevState &val)
    {
        val = dev_state_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevLong64 and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevState *&ptr)
    {
        ptr = state_ptr;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevEncoded.
     *
     * @param val A reference to a Tango::DevEncoded data which will be initialised
     * with the new value
     */
    void get_write_value(Tango::DevEncoded &val)
    {
        val = encoded_val;
    }

    /**
     * Retrieve the new value for writable attribute when attribute data type is
     * Tango::DevEncoded and the attribute is SPECTRUM or IMAGE.
     *
     * @param ptr Reference to a pointer which will be set to point to the data
     * to be written into the attribute. This pointer points into attribute
     * internal memory which must not be freed.
     */
    void get_write_value(const Tango::DevEncoded *&ptr)
    {
        ptr = encoded_ptr;
    }

    //@}

    /**@name Set new value for attribute
     * Miscellaneous method to set a WAttribute value
     */
    //@{
    /**
     * Set the writable scalar attribute value when the attribute
     * data type is not an enum.
     *
     * @param val A reference to the attribute set value
     * @param x The attribute set value x length. Default value is 1
     * @param y The attribute set value y length. Default value is 0

     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_write_value(T *val, size_t x = 1, size_t y = 0);

    /**@name Set new value for attribute
     * Miscellaneous method to set a WAttribute value
     */
    //@{
    /**
     * Set the writable scalar attribute value when the attribute
     * data type is not an enum.
     *
     * @param val The value to set as the attribute setpoint

     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_write_value(T val);

    /**@name Set new value for attribute
     * Miscellaneous method to set a WAttribute value
     */
    //@{
    /**
     * Set the writable scalar attribute value when the attribute
     * data type is an enum.
     *
     * @param val A reference to the attribute set value
     * @param x The attribute set value x length. Default value is 1
     * @param y The attribute set value y length. Default value is 0

     */
    template <class T, std::enable_if_t<std::is_enum_v<T> && !std::is_same_v<T, Tango::DevState>, T> * = nullptr>
    void set_write_value(T *val, size_t x = 1, size_t y = 0);

    /**@name Set new value for attribute
     * Miscellaneous method to set a WAttribute value
     */
    //@{
    /**
     * Set the writable spectrum or image attribute value.
     *
     * @param val A vector of values to set.
     * @param x The attribute set value x length. Default value is 1
     * @param y The attribute set value y length. Default value is 0

     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_write_value(std::vector<T> &val, size_t x = 1, size_t y = 0);

    /**
     * Set the writable scalar attribute value when the attribute data type is
     * Tango::DevString.
     *
     * @param val A reference to a std::string
     */
    void set_write_value(std::string &val);
    /**
     * Set the writable scalar attribute value when the attribute data type is
     * Tango::DevString.
     *
     * @param val A Tango::DevString
     */
    void set_write_value(Tango::DevString val);

    /**
     * Set the writable image or spectrum attribute value when the attribute data type is
     * Tango::DevString.
     *
     * @param val A vector of std::string
     */
    void set_write_value(std::vector<std::string> &val, size_t x = 1, size_t y = 0);
    //@}

    /// @privatesection

    template <typename T>
    void get_write_value(T &);

    template <typename T>
    void get_write_value(const T *&);

    void set_write_value(Tango::DevEncoded *, long x = 1, long y = 0); // Dummy method for compiler

    void set_rvalue() override;

    void rollback();

    void check_written_value(const CORBA::Any &, unsigned long, unsigned long);
    void check_written_value(const AttrValUnion &, unsigned long, unsigned long);

    void copy_data(const CORBA::Any &);
    void copy_data(const Tango::AttrValUnion &);

    long get_w_dim_x()
    {
        return w_dim_x;
    }

    long get_w_dim_y()
    {
        return w_dim_y;
    }

    void set_user_set_write_value(bool val)
    {
        uswv = val;
    }

    bool get_user_set_write_value()
    {
        return uswv;
    }

    Tango::DevVarShortArray *get_last_written_sh()
    {
        return &short_array_val;
    }

    Tango::DevVarLongArray *get_last_written_lg()
    {
        return &long_array_val;
    }

    Tango::DevVarDoubleArray *get_last_written_db()
    {
        return &double_array_val;
    }

    Tango::DevVarStringArray *get_last_written_str()
    {
        return &str_array_val;
    }

    Tango::DevVarFloatArray *get_last_written_fl()
    {
        return &float_array_val;
    }

    Tango::DevVarBooleanArray *get_last_written_boo()
    {
        return &boolean_array_val;
    }

    Tango::DevVarUShortArray *get_last_written_ush()
    {
        return &ushort_array_val;
    }

    Tango::DevVarCharArray *get_last_written_uch()
    {
        return &uchar_array_val;
    }

    Tango::DevVarLong64Array *get_last_written_lg64()
    {
        return &long64_array_val;
    }

    Tango::DevVarULong64Array *get_last_written_ulg64()
    {
        return &ulong64_array_val;
    }

    Tango::DevVarULongArray *get_last_written_ulg()
    {
        return &ulong_array_val;
    }

    Tango::DevVarStateArray *get_last_written_state()
    {
        return &state_array_val;
    }

    Tango::DevEncoded &get_last_written_encoded()
    {
        return encoded_val;
    }

    bool is_memorized()
    {
        return memorized;
    }

    bool get_memorized()
    {
        return memorized;
    }

    void set_memorized(bool mem)
    {
        memorized = mem;
    }

    bool is_memorized_init()
    {
        return memorized_init;
    }

    bool get_memorized_init()
    {
        return memorized_init;
    }

    void set_memorized_init(bool mem_init)
    {
        memorized_init = mem_init;
    }

    std::string &get_mem_value()
    {
        return mem_value;
    }

    void set_mem_value(const std::string &new_val)
    {
        mem_value = new_val;
    }

    void set_written_date();
    bool mem_value_below_above(MinMaxValueCheck, std::string &);

    void set_mem_exception(const DevErrorList &df)
    {
        mem_exception = df;
        mem_write_failed = true;
        att_mem_exception = true;
    }

    DevErrorList &get_mem_exception()
    {
        return mem_exception;
    }

    void clear_mem_exception()
    {
        mem_exception.length(0);
        mem_write_failed = false;
        att_mem_exception = false;
    }

    void set_mem_write_failed(bool bo)
    {
        mem_write_failed = bo;
    }

    bool get_mem_write_failed()
    {
        return mem_write_failed;
    }

  protected:
    /// @privatesection
    bool check_rds_alarm() override;

  private:
    /**
     * Get a ref to a pointer for the internal field for the value.
     * Do not manage memory.
     */
    template <class T>
    T &get_write_value_ptr();

    /**
     * Get a ref to the internal field for the value.
     * Only for scalar attribute.
     * Do not manage memory.
     */
    template <class T>
    T &get_last_written_value();
    /**
     * Get a ref to the internal field for the value.
     * Do not manage memory.
     */
    template <class T>
    T &get_write_value();
    /**
     * Get a ref to the internal field for the previous value.
     * Do not manage memory.
     */
    template <class T>
    T &get_old_value();

    template <typename T>
    void check_type(const std::string &);

    //
    // The extension class
    //

    class WAttributeExt
    {
      public:
        WAttributeExt() { }
    };

    // Defined prior to Tango IDL release 3

    Tango::DevShort short_val;
    Tango::DevShort old_short_val;

    Tango::DevLong long_val;
    Tango::DevLong old_long_val;

    Tango::DevDouble double_val;
    Tango::DevDouble old_double_val;

    Tango::DevString str_val;
    Tango::DevString old_str_val;

    Tango::DevFloat float_val;
    Tango::DevFloat old_float_val;

    Tango::DevBoolean boolean_val;
    Tango::DevBoolean old_boolean_val;

    Tango::DevUShort ushort_val;
    Tango::DevUShort old_ushort_val;

    Tango::DevUChar uchar_val;
    Tango::DevUChar old_uchar_val;

    Tango::DevEncoded encoded_val;
    Tango::DevEncoded old_encoded_val;

    // Added for Tango IDL release 3

    long w_dim_y;
    long w_dim_x;

    Tango::DevVarShortArray short_array_val;
    Tango::DevVarLongArray long_array_val;
    Tango::DevVarDoubleArray double_array_val;
    Tango::DevVarStringArray str_array_val;
    Tango::DevVarFloatArray float_array_val;
    Tango::DevVarBooleanArray boolean_array_val;
    Tango::DevVarUShortArray ushort_array_val;
    Tango::DevVarCharArray uchar_array_val;

    const Tango::DevShort *short_ptr;
    const Tango::DevLong *long_ptr{nullptr};
    const Tango::DevDouble *double_ptr{nullptr};
    const Tango::ConstDevString *str_ptr{nullptr};
    const Tango::DevFloat *float_ptr{nullptr};
    const Tango::DevBoolean *boolean_ptr{nullptr};
    const Tango::DevUShort *ushort_ptr{nullptr};
    const Tango::DevUChar *uchar_ptr{nullptr};
    const Tango::DevEncoded *encoded_ptr{nullptr};

    bool string_allocated{false};
    bool memorized{false};
    bool memorized_init{false};
    std::string mem_value;
    struct timeval write_date;

    std::unique_ptr<WAttributeExt> w_ext; // Class extension

    //
    // Ported from the extension class
    //

    Tango::DevLong64 long64_val;
    Tango::DevLong64 old_long64_val;
    Tango::DevULong ulong_val;
    Tango::DevULong old_ulong_val;
    Tango::DevULong64 ulong64_val;
    Tango::DevULong64 old_ulong64_val;
    Tango::DevState dev_state_val;
    Tango::DevState old_dev_state_val;

    Tango::DevVarLong64Array long64_array_val;
    Tango::DevVarULongArray ulong_array_val;
    Tango::DevVarULong64Array ulong64_array_val;
    Tango::DevVarStateArray state_array_val;

    const Tango::DevLong64 *long64_ptr{nullptr};
    const Tango::DevULong *ulong_ptr{nullptr};
    const Tango::DevULong64 *ulong64_ptr{nullptr};
    const Tango::DevState *state_ptr{nullptr};

    bool uswv{false};             // User set_write_value
    DevErrorList mem_exception;   // Exception received at start-up in case writing the
                                  // memorized att. failed
    bool mem_write_failed{false}; // Flag set to true if the memorized att setting failed
    template <class T>
    friend void _update_value(Tango::WAttribute &attr, const T &seq);
};

template <>
inline const Tango::DevShort *&WAttribute::get_write_value_ptr()
{
    return short_ptr;
}

template <>
inline Tango::DevVarShortArray &WAttribute::get_last_written_value()
{
    return short_array_val;
}

template <>
inline Tango::DevShort &WAttribute::get_write_value()
{
    return short_val;
}

template <>
inline Tango::DevShort &WAttribute::get_old_value()
{
    return old_short_val;
}

template <>
inline const Tango::DevUShort *&WAttribute::get_write_value_ptr()
{
    return ushort_ptr;
}

template <>
inline Tango::DevVarUShortArray &WAttribute::get_last_written_value()
{
    return ushort_array_val;
}

template <>
inline Tango::DevUShort &WAttribute::get_write_value()
{
    return ushort_val;
}

template <>
inline Tango::DevUShort &WAttribute::get_old_value()
{
    return old_ushort_val;
}

template <>
inline const Tango::DevLong *&WAttribute::get_write_value_ptr()
{
    return long_ptr;
}

template <>
inline Tango::DevVarLongArray &WAttribute::get_last_written_value()
{
    return long_array_val;
}

template <>
inline Tango::DevLong &WAttribute::get_write_value()
{
    return long_val;
}

template <>
inline Tango::DevLong &WAttribute::get_old_value()
{
    return old_long_val;
}

template <>
inline const Tango::DevULong *&WAttribute::get_write_value_ptr()
{
    return ulong_ptr;
}

template <>
inline Tango::DevVarULongArray &WAttribute::get_last_written_value()
{
    return ulong_array_val;
}

template <>
inline Tango::DevULong &WAttribute::get_write_value()
{
    return ulong_val;
}

template <>
inline Tango::DevULong &WAttribute::get_old_value()
{
    return old_ulong_val;
}

template <>
inline const Tango::DevLong64 *&WAttribute::get_write_value_ptr()
{
    return long64_ptr;
}

template <>
inline Tango::DevVarLong64Array &WAttribute::get_last_written_value()
{
    return long64_array_val;
}

template <>
inline Tango::DevLong64 &WAttribute::get_write_value()
{
    return long64_val;
}

template <>
inline Tango::DevLong64 &WAttribute::get_old_value()
{
    return old_long64_val;
}

template <>
inline const Tango::DevULong64 *&WAttribute::get_write_value_ptr()
{
    return ulong64_ptr;
}

template <>
inline Tango::DevVarULong64Array &WAttribute::get_last_written_value()
{
    return ulong64_array_val;
}

template <>
inline Tango::DevULong64 &WAttribute::get_write_value()
{
    return ulong64_val;
}

template <>
inline Tango::DevULong64 &WAttribute::get_old_value()
{
    return old_ulong64_val;
}

template <>
inline const Tango::DevDouble *&WAttribute::get_write_value_ptr()
{
    return double_ptr;
}

template <>
inline Tango::DevVarDoubleArray &WAttribute::get_last_written_value()
{
    return double_array_val;
}

template <>
inline Tango::DevDouble &WAttribute::get_write_value()
{
    return double_val;
}

template <>
inline Tango::DevDouble &WAttribute::get_old_value()
{
    return old_double_val;
}

template <>
inline const Tango::DevFloat *&WAttribute::get_write_value_ptr()
{
    return float_ptr;
}

template <>
inline Tango::DevVarFloatArray &WAttribute::get_last_written_value()
{
    return float_array_val;
}

template <>
inline Tango::DevFloat &WAttribute::get_write_value()
{
    return float_val;
}

template <>
inline Tango::DevFloat &WAttribute::get_old_value()
{
    return old_float_val;
}

template <>
inline const Tango::ConstDevString *&WAttribute::get_write_value_ptr()
{
    return str_ptr;
}

template <>
inline Tango::DevVarStringArray &WAttribute::get_last_written_value()
{
    return str_array_val;
}

template <>
inline Tango::DevString &WAttribute::get_write_value()
{
    return str_val;
}

template <>
inline Tango::DevString &WAttribute::get_old_value()
{
    return old_str_val;
}

template <>
inline const Tango::DevState *&WAttribute::get_write_value_ptr()
{
    return state_ptr;
}

template <>
inline Tango::DevVarStateArray &WAttribute::get_last_written_value()
{
    return state_array_val;
}

template <>
inline Tango::DevState &WAttribute::get_write_value()
{
    return dev_state_val;
}

template <>
inline Tango::DevState &WAttribute::get_old_value()
{
    return old_dev_state_val;
}

template <>
inline const Tango::DevBoolean *&WAttribute::get_write_value_ptr()
{
    return boolean_ptr;
}

template <>
inline Tango::DevVarBooleanArray &WAttribute::get_last_written_value()
{
    return boolean_array_val;
}

template <>
inline Tango::DevBoolean &WAttribute::get_write_value()
{
    return boolean_val;
}

template <>
inline Tango::DevBoolean &WAttribute::get_old_value()
{
    return old_boolean_val;
}

template <>
inline const Tango::DevEncoded *&WAttribute::get_write_value_ptr()
{
    return encoded_ptr;
}

template <>
inline Tango::DevEncoded &WAttribute::get_write_value()
{
    return encoded_val;
}

template <>
inline Tango::DevEncoded &WAttribute::get_old_value()
{
    return old_encoded_val;
}

template <>
inline const Tango::DevUChar *&WAttribute::get_write_value_ptr()
{
    return uchar_ptr;
}

template <>
inline Tango::DevVarCharArray &WAttribute::get_last_written_value()
{
    return uchar_array_val;
}

template <>
inline Tango::DevUChar &WAttribute::get_write_value()
{
    return uchar_val;
}

template <>
inline Tango::DevUChar &WAttribute::get_old_value()
{
    return old_uchar_val;
}
} // namespace Tango

#endif // _WATTRIBUTE_H
