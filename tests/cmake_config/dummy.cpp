#include <tango/tango.h>

int main(int argc, const char *argv[])
{
    auto *tg = Tango::Util::init(argc, (char **) argv);
    tg->server_init();
    tg->server_run();
    tg->server_cleanup();

    bool useDb = Tango::Util::_UseDb;
    return 0;
}
