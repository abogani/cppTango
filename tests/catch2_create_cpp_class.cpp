#include "catch2_common.h"

template <class Base>
class Loader : public Base
{
  public:
    using Base::Base;

    ~Loader() override { }

    void init_device() override { }

    void load_library(Tango::DevVarStringArray prefix)
    {
        std::vector<std::string> prefixes;

        for(std::size_t i = 0; i != prefix.length(); ++i)
        {
            prefixes.emplace_back(prefix[i]);
        }
        auto *dserver = Tango::Util::instance()->get_dserver_device();

        dserver->_create_cpp_class("DummyClass", "DummyClass", prefixes);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&Loader::load_library>("load_library"));
    }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(Loader, 6)

SCENARIO("An external library without prefix can be loaded")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        std::vector<std::string> env{"LD_LIBRARY_PATH=DummyClass/", "DYLD_LIBRARY_PATH=DummyClass/"};
        TangoTest::Context ctx{"loader", "Loader", idlver, env};
        auto device = ctx.get_proxy();
        THEN("An external library can be loaded without giving a prefix")
        {
            Tango::DeviceData args;
            Tango::DevVarStringArray prefix;

            prefix.length(0);

            args << prefix;

            REQUIRE_NOTHROW(device->command_inout("load_library", args));
        }
        AND_THEN("An external library can be loaded with giving improper prefixes")
        {
            Tango::DeviceData args;
            Tango::DevVarStringArray prefix;

            prefix.length(2);
            prefix[0] = "libtest";
            prefix[1] = "libtesttest";

            args << prefix;

            REQUIRE_NOTHROW(device->command_inout("load_library", args));
        }
    }
}

SCENARIO("An external library with a custom prefix can be loaded")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        std::vector<std::string> env{"LD_LIBRARY_PATH=DummyClass/prefix", "DYLD_LIBRARY_PATH=DummyClass/prefix"};
        TangoTest::Context ctx{"loader", "Loader", idlver, env};
        auto device = ctx.get_proxy();
        THEN("An external library can be loaded with giving a prefix")
        {
            Tango::DeviceData args;
            Tango::DevVarStringArray prefix;

            prefix.length(1);
            prefix[0] = "libtest";

            args << prefix;

            REQUIRE_NOTHROW(device->command_inout("load_library", args));
        }
        AND_THEN("An external library cannot be loaded if the proper prefix is not given")
        {
            using namespace TangoTest::Matchers;
            Tango::DeviceData args;
            Tango::DevVarStringArray prefix;

            prefix.length(1);
            prefix[0] = "libtesttest";

            args << prefix;

            REQUIRE_THROWS_MATCHES(device->command_inout("load_library", args),
                                   Tango::DevFailed,
                                   FirstErrorMatches(Reason(Tango::API_ClassNotFound)));
        }
    }
}

SCENARIO("An external library not starting with lib can be loaded")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        std::vector<std::string> env{"LD_LIBRARY_PATH=DummyClass/unprefix", "DYLD_LIBRARY_PATH=DummyClass/unprefix"};
        TangoTest::Context ctx{"loader", "Loader", idlver, env};
        auto device = ctx.get_proxy();
        THEN("An external library can be loaded with giving a prefix")
        {
            using namespace TangoTest::Matchers;
            Tango::DeviceData args;
            Tango::DevVarStringArray prefix;

            prefix.length(0);

            args << prefix;

            REQUIRE_NOTHROW(device->command_inout("load_library", args));
        }
    }
}
