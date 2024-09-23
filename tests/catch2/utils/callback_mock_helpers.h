#ifndef CATCH2_CALLBACK_MOCK_HELPERS_H
#define CATCH2_CALLBACK_MOCK_HELPERS_H

#include <tango/tango.h>

#include <vector>

namespace TangoTest
{

class AttrReadEventCopyable
{
  public:
    AttrReadEventCopyable(Tango::AttrReadEvent *event);

    Tango::DeviceProxy *device;
    std::vector<std::string> attr_names;
    std::vector<Tango::DeviceAttribute> argout;
    bool err;
    Tango::DevErrorList errors;
};

} // namespace TangoTest

#endif // CATCH2_CALLBACK_MOCK_HELPERS_H
