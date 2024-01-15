#include "common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if(argc != 3)
    {
        TEST_LOG << "usage: %s device loop" << endl;
        exit(-1);
    }

    string device_name = argv[1];
    auto loop = parse_as<int>(argv[2]);

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
    int i;

    // Test SCALAR short

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
#ifndef COMPAT
        assert(da.get_data_format() == Tango::FMT_UNKNOWN);
#endif

        try
        {
            da = device->read_attribute("Short_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        short sh;
        int data_type = da.get_type();
        da >> sh;
        assert(sh == 12);
        assert(data_type == Tango::DEV_SHORT);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar short --> OK" << endl;

    // Test SCALAR long

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
#ifndef COMPAT
        assert(da.get_data_format() == Tango::FMT_UNKNOWN);
#endif
        try
        {
            da = device->read_attribute("Long_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevLong lo;
        da >> lo;
        int data_type = da.get_type();
        assert(lo == 1246);
        assert(data_type == Tango::DEV_LONG);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar long --> OK" << endl;

    // Test SCALAR double

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Double_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        double db;
        da >> db;
        int data_type = da.get_type();
        assert(db == 3.2);
        assert(data_type == Tango::DEV_DOUBLE);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar double --> OK" << endl;

    // Test SCALAR string

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("String_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        string str;
        da >> str;
        int data_type = da.get_type();
        assert(str == "test_string");
        assert(data_type == Tango::DEV_STRING);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar C++ string --> OK" << endl;

    // Test SCALAR float

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Float_attr");
            float db;
            da >> db;
            int data_type = da.get_type();
            assert(db == 4.5);
            assert(data_type == Tango::DEV_FLOAT);
#ifndef COMPAT
            AttrDataFormat data_format = da.get_data_format();
            assert(data_format == Tango::SCALAR);
#endif
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
    }
    TEST_LOG << "   Scalar float --> OK" << endl;

    // Test SCALAR boolean

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Boolean_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        bool db;
        da >> db;
        assert(db == true);
        int data_type = da.get_type();
        assert(data_type == Tango::DEV_BOOLEAN);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar boolean --> OK" << endl;

    // Test SCALAR unsigned short

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("UShort_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        unsigned short db;
        da >> db;
        assert(db == 111);
        int data_type = da.get_type();
        assert(data_type == Tango::DEV_USHORT);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar unsigned short --> OK" << endl;

    // Test SCALAR unsigned char

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("UChar_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        unsigned char db;
        da >> db;
        assert(db == 88);
        int data_type = da.get_type();
        assert(data_type == Tango::DEV_UCHAR);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar unsigned char --> OK" << endl;

    // Test SCALAR long 64 bits

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Long64_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevLong64 lo;
        da >> lo;
        int data_type = da.get_type();

        assert(lo == 0x800000000LL);
        assert(data_type == Tango::DEV_LONG64);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar long 64 bits --> OK" << endl;

    // Test SCALAR unsigned long

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("ULong_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevULong lo;
        da >> lo;
        int data_type = da.get_type();
        assert(lo == 0xC0000000L);
        assert(data_type == Tango::DEV_ULONG);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar unsigned long --> OK" << endl;

    // Test SCALAR unsigned long 64 bits

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("ULong64_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevULong64 lo;
        da >> lo;
        int data_type = da.get_type();
        assert(lo == 0xC000000000000000LL);
        assert(data_type == Tango::DEV_ULONG64);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar unsigned long 64 bits --> OK" << endl;

    // Test SCALAR state

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
#ifndef COMPAT
        assert(da.get_data_format() == Tango::FMT_UNKNOWN);
#endif
        try
        {
            da = device->read_attribute("State_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevState lo;
        da >> lo;
        int data_type = da.get_type();

        assert(lo == Tango::FAULT);
        assert(data_type == Tango::DEV_STATE);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
#endif
    }
    TEST_LOG << "   Scalar state --> OK" << endl;

    // Test SCALAR DevEncoded

#ifndef COMPAT
    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Encoded_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevEncoded lo;
        da >> lo;
        int data_type = da.get_type();

        assert(::strcmp(lo.encoded_format.in(), "Which format?") == 0);
        assert(data_type == Tango::DEV_ENCODED);
  #ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SCALAR);
  #endif
        assert(lo.encoded_data.length() == 4);
        assert(lo.encoded_data[0] == 97);
        assert(lo.encoded_data[1] == 98);
        assert(lo.encoded_data[2] == 99);
        assert(lo.encoded_data[3] == 100);
    }
    TEST_LOG << "   Scalar DevEncoded --> OK" << endl;

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        EncodedAttribute att;
        int width, height;
        unsigned char *gray8;

  #ifdef TANGO_USE_JPEG
        try
        {
            da = device->read_attribute("Encoded_image");
            att.decode_gray8(&da, &width, &height, &gray8);
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }

        assert(width == 256);
        assert(height == 256);
        // Check a pixel (margin of 4 levels for jpeg loss)
        assert(gray8[128 + 128 * 256] >= 124);
        assert(gray8[128 + 128 * 256] <= 132);

        delete[] gray8;
  #else
        try
        {
            da = device->read_attribute("Encoded_image");
            att.decode_gray8(&da, &width, &height, &gray8);
            assert(false);
        }
        catch(Tango::DevFailed &e)
        {
            assert(string(e.errors[0].reason.in()) == API_EmptyDeviceAttribute);
        }
  #endif
    }
    TEST_LOG << "   Scalar DevEncoded (JPEG) --> OK" << endl;
#endif

    // Thirteen in one go

    for(i = 0; i < loop; i++)
    {
        vector<string> names;
        names.push_back("Short_attr");
        names.push_back("Long_attr");
        names.push_back("Double_attr");
        names.push_back("String_attr");
        names.push_back("Float_attr");
        names.push_back("Boolean_attr");
        names.push_back("UShort_attr");
        names.push_back("UChar_attr");
        names.push_back("Long64_attr_rw");
        names.push_back("ULong_attr_rw");
        names.push_back("ULong64_attr_rw");
        names.push_back("State_attr_rw");
#ifndef COMPAT
        names.push_back("Encoded_attr");

        DevEncoded enc;
#endif

        vector<DeviceAttribute> *received;

        try
        {
            received = device->read_attributes(names);
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        string str;
        DevLong lo;
        short sh;
        double db;
        float fl;
        bool bo;
        unsigned short ush;
        unsigned char uch;
        DevLong64 lo64;
        DevULong ulo;
        DevULong64 ulo64;
        DevState sta;

        (*received)[0] >> sh;
        assert(sh == 12);
        assert((*received)[0].get_type() == DEV_SHORT);
        (*received)[1] >> lo;
        assert(lo == 1246);
        assert((*received)[1].get_type() == DEV_LONG);
        (*received)[2] >> db;
        assert(db == 3.2);
        assert((*received)[2].get_type() == DEV_DOUBLE);
        (*received)[3] >> str;
        assert(str == "test_string");
        assert((*received)[3].get_type() == DEV_STRING);
        (*received)[4] >> fl;
        assert(fl == 4.5);
        assert((*received)[4].get_type() == DEV_FLOAT);
        (*received)[5] >> bo;
        assert(bo == true);
        assert((*received)[5].get_type() == DEV_BOOLEAN);
        (*received)[6] >> ush;
        assert(ush == 111);
        assert((*received)[6].get_type() == DEV_USHORT);
        (*received)[7] >> uch;
        assert(uch == 88);
        assert((*received)[7].get_type() == DEV_UCHAR);
        (*received)[8] >> lo64;
        assert(lo64 == 0x800000000LL);
        assert((*received)[8].get_type() == DEV_LONG64);
        (*received)[9] >> ulo;
        assert(ulo == 0xC0000000L);
        assert((*received)[9].get_type() == DEV_ULONG);
        (*received)[10] >> ulo64;
        assert(ulo64 == 0xC000000000000000LL);
        assert((*received)[10].get_type() == DEV_ULONG64);
        (*received)[11] >> sta;
        assert(sta == Tango::FAULT);
        assert((*received)[11].get_type() == DEV_STATE);
#ifndef COMPAT
        (*received)[12] >> enc;
        assert(enc.encoded_data.length() == 4);
        assert(::strcmp(enc.encoded_format.in(), "Which format?") == 0);
#endif

        delete received;
    }

    TEST_LOG << "   Thirteen in one call --> OK" << endl;

    //
    //---------------------------------------------------------------------------------------------
    //

    // Test SPECTRUM short

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Short_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<short> sh;
        bool ret = (da >> sh);

        assert(ret == true);
        assert(da.get_type() == DEV_SHORT);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SPECTRUM);
#endif
        assert(sh[0] == 10);
        assert(sh[1] == 20);
        assert(sh[2] == 30);
        assert(sh[3] == 40);
    }
    TEST_LOG << "   Spectrum short (C++ vector) --> OK" << endl;

    // Test SPECTRUM long

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Long_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<DevLong> lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_LONG);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SPECTRUM);
#endif
        assert(lo[0] == 0);
        assert(lo[1] == 1);
        assert(lo[2] == 2);
        assert(lo[3] == 3);
        assert(lo[9] == 9);
    }
    TEST_LOG << "   Spectrum long (C++ vector) --> OK" << endl;

    // Test SPECTRUM double

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Double_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<double> db;
        bool ret = (da >> db);
        int data_type = da.get_type();

        assert(ret == true);
        assert(db[0] == 1.11);
        assert(db[1] == 2.22);
        assert(data_type == Tango::DEV_DOUBLE);
    }
    TEST_LOG << "   Spectrum double (C++ vector) --> OK" << endl;

    // Test SPECTRUM string

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("String_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<string> str;
        bool ret = (da >> str);

        assert(ret == true);
        assert(da.get_type() == DEV_STRING);
        assert(str[0] == "Hello world");
        assert(str[1] == "Hello universe");
    }
    TEST_LOG << "   Spectrum string (C++ vector) --> OK" << endl;

    // Test SPECTRUM float

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Float_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<float> sh;
        bool ret = (da >> sh);

        assert(ret == true);
        assert(da.get_type() == DEV_FLOAT);
        assert(sh[0] == 4.5);
        assert(sh[1] == 8.5);
        assert(sh[2] == 16.5);
    }
    TEST_LOG << "   Spectrum float (C++ vector) --> OK" << endl;

    // Test SPECTRUM boolean

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Boolean_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<bool> sh;
        bool ret = (da >> sh);

        assert(ret == true);
        assert(da.get_type() == DEV_BOOLEAN);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SPECTRUM);
#endif
        assert(sh[0] == true);
        assert(sh[1] == true);
        assert(sh[2] == false);
        assert(sh[3] == true);
        assert(sh[4] == true);
    }
    TEST_LOG << "   Spectrum boolean (C++ vector) --> OK" << endl;

    // Test SPECTRUM unsigned short

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("UShort_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<unsigned short> sh;
        bool ret = (da >> sh);

        assert(ret == true);
        assert(da.get_type() == DEV_USHORT);
        assert(sh[0] == 333);
        assert(sh[1] == 444);
    }
    TEST_LOG << "   Spectrum unsigned short (C++ vector) --> OK" << endl;

    // Test SPECTRUM unsigned char

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("UChar_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<unsigned char> sh;
        bool ret = (da >> sh);

        assert(ret == true);
        assert(da.get_type() == DEV_UCHAR);
        assert(sh[0] == 28);
        assert(sh[1] == 45);
        assert(sh[2] == 156);
        assert(sh[3] == 34);
        assert(sh[4] == 200);
        assert(sh[5] == 12);
    }
    TEST_LOG << "   Spectrum unsigned char (C++ vector) --> OK" << endl;

    // Test SPECTRUM long 64 bits

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Long64_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<DevLong64> lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_LONG64);
        assert(lo[0] == 1000);
        assert(lo[1] == 10000);
        assert(lo[2] == 100000);
        assert(lo[3] == 0);
    }
    TEST_LOG << "   Spectrum long 64 bits (C++ vector) --> OK" << endl;

    // Test SPECTRUM unsigned long

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("ULong_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<DevULong> lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_ULONG);
        assert(lo[0] == 2222);
        assert(lo[1] == 22222);
        assert(lo[2] == 222222);
        assert(lo[3] == 0);
    }
    TEST_LOG << "   Spectrum unsigned long (C++ vector) --> OK" << endl;

    // Test SPECTRUM unsigned long 64 bits

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("ULong64_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<DevULong64> lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_ULONG64);
        assert(lo[0] == 8888);
        assert(lo[1] == 88888);
        assert(lo[2] == 888888);
        assert(lo[3] == 0);
    }
    TEST_LOG << "   Spectrum unsigned long 64 bits (C++ vector) --> OK" << endl;

    // Test SPECTRUM state

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("State_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        vector<DevState> lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_STATE);
        assert(lo[0] == Tango::ON);
        assert(lo[1] == Tango::OFF);
        assert(lo[2] == Tango::UNKNOWN);
    }
    TEST_LOG << "   Spectrum state (C++ vector) --> OK" << endl;

    //
    //-----------------------------------------------------------------------------------------
    //

    // Test SPECTRUM short (DevVarShortArray)

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Short_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarShortArray *sh;
        bool ret = (da >> sh);

        assert(ret == true);
        TEST_LOG << "   Short_spec_attr  da.get_type() = " << da.get_type() << endl;
        assert(da.get_type() == DEV_SHORT);
        assert((*sh)[0] == 10);
        assert((*sh)[1] == 20);
        assert((*sh)[2] == 30);
        assert((*sh)[3] == 40);

        delete sh;
    }
    TEST_LOG << "   Spectrum short (DevVarShortArray) --> OK" << endl;

    // Test SPECTRUM long

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Long_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarLongArray *lo;
        bool ret = (da >> lo);
        assert(ret == true);
        assert(da.get_type() == DEV_LONG);
        assert((*lo)[0] == 0);
        assert((*lo)[3] == 3);
        assert((*lo)[6] == 6);
        assert((*lo)[9] == 9);

        delete lo;
    }
    TEST_LOG << "   Spectrum long (DevVarLongArray) --> OK" << endl;

    // Test SPECTRUM double

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Double_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarDoubleArray *db;
        bool ret = (da >> db);
        assert(ret == true);
        assert(da.get_type() == DEV_DOUBLE);
        assert((*db)[0] == 1.11);
        assert((*db)[1] == 2.22);

        delete db;
    }
    TEST_LOG << "   Spectrum double (DevVarDoubleArray) --> OK" << endl;

    // Test SPECTRUM string

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("String_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarStringArray *str;
        bool ret = (da >> str);
        assert(ret == true);
        assert(da.get_type() == DEV_STRING);
        assert(!strcmp((*str)[0], "Hello world"));
        assert(!strcmp((*str)[1], "Hello universe"));

        delete str;
    }
    TEST_LOG << "   Spectrum string (DevVarStringArray) --> OK" << endl;

    // Test SPECTRUM float

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Float_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarFloatArray *lo;
        bool ret = (da >> lo);
        assert(ret == true);
        assert(da.get_type() == DEV_FLOAT);
        assert((*lo)[0] == 4.5);
        assert((*lo)[1] == 8.5);
        assert((*lo)[2] == 16.5);

        delete lo;
    }
    TEST_LOG << "   Spectrum float (DevVarFloatArray) --> OK" << endl;

    // Test SPECTRUM boolean

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Boolean_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarBooleanArray *lo;
        int data_type = da.get_type();
        bool ret = (da >> lo);
        assert(ret == true);
        assert((*lo)[0] == true);
        assert((*lo)[1] == true);
        assert((*lo)[2] == false);
        assert((*lo)[3] == true);
        assert((*lo)[4] == true);
        assert(data_type == Tango::DEV_BOOLEAN);

        delete lo;
    }
    TEST_LOG << "   Spectrum boolean (DevVarBooleanArray) --> OK" << endl;

    // Test SPECTRUM unsigned short

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("UShort_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarUShortArray *lo;
        bool ret = (da >> lo);
        assert(ret == true);
        assert(da.get_type() == DEV_USHORT);
        assert((*lo)[0] == 333);
        assert((*lo)[1] == 444);

        delete lo;
    }
    TEST_LOG << "   Spectrum unsigned short (DevVarUShortArray) --> OK" << endl;

    // Test SPECTRUM unsigned char

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("UChar_spec_attr");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarCharArray *lo;
        bool ret = (da >> lo);
        assert(ret == true);
        assert(da.get_type() == DEV_UCHAR);
        assert((*lo)[0] == 28);
        assert((*lo)[1] == 45);
        assert((*lo)[2] == 156);
        assert((*lo)[3] == 34);
        assert((*lo)[4] == 200);
        assert((*lo)[5] == 12);

        delete lo;
    }
    TEST_LOG << "   Spectrum unsigned char (DevVarCharArray) --> OK" << endl;

    // Test SPECTRUM long 64 bits

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Long64_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarLong64Array *lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_LONG64);
        assert((*lo)[0] == 1000);
        assert((*lo)[1] == 10000);
        assert((*lo)[2] == 100000);
        assert((*lo)[3] == 0);

        delete lo;
    }
    TEST_LOG << "   Spectrum long 64 bits (DevVarLong64Array) --> OK" << endl;

    // Test SPECTRUM unsigned long

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("ULong_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarULongArray *lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_ULONG);
        assert((*lo)[0] == 2222);
        assert((*lo)[1] == 22222);
        assert((*lo)[2] == 222222);
        assert((*lo)[3] == 0);

        delete lo;
    }
    TEST_LOG << "   Spectrum unsigned long (DevVarULongArray) --> OK" << endl;

    // Test SPECTRUM unsigned long 64 bits

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("ULong64_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarULong64Array *lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_ULONG64);
        assert((*lo)[0] == 8888);
        assert((*lo)[1] == 88888);
        assert((*lo)[2] == 888888);
        assert((*lo)[3] == 0);

        delete lo;
    }
    TEST_LOG << "   Spectrum unsigned long 64 bits (DevVarULong64Array) --> OK" << endl;

    // Test SPECTRUM state

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("State_spec_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarStateArray *lo;
        bool ret = (da >> lo);
        assert(ret == true);
        assert(da.get_type() == DEV_STATE);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::SPECTRUM);
#endif
        assert((*lo)[0] == Tango::ON);
        assert((*lo)[1] == Tango::OFF);
        assert((*lo)[2] == Tango::UNKNOWN);

        delete lo;
    }
    TEST_LOG << "   Spectrum state (DevVarStateArray) --> OK" << endl;

    // Test IMAGE short

    for(i = 0; i < loop; i++)
    {
        DeviceAttribute da;
        try
        {
            da = device->read_attribute("Short_ima_attr_rw");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
        DevVarShortArray *lo;
        bool ret = (da >> lo);

        assert(ret == true);
        assert(da.get_type() == DEV_SHORT);
#ifndef COMPAT
        AttrDataFormat data_format = da.get_data_format();
        assert(data_format == Tango::IMAGE);
#endif

        delete lo;
    }
    TEST_LOG << "   Image short (DevVarShortArray) --> OK" << endl;

    // Test exception on attribute data format unknown

    bool except = false;
    DeviceAttribute db;
#ifndef COMPAT
    db.set_exceptions(DeviceAttribute::unknown_format_flag);

    try
    {
        db.get_data_format();
    }
    catch(Tango::DevFailed &)
    {
        except = true;
    }
    assert(except == true);

    db.reset_exceptions(DeviceAttribute::unknown_format_flag);

    AttrDataFormat df = db.get_data_format();
    assert(df == Tango::FMT_UNKNOWN);

    TEST_LOG << "   Exception/Error for unknown attribute data format --> OK" << endl;
#endif

    // Test DeviceAttribute::get_type() on empty spectrum attribute

    for(i = 0; i < loop; i++)
    {
        bool except = false;
        try
        {
            std::vector<std::string> str_vec;
            Tango::DeviceAttribute da1("String_empty_spec_attr_rw", str_vec);
            da1.reset_exceptions(Tango::DeviceAttribute::isempty_flag);
            assert(da1.is_empty());
            assert(da1.get_type() == Tango::DEV_STRING);
            device->write_attribute(da1);

            Tango::DeviceAttribute da = device->read_attribute("String_empty_spec_attr_rw");
            da.reset_exceptions(Tango::DeviceAttribute::isempty_flag);
            assert(da.is_empty());
            assert(da.get_type() == Tango::DEV_STRING);
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            except = true;
        }
        assert(except == false);
    }
    TEST_LOG << "   Test DeviceAttribute::get_type() on empty spectrum attribute --> OK" << endl;

    // Test DeviceAttribute::get_type() on default DeviceAttribute object
    for(i = 0; i < loop; i++)
    {
        Tango::DeviceAttribute da;
        assert(da.get_type() == Tango::DATA_TYPE_UNKNOWN);
    }
    TEST_LOG << "   Test DeviceAttribute::get_type() on default DeviceAttribute object --> OK" << endl;

    // Test DeviceAttribute::get_type() on attribute throwing an exception
    TEST_LOG << "   Configuring DevTest to throw an exception when reading Long_Attr attribute..." << endl;
    try
    {
        Tango::DeviceData din;
        vector<short> short_vec;
        short_vec.push_back(5);
        short_vec.push_back(1);
        din << short_vec;
        device->command_inout("IOAttrThrowEx", din);
    }
    catch(const Tango::DevFailed &e)
    {
        Tango::Except::print_exception(e);
        bool IOAttrThrowEx_except = true;
        assert(IOAttrThrowEx_except == false);
    }
    for(i = 0; i < loop; i++)
    {
        Tango::DeviceAttribute da;
        try
        {
            da = device->read_attribute("Long_Attr");
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            TEST_LOG << "   Restoring DevTest Long_Attr default behaviour..." << endl;
            try
            {
                Tango::DeviceData din;
                vector<short> short_vec;
                short_vec.push_back(5);
                short_vec.push_back(0);
                din << short_vec;
                device->command_inout("IOAttrThrowEx", din);
            }
            catch(const Tango::DevFailed &e)
            {
                Tango::Except::print_exception(e);
                bool IOAttrThrowEx_except = true;
                assert(IOAttrThrowEx_except == false);
            }
            bool read_attribute_except = true;
            assert(read_attribute_except == false);
        }
        da.reset_exceptions(Tango::DeviceAttribute::isempty_flag);
        assert(da.is_empty());
        assert(da.get_type() == Tango::DATA_TYPE_UNKNOWN);
    }
    TEST_LOG << "   Restoring DevTest Long_Attr default behaviour..." << endl;
    try
    {
        Tango::DeviceData din;
        vector<short> short_vec;
        short_vec.push_back(5);
        short_vec.push_back(0);
        din << short_vec;
        device->command_inout("IOAttrThrowEx", din);
    }
    catch(const Tango::DevFailed &e)
    {
        Tango::Except::print_exception(e);
        bool IOAttrThrowEx_except = true;
        assert(IOAttrThrowEx_except == false);
    }
    TEST_LOG << "   Test DeviceAttribute::get_type() on attribute throwing an exception --> OK" << endl;

    delete device;

    return 0;
}
