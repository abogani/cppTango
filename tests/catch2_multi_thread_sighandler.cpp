#include "catch2/utils/platform/platform.h"

#include "catch2_common.h"

SCENARIO("Device server handles exiting signals correctly")
{
    using TestServer = TangoTest::TestServer;
    int signal_to_send = GENERATE(from_range(TestServer::relevant_sendable_signals()));
    bool start_background_thread = GENERATE(true, false);

    GIVEN("A device server " << (start_background_thread ? "with" : "without") << " a dummy background thread")
    {
        TestServer server;
        std::vector<std::string> extra_args = {"-nodb"};
        auto env = TangoTest::platform::default_env();
        {
            std::stringstream ss;
            ss << TestServer::k_start_bg_thread << "=" << (start_background_thread ? "1" : "0");
            env.emplace_back(ss.str());
        }
        {
            std::stringstream ss;
            ss << TangoTest::detail::k_log_file_env_var << "=" << TangoTest::get_current_log_file_path();
            env.emplace_back(ss.str());
        }
        server.start("test_signal_handler", extra_args, env);

        WHEN("we send signal " << signal_to_send)
        {
            server.send_signal(signal_to_send);
            THEN("the server exits successfully")
            {
                REQUIRE(server.wait_for_exit().is_success());
            }
        }
    }
}
