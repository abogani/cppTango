#include "utils/utils.h"

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_translate_exception.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <tango/tango.h>

#include <random>
#include <sstream>
#include <cstdlib>

CATCH_TRANSLATE_EXCEPTION(const Tango::DevFailed &ex)
{
    std::stringstream ss;
    Tango::Except::print_exception(ex, ss);
    return ss.str();
}

namespace
{
std::string reason(const Tango::DevFailed &e)
{
    return std::string(e.errors[0].reason.in());
}
} // namespace

namespace TangoTest
{

namespace
{
constexpr const char *k_log_file_environment_variable = "TANGO_TEST_LOG_FILE";
}

std::string make_nodb_fqtrl(int port, std::string_view device_name)
{
    std::stringstream ss;
    ss << "tango://127.0.0.1:" << port << "/" << device_name << "#dbase=no";
    return ss.str();
}

Context::Context(const std::string &instance_name, const std::string &tmpl_name, int idlversion)
{
    std::string dlist_arg = [&]()
    {
        std::stringstream ss;
        ss << tmpl_name << "_" << idlversion << "::TestServer/tests/1";
        return ss.str();
    }();

    TANGO_LOG_INFO << "Starting server \"" << instance_name << "\" with device class "
                   << "\"" << tmpl_name << "_" << idlversion;

    std::vector<const char *> extra_args = {"-nodb", "-dlist", dlist_arg.c_str()};
    m_server.start(instance_name, extra_args);

    TANGO_LOG_INFO << "Started server \"" << instance_name << "\" on port " << m_server.get_port() << " redirected to "
                   << m_server.get_redirect_file();
}

std::unique_ptr<Tango::DeviceProxy> Context::get_proxy()
{
    std::string fqtrl = make_nodb_fqtrl(m_server.get_port(), "TestServer/tests/1");

    return std::make_unique<Tango::DeviceProxy>(fqtrl);
}

// Listener to cleanup the Tango client ApiUtil singleton
class TangoListener : public Catch::EventListenerBase
{
    using Catch::EventListenerBase::EventListenerBase;

    // Buffer to hold environment variable TANGO_TEST_LOG_FILE. This must last
    // for the duration of the program as it is passed to putenv.
    char m_env_buffer[4096];

    void testRunStarting(const Catch::TestRunInfo &info) override
    {
        {
            constexpr const size_t k_prefix_length = 3;

            std::minstd_rand rng;
            rng.seed(Catch::getSeed());

            constexpr const char k_base62[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            static_assert(sizeof(k_base62) - 1 == 62); // - 1 for \0

            std::uniform_int_distribution dist{0, 61};

            // + 1 for the '_'
            detail::g_log_filename_prefix.reserve(k_prefix_length + 1);
            for(size_t i = 0; i < k_prefix_length; ++i)
            {
                detail::g_log_filename_prefix += k_base62[dist(rng)];
            }
            detail::g_log_filename_prefix += '_';
        }

        char timestamp[64];
        {
            std::time_t t = std::time(nullptr);
            std::tm *now = std::localtime(&t);
            std::strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", now);
        }

        std::string log_file_path = [&]()
        {
            std::stringstream ss;
            ss << TANGO_TEST_CATCH2_LOG_DIRECTORY_PATH << "/" << detail::g_log_filename_prefix << "_" << timestamp
               << ".log";
            return ss.str();
        }();

        std::cout << "Logging to file " << log_file_path << "\n";

        // Set an environment variable, used by setup_topic_log_appender here
        // and in the device servers.
        {
            std::stringstream ss;
            ss << k_log_file_environment_variable << "=" << log_file_path;
            strncpy(m_env_buffer, ss.str().c_str(), sizeof(m_env_buffer));
            putenv(m_env_buffer);
        }

        // As we are not a device server, so we need to set the logger up ourselves
        Tango::_core_logger = new log4tango::Logger("Catch2Tests", log4tango::Level::DEBUG);
        detail::setup_topic_log_appender("test");

        TANGO_LOG_INFO << "Test run \"" << info.name << "\" starting";
    }

    void testRunEnded(const Catch::TestRunStats &) override
    {
        Tango::ApiUtil::cleanup();
    }

    void testCaseStarting(const Catch::TestCaseInfo &info) override
    {
        TANGO_LOG_INFO << "Test case \"" << info.name << "\" starting";
    }

    void testCasePartialStarting(const Catch::TestCaseInfo &info, uint64_t part) override
    {
        TANGO_LOG_INFO << "Test case partial \"" << info.name << "\" part " << part << " starting";
    }

    void sectionStarting(const Catch::SectionInfo &info) override
    {
        TANGO_LOG_INFO << "Section \"" << info.name << "\" starting";
    }
};

CATCH_REGISTER_LISTENER(TangoListener)

DevFailedReasonMatcher::DevFailedReasonMatcher(const std::string &msg) :
    reason{msg}
{
}

bool DevFailedReasonMatcher::match(const Tango::DevFailed &exception) const
{
    return reason == ::reason(exception);
}

std::string DevFailedReasonMatcher::describe() const
{
    return "Exception reason is: " + reason;
}

namespace detail
{
std::string g_log_filename_prefix;

std::string filename_from_test_case_name(std::string_view test_case_name, std::string_view suffix)
{
    // This is the limit for path component length on Linux and Windows
    constexpr static size_t k_max_filename_length = 255;

    size_t max_length = k_max_filename_length - g_log_filename_prefix.size() - suffix.size();

    const char *end = test_case_name.end();
    if(test_case_name.size() > max_length)
    {
        end = test_case_name.begin() + max_length;
    }

    std::stringstream ss;
    ss << g_log_filename_prefix;
    std::transform(test_case_name.begin(),
                   end,
                   std::ostream_iterator<char>(ss),
                   [](char c)
                   {
                       if(c == ' ')
                       {
                           return '_';
                       }
                       return c;
                   });
    ss << suffix;

    std::string filename = ss.str();
    TANGO_ASSERT(filename.size() <= k_max_filename_length);

    return filename;
}

void setup_topic_log_appender(std::string_view topic)
{
    const char *filename = getenv(k_log_file_environment_variable);
    if(filename == nullptr)
    {
        return;
    }

    log4tango::Logger *logger = Tango::Logging::get_core_logger();
    TANGO_ASSERT(logger);

    auto *appender = new log4tango::FileAppender("test-log-file", filename);
    auto *layout = new log4tango::PatternLayout();
    std::stringstream pattern;
    pattern << std::setw(15) << topic << " %d{%H:%M:%S.%l} %p %F:%L %m%n";
    layout->set_conversion_pattern(pattern.str());
    appender->set_layout(layout);
    logger->add_appender(appender);
}

} // namespace detail

} // namespace TangoTest
