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

void remove_file(const std::string &filename)
{
    if(std::remove(filename.c_str()) != 0)
    {
        TANGO_LOG_WARN << "Failed to remove \"" << filename << "\": " << strerror(errno);
    }
}

} // namespace

int TestServer::s_next_port;
std::unique_ptr<Logger> TestServer::s_logger;

std::ostream &operator<<(std::ostream &os, const ExitStatus &status)
{
    using Kind = ExitStatus::Kind;

    switch(status.kind)
    {
    case Kind::Normal:
        os << status.code;
        break;
    case Kind::Aborted:
        os << "(Aborted by signal " << status.signal << ")";
        break;
    case Kind::AbortedNoSignal:
        os << "(Aborted)";
    }
    return os;
}

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
        TANGO_TEST_CATCH2_SERVER_BINARY_NAME,
        instance_name.c_str(),
        "-ORBendPoint",
        "", // filled in later
    };

    for(const auto &arg : extra_args)
    {
        args.push_back(arg);
    }

    std::vector<std::string> env = platform::default_env();

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
            f.close();

            remove_file(m_redirect_file);
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
        try
        {
            stop();
        }
        catch(std::exception &e)
        {
            std::stringstream ss;
            ss << "TestServer::stop() threw an exception during teardown: " << e.what();
            s_logger->log(ss.str());
        }
    }

    g_used_ports.push_back(m_port);
}

void TestServer::stop(std::chrono::milliseconds timeout)
{
    if(m_handle == nullptr)
    {
        return;
    }

    struct CombinedResult
    {
        enum Kind
        {
            Timeout, // exit_status undefined
            ExitedEarlyUnexpected,
            ExitedEarlyExpected,
            Exited,
        };

        Kind kind;
        ExitStatus exit_status;
    };

    using Kind = CombinedResult::Kind;
    CombinedResult result;
    {
        if(m_exit_status.has_value())
        {
            result.kind = Kind::ExitedEarlyExpected;
            result.exit_status = *m_exit_status;
        }
        else
        {
            using StopKind = platform::StopServerResult::Kind;
            using WaitKind = platform::WaitForStopResult::Kind;
            auto stop_result = platform::stop_server(m_handle);
            if(stop_result.kind == StopKind::Exiting)
            {
                auto wait_result = platform::wait_for_stop(m_handle, timeout);

                if(wait_result.kind == WaitKind::Timeout)
                {
                    result.kind = Kind::Timeout;
                }
                else
                {
                    result.kind = Kind::Exited;
                    result.exit_status = wait_result.exit_status;
                }
            }
            else
            {
                result.kind = Kind::ExitedEarlyUnexpected;
                result.exit_status = stop_result.exit_status;
            }
        }
    }

    switch(result.kind)
    {
    case Kind::Timeout:
    {
        std::stringstream ss;
        ss << "Timeout waiting for TestServer to exit. Server output:\n";
        std::ifstream f{m_redirect_file};
        append_logs(f, ss);

        s_logger->log(ss.str());
        break;
    }

    case Kind::ExitedEarlyUnexpected:
        [[fallthrough]];
    case Kind::ExitedEarlyExpected:
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
        bool exited_early = result.kind == Kind::ExitedEarlyExpected || result.kind == Kind::ExitedEarlyUnexpected;
        std::stringstream ss;
        if(exited_early || !result.exit_status.is_success())
        {
            ss << "TestServer exited with exit status " << result.exit_status << " during the test. Server output:\n";
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

        // When we are ExitedEarlyExpected then the test knows what the exit
        // status is. So, if it hasn't failed the test, then the exit code isn't
        // suspicious even if it non-zero.
        bool suspicious_exit_status = !result.exit_status.is_success() && result.kind != Kind::ExitedEarlyExpected;
        if(result.kind == Kind::ExitedEarlyUnexpected || suspicious_exit_status || test_has_failed)
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

    remove_file(m_redirect_file.c_str());

    m_handle = nullptr;
    m_redirect_file = "";
    m_exit_status = std::nullopt;
}

ExitStatus TestServer::wait_for_exit(std::chrono::milliseconds timeout)
{
    TANGO_ASSERT(is_running());

    using Kind = platform::WaitForStopResult::Kind;

    auto result = platform::wait_for_stop(m_handle, timeout);

    if(result.kind == Kind::Timeout)
    {
        throw std::runtime_error("Timeout exceeded");
    }

    // We don't report the contents of the redirect file here as don't know if
    // the test has failed or not at this point and we want to WARN with the
    // server output if it has.
    //
    // We save the m_exit_status for later so we can report it during stop.
    m_exit_status = result.exit_status;

    return result.exit_status;
}

} // namespace TangoTest
