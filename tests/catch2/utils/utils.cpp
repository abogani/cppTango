#include "utils/utils.h"

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_translate_exception.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <tango/tango.h>

#include <random>
#include <sstream>

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

    std::vector<const char *> extra_args = {"-nodb", "-dlist", dlist_arg.c_str()};
    m_server.start(instance_name, extra_args);
}

std::string Context::info()
{
    std::stringstream ss;
    const std::string &filename = m_server.get_redirect_file();
    size_t sep = filename.find_last_of("/\\");
    ss << "Started server on port " << m_server.get_port() << " redirected to " << filename.substr(sep + 1);
    return ss.str();
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

    void testRunStarting(const Catch::TestRunInfo &) override
    {
        {
            std::minstd_rand rng;
            rng.seed(Catch::getSeed());

            constexpr const char k_base62[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            static_assert(sizeof(k_base62) - 1 == 62); // - 1 for \0

            char uid[detail::k_log_filename_prefix_length];
            std::uniform_int_distribution dist{0, 61};
            for(size_t i = 0; i < detail::k_log_filename_prefix_length - 1; ++i)
            {
                uid[i] = k_base62[dist(rng)];
            }
            uid[detail::k_log_filename_prefix_length - 1] = '\0';
            memcpy(detail::g_log_filename_prefix, uid, 4);
        }
    }

    void testRunEnded(const Catch::TestRunStats &) override
    {
        Tango::ApiUtil::cleanup();
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
char g_log_filename_prefix[k_log_filename_prefix_length];
}

} // namespace TangoTest
