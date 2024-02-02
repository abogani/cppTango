#include "utils/test_server.h"

#include <tango/tango.h>
#include <catch2/catch_session.hpp>

#include <string_view>
#include <iostream>

namespace TangoTest
{

int test_main(int argc, const char *argv[])
{
    return Catch::Session().run(argc, argv);
}

int server_main(int argc, const char *argv[])
{
    try
    {
        auto *tg = Tango::Util::init(argc, (char **) argv);
        tg->server_init();
        std::cout << TestServer::k_ready_string << '\n' << std::flush;
        tg->server_run();
        tg->server_cleanup();
    }
    catch(std::exception &ex)
    {
        std::cerr << "Server initialisation failed: " << ex.what() << "\n" << std::flush;
        return 2;
    }
    catch(Tango::DevFailed &ex)
    {
        Tango::Except::print_exception(ex);
        return 2;
    }
    catch(CORBA::Exception &ex)
    {
        Tango::Except::print_exception(ex);
        return 2;
    }

    return 0;
}

} // namespace TangoTest

namespace
{
// Returns true if `string` ends with `suffix`
bool ends_with(std::string_view string, std::string_view suffix)
{
    size_t size = string.size();
    size_t suffix_size = suffix.size();
    if(size < suffix_size)
    {
        return false;
    }

    std::string_view ending = string.substr(size - suffix_size);
    return ending == suffix;
}
} // namespace

int main(int argc, const char *argv[])
{
    std::string name{argv[0]};

    if(ends_with(name, "TestServer"))
    {
        return TangoTest::server_main(argc, argv);
    }
    else if(ends_with(name, "Catch2Tests"))
    {
        return TangoTest::test_main(argc, argv);
    }

    std::cout << "Unexpected argv[0] " << name << "\n";
    return 1;
}
