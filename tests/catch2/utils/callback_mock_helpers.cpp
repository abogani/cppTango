#include "callback_mock_helpers.h"

#include <catch2/catch_test_macros.hpp>

namespace TangoTest
{

AttrReadEventCopyable::AttrReadEventCopyable(Tango::AttrReadEvent *event)
{
    REQUIRE(event != nullptr);

    device = event->device;
    attr_names = event->attr_names;
    if(event->argout)
    {
        argout = *(event->argout);
        delete event->argout;
        event->argout = nullptr;
    }
    err = event->err;
    errors = event->errors;
}

AttrWrittenEventCopyable::AttrWrittenEventCopyable(Tango::AttrWrittenEvent *event)
{
    REQUIRE(event != nullptr);

    device = event->device;
    attr_names = event->attr_names;
    err = event->err;
    errors = event->errors;
}

CmdDoneEventCopyable::CmdDoneEventCopyable(Tango::CmdDoneEvent *event)
{
    REQUIRE(event != nullptr);

    device = event->device;
    cmd_name = event->cmd_name;
    argout = event->argout;
    err = event->err;
    errors = event->errors;
}

} // namespace TangoTest
