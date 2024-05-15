// NOLINTBEGIN(*)

#include <array>
#include <cstdio>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME FileDatabaseTestSuite

class FileDatabaseTestSuite : public CxxTest::TestSuite
{
  private:
    static void create_dbfile(const std::string &filename, const std::string &device_name)
    {
        std::ofstream dbfile(filename);
        dbfile << "DeviceServer/instance/DEVICE/Class: " << device_name << '\n';
        dbfile.close();
    }

    static CORBA::Any as_any(std::vector<std::string> values)
    {
        auto *varstringarray = new DevVarStringArray();
        varstringarray->length(values.size());
        for(std::size_t i = 0; i != values.size(); i++)
        {
            (*varstringarray)[i] = Tango::string_dup(values[i].c_str());
        }
        CORBA::Any _as_any;
        _as_any <<= varstringarray;
        return _as_any;
    }

    static std::vector<std::string> from_any(const CORBA::Any &any)
    {
        auto *varstringarray = new DevVarStringArray();
        any >>= varstringarray;
        std::vector<std::string> values;
        for(std::size_t i = 0; i != varstringarray->length(); i++)
        {
            values.push_back(std::string((*varstringarray)[i]));
        }
        return values;
    }

    static void put_device_property(FileDatabase &db,
                                    const std::string &device_name,
                                    const std::string &property_name,
                                    const std::vector<std::string> &values)
    {
        std::vector<std::string> property{device_name, "1", property_name, std::to_string(values.size())};
        property.insert(property.end(), values.begin(), values.end());
        auto property_as_any = as_any(property);
        db.DbPutDeviceProperty(property_as_any);
    }

    static std::vector<std::string>
        get_device_property(FileDatabase &db, const std::string &device_name, const std::string &property_name)
    {
        std::vector<std::string> property_query_data{device_name, property_name};
        auto property_query_as_any = as_any(property_query_data);
        auto *property_as_any = db.DbGetDeviceProperty(property_query_as_any);
        auto property = from_any(*property_as_any);
        TS_ASSERT(property.size() > 4);
        TS_ASSERT_EQUALS(device_name, property[0]);
        TS_ASSERT_EQUALS("1", property[1]);
        TS_ASSERT_EQUALS(property_name, property[2]);
        auto size = std::stoul(property[3].c_str());
        std::vector<std::string> values{std::next(property.begin(), 4), property.end()};
        TS_ASSERT_EQUALS(size, values.size());
        return values;
    }

    void assert_device_property(FileDatabase &db,
                                const std::string &device_name,
                                const std::string &property_name,
                                const std::vector<std::string> &property_value)
    {
        auto property_value_from_db = get_device_property(db, device_name, property_name);
        TS_ASSERT_EQUALS(property_value, property_value_from_db);
    }

    void _test_string_property_roundtrip(const std::vector<std::string> &property_value)
    {
        std::string db_filename{"test.db"};
        std::string device_name{"test/device/01"};
        std::string property_name{"property"};
        {
            create_dbfile(db_filename, device_name);
            FileDatabase db(db_filename);
            put_device_property(db, device_name, property_name, property_value);
        }
        {
            FileDatabase db(db_filename);
            assert_device_property(db, device_name, property_name, property_value);
        }
        remove(db_filename.c_str());
    }

  public:
    //
    // the example file shown in the public online documentation
    //
    void test_example()
    {
        auto example_property_file = CxxTest::TangoPrinter::get_param("refpath") + "example_property_file.db";
        FileDatabase db(example_property_file);
        assert_device_property(db, "et/to/01", "StringProp", {"Property"});
        assert_device_property(db, "et/to/01", "ArrayProp", {"1", "2", "3"});
        assert_device_property(db, "et/to/01", "attr_min_poll_period", {"TheAttr", "1000"});
        assert_device_property(db, "et/to/01", "AnotherStringProp", {"A long string"});
        assert_device_property(db, "et/to/01", "ArrayStringProp", {"the first prop", "the second prop"});
    }

    //
    // a file with only a device declaration
    //
    void test_device_only()
    {
        std::string valid_db_filename{"valid.db"};
        std::string device_name{"test/device/01"};
        create_dbfile(valid_db_filename, device_name);
        FileDatabase valid_db(valid_db_filename);

        auto device_list_query = as_any({"DeviceServer/instance", "Class"});
        std::vector<std::string> expected_device_list{device_name};
        auto *device_list_any = valid_db.DbGetDeviceList(device_list_query);
        auto device_list = from_any(*device_list_any);
        TS_ASSERT_EQUALS(expected_device_list, device_list);
        remove(valid_db_filename.c_str());
    }

    //
    // a file with a device and a string property whose value has quotes and/or spaces
    //
    void test_string_property_value_with_quotes()
    {
        for(const auto *const value : {"hi", "hi ", "\"hi\"", "\"hi \""})
        {
            _test_string_property_roundtrip({value});
        }
    }

    //
    // A file with a device and a string property whose values has newlines
    //
    void test_string_property_value_with_newlines()
    {
        for(const auto newline_at_beginning : {true, false})
        {
            for(const auto newline_at_end : {true, false})
            {
                for(const auto in_quotes : {true, false})
                {
                    std::string value = "hi";
                    if(newline_at_beginning)
                    {
                        value = '\n' + value;
                    }
                    if(newline_at_end)
                    {
                        value = value + '\n';
                    }
                    if(in_quotes)
                    {
                        value = '"' + value + '"';
                    }
                    _test_string_property_roundtrip({value});
                }
            }
        }
    }
};

// NOLINTEND(*)
