#include "catch2_common.h"

#include <type_traits>

#include <tango/internal/stl_corba_helpers.h>

namespace
{

using Var = Tango::DevErrorList_var;
using Seq = Tango::DevErrorList;
using Elem = Tango::DevError;

static_assert(std::is_same_v<Tango::detail::corba_ut_from_var_t<Var>, Seq>);
static_assert(std::is_same_v<Tango::detail::corba_ut_from_seq_t<Seq>, Elem>);
static_assert(std::is_same_v<Tango::detail::corba_ut_from_var_from_seq_t<Var>, Elem>);

static_assert(Tango::detail::is_corba_var_v<Var>);
static_assert(Tango::detail::is_corba_seq_v<Seq>);
static_assert(Tango::detail::is_corba_var_from_seq_v<Var>);

static_assert(Tango::detail::is_corba_var_v<Tango::DevVarStringArray_var>);
static_assert(Tango::detail::is_corba_seq_v<Tango::DevVarStringArray>);
static_assert(std::is_same_v<Tango::detail::corba_ut_from_seq_t<Tango::DevVarStringArray>, Tango::DevString>);
static_assert(
    std::is_same_v<Tango::detail::corba_ut_from_var_from_seq_t<Tango::DevVarStringArray_var>, Tango::DevString>);

// it is var class but does not have an underlying sequence
static_assert(!Tango::detail::is_corba_var_from_seq_v<Tango::DevString_var>);

static_assert(TangoTest::detail::has_corba_extract_operator_to<CORBA::Any, CORBA::Long>);
static_assert(TangoTest::detail::has_corba_extract_operator_to<CORBA::Any, CORBA::Boolean>);

// check iterator types
static_assert(std::is_same_v<decltype(begin(std::declval<Tango::DevVarStringArray &>())), Tango::DevString *>);
static_assert(std::is_same_v<decltype(end(std::declval<Tango::DevVarStringArray &>())), Tango::DevString *>);
static_assert(std::is_same_v<decltype(cbegin(std::declval<Tango::DevVarStringArray &>())), const Tango::DevString *>);
static_assert(std::is_same_v<decltype(cend(std::declval<Tango::DevVarStringArray &>())), const Tango::DevString *>);

} // anonymous namespace

SCENARIO("STL helpers for CORBA classes")
{
    GIVEN("an empty list")
    {
        Tango::DevErrorList err;

        WHEN("we can check that the sequence is empty")
        {
            REQUIRE(size(err) == 0u);
            REQUIRE(empty(err));
            REQUIRE(cbegin(err) == cend(err));
            REQUIRE(begin(err) == end(err));
        }
    }
    GIVEN("an unfilled var")
    {
        Tango::DevErrorList_var var;

        WHEN("we can check that the var is empty")
        {
            REQUIRE(size(var) == 0u);
            REQUIRE(empty(var));
            REQUIRE(cbegin(var) == cend(var));
            REQUIRE(begin(var) == end(var));
        }
    }
    GIVEN("a filled var pointing to an empty list")
    {
        auto *err = new Tango::DevErrorList;
        Tango::DevErrorList_var var(err);

        WHEN("we can check that the var is empty")
        {
            REQUIRE(size(var) == 0u);
            REQUIRE(empty(var));
            REQUIRE(cbegin(var) == cend(var));
            REQUIRE(begin(var) == end(var));
        }
    }
    GIVEN("a filled list")
    {
        Tango::DevErrorList err;
        err.length(3);

        WHEN("we can check its size")
        {
            REQUIRE(size(err) == 3u);
            REQUIRE(!empty(err));
            REQUIRE(cbegin(err) < cend(err));
            REQUIRE(begin(err) < end(err));

            {
                size_t i = 0;
                for([[maybe_unused]] auto &elem : err)
                {
                    i++;
                }
                REQUIRE(i == size(err));
            }

            // const overloads
            {
                const auto &cerr = err;
                size_t i = 0;
                for([[maybe_unused]] auto &elem : cerr)
                {
                    i++;
                }
                REQUIRE(i == size(cerr));
            }
        }
    }
    GIVEN("a filled var pointing to a filled list")
    {
        auto *err = new Tango::DevErrorList;
        err->length(3);
        Tango::DevErrorList_var var(err);

        WHEN("we can check its size")
        {
            REQUIRE(size(var) == 3u);
            REQUIRE(!empty(var));
            REQUIRE(cbegin(var) < cend(var));
            REQUIRE(begin(var) < end(var));

            {
                size_t i = 0;
                for([[maybe_unused]] auto &elem : var)
                {
                    i++;
                }
                REQUIRE(i == size(var));
            }

            // const overloads
            {
                const auto &cvar = var;
                size_t i = 0;
                for([[maybe_unused]] auto &elem : cvar)
                {
                    i++;
                }
                REQUIRE(i == size(cvar));
            }
        }
    }
    GIVEN("an empty DevVarStringArray")
    {
        Tango::DevVarStringArray list;

        WHEN("we can check that the sequence is empty")
        {
            REQUIRE(size(list) == 0u);
            REQUIRE(empty(list));
            REQUIRE(cbegin(list) == cend(list));
            REQUIRE(begin(list) == end(list));
        }
    }
    GIVEN("a filled DevVarStringArray")
    {
        Tango::DevVarStringArray list;
        list.length(3);

        WHEN("we can check its size")
        {
            REQUIRE(size(list) == 3u);
            REQUIRE(!empty(list));
            REQUIRE(cbegin(list) < cend(list));
            REQUIRE(begin(list) < end(list));

            {
                size_t i = 0;
                for([[maybe_unused]] auto &elem : list)
                {
                    i++;
                }
                REQUIRE(i == size(list));
            }

            // const overloads
            {
                const auto &clist = list;
                size_t i = 0;
                for([[maybe_unused]] auto &elem : clist)
                {
                    i++;
                }
                REQUIRE(i == size(clist));
            }
        }
    }
}
