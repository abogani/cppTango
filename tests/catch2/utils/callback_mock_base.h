#include <tango/tango.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>

namespace TangoTest
{

/// Common base class for all event types
template <typename TEvent, typename TEventCopyable = TEvent>
class CallbackMockBase : public Tango::CallBack
{
  public:
    using Event = TEvent;
    using EventCopyable = TEventCopyable;

    explicit CallbackMockBase() = default;
    CallbackMockBase(const CallbackMockBase &) = delete;
    CallbackMockBase(CallbackMockBase &&) = delete;
    CallbackMockBase &operator=(const CallbackMockBase &) = delete;
    CallbackMockBase &operator=(CallbackMockBase &&) = delete;

    constexpr static const std::chrono::milliseconds k_default_timeout{(2 * TANGO_TEST_CATCH2_DEFAULT_POLL_PERIOD) +
                                                                       300};

    std::optional<TEventCopyable> pop_next_event(std::chrono::milliseconds timeout = k_default_timeout)
    {
        auto doit = [&]()
        {
            TEventCopyable event = events.front();
            events.pop_front();
            return event;
        };

        std::unique_lock<std::mutex> lk(m);

        if(!events.empty())
        {
            return doit();
        }

        if(cv.wait_for(lk, timeout, [this] { return !events.empty(); }))
        {
            return doit();
        }

        return std::nullopt;
    }

  protected:
    void collect_event(TEventCopyable event)
    {
        {
            std::unique_lock<std::mutex> lk(m);
            events.emplace_back(event);
        }

        cv.notify_one();
    }

  private:
    std::deque<TEventCopyable> events{};
    std::mutex m;
    std::condition_variable cv;
};

} // namespace TangoTest
