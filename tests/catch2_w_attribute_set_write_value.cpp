#include "catch2_common.h"

#include <vector>

SCENARIO("Call set_write_value with a std::string")
{
    GIVEN("A scalar w_attribute of type string")
    {
        std::vector<Tango::AttrProperty> properties;
        Tango::Attr attribute("test", Tango::DEV_STRING, Tango::READ_WRITE);
        Tango::WAttribute attr(properties, attribute, "test/test/test", 0);

        THEN("The setpoint is not initialized")
        {
            Tango::DevString res;
            REQUIRE_NOTHROW(attr.get_write_value(res));

            using Catch::Matchers::Equals;
            REQUIRE_THAT(res, Equals("Not initialised"));
        }
        WHEN("Calling set_write_value with a vector of string")
        {
            std::vector<std::string> values;
            values.push_back("val_1");
            REQUIRE_NOTHROW(attr.set_write_value(values));
            THEN("the setpoint is properly set")
            {
                Tango::DevString res;
                REQUIRE_NOTHROW(attr.get_write_value(res));

                using Catch::Matchers::Equals;
                REQUIRE_THAT(res, Equals("val_1"));
            }
        }
    }
}

SCENARIO("Call set_write_value with a bool")
{
    GIVEN("A scalar w_attribute of type bool")
    {
        std::vector<Tango::AttrProperty> properties;
        Tango::Attr attribute("test", Tango::DEV_BOOLEAN, Tango::READ_WRITE);
        Tango::WAttribute attr(properties, attribute, "test/test/test", 0);

        THEN("The setpoint is not initialized")
        {
            bool res;
            REQUIRE_NOTHROW(attr.get_write_value(res));
            REQUIRE(res);
        }
        WHEN("Calling set_write_value with a vector of bool")
        {
            std::vector<bool> values;
            values.push_back(false);
            REQUIRE_NOTHROW(attr.set_write_value(values));
            THEN("the setpoint is properly set")
            {
                bool res;
                REQUIRE_NOTHROW(attr.get_write_value(res));
                REQUIRE(!res);
            }
        }
    }
}
