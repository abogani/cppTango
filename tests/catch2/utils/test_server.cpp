#include "utils/test_server.h"
#include "utils/utils.h"

#include "utils/platform/platform.h"

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>

namespace TangoTest
{

namespace
{

class CatchLogger : public Logger
{
  public:
    void log(const std::string &message) override
    {
        TANGO_LOG_WARN << message;
        WARN(message);
    }

    ~CatchLogger() override { }
};

struct FilenameBuilder
{
    // The current test case we are running
    std::string current_test_case_name;
    // The current test case part that we are running
    uint64_t current_part_number;
    // Number of servers launch for this part
    uint64_t server_count;

    std::string build()
    {
        std::string suffix = [this]()
        {
            std::stringstream ss;
            ss << "_prt" << current_part_number;
            ss << "_srv" << server_count;
            return ss.str();
        }();
        server_count += 1;

        std::string filename = detail::filename_from_test_case_name(current_test_case_name, suffix);

        std::string path = [&]()
        {
            std::stringstream ss;
            ss << TangoTest::platform::k_output_directory_path << "/" << filename;
            return ss.str();
        }();

        return path;
    }
};

constexpr int k_min_port = 10000;
constexpr int k_max_port = 20000;

std::minstd_rand g_rng;
FilenameBuilder g_filename_builder;
std::vector<int> g_used_ports; // Work around for #1230

class PlatformListener : public Catch::EventListenerBase
{
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const &) override
    {
        g_rng.seed(Catch::getSeed());
        TestServer::s_logger = std::make_unique<CatchLogger>();

        {
            std::uniform_int_distribution dist{k_min_port, k_max_port};
            TestServer::s_next_port = dist(g_rng);
        }

        platform::init();
    }

    void testCasePartialStarting(Catch::TestCaseInfo const &info, uint64_t part_number) override
    {
        g_filename_builder.current_test_case_name = info.name;
        g_filename_builder.current_part_number = part_number;
        g_filename_builder.server_count = 0;
    }
};

CATCH_REGISTER_LISTENER(PlatformListener)

/** Append the logs from the in stream to the out stream
 */
bool append_logs(std::istream &in, std::ostream &out)
{
    for(std::string line; std::getline(in, line);)
    {
        out << "\t" << line << "\n";
    }

    return false;
}

[[noreturn]] void throw_runtime_error(const std::string &message)
{
    TANGO_LOG_ERROR << message;
    throw std::runtime_error(message);
}

} // namespace

int TestServer::s_next_port;
std::unique_ptr<Logger> TestServer::s_logger;

void TestServer::start(const std::string &instance_name,
                       const std::vector<std::string> &extra_args,
                       const std::vector<std::string> &extra_env,
                       std::chrono::milliseconds timeout)
{
    using Kind = platform::StartServerResult::Kind;

    TANGO_ASSERT(!is_running());
    TANGO_ASSERT(instance_name != "");

    m_redirect_file = g_filename_builder.build();

    std::vector<std::string> args{
        "TestServer",
        instance_name.c_str(),
        "-ORBendPoint",
        "", // filled in later
    };

    for(const auto &arg : extra_args)
    {
        args.push_back(arg);
    }

    std::vector<std::string> env{
#ifdef __APPLE__
        "PATH="
#endif
    };

    for(const auto &e : extra_env)
    {
        env.push_back(e);
    }

    // This will point to the slot after "-ORBendPoint"
    auto end_point_slot = std::find(args.begin(), args.end(), "");

    int num_tries = k_num_port_tries;

    // We are restarting the server on the same port and we want to fail if we
    // cannot get that port.
    if(m_port != -1)
    {
        s_next_port = m_port;
        num_tries = 1;
    }

    std::uniform_int_distribution dist{k_min_port, k_max_port};
    for(int i = 0; i < num_tries; ++i)
    {
        if(i != 0)
        {
            std::stringstream ss;
            ss << "Port " << m_port << " in use. Retrying...";
            s_logger->log(ss.str());
        }

        // We cannot reuse a port that the ORB has already connected to for a
        // new DeviceProxy due to #1230.

        do
        {
            m_port = dist(g_rng);
            std::swap(m_port, s_next_port);
        } while(std::find(g_used_ports.begin(), g_used_ports.end(), m_port) != g_used_ports.end());

        *end_point_slot = [&]()
        {
            std::stringstream ss;
            ss << "giop:tcp::" << m_port;
            return ss.str();
        }();

        TANGO_LOG_INFO << "Starting server with arguments "
                       << Catch::StringMaker<std::vector<std::string>>::convert(args) << " and environment "
                       << Catch::StringMaker<std::vector<std::string>>::convert(env);
        auto start_result = platform::start_server(args, env, m_redirect_file, k_ready_string, timeout);

        switch(start_result.kind)
        {
        case Kind::Started:
        {
            m_handle = start_result.handle;
            return;
        }
        case Kind::Timeout:
        {
            std::stringstream ss;
            ss << "Timeout waiting for TestServer to start. Server output:\n";
            std::ifstream f{m_redirect_file};

            m_handle = start_result.handle;
            stop(timeout);
            append_logs(f, ss);

            throw_runtime_error(ss.str());
        }
        case Kind::Exited:
        {
            std::stringstream ss;
            ss << "TestServer exited with exit status " << start_result.exit_status << ". Server output:\n";
            std::ifstream f{m_redirect_file};
            bool port_in_use = false;
            for(std::string line; std::getline(f, line);)
            {
                if(line.find(k_port_in_use_string) != std::string::npos)
                {
                    port_in_use = true;
                    break;
                }
                ss << "\t" << line << "\n";
            }

            std::remove(m_redirect_file.c_str());
            if(!port_in_use)
            {
                throw_runtime_error(ss.str());
            }
        }
        }
    }
}

TestServer::~TestServer()
{
    if(is_running())
    {
        stop();
    }

    g_used_ports.push_back(m_port);
}

void TestServer::stop(std::chrono::milliseconds timeout)
{
    if(m_handle == nullptr)
    {
        return;
    }

    using Kind = platform::StopServerResult::Kind;
    auto stop_result = platform::stop_server(m_handle, timeout);

    switch(stop_result.kind)
    {
    case Kind::Timeout:
    {
        std::stringstream ss;
        ss << "Timeout waiting for TestServer to exit. Server output:";
        std::ifstream f{m_redirect_file};
        append_logs(f, ss);

        s_logger->log(ss.str());
        break;
    }
    case Kind::ExitedEarly:
        [[fallthrough]];
    case Kind::Exited:
    {
        // `test_has_failed == true` if we are currently stopping the server
        // in some dtor while there is an uncaught exception in flight.  This
        // means either:
        //  - the code under test has throw an exception; or
        //  - some assertion has failed (where Catch2 will throw an exception)
        // In either case the test has failed.
        bool test_has_failed = std::uncaught_exceptions() > 0;
        std::stringstream ss;
        if(stop_result.kind == Kind::ExitedEarly || stop_result.exit_status != 0)
        {
            ss << "TestServer exited with exit status " << stop_result.exit_status
               << " during the test. Server output:\n";
        }
        else if(test_has_failed)
        {
            ss << "Test server exited cleanly, but we detected that test failed. Server output:\n";
        }
        else
        {
            ss << "Test server exited cleanly. Server output:\n";
        }
        std::ifstream f{m_redirect_file};
        append_logs(f, ss);

        if(stop_result.kind == Kind::ExitedEarly || stop_result.exit_status != 0 || test_has_failed)
        {
            s_logger->log(ss.str());
        }
        else
        {
            TANGO_LOG_INFO << ss.str();
        }

        break;
    }
    }

    std::remove(m_redirect_file.c_str());

    m_handle = nullptr;
    m_redirect_file = "";
}

} // namespace TangoTest
