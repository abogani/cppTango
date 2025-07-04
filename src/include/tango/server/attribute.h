
//===================================================================================================================
//
// file :        Attribute.h
//
// description :    Include file for the Attribute classes.
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
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//====================================================================================================================

#ifndef _ATTRIBUTE_H
#define _ATTRIBUTE_H

#include <tango/common/tango_type_traits.h>
#include <tango/server/attrdesc.h>
#include <tango/server/fwdattrdesc.h>
#include <tango/server/classattribute.h>
#include <tango/server/encoded_attribute.h>
#include <tango/server/tango_clock.h>
#include <tango/server/attrprop.h>
#include <tango/server/logging.h>
#include <tango/client/apiexcept.h>
#include <tango/client/DbDatum.h>
#include <tango/server/exception_reason_consts.h>
#include <tango/common/utils/assert.h>
#include <tango/server/tango_config.h>

#include <iterator>
#include <type_traits>
#include <variant>
#include <vector>
#include <map>
#include <string>

namespace Tango
{

class AttrProperty;
class DeviceClass;
class EncodedAttribute;

typedef union _Attr_CheckVal
{
    short sh;
    DevLong lg;
    double db;
    float fl;
    unsigned short ush;
    unsigned char uch;
    DevLong64 lg64;
    DevULong ulg;
    DevULong64 ulg64;
    DevState d_sta;
    /**
     * Returns a ref to the proper field based on type.
     * Does not manage memory.
     */
    template <class T>
    const T &get_value() const;
} Attr_CheckVal;

template <>
inline const Tango::DevShort &Attr_CheckVal::get_value() const
{
    return sh;
}

template <>
inline const Tango::DevLong &Attr_CheckVal::get_value() const
{
    return lg;
}

template <>
inline const Tango::DevDouble &Attr_CheckVal::get_value() const
{
    return db;
}

template <>
inline const Tango::DevFloat &Attr_CheckVal::get_value() const
{
    return fl;
}

template <>
inline const Tango::DevUShort &Attr_CheckVal::get_value() const
{
    return ush;
}

template <>
inline const Tango::DevUChar &Attr_CheckVal::get_value() const
{
    return uch;
}

template <>
inline const Tango::DevLong64 &Attr_CheckVal::get_value() const
{
    return lg64;
}

template <>
inline const Tango::DevULong &Attr_CheckVal::get_value() const
{
    return ulg;
}

template <>
inline const Tango::DevULong64 &Attr_CheckVal::get_value() const
{
    return ulg64;
}

template <>
inline const Tango::DevState &Attr_CheckVal::get_value() const
{
    return d_sta;
}

class AttrValue
{
  public:
    // Define a variant type that holds unique pointers to each allowed array type.
    using ValueVariant = std::variant<std::monostate, // Represents an empty state.
                                      std::unique_ptr<DevVarShortArray>,
                                      std::unique_ptr<DevVarLongArray>,
                                      std::unique_ptr<DevVarFloatArray>,
                                      std::unique_ptr<DevVarDoubleArray>,
                                      std::unique_ptr<DevVarStringArray>,
                                      std::unique_ptr<DevVarUShortArray>,
                                      std::unique_ptr<DevVarBooleanArray>,
                                      std::unique_ptr<DevVarCharArray>,
                                      std::unique_ptr<DevVarLong64Array>,
                                      std::unique_ptr<DevVarULongArray>,
                                      std::unique_ptr<DevVarULong64Array>,
                                      std::unique_ptr<DevVarStateArray>,
                                      std::unique_ptr<DevVarEncodedArray>>;

    AttrValue() :
        data_{std::monostate{}}
    {
    }

    // Setter: accepts a smart pointer to one of the allowed types.
    template <typename T>
    void set(std::unique_ptr<T> value)
    {
        TANGO_LOG_DEBUG << "AttrValue::set()" << std::endl;
        static_assert(is_valid<T>(), "Type not allowed in AttrValue");
        data_ = std::move(value);
    }

    // Getter: returns a raw pointer (or nullptr if the stored type doesn't match).
    template <typename T>
    T *get()
    {
        TANGO_LOG_DEBUG << "AttrValue::get()" << std::endl;
        static_assert(is_valid<T>(), "Type not allowed in AttrValue");
        if(auto ptr = std::get_if<std::unique_ptr<T>>(&data_))
        {
            return ptr->get(); //
        }
        return nullptr;
    }

    template <typename T>
    std::unique_ptr<T> release()
    {
        TANGO_LOG_DEBUG << "AttrValue::release()" << std::endl;

        static_assert(is_valid<T>(), "Type not allowed in AttrValue");
        if(auto ptr = std::get_if<std::unique_ptr<T>>(&data_))
        {
            std::unique_ptr<T> temp = std::move(*ptr);
            data_ = std::monostate{};
            return temp;
        }
        return nullptr;
    }

    // Reset clears any stored value. After calling reset, has_value() returns false.
    void reset()
    {
        TANGO_LOG_DEBUG << "AttrValue::reset()" << std::endl;
        data_ = std::monostate{};
    }

    // Check if any value is set.
    bool has_value() const
    {
        TANGO_LOG_DEBUG << "AttrValue::has_value()" << std::endl;
        return !std::holds_alternative<std::monostate>(data_);
    }

  private:
    ValueVariant data_;

    // Helper to restrict the types that can be stored.
    template <typename T>
    static constexpr bool is_valid()
    {
        return std::is_same_v<T, DevVarShortArray> || std::is_same_v<T, DevVarLongArray> ||
               std::is_same_v<T, DevVarFloatArray> || std::is_same_v<T, DevVarDoubleArray> ||
               std::is_same_v<T, DevVarStringArray> || std::is_same_v<T, DevVarUShortArray> ||
               std::is_same_v<T, DevVarBooleanArray> || std::is_same_v<T, DevVarCharArray> ||
               std::is_same_v<T, DevVarLong64Array> || std::is_same_v<T, DevVarULongArray> ||
               std::is_same_v<T, DevVarULong64Array> || std::is_same_v<T, DevVarStateArray> ||
               std::is_same_v<T, DevVarEncodedArray>;
    }
};

typedef union _Attr_Value
{
    DevVarShortArray *sh_seq;
    DevVarLongArray *lg_seq;
    DevVarFloatArray *fl_seq;
    DevVarDoubleArray *db_seq;
    DevVarStringArray *str_seq;
    DevVarUShortArray *ush_seq;
    DevVarBooleanArray *boo_seq;
    DevVarCharArray *cha_seq;
    DevVarLong64Array *lg64_seq;
    DevVarULongArray *ulg_seq;
    DevVarULong64Array *ulg64_seq;
    DevVarStateArray *state_seq;
    DevVarEncodedArray *enc_seq;
} Attr_Value;

struct LastAttrValue
{
    bool inited;
    Tango::AttrQuality quality;
    CORBA::Any value;
    bool err;
    DevFailed except;
    AttrValUnion value_4;
    void store(const AttributeValue_5 *,
               const AttributeValue_4 *,
               const AttributeValue_3 *,
               const AttributeValue *,
               DevFailed *);
};

class EventSupplier;

//=============================================================================
//
//            The Attribute class
//
//
// description :    There is one instance of this class for each attribute
//            for each device. This class stores the attribute
//            properties and the attribute value.
//
//=============================================================================

/**
 * This class represents a Tango attribute.
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

class Attribute
{
  public:
    /// @privatesection
    enum alarm_flags
    {
        min_level,
        max_level,
        rds,
        min_warn,
        max_warn,
        numFlags
    };

    struct CheckOneStrProp
    {
        DbData *db_d;
        long *prop_to_update;
        DbData *db_del;
        long *prop_to_delete;
        std::vector<AttrProperty> *def_user_prop;
        std::vector<AttrProperty> *def_class_prop;
    };

    enum _DbAction
    {
        UPD = 0,
        UPD_FROM_DB,
        UPD_FROM_VECT_STR,
        DEL
    };

    typedef _DbAction DbAction;

    struct _AttPropDb
    {
        std::string name;
        DbAction dba;
        std::string db_value;
        std::vector<double> db_value_db;
        std::vector<std::string> db_value_v_str;
    };

    typedef _AttPropDb AttPropDb;
    /// @publicsection

    /**@name Constructors
     * Miscellaneous constructors */
    //@{
    /**
     * Create a new Attribute object.
     *
     * @param prop_list The attribute properties list. Each property is an object
     * of the AttrProperty class
     * @param tmp_attr Temporary attribute object built from user parameters
     * @param dev_name The device name
     * @param idx The index of the related Attr object in the MultiClassAttribute
     *            vector of Attr object
     */
    Attribute(std::vector<AttrProperty> &prop_list, Attr &tmp_attr, const std::string &dev_name, long idx);
//@}

// remove once https://gitlab.com/tango-controls/cppTango/-/issues/786 is fixed
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdeprecated"
#endif

    /**@name Destructor
     * Only one desctructor is defined for this class
     */
    //@{
    /**
     * The attribute destructor.
     */
    virtual ~Attribute();
    //@}

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

    /**@name Check attribute methods
     * Miscellaneous method returning boolean flag according to attribute state
     */
    //@{
    /**
     * Check if the attribute has an associated writable attribute.
     *
     * This method returns a boolean set to true if the attribute has a writable
     * attribute associated to it.
     *
     * @return A boolean set to true if there is an associated writable attribute
     */
    bool is_writ_associated();

    /**
     * Check if the attribute is in minimum alarm condition .
     *
     * @return A boolean set to true if the attribute is in alarm condition (read
     * value below the min. alarm).
     */
    bool is_min_alarm()
    {
        return alarm.test(min_level);
    }

    /**
     * Check if the attribute is in maximum alarm condition .
     *
     * @return A boolean set to true if the attribute is in alarm condition (read
     * value above the max. alarm).
     */
    bool is_max_alarm()
    {
        return alarm.test(max_level);
    }

    /**
     * Check if the attribute is in minimum warning condition .
     *
     * @return A boolean set to true if the attribute is in warning condition (read
     * value below the min. warning).
     */
    bool is_min_warning()
    {
        return alarm.test(min_warn);
    }

    /**
     * Check if the attribute is in maximum warning condition .
     *
     * @return A boolean set to true if the attribute is in warning condition (read
     * value above the max. warning).
     */
    bool is_max_warning()
    {
        return alarm.test(max_warn);
    }

    /**
     * Check if the attribute is in RDS alarm condition .
     *
     * @return A boolean set to true if the attribute is in RDS condition (Read
     * Different than Set).
     */
    bool is_rds_alarm()
    {
        return alarm.test(rds);
    }

    /**
     * Check if the attribute has an alarm defined.
     *
     * This method returns a set of bits. Each alarm type is defined by one
     * bit.
     *
     * @return A bitset. Each bit is set if the coresponding alarm is on
     */
    std::bitset<numFlags> &is_alarmed()
    {
        return alarm_conf;
    }

    std::bitset<numFlags> const &is_alarmed() const
    {
        return alarm_conf;
    }

    /**
     * Check if the attribute is polled .
     *
     * @return A boolean set to true if the attribute is polled.
     */
    bool is_polled();

    /**
     * Check the attribute's value to determine if it should be
     * in alarm based on its configuration and update its quality and alarm reason
     * flag appropriately.
     *
     * The attribute can be determined to be in alarm for one of the following
     * reasons:
     *
     *  1. The attribute's value is outside the configured warning levels ([min_warning,
     *      max_warning]).
     *      The quality is set to ATTR_WARNING, this function will return true and one of
     *      is_min_warning() or is_max_warning() will be true.
     *  2. The attribute's value is outside the configured alarm levels ([min_alarm,
     *      max_alarm]).
     *      The quality is set to ATTR_ALARM, this function will return true and one of
     *      is_min_alarm() or is_max_alarm() will be true.
     *  3. For a WAttribute, the read value and write value have differed by more
     *      than the configured value (delta_val) for more than the configured time
     *      (delta_t).
     *      The quality is set to ATTR_ALARM, this function will return true and
     *      is_rds_alarm() will be true.
     *
     * If the quality is not ATTR_VALID on entry to this function, no calculation
     * to determine if the attribute should be in alarm is performed and the quality
     * and alarm reason flags are unchanged.  In this case `check_alarm` returns true
     * if the quality was either ATTR_ALARM or ATTR_WARNING on entry to the function.
     *
     * @note This function will generate a dev error (warning) stream log if
     * the quality of the attribute is ATTR_ALARM (ATTR_WARNING) and it is
     * different from the quality of the attribute before the last time the
     * attribute was read.  Similarly, if the reason the attribute is in alarm
     * has changed an appropriate dev stream log will be generated. This means
     * if a device server calls this function in between reads it can result in
     * multiple such logs.
     *
     * @return A boolean set to true if the attribute is in alarm condition.
     * @exception DevFailed If no alarm level is defined.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    bool check_alarm();

    //@}

    /**@name Get/Set object members.
     * These methods allow the external world to get/set DeviceImpl instance
     * data members
     */
    //@{
    /**
     * Get the attribute writable type (RO/WO/RW).
     *
     * @return The attribute write type.
     */
    Tango::AttrWriteType get_writable() const
    {
        return writable;
    }

    /**
     * Get attribute name
     *
     * @return The attribute name
     */
    std::string &get_name()
    {
        return name;
    }

    std::string const &get_name() const
    {
        return name;
    }

    /**
     * Get attribute data type
     *
     * @return The attribute data type
     */
    long get_data_type()
    {
        return data_type;
    }

    /**
     * Get attribute data format
     *
     * @return The attribute data format
     */
    Tango::AttrDataFormat get_data_format()
    {
        return data_format;
    }

    /**
     * Get name of the associated writable attribute
     *
     * @return The associated writable attribute name
     */
    std::string &get_assoc_name()
    {
        return writable_attr_name;
    }

    /**
     * Get index of the associated writable attribute
     *
     * @return The index in the main attribute vector of the associated writable
     * attribute
     */
    long get_assoc_ind() const
    {
        return assoc_ind;
    }

    /**
     * Set index of the associated writable attribute
     *
     * @param val The new index in the main attribute vector of the associated writable
     * attribute
     */
    void set_assoc_ind(long val)
    {
        assoc_ind = val;
    }

    /**
     * Get attribute date
     *
     * @return The attribute date
     */
    Tango::TimeVal &get_date()
    {
        return when;
    }

    /**
     * Set attribute date
     *
     * @param new_date The attribute date
     */
    void set_date(Tango::TimeVal &new_date)
    {
        when = new_date;
    }

    /**
     * Set attribute date
     *
     * @param The attribute date
     */
    void set_date(const TangoTimestamp &t)
    {
        when = make_TimeVal(t);
    }

    /**
     * Set attribute date
     *
     * @param new_date The attribute date
     */
    void set_date(time_t new_date)
    {
        when.tv_sec = (long) new_date;
        when.tv_usec = 0;
        when.tv_nsec = 0;
    }

    /**
     * Get attribute label property
     *
     * @return The attribute label
     */
    std::string &get_label()
    {
        return label;
    }

    /**
     * Get attribute data quality
     *
     * @return The attribute data quality
     */
    Tango::AttrQuality &get_quality()
    {
        return quality;
    }

    /**
     * Set attribute data quality
     *
     * @param qua    The new attribute data quality
     * @param send_event Boolean set to true if a change event should be sent
     */
    void set_quality(Tango::AttrQuality qua, bool send_event = false);

    /**
     * Get attribute data size
     *
     * @return The attribute data size
     */
    long get_data_size()
    {
        TANGO_ASSERT(data_size <= std::uint32_t((std::numeric_limits<long>::max)()));
        return static_cast<long>(data_size);
    }

    /**
     * Get attribute data size in x dimension
     *
     * @return The attribute data size in x dimension. Set to 1 for scalar attribute
     */
    long get_x()
    {
        return dim_x;
    }

    /**
     * Get attribute maximum data size in x dimension
     *
     * @return The attribute maximum data size in x dimension. Set to 1 for scalar attribute
     */
    long get_max_dim_x()
    {
        return max_x;
    }

    /**
     * Get attribute data size in y dimension
     *
     * @return The attribute data size in y dimension. Set to 0 for scalar and
     * spectrum attribute
     */
    long get_y()
    {
        return dim_y;
    }

    /**
     * Get attribute maximum data size in y dimension
     *
     * @return The attribute maximum data size in y dimension. Set to 0 for scalar and
     * spectrum attribute
     */
    long get_max_dim_y()
    {
        return max_y;
    }

    /**
     * Get attribute polling period
     *
     * @return The attribute polling period in mS. Set to 0 when the attribute is
     * not polled
     */
    long get_polling_period()
    {
        return poll_period;
    }

    /**
     * Get all modifiable attribute properties in one call
     *
     * This method initializes the members of a MultiAttrProp object with the modifiable
     * attribute properties values
     *
     * @param props A MultiAttrProp object.
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void get_properties(MultiAttrProp<T> &);
    /**
     * Set all modifiable attribute properties in one call
     *
     * This method sets the modifiable attribute properties with the values
     * provided as members of MultiAttrProps object
     *
     * @param props A MultiAttrProp object.
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_properties(const MultiAttrProp<T> &);
    /**
     * Set attribute serialization model
     *
     * This method allows the user to choose the attribute serialization
     * model.
     *
     * @param ser_model The new serialisation model. The serialization model must be
     * one of ATTR_BY_KERNEL, ATTR_BY_USER or ATTR_NO_SYNC
     */
    void set_attr_serial_model(AttrSerialModel ser_model);

    /**
     * Get attribute serialization model
     *
     * Get the attribute serialization model
     *
     * @return The attribute serialization model
     */
    AttrSerialModel get_attr_serial_model()
    {
        return attr_serial_model;
    }

    /**
     * Set attribute user mutex
     *
     * This method allows the user to give to the attribute object the pointer to
     * the omni_mutex used to protect its buffer. The mutex has to be locked when passed
     * to this method. The Tango kernel will unlock it when the data will be transferred
     * to the client.
     *
     * @param mut_ptr The user mutex pointer
     */
    void set_user_attr_mutex(omni_mutex *mut_ptr)
    {
        ext->user_attr_mutex = mut_ptr;
    }

    //@}

    /**@name Set attribute value methods.
     * These methods allows the external world to set attribute object internal
     * value
     */
    //@{

    /**
     * This method stores the attribute value inside the Attribute object and set current time as readout time
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     *
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data to be allocated with operator new for SCALAR attributes and operator new[]
     * otherwise. For the DevString's, when release is true we expect value to have been allocated with
     * Tango::string_dup or equivalent.
     *
     * @param T *p_data: pointer to value
     * @param long x=1: the attribute x dimension
     * @param long y=0: the attribute y dimension
     * @param bool release: The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     * @exception DevFailed If the attribute data type is not coherent, enum labels are not defined, wrong size of data,
     * or wrong Enum value.
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_value(T *p_data, long x = 1, long y = 0, bool release = false);

    /**
     * This method stores the attribute value inside the Attribute object and set current time as readout time
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     *
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data to be allocated with operator new for SCALAR attributes and operator new[]
     * otherwise. For the DevString's, when release is true we expect value to have been allocated with
     * Tango::string_dup or equivalent.
     *
     * @param Tango::DevEncoded *p_data: pointer to value
     * @param long x=1: the attribute x dimension
     * @param long y=0: the attribute y dimension
     * @param bool release: The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     * @exception DevFailed If the attribute data type is not coherent, enum labels are not defined, wrong size of data,
     * or wrong Enum value.
     */
    void set_value(Tango::DevEncoded *p_data, long x = 1, long y = 0, bool release = false);

    /**
     * This is special realization for scalar C++11 scoped enum with short as underlying data type or old enum of
     * set_value method to store the attribute value inside the Attribute object, and set current time as readout time
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data to be allocated with operator new for SCALAR attributes and operator new[]
     * otherwise.
     *
     * @param T *enum_ptr: pointer to Enum value
     * @param long x=1: the attribute x dimension
     * @param long y=0: the attribute y dimension
     * @param bool release: The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     *
     * @exception DevFailed If the attribute data type is not coherent or data size exceed attribute length..
     */
    template <class T, std::enable_if_t<std::is_enum_v<T> && !std::is_same_v<T, Tango::DevState>, T> * = nullptr>
    void set_value(T *enum_ptr, long x = 1, long y = 0, bool release = false);

    /**
     * This is special realization for Tango::DevEncoded attribute data type of
     * set_value method to store the attribute value inside the Attribute object, set current time as readout time
     * and calculate attribute quality factor (if warning/alarm levels are defined)
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     *
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data_str  have been allocated with Tango::string_dup or equivalent
     * and p_data with operator new[].
     *
     * @param p_data_str The attribute string part read value
     * @param p_data The attribute raw data part read value
     * @param size Size of the attribute raw data part
     * @param release The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     * @exception DevFailed If the attribute data type is not coherent
     */
    void set_value(Tango::DevString *p_data_str, Tango::DevUChar *p_data, long size, bool release = false);

    /**
     * This is special realization for Tango::EncodedAttribute attribute data type of
     * set_value method to store the attribute value inside the Attribute object
     *
     * The value will be later send to the client either as result of attribute readout of event value
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data to be allocated with operator new.
     *
     * @param attr pointer to to EncodedAttribute object
     * @exception DevFailed If the attribute data type is not coherent.
     */
    void set_value(Tango::EncodedAttribute *attr);

    /**
     * This method stores the attribute value inside the Attribute object, with readout time and quality, provided by
     * user
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     *
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data to be allocated with operator new for SCALAR attributes and operator new[]
     * otherwise. For the DevString's, when release is true we expect value to have been allocated with
     * Tango::string_dup or equivalent.
     *
     * @param T *p_data: pointer to value
     * @param time_t time: readout time
     * @param Tango::AttrQuality quality: the attribute quality
     * @param long x=1: the attribute x dimension
     * @param long y=0: the attribute y dimension
     * @param bool release: The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     * @exception DevFailed If the attribute data type is not coherent.
     */
    template <class T>
    void set_value_date_quality(
        T *p_data, time_t time, Tango::AttrQuality quality, long x = 1, long y = 0, bool rel = false);

    /**
     * This method stores the attribute value inside the Attribute object, with readout time and quality, provided by
     * user
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     *
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data to be allocated with operator new for SCALAR attributes and operator new[]
     * otherwise. For the DevString's, when release is true we expect value to have been allocated with
     * Tango::string_dup or equivalent.
     *
     * @param T *p_data: pointer to value
     * @param const TangoTimestamp time: readout time
     * @param Tango::AttrQuality quality: the attribute quality
     * @param long x=1: the attribute x dimension
     * @param long y=0: the attribute y dimension
     * @param bool release: The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     * @exception DevFailed If the attribute data type is not coherent.
     */
    template <class T>
    void set_value_date_quality(
        T *p_data, const TangoTimestamp &time, Tango::AttrQuality quality, long x = 1, long y = 0, bool rel = false);

    /**
     * This is special realization for Tango::DevEncoded attribute data type of
     * set_value_date_quality method to store the attribute value inside the Attribute object,
     * with readout time attribute quality, provided by user
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     *
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data_str  have been allocated with Tango::string_dup or equivalent
     * and p_data with operator new[].
     *
     * @param Tango::DevString *p_data_str: the attribute string part read value
     * @param Tango::DevUChar *p_data: the attribute raw data part read value
     * @param long size: size of raw data part
     * @param time_t time: readout time
     * @param Tango::AttrQuality quality: the attribute quality
     * @param bool release: The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     * @exception DevFailed If the attribute data type is not coherent.
     */
    void set_value_date_quality(Tango::DevString *p_data_str,
                                Tango::DevUChar *p_data,
                                long size,
                                time_t t,
                                Tango::AttrQuality qual,
                                bool release = false);

    /**
     * This is special realization for Tango::DevEncoded attribute data type of
     * set_value_date_quality method to store the attribute value inside the Attribute object,
     * with readout time attribute quality, provided by user
     *
     * The value will be later send to the client either as result of attribute readout of event value,
     * or utilized to define current device State (when reading device State and warning/alarm levels are defined)
     *
     * After readout value will be destroyed.
     *
     * When release is true, we expect p_data_str  have been allocated with Tango::string_dup or equivalent
     * and p_data with operator new[].
     *
     * @param Tango::DevString *p_data_str: The attribute string part read value
     * @param Tango::DevUChar *p_data: The attribute raw data part read value
     * @param long size: size of raw data part
     * @param TangoTimestamp time: readout time
     * @param Tango::AttrQuality quality: the attribute quality
     * @param bool release: The release flag. If true, memory pointed to by p_data will be
     *           freed after being send to the client. Default value is false.
     * @exception DevFailed If the attribute data type is not coherent.
     */
    void set_value_date_quality(Tango::DevString *p_data_str,
                                Tango::DevUChar *p_data,
                                long size,
                                const TangoTimestamp &,
                                Tango::AttrQuality qual,
                                bool release = false);

    //---------------------------------------------------------------------------

    /**
     * Fire a change event for the attribute value. The event is pushed to the notification
     * daemon.
     * The attribute data must be set with one of the Attribute::set_value or
     * Attribute::setvalue_date_quality methods before fireing the event.
     * The event is triggered with or without the change event criteria depending
     * on the configuration choosen with set_change_event().
     * ATTENTION: The couple set_value() and fire_change_event() needs to be protected
     * against concurrent accesses to the same attribute. Such an access might happen during
     * a synchronous read or by a reading from the polling thread.
     * Inside all methods reading or writing commands and attributes this protection is
     * automatically done by the Tango serialisation monitor.
     * When fireing change events in your own code, you should use the push_change_event
     * methods of the DeviceImpl class or protect your code with the
     * Tango::AutoTangoMonitor on your device.
     * Example:
     *
     *    {
     *         Tango::AutoTangoMonitor synch(this);
     *        att_temp_seq.set_value (temp_seq, 100);
     *         att_temp_seq.fire_archive_event ();
     *    }
     *
     * @param except A pointer to a DevFailed exception to be thrown as archive event.
     */
    void fire_change_event(DevFailed *except = nullptr);

    /**
     * Set a flag to indicate that the server fires change events manually, without
     * the polling to be started for the attribute.
     * If the detect parameter is set to true, the criteria specified for the change
     * event are verified and the event is only pushed if they are fulfilled.
     * If detect is set to false the event is fired without any value checking!
     *
     * @param implemented True when the server fires change events manually.
     * @param detect Triggers the verification of the change event properties when set to true.
     */
    void set_change_event(bool implemented, bool detect = true)
    {
        change_event_implmented = implemented;
        check_change_event_criteria = detect;
        if(!detect)
        {
            prev_change_event.err = false;
            prev_change_event.quality = Tango::ATTR_VALID;
        }
    }

    /**
     * Check if the change event is fired manually (without polling) for this attribute.
     *
     * @return A boolean set to true if a manual fire change event is implemented.
     */
    bool is_change_event()
    {
        return change_event_implmented;
    }

    /**
     * Check if the change event criteria should be checked when firing
     * the event manually.
     *
     * @return A boolean set to true if a change event criteria will be checked.
     */
    bool is_check_change_criteria()
    {
        return check_change_event_criteria;
    }

    /**
     * Fire an alarm event for the attribute value. The event is pushed to ZMQ.
     *
     * The attribute data must be set with one of the Attribute::set_value or
     * Attribute::setvalue_date_quality methods before firing the event.
     * ATTENTION: The couple set_value() and fire_alarm_event() needs to be
     * protected against concurrent accesses to the same attribute. Such an access
     * might happen during a synchronous read or by a reading from the polling
     * thread.
     * Inside all methods reading or writing commands and attributes this protection is automatically done by the Tango
     * serialisation monitor.
     *
     * @param except A pointer to a DevFailed exception to be thrown as alarm event.
     */
    void fire_alarm_event(DevFailed *except = nullptr);

    /**
     * Set a flag to indicate that the server fires alarm events manually, without
     * the polling to be started for the attribute.
     * If the detect parameter is set to true, the criteria specified for the alarm
     * event are verified and the event is only pushed if they are fulfilled.
     * If detect is set to false the event is fired without any value checking!
     *
     * @param implemented True when the server fires alarm events manually.
     * @param detect Triggers the verification of the alarm event properties when set to true.
     */
    void set_alarm_event(bool implemented, bool detect = true)
    {
        alarm_event_implmented = implemented;
        check_alarm_event_criteria = detect;
        if(!detect)
        {
            prev_alarm_event.err = false;
            prev_alarm_event.quality = Tango::ATTR_VALID;
        }
    }

    /**
     * Check if the alarm event is fired manually (without polling) for this attribute.
     *
     * @return A boolean set to true if a manual fire alarm event is implemented.
     */
    bool is_alarm_event()
    {
        return alarm_event_implmented;
    }

    /**
     * Check if the alarm event criteria should be checked when firing
     * the event manually.
     *
     * @return A boolean set to true if an alarm event criteria will be checked.
     */
    bool is_check_alarm_criteria()
    {
        return check_alarm_event_criteria;
    }

    /**
     * Fire an archive event for the attribute value. The event is pushed to the notification
     * daemon.
     * The attribute data must be set with one of the Attribute::set_value or
     * Attribute::setvalue_date_quality methods before fireing the event.
     * The event is triggered with or without the archive event criteria depending
     * on the configuration choosen with set_archive_event().
     * ATTENTION: The couple set_value() and fire_archive_event() needs to be protected
     * against concurrent accesses to the same attribute. Such an access might happen during
     * a synchronous read or by a reading from the polling thread.
     * Inside all methods reading or writing commands and attributes this protection is
     * automatically done by the Tango serialisation monitor.
     * When fireing archive events in your own code, you should use the push_archive_event
     * methods of the DeviceImpl class or protect your code with the
     * Tango::AutoTangoMonitor on your device.
     * Example:
     *
     *    {
     *         Tango::AutoTangoMonitor synch(this);
     *        att_temp_seq.set_value (temp_seq, 100);
     *         att_temp_seq.fire_archive_event ();
     *    }
     *
     * @param except A pointer to a DevFailed exception to be thrown as archive event.
     */
    void fire_archive_event(DevFailed *except = nullptr);

    /**
     * Set a flag to indicate that the server fires archive events manually, without
     * the polling to be started for the attribute
     * If the detect parameter is set to true, the criteria specified for the archive
     * event are verified and the event is only pushed if they are fulfilled.
     * If detect is set to false the event is fired without any value checking!
     *
     * @param implemented True when the server fires archive events manually.
     * @param detect Triggers the verification of the archive event properties when set to true.
     */
    void set_archive_event(bool implemented, bool detect = true)
    {
        archive_event_implmented = implemented;
        check_archive_event_criteria = detect;
        if(!detect)
        {
            prev_archive_event.err = false;
            prev_archive_event.quality = Tango::ATTR_VALID;
        }
    }

    /**
     * Check if the archive event is fired manually for this attribute.
     *
     * @return A boolean set to true if a manual fire archive event is implemented.
     */
    bool is_archive_event()
    {
        return archive_event_implmented;
    }

    /**
     * Check if the archive event criteria should be checked when firing
     * the event manually.
     *
     * @return A boolean set to true if a archive event criteria will be checked.
     */
    bool is_check_archive_criteria()
    {
        return check_archive_event_criteria;
    }

    /**
     * Set a flag to indicate that the server fires data ready events
     *
     * @param implemented True when the server fires change events manually.
     */
    void set_data_ready_event(bool implemented)
    {
        dr_event_implmented = implemented;
    }

    /**
     * Check if the data ready event is fired for this attribute.
     *
     * @return A boolean set to true if a fire data ready event is implemented.
     */
    bool is_data_ready_event()
    {
        return dr_event_implmented;
    }

    /**
     * Fire a user event for the attribute value. The event is pushed to the notification
     * daemon.
     * The attribute data must be set with one of the Attribute::set_value or
     * Attribute::setvalue_date_quality methods before fireing the event.
     * ATTENTION: The couple set_value() and fire_event() needs to be protected
     * against concurrent accesses to the same attribute. Such an access might happen during
     * a synchronous read or by a reading from the polling thread.
     * Inside all methods reading or writing commands and attributes this protection is
     * automatically done by the Tango serialisation monitor.
     * When fireing archive events in your own code, you should use the push_event
     * methods of the DeviceImpl class or protect your code with the
     * Tango::AutoTangoMonitor on your device.
     * Example:
     *
     *    {
     *         Tango::AutoTangoMonitor synch(this);
     *        att_temp_seq.set_value (temp_seq, 100);
     *         att_temp_seq.fire_event ();
     *    }
     *
     * @param filt_names The filterable fields name
     * @param filt_vals The filterable fields value (as double)
     * @param except A pointer to a DevFailed exception to be thrown as archive event.
     */
    void fire_event(const std::vector<std::string> &filt_names,
                    const std::vector<double> &filt_vals,
                    DevFailed *except = nullptr);

    /**
     * Remove the attribute configuration from the database.
     * This method can be used to clean-up all the configuration of an attribute to come back to
     * its default values or the remove all configuration of a dynamic attribute before deleting it.
     *
     * The method removes all configured attribute properties and removes the attribute from the
     * list of polled attributes.
     *
     * @exception DevFailed In case of database access problems.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void remove_configuration();
    //@}

    /**@name Set/Get attribute ranges (min_alarm, min_warning, max_warning, max_alarm) methods.
     * These methods allow the external world to set attribute object min_alarm, min_warning,
     * max_warning and max_alarm values
     */
    //@{

    /**
     * Set attribute minimum alarm.
     *
     * This method sets the attribute minimum alarm.
     *
     * @param new_min_alarm The new attribute minimum alarm value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_min_alarm(const T &new_min_alarm);
    void set_min_alarm(const std::string &new_min_alarm);

    /**
     * Set attribute minimum alarm.
     *
     * This method sets the attribute minimum alarm.
     *
     * @param new_min_alarm The new attribute minimum alarm value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_min_alarm(char *new_min_alarm);
    /**
     * Set attribute minimum alarm.
     *
     * This method sets the attribute minimum alarm.
     *
     * @param new_min_alarm The new attribute minimum alarm value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_min_alarm(const char *new_min_alarm);

    /**
     * Get attribute minimum alarm or throw an exception if the attribute
     * does not have the minimum alarm
     *
     * @param min_al Reference to a variable which value will be set to the attribute's
     * minimum alarm
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void get_min_alarm(T &min_al);

    /**
     * Set attribute maximum alarm.
     *
     * This method sets the attribute maximum alarm.
     *
     * @param new_max_alarm The new attribute maximum alarm value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_max_alarm(const T &new_max_alarm);
    void set_max_alarm(const std::string &new_max_alarm);

    /**
     * Set attribute maximum alarm.
     *
     * This method sets the attribute maximum alarm.
     *
     * @param new_max_alarm The new attribute maximum alarm value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_max_alarm(char *new_max_alarm);
    /**
     * Set attribute maximum alarm.
     *
     * This method sets the attribute maximum alarm.
     *
     * @param new_max_alarm The new attribute maximum alarm value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_max_alarm(const char *new_max_alarm);

    /**
     * Get attribute maximum alarm or throw an exception if the attribute
     * does not have the maximum alarm set
     *
     * @param max_al Reference to a variable which value will be set to the attribute's
     * maximum alarm
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void get_max_alarm(T &max_al);

    /**
     * Set attribute minimum warning.
     *
     * This method sets the attribute minimum warning.
     *
     * @param new_min_warning The new attribute minimum warning value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_min_warning(const T &new_min_warning);
    void set_min_warning(const std::string &new_min_warning);

    /**
     * Set attribute minimum warning.
     *
     * This method sets the attribute minimum warning.
     *
     * @param new_min_warning The new attribute minimum warning value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_min_warning(char *new_min_warning);
    /**
     * Set attribute minimum warning.
     *
     * This method sets the attribute minimum warning.
     *
     * @param new_min_warning The new attribute minimum warning value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_min_warning(const char *new_min_warning);

    /**
     * Get attribute minimum warning or throw an exception if the attribute
     * does not have the minimum warning set
     *
     * @param min_war Reference to a variable which value will be set to the attribute's
     * minimum warning
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void get_min_warning(T &min_warning);

    /**
     * Set attribute maximum warning.
     *
     * This method sets the attribute maximum warning.
     *
     * @param new_max_warning The new attribute maximum warning value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void set_max_warning(const T &new_max_warning);
    void set_max_warning(const std::string &new_max_warning);

    /**
     * Set attribute maximum warning.
     *
     * This method sets the attribute maximum warning.
     *
     * @param new_max_warning The new attribute maximum warning value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_max_warning(char *new_max_warning);
    /**
     * Set attribute maximum warning.
     *
     * This method sets the attribute maximum warning.
     *
     * @param new_max_warning The new attribute maximum warning value
     * @exception DevFailed If the attribute data type is not coherent.
     * Click <a href="https://tango-controls.readthedocs.io/en/latest/development/advanced/IDL.html#exceptions">here</a>
     * to read <b>DevFailed</b> exception specification
     */
    void set_max_warning(const char *new_max_warning);

    /**
     * Get attribute maximum warning or throw an exception if the attribute
     * does not have the maximum warning set
     *
     * @param max_war Reference to a variable which value will be set to the attribute's
     * maximum warning
     */
    template <class T, std::enable_if_t<Tango::is_tango_base_type_v<T>> * = nullptr>
    void get_max_warning(T &max_warning);
    //@}

  protected:
    /**@name Class data members */
    //@{
    /**
     * The date when attribute was read
     */
    Tango::TimeVal when;
    /**
     * Flag set to true if the date must be set
     */
    bool date{true};
    /**
     * The attribute quality factor
     */
    Tango::AttrQuality quality{Tango::ATTR_VALID};

    /**
     * The attribute name
     */
    std::string name;
    /**
     * The attribute writable flag
     */
    Tango::AttrWriteType writable;
    /**
     * The attribute data type.
     *
     * Forteen types are suported. They are Tango::DevShort, Tango::DevUShort, Tango::DevLong, Tango::DevULong,
     * Tango::DevLong64, Tango::DevULong64, Tango::DevDouble, Tango::DevString, , Tango::DevUChar,
     * Tango::DevFloat, Tango::DevBoolean, Tango::DevState, Tango::DevEncoded and Tango::DevEnum
     */
    long data_type;
    /**
     * The attribute data format.
     *
     * Three data formats are supported. They are SCALAR, SPECTRUM and IMAGE
     */
    Tango::AttrDataFormat data_format;
    /**
     * The attribute maximum x dimension.
     *
     * It is needed for SPECTRUM or IMAGE data format
     */
    long max_x;
    /**
     * The attribute maximum y dimension.
     *
     * It is necessary only for IMAGE data format
     */
    long max_y;
    /**
     * The attribute label
     */
    std::string label;
    /**
     * The attribute description
     */
    std::string description;
    /**
     * The attribute unit
     */
    std::string unit;
    /**
     * The attribute standard unit
     */
    std::string standard_unit;
    /**
     * The attribute display unit
     */
    std::string display_unit;
    /**
     * The attribute format.
     *
     * This string specifies how an attribute value must be printed
     */
    std::string format;
    /**
     * The name of the associated writable attribute
     */
    std::string writable_attr_name;
    /**
     * The attribute minimum alarm level
     */
    std::string min_alarm_str;
    /**
     * The attribute maximun alarm level
     */
    std::string max_alarm_str;
    /**
     * The attribute minimum value
     */
    std::string min_value_str;
    /**
     * The attribute maximum value
     */
    std::string max_value_str;
    /**
     * The attribute minimun  warning
     */
    std::string min_warning_str;
    /**
     * The attribute maximum warning
     */
    std::string max_warning_str;
    /**
     * The attribute delta value RDS alarm
     */
    std::string delta_val_str;
    /**
     * The attribute delta time RDS alarm
     */
    std::string delta_t_str;
    /**
     * Index in the main attribute vector of the associated writable attribute (if any)
     */
    long assoc_ind;
    /**
     * The attribute minimum alarm in binary format
     */
    Tango::Attr_CheckVal min_alarm;
    /**
     * The attribute maximum alarm in binary format
     */
    Tango::Attr_CheckVal max_alarm;
    /**
     * The attribute minimum warning in binary format
     */
    Tango::Attr_CheckVal min_warning;
    /**
     * The attribute maximum warning in binary format
     */
    Tango::Attr_CheckVal max_warning;
    /**
     * The attribute minimum value in binary format
     */
    Tango::Attr_CheckVal min_value;
    /**
     * The attribute maximum value in binary format
     */
    Tango::Attr_CheckVal max_value;
    /**
     * The attribute value
     */
    Tango::AttrValue attribute_value;
    /**
     * The attribute data size
     */
    std::uint32_t data_size = 0;
    /**
     * Flag set to true if a minimum value is defined
     */
    bool check_min_value{false};
    /**
     * Flag set to true if a maximum alarm is defined
     */
    bool check_max_value{false};
    /**
     * Authorized delta between the last written value and the
     * actual read. Used if the attribute has an alarm on
     * Read Different Than Set (RDS)
     */
    Tango::Attr_CheckVal delta_val;
    /**
     * Delta time after which the read value must be checked again the
     * last written value if the attribute has an alarm on
     * Read Different Than Set (RDS)
     */
    long delta_t;
    /**
     * Enumeration labels when the attribute data type is DevEnum
     */
    std::vector<std::string> enum_labels;
    //@}

  public:
    /// @privatesection
    /**
     * Returns the internal buffer to keep data of this type.
     * Buffer is saved in as std::unique_ptr
     */
    template <class T>
    T *get_value_storage()
    {
        return attribute_value.get<T>(); // Temporary, non-owning access
    }

    //
    // methods not usable for the external world (outside the lib)
    //

    void get_properties(Tango::AttributeConfig &);
    void get_properties(Tango::AttributeConfig_2 &);
    void get_properties(Tango::AttributeConfig_3 &);
    void get_properties(Tango::AttributeConfig_5 &);

    void set_properties(const Tango::AttributeConfig &, const std::string &, bool, std::vector<AttPropDb> &);
    void set_properties(const Tango::AttributeConfig_3 &, const std::string &, bool, std::vector<AttPropDb> &);
    void set_properties(const Tango::AttributeConfig_5 &, const std::string &, bool, std::vector<AttPropDb> &);

    void upd_database(std::vector<AttPropDb> &);

    void get_prop(Tango::AttributeConfig_3 &_a)
    {
        get_properties(_a);
    }

    void get_prop(Tango::AttributeConfig_5 &_a)
    {
        get_properties(_a);
    }

    std::vector<std::string> &get_enum_labels()
    {
        return enum_labels;
    }

    template <typename T>
    void set_upd_properties(const T &_c)
    {
        set_upd_properties(_c, d_name);
    }

    void set_upd_properties(const AttributeConfig &);
    void set_upd_properties(const AttributeConfig_3 &);
    void set_upd_properties(const AttributeConfig_5 &);
    void set_upd_properties(const AttributeConfig &, const std::string &, bool f_s = false);
    void set_upd_properties(const AttributeConfig_3 &, const std::string &, bool f_s = false);
    void set_upd_properties(const AttributeConfig_5 &, const std::string &, bool f_s = false);

    template <typename T>
    void delete_data_if_needed(T *data, bool release);

    virtual void set_rvalue() { }

    void delete_seq();
    void delete_seq_and_reset_alarm();

    void wanted_date(bool flag)
    {
        date = flag;
    }

    bool get_wanted_date()
    {
        return date;
    }

    Tango::TimeVal &get_when()
    {
        return when;
    }

    void set_time();

    Tango::DevVarShortArray *get_short_value()
    {
        return attribute_value.get<Tango::DevVarShortArray>();
    }

    Tango::DevVarLongArray *get_long_value()
    {
        return attribute_value.get<Tango::DevVarLongArray>();
    }

    Tango::DevVarDoubleArray *get_double_value()
    {
        return attribute_value.get<Tango::DevVarDoubleArray>();
    }

    Tango::DevVarStringArray *get_string_value()
    {
        return attribute_value.get<Tango::DevVarStringArray>();
    }

    Tango::DevVarFloatArray *get_float_value()
    {
        return attribute_value.get<Tango::DevVarFloatArray>();
    }

    Tango::DevVarBooleanArray *get_boolean_value()
    {
        return attribute_value.get<Tango::DevVarBooleanArray>();
    }

    Tango::DevVarUShortArray *get_ushort_value()
    {
        return attribute_value.get<Tango::DevVarUShortArray>();
    }

    Tango::DevVarCharArray *get_uchar_value()
    {
        return attribute_value.get<Tango::DevVarCharArray>();
    }

    Tango::DevVarLong64Array *get_long64_value()
    {
        return attribute_value.get<Tango::DevVarLong64Array>();
    }

    Tango::DevVarULongArray *get_ulong_value()
    {
        return attribute_value.get<Tango::DevVarULongArray>();
    }

    Tango::DevVarULong64Array *get_ulong64_value()
    {
        return attribute_value.get<Tango::DevVarULong64Array>();
    }

    Tango::DevVarStateArray *get_state_value()
    {
        return attribute_value.get<Tango::DevVarStateArray>();
    }

    Tango::DevVarEncodedArray *get_encoded_value()
    {
        return attribute_value.get<Tango::DevVarEncodedArray>();
    }

    unsigned long get_name_size()
    {
        return name_size;
    }

    std::string &get_name_lower()
    {
        return name_lower;
    }

    void reset_value()
    {
        attribute_value.reset();
    }

    bool value_is_set() const
    {
        return attribute_value.has_value();
    }

    DispLevel get_disp_level()
    {
        return disp_level;
    }

    omni_mutex *get_attr_mutex()
    {
        return &(ext->attr_mutex);
    }

    omni_mutex *get_user_attr_mutex()
    {
        return ext->user_attr_mutex;
    }

    bool change_event_subscribed();
    bool alarm_event_subscribed();
    bool periodic_event_subscribed();
    bool archive_event_subscribed();
    bool user_event_subscribed();
    bool attr_conf_event_subscribed();
    bool data_ready_event_subscribed();

    bool use_notifd_event()
    {
        return notifd_event;
    }

    bool use_zmq_event()
    {
        return zmq_event;
    }

    //
    // Warning, methods below are not protected !
    //

    void set_change_event_sub(int);

    time_t get_change5_event_sub()
    {
        return event_change5_subscription;
    }

    void set_alarm_event_sub(int);

    time_t get_alarm6_event_sub()
    {
        return event_alarm6_subscription;
    }

    void set_periodic_event_sub(int);

    time_t get_periodic5_event_sub()
    {
        return event_periodic5_subscription;
    }

    void set_archive_event_sub(int);

    time_t get_archive5_event_sub()
    {
        return event_archive5_subscription;
    }

    void set_user_event_sub(int);

    time_t get_user5_event_sub()
    {
        return event_user5_subscription;
    }

    void set_att_conf_event_sub(int);

    void set_data_ready_event_sub()
    {
        event_data_ready_subscription = Tango::get_current_system_datetime();
    }

    time_t get_data_ready_event_sub()
    {
        return event_data_ready_subscription;
    }

    // End of warning

    void set_use_notifd_event()
    {
        notifd_event = true;
    }

    void set_use_zmq_event()
    {
        zmq_event = true;
    }

    long get_attr_idx()
    {
        return idx_in_attr;
    }

    void set_attr_idx(long new_idx)
    {
        idx_in_attr = new_idx;
    }

    //+-------------------------------------------------------------------------
    //
    // method :         Attribute::add_write_value
    //
    // description :    These methods add the associated writable attribute
    //                  value to the attribute value sequence
    //
    // in :    val : The associated write attribute value
    //
    //--------------------------------------------------------------------------

    void add_write_value(Tango::DevVarShortArray *val_ptr);
    void add_write_value(Tango::DevVarLongArray *val_ptr);
    void add_write_value(Tango::DevVarDoubleArray *val_ptr);
    void add_write_value(Tango::DevVarStringArray *val_ptr);
    void add_write_value(Tango::DevVarFloatArray *val_ptr);
    void add_write_value(Tango::DevVarBooleanArray *val_ptr);
    void add_write_value(Tango::DevVarUShortArray *val_ptr);
    void add_write_value(Tango::DevVarCharArray *val_ptr);
    void add_write_value(Tango::DevVarLong64Array *val_ptr);
    void add_write_value(Tango::DevVarULongArray *val_ptr);
    void add_write_value(Tango::DevVarULong64Array *val_ptr);
    void add_write_value(Tango::DevVarStateArray *val_ptr);
    void add_write_value(Tango::DevEncoded &val_ptr);

    DeviceImpl *get_att_device();

    template <typename T>
    void Attribute_2_AttributeValue_base(T *, Tango::DeviceImpl *);
    void Attribute_2_AttributeValue(Tango::AttributeValue_3 *, DeviceImpl *);
    void Attribute_2_AttributeValue(Tango::AttributeValue_4 *, DeviceImpl *);
    void Attribute_2_AttributeValue(Tango::AttributeValue_5 *, DeviceImpl *);

    template <typename T, typename V>
    void AttrValUnion_fake_copy(const T *, V *);
    template <typename T>
    void AttrValUnion_2_Any(const T *, CORBA::Any &);

    void AttributeValue_4_2_AttributeValue_3(const Tango::AttributeValue_4 *, Tango::AttributeValue_3 *);
    void AttributeValue_5_2_AttributeValue_3(const Tango::AttributeValue_5 *, Tango::AttributeValue_3 *);

    void AttributeValue_3_2_AttributeValue_4(const Tango::AttributeValue_3 *, Tango::AttributeValue_4 *);
    void AttributeValue_5_2_AttributeValue_4(const Tango::AttributeValue_5 *, Tango::AttributeValue_4 *);

    void AttributeValue_3_2_AttributeValue_5(const Tango::AttributeValue_3 *, Tango::AttributeValue_5 *);
    void AttributeValue_4_2_AttributeValue_5(const Tango::AttributeValue_4 *, Tango::AttributeValue_5 *);

    void AttributeConfig_5_2_AttributeConfig_3(const Tango::AttributeConfig_5 &, Tango::AttributeConfig_3 &);
    void AttributeConfig_3_2_AttributeConfig_5(const Tango::AttributeConfig_3 &, Tango::AttributeConfig_5 &);

    void AttributeConfig_5_2_AttributeConfig_3(const Tango::AttributeConfig_3 &, Tango::AttributeConfig_3 &) {
    } // Templ

    void AttributeConfig_3_2_AttributeConfig_5(const Tango::AttributeConfig_5 &, Tango::AttributeConfig_5 &) {
    } // Templ

    void set_mcast_event(const std::vector<std::string> &vs)
    {
        mcast_event.clear();
        copy(vs.begin(), vs.end(), back_inserter(mcast_event));
    }

    bool is_polled(DeviceImpl *);

    void set_polling_period(long per)
    {
        poll_period = per;
    }

    void save_alarm_quality()
    {
        old_quality = quality;
        old_alarm = alarm;
    }

    bool is_startup_exception()
    {
        return check_startup_exceptions;
    }

    void throw_startup_exception(const char *);

    bool is_mem_exception()
    {
        return att_mem_exception;
    }

    virtual bool is_fwd_att()
    {
        return false;
    }

    void set_client_lib(int, EventType);

    std::vector<int> &get_client_lib(EventType _et)
    {
        return client_lib[_et];
    }

    void remove_client_lib(int, const std::string &);

    void add_config_5_specific(AttributeConfig_5 &);
    void add_startup_exception(std::string, const DevFailed &);

    void fire_error_periodic_event(DevFailed *);

    /**
     * Extract internal value to dest.
     * Free internal memory.
     * @param dest, receiving Any that will contain the data.
     */
    void extract_value(CORBA::Any &dest);

    friend class EventSupplier;
    friend class ZmqEventSupplier;
    friend class DServer;

  private:
    /*
     * Implementations of methods, which add the associated writable attribute value
     * to the attribute value sequence
     */

    template <class T>
    void add_write_value_impl(T *val_ptr);
    void add_write_value_impl(Tango::DevVarStringArray *val_ptr);
    void add_write_value_impl(Tango::DevEncoded &val_ref);

    /**
     * Fire an event for the attribute value. The event is pushed to ZMQ.
     *
     * The attribute data must be set with one of the Attribute::set_value or
     * Attribute::set_value_date_quality methods before firing the event.
     * ATTENTION: The couple set_value() and fire_alarm_event() needs to be
     * protected against concurrent accesses to the same attribute. Such an access
     * might happen during a synchronous read or by a reading from the polling
     * thread.
     * Inside all methods reading or writing commands and attributes this protection is automatically done by the Tango
     * serialisation monitor.
     *
     * @param event_type Event type to be send.
     * @param except A pointer to a DevFailed exception to be thrown as alarm event.
     * @param should_delete_seq If true the delete_seq() will be called before
     * this returns
     */
    void generic_fire_event(const EventType &event_type,
                            DevFailed *except,
                            bool should_delete_seq = true,
                            std::vector<std::string> filterable_names = std::vector<std::string>(),
                            std::vector<double> filterable_data = std::vector<double>());

    /**
     * Extract internal value to dest depending on the type.
     * Free internal memory.
     * @param dest, receiving Any that will contain the data.
     */
    template <class T>
    void _extract_value(CORBA::Any &dest);

    void set_data_size();
    void throw_min_max_value(const std::string &, const std::string &, MinMaxValueCheck);
    void log_quality();
    void log_alarm_quality() const;

    void init_string_prop(std::vector<AttrProperty> &prop_list, std::string &attr, const char *attr_name)
    {
        try
        {
            attr = get_attr_value(prop_list, attr_name);
        }
        catch(DevFailed &e)
        {
            add_startup_exception(attr_name, e);
        }
    }

    bool is_value_set(const char *attr_name)
    {
        if(strcmp(attr_name, "min_alarm") == 0)
        {
            return alarm_conf.test(max_level);
        }
        else if(strcmp(attr_name, "max_alarm") == 0)
        {
            return alarm_conf.test(min_level);
        }
        else if(strcmp(attr_name, "min_value") == 0)
        {
            return check_max_value;
        }
        else if(strcmp(attr_name, "max_value") == 0)
        {
            return check_min_value;
        }
        else if(strcmp(attr_name, "min_warning") == 0)
        {
            return alarm_conf.test(max_warn);
        }
        else if(strcmp(attr_name, "max_warning") == 0)
        {
            return alarm_conf.test(min_warn);
        }
        else
        {
            return false;
        }
    }

    bool init_check_val_prop(std::vector<AttrProperty> &,
                             const std::string &,
                             const char *,
                             std::string &,
                             Tango::Attr_CheckVal &,
                             const Tango::Attr_CheckVal &);

    unsigned long name_size;
    std::string name_lower;
    DevEncoded enc_help;

    //+-------------------------------------------------------------------------
    //
    // method :         Attribute::general_check_alarm
    //
    // description :   These methods check if the attribute is out of alarm/warning level
    //                 and return a boolean set to true if the attribute out of the thresholds,
    //                 and it also set the attribute quality factor to ALARM/WARNING
    //
    // in :     const Tango::AttrQuality &alarm_type: type of alarm to check
    //          const T &max_value: max level threshold
    //          const T &min_value: min level threshold
    //
    // out :    bool: true if value out of the limits
    //
    //--------------------------------------------------------------------------

    template <typename T>
    bool general_check_alarm(const Tango::AttrQuality &alarm_type, const T &min_value, const T &max_value);

    // unfortunately DevEncoded is special and cannot be templated, but functionality is similar to general_check_alarm
    bool general_check_devencoded_alarm(const Tango::AttrQuality &alarm_type,
                                        const unsigned char &min_value,
                                        const unsigned char &max_value);

  protected:
    /// @privatesection

    //
    // The extension class
    //

    class AttributeExt
    {
      public:
        AttributeExt() { }

        omni_mutex attr_mutex;                // Mutex to protect the attributes shared data buffer
        omni_mutex *user_attr_mutex{nullptr}; // Ptr for user mutex in case he manages exclusion
    };

    AttributeExt *ext;

    virtual void init_opt_prop(std::vector<AttrProperty> &, const std::string &);
    virtual void init_event_prop(std::vector<AttrProperty> &, const std::string &, Attr &);
    void init_enum_prop(std::vector<AttrProperty> &);
    std::string &get_attr_value(std::vector<AttrProperty> &, const char *);
    long get_lg_attr_value(std::vector<AttrProperty> &, const char *);

    virtual bool check_rds_alarm()
    {
        return false;
    }

    bool check_level_alarm();
    bool check_warn_alarm();
    DeviceClass *get_att_device_class(const std::string &);

    template <typename T>
    void check_hard_coded_properties(const T &);

    template <typename T>
    void set_hard_coded_properties(const T &);

    void check_hard_coded(const AttributeConfig_5 &);

    void throw_hard_coded_prop(const char *);
    void validate_change_properties(const std::string &,
                                    const char *,
                                    std::string &,
                                    std::vector<double> &,
                                    std::vector<bool> &,
                                    std::vector<bool> &);
    void validate_change_properties(const std::string &, const char *, std::string &, std::vector<double> &);
    void set_format_notspec();
    bool is_format_notspec(const char *);
    void def_format_in_dbdatum(DbDatum &);

    void convert_prop_value(const char *, std::string &, Attr_CheckVal &, const std::string &);

    void db_access(const struct CheckOneStrProp &, const std::string &);
    void set_prop_5_specific(const AttributeConfig_5 &, const std::string &, bool, std::vector<AttPropDb> &);
    void build_check_enum_labels(const std::string &);

    void set_one_str_prop(const char *,
                          const CORBA::String_member &,
                          std::string &,
                          std::vector<AttPropDb> &,
                          const std::vector<AttrProperty> &,
                          const std::vector<AttrProperty> &,
                          const char *);
    void set_one_alarm_prop(const char *,
                            const CORBA::String_member &,
                            std::string &,
                            Tango::Attr_CheckVal &,
                            std::vector<AttPropDb> &,
                            const std::vector<AttrProperty> &,
                            const std::vector<AttrProperty> &,
                            bool &);
    void set_rds_prop(const AttributeAlarm &,
                      const std::string &,
                      std::vector<AttPropDb> &,
                      const std::vector<AttrProperty> &,
                      const std::vector<AttrProperty> &);
    void set_rds_prop_val(const AttributeAlarm &,
                          const std::string &,
                          const std::vector<AttrProperty> &,
                          const std::vector<AttrProperty> &);
    void set_rds_prop_db(const AttributeAlarm &,
                         std::vector<AttPropDb> &,
                         const std::vector<AttrProperty> &,
                         const std::vector<AttrProperty> &);
    void set_one_event_prop(const char *,
                            const CORBA::String_member &,
                            double *,
                            std::vector<AttPropDb> &,
                            const std::vector<AttrProperty> &,
                            const std::vector<AttrProperty> &);
    void event_prop_db_xxx(const std::vector<double> &,
                           const std::vector<double> &,
                           std::vector<AttPropDb> &,
                           AttPropDb &);
    void set_one_event_period(const char *,
                              const CORBA::String_member &,
                              int &,
                              const int &,
                              std::vector<AttPropDb> &,
                              const std::vector<AttrProperty> &,
                              const std::vector<AttrProperty> &);

    std::bitset<numFlags> alarm_conf;
    std::bitset<numFlags> alarm;

    long dim_x;
    long dim_y;

    std::vector<AttrProperty>::iterator pos_end;

    std::uint32_t enum_nb{0};     // For enum attribute
    short *loc_enum_ptr{nullptr}; // For enum attribute

    //
    // Ported from the extension class
    //

    Tango::DispLevel disp_level;      // Display level
    long poll_period{0};              // Polling period
    double rel_change[2];             // Delta for relative change events in %
    double abs_change[2];             // Delta for absolute change events
    double archive_rel_change[2];     // Delta for relative archive change events in %
    double archive_abs_change[2];     // Delta for absolute change events
    int event_period{0};              // Delta for periodic events in ms
    int archive_period{0};            // Delta for archive periodic events in ms
    long periodic_counter{0};         // Number of periodic events sent so far
    long archive_periodic_counter{0}; // Number of periodic events sent so far
    LastAttrValue prev_change_event;  // Last change attribute
    LastAttrValue prev_alarm_event;   // Last alarm event attribute
    LastAttrValue prev_archive_event; // Last archive attribute

    PollClock::time_point last_periodic;         // Last time a periodic event was detected
    PollClock::time_point archive_last_periodic; // Last time an archive periodic event was detected
    PollClock::time_point archive_last_event;    // Last time an archive event was detected (periodic or not)

    time_t event_change3_subscription; // Last time() a subscription was made
    time_t event_change4_subscription;
    time_t event_change5_subscription;
    time_t event_alarm6_subscription;    // Last time an alarm subscription was made.
    time_t event_periodic3_subscription; // Last time() a subscription was made
    time_t event_periodic4_subscription;
    time_t event_periodic5_subscription;
    time_t event_archive3_subscription; // Last time() a subscription was made
    time_t event_archive4_subscription;
    time_t event_archive5_subscription;
    time_t event_user3_subscription; // Last time() a subscription was made
    time_t event_user4_subscription;
    time_t event_user5_subscription;
    time_t event_attr_conf_subscription;  // Last time() a subscription was made
    time_t event_attr_conf5_subscription; // Last time() a subscription was made
    time_t event_data_ready_subscription; // Last time() a subscription was made

    long idx_in_attr;                        // Index in MultiClassAttribute vector
    std::string d_name;                      // The device name
    DeviceImpl *dev{nullptr};                // The device object
    bool change_event_implmented{false};     // Flag true if a manual fire change event is implemented.
    bool alarm_event_implmented{false};      // Flag true if a manual fire alarm event is implemented.
    bool archive_event_implmented{false};    // Flag true if a manual fire archive event is implemented.
    bool check_change_event_criteria{true};  // True if change event criteria should be checked when sending the event
    bool check_alarm_event_criteria{true};   // True if alarm event criteria should be checked when sending the event
    bool check_archive_event_criteria{true}; // True if change event criteria should be checked when sending the event
    AttrSerialModel attr_serial_model;       // Flag for attribute serialization model
    bool dr_event_implmented{false};         // Flag true if fire data ready event is implemented
    bool scalar_str_attr_release{false};     // Need memory freeing (scalar string attr, R/W att)
    bool notifd_event{false};                // Set to true if event required using notifd
    bool zmq_event{false};                   // Set to true if event required using ZMQ
    std::vector<std::string> mcast_event;    // In case of multicasting used for event transport
    AttrQuality old_quality;                 // Previous attribute quality
    std::bitset<numFlags> old_alarm;         // Previous attribute alarm
    std::map<std::string, DevFailed> startup_exceptions; // Map containing exceptions related to attribute configuration
                                                         // raised during the server startup sequence
    bool check_startup_exceptions{
        false}; // Flag set to true if there is at least one exception in startup_exceptions map
    bool startup_exceptions_clear{
        true};                     // Flag set to true when the cause for the device startup exceptions has been fixed
    bool att_mem_exception{false}; // Flag set to true if the attribute is writable and
                                   // memorized and if it failed at init
    std::vector<int> client_lib[numEventType]; // Clients lib used (for event sending and compat)
};

//
// Some inline methods
//

//+------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::throw_hard_coded_prop
//
// description :
//        Throw a "Hard coded properties can't be changed" exception
//
// args:
//        in :
//            - prop_name : The name of the property which should be modified
//
//--------------------------------------------------------------------------------------------------------------------

inline void Attribute::throw_hard_coded_prop(const char *prop_name)
{
    TangoSys_OMemStream desc;
    desc << "Attribute property " << prop_name << " is not changeable at run time" << std::ends;

    TANGO_THROW_EXCEPTION(API_AttrNotAllowed, desc.str());
}

//+-------------------------------------------------------------------------------------------------------------------
//
// method :
//        Attribute::throw_startup_exception
//
// description :
//        Throw a startup exception
//
// args:
//        in :
//            - origin : The method name where this method is called from
//
//-------------------------------------------------------------------------------------------------------------------

inline void Attribute::throw_startup_exception(const char *origin)
{
    if(check_startup_exceptions)
    {
        std::string err_msg;
        std::vector<std::string> event_exceptions;
        std::vector<std::string> opt_exceptions;
        for(auto it = startup_exceptions.begin(); it != startup_exceptions.end(); ++it)
        {
            if(it->first == "event_period" || it->first == "archive_period" || it->first == "rel_change" ||
               it->first == "abs_change" || it->first == "archive_rel_change" || it->first == "archive_abs_change")
            {
                event_exceptions.push_back(it->first);
            }
            else
            {
                opt_exceptions.push_back(it->first);
            }
            for(CORBA::ULong i = 0; i < it->second.errors.length(); i++)
            {
                std::string tmp_msg = std::string(it->second.errors[i].desc);
                size_t pos = tmp_msg.rfind('\n');
                if(pos != std::string::npos)
                {
                    tmp_msg.erase(0, pos + 1);
                }
                err_msg += "\n" + tmp_msg;
            }
        }
        err_msg = "\nDevice " + d_name + "-> Attribute : " + name + err_msg;

        if(event_exceptions.size() == startup_exceptions.size())
        {
            if(event_exceptions.size() == 1)
            {
                err_msg +=
                    "\nSetting a valid value (also 'NaN', 'Not specified' and '' - empty string) for any property for "
                    "this attribute will automatically bring the above-mentioned property to its library defaults";
            }
            else
            {
                err_msg +=
                    "\nSetting a valid value (also 'NaN', 'Not specified' and '' - empty string) for any property for "
                    "this attribute will automatically bring the above-listed properties to their library defaults";
            }
            err_msg += "\nHint : Check also class level attribute properties";
        }
        else if(!event_exceptions.empty())
        {
            if(opt_exceptions.size() == 1)
            {
                err_msg += "\nSetting valid value (also 'NaN', 'Not specified' and '' - empty string) for " +
                           opt_exceptions[0] + " ";
            }
            else
            {
                err_msg += "\nSetting valid values (also 'NaN', 'Not specified' and '' - empty string) for ";
                for(size_t i = 0; i < opt_exceptions.size(); i++)
                {
                    err_msg += ((i == (opt_exceptions.size() - 1) && i != 0) ? "and " : "") + opt_exceptions[i] +
                               ((i != (opt_exceptions.size() - 1) && i != (opt_exceptions.size() - 2)) ? "," : "") +
                               " ";
                }
            }
            err_msg += "will automatically bring ";
            for(size_t i = 0; i < event_exceptions.size(); i++)
            {
                err_msg += ((i == (event_exceptions.size() - 1) && i != 0) ? "and " : "") + event_exceptions[i] +
                           ((i != (event_exceptions.size() - 1) && i != (event_exceptions.size() - 2)) ? "," : "") +
                           " ";
            }
            if(event_exceptions.size() == 1)
            {
                err_msg += "to its library defaults";
            }
            else
            {
                err_msg += "to their library defaults";
            }

            err_msg += "\nHint : Check also class level attribute properties";
        }

        Except::throw_exception(API_AttrConfig, err_msg, origin);
    }
}

inline void Attribute::set_change_event_sub(int cl_lib)
{
    switch(cl_lib)
    {
    case 6:
    case 5:
        event_change5_subscription = Tango::get_current_system_datetime();
        break;

    case 4:
        event_change4_subscription = Tango::get_current_system_datetime();
        break;

    default:
        event_change3_subscription = Tango::get_current_system_datetime();
        break;
    }
}

inline void Attribute::set_alarm_event_sub(int cl_lib)
{
    switch(cl_lib)
    {
    case 6:
        event_alarm6_subscription = Tango::get_current_system_datetime();
        break;

    default:
        TANGO_THROW_EXCEPTION(API_ClientTooOld,
                              "Alarm events are only supported from client library version 6 onwards.");
        break;
    }
}

inline void Attribute::set_periodic_event_sub(int cl_lib)
{
    switch(cl_lib)
    {
    case 6:
    case 5:
        event_periodic5_subscription = Tango::get_current_system_datetime();
        break;

    case 4:
        event_periodic4_subscription = Tango::get_current_system_datetime();
        break;

    default:
        event_periodic3_subscription = Tango::get_current_system_datetime();
        break;
    }
}

inline void Attribute::set_archive_event_sub(int cl_lib)
{
    switch(cl_lib)
    {
    case 6:
    case 5:
        event_archive5_subscription = Tango::get_current_system_datetime();
        break;

    case 4:
        event_archive4_subscription = Tango::get_current_system_datetime();
        break;

    default:
        event_archive3_subscription = Tango::get_current_system_datetime();
        break;
    }
}

inline void Attribute::set_user_event_sub(int cl_lib)
{
    switch(cl_lib)
    {
    case 6:
    case 5:
        event_user5_subscription = Tango::get_current_system_datetime();
        break;

    case 4:
        event_user4_subscription = Tango::get_current_system_datetime();
        break;

    default:
        event_user3_subscription = Tango::get_current_system_datetime();
        break;
    }
}

inline void Attribute::set_att_conf_event_sub(int cl_lib)
{
    switch(cl_lib)
    {
    case 6:
    case 5:
        event_attr_conf5_subscription = Tango::get_current_system_datetime();
        break;

    default:
        event_attr_conf_subscription = Tango::get_current_system_datetime();
        break;
    }
}

//+-------------------------------------------------------------------------
//
// method :      Attribute::delete_data_if_needed
//
// description : The method frees the memory of the T*
//               attribute if the release = true
//
// in :          data : The attribute name
//               release : A flag set to true if memory must be
//                        de-allocated
//
//--------------------------------------------------------------------------
template <typename T>
inline void Attribute::delete_data_if_needed(T *data, bool release)
{
    if(!release || !data)
    {
        return;
    }

    if(is_fwd_att())
    {
        // Note that here we assume that the generated sequence class is inherited
        // from _CORBA_Sequence. This should be fixed once we have a mapping from
        // data types to sequence types.
        _CORBA_Sequence<T>::freebuf(data);
    }
    else
    {
        if(data_format == Tango::SCALAR)
        {
            delete data;
        }
        else
        {
            delete[] data;
        }
    }
}

//+-------------------------------------------------------------------------
//
// method :      Attribute::delete_data_if_needed
//
// description : Template specialization  which frees the memory of the
//               Tango::DevString* attribute if the release = true,
//               it is necessary due to the different allocation
//
// in :          data : The attribute name
//               release : A flag set to true if memory must be
//                        de-allocated
//
//--------------------------------------------------------------------------
template <>
inline void Attribute::delete_data_if_needed<Tango::DevString>(Tango::DevString *data, bool release)
{
    if(!release || (data == nullptr))
    {
        return;
    }

    if(is_fwd_att())
    {
        // p_data is the underlying buffer of DevVarStringArray
        // and it must be released using freebuf, not delete[].
        // Note that freebuf also releases memory for all array
        // elements. We assign null pointer to prevent that.
        *data = nullptr;
        Tango::DevVarStringArray::freebuf(data);
    }
    else
    {
        if(data_format == Tango::SCALAR)
        {
            delete data;
        }
        else
        {
            delete[] data;
        }
    }
}

/*
 * method: Attribute::set_value_date_quality
 *
 * These methods store the attribute value inside the Attribute object,
 * with readout time and attribute quality factor provided by user.
 *
 *
 */

template <class T>
inline void
    Attribute::set_value_date_quality(T *p_data, time_t t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);

    if(qual == Tango::ATTR_INVALID)
    {
        delete_seq();
    }
}

template <class T>
inline void Attribute::set_value_date_quality(
    T *p_data, const TangoTimestamp &t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);

    if(qual == Tango::ATTR_INVALID)
    {
        delete_seq();
    }
}

template <>
inline void Attribute::set_value_date_quality(
    Tango::DevEncoded *p_data, time_t t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);
}

template <>
inline void Attribute::set_value_date_quality(
    Tango::DevEncoded *p_data, const TangoTimestamp &t, Tango::AttrQuality qual, long x, long y, bool release)
{
    set_value(p_data, x, y, release);
    set_quality(qual, false);
    set_date(t);
}

//
// Macro to help coding
//

#define MEM_STREAM_2_CORBA(A, B)          \
    if(true)                              \
    {                                     \
        std::string s = B.str();          \
        A = Tango::string_dup(s.c_str()); \
        B.str("");                        \
        B.clear();                        \
    }                                     \
    else                                  \
        (void) 0

//
// Throw exception if pointer is null
//

#define CHECK_PTR(A, B)                                         \
    if(A == nullptr)                                            \
    {                                                           \
        std::stringstream o;                                    \
        o << "Data pointer for attribute " << B << " is NULL!"; \
        TANGO_THROW_EXCEPTION(Tango::API_AttrOptProp, o.str()); \
    }                                                           \
    else                                                        \
        (void) 0

//
// Yet another macros !!
// Arg list :     A : The sequence pointer
//        B : Index in sequence
//        C : Attribute reference
//

#define GIVE_ATT_MUTEX(A, B, C)                        \
    if(true)                                           \
    {                                                  \
        Tango::AttributeValue_4 *tmp_ptr = &((*A)[B]); \
        (tmp_ptr)->set_attr_mutex(C.get_attr_mutex()); \
    }                                                  \
    else                                               \
        (void) 0

#define GIVE_ATT_MUTEX_5(A, B, C)                      \
    if(true)                                           \
    {                                                  \
        Tango::AttributeValue_5 *tmp_ptr = &((*A)[B]); \
        (tmp_ptr)->set_attr_mutex(C.get_attr_mutex()); \
    }                                                  \
    else                                               \
        (void) 0

#define GIVE_USER_ATT_MUTEX(A, B, C)                        \
    if(true)                                                \
    {                                                       \
        Tango::AttributeValue_4 *tmp_ptr = &((*A)[B]);      \
        (tmp_ptr)->set_attr_mutex(C.get_user_attr_mutex()); \
    }                                                       \
    else                                                    \
        (void) 0

#define GIVE_USER_ATT_MUTEX_5(A, B, C)                      \
    if(true)                                                \
    {                                                       \
        Tango::AttributeValue_5 *tmp_ptr = &((*A)[B]);      \
        (tmp_ptr)->set_attr_mutex(C.get_user_attr_mutex()); \
    }                                                       \
    else                                                    \
        (void) 0

//
// Yet another macro !!
// Arg list :     A : The sequence pointer
//        B : Index in sequence
//        C : Attribute reference
//

#define REL_ATT_MUTEX(A, B, C)                         \
    if(C.get_attr_serial_model() != ATTR_NO_SYNC)      \
    {                                                  \
        Tango::AttributeValue_4 *tmp_ptr = &((*A)[B]); \
        (tmp_ptr)->rel_attr_mutex();                   \
    }                                                  \
    else                                               \
        (void) 0

#define REL_ATT_MUTEX_5(A, B, C)                       \
    if(C.get_attr_serial_model() != ATTR_NO_SYNC)      \
    {                                                  \
        Tango::AttributeValue_5 *tmp_ptr = &((*A)[B]); \
        (tmp_ptr)->rel_attr_mutex();                   \
    }                                                  \
    else                                               \
        (void) 0

} // namespace Tango
#endif // _ATTRIBUTE_H
