#include <tango/tango.h>
#include <utils/utils.h>

#include <tango/internal/utils.h>

template <typename T>
struct TestData
{
    const std::vector<T> vec;
    const std::string sep;
    const std::string result;
};

SCENARIO("stringify_vector behaves as designed")
{
    {
        auto data = GENERATE((TestData<std::string>{{"a", "b"}, ", ", "a, b"}),
                             (TestData<std::string>{{}, ", ", ""}),
                             (TestData<std::string>{{"a", "b"}, "", "ab"}),
                             (TestData<std::string>{{"a"}, ", ", "a"}));

        GIVEN("when serializing a vector with separator " << data.sep)
        {
            WHEN("we then expect it to be: " << data.result)
            {
                std::stringstream sstr;
                Tango::detail::stringify_vector(sstr, data.vec, data.sep);
                REQUIRE(sstr.str() == data.result);
            }
        }
    }

    {
        auto data = GENERATE((TestData<int>{{1, 2, 3}, "|", "1|2|3"}));

        std::stringstream sstr;
        Tango::detail::stringify_vector(sstr, data.vec, data.sep);
        REQUIRE(sstr.str() == data.result);
    }
}

SCENARIO("to_lower/to_upper perform")
{
    struct TestData
    {
        const std::string lower;
        const std::string UPPER;
    };

    auto data = GENERATE((TestData{"", ""}), (TestData{"a123.b", "A123.B"}));

    GIVEN("some strings")
    {
        WHEN("a case conversion")
        {
            REQUIRE(Tango::detail::to_lower(data.lower) == data.lower);
            REQUIRE(Tango::detail::to_lower(data.UPPER) == data.lower);

            REQUIRE(Tango::detail::to_upper(data.lower) == data.UPPER);
            REQUIRE(Tango::detail::to_upper(data.UPPER) == data.UPPER);
        }
    }
}
