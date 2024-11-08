#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "callback_mock_helpers.h"

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

class EventTypeMatcher : public Catch::Matchers::MatcherBase<std::optional<Tango::EventData>>
{
  public:
    EventTypeMatcher(Tango::EventType event_type) :
        m_event_type{CATCH_MOVE(event_type)}
    {
    }

    bool match(const std::optional<Tango::EventData> &event) const override
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

    bool match(const std::optional<Tango::EventData> &event) const
    {
        TANGO_ASSERT(event.has_value());

        if(!event->err)
        {
            return false;
        }

        return m_matcher.match(event->errors);
    }

    bool match(const std::optional<TangoTest::AttrReadEventCopyable> &event) const
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

} // namespace Matchers

} // namespace TangoTest
