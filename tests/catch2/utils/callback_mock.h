#include <tango/tango.h>

#include "callback_mock_base.h"

namespace TangoTest
{

/// Common callback class for mocking in tests
///
/// Steps for supporting new types:
/// - Introduce a specialization for your event data type here and override the
///   designated function
template <typename TEvent>
class CallbackMock;

template <>
class CallbackMock<Tango::EventData> : public CallbackMockBase<Tango::EventData>
{
  public:
    void push_event(Tango::EventData *event) override
    {
        REQUIRE(event != nullptr);
        collect_event(*event);
    }
};

} // namespace TangoTest
