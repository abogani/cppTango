// NOLINTBEGIN(*)

#include "old_common.h"

int main()
{
    // Test empty

    DeviceData da;
    long lo;

    bitset<DeviceData::numFlags> flags;
    flags.reset(DeviceData::isempty_flag);
    da.exceptions(flags);

    bool ret = da >> lo;
    assert(ret == false);

    TEST_LOG << "   Extraction from empty object --> OK" << endl;

    flags.set(DeviceData::isempty_flag);
    da.exceptions(flags);

    try
    {
        da >> lo;
        assert(false);
    }
    catch(Tango::DevFailed &)
    {
    }

    TEST_LOG << "   Extraction from empty object (exception) --> OK" << endl;
    flags.reset();

    // Test wrong type

    DeviceData db;
    long l = 2;
    db << l;

    float fl;

    ret = db >> fl;
    assert(ret == false);

    TEST_LOG << "   Extraction with wrong type --> OK" << endl;

    flags.set(DeviceData::wrongtype_flag);
    db.exceptions(flags);

    try
    {
        db >> fl;
        assert(false);
    }
    catch(Tango::DevFailed &)
    {
    }

    TEST_LOG << "   Extraction with wrong type (exception) --> OK" << endl;

    // Test assignement operator

    DeviceData dd, dd_c;
    vector<string> v_str;
    v_str.push_back("abc");
    v_str.push_back("def");

    dd << v_str;
    dd_c = dd;

    vector<string> out;
    dd_c >> out;

    assert(out[0] == "abc");
    assert(out[1] == "def");

    TEST_LOG << "   assignement operator --> OK" << endl;

    // Test copy constructor

    DeviceData d;
    double db2 = 3.45;
    d << db2;

    DeviceData dc(d);

    double db_out;
    dc >> db_out;

    assert(db_out == db2);

    TEST_LOG << "   Copy constructor --> OK" << endl;

    // Test move assignement (if available)

    DeviceData ma;
    float fl_move = 3.0;
    ma << fl_move;

    DeviceData mb;
    mb = std::move(ma);
    float fl_move_out;
    mb >> fl_move_out;

    assert(fl_move_out == fl_move);

    TEST_LOG << "   Move assignement --> OK" << endl;

    return 0;
}

// NOLINTEND(*)
