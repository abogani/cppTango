#include "utils/entry_points.h"

#include "utils/bdd_server.h"

#include <tango/tango.h>
#include <catch2/catch_session.hpp>

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
        std::cout << BddServer::k_ready_string << '\n' << std::flush;
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
