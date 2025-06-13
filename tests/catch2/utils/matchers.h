#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "callback_mock_helpers.h"
#include "test_server.h"

#include <tango/tango.h>

#include <tango/common/utils/type_info.h>
#include <tango/internal/stl_corba_helpers.h>
#include <tango/internal/type_traits.h>

#include <type_traits>

namespace TangoTest
{
namespace detail
{

template <typename AnyLike, typename T>
using corba_extraction_result_t =
    decltype(std::declval<const AnyLike>() >>= std::declval<std::add_lvalue_reference_t<T>>());

template <typename AnyLike, typename T>
constexpr bool has_corba_extract_operator_to = Tango::detail::is_detected_v<corba_extraction_result_t, AnyLike, T>;

template <typename>
struct first_argument;

template <typename Matcher, typename Arg>
struct first_argument<bool (Matcher::*)(Arg) const>
{
    using type = std::remove_cv_t<std::remove_reference_t<Arg>>;
};

// The type a Matcher is expecting to be passed.
// Some Catch2 matchers are generic, in which case this will not work.
template <typename Matcher>
using matchee_t = typename first_argument<decltype(&Matcher::match)>::type;

/// @brief Return the name of the type T
template <typename T>
std::string get_ref_type(T val)
{
    Tango::DeviceData dd;

    dd << val;

    return Tango::detail::corba_any_to_type_name(dd.any);
}

} // namespace detail

namespace Matchers
{

/**
 * @brief Matches an "any like" type that contains a specific value
 *
 * An "any like" type is something such as a `CORBA::Any` or `Tango::DeviceData`
 * where we can do one of the following to extract a value:
 *
 *   T value;
 *   bool extraction_succeeded = any_like >>= value;
 *
 * or
 *
 *   T value;
 *   bool extraction_succeeded = any_like >> value;
 *
 * @tparam T type of value to match
 * @param v value to match
 */
template <typename T>
struct AnyLikeContainsMatcher : Catch::Matchers::MatcherGenericBase
{
    AnyLikeContainsMatcher(const T &v) :
        value{v}

    {
    }

    template <typename AnyLike>
    bool match(AnyLike &any) const
    {
        T other;

        if constexpr(detail::has_corba_extract_operator_to<AnyLike, T>)
        {
            if(!(any >>= other))
            {
                return false;
            }
        }
        else
        {
            if(!(any >> other))
            {
                return false;
            }
        }

        return other == value;
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "contains (" << detail::get_ref_type(T{}) << ") " << Catch::StringMaker<T>::convert(value);

        return os.str();
    }

  private:
    T value;
};

template <typename T>
auto AnyLikeContains(const T &v)
{
    return AnyLikeContainsMatcher{v};
}

/**
 * @brief Matches an "any like" type that contains a value to be matched against
 *
 * An "any like" type is something such as a `CORBA::Any` or `Tango::DeviceData`
 * where we can do one of the following to extract a value:
 *
 *   T value;
 *   bool extraction_succeeded = any_like >>= value;
 *
 * or
 *
 *   T value;
 *   bool extraction_succeeded = any_like >> value;
 *
 * @tparam ContentsMatcher type of matcher
 * @tparam T type of value that should be stored in the any like
 * @param matcher to match against
 */
template <typename ContentsMatcher, typename T>
struct AnyLikeMatchesMatcher : Catch::Matchers::MatcherGenericBase
{
    AnyLikeMatchesMatcher(ContentsMatcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    template <typename AnyLike>
    bool match(AnyLike &any) const
    {
        T other;

        if constexpr(detail::has_corba_extract_operator_to<AnyLike, T>)
        {
            if(!(any >>= other))
            {
                return false;
            }
        }
        else
        {
            if(!(any >> other))
            {
                return false;
            }
        }

        return m_matcher.match(other);
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "contains (" << detail::get_ref_type(T{}) << ") that " << m_matcher.describe();
        return os.str();
    }

  private:
    ContentsMatcher m_matcher;
};

// Match the contents of the AnyLike and try to match it to ContentsMatcher.
// Requires that ContentsMatcher does not inherit from GenericMatcherBase.
template <typename ContentsMatcher>
auto AnyLikeMatches(const ContentsMatcher &&matcher)
{
    return AnyLikeMatchesMatcher<ContentsMatcher, detail::matchee_t<ContentsMatcher>>(CATCH_FORWARD(matcher));
}

// The type T here is the type to extract the AnyLike into.
// declval(ContentsMatcher).match(declval(T)) must be a valid expression.
template <typename T, typename ContentsMatcher>
auto AnyLikeMatches(const ContentsMatcher &&matcher)
{
    return AnyLikeMatchesMatcher<ContentsMatcher, T>(CATCH_FORWARD(matcher));
}

class ReasonMatcher : public Catch::Matchers::MatcherBase<Tango::DevError>
{
  public:
    ReasonMatcher(std::string reason) :
        m_reason{CATCH_MOVE(reason)}
    {
    }

    bool match(const Tango::DevError &error) const override
    {
        return strcmp(m_reason.c_str(), error.reason) == 0;
    }

    std::string describe() const override
    {
        return "reason equals \"" + m_reason + "\"";
    }

  private:
    std::string m_reason;
};

inline ReasonMatcher Reason(std::string reason)
{
    return {CATCH_MOVE(reason)};
}

class SeverityMatcher : public Catch::Matchers::MatcherBase<Tango::DevError>
{
  public:
    SeverityMatcher(Tango::ErrSeverity severity) :
        m_severity{CATCH_MOVE(severity)}
    {
    }

    bool match(const Tango::DevError &error) const override
    {
        return m_severity == error.severity;
    }

    std::string describe() const override
    {
        std::stringstream ss;
        ss << "severity equals ";
        ss << m_severity;
        return ss.str();
    }

  private:
    Tango::ErrSeverity m_severity;
};

inline SeverityMatcher Severity(Tango::ErrSeverity severity)
{
    return {CATCH_MOVE(severity)};
}

template <typename StringMatcher>
class DescriptionMatchesMatcher : public Catch::Matchers::MatcherBase<Tango::DevError>
{
  public:
    DescriptionMatchesMatcher(StringMatcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    bool match(const Tango::DevError &error) const override
    {
        return m_matcher.match(error.desc.in());
    }

    std::string describe() const override
    {
        return "contains a description that " + m_matcher.describe();
    }

  private:
    StringMatcher m_matcher;
};

template <typename StringMatcher>
DescriptionMatchesMatcher<StringMatcher> DescriptionMatches(StringMatcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}

template <typename ErrorMatcher>
class FirstErrorMatchesMatcher : public Catch::Matchers::MatcherBase<Tango::DevFailed>
{
  public:
    FirstErrorMatchesMatcher(ErrorMatcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    bool match(const Tango::DevFailed &ex) const override
    {
        return m_matcher.match(ex.errors[0]);
    }

    std::string describe() const override
    {
        return "has a first error that " + m_matcher.describe();
    }

  private:
    ErrorMatcher m_matcher;
};

template <typename ErrorMatcher>
FirstErrorMatchesMatcher<ErrorMatcher> FirstErrorMatches(ErrorMatcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}

class EventTypeMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventTypeMatcher(Tango::EventType event_type) :
        m_event_type{CATCH_MOVE(event_type)}
    {
    }

    template <typename T>
    bool match(const std::optional<T> &event) const
    {
        TANGO_ASSERT(event.has_value());
        return event->event == Tango::EventName[m_event_type];
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "has event type that equals \"" << Tango::EventName[m_event_type] << "\"";
        return os.str();
    }

  private:
    Tango::EventType m_event_type;
};

inline EventTypeMatcher EventType(Tango::EventType event_type)
{
    REQUIRE(event_type >= 0);
    REQUIRE(event_type < Tango::numEventType);

    return {CATCH_FORWARD(event_type)};
}

class EventCounterMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventCounterMatcher(int counter) :
        m_counter{CATCH_MOVE(counter)}
    {
    }

    virtual bool match(const std::optional<Tango::DataReadyEventData> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        return event->ctr == m_counter;
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "has counter that equals \"" << m_counter << "\"";
        return os.str();
    }

  private:
    int m_counter;
};

inline EventCounterMatcher EventCounter(int counter)
{
    return {CATCH_FORWARD(counter)};
}

class EventAttrTypeMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventAttrTypeMatcher(int attr_type) :
        m_attr_type{CATCH_MOVE(attr_type)}
    {
    }

    virtual bool match(const std::optional<Tango::DataReadyEventData> &event) const
    {
        TANGO_ASSERT(event.has_value());
        return event->attr_data_type == m_attr_type;
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "has attribute data type that equals \"" << Tango::data_type_to_string(m_attr_type) << "\"";
        return os.str();
    }

  private:
    int m_attr_type;
};

inline EventAttrTypeMatcher EventAttrType(int attr_type)
{
    return {CATCH_FORWARD(attr_type)};
}

class AttrQualityMatcher : public Catch::Matchers::MatcherBase<Tango::DeviceAttribute>
{
  public:
    AttrQualityMatcher(Tango::AttrQuality attr_quality) :
        m_attr_quality{CATCH_MOVE(attr_quality)}
    {
    }

    bool match(const Tango::DeviceAttribute &attr) const override
    {
        return const_cast<Tango::DeviceAttribute &>(attr).get_quality() == m_attr_quality;
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "has attribute quality that equals \"" << m_attr_quality << "\"";
        return os.str();
    }

  private:
    Tango::AttrQuality m_attr_quality;
};

inline AttrQualityMatcher AttrQuality(Tango::AttrQuality attr_quality)
{
    REQUIRE(attr_quality >= 0);
    // hardcoded limit for IDL 6.0.2
    REQUIRE(attr_quality < 5);

    return {CATCH_FORWARD(attr_quality)};
}

class EventDeviceStartedMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventDeviceStartedMatcher(bool device_started) :
        m_device_started{CATCH_MOVE(device_started)}
    {
    }

    virtual bool match(const std::optional<Tango::DevIntrChangeEventData> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        return event->dev_started == m_device_started;
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "has dev_started equal to \"" << std::boolalpha << m_device_started << "\"";
        return os.str();
    }

  private:
    bool m_device_started;
};

inline EventDeviceStartedMatcher EventDeviceStarted(bool device_started)
{
    return {CATCH_FORWARD(device_started)};
}

template <typename Matcher>
class EventValueMatchesMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventValueMatchesMatcher(Matcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    bool match(const std::optional<Tango::EventData> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        return event->attr_value != nullptr && m_matcher.match(*event->attr_value);
    }

    bool match(std::optional<TangoTest::AttrReadEventCopyable> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        return m_matcher.match(event->argout);
    }

    bool match(std::optional<TangoTest::AttrWrittenEventCopyable> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        return true;
    }

    bool match(std::optional<TangoTest::CmdDoneEventCopyable> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        return m_matcher.match(event->argout);
    }

    std::string describe() const override
    {
        return "has attr_value that " + m_matcher.describe();
    }

  private:
    Matcher m_matcher;
};

template <typename Matcher>
EventValueMatchesMatcher<Matcher> EventValueMatches(Matcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}

template <typename Matcher>
class EventErrorMatchesMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventErrorMatchesMatcher(Matcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    template <typename T>
    bool match(const std::optional<T> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(!event->err)
        {
            return false;
        }

        return m_matcher.match(event->errors);
    }

    std::string describe() const override
    {
        return "contains errors that " + m_matcher.describe();
    }

  private:
    Matcher m_matcher;
};

template <typename Matcher>
EventErrorMatchesMatcher<Matcher> EventErrorMatches(Matcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}

template <typename Matcher>
class ErrorListMatchesMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    ErrorListMatchesMatcher(Matcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    bool match(const Tango::DevFailed &e) const
    {
        return m_matcher.match(e.errors);
    }

    bool match(Tango::DeviceAttribute &e) const
    {
        return m_matcher.match(e.get_error_list());
    }

    std::string describe() const override
    {
        return "contains errors that " + m_matcher.describe();
    }

  private:
    Matcher m_matcher;
};

template <typename Matcher>
ErrorListMatchesMatcher<Matcher> ErrorListMatches(Matcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}

class IsSuccessMatcher : public Catch::Matchers::MatcherBase<TangoTest::ExitStatus>
{
  public:
    IsSuccessMatcher() = default;

    bool match(const TangoTest::ExitStatus &status) const override
    {
        return status.is_success();
    }

    std::string describe() const override
    {
        std::ostringstream os;
        os << "is successful";
        return os.str();
    }
};

inline IsSuccessMatcher IsSuccess()
{
    return IsSuccessMatcher();
}

template <typename Rep, typename Period>
class WithinTimeAbsMatcher : public Catch::Matchers::MatcherBase<Tango::TimeVal>
{
  public:
    WithinTimeAbsMatcher(const Tango::TimeVal &ref, std::chrono::duration<Rep, Period> margin) :
        m_margin{CATCH_MOVE(margin)},
        m_ref{CATCH_MOVE(ref)}
    {
    }

    bool match(const Tango::TimeVal &val) const override
    {
        auto ref_chrono_tp = Tango::make_system_time(m_ref);
        auto val_chrono_tp = Tango::make_system_time(val);

        return std::chrono::duration_cast<std::chrono::nanoseconds>(ref_chrono_tp - val_chrono_tp) <= m_margin;
    }

    std::string describe() const override
    {
        const auto margin_ns = std::chrono::nanoseconds(m_margin).count();

        std::ostringstream os;
        os << "has TimeVal which is within \"" << margin_ns << " [ns] \" of the reference TimeVal \""
           << Catch::StringMaker<Tango::TimeVal>::convert(m_ref) << "\"";
        return os.str();
    }

  private:
    std::chrono::duration<Rep, Period> m_margin;
    Tango::TimeVal m_ref;
};

template <typename Rep, typename Period>
inline WithinTimeAbsMatcher<Rep, Period> WithinTimeAbs(Tango::TimeVal ref, std::chrono::duration<Rep, Period> margin)
{
    return {CATCH_FORWARD(ref, margin)};
}

template <typename Matcher>
class EventCommandNamesMatchesMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventCommandNamesMatchesMatcher(Matcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    bool match(const std::optional<Tango::DevIntrChangeEventData> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        std::vector<std::string> cmds;
        std::for_each(begin(event->cmd_list),
                      end(event->cmd_list),
                      [&cmds](const Tango::CommandInfo &ci) { cmds.emplace_back(ci.cmd_name); });

        return m_matcher.match(cmds);
    }

    std::string describe() const override
    {
        return "contains command names that " + m_matcher.describe();
    }

  private:
    Matcher m_matcher;
};

template <typename Matcher>
EventCommandNamesMatchesMatcher<Matcher> EventCommandNamesMatches(Matcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}

template <typename Matcher>
class EventAttributeNamesMatchesMatcher : public Catch::Matchers::MatcherGenericBase
{
  public:
    EventAttributeNamesMatchesMatcher(Matcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    bool match(const std::optional<Tango::DevIntrChangeEventData> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(event->err)
        {
            return false;
        }

        std::vector<std::string> attrs;
        std::for_each(begin(event->att_list),
                      end(event->att_list),
                      [&attrs](const Tango::AttributeInfoEx &ci) { attrs.emplace_back(ci.name); });

        return m_matcher.match(attrs);
    }

    std::string describe() const override
    {
        return "contains attribute names that " + m_matcher.describe();
    }

  private:
    Matcher m_matcher;
};

template <typename Matcher>
EventAttributeNamesMatchesMatcher<Matcher> EventAttributeNamesMatches(Matcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}

} // namespace Matchers

} // namespace TangoTest
