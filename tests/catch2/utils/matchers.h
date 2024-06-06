#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include <tango/tango.h>

#include "utils/type_traits.h"

namespace TangoTest
{
namespace detail
{

template <typename AnyLike, typename T>
using corba_extraction_result_t = decltype(std::declval<AnyLike>() >>= std::declval<T>());

template <typename AnyLike, typename T>
constexpr bool has_corba_extract_operator_to = is_detected_v<corba_extraction_result_t, AnyLike, T>;

} // namespace detail

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

    // TODO: Improve the output here.
    //
    // In addition to the values we show, ideally, we need to include:
    //  - A name for AnyLike
    //  - A name for T
    //  - A name for the type actually stored in the AnyLike
    //
    //  This will involve writing our own Catch::StringMaker specialisations
    std::string describe() const override
    {
        return "== " + Catch::StringMaker<T>::convert(value);
    }

  private:
    T value;
};

template <typename T>
auto AnyLikeContains(const T &v)
{
    return AnyLikeContainsMatcher{v};
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
        return "description " + m_matcher.describe();
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
class AnyErrorMatchesMatcher : public Catch::Matchers::MatcherBase<Tango::DevFailed>
{
  public:
    AnyErrorMatchesMatcher(ErrorMatcher matcher) :
        m_matcher{CATCH_MOVE(matcher)}
    {
    }

    bool match(const Tango::DevFailed &ex) const override
    {
        for(size_t i = 0; i < ex.errors.length(); ++i)
        {
            if(m_matcher.match(ex.errors[i]))
            {
                return true;
            }
        }

        return false;
    }

    std::string describe() const override
    {
        return "has error matching " + m_matcher.describe();
    }

  private:
    ErrorMatcher m_matcher;
};

template <typename ErrorMatcher>
AnyErrorMatchesMatcher<ErrorMatcher> AnyErrorMatches(ErrorMatcher &&matcher)
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
        return "has a first error matching " + m_matcher.describe();
    }

  private:
    ErrorMatcher m_matcher;
};

template <typename ErrorMatcher>
FirstErrorMatchesMatcher<ErrorMatcher> FirstErrorMatches(ErrorMatcher &&matcher)
{
    return {CATCH_FORWARD(matcher)};
}
} // namespace TangoTest
