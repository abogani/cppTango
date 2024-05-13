#include "utils/utils.h"
#include "utils/options.h"

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_translate_exception.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <tango/tango.h>

#include <random>
#include <sstream>
#include <cstdlib>
#include <cstdio>

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
constexpr const char *k_log_file_env_var = "TANGO_TEST_LOG_FILE";
constexpr const char *k_log_directory_path = TANGO_TEST_CATCH2_LOG_DIRECTORY_PATH;
constexpr const char *k_filedb_directory_path = TANGO_TEST_CATCH2_FILEDB_DIRECTORY_PATH;
std::string g_current_log_file_path;
} // namespace

std::string make_nodb_fqtrl(int port, std::string_view device_name)
{
    std::stringstream ss;
    ss << "tango://127.0.0.1:" << port << "/" << device_name << "#dbase=no";
    return ss.str();
}

const char *get_current_log_file_path()
{
    return g_current_log_file_path.c_str();
}

namespace
{
/**
 * @brief Append standard environment entries to env vector
 *
 * Note, env does not own the strings it contains, instead the return value does
 * so the return value must be kept around while env is in use.
 *
 * @param env environment vector containing entries of the form "key=value"
 * @param class_name name of the Tango device class
 */
std::vector<std::string> append_std_entries_to_env(std::vector<const char *> &env, std::string_view class_name)
{
    std::vector<std::string> result;

    result.emplace_back(
        []()
        {
            std::stringstream ss;
            ss << k_log_file_env_var << "=" << g_current_log_file_path;
            return ss.str();
        }());
    env.emplace_back(result.back().c_str());

    result.emplace_back(
        [&]()
        {
            std::stringstream ss;
            ss << detail::k_enabled_classes_env_var << "=" << class_name;
            return ss.str();
        }());
    env.emplace_back(result.back().c_str());

    // append trailing NULL
    env.emplace_back(nullptr);

    return result;
}
} // namespace

// TODO:  Don't handle filedb strings directly, but instead manipulate a
// Tango::Filedatabase to build the database.
Context::Context(const std::string &instance_name,
                 const std::string &tmpl_name,
                 int idlversion,
                 const std::string &extra_filedb_contents,
                 std::vector<const char *> env)
{
    {
        static int filedb_count = 0;

        std::stringstream ss;
        ss << k_filedb_directory_path << "/";
        ss << detail::g_log_filename_prefix << filedb_count++ << ".db";

        m_filedb_path = ss.str();
    }

    std::string class_name = tmpl_name + "_" + std::to_string(idlversion);

    TANGO_LOG_INFO << "Starting server \"" << instance_name << "\" with device class "
                   << "\"" << class_name << "\" and filedb \"" << *m_filedb_path << "\".";

    {
        std::ofstream out{*m_filedb_path};

        auto write_and_log = [&out](auto &&...args)
        {
            if(API_LOGGER && API_LOGGER->is_info_enabled())
            {
                log4tango::LoggerStream::SourceLocation loc{::Tango::logging_detail::basename(__FILE__), __LINE__};
                auto stream = API_LOGGER->info_stream();
                stream << log4tango::_begin_log << loc << "Writing to filedb: '";
                (stream << ... << args);
                stream << "'";
            }
            (out << ... << args);
        };

        write_and_log("TestServer/", instance_name, "/DEVICE/", class_name, ": ", "TestServer/tests/1\n");
        write_and_log(extra_filedb_contents);
    }

    std::vector<std::string> owner = append_std_entries_to_env(env, class_name);

    std::string file_arg = std::string{"-file="} + *m_filedb_path;
    std::vector<const char *> extra_args = {file_arg.c_str()};
    m_server.start(instance_name, extra_args, env);

    TANGO_LOG_INFO << "Started server \"" << instance_name << "\" on port " << m_server.get_port() << " redirected to "
                   << m_server.get_redirect_file();
}

Context::Context(const std::string &instance_name,
                 const std::string &tmpl_name,
                 int idlversion,
                 std::vector<const char *> env)
{
    std::string class_name = tmpl_name + "_" + std::to_string(idlversion);

    std::string dlist_arg = [&]()
    {
        std::stringstream ss;
        ss << class_name << "::TestServer/tests/1";
        return ss.str();
    }();

    TANGO_LOG_INFO << "Starting server \"" << instance_name << "\" with device class "
                   << "\"" << class_name << "\"";

    std::vector<std::string> owner = append_std_entries_to_env(env, class_name);

    std::vector<const char *> extra_args = {"-nodb", "-dlist", dlist_arg.c_str()};
    m_server.start(instance_name, extra_args, env);

    TANGO_LOG_INFO << "Started server \"" << instance_name << "\" on port " << m_server.get_port() << " redirected to "
                   << m_server.get_redirect_file();
}

Context::~Context()
{
    m_server.stop();

    if(m_filedb_path.has_value())
    {
        std::remove(m_filedb_path->c_str());
    }
}

std::unique_ptr<Tango::DeviceProxy> Context::get_proxy()
{
    std::string fqtrl = make_nodb_fqtrl(m_server.get_port(), "TestServer/tests/1");

    return std::make_unique<Tango::DeviceProxy>(fqtrl);
}

void Context::stop_server(std::chrono::milliseconds timeout)
{
    m_server.stop(timeout);
}

// Listener to cleanup the Tango client ApiUtil singleton
class TangoListener : public Catch::EventListenerBase
{
    using Catch::EventListenerBase::EventListenerBase;

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

        // As we are not a device server, so we need to set the logger up ourselves
        Tango::_core_logger = new log4tango::Logger("Catch2Tests", log4tango::Level::DEBUG);

        if(!g_options.log_file_per_test_case)
        {
            // In this case we just have a single log file for the duration of
            // the test run.  We will include a timestamp in the filename just
            // to add something meaningful to help users distinguish log files.

            char timestamp[64];
            {
                std::time_t t = std::time(nullptr);
                std::tm *now = std::localtime(&t);
                std::strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", now);
            }

            g_current_log_file_path = [&]()
            {
                std::stringstream ss;
                ss << k_log_directory_path << "/" << detail::g_log_filename_prefix << timestamp << ".log";
                return ss.str();
            }();

            std::cout << "Logging to file " << g_current_log_file_path << "\n";

            detail::setup_topic_log_appender("test", g_current_log_file_path.c_str());
        }

        TANGO_LOG_INFO << "Test run \"" << info.name << "\" starting";
    }

    void testRunEnded(const Catch::TestRunStats &) override
    {
        Tango::ApiUtil::cleanup();
    }

    void testCaseStarting(const Catch::TestCaseInfo &info) override
    {
        if(g_options.log_file_per_test_case)
        {
            g_current_log_file_path = [&]()
            {
                std::stringstream ss;
                ss << k_log_directory_path << "/" << detail::filename_from_test_case_name(info.name, ".log");
                return ss.str();
            }();

            detail::setup_topic_log_appender("test", g_current_log_file_path.c_str());
        }

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

    auto end = test_case_name.end();
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

void setup_topic_log_appender(std::string_view topic, const char *filename)
{
    constexpr const char *k_appender_name = "test-log-file";

    log4tango::Logger *logger = Tango::Logging::get_core_logger();
    TANGO_ASSERT(logger);

    logger->remove_appender(k_appender_name);

    if(filename == nullptr)
    {
        filename = getenv(k_log_file_env_var);
        if(filename == nullptr)
        {
            std::cout << k_log_file_env_var << " is unset.  Not logging.";
            return;
        }
    }

    auto *appender = new log4tango::FileAppender(k_appender_name, filename);
    auto *layout = new log4tango::PatternLayout();
    std::stringstream pattern;
    pattern << std::setw(15) << topic << " %d{%H:%M:%S.%l} %p %F:%L %m%n";
    layout->set_conversion_pattern(pattern.str());
    appender->set_layout(layout);
    logger->add_appender(appender);
}

} // namespace detail

} // namespace TangoTest
