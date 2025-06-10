#include <tango/tango.h>

#include <functional>

#include "callback_mock_base.h"

#include "callback_mock_helpers.h"

namespace TangoTest
{

/// Common callback class for mocking in tests
///
/// Specializations are only needed if the event data class is not-copyable, to handle that
/// write a wrapper class in callback_mock_helpers.h and use that as second template argument
template <typename TEvent, typename TEventCopyable = TEvent>
class CallbackMock : public CallbackMockBase<TEvent>
{
    static_assert(std::is_same_v<TEvent, TEventCopyable>, "Non-matching types require specialization");

  public:
    void push_event(TEvent *event) override
    {
        REQUIRE(event != nullptr);
        this->collect_event(*event);
        this->raise_if_needed();
    }
};

template <>
class CallbackMock<Tango::AttrReadEvent, AttrReadEventCopyable>
    : public CallbackMockBase<Tango::AttrReadEvent, AttrReadEventCopyable>
{
  public:
    void attr_read(Tango::AttrReadEvent *event) override
    {
        collect_event(AttrReadEventCopyable(event));
        raise_if_needed();
    }

    std::optional<AttrReadEventCopyable> pop_next_event(const std::function<void()> &poll_func)
    {
        constexpr std::chrono::milliseconds time_slice{100};
        constexpr size_t num_slices{50};

        for(size_t i = 0; i < num_slices; i++)
        {
            poll_func();
            auto event = CallbackMockBase<Tango::AttrReadEvent, AttrReadEventCopyable>::pop_next_event(time_slice);

            if(event.has_value())
            {
                return event;
            }
        }

        return std::nullopt;
    }
};

template <>
class CallbackMock<Tango::AttrWrittenEvent, AttrWrittenEventCopyable>
    : public CallbackMockBase<Tango::AttrWrittenEvent, AttrWrittenEventCopyable>
{
  public:
    void attr_written(Tango::AttrWrittenEvent *event) override
    {
        collect_event(AttrWrittenEventCopyable(event));
        raise_if_needed();
    }

    std::optional<AttrWrittenEventCopyable> pop_next_event(const std::function<void()> &poll_func)
    {
        constexpr std::chrono::milliseconds time_slice{100};
        constexpr size_t num_slices{50};

        for(size_t i = 0; i < num_slices; i++)
        {
            poll_func();
            auto event =
                CallbackMockBase<Tango::AttrWrittenEvent, AttrWrittenEventCopyable>::pop_next_event(time_slice);

            if(event.has_value())
            {
                return event;
            }
        }

        return std::nullopt;
    }
};

template <>
class CallbackMock<Tango::CmdDoneEvent, CmdDoneEventCopyable>
    : public CallbackMockBase<Tango::CmdDoneEvent, CmdDoneEventCopyable>
{
  public:
    void cmd_ended(Tango::CmdDoneEvent *event) override
    {
        collect_event(CmdDoneEventCopyable(event));
        raise_if_needed();
    }

    std::optional<CmdDoneEventCopyable> pop_next_event(const std::function<void()> &poll_func)
    {
        constexpr std::chrono::milliseconds time_slice{100};
        constexpr size_t num_slices{50};

        for(size_t i = 0; i < num_slices; i++)
        {
            poll_func();
            auto event = CallbackMockBase<Tango::CmdDoneEvent, CmdDoneEventCopyable>::pop_next_event(time_slice);

            if(event.has_value())
            {
                return event;
            }
        }

        return std::nullopt;
    }
};

} // namespace TangoTest
