#ifndef EnumTestSuite_h
#define EnumTestSuite_h

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME EnumAttTestSuite

class EnumAttTestSuite : public CxxTest::TestSuite
{
  protected:
    DeviceProxy *device1, *adm_dev;
    string device1_name;

  public:
    SUITE_NAME()
    {
        //
        // Arguments check -------------------------------------------------
        //

        // user arguments, obtained from the command line sequentially
        device1_name = CxxTest::TangoPrinter::get_param("device1");

        // always add this line, otherwise arguments will not be parsed correctly
        CxxTest::TangoPrinter::validate_args();

        //
        // Initialization --------------------------------------------------
        //

        try
        {
            device1 = new DeviceProxy(device1_name);
            device1->ping();

            string adm_name = device1->adm_name();
            adm_dev = new DeviceProxy(adm_name);
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
    }

    virtual ~SUITE_NAME()
    {
        if(CxxTest::TangoPrinter::is_restore_set("poll_att"))
        {
            DevVarStringArray rem_attr_poll;
            DeviceData din;
            rem_attr_poll.length(3);

            rem_attr_poll[0] = device1_name.c_str();
            rem_attr_poll[1] = "attribute";
            rem_attr_poll[2] = "Enum_attr_rw";
            din << rem_attr_poll;
            try
            {
                adm_dev->command_inout("RemObjPolling", din);
            }
            catch(Tango::DevFailed &e)
            {
                Tango::Except::print_exception(e);
            }
        }

        if(CxxTest::TangoPrinter::is_restore_set("dyn_enum_att"))
        {
            string att_name("DynEnum_attr");
            DbAttribute dba(att_name, device1_name);
            DbData dbd;
            DbDatum a(att_name);
            a << (short) 1;
            dbd.push_back(a);
            dbd.push_back(DbDatum("enum_labels"));
            dba.delete_property(dbd);

            Tango::DevShort f_val = 2;
            Tango::DeviceData din;
            din << f_val;
            device1->command_inout("ForbiddenEnumValue", din);
        }

        delete device1;
        delete adm_dev;
    }

    static SUITE_NAME *createSuite()
    {
        return new SUITE_NAME();
    }

    static void destroySuite(SUITE_NAME *suite)
    {
        delete suite;
    }

    //
    // Tests -------------------------------------------------------
    //

    // Test enum labels management in attribute config

    void test_enum_attribute_configuration()
    {
        Tango::AttributeInfoEx aie;
        TS_ASSERT_THROWS_NOTHING(aie = device1->get_attribute_config("Enum_attr_rw"));

        TS_ASSERT_EQUALS(aie.name, "Enum_attr_rw");
        TS_ASSERT_EQUALS(aie.writable, Tango::READ_WRITE);
        TS_ASSERT_EQUALS(aie.data_format, Tango::SCALAR);
        TS_ASSERT_EQUALS(aie.data_type, Tango::DEV_ENUM);

        TS_ASSERT_EQUALS(aie.enum_labels.size(), 4u);
        TS_ASSERT_EQUALS(aie.enum_labels[0], "North");
        TS_ASSERT_EQUALS(aie.enum_labels[1], "South");
        TS_ASSERT_EQUALS(aie.enum_labels[2], "East");
        TS_ASSERT_EQUALS(aie.enum_labels[3], "West");

        // Change enum labels

        aie.enum_labels[0] = "Nord";
        aie.enum_labels[1] = "Sud";
        aie.enum_labels[2] = "Est";
        aie.enum_labels[3] = "Ouest";

        Tango::AttributeInfoListEx aile;
        aile.push_back(aie);

        TS_ASSERT_THROWS_NOTHING(device1->set_attribute_config(aile));

        Tango::AttributeInfoEx aie2;
        TS_ASSERT_THROWS_NOTHING(aie2 = device1->get_attribute_config("Enum_attr_rw"));

        TS_ASSERT_EQUALS(aie2.enum_labels.size(), 4u);
        TS_ASSERT_EQUALS(aie2.enum_labels[0], "Nord");
        TS_ASSERT_EQUALS(aie2.enum_labels[1], "Sud");
        TS_ASSERT_EQUALS(aie2.enum_labels[2], "Est");
        TS_ASSERT_EQUALS(aie2.enum_labels[3], "Ouest");

        // Restart device and get att conf

        adm_dev->command_inout("RestartServer");
        std::this_thread::sleep_for(std::chrono::seconds(3));

        TS_ASSERT_THROWS_NOTHING(aie2 = device1->get_attribute_config("Enum_attr_rw"));

        TS_ASSERT_EQUALS(aie2.enum_labels.size(), 4u);
        TS_ASSERT_EQUALS(aie2.enum_labels[0], "Nord");
        TS_ASSERT_EQUALS(aie2.enum_labels[1], "Sud");
        TS_ASSERT_EQUALS(aie2.enum_labels[2], "Est");
        TS_ASSERT_EQUALS(aie2.enum_labels[3], "Ouest");

        // Reset to user default

        aile[0].enum_labels.clear();
        aile[0].enum_labels.push_back("");

        TS_ASSERT_THROWS_NOTHING(device1->set_attribute_config(aile));

        TS_ASSERT_THROWS_NOTHING(aie2 = device1->get_attribute_config("Enum_attr_rw"));

        TS_ASSERT_EQUALS(aie2.enum_labels.size(), 4u);
        TS_ASSERT_EQUALS(aie2.enum_labels[0], "North");
        TS_ASSERT_EQUALS(aie2.enum_labels[1], "South");
        TS_ASSERT_EQUALS(aie2.enum_labels[2], "East");
        TS_ASSERT_EQUALS(aie2.enum_labels[3], "West");

        // Two times the same label is invalid

        aile[0].enum_labels.clear();
        aile[0].enum_labels.push_back("North");
        aile[0].enum_labels.push_back("South");
        aile[0].enum_labels.push_back("East");
        aile[0].enum_labels.push_back("North");

        TS_ASSERT_THROWS_ASSERT(device1->set_attribute_config(aile),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        // Reset to lib default is invalid

        aile[0].enum_labels.clear();
        aile[0].enum_labels.push_back("Not specified");

        TS_ASSERT_THROWS_ASSERT(device1->set_attribute_config(aile),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        // Change enum labels number is not authorized from outside the Tango class

        aile[0].enum_labels.clear();
        aile[0].enum_labels.push_back("North");
        aile[0].enum_labels.push_back("South");

        TS_ASSERT_THROWS_ASSERT(device1->set_attribute_config(aile),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_NotSupportedFeature);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));
    }

    // Test read/write enumerated attribute

    void test_enum_attribute_reading()
    {
        DeviceAttribute da;
        TS_ASSERT_THROWS_NOTHING(da = device1->read_attribute("Enum_attr_rw"));

        short sh;
        TS_ASSERT_THROWS_NOTHING(da >> sh);
        TS_ASSERT_EQUALS(sh, 1);
        TS_ASSERT_EQUALS(da.get_type(), Tango::DEV_ENUM);

        TS_ASSERT_THROWS_NOTHING(da = device1->read_attribute("Enum_spec_attr_rw"));

        vector<short> v_sh_read;
        vector<short> v_sh_write;
        TS_ASSERT_THROWS_NOTHING(da.extract_read(v_sh_read));
        TS_ASSERT_THROWS_NOTHING(da.extract_set(v_sh_write));

        TS_ASSERT_EQUALS(v_sh_read.size(), 3u);
        TS_ASSERT_EQUALS(v_sh_read[0], 1);
        TS_ASSERT_EQUALS(v_sh_read[1], 0);
        TS_ASSERT_EQUALS(v_sh_read[2], 3);

        TS_ASSERT_EQUALS(v_sh_write.size(), 1u);
        TS_ASSERT_EQUALS(v_sh_write[0], 0);

        TS_ASSERT_EQUALS(da.get_type(), Tango::DEV_ENUM);
    }

    void test_enum_attribute_writing()
    {
        // Scalar att

        short sh_wr = 2;
        DeviceAttribute da_wr("Enum_attr_rw", sh_wr);
        TS_ASSERT_THROWS_NOTHING(device1->write_attribute(da_wr));

        DeviceAttribute da_read;
        TS_ASSERT_THROWS_NOTHING(da_read = device1->read_attribute("Enum_attr_rw"));
        vector<short> sh_rd;
        TS_ASSERT_THROWS_NOTHING(da_read >> sh_rd);
        TS_ASSERT_EQUALS(sh_rd.size(), 2u);
        TS_ASSERT_EQUALS(sh_rd[0], 1);
        TS_ASSERT_EQUALS(sh_rd[1], 2);

        // Spectrum att

        vector<short> v_sh_wr;
        v_sh_wr.push_back(1);
        v_sh_wr.push_back(1);
        DeviceAttribute da_wr2("Enum_spec_attr_rw", v_sh_wr);
        TS_ASSERT_THROWS_NOTHING(device1->write_attribute(da_wr2));

        DeviceAttribute da_read2;
        TS_ASSERT_THROWS_NOTHING(da_read2 = device1->read_attribute("Enum_spec_attr_rw"));

        vector<short> v_sh_read;
        vector<short> v_sh_write;
        TS_ASSERT_THROWS_NOTHING(da_read2.extract_read(v_sh_read));
        TS_ASSERT_THROWS_NOTHING(da_read2.extract_set(v_sh_write));

        TS_ASSERT_EQUALS(v_sh_read.size(), 3u);
        TS_ASSERT_EQUALS(v_sh_read[0], 1);
        TS_ASSERT_EQUALS(v_sh_read[1], 0);
        TS_ASSERT_EQUALS(v_sh_read[2], 3);

        TS_ASSERT_EQUALS(v_sh_write.size(), 2u);
        TS_ASSERT_EQUALS(v_sh_write[0], 1);
        TS_ASSERT_EQUALS(v_sh_write[1], 1);
    }

    void test_enum_attribute_write_read()
    {
        // Scalar att

        short sh_wr = 1;
        DeviceAttribute da_wr("Enum_attr_rw", sh_wr);
        DeviceAttribute da_rd;
        TS_ASSERT_THROWS_NOTHING(da_rd = device1->write_read_attribute(da_wr));

        vector<short> sh_rd;
        TS_ASSERT_THROWS_NOTHING(da_rd >> sh_rd);
        TS_ASSERT_EQUALS(sh_rd.size(), 2u);
        TS_ASSERT_EQUALS(sh_rd[0], 1);
        TS_ASSERT_EQUALS(sh_rd[1], 1);
        TS_ASSERT_EQUALS(da_rd.get_type(), Tango::DEV_ENUM);
    }

    void test_enum_attribute_memorized()
    {
        // Scalar att

        short sh_wr = 2;
        DeviceAttribute da_wr("Enum_attr_rw", sh_wr);
        TS_ASSERT_THROWS_NOTHING(device1->write_attribute(da_wr));

        // Restart the server

        TS_ASSERT_THROWS_NOTHING(adm_dev->command_inout("RestartServer"));

        std::this_thread::sleep_for(std::chrono::seconds(3));

        delete device1;
        device1 = new DeviceProxy(device1_name);

        // Read attribute

        DeviceAttribute da_read;
        TS_ASSERT_THROWS_NOTHING(da_read = device1->read_attribute("Enum_attr_rw"));
        vector<short> sh_rd;
        TS_ASSERT_THROWS_NOTHING(da_read >> sh_rd);
        TS_ASSERT_EQUALS(sh_rd.size(), 2u);
        TS_ASSERT_EQUALS(sh_rd[0], 1);
        TS_ASSERT_EQUALS(sh_rd[1], 2);
    }

    void test_enum_attribute_polling()
    {
        DeviceData din, dout;
        DevVarLongStringArray attr_poll;
        attr_poll.lvalue.length(1);
        attr_poll.svalue.length(3);

        // Start polling

        attr_poll.lvalue[0] = 300;
        attr_poll.svalue[0] = device1_name.c_str();
        attr_poll.svalue[1] = "attribute";
        attr_poll.svalue[2] = "Enum_attr_rw";
        din << attr_poll;
        TS_ASSERT_THROWS_NOTHING(adm_dev->command_inout("AddObjPolling", din));
        CxxTest::TangoPrinter::restore_set("poll_att");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Read attribute from polling buffer

        DeviceAttribute da;
        device1->set_source(CACHE);
        TS_ASSERT_THROWS_NOTHING(da = device1->read_attribute("Enum_attr_rw"));

        short sh;
        TS_ASSERT_THROWS_NOTHING(da >> sh);
        TS_ASSERT_EQUALS(sh, 1);
        TS_ASSERT_EQUALS(da.get_type(), Tango::DEV_ENUM);
        device1->set_source(CACHE_DEV);

        // Read data history

        vector<DeviceAttributeHistory> *hist = nullptr;

        TS_ASSERT_THROWS_NOTHING(hist = device1->attribute_history("Enum_attr_rw", 5));

        for(int i = 0; i < 5; i++)
        {
            bool fail = (*hist)[i].has_failed();
            TS_ASSERT(!fail);

            DevShort hist_val;
            (*hist)[i] >> hist_val;
            TS_ASSERT_EQUALS(hist_val, 1);
            TS_ASSERT_EQUALS((*hist)[i].get_type(), Tango::DEV_ENUM);
        }
        delete hist;

        // Stop polling

        DevVarStringArray rem_attr_poll;
        rem_attr_poll.length(3);

        rem_attr_poll[0] = device1_name.c_str();
        rem_attr_poll[1] = "attribute";
        rem_attr_poll[2] = "Enum_attr_rw";
        din << rem_attr_poll;
        TS_ASSERT_THROWS_NOTHING(adm_dev->command_inout("RemObjPolling", din));

        const DevVarStringArray *polled_devices;
        TS_ASSERT_THROWS_NOTHING(dout = adm_dev->command_inout("PolledDevice"));
        dout >> polled_devices;
        TS_ASSERT_EQUALS((*polled_devices).length(), 0u);

        CxxTest::TangoPrinter::restore_unset("poll_att");
    }

    void test_dynamic_attribute_of_enum_type()
    {
        Tango::ConstDevString ds = "Added_enum_attr";
        Tango::DeviceData din;
        din << ds;

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOAddAttribute", din));
        Tango::AttributeInfoEx aie;
        TS_ASSERT_THROWS_NOTHING(aie = device1->get_attribute_config("Added_enum_attr"));

        TS_ASSERT_EQUALS(aie.enum_labels.size(), 3u);
        TS_ASSERT_EQUALS(aie.enum_labels[0], "Red");
        TS_ASSERT_EQUALS(aie.enum_labels[1], "Green");
        TS_ASSERT_EQUALS(aie.enum_labels[2], "Blue");

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IORemoveAttribute", din));
    }

    void test_Dyn_enum()
    {
        CxxTest::TangoPrinter::restore_set("dyn_enum_att");

        Tango::DeviceAttribute da;
        Tango::DevShort sh;
        /*        TS_ASSERT_THROWS_NOTHING(da = device1->read_attribute("DynEnum_attr"));

                TS_ASSERT_THROWS_ASSERT(da >> sh,Tango::DevFailed &e,
                        TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrConfig);
                        TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));*/

        // Add labels to the enum

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("SetEnumLabels"));
        TS_ASSERT_THROWS_NOTHING(da = device1->read_attribute("DynEnum_attr"));

        da >> sh;

        TS_ASSERT_EQUALS(sh, 2);
        TS_ASSERT_EQUALS(da.get_type(), Tango::DEV_ENUM);

        // Get att config, add one label and check it is there in conf

        Tango::AttributeInfoEx aie, aie2;
        TS_ASSERT_THROWS_NOTHING(aie = device1->get_attribute_config("DynEnum_attr"));

        TS_ASSERT_EQUALS(aie.enum_labels.size(), 4u);

        Tango::ConstDevString ds = "Four";
        Tango::DeviceData din;
        din << ds;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("AddEnumLabel", din));

        TS_ASSERT_THROWS_NOTHING(aie2 = device1->get_attribute_config("DynEnum_attr"));
        TS_ASSERT_EQUALS(aie2.enum_labels.size(), 5u);
        TS_ASSERT_EQUALS(aie2.enum_labels[4], "Four");

        // Check forbidden value

        Tango::DevShort f_val = 1000;
        din << f_val;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("ForbiddenEnumValue", din));

        TS_ASSERT_THROWS_NOTHING(da = device1->read_attribute("DynEnum_attr"));
        TS_ASSERT_THROWS_ASSERT(
            da >> sh, Tango::DevFailed & e, TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
            TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        f_val = 4;
        din << f_val;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("ForbiddenEnumValue", din));

        f_val = 2;
        din << f_val;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("ForbiddenEnumValue", din));
    }

    void test_min_max_enum()
    {
        Tango::AttributeInfoEx config;
        Tango::AttributeInfoListEx config_list;
        Tango::AttributeInfoEx new_config;

        // Retrieve the config
        TS_ASSERT_THROWS_NOTHING(config = device1->get_attribute_config("Enum_attr_rw"));

        // Try to set max_alarm
        new_config = config;
        new_config.alarms.max_alarm = "2";
        config_list.push_back(new_config);

        TS_ASSERT_THROWS_ASSERT(device1->set_attribute_config(config_list),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        config_list.clear();

        // Try to set max_warning
        new_config = config;
        new_config.alarms.max_warning = "2";
        config_list.push_back(new_config);

        TS_ASSERT_THROWS_ASSERT(device1->set_attribute_config(config_list),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));
        config_list.clear();

        // Try to set min_alarm
        new_config = config;
        new_config.alarms.min_alarm = "1";
        config_list.push_back(new_config);

        TS_ASSERT_THROWS_ASSERT(device1->set_attribute_config(config_list),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));
        config_list.clear();

        // Try to set min_warning
        new_config = config;
        new_config.alarms.min_warning = "1";
        config_list.push_back(new_config);

        TS_ASSERT_THROWS_ASSERT(device1->set_attribute_config(config_list),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));
        config_list.clear();
    }

    void test_write_enum_attribute_out_of_range()
    {
        DeviceData write_val;
        write_val = device1->command_inout("GetEnumWriteValue");
        short before;
        write_val >> before;

        short sh_wr = -1;
        DeviceAttribute da_wr("Enum_attr_rw", sh_wr);
        std::vector<DeviceAttribute> list;
        list.push_back(da_wr);

        TS_ASSERT_THROWS_ASSERT(device1->write_attributes(list),
                                Tango::NamedDevFailedList & e,
                                TS_ASSERT(string(e.err_list[0].err_stack[0].reason.in()) == "API_WAttrOutsideLimit" &&
                                          e.errors[0].severity == Tango::ERR));

        write_val = device1->command_inout("GetEnumWriteValue");
        short after;
        write_val >> after;
        TS_ASSERT_EQUALS(before, after);
    }
};

#endif // EnumTestSuite_h
