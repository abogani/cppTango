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

ContextDescriptor make_descriptor(const std::string &instance_name,
                                  const std::string &tmpl_name,
                                  int idlversion,
                                  const std::string &extra_filedb_contents,
                                  std::vector<std::string> env)
{
    ContextDescriptor result;

    ServerDescriptor srv;
    srv.instance_name = instance_name;
    srv.class_name = tmpl_name;
    srv.idlversion = idlversion;
    srv.extra_filedb_contents = extra_filedb_contents;
    srv.extra_env = env;

    result.servers.push_back(srv);
    return result;
}

ContextDescriptor
    make_descriptor(const std::string &instance_name, const std::string &class_name, std::vector<std::string> env)
{
    ContextDescriptor result;

    ServerDescriptor srv;
    srv.instance_name = instance_name;
    srv.class_name = class_name;
    srv.extra_env = std::move(env);

    result.servers.push_back(srv);
    return result;
}

ContextDescriptor make_descriptor(const std::string &instance_name,
                                  const std::string &tmpl_name,
                                  int idlversion,
                                  std::vector<std::string> env)
{
    ContextDescriptor result;

    ServerDescriptor srv;
    srv.instance_name = instance_name;
    srv.class_name = tmpl_name;
    srv.idlversion = idlversion;
    srv.extra_env = std::move(env);

    result.servers.push_back(srv);
    return result;
}

} // namespace

std::string make_nodb_fqtrl(int port, std::string_view device_name, std::string_view attr_name)
{
    std::stringstream ss;
    ss << "tango://127.0.0.1:" << port << "/" << device_name;
    if(!attr_name.empty())
    {
        ss << "/" << attr_name;
    }
    ss << "#dbase=no";
    return ss.str();
}

const char *get_current_log_file_path()
{
    return g_current_log_file_path.c_str();
}

std::string get_next_file_database_location()
{
    static int filedb_count = 0;

    std::stringstream ss;
    ss << k_filedb_directory_path << "/";
    ss << detail::g_log_filename_prefix << filedb_count++ << ".db";

    return ss.str();
}

Context::Context(const ContextDescriptor &desc)
{
    for(const auto &srv_desc : desc.servers)
    {
        add_server_job(srv_desc);
    }

    // TODO: Start these in parallel
    for(auto &job : m_server_jobs)
    {
        job.process.start(job.instance_name, job.extra_args, job.extra_env);
    }
}

void Context::add_server_job(const ServerDescriptor &desc)
{
    m_server_jobs.emplace_back();
    ServerJob &job = m_server_jobs.back();

    job.instance_name = desc.instance_name;
    job.device_name = "TestServer/tests/" + std::to_string(m_server_jobs.size());

    if(desc.idlversion.has_value())
    {
        job.class_name = make_class_name(desc.class_name, *desc.idlversion);
    }
    else
    {
        job.class_name = desc.class_name;
    }

    job.extra_env = desc.extra_env;
    append_std_entries_to_env(job.extra_env, job.class_name);

    // TODO:  Don't handle filedb strings directly, but instead manipulate a
    // Tango::Filedatabase to build the database.
    // Needs Filedatabase::DbAddDevice/DbAddServer implemented to do that.
    if(desc.extra_filedb_contents.has_value())
    {
        job.filedb_path = get_next_file_database_location();

        TANGO_LOG_INFO << "Setting up server \"" << job.instance_name << "\" with device class "
                       << "\"" << job.class_name << "\", device name \"" << job.device_name << "\" and filedb \""
                       << *job.filedb_path << "\".";

        {
            std::ofstream out{*job.filedb_path};

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

            write_and_log("TestServer/", job.instance_name, "/DEVICE/", job.class_name, ": ", job.device_name, "\n");
            write_and_log(*desc.extra_filedb_contents);
        }

        std::string file_arg = std::string{"-file="} + *job.filedb_path;
        job.extra_args = {file_arg};
    }
    else
    {
        std::string dlist_arg = [&]()
        {
            std::stringstream ss;
            ss << job.class_name << "::" << job.device_name;
            return ss.str();
        }();

        TANGO_LOG_INFO << "Setting up server \"" << job.instance_name << "\" with device class "
                       << "\"" << job.class_name << "\" and device name " << "\"" << job.device_name << "\"";

        job.extra_args = {"-nodb", "-dlist", dlist_arg};
    }
}

Context::Context(const std::string &instance_name,
                 const std::string &tmpl_name,
                 int idlversion,
                 const std::string &extra_filedb_contents,
                 std::vector<std::string> env) :
    Context{make_descriptor(instance_name, tmpl_name, idlversion, extra_filedb_contents, std::move(env))}
{
}

Context::Context(const std::string &instance_name, const std::string &class_name, std::vector<std::string> env) :
    Context{make_descriptor(instance_name, class_name, std::move(env))}
{
}

Context::Context(const std::string &instance_name,
                 const std::string &tmpl_name,
                 int idlversion,
                 std::vector<std::string> env) :
    Context{make_descriptor(instance_name, tmpl_name, idlversion, std::move(env))}
{
}

void Context::restart_server(std::chrono::milliseconds timeout)
{
    auto &job = get_only_job();

    job.process.start(job.instance_name, job.extra_args, job.extra_env, timeout);

    TANGO_LOG_INFO << "Started server \"" << job.instance_name << "\" on port " << job.process.get_port()
                   << " redirected to " << job.process.get_redirect_file();
}

Context::~Context()
{
    // TODO: Stop these in parallel
    for(auto &job : m_server_jobs)
    {
        job.process.stop();

        if(job.filedb_path.has_value())
        {
            std::remove(job.filedb_path->c_str());
        }
    }
}

std::string Context::get_fqtrl(std::string_view instance, std::string_view attr_name)
{
    const auto &job = get_job(instance);

    return make_nodb_fqtrl(job.process.get_port(), job.device_name, attr_name);
}

std::unique_ptr<Tango::DeviceProxy> Context::get_proxy()
{
    auto &job = get_only_job();

    std::string fqtrl = make_nodb_fqtrl(job.process.get_port(), job.device_name);

    return std::make_unique<Tango::DeviceProxy>(fqtrl);
}

std::unique_ptr<Tango::DeviceProxy> Context::get_proxy(std::string_view instance)
{
    const auto &job = get_job(instance);

    std::string fqtrl = make_nodb_fqtrl(job.process.get_port(), job.device_name);

    return std::make_unique<Tango::DeviceProxy>(fqtrl);
}

std::unique_ptr<Tango::DeviceProxy> Context::get_admin_proxy()
{
    auto &job = get_only_job();

    std::string fqtrl = make_nodb_fqtrl(job.process.get_port(), "dserver/TestServer/" + job.instance_name);

    return std::make_unique<Tango::DeviceProxy>(fqtrl);
}

std::unique_ptr<Tango::DeviceProxy> Context::get_admin_proxy(std::string_view instance)
{
    auto &job = get_job(instance);

    std::string fqtrl = make_nodb_fqtrl(job.process.get_port(), "dserver/TestServer/" + job.instance_name);

    return std::make_unique<Tango::DeviceProxy>(fqtrl);
}

const std::string &Context::get_redirect_file() const
{
    const auto &job = get_only_job();

    return job.process.get_redirect_file();
}

void Context::stop_server(std::chrono::milliseconds timeout)
{
    auto &job = get_only_job();

    job.process.stop(timeout);
}

ExitStatus Context::wait_for_exit(std::chrono::milliseconds timeout)
{
    auto &job = get_only_job();

    return job.process.wait_for_exit(timeout);
}

std::string Context::get_file_database_path()
{
    auto &job = get_only_job();

    if(job.filedb_path.has_value())
    {
        return *job.filedb_path;
    }

    throw std::runtime_error("Non existing filedatabase");
}

std::string Context::get_class_name()
{
    auto &job = get_only_job();

    return job.class_name;
}

Context::ServerJob &Context::get_only_job()
{
    TANGO_ASSERT(m_server_jobs.size() == 1);

    return m_server_jobs[0];
}

const Context::ServerJob &Context::get_only_job() const
{
    return const_cast<Context *>(this)->get_only_job();
}

Context::ServerJob &Context::get_job(std::string_view instance)
{
    auto job = std::find_if(
        m_server_jobs.begin(), m_server_jobs.end(), [=](const ServerJob &j) { return j.instance_name == instance; });

    TANGO_ASSERT(job != m_server_jobs.end());

    return *job;
}

const Context::ServerJob &Context::get_job(std::string_view instance) const
{
    return const_cast<Context *>(this)->get_job(instance);
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
        else
        {
            std::cout << "Logging to a file per test case.  Filename prefix is \"" << detail::g_log_filename_prefix
                      << "\"\n";
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

    // NOLINTNEXTLINE(readability-qualified-auto)
    for(auto it = test_case_name.begin(); it != end; ++it)
    {
        switch(*it)
        {
        case '<':
            [[fallthrough]];
        case '>':
            [[fallthrough]];
        case ':':
            [[fallthrough]];
        case '"':
            [[fallthrough]];
        case '/':
            [[fallthrough]];
        case '\\':
            [[fallthrough]];
        case '|':
            [[fallthrough]];
        case '?':
            [[fallthrough]];
        case '*':
            // None of these characters are allowed on all platforms we support
            // so let's just skip them
            continue;
        case ' ':
            // Spaces in filenames are annoying
            ss << '_';
            break;
        default:
            ss << *it;
        }
    }
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
    pattern << std::setw(15) << topic << " %d{%H:%M:%S.%l} %p %T(%t) %F:%L %m%n";
    layout->set_conversion_pattern(pattern.str());
    appender->set_layout(layout);
    logger->add_appender(appender);
}

} // namespace detail

} // namespace TangoTest
