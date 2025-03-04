#include "utils/utils.h"

#include <tango/tango.h>

#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>

#include <deque>
#include <optional>
#include <regex>

namespace
{

struct TestLogger : public TangoTest::Logger
{
    void log(const std::string &message) override
    {
        TANGO_LOG_INFO << "Got log: \"" << message << "\"";
        logs.push_back(message);
    }

    ~TestLogger() override { }

    void remove_port_in_use_logs()
    {
        auto it =
            std::remove_if(logs.begin(),
                           logs.end(),
                           [](const std::string &log) { return std::regex_search(log.begin(), log.end(), log_regex); });
        logs.erase(it, logs.end());
    }

    std::deque<std::string> logs;
    static std::regex log_regex;
};

std::regex TestLogger::log_regex{"port \\d+ in use", std::regex_constants::ECMAScript | std::regex_constants::icase};

struct LoggerSwapper
{
    LoggerSwapper()
    {
        logger = std::make_unique<TestLogger>();
        std::swap(TangoTest::TestServer::s_logger, logger);
    }

    ~LoggerSwapper()
    {
        std::swap(TangoTest::TestServer::s_logger, logger);
    }

    std::unique_ptr<TangoTest::Logger> logger;
};

} // namespace

template <class Base>
class Empty : public Base
{
  public:
    using Base::Base;

    ~Empty() override { }

    void init_device() override { }
};

TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(Empty<TANGO_BASE_CLASS>, Empty)

SCENARIO("test servers can be started and stopped")
{
    using TestServer = TangoTest::TestServer;
    LoggerSwapper ls;
    auto *logger = static_cast<TestLogger *>(TestServer::s_logger.get());

    GIVEN("a server started with basic device class")
    {
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "Empty::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "Empty");

        TestServer server;
        server.start("self_test", extra_args, env);
        INFO("server port is " << server.get_port() << " and redirect file is " << server.get_redirect_file());

        WHEN("we create a DeviceProxy to the device")
        {
            std::string fqtrl = TangoTest::make_nodb_fqtrl(server.get_port(), "TestServer/tests/1");

            auto dp = std::make_unique<Tango::DeviceProxy>(fqtrl);

            THEN("we can ping the device")
            {
                REQUIRE_NOTHROW(dp->ping());

                AND_THEN("the logs should only (maybe) contain messages about ports in use")
                {
                    using namespace Catch::Matchers;
                    logger->remove_port_in_use_logs();
                    REQUIRE_THAT(logger->logs, IsEmpty());
                }
            }
        }

#ifndef _TG_WINDOWS_
        // When we provide a specific port for our ORBendPoint, omniORB will
        // set the SO_REUSEADDR option for the socket we bind.  Unfortunately,
        // on Windows SO_REUSEADDR has different behaviour to most other BSD
        // socket implementations.  To cut a long story short this means if we
        // start two Tango device servers with the same port, Windows will
        // allow the second one to bind to the port even though it is already
        // in use by the first and it is basically random which device server
        // we end up talking to when we try to connect().
        //
        // The consequence of this is that if you run multiple copies of the
        // tests in parallel on Windows, you will occasionally get random
        // failures because two device servers are using the same port.
        //
        // If you are only running one copy of Catch2Tests.exe at a time, it
        // should be fine because because the tests do not use the same port
        // twice (except for this test which we are ifdef'ing away here).
        //
        // See
        // https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ
        // for a nice version of the long story.

        WHEN("we start another sever with the same port")
        {
            TestServer::s_next_port = server.get_port();
            TestServer server2;

            // Reset the logs in case there were any from the initial server
            // starting
            logger->logs.clear();
            server2.start("self_test2", extra_args, env);

            THEN("we can create device proxies and ping both devices")
            {
                for(int port : {server.get_port(), server2.get_port()})
                {
                    std::string fqtrl = TangoTest::make_nodb_fqtrl(port, "TestServer/tests/1");
                    auto dp = std::make_unique<Tango::DeviceProxy>(fqtrl);
                    REQUIRE_NOTHROW(dp->ping());
                }

                AND_THEN("we find a warning about the port being in use")
                {
                    using namespace Catch::Matchers;

                    REQUIRE_THAT(logger->logs, !IsEmpty());

                    const std::string expected = [](int port)
                    {
                        std::stringstream ss;
                        ss << "Port " << port << " in use";
                        return ss.str();
                    }(server.get_port());

                    // It has to be the first warning, as that is the first port we
                    // tried.
                    REQUIRE_THAT(logger->logs[0], StartsWith(expected));

                    AND_THEN("we only find logs about other ports in use (if any)")
                    {
                        using namespace Catch::Matchers;
                        logger->remove_port_in_use_logs();
                        REQUIRE_THAT(logger->logs, IsEmpty());
                    }
                }
            }
        }

#endif

        WHEN("we stop the server")
        {
            REQUIRE_NOTHROW(server.stop());

            THEN("there should be no logs generated")
            {
                using namespace Catch::Matchers;
                REQUIRE_THAT(logger->logs, IsEmpty());
            }
        }
    }
}

constexpr const char *k_helpful_message = "A helpful diagnostic message";

template <class Base>
class InitCrash : public Base
{
  public:
    InitCrash(Tango::DeviceClass *device_class, const std::string &dev_name) :
        Base(device_class, dev_name)
    {
        std::cout << k_helpful_message << "\n" << std::flush;
        std::exit(0); // Exit 0 as we should always report this
    }

    ~InitCrash() override { }

    void init_device() override { }
};

template <class Base>
class ExitCrash : public Base
{
  public:
    using Base::Base;

#ifdef _MSC_VER
  #pragma warning(push)
    // C4722: No return from dtor might cause a memory leak.
    // This is the point of the test, so we disable the warning here.
  #pragma warning(disable : 4722)
#endif

    ~ExitCrash() override
    {
        std::cout << k_helpful_message << "\n" << std::flush;
        std::exit(42); // Exit 42 as we should only report if the server fails
    }

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

    void init_device() override { }
};

template <class Base>
class DuringCrash : public Base
{
  public:
    using Base::Base;

    ~DuringCrash() override { }

    void init_device() override { }

    void read_attribute(Tango::Attribute &)
    {
        std::cout << k_helpful_message << "\n" << std::flush;
        std::exit(0); // Exit 0 as we should always report this
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&DuringCrash::read_attribute>("bad_attr", Tango::DEV_DOUBLE));
    }
};

template <class Base>
class InitTimeout : public Base
{
  public:
    InitTimeout(Tango::DeviceClass *device_class, const std::string &dev_name) :
        Base(device_class, dev_name)
    {
        std::cout << k_helpful_message << "\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }

    ~InitTimeout() override { }

    void init_device() override { }
};

template <class Base>
class ExitTimeout : public Base
{
  public:
    using Base::Base;

    ~ExitTimeout() override
    {
        std::cout << k_helpful_message << "\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }

    void init_device() override { }
};

TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(InitCrash<TANGO_BASE_CLASS>, InitCrash)
TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(ExitCrash<TANGO_BASE_CLASS>, ExitCrash)
TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DuringCrash<TANGO_BASE_CLASS>, DuringCrash)
TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(InitTimeout<TANGO_BASE_CLASS>, InitTimeout)
TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(ExitTimeout<TANGO_BASE_CLASS>, ExitTimeout)

SCENARIO("test server crashes and timeouts are reported")
{
    using TestServer = TangoTest::TestServer;
    LoggerSwapper ls;
    auto *logger = static_cast<TestLogger *>(TestServer::s_logger.get());

    GIVEN("a server that crashes on start")
    {
        TestServer server;
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "InitCrash::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "InitCrash");

        WHEN("we start the server")
        {
            std::optional<std::string> what = std::nullopt;
            try
            {
                server.start("self_test", extra_args, env);
            }
            catch(std::exception &ex)
            {
                what = ex.what();
            }

            THEN("a exception should be raised, reporting the helpful message and exit status")
            {
                using namespace Catch::Matchers;
                REQUIRE(what);
                REQUIRE_THAT(*what, ContainsSubstring(k_helpful_message));
                REQUIRE_THAT(*what, ContainsSubstring("exit status 0"));
            }

            THEN("there should be no (non-port-in-use) logs")
            {
                using namespace Catch::Matchers;
                logger->remove_port_in_use_logs();
                REQUIRE_THAT(logger->logs, IsEmpty());
            }
        }
    }

    GIVEN("a server that crashes on during a test")
    {
        TestServer server;
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "DuringCrash::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "DuringCrash");

        server.start("self_test", extra_args, env);

        WHEN("we run the test that crashes the device server")
        {
            std::string fqtrl = TangoTest::make_nodb_fqtrl(server.get_port(), "TestServer/tests/1");

            auto dp = std::make_unique<Tango::DeviceProxy>(fqtrl);
            Tango::DeviceAttribute da;
            REQUIRE_THROWS(da = dp->read_attribute("bad_attr"));

            AND_WHEN("we stop the server")
            {
                server.stop();

                THEN("there should be a single (non-port-in-use) log containing the helpful diagnostic and exit status")
                {
                    using namespace Catch::Matchers;

                    logger->remove_port_in_use_logs();
                    REQUIRE_THAT(logger->logs, SizeIs(1));
                    REQUIRE_THAT(logger->logs[0], ContainsSubstring(k_helpful_message));
                    REQUIRE_THAT(logger->logs[0], ContainsSubstring("exit status 0"));
                }
            }
        }
    }

    GIVEN("a server that crashes on exit")
    {
        TestServer server;
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "ExitCrash::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "ExitCrash");

        server.start("self_test", extra_args, env);

        WHEN("we stop the server")
        {
            server.stop();

            THEN("there should be a single (non-port-in-use) log containing the helpful diagnostic and exit status")
            {
                using namespace Catch::Matchers;

                logger->remove_port_in_use_logs();
                REQUIRE_THAT(logger->logs, SizeIs(1));
                REQUIRE_THAT(logger->logs[0], ContainsSubstring(k_helpful_message));
                REQUIRE_THAT(logger->logs[0], ContainsSubstring("exit status 42"));
            }
        }
    }

    GIVEN("a sever that times out on exit")
    {
        TestServer server;
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "ExitTimeout::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "ExitTimeout");

        server.start("self_test", extra_args, env);

        WHEN("we stop the server")
        {
            using namespace std::chrono_literals;
            server.stop(300ms);

            THEN("there should be a single (non-port-in-use) log, reporting the timeout the helpful diagnostic")
            {
                using namespace Catch::Matchers;

                logger->remove_port_in_use_logs();
                REQUIRE_THAT(logger->logs, SizeIs(1));
                REQUIRE_THAT(logger->logs[0], ContainsSubstring("Timeout waiting for TestServer to exit"));
                REQUIRE_THAT(logger->logs[0], ContainsSubstring(k_helpful_message));
            }
        }
    }
}

SCENARIO("test server timeouts during startup are reported", "[!mayfail]")
{
    using TestServer = TangoTest::TestServer;
    LoggerSwapper ls;
    auto *logger = static_cast<TestLogger *>(TestServer::s_logger.get());

    GIVEN("a server that times out on startup")
    {
        TestServer server;
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "InitTimeout::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "InitTimeout");

        WHEN("we start the server")
        {
            std::optional<std::string> what = std::nullopt;
            try
            {
                using namespace std::chrono_literals;
                server.start("self_test", extra_args, env, 300ms);
            }
            catch(std::exception &ex)
            {
                what = ex.what();
            }

            THEN("a exception should be raised, reporting the timeout and the helpful message")
            {
                using namespace Catch::Matchers;
                REQUIRE(what);
                REQUIRE_THAT(*what, ContainsSubstring("Timeout waiting for TestServer to start"));
                REQUIRE_THAT(*what, ContainsSubstring(k_helpful_message));
            }

            THEN("there should be no (non-port-in-use) logs")
            {
                using namespace Catch::Matchers;
                logger->remove_port_in_use_logs();
                REQUIRE_THAT(logger->logs, IsEmpty());
            }
        }
    }
}

template <class Base>
class TestEnvDS : public Base
{
  public:
    using Base::Base;

    ~TestEnvDS() override { }

    void init_device() override { }

    void read_attribute(Tango::Attribute &att)
    {
        envValue = std::getenv("TANGO_TEST_ENV");
        ptr = envValue.data();
        // fake lvalue for &
        att.set_value(&ptr);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&TestEnvDS::read_attribute>("env", Tango::DEV_STRING));
    }

  private:
    std::string envValue;
    char *ptr;
};

TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(TestEnvDS<TANGO_BASE_CLASS>, TestEnvDS)

SCENARIO("The env parameter for starting the server works")
{
    using TestServer = TangoTest::TestServer;

    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TestServer server;
        std::vector<std::string> extra_args = {"-nodb", "-dlist", "TestEnvDS::TestServer/tests/1"};
        std::vector<std::string> env;
        TangoTest::append_std_entries_to_env(env, "TestEnvDS");

        env.emplace_back("TANGO_TEST_ENV=abcd");
        server.start("self_test", extra_args, env);

        std::string fqtrl = TangoTest::make_nodb_fqtrl(server.get_port(), "TestServer/tests/1");

        auto device = std::make_unique<Tango::DeviceProxy>(fqtrl);

        REQUIRE(idlver == device->get_idl_version());

        WHEN("we read the attribute")
        {
            std::string att{"env"};

            Tango::DeviceAttribute da;
            REQUIRE_NOTHROW(da = device->read_attribute(att));
            THEN("the read value gives the expected value from the server")
            {
                std::string att_value;
                da >> att_value;
                REQUIRE(att_value == "abcd");

                AND_THEN("but the environment variable is not present in here")
                {
                    REQUIRE(std::getenv("TANGO_TEST_ENV") == nullptr);
                }
            }
        }
    }
}
