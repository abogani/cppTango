#include <catch2/matchers/catch_matchers_string.hpp>
#include <tango/tango.h>

#include <memory>

#include <omnithread.h>

#include "utils/utils.h"

// A thread that holds a TangoMonitor for the duration it is running.
class HoldMonitorThread : public omni_thread
{
  public:
    HoldMonitorThread(const HoldMonitorThread &) = delete;
    HoldMonitorThread(HoldMonitorThread &&) = delete;
    HoldMonitorThread &operator=(const HoldMonitorThread &) = delete;
    HoldMonitorThread &operator=(HoldMonitorThread &&) = delete;

  private:
    static void stop_and_join(HoldMonitorThread *thread)
    {
        thread->m_done.post();
        thread->join(nullptr);
    }

  public:
    // `omni_thread` has a weird API where you are not supposed to call `delete`,
    // but instead `join`, however, we need an RAII wrapper around this so that
    // we clean up in the case a `REQUIRE_<X>(..)` fails (which will throw
    // an exception).  This unique_ptr will handle this for us.
    using UniquePtr = std::unique_ptr<HoldMonitorThread, decltype(&stop_and_join)>;

    static UniquePtr create(const char *name)
    {
        return {new HoldMonitorThread{name}, stop_and_join};
    }

    // Wait until the thread has grabbed the monitor.
    void wait_until_started()
    {
        m_ready.wait();
    }

    // When called after `wait_until_started` this should throw a timeout error.
    void grab_monitor()
    {
        m_monitor.get_monitor();
    }

  private:
    HoldMonitorThread(const char *name) :
        m_monitor{name},
        m_ready{0},
        m_done{0}
    {
        m_monitor.timeout(500); // 500 ms
        start_undetached();
    }

    ~HoldMonitorThread() override { }

    void *run_undetached(void *) override
    {
        Tango::AutoTangoMonitor guard{&m_monitor};

        m_ready.post();
        m_done.wait();

        return nullptr;
    }

    Tango::TangoMonitor m_monitor; // The monitor being held by the thread
    omni_semaphore m_ready;        // post()'d when by the thread when it has grabbed the lock
    omni_semaphore m_done;         // post()'d when by the test when it has finished
};

SCENARIO("TangoMonitor provides good error messages")
{
    constexpr const char *k_name = "a-descriptive-name";

    GIVEN("a named TangoMonitor lock by another thread")
    {
        auto thread = HoldMonitorThread::create(k_name);

        WHEN("we try to grab the monitor")
        {
            thread->wait_until_started();
            THEN("we timeout with a DevFailed that mentions the lock's name and the threads involved")
            {
                using namespace TangoTest::Matchers;
                using namespace Catch::Matchers;

                std::string self = "Thread " + std::to_string(omni_thread::self()->id());
                std::string other = "held by thread " + std::to_string(thread->id());

                REQUIRE_THROWS_MATCHES(
                    thread->grab_monitor(),
                    Tango::DevFailed,
                    ErrorListMatches(AnyMatch(Reason(Tango::API_CommandTimedOut) &&
                                              DescriptionMatches(ContainsSubstring(k_name) && ContainsSubstring(self) &&
                                                                 ContainsSubstring(other)))));
            }
        }
    }
}
