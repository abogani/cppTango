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

CATCH_TRANSLATE_EXCEPTION(const CORBA::Exception &ex)
{
    std::stringstream ss;
    Tango::Except::print_exception(ex, ss);
    return ss.str();
}

CATCH_TRANSLATE_EXCEPTION(const omni_thread_fatal &ex)
{
    std::stringstream ss;
    ss << "omni_thread_fatal error: " << strerror(ex.error) << " (" << ex.error << ")";
    return ss.str();
}

namespace
{

std::string make_class_name(const std::string &tmpl_name, int idlversion)
{
    return tmpl_name + "_" + std::to_string(idlversion);
}

} // namespace

namespace TangoTest
{

namespace
{
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
 * @brief Append standard environment entries to the env vector
 *
 * @param env environment vector containing entries of the form "key=value"
 * @param class_name name of the Tango device class
 */
void append_std_entries_to_env(std::vector<std::string> &env, std::string_view class_name)
{
    env.emplace_back(
        []()
        {
            std::stringstream ss;
            ss << detail::k_log_file_env_var << "=" << g_current_log_file_path;
            return ss.str();
        }());

    env.emplace_back(
        [&]()
        {
            std::stringstream ss;
            ss << detail::k_enabled_classes_env_var << "=" << class_name;
            return ss.str();
        }());
}

} // namespace

std::string get_next_file_database_location()
{
    static int filedb_count = 0;

    std::stringstream ss;
    ss << k_filedb_directory_path << "/";
    ss << detail::g_log_filename_prefix << filedb_count++ << ".db";

    return ss.str();
}

// TODO:  Don't handle filedb strings directly, but instead manipulate a
// Tango::Filedatabase to build the database.
// Needs Filedatabase::DbAddDevice/DbAddServer implemented to do that.
Context::Context(const std::string &instance_name,
                 const std::string &tmpl_name,
                 int idlversion,
                 const std::string &extra_filedb_contents,
                 std::vector<std::string> env) :
    m_instance_name{instance_name},
    m_extra_env{std::move(env)}
{
    m_filedb_path = get_next_file_database_location();
    m_class_name = make_class_name(tmpl_name, idlversion);

    TANGO_LOG_INFO << "Setting up server \"" << m_instance_name << "\" with device class "
                   << "\"" << m_class_name << "\" and filedb \"" << *m_filedb_path << "\".";

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

        write_and_log("TestServer/", m_instance_name, "/DEVICE/", m_class_name, ": ", "TestServer/tests/1\n");
        write_and_log(extra_filedb_contents);
    }

    append_std_entries_to_env(m_extra_env, m_class_name);

    std::string file_arg = std::string{"-file="} + *m_filedb_path;
    m_extra_args = {file_arg};

    restart_server();
}

Context::Context(const std::string &instance_name, const std::string &class_name, std::vector<std::string> env) :
    m_class_name{class_name},
    m_instance_name{instance_name},
    m_extra_env{std::move(env)}
{
    std::string dlist_arg = [&]()
    {
        std::stringstream ss;
        ss << m_class_name << "::TestServer/tests/1";
        return ss.str();
    }();

    TANGO_LOG_INFO << "Setting up server \"" << m_instance_name << "\" with device class "
                   << "\"" << m_class_name << "\"";

    append_std_entries_to_env(m_extra_env, m_class_name);

    m_extra_args = {"-nodb", "-dlist", dlist_arg};

    restart_server();
}

Context::Context(const std::string &instance_name,
                 const std::string &tmpl_name,
                 int idlversion,
                 std::vector<std::string> env) :
    Context{instance_name, make_class_name(tmpl_name, idlversion), env}
{
}

void Context::restart_server(std::chrono::milliseconds timeout)
{
    m_server.start(m_instance_name, m_extra_args, m_extra_env, timeout);

    TANGO_LOG_INFO << "Started server \"" << m_instance_name << "\" on port " << m_server.get_port()
                   << " redirected to " << m_server.get_redirect_file();
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

std::unique_ptr<Tango::DeviceProxy> Context::get_admin_proxy()
{
    std::string fqtrl = make_nodb_fqtrl(m_server.get_port(), "dserver/TestServer/" + m_instance_name);

    return std::make_unique<Tango::DeviceProxy>(fqtrl);
}

const std::string &Context::get_redirect_file() const
{
    return m_server.get_redirect_file();
}

void Context::stop_server(std::chrono::milliseconds timeout)
{
    m_server.stop(timeout);
}

int Context::wait_for_exit(std::chrono::milliseconds timeout)
{
    return m_server.wait_for_exit(timeout);
}

std::string Context::get_file_database_path()
{
    if(m_filedb_path.has_value())
    {
        return *m_filedb_path;
    }

    throw std::runtime_error("Non existing filedatabase");
}

std::string Context::get_class_name()
{
    return m_class_name;
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

    void assertionEnded(const Catch::AssertionStats &stats) override
    {
        if(stats.assertionResult.isOk())
        {
            return;
        }

        if(API_LOGGER && API_LOGGER->is_warn_enabled())
        {
            auto stream = API_LOGGER->warn_stream();
            stream << log4tango::_begin_log
                   << log4tango::LoggerStream::SourceLocation{::Tango::logging_detail::basename(__FILE__), __LINE__};

            stream << "Assertion";
            if(stats.assertionResult.hasExpression())
            {
                stream << " \"" << stats.assertionResult.getExpression() << "\"";
            }
            if(stats.assertionResult.hasExpandedExpression())
            {
                stream << " (" << stats.assertionResult.getExpandedExpression() << ")";
            }
            stream << " failed.";
        }
    }
};

CATCH_REGISTER_LISTENER(TangoListener)

namespace detail
{
std::string g_log_filename_prefix;

std::string filename_from_test_case_name(std::string_view test_case_name, std::string_view suffix)
{
    // This is the limit for path component length on Linux and Windows
    constexpr static size_t k_max_filename_length = 255;

    size_t max_length = k_max_filename_length - g_log_filename_prefix.size() - suffix.size();

    // With libstdc++ std::string_view::iterator is a const char * and
    // readability-qualified-auto recommends adding a * here.
    // However, with MSVC std::string_view::iterator is a class, and so adding
    // the * fails to compile on Windows.

    // NOLINTNEXTLINE(readability-qualified-auto)
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
            std::cout << k_log_file_env_var << " is unset. Not logging.\n";
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
