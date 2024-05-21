// NOLINTBEGIN(*)

#include "old_common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if(argc == 1)
    {
        TEST_LOG << "usage: %s device" << std::endl;
        exit(-1);
    }

    std::string device_name = argv[1];

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << std::endl << "new DeviceProxy(" << device->name() << ") returned" << std::endl << std::endl;

    try
    {
        // Read one attribute

        long id;
        DeviceAttribute *received = nullptr;

        id = device->read_attribute_asynch("attr_asyn");

        // Check if attribute returned

        bool finish = false;
        long nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                received = device->read_attribute_reply(id);
                double db;
                *received >> db;
                assert(db == 5.55);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                TEST_LOG << "Attribute not yet read" << std::endl;
                nb_not_arrived++;
            }
            if(finish == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        delete received;

        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Asynchronous read_attribute in polling mode --> OK" << std::endl;

        // Read one attribute of the DevEncoded data type
        // The attribute used to test DevEncoded does not have any
        // "sleep" in its code -> Do not check the nb_not_arrived data

#ifndef COMPAT
        id = device->read_attribute_asynch("encoded_attr");

        // Check if attribute returned

        finish = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                received = device->read_attribute_reply(id);
                TEST_LOG << "Attribute result arrived" << std::endl;
                Tango::DevEncoded enc_data;
                *received >> enc_data;
                assert(::strcmp(enc_data.encoded_format, "Which format?") == 0);
                assert(enc_data.encoded_data.length() == 4);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                TEST_LOG << "Attribute not yet read" << std::endl;
                nb_not_arrived++;
            }
            if(finish == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        delete received;

        TEST_LOG << "   Asynchronous read_attribute (DevEncoded data type) in polling mode --> OK" << std::endl;
#endif

        // Read attribute to check polling with blocking with timeout

        id = device->read_attribute_asynch("attr_asyn");
        //        assert( id == 2);

        // Check if attribute returned

        finish = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                received = device->read_attribute_reply(id, 200);
                double l;
                *received >> l;
                assert(l == 5.55);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Attribute not yet read" << std::endl;
                nb_not_arrived++;
            }
        }
        delete received;

        assert(nb_not_arrived >= 4);

        TEST_LOG << "   Asynchronous read_attribute in blocking mode with call timeout --> OK" << std::endl;

        // Send a command to check polling with blocking

        id = device->read_attribute_asynch("attr_asyn");
        //        assert( id == 3);

        // Check if command returned

        received = device->read_attribute_reply(id, 0);
        double l;
        *received >> l;
        delete received;

        assert(l == 5.55);

        TEST_LOG << "   Asynchronous read_attribute in blocking mode --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test Timeout exception and asynchronous calls
        //
        //---------------------------------------------------------------------------

        // Change timeout in order to test asynchronous calls and timeout

        //        device->set_timeout_millis(2000);

        // Read an attribute

        id = device->read_attribute_asynch("attr_asyn_to");

        // Check if attribute returned

        finish = false;
        bool to = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                received = device->read_attribute_reply(id);
                finish = true;
                delete received;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                nb_not_arrived++;
                TEST_LOG << "Attribute not yet read" << std::endl;
            }
            catch(CommunicationFailed &e)
            {
                finish = true;
                if(strcmp(e.errors[1].reason, API_DeviceTimedOut) == 0)
                {
                    to = true;
                    TEST_LOG << "Timeout exception" << std::endl;
                }
                else
                {
                    TEST_LOG << "Comm exception" << std::endl;
                }
            }
            if(finish == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        assert(to == true);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Device timeout exception with non blocking command_inout_reply --> OK" << std::endl;

        // Read an attribute to check timeout with polling and blocking with timeout

        id = device->read_attribute_asynch("attr_asyn_to");

        // Check if command returned

        finish = false;
        to = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                received = device->read_attribute_reply(id, 500);
                finish = true;
                delete received;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Attribute not yet read" << std::endl;
                nb_not_arrived++;
            }
            catch(CommunicationFailed &e)
            {
                finish = true;
                if(strcmp(e.errors[1].reason, API_DeviceTimedOut) == 0)
                {
                    to = true;
                    TEST_LOG << "Timeout exception" << std::endl;
                }
                else
                {
                    TEST_LOG << "Comm exception" << std::endl;
                }
            }
        }
        assert(to == true);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Device timeout with blocking command_inout_reply with call timeout --> OK" << std::endl;

        // Read an attribute to check polling with blocking

        id = device->read_attribute_asynch("attr_asyn_to");

        // Check if attribute returned

        to = false;
        try
        {
            received = device->read_attribute_reply(id, 0);
            finish = true;
            delete received;
        }
        catch(CommunicationFailed &e)
        {
            if(strcmp(e.errors[1].reason, API_DeviceTimedOut) == 0)
            {
                to = true;
                TEST_LOG << "Timeout exception" << std::endl;
            }
            else
            {
                TEST_LOG << "Comm exception" << std::endl;
            }
        }
        assert(to == true);

        TEST_LOG << "   Device timeout with blocking command_inout_reply --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test DevFailed exception sent by server
        //
        //---------------------------------------------------------------------------

        TEST_LOG << "   Waiting for server to execute all previous requests" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Change timeout in order to test asynchronous calls and DevFailed exception

        //        device->set_timeout_millis(5000);

        // Read attribute

        id = device->read_attribute_asynch("attr_asyn_except");

        // Check if attribute returned

        finish = false;
        bool failed = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                received = device->read_attribute_reply(id);
                finish = true;
                double db;
                (*received) >> db;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                nb_not_arrived++;
                TEST_LOG << "Attribute not yet read" << std::endl;
            }
            catch(DevFailed &e)
            {
                finish = true;
                if(strcmp(e.errors[0].reason, "aaa") == 0)
                {
                    failed = true;
                    TEST_LOG << "Server exception" << std::endl;
                }
                else
                {
                    TEST_LOG << "Comm exception" << std::endl;
                }
            }
            if(finish == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        delete received;

        assert(failed == true);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Device exception with non blocking read_attribute_reply --> OK" << std::endl;

        // Read an attribute to check timeout with polling and blocking with timeout

        id = device->read_attribute_asynch("attr_asyn_except");

        // Check if attribute returned

        finish = false;
        failed = false;
        while(finish == false)
        {
            try
            {
                received = device->read_attribute_reply(id, 500);
                finish = true;
                double db;
                (*received) >> db;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Attribute not yet read" << std::endl;
            }
            catch(DevFailed &e)
            {
                finish = true;
                if(strcmp(e.errors[0].reason, "aaa") == 0)
                {
                    failed = true;
                    TEST_LOG << "Server exception" << std::endl;
                }
                else
                {
                    TEST_LOG << "Comm exception" << std::endl;
                }
            }
        }

        delete received;
        assert(failed == true);

        TEST_LOG << "   Device exception with blocking read_attribute_reply with call timeout --> OK" << std::endl;

        // Read an attribute to check polling with blocking

        id = device->read_attribute_asynch("attr_asyn_except");

        // Check if attribute returned

        failed = false;
        try
        {
            received = device->read_attribute_reply(id, 0);
            finish = true;
            double db;
            (*received) >> db;
        }
        catch(DevFailed &e)
        {
            if(strcmp(e.errors[0].reason, "aaa") == 0)
            {
                failed = true;
                TEST_LOG << "Server exception" << std::endl;
            }
            else
            {
                TEST_LOG << "Comm exception" << std::endl;
            }
        }

        delete received;
        assert(failed == true);

        TEST_LOG << "   Device exception with blocking read_attribute_reply --> OK" << std::endl;
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }
    catch(CORBA::Exception &ex)
    {
        Except::print_exception(ex);
        exit(-1);
    }

    delete device;

    return 0;
}

// NOLINTEND(*)
