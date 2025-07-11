#include "utils/test_server.h"
#include "utils/utils.h"
#include "utils/options.h"

#include <tango/tango.h>
#include <catch2/catch_session.hpp>

#include <string_view>
#include <iostream>
#include <thread>

namespace TangoTest
{

Options g_options;

int test_main(int argc, const char *argv[])
{
    using namespace Catch::Clara;

    Catch::Session session;

    auto cli = session.cli();
    cli |= Opt(g_options.log_file_per_test_case)["--log-file-per-test-case"](
        "create a log file for each test case, otherwise use a single log file for the whole run");
    cli |= Opt([](int value) { g_options.only_idl_version = value; },
               "idlversion")["--only-idl"]("only idl version of servers to run tests against");

    session.cli(cli);

    return session.run(argc, argv);
}

class event
{
  private:
    bool flag{false};
    std::condition_variable cv;
    std::mutex mutex;

  public:
    void set()
    {
        {
            std::lock_guard<std::mutex> lock{mutex};
            flag = true;
        }
        cv.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock{mutex};
        cv.wait(lock, [this] { return flag; });
    }

    void clear()
    {
        flag = false;
    }
};

class DummyBackgroundThread
{
  public:
    DummyBackgroundThread()
    {
        auto *start_bg_thread = std::getenv(TestServer::k_start_bg_thread);
        if(start_bg_thread && strncmp(start_bg_thread, "1", 1) == 0)
        {
            start_thread();
        }
    }

    ~DummyBackgroundThread()
    {
        m_stop_event.set();
        if(m_thread.joinable())
        {
            m_thread.join();
        }
    }

  private:
    void start_thread()
    {
        event start_event;
        m_thread = std::thread(
            [&]()
            {
                start_event.set();
                m_stop_event.wait();
            });
        start_event.wait();
    }

    std::thread m_thread;
    event m_stop_event;
};

int server_main(int argc, const char *argv[])
{
    DummyBackgroundThread bg_thread;
    try
    {
        auto *tg = Tango::Util::init(argc, (char **) argv);
        tg->set_trace_level(5);
        detail::setup_topic_log_appender(tg->get_ds_inst_name());
        Tango::Logging::get_core_logger()->set_level(log4tango::Level::DEBUG);
        tg->server_init();
        std::cout << TestServer::k_ready_string << '\n' << std::flush;
        tg->server_run();
        tg->server_cleanup();
    }
    catch(std::exception &ex)
    {
        std::cerr << "Server initialisation failed: " << ex.what() << "\n" << std::flush;
        return 2;
    }
    catch(Tango::DevFailed &ex)
    {
        Tango::Except::print_exception(ex);
        return 2;
    }
    catch(CORBA::Exception &ex)
    {
        Tango::Except::print_exception(ex);
        return 2;
    }

    return 0;
}

} // namespace TangoTest

namespace
{
// Returns true if `string` ends with `suffix`
bool ends_with(std::string_view string, std::string_view suffix)
{
    size_t size = string.size();
    size_t suffix_size = suffix.size();
    if(size < suffix_size)
    {
        return false;
    }

    std::string_view ending = string.substr(size - suffix_size);
    return ending == suffix;
}
} // namespace

int main(int argc, const char *argv[])
{
    std::string name{argv[0]};

    if(ends_with(name, TANGO_TEST_CATCH2_SERVER_BINARY_NAME))
    {
        return TangoTest::server_main(argc, argv);
    }
    else if(ends_with(name, TANGO_TEST_CATCH2_TEST_BINARY_NAME))
    {
        return TangoTest::test_main(argc, argv);
    }

    std::cerr << "Unexpected argv[0] " << name << "\n";
    return 1;
}
