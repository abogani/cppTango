// NOLINTBEGIN(*)

#include "old_common.h"

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        TEST_LOG << "usage: print_data <device>" << endl;
        exit(-1);
    }

    string device_name = argv[1];

    // Test boolean

    DeviceData din;
    bool in = true;
    bool out;
    din << in;
    TEST_LOG << "Data = " << din << endl;
    din >> out;
    assert(out == true);

    TEST_LOG << "   Boolean --> OK" << endl;

    // test short

    short s_in = 2;
    short s_out;
    din << s_in;
    TEST_LOG << "Data = " << din << endl;
    din >> s_out;
    assert(s_out == 2);

    TEST_LOG << "   Short --> OK" << endl;

    // test long

    DevLong l_in = 3;
    DevLong l_out;
    din << l_in;
    TEST_LOG << "Data = " << din << endl;
    din >> l_out;
    assert(l_out == 3);

    TEST_LOG << "   Long --> OK" << endl;

    // test float

    float f_in = (float) 3.1;
    float f_out;
    din << f_in;
    TEST_LOG << "Data = " << din << endl;
    din >> f_out;
    assert(f_out == (float) 3.1);

    TEST_LOG << "   Float --> OK" << endl;

    // test double

    double db_in = 1.3;
    double db_out;
    din << db_in;
    TEST_LOG << "Data = " << din << endl;
    din >> db_out;
    assert(db_out == 1.3);

    TEST_LOG << "   Double --> OK" << endl;

    // test unsigned short

    unsigned short us_in = 100;
    unsigned short us_out;
    din << us_in;
    TEST_LOG << "Data = " << din << endl;
    din >> us_out;
    assert(us_out == 100);

    TEST_LOG << "   Unsigned Short --> OK" << endl;

    // test unsigned long

    DevULong ul_in = 1000;
    DevULong ul_out;
    din << ul_in;
    TEST_LOG << "Data = " << din << endl;
    din >> ul_out;
    assert(ul_out == 1000);

    TEST_LOG << "   Unsigned Long --> OK" << endl;

    // test string

    string str("abc");
    string str_out;
    din << str;
    TEST_LOG << "Data = " << din << endl;
    din >> str_out;
    assert(str_out == "abc");

    TEST_LOG << "   String --> OK" << endl;

    // test DevVarCharArray

    vector<unsigned char> ch_in;
    vector<unsigned char> ch_out;
    ch_in.push_back(0);
    ch_in.push_back(1);
    din << ch_in;
    TEST_LOG << din << endl;
    din >> ch_out;
    assert(ch_out[0] == 0);
    assert(ch_out[1] == 1);

    TEST_LOG << "   DevVarCharArray --> OK" << endl;

    // test DevVarShortArray

    DevVarShortArray *sh_in = new DevVarShortArray(2);
    vector<short> sh_out;
    sh_in->length(2);
    (*sh_in)[0] = 10;
    (*sh_in)[1] = 20;
    din << sh_in;
    TEST_LOG << din << endl;
    din >> sh_out;
    assert(sh_out[0] == 10);
    assert(sh_out[1] == 20);

    TEST_LOG << "   DevVarShortArray --> OK" << endl;

    // test DevVarLongArray

    DevVarLongArray *lg_arr = new DevVarLongArray(2);
    vector<DevLong> lg_arr_out;
    lg_arr->length(2);
    (*lg_arr)[0] = 111;
    (*lg_arr)[1] = 222;
    din << lg_arr;
    TEST_LOG << din << endl;
    din >> lg_arr_out;
    assert(lg_arr_out[0] == 111);
    assert(lg_arr_out[1] == 222);

    TEST_LOG << "   DevVarLongArray --> OK" << endl;

    // test DevVarFloatArray

    DevVarFloatArray *fl_arr = new DevVarFloatArray(2);
    vector<float> fl_arr_out;
    fl_arr->length(2);
    (*fl_arr)[0] = (float) 1.11;
    (*fl_arr)[1] = (float) 2.22;
    din << fl_arr;
    TEST_LOG << din << endl;
    din >> fl_arr_out;
    assert(fl_arr_out[0] == (float) 1.11);
    assert(fl_arr_out[1] == (float) 2.22);

    TEST_LOG << "   DevVarFloatArray --> OK" << endl;

    // test DevVarDoubleArray

    DevVarDoubleArray *db_arr = new DevVarDoubleArray(2);
    vector<double> db_arr_out;
    db_arr->length(2);
    (*db_arr)[0] = 1.12;
    (*db_arr)[1] = 3.45;
    din << db_arr;
    TEST_LOG << din << endl;
    din >> db_arr_out;
    assert(db_arr_out[0] == 1.12);
    assert(db_arr_out[1] == 3.45);

    TEST_LOG << "   DevVarDoubleArray --> OK" << endl;

    // test DevVarUShortArray

    DevVarUShortArray *us_arr = new DevVarUShortArray(2);
    vector<unsigned short> us_arr_out;
    us_arr->length(2);
    (*us_arr)[0] = 11;
    (*us_arr)[1] = 22;
    din << us_arr;
    TEST_LOG << din << endl;
    din >> us_arr_out;
    assert(us_arr_out[0] == 11);
    assert(us_arr_out[1] == 22);

    TEST_LOG << "   DevVarUShortArray --> OK" << endl;

    // test DevVarULongArray

    DevVarULongArray *ul_arr = new DevVarULongArray(2);
    vector<DevULong> ul_arr_out;
    ul_arr->length(2);
    (*ul_arr)[0] = 1111;
    (*ul_arr)[1] = 2222;
    din << ul_arr;
    TEST_LOG << din << endl;
    din >> ul_arr_out;
    assert(ul_arr_out[0] == 1111);
    assert(ul_arr_out[1] == 2222);

    TEST_LOG << "   DevVarULongArray --> OK" << endl;

    // test DevVarStringArray

    DevVarStringArray *str_arr = new DevVarStringArray(2);
    vector<string> str_arr_out;
    str_arr->length(3);
    (*str_arr)[0] = Tango::string_dup("abc");
    (*str_arr)[1] = Tango::string_dup("def");
    (*str_arr)[2] = Tango::string_dup("ghi");
    din << str_arr;
    TEST_LOG << din << endl;
    din >> str_arr_out;
    assert(str_arr_out[0] == "abc");
    assert(str_arr_out[1] == "def");
    assert(str_arr_out[2] == "ghi");

    TEST_LOG << "   DevVarStringArray --> OK" << endl;

    // test DevVarLongStringArray

    DevVarLongStringArray *lgstr_arr = new DevVarLongStringArray();
    vector<DevLong> lg_lgstr;
    vector<string> str_lgstr;
    lgstr_arr->lvalue.length(2);
    lgstr_arr->lvalue[0] = 1110;
    lgstr_arr->lvalue[1] = 2220;
    lgstr_arr->svalue.length(2);
    lgstr_arr->svalue[0] = Tango::string_dup("zxc");
    lgstr_arr->svalue[1] = Tango::string_dup("qwe");
    din << lgstr_arr;
    TEST_LOG << din << endl;
    din.extract(lg_lgstr, str_lgstr);
    assert(lg_lgstr[0] == 1110);
    assert(lg_lgstr[1] == 2220);
    assert(str_lgstr[0] == "zxc");
    assert(str_lgstr[1] == "qwe");

    TEST_LOG << "   DevVarLongStringArray --> OK" << endl;

    // test DevVarDoubleStringArray

    DevVarDoubleStringArray *dbstr_arr = new DevVarDoubleStringArray();
    vector<double> db_dbstr;
    vector<string> str_dbstr;
    dbstr_arr->dvalue.length(2);
    dbstr_arr->dvalue[0] = 1.11;
    dbstr_arr->dvalue[1] = 22.2;
    dbstr_arr->svalue.length(3);
    dbstr_arr->svalue[0] = Tango::string_dup("iop");
    dbstr_arr->svalue[1] = Tango::string_dup("jkl");
    dbstr_arr->svalue[2] = Tango::string_dup("bnm");
    din << dbstr_arr;
    TEST_LOG << din << endl;
    din.extract(db_dbstr, str_dbstr);
    assert(db_dbstr[0] == 1.11);
    assert(db_dbstr[1] == 22.2);
    assert(str_dbstr[0] == "iop");
    assert(str_dbstr[1] == "jkl");
    assert(str_dbstr[2] == "bnm");

    TEST_LOG << "   DevVarDoubleStringArray --> OK" << endl;

    // test DevState

    DevState sta = Tango::STANDBY;
    DevState sta_out;
    din << sta;
    TEST_LOG << "State = " << din << endl;
    din >> sta_out;
    assert(sta_out == Tango::STANDBY);

    TEST_LOG << "   DevState --> OK" << endl;

    // test DevEncoded

    DevEncoded de;
    de.encoded_format = Tango::string_dup("the string");
    de.encoded_data.length(2);
    de.encoded_data[0] = 11;
    de.encoded_data[1] = 22;
    din << de;
    TEST_LOG << "DevEncoded = " << din << endl;
    DevEncoded dout;
    din >> dout;
    assert(!strcmp(dout.encoded_format, "the string"));
    assert(dout.encoded_data.length() == 2);
    assert(dout.encoded_data[0] == 11);
    assert(dout.encoded_data[1] == 22);

    TEST_LOG << "   DevEncoded --> OK" << endl;

    // Attribute

    DeviceAttribute da;
    TEST_LOG << da << endl;
    TEST_LOG << "Empty attribute OK" << endl;

    short s_attr = 20;
    da << s_attr;
    TEST_LOG << da << endl;
    TEST_LOG << "DA with init value OK" << endl;

    DeviceProxy dev(device_name);

    da = dev.read_attribute("Long_attr");
    TEST_LOG << "Attribute read" << endl;
    TEST_LOG << da << endl;
    Tango::DevLong la;
    da >> la;
    assert(la == 1246);

    TEST_LOG << "    DeviceAttribute --> OK" << endl;

    return 0;
}

// NOLINTEND(*)
