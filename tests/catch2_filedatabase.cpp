#include <tango/tango.h>

#include <memory>
#include <filesystem>

#include "utils/utils.h"

#include "common.h"

namespace
{

std::string create_dbfile(const std::string &device_name)
{
    auto filename = TangoTest::get_next_file_database_location();

    {
        std::ofstream dbfile(filename);
        dbfile << "DeviceServer/instance/DEVICE/Class: " << device_name << '\n';
    }

    return filename;
}

CORBA::Any as_any(std::vector<std::string> values)
{
    auto *varstringarray = new Tango::DevVarStringArray();
    varstringarray->length(values.size());
    for(std::size_t i = 0; i != values.size(); i++)
    {
        (*varstringarray)[i] = Tango::string_dup(values[i].c_str());
    }
    CORBA::Any _as_any;
    _as_any <<= varstringarray;
    return _as_any;
}

std::vector<std::string> from_any(const CORBA::Any_var &any)
{
    auto *varstringarray = new Tango::DevVarStringArray();
    any >>= varstringarray;
    std::vector<std::string> values;
    for(std::size_t i = 0; i != varstringarray->length(); i++)
    {
        values.push_back(std::string((*varstringarray)[i]));
    }
    return values;
}

void put_device_property(Tango::FileDatabase &db,
                         const std::string &device_name,
                         const std::string &property_name,
                         const std::vector<std::string> &values)
{
    std::vector<std::string> property{device_name, "1", property_name, std::to_string(values.size())};
    property.insert(property.end(), values.begin(), values.end());
    auto property_as_any = as_any(property);
    db.DbPutDeviceProperty(property_as_any);
}

void delete_device_property(Tango::FileDatabase &db, const std::string &device_name, const std::string &property_name)
{
    std::vector<std::string> property{device_name, "1", property_name};
    auto property_as_any = as_any(property);
    db.DbDeleteDeviceProperty(property_as_any);
}

std::vector<std::string>
    get_device_property(Tango::FileDatabase &db, const std::string &device_name, const std::string &property_name)
{
    std::vector<std::string> property_query_data{device_name, property_name};
    auto property_query_as_any = as_any(property_query_data);
    auto property_as_any = db.DbGetDeviceProperty(property_query_as_any);
    auto property = from_any(property_as_any);
    REQUIRE(property.size() > 4);
    REQUIRE(device_name == property[0]);
    REQUIRE("1" == property[1]);
    REQUIRE(property_name == property[2]);

    auto size = parse_as<size_t>(property[3]);
    if(size == 0)
    {
        return {};
    }

    std::vector<std::string> values{std::next(property.begin(), 4), property.end()};
    REQUIRE(size == values.size());

    return values;
}

void assert_device_property(Tango::FileDatabase &db,
                            const std::string &device_name,
                            const std::string &property_name,
                            const std::vector<std::string> &property_value)
{
    auto property_value_from_db = get_device_property(db, device_name, property_name);
    REQUIRE(property_value == property_value_from_db);
}

void _test_string_property_roundtrip(const std::vector<std::string> &property_value)
{
    std::string device_name{"test/device/01"};
    std::string property_name{"property"};
    const auto db_filename = create_dbfile(device_name);

    {
        Tango::FileDatabase db(db_filename);
        put_device_property(db, device_name, property_name, property_value);
    }

    {
        Tango::FileDatabase db(db_filename);
        assert_device_property(db, device_name, property_name, property_value);
    }
}

void put_class_property(Tango::FileDatabase &db,
                        const std::string &class_name,
                        const std::string &property_name,
                        const std::vector<std::string> &values)
{
    std::vector<std::string> property{class_name, "1", property_name, std::to_string(values.size())};
    property.insert(property.end(), values.begin(), values.end());
    auto property_as_any = as_any(property);
    db.DbPutClassProperty(property_as_any);
}

void delete_class_property(Tango::FileDatabase &db, const std::string &class_name, const std::string &property_name)
{
    std::vector<std::string> property{class_name, "1", property_name};
    auto property_as_any = as_any(property);
    db.DbDeleteClassProperty(property_as_any);
}

std::vector<std::string>
    get_class_property(Tango::FileDatabase &db, const std::string &class_name, const std::string &property_name)
{
    std::vector<std::string> property_query_data{class_name, property_name};
    auto property_query_as_any = as_any(property_query_data);
    auto property_as_any = db.DbGetClassProperty(property_query_as_any);
    auto property = from_any(property_as_any);
    REQUIRE(property.size() > 3);
    REQUIRE(class_name == property[0]);
    REQUIRE("1" == property[1]);
    REQUIRE(property_name == property[2]);

    auto size = parse_as<size_t>(property[3]);
    if(size == 0)
    {
        return {};
    }

    std::vector<std::string> values{std::next(property.begin(), 4), property.end()};
    REQUIRE(size == values.size());

    return values;
}

void assert_class_property(Tango::FileDatabase &db,
                           const std::string &class_name,
                           const std::string &property_name,
                           const std::vector<std::string> &property_value)
{
    auto property_value_from_db = get_class_property(db, class_name, property_name);
    REQUIRE(property_value == property_value_from_db);
}

void put_device_attr_property(Tango::FileDatabase &db,
                              const std::string &device_name,
                              const std::string &attribute_name,
                              const std::string &property_name,
                              const std::vector<std::string> &values)
{
    std::vector<std::string> property{
        device_name, "1", attribute_name, "1", property_name, std::to_string(values.size())};
    property.insert(property.end(), values.begin(), values.end());
    auto property_as_any = as_any(property);
    db.DbPutDeviceAttributeProperty(property_as_any);
}

void delete_device_attr_property(Tango::FileDatabase &db,
                                 const std::string &device_name,
                                 const std::string &attribute_name,
                                 const std::string &property_name)
{
    std::vector<std::string> property{device_name, attribute_name, property_name};
    auto property_as_any = as_any(property);
    db.DbDeleteDeviceAttributeProperty(property_as_any);
}

std::vector<std::string> get_device_attr_property(Tango::FileDatabase &db,
                                                  const std::string &device_name,
                                                  const std::string &attribute_name,
                                                  const std::string &property_name)
{
    std::vector<std::string> property_query_data{device_name, attribute_name};
    auto property_query_as_any = as_any(property_query_data);
    auto property_as_any = db.DbGetDeviceAttributeProperty(property_query_as_any);
    auto list = from_any(property_as_any);

    CAPTURE(list);

    REQUIRE(list.size() > 3);

    REQUIRE(device_name == list[0]);

    REQUIRE("1" == list[1]);
    REQUIRE(attribute_name == list[2]);

    if(list[3] == "0")
    {
        return {};
    }

    REQUIRE("1" == list[3]);

    REQUIRE(list.size() > 6);

    REQUIRE(property_name == list[4]);

    auto size = parse_as<size_t>(list[5]);
    if(size == 0)
    {
        return {};
    }

    std::vector<std::string> values{std::next(list.begin(), 6), list.end()};
    REQUIRE(size == values.size());

    return values;
}

void assert_device_attr_property(Tango::FileDatabase &db,
                                 const std::string &device_name,
                                 const std::string &attribute_name,
                                 const std::string &property_name,
                                 const std::vector<std::string> &property_values)
{
    auto property_value_from_db = get_device_attr_property(db, device_name, attribute_name, property_name);
    REQUIRE(property_value_from_db == property_values);
}

void put_class_attr_property(Tango::FileDatabase &db,
                             const std::string &class_name,
                             const std::string &attribute_name,
                             const std::string &property_name,
                             const std::vector<std::string> &values)
{
    std::vector<std::string> property{
        class_name, "1", attribute_name, "1", property_name, std::to_string(values.size())};
    property.insert(property.end(), values.begin(), values.end());
    auto property_as_any = as_any(property);
    db.DbPutClassAttributeProperty(property_as_any);
}

std::vector<std::string> get_class_attr_property(Tango::FileDatabase &db,
                                                 const std::string &class_name,
                                                 const std::string &attribute_name,
                                                 const std::string &property_name)
{
    std::vector<std::string> property_query_data{class_name, attribute_name};
    auto property_query_as_any = as_any(property_query_data);
    auto property_as_any = db.DbGetClassAttributeProperty(property_query_as_any);
    auto list = from_any(property_as_any);

    CAPTURE(list);

    REQUIRE(list.size() > 3);

    REQUIRE(class_name == list[0]);

    REQUIRE("1" == list[1]);
    REQUIRE(attribute_name == list[2]);

    if(list[3] == "0")
    {
        return {};
    }

    REQUIRE("1" == list[3]);

    REQUIRE(list.size() > 6);

    REQUIRE(property_name == list[4]);

    auto size = parse_as<size_t>(list[5]);
    if(size == 0)
    {
        return {};
    }

    std::vector<std::string> values{std::next(list.begin(), 6), list.end()};
    REQUIRE(size == values.size());

    return values;
}

void assert_class_attr_property(Tango::FileDatabase &db,
                                const std::string &class_name,
                                const std::string &attribute_name,
                                const std::string &property_name,
                                const std::vector<std::string> &property_values)
{
    auto property_value_from_db = get_class_attr_property(db, class_name, attribute_name, property_name);
    REQUIRE(property_value_from_db == property_values);
}

std::string get_example_db()
{
    auto source = std::string(TANGO_TEST_CATCH2_RESOURCE_PATH) + "/example_property_file.db";
    auto target = TangoTest::get_next_file_database_location();

    namespace fs = std::filesystem;
    REQUIRE_NOTHROW(fs::copy_file(source, target, fs::copy_options::overwrite_existing));

    return target;
}

} // namespace

SCENARIO("file from documentation can be parsed")
{
    GIVEN("a filedatabase from the documentation")
    {
        auto db = Tango::FileDatabase(get_example_db());
        WHEN("the expected device properties exist")
        {
            assert_device_property(db, "et/to/01", "StringProp", {"Property"});
            assert_device_property(db, "et/to/01", "ArrayProp", {"1", "2", "3"});
            assert_device_property(db, "et/to/01", "attr_min_poll_period", {"TheAttr", "1000"});
            assert_device_property(db, "et/to/01", "AnotherStringProp", {"A long string"});
            assert_device_property(db, "et/to/01", "ArrayStringProp", {"the first prop", "the second prop"});
        }
    }
}

SCENARIO("a file with only a device declaration")
{
    std::string device_name{"test/device/01"};
    const auto db_filename = create_dbfile(device_name);

    GIVEN("a very simple file")
    {
        Tango::FileDatabase valid_db(db_filename);

        WHEN("we can gather the list of devices")
        {
            auto device_list_query = as_any({"DeviceServer/instance", "Class"});
            std::vector<std::string> expected_device_list{device_name};
            auto device_list_any = valid_db.DbGetDeviceList(device_list_query);
            auto device_list = from_any(device_list_any);
            REQUIRE(expected_device_list == device_list);
        }
    }
}

SCENARIO("string properties with quotes and/or spaces")
{
    auto data = GENERATE("hi", "hi ", "\"hi\"", "\"hi \"");

    GIVEN("the string " << data)
    {
        WHEN("check that this roundtrips")
        {
            _test_string_property_roundtrip({data});
        }
    }
}

SCENARIO("A file with a device and a string property whose values has newlines")
{
    auto newline_at_beginning = GENERATE(true, false);
    GIVEN("newline at beginning " << std::boolalpha << newline_at_beginning)
    {
        auto newline_at_end = GENERATE(true, false);
        AND_GIVEN("newline at end " << std::boolalpha << newline_at_beginning)
        {
            auto in_quotes = GENERATE(true, false);
            AND_GIVEN("and in quotes " << std::boolalpha << newline_at_beginning)
            {
                WHEN("this roundtrips")
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
}

SCENARIO("Check that unimplemented calls throw")
{
    GIVEN("a filedatabase from the documentation")
    {
        auto db = Tango::FileDatabase(get_example_db());

        CORBA::Any any;
        std::vector<std::function<void(void)>> funcs = {
            std::bind(&Tango::FileDatabase::DbDeleteClassAttributeProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbImportDevice, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbExportDevice, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbUnExportDevice, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbAddDevice, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbDeleteDevice, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbAddServer, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbDeleteServer, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbExportServer, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbPutProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbDeleteProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbGetAliasDevice, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbGetDeviceAlias, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbGetAttributeAlias, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbGetDeviceAliasList, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbGetAttributeAliasList, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbGetClassPipeProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbGetDevicePipeProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbDeleteClassPipeProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbDeleteDevicePipeProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbPutClassPipeProperty, &db, std::ref(any)),
            std::bind(&Tango::FileDatabase::DbPutDevicePipeProperty, &db, std::ref(any))};

        // using GENERATE_REF directly does not work
        auto func = GENERATE_REF(Catch::Generators::from_range(funcs));

        AND_GIVEN("an unsupported function")
        {
            WHEN("we always throw")
            {
                REQUIRE_THROWS_MATCHES(
                    func(), Tango::DevFailed, TangoTest::DevFailedReasonEquals(Tango::API_NotSupported));
            }
        }
    }
}

SCENARIO("Check that DbPutDeviceProperty")
{
    GIVEN("does nothing")
    {
        CORBA::Any any;
        auto db = Tango::FileDatabase(get_example_db());

        std::string device_name{"unknownDevice"};
        std::string property_name{"someProp"};
        std::vector<std::string> property_value{"someValue"};

        WHEN("given a non matching device")
        {
            put_device_property(db, device_name, property_name, property_value);
            auto results = get_device_property(db, device_name, property_name);
            REQUIRE(results.empty());
        }
    }
}

SCENARIO("Check that DbDeleteDeviceProperty")
{
    GIVEN("works as expected")
    {
        std::string device_name{"test/device/01"};
        std::string property_name{"property"};
        std::vector<std::string> property_value{"someValue"};

        const auto db_filename = create_dbfile(device_name);
        Tango::FileDatabase db(db_filename);

        WHEN("adding and deleting a property")
        {
            put_device_property(db, device_name, property_name, property_value);
            delete_device_property(db, device_name, property_name);
            auto results = get_device_property(db, device_name, property_name);
            REQUIRE(results.empty());
        }
    }
}

SCENARIO("Check that DbXXXClassProperty")
{
    GIVEN("works as expected")
    {
        std::string device_name{"test/device/01"};
        std::string property_name{"property"};
        std::vector<std::string> property_value{"someValue"};

        const auto db_filename = create_dbfile(device_name);

        WHEN("adding and deleting a property")
        {
            std::string class_name{"class"};

            {
                Tango::FileDatabase db(db_filename);
                put_class_property(db, class_name, property_name, property_value);
            }

            {
                Tango::FileDatabase db(db_filename);
                assert_class_property(db, class_name, property_name, property_value);
            }

            {
                Tango::FileDatabase db(db_filename);
                delete_class_property(db, class_name, property_name);
                auto results = get_class_property(db, class_name, property_name);
                REQUIRE(results.empty());
            }
        }
    }
}

SCENARIO("Check that DbXXXDeviceAttributeProperty")
{
    GIVEN("works as expected")
    {
        std::string device_name{"test/device/01"};
        std::string attribute_name{"someAttr"};
        std::string property_name{"property"};
        std::vector<std::string> property_value{"someValue"};

        const auto db_filename = create_dbfile(device_name);

        WHEN("adding and deleting a property")
        {
            {
                Tango::FileDatabase db(db_filename);
                put_device_attr_property(db, device_name, attribute_name, property_name, property_value);
            }

            {
                Tango::FileDatabase db(db_filename);
                assert_device_attr_property(db, device_name, attribute_name, property_name, property_value);
            }

            {
                Tango::FileDatabase db(db_filename);
                delete_device_attr_property(db, device_name, attribute_name, property_name);
                auto results = get_device_attr_property(db, device_name, attribute_name, property_name);
                REQUIRE(results.empty());
            }
        }
    }
}

SCENARIO("Check that DbXXXClassAttributeProperty")
{
    GIVEN("works as expected")
    {
        std::string device_name{"test/device/01"};
        std::string attribute_name{"someAttr"};
        std::string property_name{"property"};
        std::vector<std::string> property_value{"someValue"};

        const auto db_filename = create_dbfile(device_name);

        WHEN("adding and deleting a property")
        {
            std::string class_name{"class"};

            {
                Tango::FileDatabase db(db_filename);
                put_class_attr_property(db, class_name, attribute_name, property_name, property_value);
            }

            {
                Tango::FileDatabase db(db_filename);
                assert_class_attr_property(db, class_name, attribute_name, property_name, property_value);
            }

            // delete_class_attr_property can not be implemented as DbDeleteClassAttributeProperty is not implemented
        }
    }
}
