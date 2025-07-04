// NOLINTBEGIN(*)

#include "old_common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if((argc == 1) || (argc > 3))
    {
        TEST_LOG << "usage: %s device" << endl;
        exit(-1);
    }

    string device_name = argv[1];

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << endl << "new DeviceProxy(" << device->name() << ") returned" << endl << endl;

    //**************************************************************************
    //
    //            Check that state and status are defined as attr.
    //
    //**************************************************************************

    try
    {
        string att_name("Short_spec_attr_rw");

        // Set attribute rds property (delta_t and delta_val)

        DbAttribute dba(att_name, device_name);
        DbDatum att_na("Short_spec_attr_rw");
        DbDatum dt("delta_t");
        DbDatum dv("delta_val");
        att_na << (short) 2;
        dt << (long) 1000;
        dv << (short) 2;

        DbData db;
        db.push_back(att_na);
        db.push_back(dt);
        db.push_back(dv);

        dba.put_property(db);

        // Restart the device

        string adm_name = device->adm_name();
        DeviceProxy adm_dev(adm_name);

        DeviceData dd;
        dd << device_name;
#ifdef VALGRIND
        adm_dev.set_timeout_millis(15000);
#endif
        adm_dev.command_inout("DevRestart", dd);

#ifdef WIN32
        std::this_thread::sleep_for(std::chrono::seconds(2));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
#endif

        delete device;
        device = new DeviceProxy(device_name);

        // Write the attribute without setting the rds

        vector<short> vs_w;
        vector<short> vs;
        vs_w.push_back(7);
        vs_w.push_back(8);
        DeviceAttribute da_w(att_name, vs_w);
        device->write_attribute(da_w);

        DeviceAttribute da;
        da = device->read_attribute(att_name);
        da >> vs;

        unsigned int i;
        for(i = 0; i < vs.size(); i++)
        {
            TEST_LOG << "Attribute vector " << i << " = " << vs[i] << endl;
        }

        // Get device state and status

        DevState sta;
        sta = device->state();
        TEST_LOG << "State = " << DevStateName[sta] << endl;

        string status;
        status = device->status();
        TEST_LOG << "Status = " << status << endl;

        string::size_type pos;
        pos = status.find("ON state");

        assert(sta == Tango::ON);
        assert(pos != string::npos);

#ifdef WIN32
        std::this_thread::sleep_for(std::chrono::seconds(2));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
#endif

        sta = device->state();
        TEST_LOG << "State = " << DevStateName[sta] << endl;

        status = device->status();
        TEST_LOG << "Status = " << status << endl;

        pos = status.find("ON state");

        assert(sta == Tango::ON);
        assert(pos != string::npos);

        TEST_LOG << "   Write without setting RDS --> OK" << endl;

        // Write the attribute with setting the rds

        vs_w.clear();
        vs_w.push_back(7);
        vs_w.push_back(25);
        DeviceAttribute da_w2(att_name, vs_w);
        device->write_attribute(da_w2);

        da = device->read_attribute(att_name);
        da >> vs;

        for(i = 0; i < vs.size(); i++)
        {
            TEST_LOG << "Attribute vector " << i << " = " << vs[i] << endl;
        }

        // Get device state and status

        sta = device->state();
        TEST_LOG << "State = " << DevStateName[sta] << endl;

        status = device->status();
        TEST_LOG << "Status = " << status << endl;

        pos = status.find("ON state");

        assert(sta == Tango::ON);
        assert(pos != string::npos);

#ifdef WIN32
        std::this_thread::sleep_for(std::chrono::seconds(2));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
#endif

        sta = device->state();
        TEST_LOG << "State = " << DevStateName[sta] << endl;
        assert(sta == Tango::ALARM);

        status = device->status();
        TEST_LOG << "Status = " << status << endl;

        pos = status.find("ALARM state");
        assert(pos != string::npos);
        pos = status.find("RDS");
        assert(pos != string::npos);
        pos = status.find("Short_spec_attr_rw");
        assert(pos != string::npos);

        TEST_LOG << "   Write with setting RDS --> OK" << endl;

        // Write the attribute without setting the rds

        vs_w.clear();
        vs_w.push_back(7);
        vs_w.push_back(8);
        DeviceAttribute da_w3(att_name, vs_w);
        device->write_attribute(da_w3);

        da = device->read_attribute(att_name);
        da >> vs;

        for(i = 0; i < vs.size(); i++)
        {
            TEST_LOG << "Attribute vector " << i << " = " << vs[i] << endl;
        }

        // Get device state and status

        sta = device->state();
        TEST_LOG << "State = " << DevStateName[sta] << endl;

        status = device->status();
        TEST_LOG << "Status = " << status << endl;

        pos = status.find("ON state");

        assert(sta == Tango::ON);
        assert(pos != string::npos);

#ifdef WIN32
        std::this_thread::sleep_for(std::chrono::seconds(2));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
#endif

        sta = device->state();
        TEST_LOG << "State = " << DevStateName[sta] << endl;

        status = device->status();
        TEST_LOG << "Status = " << status << endl;

        pos = status.find("ON state");

        assert(sta == Tango::ON);
        assert(pos != string::npos);

        TEST_LOG << "   Write without setting RDS --> OK" << endl;

        // Remove rds property (delta_t and delta_val) and restart device

        dba.delete_property(db);

        DeviceData late_dd;
        late_dd << device_name;
#ifdef VALGRIND
        adm_dev.set_timeout_millis(15000);
#endif
        adm_dev.command_inout("DevRestart", late_dd);
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    delete device;
    return 0;
}

// NOLINTEND(*)
