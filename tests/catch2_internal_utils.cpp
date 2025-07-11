#include "catch2_common.h"

#include <tango/internal/utils.h>
#include <tango/internal/base_classes.h>

#include <type_traits>

namespace
{
class Derived : public Tango::detail::NonCopyable
{
};

static_assert(std::is_default_constructible_v<Derived>);
static_assert(!std::is_copy_constructible_v<Derived>);
static_assert(!std::is_copy_assignable_v<Derived>);
static_assert(!std::is_move_constructible_v<Derived>);
static_assert(!std::is_move_assignable_v<Derived>);

} // namespace

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

SCENARIO("to_boolean")
{
    GIVEN("some strings")
    {
        WHEN("parses them as std::optional<boolean>")
        {
            REQUIRE(!Tango::detail::to_boolean("").has_value());
            REQUIRE(!Tango::detail::to_boolean("bs").has_value());
            // case matters
            REQUIRE(!Tango::detail::to_boolean("FALSE").has_value());

            REQUIRE(Tango::detail::to_boolean("0") == false);
            REQUIRE(Tango::detail::to_boolean("off") == false);
            REQUIRE(Tango::detail::to_boolean("false") == false);

            REQUIRE(Tango::detail::to_boolean("1") == true);
            REQUIRE(Tango::detail::to_boolean("on") == true);
            REQUIRE(Tango::detail::to_boolean("true") == true);
        }
    }
}

SCENARIO("get_boolean_env_var")
{
    GIVEN("a non existing env var")
    {
        const char *name = "I_DONT_EXIST";

        WHEN("returns the default value")
        {
            REQUIRE(unset_env(name) == 0);
            REQUIRE(Tango::detail::get_boolean_env_var(name, false) == false);
            REQUIRE(Tango::detail::get_boolean_env_var(name, true) == true);
        }
    }
    GIVEN("a non-boolean entry")
    {
        const char *name = "testvar";
        WHEN("throws")
        {
            using namespace TangoTest::Matchers;

            REQUIRE(set_env(name, "abcd", true) == 0);

            REQUIRE_THROWS_MATCHES(Tango::detail::get_boolean_env_var(name, true),
                                   Tango::DevFailed,
                                   FirstErrorMatches(Reason(Tango::API_InvalidArgs)));
            REQUIRE(unset_env(name) == 0);
        }
    }

    GIVEN("something which to_boolean groks")
    {
        const char *name = "testvar";
        WHEN("works")
        {
            REQUIRE(set_env(name, "1", true) == 0);
            REQUIRE(Tango::detail::get_boolean_env_var(name, false) == true);

            REQUIRE(set_env(name, "off", true) == 0);
            REQUIRE(Tango::detail::get_boolean_env_var(name, true) == false);

            REQUIRE(unset_env(name) == 0);
        }
    }
}

SCENARIO("stringify_any")
{
    GIVEN("an empty any")
    {
        CORBA::Any_var any = new CORBA::Any;

        WHEN("works")
        {
            std::ostringstream os;
            REQUIRE_NOTHROW(Tango::detail::stringify_any(os, any));
            REQUIRE(os.str() == "empty");
        }
    }

    GIVEN("a filled any")
    {
        CORBA::Any_var any = new CORBA::Any;

        any <<= 123.4;

        WHEN("works")
        {
            std::ostringstream os;
            REQUIRE_NOTHROW(Tango::detail::stringify_any(os, any));
            REQUIRE(os.str() == "123.4");
        }
    }
    GIVEN("a filled Any with an incompatible type")
    {
        CORBA::Any_var any = new CORBA::Any;
        CORBA::Any_var in = new CORBA::Any;
        any <<= in;

        WHEN("we throw")
        {
            using namespace Catch::Matchers;
            using namespace TangoTest::Matchers;

            std::ostringstream os;
            REQUIRE_THROWS_MATCHES(Tango::detail::stringify_any(os, any),
                                   Tango::DevFailed,
                                   FirstErrorMatches(Reason(Tango::API_InvalidCorbaAny)));
        }
    }
}

SCENARIO("Event name functions")
{
    GIVEN("A qualified and unqualified event name")
    {
        const std::string qual_event_name_intr = "tango://127.0.0.1:10363/testserver/tests/1#dbase=no.intr_change";
        const std::string unqual_event_name_intr = "intr_change";
        const std::string qual_event_name =
            "tango://127.0.0.1:11570/testserver/tests/1/short_attr#dbase=no.idl5_change";
        const std::string unqual_event_name = "idl5_change";

        WHEN("we can remove the idl prefix")
        {
            REQUIRE(Tango::detail::remove_idl_prefix(unqual_event_name_intr) == unqual_event_name_intr);
            REQUIRE(Tango::detail::remove_idl_prefix(unqual_event_name) == "change");
        }

        WHEN("add an idl prefix")
        {
            REQUIRE(Tango::detail::add_idl_prefix("change") == unqual_event_name);
        }

        WHEN("extract the IDL version")
        {
            REQUIRE(Tango::detail::extract_idl_version_from_event_name(unqual_event_name) == 5);
            REQUIRE(Tango::detail::extract_idl_version_from_event_name(qual_event_name) == 5);
            REQUIRE(!Tango::detail::extract_idl_version_from_event_name(qual_event_name_intr).has_value());
        }

        WHEN("prefix the event name with idl")
        {
            REQUIRE(Tango::detail::insert_idl_for_compat(
                        "tango://127.0.0.1:11570/testserver/tests/1/short_attr#dbase=no.change") == qual_event_name);
        }

        WHEN("remove the idl with the event name")
        {
            REQUIRE(Tango::detail::remove_idl_for_compat(qual_event_name) ==
                    "tango://127.0.0.1:11570/testserver/tests/1/short_attr#dbase=no");
        }

        WHEN("we can get the event name")
        {
            REQUIRE(Tango::detail::get_event_name(qual_event_name) == "change");
            REQUIRE(Tango::detail::get_event_name(qual_event_name_intr) == "intr_change");
        }
    }
}
