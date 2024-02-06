#include "utils/utils.h"

#include <catch2/catch_translate_exception.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <tango/tango.h>

#include <sstream>

CATCH_TRANSLATE_EXCEPTION(const Tango::DevFailed &ex)
{
    std::stringstream ss;
    Tango::Except::print_exception(ex, ss);
    return ss.str();
}

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
    ss << "Started server on port " << m_server.get_port() << " redirected to \n" << m_server.get_redirect_file();
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

    void testRunEnded(Catch::TestRunStats const &) override
    {
        Tango::ApiUtil::cleanup();
    }
};

CATCH_REGISTER_LISTENER(TangoListener)

} // namespace TangoTest
