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
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "Empty::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "Empty");
        env.emplace_back(
            [&start_background_thread]()
            {
                std::stringstream ss;
                ss << TestServer::k_start_bg_thread << "=" << (start_background_thread ? "1" : "0");
                return ss.str();
            }());

        server.start("test_signal_handler", extra_args, env);

        WHEN("we send signal " << signal_to_send)
        {
            server.send_signal(signal_to_send);
            THEN("the server exits successfully")
            {
                using namespace TangoTest::Matchers;

                REQUIRE_THAT(server.wait_for_exit(), IsSuccess());
            }
        }
    }
}
