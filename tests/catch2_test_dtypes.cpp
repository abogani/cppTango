//
// Created by matveyev on 7/15/24.
//

#include <type_traits>
#include <vector>

#include <tango/tango.h>
#include "utils/utils.h"

#include "common.h"

static constexpr int N_ELEMENTS_IN_SPECTRUM_ATTRS = 5;

template <class T>
struct is_container : public std::false_type
{
    using underlying_type = void;
};

template <class T, class Alloc>
struct is_container<std::vector<T, Alloc>> : public std::true_type
{
    using underlying_type = T;
};

enum class TestEnum : short
{
    ONE = 0,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN
};

static const float NUMERIC_NORMAL_VALUE = 6.6;
static const float NUMERIC_MIN_WARNING_LEVEL = 4.4;
static const float NUMERIC_MIN_WARNING_VALUE = 3.3;
static const float NUMERIC_MAX_WARNING_LEVEL = 7.7;
static const float NUMERIC_MAX_WARNING_VALUE = 8.8;
static const float NUMERIC_MIN_ALARM_LEVEL = 2.2;
static const float NUMERIC_MIN_ALARM_VALUE = 1.1;
static const float NUMERIC_MAX_ALARM_LEVEL = 9.9;
static const float NUMERIC_MAX_ALARM_VALUE = 10.1;

static const int RDS_DELTA_T = 1;
static const float NUMERIC_RDS_DELTA = 10;
static const float NUMERIC_RDS_WRITE_VALUE = 25.5;

static const Tango::DevBoolean BOOLEAN_NORMAL_VALUE = true;
static const Tango::DevState STATE_NORMAL_VALUE = Tango::ON;
static const std::string STRING_NORMAL_VALUE = "Test string";

static const TestEnum ENUM_NORMAL_VALUE = TestEnum::SEVEN;

enum ValueToTest
{
    NORMAL,
    INVALID,
    MIN_ALARM,
    MIN_WARNING,
    MAX_WARNING,
    MAX_ALARM,
    RDS_WRITE,
    NUMERIC_LIMIT_MIN,
    RDS_OVERFLOW
};

std::string valueToTestToString(ValueToTest v)
{
    switch(v)
    {
    case ValueToTest::NORMAL:
        return "NORMAL";
    case ValueToTest::INVALID:
        return "INVALID";
    case ValueToTest::MIN_ALARM:
        return "MIN_ALARM";
    case ValueToTest::MIN_WARNING:
        return "MIN_WARNING";
    case ValueToTest::MAX_WARNING:
        return "MAX_WARNING";
    case ValueToTest::MAX_ALARM:
        return "MAX_ALARM";
    case ValueToTest::RDS_WRITE:
        return "RDS_WRITE";
    case ValueToTest::NUMERIC_LIMIT_MIN:
        return "NUMERIC_LIMIT_MIN";
    case ValueToTest::RDS_OVERFLOW:
        return "RDS_OVERFLOW";
    default:
        return "Unknown";
    }
}

namespace tango_traits
{
template <typename T, typename Enable = void>
struct is_scalar_state_boolean_string_enum : std::false_type
{
};

// Specialization for enums
template <typename T>
struct is_scalar_state_boolean_string_enum<T, std::enable_if_t<std::is_enum_v<T>>> : std::true_type
{
};

template <>
struct is_scalar_state_boolean_string_enum<Tango::DevState> : std::true_type
{
};

template <>
struct is_scalar_state_boolean_string_enum<Tango::DevBoolean> : std::true_type
{
};

template <>
struct is_scalar_state_boolean_string_enum<Tango::DevString> : std::true_type
{
};

template <>
struct is_scalar_state_boolean_string_enum<std::string> : std::true_type
{
};

template <>
struct is_scalar_state_boolean_string_enum<Tango::DevEnum> : std::true_type
{
};
} // namespace tango_traits

template <typename T>
void get_value_for_test(const ValueToTest &requested_value, T &value)
{
    switch(requested_value)
    {
    case ValueToTest::MIN_ALARM:
        value = static_cast<T>(NUMERIC_MIN_ALARM_VALUE);
        break;
    case ValueToTest::MIN_WARNING:
        value = static_cast<T>(NUMERIC_MIN_WARNING_VALUE);
        break;
    case ValueToTest::MAX_WARNING:
        value = static_cast<T>(NUMERIC_MAX_WARNING_VALUE);
        break;
    case ValueToTest::MAX_ALARM:
        value = static_cast<T>(NUMERIC_MAX_ALARM_VALUE);
        break;
    case ValueToTest::RDS_WRITE:
        value = static_cast<T>(NUMERIC_RDS_WRITE_VALUE);
        break;
    case ValueToTest::RDS_OVERFLOW:
        if constexpr(std::is_signed_v<T>)
        {
            value = std::numeric_limits<T>::max();
        }
        else
        {
            value = std::numeric_limits<T>::min() + 1;
        }
        break;
    case ValueToTest::NUMERIC_LIMIT_MIN:
        value = std::numeric_limits<T>::min();
        break;
    case ValueToTest::NORMAL:
    case ValueToTest::INVALID:
    default:
        value = static_cast<T>(NUMERIC_NORMAL_VALUE);
    }
}

template <>
void get_value_for_test<Tango::DevBoolean>(const ValueToTest &requested_value, Tango::DevBoolean &value)
{
    switch(requested_value)
    {
    case ValueToTest::NORMAL:
    case ValueToTest::INVALID:
        value = BOOLEAN_NORMAL_VALUE;
        break;
    default:
        throw std::runtime_error("Tango::DevBoolean does not have alarm thresholds");
    }
}

template <>
void get_value_for_test<Tango::DevState>(const ValueToTest &requested_value, Tango::DevState &value)
{
    switch(requested_value)
    {
    case ValueToTest::NORMAL:
    case ValueToTest::INVALID:
        value = STATE_NORMAL_VALUE;
        break;
    default:
        throw std::runtime_error("Tango::DevState does not have alarm thresholds");
    }
}

template <>
void get_value_for_test<std::string>(const ValueToTest &requested_value, std::string &value)
{
    switch(requested_value)
    {
    case ValueToTest::NORMAL:
    case ValueToTest::INVALID:
        value = STRING_NORMAL_VALUE;
        break;
    default:
        throw std::runtime_error("Tango::DevString does not have alarm thresholds");
    }
}

template <>
void get_value_for_test<Tango::DevString>(const ValueToTest &requested_value, Tango::DevString &value)
{
    switch(requested_value)
    {
    case ValueToTest::NORMAL:
    case ValueToTest::INVALID:
        value = const_cast<Tango::DevString>(STRING_NORMAL_VALUE.c_str());
        break;
    default:
        throw std::runtime_error("Tango::DevString does not have alarm thresholds");
    }
}

template <>
void get_value_for_test<TestEnum>(const ValueToTest &requested_value, TestEnum &value)
{
    switch(requested_value)
    {
    case ValueToTest::NORMAL:
    case ValueToTest::INVALID:
        value = ENUM_NORMAL_VALUE;
        break;
    default:
        throw std::runtime_error("Tango::DevEnum does not have alarm thresholds");
    }
}

template <>
void get_value_for_test<Tango::DevEncoded>(const ValueToTest &requested_value, Tango::DevEncoded &value)
{
    value.encoded_format = Tango::string_dup("Which format?");
    value.encoded_data.length(N_ELEMENTS_IN_SPECTRUM_ATTRS);
    for(int i = 1; i < N_ELEMENTS_IN_SPECTRUM_ATTRS; ++i)
    {
        value.encoded_data[i] = static_cast<unsigned char>(NUMERIC_NORMAL_VALUE);
    }

    switch(requested_value)
    {
    case ValueToTest::MIN_ALARM:
        value.encoded_data[0] = static_cast<unsigned char>(NUMERIC_MIN_ALARM_VALUE);
        break;
    case ValueToTest::MIN_WARNING:
        value.encoded_data[0] = static_cast<unsigned char>(NUMERIC_MIN_WARNING_VALUE);
        break;
    case ValueToTest::MAX_WARNING:
        value.encoded_data[0] = static_cast<unsigned char>(NUMERIC_MAX_WARNING_VALUE);
        break;
    case ValueToTest::MAX_ALARM:
        value.encoded_data[0] = static_cast<unsigned char>(NUMERIC_MAX_ALARM_VALUE);
        break;
    case ValueToTest::RDS_WRITE:
        value.encoded_data[0] = static_cast<unsigned char>(NUMERIC_RDS_WRITE_VALUE);
        break;
    case ValueToTest::NUMERIC_LIMIT_MIN:
        value.encoded_data[0] = std::numeric_limits<unsigned char>::min();
        break;
    case ValueToTest::RDS_OVERFLOW:
        value.encoded_data[0] = std::numeric_limits<unsigned char>::min() + 1;
        break;
    case ValueToTest::NORMAL:
    case ValueToTest::INVALID:
    default:
        value.encoded_data[0] = static_cast<unsigned char>(NUMERIC_NORMAL_VALUE);
    }
}

template <
    typename T,
    typename std::enable_if_t<(std::is_arithmetic_v<T> || tango_traits::is_scalar_state_boolean_string_enum<T>::value ||
                               std::is_same_v<T, Tango::DevEncoded>),
                              T> * = nullptr>
void get_value_quality_for_test(const ValueToTest &requested_value, T &value, Tango::AttrQuality &quality)
{
    get_value_for_test(requested_value, value);
    switch(requested_value)
    {
    case ValueToTest::MIN_ALARM:
    case ValueToTest::MAX_ALARM:
        quality = Tango::ATTR_ALARM;
        break;
    case ValueToTest::MIN_WARNING:
    case ValueToTest::MAX_WARNING:
        quality = Tango::ATTR_WARNING;
        break;
    case ValueToTest::INVALID:
        quality = Tango::ATTR_INVALID;
        break;
    case ValueToTest::RDS_WRITE:
        quality = Tango::ATTR_ALARM;
        break;
    case ValueToTest::NUMERIC_LIMIT_MIN:
    case ValueToTest::RDS_OVERFLOW:
    case ValueToTest::NORMAL:
    default:
        quality = Tango::ATTR_VALID;
        break;
    }
}

template <typename T,
          typename std::enable_if_t<(!std::is_arithmetic_v<T> &&
                                     !tango_traits::is_scalar_state_boolean_string_enum<T>::value &&
                                     !std::is_same_v<T, Tango::DevEncoded>),
                                    T> * = nullptr>
void get_value_quality_for_test(const ValueToTest &requested_value, T &value, Tango::AttrQuality &quality)
{
    value.resize(N_ELEMENTS_IN_SPECTRUM_ATTRS);

    for(int i = N_ELEMENTS_IN_SPECTRUM_ATTRS; i > 0; --i)
    {
        ValueToTest request = i == 1 ? requested_value : ValueToTest::NORMAL;
        get_value_quality_for_test(request, value[i - 1], quality);
    }
}

template <>
void get_value_quality_for_test<std::vector<Tango::DevBoolean>>(const ValueToTest &requested_value,
                                                                std::vector<Tango::DevBoolean> &value,
                                                                Tango::AttrQuality &quality)
{
    value.resize(N_ELEMENTS_IN_SPECTRUM_ATTRS);

    for(int i = N_ELEMENTS_IN_SPECTRUM_ATTRS; i > 0; --i)
    {
        Tango::DevBoolean temp_value = value[i - 1];
        ValueToTest request = i == 1 ? requested_value : ValueToTest::NORMAL;
        get_value_quality_for_test(request, temp_value, quality);
        value[i - 1] = temp_value;
    }
}

template <typename T>
void get_value_quality_for_test(const ValueToTest &requested_value, T *value, Tango::AttrQuality &quality)
{
    for(int i = 1; i < N_ELEMENTS_IN_SPECTRUM_ATTRS; ++i)
    {
        get_value_quality_for_test<T>(
            ValueToTest::NORMAL, value[i], quality); // have to force specialization due to ambiguous with DevString
    }
    get_value_quality_for_test<T>(requested_value, value[0], quality);
}

/*

 Server section

 */

template <typename T>
void set_scalar_attribute_value(Tango::Attribute &att, T &value_to_set, const Tango::AttrQuality &quality, bool release)
{
    if(release)
    {
        T *tmp_val = new T;
        *tmp_val = value_to_set;
        att.set_value_date_quality(tmp_val, 0, quality, 1, 0, true);
    }
    else
    {
        att.set_value_date_quality(&value_to_set, 0, quality, 1, 0, false);
    }
}

template <>
void set_scalar_attribute_value<Tango::DevString>(Tango::Attribute &att,
                                                  Tango::DevString &value_to_set,
                                                  const Tango::AttrQuality &quality,
                                                  bool release)
{
    if(release)
    {
        Tango::DevString *tmp_val = new Tango::DevString;
        *tmp_val = Tango::string_dup(value_to_set);
        att.set_value_date_quality(tmp_val, 0, quality, 1, 0, true);
    }
    else
    {
        att.set_value_date_quality(&value_to_set, 0, quality, 1, 0, false);
    }
}

template <typename T>
void set_spectrum_attribute_value(Tango::Attribute &att,
                                  T *value_to_set,
                                  const Tango::AttrQuality &quality,
                                  bool release)
{
    if(release)
    {
        T *tmp_val = new T[N_ELEMENTS_IN_SPECTRUM_ATTRS];
        std::copy(value_to_set, value_to_set + N_ELEMENTS_IN_SPECTRUM_ATTRS, tmp_val);
        att.set_value_date_quality(tmp_val, 0, quality, N_ELEMENTS_IN_SPECTRUM_ATTRS, 0, true);
    }
    else
    {
        att.set_value_date_quality(value_to_set, 0, quality, N_ELEMENTS_IN_SPECTRUM_ATTRS, 0, false);
    }
}

template <>
void set_spectrum_attribute_value<Tango::DevString>(Tango::Attribute &att,
                                                    Tango::DevString *value_to_set,
                                                    const Tango::AttrQuality &quality,
                                                    bool release)
{
    if(release)
    {
        Tango::DevString *tmp_val = new Tango::DevString[N_ELEMENTS_IN_SPECTRUM_ATTRS];
        for(int i = 0; i < N_ELEMENTS_IN_SPECTRUM_ATTRS; ++i)
        {
            tmp_val[i] = Tango::string_dup(value_to_set[i]);
        }
        att.set_value_date_quality(tmp_val, 0, quality, N_ELEMENTS_IN_SPECTRUM_ATTRS, 0, true);
    }
    else
    {
        att.set_value_date_quality(value_to_set, 0, quality, N_ELEMENTS_IN_SPECTRUM_ATTRS, 0, false);
    }
}

template <class Base>
class DtypeDev : public Base
{
  public:
    using Base::Base;

    ~DtypeDev() override { }

    void init_device() override
    {
        Base::set_state(Tango::ON);
    }

    void set_release_flag(bool val_in)
    {
        release_flag = val_in;
    }

    void set_tested_value(short int val_in)
    {
        tested_value = (ValueToTest) val_in;
    }

    void set_tested_attribute(Tango::DevString attr_name)
    {
        tested_attribute = attr_name;
    }

    void read_attribute(Tango::Attribute &att)
    {
        std::string my_name = att.get_name();

        ValueToTest requested_value = (my_name == tested_attribute) ? tested_value : ValueToTest::NORMAL;

        // Scalars

        if(my_name == "scalar_short" || my_name == "scalar_short_rds")
        {
            get_value_quality_for_test(requested_value, scalar_short, attr_quality);
            set_scalar_attribute_value(att, scalar_short, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_long" || my_name == "scalar_long_rds")
        {
            get_value_quality_for_test(requested_value, scalar_long, attr_quality);
            set_scalar_attribute_value(att, scalar_long, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_float" || my_name == "scalar_float_rds")
        {
            get_value_quality_for_test(requested_value, scalar_float, attr_quality);
            set_scalar_attribute_value(att, scalar_float, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_double" || my_name == "scalar_double_rds")
        {
            get_value_quality_for_test(requested_value, scalar_double, attr_quality);
            set_scalar_attribute_value(att, scalar_double, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_ushort" || my_name == "scalar_ushort_rds")
        {
            get_value_quality_for_test(requested_value, scalar_ushort, attr_quality);
            set_scalar_attribute_value(att, scalar_ushort, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_ulong" || my_name == "scalar_ulong_rds")
        {
            get_value_quality_for_test(requested_value, scalar_ulong, attr_quality);
            set_scalar_attribute_value(att, scalar_ulong, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_uchar" || my_name == "scalar_uchar_rds")
        {
            get_value_quality_for_test(requested_value, scalar_uchar, attr_quality);
            set_scalar_attribute_value(att, scalar_uchar, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_long64" || my_name == "scalar_long64_rds")
        {
            get_value_quality_for_test(requested_value, scalar_long64, attr_quality);
            set_scalar_attribute_value(att, scalar_long64, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_ulong64" || my_name == "scalar_ulong64_rds")
        {
            get_value_quality_for_test(requested_value, scalar_ulong64, attr_quality);
            set_scalar_attribute_value(att, scalar_ulong64, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_state")
        {
            get_value_quality_for_test(requested_value, scalar_state, attr_quality);
            set_scalar_attribute_value(att, scalar_state, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_boolean")
        {
            get_value_quality_for_test(requested_value, scalar_boolean, attr_quality);
            set_scalar_attribute_value(att, scalar_boolean, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_string")
        {
            get_value_quality_for_test<Tango::DevString>(requested_value, scalar_string, attr_quality);
            set_scalar_attribute_value(att, scalar_string, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_encoded" || my_name == "scalar_encoded_rds")
        {
            get_value_quality_for_test(requested_value, scalar_encoded, attr_quality);
            set_scalar_attribute_value(att, scalar_encoded, attr_quality, release_flag);
            return;
        }
        else if(my_name == "scalar_enum")
        {
            get_value_quality_for_test(requested_value, scalar_enum, attr_quality);
            set_scalar_attribute_value(att, scalar_enum, attr_quality, release_flag);
            return;
        }

        // Spectrum

        if(my_name == "spectrum_short" || my_name == "spectrum_short_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_short, attr_quality);
            set_spectrum_attribute_value(att, spectrum_short, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_long" || my_name == "spectrum_long_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_long, attr_quality);
            set_spectrum_attribute_value(att, spectrum_long, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_float" || my_name == "spectrum_float_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_float, attr_quality);
            set_spectrum_attribute_value(att, spectrum_float, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_double" || my_name == "spectrum_double_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_double, attr_quality);
            set_spectrum_attribute_value(att, spectrum_double, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_ushort" || my_name == "spectrum_ushort_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_ushort, attr_quality);
            set_spectrum_attribute_value(att, spectrum_ushort, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_ulong" || my_name == "spectrum_ulong_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_ulong, attr_quality);
            set_spectrum_attribute_value(att, spectrum_ulong, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_uchar" || my_name == "spectrum_uchar_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_uchar, attr_quality);
            set_spectrum_attribute_value(att, spectrum_uchar, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_long64" || my_name == "spectrum_long64_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_long64, attr_quality);
            set_spectrum_attribute_value(att, spectrum_long64, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_ulong64" || my_name == "spectrum_ulong64_rds")
        {
            get_value_quality_for_test(requested_value, spectrum_ulong64, attr_quality);
            set_spectrum_attribute_value(att, spectrum_ulong64, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_state")
        {
            get_value_quality_for_test(requested_value, spectrum_state, attr_quality);
            set_spectrum_attribute_value(att, spectrum_state, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_boolean")
        {
            get_value_quality_for_test(requested_value, spectrum_boolean, attr_quality);
            set_spectrum_attribute_value(att, spectrum_boolean, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum_string")
        {
            get_value_quality_for_test(requested_value, spectrum_string, attr_quality);
            set_spectrum_attribute_value(att, spectrum_string, attr_quality, release_flag);
            return;
        }
        else if(my_name == "spectrum__enum")
        {
            get_value_quality_for_test(requested_value, spectrum_enum, attr_quality);
            set_spectrum_attribute_value(att, spectrum_enum, attr_quality, release_flag);
            return;
        }

        throw std::runtime_error("Unknown attribute");
    }

    void write_attribute(Tango::WAttribute & /*attr*/) { }

    template <typename T, bool is_spectrum, bool set_rds>
    static void attributes_with_limits_factory(std::vector<Tango::Attr *> &attrs, const std::string &attr_name)
    {
        const auto &attr_type = Tango::tango_type_traits<T>::type_value();
        Tango::UserDefaultAttrProp props;
        if constexpr(!set_rds)
        {
            if constexpr(std::is_same_v<T, Tango::DevEncoded>)
            {
                props.set_min_warning(std::to_string(static_cast<unsigned char>(NUMERIC_MIN_WARNING_LEVEL)).c_str());
                props.set_max_warning(std::to_string(static_cast<unsigned char>(NUMERIC_MAX_WARNING_LEVEL)).c_str());
                props.set_min_alarm(std::to_string(static_cast<unsigned char>(NUMERIC_MIN_ALARM_LEVEL)).c_str());
                props.set_max_alarm(std::to_string(static_cast<unsigned char>(NUMERIC_MAX_ALARM_LEVEL)).c_str());
            }
            else
            {
                props.set_min_warning(std::to_string(static_cast<T>(NUMERIC_MIN_WARNING_LEVEL)).c_str());
                props.set_max_warning(std::to_string(static_cast<T>(NUMERIC_MAX_WARNING_LEVEL)).c_str());
                props.set_min_alarm(std::to_string(static_cast<T>(NUMERIC_MIN_ALARM_LEVEL)).c_str());
                props.set_max_alarm(std::to_string(static_cast<T>(NUMERIC_MAX_ALARM_LEVEL)).c_str());
            }
        }
        else
        {
            if constexpr(std::is_same_v<T, Tango::DevEncoded>)
            {
                props.set_delta_val(std::to_string(static_cast<unsigned char>(NUMERIC_RDS_DELTA)).c_str());
            }
            else
            {
                props.set_delta_val(std::to_string(static_cast<T>(NUMERIC_RDS_DELTA)).c_str());
            }
            props.set_delta_t(std::to_string(RDS_DELTA_T).c_str());
        }

        if constexpr(is_spectrum)
        {
            auto attr = new TangoTest::AutoSpectrumAttr<&DtypeDev::read_attribute, &DtypeDev::write_attribute>(
                attr_name.c_str(), attr_type, N_ELEMENTS_IN_SPECTRUM_ATTRS);
            attr->set_default_properties(props);
            attrs.push_back(attr);
        }
        else
        {
            auto attr = new TangoTest::AutoAttr<&DtypeDev::read_attribute, &DtypeDev::write_attribute>(
                attr_name.c_str(), attr_type);
            attr->set_default_properties(props);
            attrs.push_back(attr);
        }
    }

    static void attributes_no_limits_factory(std::vector<Tango::Attr *> &attrs,
                                             bool is_spectrum,
                                             const std::string &attr_name,
                                             const Tango::CmdArgType &attr_type)
    {
        if(is_spectrum)
        {
            auto attr = new TangoTest::AutoSpectrumAttr<&DtypeDev::read_attribute, &DtypeDev::write_attribute>(
                attr_name.c_str(), attr_type, N_ELEMENTS_IN_SPECTRUM_ATTRS);
            attrs.push_back(attr);
        }
        else
        {
            auto attr = new TangoTest::AutoAttr<&DtypeDev::read_attribute, &DtypeDev::write_attribute>(
                attr_name.c_str(), attr_type);
            attrs.push_back(attr);
        }
    }

    static void
        attributes_enum_factory(std::vector<Tango::Attr *> &attrs, bool is_spectrum, const std::string &attr_name)
    {
        Tango::UserDefaultAttrProp props;
        std::vector<std::string> labels;
        labels.emplace_back("ONE");
        labels.emplace_back("TWO");
        labels.emplace_back("THREE");
        labels.emplace_back("FOUR");
        labels.emplace_back("FIVE");
        labels.emplace_back("SIX");
        labels.emplace_back("SEVEN");
        props.set_enum_labels(labels);
        // to check that subsequent labels sets overwrite previous ones
        props.set_enum_labels(labels);

        if(is_spectrum)
        {
            auto attr =
                new TangoTest::AutoEnumSpectrumAttr<TestEnum, &DtypeDev::read_attribute, &DtypeDev::write_attribute>(
                    attr_name.c_str(), N_ELEMENTS_IN_SPECTRUM_ATTRS);
            attr->set_default_properties(props);
            attrs.push_back(attr);
        }
        else
        {
            auto attr = new TangoTest::AutoEnumAttr<TestEnum, &DtypeDev::read_attribute, &DtypeDev::write_attribute>(
                attr_name.c_str());
            attr->set_default_properties(props);
            attrs.push_back(attr);
        }
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        // Scalar with limits

        attributes_with_limits_factory<Tango::DevShort, false, false>(attrs, "scalar_short");
        attributes_with_limits_factory<Tango::DevLong, false, false>(attrs, "scalar_long");
        attributes_with_limits_factory<Tango::DevFloat, false, false>(attrs, "scalar_float");
        attributes_with_limits_factory<Tango::DevDouble, false, false>(attrs, "scalar_double");
        attributes_with_limits_factory<Tango::DevUShort, false, false>(attrs, "scalar_ushort");
        attributes_with_limits_factory<Tango::DevULong, false, false>(attrs, "scalar_ulong");
        attributes_with_limits_factory<Tango::DevUChar, false, false>(attrs, "scalar_uchar");
        attributes_with_limits_factory<Tango::DevLong64, false, false>(attrs, "scalar_long64");
        attributes_with_limits_factory<Tango::DevULong64, false, false>(attrs, "scalar_ulong64");

        // Spectrum with limits

        attributes_with_limits_factory<Tango::DevShort, true, false>(attrs, "spectrum_short");
        attributes_with_limits_factory<Tango::DevLong, true, false>(attrs, "spectrum_long");
        attributes_with_limits_factory<Tango::DevFloat, true, false>(attrs, "spectrum_float");
        attributes_with_limits_factory<Tango::DevDouble, true, false>(attrs, "spectrum_double");
        attributes_with_limits_factory<Tango::DevUShort, true, false>(attrs, "spectrum_ushort");
        attributes_with_limits_factory<Tango::DevULong, true, false>(attrs, "spectrum_ulong");
        attributes_with_limits_factory<Tango::DevUChar, true, false>(attrs, "spectrum_uchar");
        attributes_with_limits_factory<Tango::DevLong64, true, false>(attrs, "spectrum_long64");
        attributes_with_limits_factory<Tango::DevULong64, true, false>(attrs, "spectrum_ulong64");

        // Scalar no limits

        attributes_no_limits_factory(attrs, false, "scalar_boolean", Tango::DEV_BOOLEAN);
        attributes_no_limits_factory(attrs, false, "scalar_state", Tango::DEV_STATE);
        attributes_no_limits_factory(attrs, false, "scalar_string", Tango::DEV_STRING);

        // Spectrum no limits

        attributes_no_limits_factory(attrs, true, "spectrum_boolean", Tango::DEV_BOOLEAN);
        attributes_no_limits_factory(attrs, true, "spectrum_state", Tango::DEV_STATE);
        attributes_no_limits_factory(attrs, true, "spectrum_string", Tango::DEV_STRING);

        // DevEncoded

        attributes_with_limits_factory<Tango::DevEncoded, false, false>(attrs, "scalar_encoded");

        // DevEnum scalar and spectrum

        attributes_enum_factory(attrs, false, "scalar_enum");
        attributes_enum_factory(attrs, true, "spectrum_enum");

        // attributes with rds limits
        attributes_with_limits_factory<Tango::DevShort, false, true>(attrs, "scalar_short_rds");
        attributes_with_limits_factory<Tango::DevLong, false, true>(attrs, "scalar_long_rds");
        attributes_with_limits_factory<Tango::DevFloat, false, true>(attrs, "scalar_float_rds");
        attributes_with_limits_factory<Tango::DevDouble, false, true>(attrs, "scalar_double_rds");
        attributes_with_limits_factory<Tango::DevUShort, false, true>(attrs, "scalar_ushort_rds");
        attributes_with_limits_factory<Tango::DevULong, false, true>(attrs, "scalar_ulong_rds");
        attributes_with_limits_factory<Tango::DevUChar, false, true>(attrs, "scalar_uchar_rds");
        attributes_with_limits_factory<Tango::DevLong64, false, true>(attrs, "scalar_long64_rds");
        attributes_with_limits_factory<Tango::DevULong64, false, true>(attrs, "scalar_ulong64_rds");

        attributes_with_limits_factory<Tango::DevShort, true, true>(attrs, "spectrum_short_rds");
        attributes_with_limits_factory<Tango::DevLong, true, true>(attrs, "spectrum_long_rds");
        attributes_with_limits_factory<Tango::DevFloat, true, true>(attrs, "spectrum_float_rds");
        attributes_with_limits_factory<Tango::DevDouble, true, true>(attrs, "spectrum_double_rds");
        attributes_with_limits_factory<Tango::DevUShort, true, true>(attrs, "spectrum_ushort_rds");
        attributes_with_limits_factory<Tango::DevULong, true, true>(attrs, "spectrum_ulong_rds");
        attributes_with_limits_factory<Tango::DevUChar, true, true>(attrs, "spectrum_uchar_rds");
        attributes_with_limits_factory<Tango::DevLong64, true, true>(attrs, "spectrum_long64_rds");
        attributes_with_limits_factory<Tango::DevULong64, true, true>(attrs, "spectrum_ulong64_rds");

        attributes_with_limits_factory<Tango::DevEncoded, false, true>(attrs, "scalar_encoded_rds");
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&DtypeDev::set_release_flag>("set_release_flag"));
        cmds.push_back(new TangoTest::AutoCommand<&DtypeDev::set_tested_value>("set_tested_value"));
        cmds.push_back(new TangoTest::AutoCommand<&DtypeDev::set_tested_attribute>("set_tested_attribute"));
    }

  private:
    bool release_flag = false;
    Tango::AttrQuality attr_quality = Tango::ATTR_VALID;
    ValueToTest tested_value = NORMAL;
    std::string tested_attribute;

    Tango::DevShort scalar_short;
    Tango::DevLong scalar_long;
    Tango::DevFloat scalar_float;
    Tango::DevDouble scalar_double;
    Tango::DevUShort scalar_ushort;
    Tango::DevULong scalar_ulong;
    Tango::DevUChar scalar_uchar;
    Tango::DevLong64 scalar_long64;
    Tango::DevULong64 scalar_ulong64;
    Tango::DevEncoded scalar_encoded;

    Tango::DevBoolean scalar_boolean;
    Tango::DevState scalar_state;
    Tango::DevString scalar_string;
    TestEnum scalar_enum;

    Tango::DevShort spectrum_short[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevLong spectrum_long[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevFloat spectrum_float[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevDouble spectrum_double[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevUShort spectrum_ushort[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevULong spectrum_ulong[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevUChar spectrum_uchar[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevLong64 spectrum_long64[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevULong64 spectrum_ulong64[N_ELEMENTS_IN_SPECTRUM_ATTRS];

    Tango::DevBoolean spectrum_boolean[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevState spectrum_state[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    Tango::DevString spectrum_string[N_ELEMENTS_IN_SPECTRUM_ATTRS];
    TestEnum spectrum_enum[N_ELEMENTS_IN_SPECTRUM_ATTRS];
};

/*

 Client section

 */

void set_release_flag(std::unique_ptr<Tango::DeviceProxy> &device, const bool &release_flag)
{
    Tango::DeviceData dd;
    dd << release_flag;
    REQUIRE_NOTHROW(device->command_inout("set_release_flag", dd));
}

void set_tested_value(std::unique_ptr<Tango::DeviceProxy> &device, const ValueToTest &value_to_test)
{
    Tango::DeviceData dd;
    dd << static_cast<short int>(value_to_test);
    REQUIRE_NOTHROW(device->command_inout("set_tested_value", dd));
}

// compare read value with expected

template <
    typename T,
    typename std::enable_if_t<(std::is_arithmetic_v<T> || tango_traits::is_scalar_state_boolean_string_enum<T>::value),
                              T> * = nullptr>
void compare_attribute_value(const T &got_val, const T &expected_val, bool)
{
    REQUIRE(got_val == expected_val);
}

template <typename T,
          typename std::enable_if_t<((!std::is_arithmetic_v<T> &&
                                      !tango_traits::is_scalar_state_boolean_string_enum<T>::value)),
                                    T> * = nullptr>
void compare_attribute_value(const T &got_val, const T &expected_val, bool)
{
    CHECK(got_val.size() >= expected_val.size());

    for(size_t i = 0; i < expected_val.size(); ++i)
    {
        CHECK(got_val[i] == expected_val[i]);
    }
}

template <>
void compare_attribute_value<Tango::DevEncoded>(const Tango::DevEncoded &got_val,
                                                const Tango::DevEncoded &expected_val,
                                                bool check_format)
{
    if(check_format)
    {
        CHECK(::strcmp(got_val.encoded_format, expected_val.encoded_format) == 0);
    }

    for(unsigned int i = 0; i < expected_val.encoded_data.length(); ++i)
    {
        CHECK(got_val.encoded_data[i] == expected_val.encoded_data[i]);
    }
}

template <typename T>
void read_and_compare_attribute_value(std::unique_ptr<Tango::DeviceProxy> &device,
                                      const std::string &attr_name,
                                      const T &expected_val,
                                      Tango::AttrQuality &expected_quality,
                                      const Tango::CmdArgType &expected_type,
                                      const Tango::AttrDataFormat &expected_format,
                                      bool check_format = true)
{
    Tango::DeviceAttribute da;
    REQUIRE(da.get_data_format() == Tango::FMT_UNKNOWN);

    T read_val;

    int read_type;

    Tango::AttrQuality read_quality;

    WHEN("Reading attribute " + attr_name)
    {
        REQUIRE_NOTHROW(da = device->read_attribute(attr_name));

        THEN("Comparing value, data type and format")
        {
            Tango::AttrDataFormat read_format = da.get_data_format();
            CHECK(read_format == expected_format);

            read_quality = da.get_quality();
            CHECK(read_quality == expected_quality);

            if(expected_quality != Tango::ATTR_INVALID)
            {
                if constexpr(!std::is_same_v<T, Tango::DevEnum>)
                {
                    read_type = da.get_type();
                    CHECK(read_type == (int) expected_type);
                }

                bool ret = (da >> read_val);
                CHECK(ret == true);

                compare_attribute_value(read_val, expected_val, check_format);
            }
        }
    }
}

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(DtypeDev, 4)

/*
 *  Set of test reading attributes with release flag true or false
 */

template <typename T>
void test_release_flag(const std::string &attr_name,
                       const Tango::CmdArgType &expected_type,
                       const Tango::AttrDataFormat &expected_format)
{
    T expected_value;
    Tango::AttrQuality expected_quality;

    get_value_quality_for_test(ValueToTest::NORMAL, expected_value, expected_quality);

    int idlver = GENERATE(TangoTest::idlversion(4));

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"dtype_tests", "DtypeDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        // GENERATE is the correct approach, but slower....
        bool release_flags[] = {true, false};
        for(bool release_flag : release_flags)
        //        bool release_flag = GENERATE(true, false);
        {
            AND_GIVEN("Testing " + attr_name + " with release=" << std::boolalpha << release_flag)
            {
                set_release_flag(device, release_flag);
                read_and_compare_attribute_value(
                    device, attr_name, expected_value, expected_quality, expected_type, expected_format);
            }
        }
    }
}

// Scalars

SCENARIO("Testing release parameter for scalar Tango::DevShort")
{
    test_release_flag<Tango::DevShort>("scalar_short", Tango::DEV_SHORT, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevLong")
{
    test_release_flag<Tango::DevLong>("scalar_long", Tango::DEV_LONG, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevFloat")
{
    test_release_flag<Tango::DevFloat>("scalar_float", Tango::DEV_FLOAT, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevDouble")
{
    test_release_flag<Tango::DevDouble>("scalar_double", Tango::DEV_DOUBLE, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevUShort")
{
    test_release_flag<Tango::DevUShort>("scalar_ushort", Tango::DEV_USHORT, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevULong")
{
    test_release_flag<Tango::DevULong>("scalar_ulong", Tango::DEV_ULONG, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevUChar")
{
    test_release_flag<Tango::DevUChar>("scalar_uchar", Tango::DEV_UCHAR, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevLong64")
{
    test_release_flag<Tango::DevLong64>("scalar_long64", Tango::DEV_LONG64, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevULong64")
{
    test_release_flag<Tango::DevULong64>("scalar_ulong64", Tango::DEV_ULONG64, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevBoolean")
{
    test_release_flag<Tango::DevBoolean>("scalar_boolean", Tango::DEV_BOOLEAN, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevState")
{
    test_release_flag<Tango::DevState>("scalar_state", Tango::DEV_STATE, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevString")
{
    test_release_flag<std::string>("scalar_string", Tango::DEV_STRING, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevEnum")
{
    test_release_flag<Tango::DevEnum>("scalar_enum", Tango::DEV_ENUM, Tango::SCALAR);
}

SCENARIO("Testing release parameter for scalar Tango::DevEncoded")
{
    test_release_flag<Tango::DevEncoded>("scalar_encoded", Tango::DEV_ENCODED, Tango::SCALAR);
}

// Spectrum (aka cpp <vector>)

SCENARIO("Testing release parameter for spectrum Tango::DevShort")
{
    test_release_flag<std::vector<Tango::DevShort>>("spectrum_short", Tango::DEV_SHORT, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevLong")
{
    test_release_flag<std::vector<Tango::DevLong>>("spectrum_long", Tango::DEV_LONG, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevFloat")
{
    test_release_flag<std::vector<Tango::DevFloat>>("spectrum_float", Tango::DEV_FLOAT, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevDouble")
{
    test_release_flag<std::vector<Tango::DevDouble>>("spectrum_double", Tango::DEV_DOUBLE, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevUShort")
{
    test_release_flag<std::vector<Tango::DevUShort>>("spectrum_ushort", Tango::DEV_USHORT, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevULong")
{
    test_release_flag<std::vector<Tango::DevULong>>("spectrum_ulong", Tango::DEV_ULONG, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevUChar")
{
    test_release_flag<std::vector<Tango::DevUChar>>("spectrum_uchar", Tango::DEV_UCHAR, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevLong64")
{
    test_release_flag<std::vector<Tango::DevLong64>>("spectrum_long64", Tango::DEV_LONG64, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevULong64")
{
    test_release_flag<std::vector<Tango::DevULong64>>("spectrum_ulong64", Tango::DEV_ULONG64, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevBoolean")
{
    test_release_flag<std::vector<Tango::DevBoolean>>("spectrum_boolean", Tango::DEV_BOOLEAN, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevState")
{
    test_release_flag<std::vector<Tango::DevState>>("spectrum_state", Tango::DEV_STATE, Tango::SPECTRUM);
}

SCENARIO("Testing release parameter for spectrum Tango::DevString")
{
    test_release_flag<std::vector<std::string>>("spectrum_string", Tango::DEV_STRING, Tango::SPECTRUM);
}

// SCENARIO("Testing release parameter for spectrum Tango::DevEnum")
//{
//     test_release_flag<std::vector<Tango::DevEnum>>("spectrum_enum", Tango::DEV_ENUM, Tango::SPECTRUM);
// }

/*
 *  Set of test that alarm and warning levels set when reading attributes
 */

void set_tested_attribute(std::unique_ptr<Tango::DeviceProxy> &device, const std::string &attribute_to_test)
{
    Tango::DeviceData dd;
    dd << attribute_to_test;
    REQUIRE_NOTHROW(device->command_inout("set_tested_attribute", dd));
}

template <typename T>
void test_alarm_warning_when_read_value(const std::string &attr_name,
                                        const Tango::CmdArgType &expected_type,
                                        const Tango::AttrDataFormat &expected_format)
{
    T expected_value;
    Tango::AttrQuality expected_quality;

    int idlver = GENERATE(TangoTest::idlversion(4));

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"dtype_tests", "DtypeDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        set_tested_attribute(device, attr_name);

        // GENERATE is the correct approach, but slower....
        for(int enum_val = ValueToTest::NORMAL; enum_val < ValueToTest::RDS_WRITE; enum_val++)
        //        int enum_val = GENERATE(range(0, static_cast<int>(ValueToTest::RDS_WRITE)));
        {
            ValueToTest value_to_test = static_cast<ValueToTest>(enum_val);
            get_value_quality_for_test(value_to_test, expected_value, expected_quality);

            AND_GIVEN("Testing " + valueToTestToString(value_to_test) + " value for " + attr_name)
            {
                set_tested_value(device, value_to_test);
                read_and_compare_attribute_value(
                    device, attr_name, expected_value, expected_quality, expected_type, expected_format);
            }
        }
    }
}

// Scalars

SCENARIO("Testing alarm warning when read for scalar Tango::DevShort")
{
    test_alarm_warning_when_read_value<Tango::DevShort>("scalar_short", Tango::DEV_SHORT, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevLong")
{
    test_alarm_warning_when_read_value<Tango::DevLong>("scalar_long", Tango::DEV_LONG, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevFloat")
{
    test_alarm_warning_when_read_value<Tango::DevFloat>("scalar_float", Tango::DEV_FLOAT, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevDouble")
{
    test_alarm_warning_when_read_value<Tango::DevDouble>("scalar_double", Tango::DEV_DOUBLE, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevUShort")
{
    test_alarm_warning_when_read_value<Tango::DevUShort>("scalar_ushort", Tango::DEV_USHORT, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevULong")
{
    test_alarm_warning_when_read_value<Tango::DevULong>("scalar_ulong", Tango::DEV_ULONG, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevUChar")
{
    test_alarm_warning_when_read_value<Tango::DevUChar>("scalar_uchar", Tango::DEV_UCHAR, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevLong64")
{
    test_alarm_warning_when_read_value<Tango::DevLong64>("scalar_long64", Tango::DEV_LONG64, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevULong64")
{
    test_alarm_warning_when_read_value<Tango::DevULong64>("scalar_ulong64", Tango::DEV_ULONG64, Tango::SCALAR);
}

SCENARIO("Testing alarm warning when read for scalar Tango::DevEncoded")
{
    test_alarm_warning_when_read_value<Tango::DevEncoded>("scalar_encoded", Tango::DEV_ENCODED, Tango::SCALAR);
}

// Spectrum (aka cpp <vector>)

SCENARIO("Testing alarm warning when read for spectrum Tango::DevShort")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevShort>>(
        "spectrum_short", Tango::DEV_SHORT, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevLong")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevLong>>("spectrum_long", Tango::DEV_LONG, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevFloat")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevFloat>>(
        "spectrum_float", Tango::DEV_FLOAT, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevDouble")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevDouble>>(
        "spectrum_double", Tango::DEV_DOUBLE, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevUShort")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevUShort>>(
        "spectrum_ushort", Tango::DEV_USHORT, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevULong")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevULong>>(
        "spectrum_ulong", Tango::DEV_ULONG, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevUChar")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevUChar>>(
        "spectrum_uchar", Tango::DEV_UCHAR, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevLong64")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevLong64>>(
        "spectrum_long64", Tango::DEV_LONG64, Tango::SPECTRUM);
}

SCENARIO("Testing alarm warning when read for spectrum Tango::DevULong64")
{
    test_alarm_warning_when_read_value<std::vector<Tango::DevULong64>>(
        "spectrum_ulong64", Tango::DEV_ULONG64, Tango::SPECTRUM);
}

/*
 *  Set of test for read value != written (rds alarm)
 */

template <typename T>
void write_attribute(std::unique_ptr<Tango::DeviceProxy> &device, const std::string &attr_name, const T &value_to_write)
{
    Tango::DeviceAttribute attr;
    attr.set_name(attr_name);
    attr << value_to_write;
    REQUIRE_NOTHROW(device->write_attribute(attr));
}

template <typename T>
void test_rds_alarm(const std::string &attr_name,
                    const Tango::CmdArgType &expected_type,
                    const Tango::AttrDataFormat &expected_format)
{
    T expected_value;
    T write_value;
    Tango::AttrQuality expected_quality;

    get_value_quality_for_test(ValueToTest::NORMAL, expected_value, expected_quality);
    get_value_quality_for_test(ValueToTest::RDS_WRITE, write_value, expected_quality);

    int idlver = GENERATE(TangoTest::idlversion(4));

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"dtype_tests", "DtypeDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("Testing " + attr_name)
        {
            write_attribute(device, attr_name, write_value);
            std::this_thread::sleep_for(std::chrono::milliseconds(10 * RDS_DELTA_T));
            read_and_compare_attribute_value(
                device, attr_name, expected_value, expected_quality, expected_type, expected_format);
        }
        AND_GIVEN("Testing " + attr_name + " rds overflow")
        {
            set_tested_value(device, ValueToTest::RDS_OVERFLOW);
            set_tested_attribute(device, attr_name);
            get_value_quality_for_test(ValueToTest::NUMERIC_LIMIT_MIN, write_value, expected_quality);
            get_value_quality_for_test(ValueToTest::RDS_OVERFLOW, expected_value, expected_quality);
            if constexpr(std::is_signed_v<T> ||
                         (is_container<T>::value && std::is_signed_v<typename is_container<T>::underlying_type>) )
            {
                expected_quality = Tango::ATTR_ALARM;
            }
            else
            {
                expected_quality = Tango::ATTR_VALID;
            }
            write_attribute(device, attr_name, write_value);
            std::this_thread::sleep_for(std::chrono::milliseconds(10 * RDS_DELTA_T));
            read_and_compare_attribute_value(
                device, attr_name, expected_value, expected_quality, expected_type, expected_format);
        }
        if constexpr(std::is_same_v<T, Tango::DevEncoded>)
        {
            write_value = expected_value;
            AND_GIVEN("Testing different format for " + attr_name)
            {
                write_value = expected_value;
                expected_value.encoded_format = Tango::string_dup("fmt");
                write_value.encoded_format = Tango::string_dup("other_fmt");
                write_attribute(device, attr_name, write_value);
                std::this_thread::sleep_for(std::chrono::milliseconds(10 * RDS_DELTA_T));
                read_and_compare_attribute_value(
                    device, attr_name, expected_value, expected_quality, expected_type, expected_format, false);
            }
        }
    }
}

//// Scalars

SCENARIO("Testing rds alarm for scalar Tango::DevShort")
{
    test_rds_alarm<Tango::DevShort>("scalar_short_rds", Tango::DEV_SHORT, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevLong")
{
    test_rds_alarm<Tango::DevLong>("scalar_long_rds", Tango::DEV_LONG, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevFloat")
{
    test_rds_alarm<Tango::DevFloat>("scalar_float_rds", Tango::DEV_FLOAT, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevDouble")
{
    test_rds_alarm<Tango::DevDouble>("scalar_double_rds", Tango::DEV_DOUBLE, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevUShort")
{
    test_rds_alarm<Tango::DevUShort>("scalar_ushort_rds", Tango::DEV_USHORT, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevULong")
{
    test_rds_alarm<Tango::DevULong>("scalar_ulong_rds", Tango::DEV_ULONG, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevUChar")
{
    test_rds_alarm<Tango::DevUChar>("scalar_uchar_rds", Tango::DEV_UCHAR, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevLong64")
{
    test_rds_alarm<Tango::DevLong64>("scalar_long64_rds", Tango::DEV_LONG64, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevULong64")
{
    test_rds_alarm<Tango::DevULong64>("scalar_ulong64_rds", Tango::DEV_ULONG64, Tango::SCALAR);
}

SCENARIO("Testing rds alarm for scalar Tango::DevEncoded")
{
    test_rds_alarm<Tango::DevEncoded>("scalar_encoded_rds", Tango::DEV_ENCODED, Tango::SCALAR);
}

// Spectrum (aka cpp <vector>)

SCENARIO("Testing rds alarm for spectrum Tango::DevShort")
{
    test_rds_alarm<std::vector<Tango::DevShort>>("spectrum_short_rds", Tango::DEV_SHORT, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevLong")
{
    test_rds_alarm<std::vector<Tango::DevLong>>("spectrum_long_rds", Tango::DEV_LONG, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevFloat")
{
    test_rds_alarm<std::vector<Tango::DevFloat>>("spectrum_float_rds", Tango::DEV_FLOAT, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevDouble")
{
    test_rds_alarm<std::vector<Tango::DevDouble>>("spectrum_double_rds", Tango::DEV_DOUBLE, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevUShort")
{
    test_rds_alarm<std::vector<Tango::DevUShort>>("spectrum_ushort_rds", Tango::DEV_USHORT, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevULong")
{
    test_rds_alarm<std::vector<Tango::DevULong>>("spectrum_ulong_rds", Tango::DEV_ULONG, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevUChar")
{
    test_rds_alarm<std::vector<Tango::DevUChar>>("spectrum_uchar_rds", Tango::DEV_UCHAR, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevLong64")
{
    test_rds_alarm<std::vector<Tango::DevLong64>>("spectrum_long64_rds", Tango::DEV_LONG64, Tango::SPECTRUM);
}

SCENARIO("Testing rds alarm for spectrum Tango::DevULong64")
{
    test_rds_alarm<std::vector<Tango::DevULong64>>("spectrum_ulong64_rds", Tango::DEV_ULONG64, Tango::SPECTRUM);
}

/*
 *  Set of test for device state goes to ALARM when one of attribute out of limits
 *  (we also check, that state machine does proper memory management)
 */

void test_auto_alarm_device_state(const std::string &attr_name)
{
    Tango::DeviceData dout;
    Tango::DevState read_state;

    int idlver = GENERATE(TangoTest::idlversion(4));

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"dtype_tests", "DtypeDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        set_tested_value(device, ValueToTest::MAX_ALARM);
        set_tested_attribute(device, attr_name);

        // GENERATE is the correct approach, but slower....
        bool release_flags[] = {true, false};
        for(bool release_flag : release_flags)
        //        bool release_flag = GENERATE(true, false);
        {
            std::string flag_str = release_flag ? "true" : "false";
            AND_GIVEN("Testing " + attr_name + " with release=" + flag_str)
            {
                set_release_flag(device, release_flag);
                REQUIRE_NOTHROW(dout = device->command_inout("State"));
                dout >> read_state;
                CHECK(read_state == Tango::ALARM);
            }
        }
    }
}

// Scalars

SCENARIO("Testing auto alarm device state for scalar Tango::DevShort")
{
    test_auto_alarm_device_state("scalar_short");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevLong")
{
    test_auto_alarm_device_state("scalar_long");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevFloat")
{
    test_auto_alarm_device_state("scalar_float");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevDouble")
{
    test_auto_alarm_device_state("scalar_double");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevUShort")
{
    test_auto_alarm_device_state("scalar_ushort");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevULong")
{
    test_auto_alarm_device_state("scalar_ulong");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevUChar")
{
    test_auto_alarm_device_state("scalar_uchar");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevLong64")
{
    test_auto_alarm_device_state("scalar_long64");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevULong64")
{
    test_auto_alarm_device_state("scalar_ulong64");
}

SCENARIO("Testing auto alarm device state for scalar Tango::DevEncoded")
{
    test_auto_alarm_device_state("scalar_encoded");
}

// Spectrum (aka cpp <vector>)

SCENARIO("Testing auto alarm device state for spectrum Tango::DevShort")
{
    test_auto_alarm_device_state("spectrum_short");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevLong")
{
    test_auto_alarm_device_state("spectrum_long");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevFloat")
{
    test_auto_alarm_device_state("spectrum_float");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevDouble")
{
    test_auto_alarm_device_state("spectrum_double");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevUShort")
{
    test_auto_alarm_device_state("spectrum_ushort");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevULong")
{
    test_auto_alarm_device_state("spectrum_ulong");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevUChar")
{
    test_auto_alarm_device_state("spectrum_uchar");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevLong64")
{
    test_auto_alarm_device_state("spectrum_long64");
}

SCENARIO("Testing auto alarm device state for spectrum Tango::DevULong64")
{
    test_auto_alarm_device_state("spectrum_ulong64");
}
