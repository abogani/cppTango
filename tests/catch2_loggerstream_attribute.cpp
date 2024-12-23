#include "catch2_common.h"

#include <vector>
#include <sstream>

namespace details
{
}

//
// Tests -------------------------------------------------------
//
// Check the encoding functions
SCENARIO("LoggerStream displays proper information if the attribute is writable.")
{
    GIVEN("A logger")
    {
        log4tango::Logger logger("logger", log4tango::Level::INFO);
        std::stringstream output;
        log4tango::Appender *appender = new log4tango::OstreamAppender("appender", &output);
        appender->set_layout(new log4tango::Layout());
        logger.add_appender(appender);
        log4tango::LoggerStream ls = logger.get_stream(log4tango::Level::INFO, false);

        AND_GIVEN("A readable attribute")
        {
            std::vector<Tango::AttrProperty> properties;
            Tango::Attr tmp("attribute", 1l, Tango::AttrWriteType::READ, Tango::AssocWritNotSpec);
            Tango::Attribute attr(properties, tmp, "device", 0l);

            WHEN("Displaying the attribute in the logger")
            {
                REQUIRE_NOTHROW(ls << attr);
                REQUIRE_NOTHROW(ls.flush());
                THEN("It displays that the attribute is not writable")
                {
                    using namespace Catch::Matchers;
                    REQUIRE_THAT(output.str(), ContainsSubstring("Attribute is not writable"));
                }
            }
        }
        output.clear();

        AND_GIVEN("A readable with writable attribute")
        {
            std::vector<Tango::AttrProperty> properties;
            Tango::Attr tmp("attribute", 1l, Tango::AttrWriteType::READ_WITH_WRITE, "attribute");
            Tango::Attribute attr(properties, tmp, "device", 0l);

            WHEN("Displaying the attribute in the logger")
            {
                REQUIRE_NOTHROW(ls << attr);
                REQUIRE_NOTHROW(ls.flush());
                THEN("It displays that the attribute is writable")
                {
                    using namespace Catch::Matchers;
                    REQUIRE_THAT(output.str(), ContainsSubstring("Attribute is writable"));
                }
            }
        }
        output.clear();

        AND_GIVEN("A writable attribute")
        {
            std::vector<Tango::AttrProperty> properties;
            Tango::Attr tmp("attribute", 1l, Tango::AttrWriteType::WRITE, Tango::AssocWritNotSpec);
            Tango::Attribute attr(properties, tmp, "device", 0l);

            WHEN("Displaying the attribute in the logger")
            {
                REQUIRE_NOTHROW(ls << attr);
                REQUIRE_NOTHROW(ls.flush());
                THEN("It displays that the attribute is writable")
                {
                    using namespace Catch::Matchers;
                    REQUIRE_THAT(output.str(), ContainsSubstring("Attribute is writable"));
                }
            }
        }
        output.clear();

        AND_GIVEN("A readable writable attribute")
        {
            std::vector<Tango::AttrProperty> properties;
            Tango::Attr tmp("attribute", 1l, Tango::AttrWriteType::READ_WRITE, Tango::AssocWritNotSpec);
            Tango::Attribute attr(properties, tmp, "device", 0l);

            WHEN("Displaying the attribute in the logger")
            {
                REQUIRE_NOTHROW(ls << attr);
                REQUIRE_NOTHROW(ls.flush());
                THEN("It displays that the attribute is writable")
                {
                    using namespace Catch::Matchers;
                    REQUIRE_THAT(output.str(), ContainsSubstring("Attribute is writable"));
                }
            }
        }
        output.clear();

        AND_GIVEN("An attribute with unknown writability")
        {
            std::vector<Tango::AttrProperty> properties;
            Tango::Attr tmp("attribute", 1l, Tango::AttrWriteType::WT_UNKNOWN, Tango::AssocWritNotSpec);
            Tango::Attribute attr(properties, tmp, "device", 0l);

            WHEN("Displaying the attribute in the logger")
            {
                REQUIRE_NOTHROW(ls << attr);
                REQUIRE_NOTHROW(ls.flush());
                THEN("It displays that the attribute is not writable")
                {
                    using namespace Catch::Matchers;
                    REQUIRE_THAT(output.str(), ContainsSubstring("Attribute is not writable"));
                }
            }
        }
        output.clear();
    }
}
