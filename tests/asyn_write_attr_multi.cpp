// NOLINTBEGIN(*)

#include "common.h"

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
        // Change timeout (with a useless name as a workaround for ORBacus bug)

        //        device->set_timeout_millis(6000);

        // Write one attribute

        long id;

        std::vector<DeviceAttribute> send(2);

        send[0].set_name("attr_asyn_write");
        DevLong lg = 222;
        send[0] << lg;

        send[1].set_name("Double_attr_w");
        double db = 4.55;
        send[1] << db;

        id = device->write_attributes_asynch(send);

        // Check if attribute returned

        bool finish = false;
        long nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                device->write_attributes_reply(id);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                TEST_LOG << "Attributes not yet written" << std::endl;
                nb_not_arrived++;
            }
            if(finish == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Asynchronous write_attributes in polling mode --> OK" << std::endl;

        // Write attribute to check polling with blocking with timeout

        id = device->write_attributes_asynch(send);
        //        assert( id == 2);

        // Check if attribute returned

        finish = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                device->write_attributes_reply(id, 200);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Attributes not yet written" << std::endl;
                nb_not_arrived++;
            }
        }

        assert(nb_not_arrived >= 4);

        TEST_LOG << "   Asynchronous write_attributes in blocking mode with call timeout --> OK" << std::endl;

        // Write an attribute to check polling with blocking

        id = device->write_attributes_asynch(send);
        //        assert( id == 3);

        // Check if attribute returned

        device->write_attributes_reply(id, 0);

        TEST_LOG << "   Asynchronous write_attributes in blocking mode --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test Timeout exception and asynchronous calls
        //
        //---------------------------------------------------------------------------

        // Change timeout in order to test asynchronous calls and timeout

        //        device->set_timeout_millis(2000);

        // Write an attribute

        send[0].set_name("attr_asyn_write_to");
        id = device->write_attributes_asynch(send);

        // Check if attribute returned

        finish = false;
        bool to = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                device->write_attributes_reply(id);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                nb_not_arrived++;
                TEST_LOG << "Attributes not yet written" << std::endl;
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

        TEST_LOG << "   Device timeout exception with non blocking write_attributes_reply --> OK" << std::endl;

        // Write an attribute to check timeout with polling and blocking with timeout

        id = device->write_attributes_asynch(send);

        // Check if attribute returned

        finish = false;
        to = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                device->write_attributes_reply(id, 200);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Attributes not yet written" << std::endl;
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

        TEST_LOG << "   Device timeout with blocking write_attributes_reply with call timeout --> OK" << std::endl;

        // Write an attribute to check polling with blocking

        id = device->write_attributes_asynch(send);

        // Check if attribute returned

        to = false;
        try
        {
            device->write_attributes_reply(id, 0);
            finish = true;
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

        TEST_LOG << "   Device timeout with blocking write_attributes_reply --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test DevFailed exception sent by server
        //
        //---------------------------------------------------------------------------

        TEST_LOG << "   Waiting for server to execute all previous requests" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Change timeout in order to test asynchronous calls and DevFailed exception

        //        device->set_timeout_millis(5000);

        // Write attribute

        send[0].set_name("attr_asyn_write_except");

        id = device->write_attributes_asynch(send);

        // Check if attribute returned

        finish = false;
        bool failed = false;
        nb_not_arrived = 0;
        long nb_except = 0;
        long faulty_idx = 0;
        while(finish == false)
        {
            try
            {
                device->write_attributes_reply(id);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                nb_not_arrived++;
                TEST_LOG << "Attribute not yet written" << std::endl;
            }
            catch(NamedDevFailedList &e)
            {
                finish = true;
                nb_except = e.err_list.size();
                faulty_idx = e.err_list[0].idx_in_call;
                if(strcmp(e.err_list[0].err_stack[0].reason, "aaa") == 0)
                {
                    failed = true;
                    TEST_LOG << "Server exception" << std::endl;
                }
            }
            catch(DevFailed &)
            {
                TEST_LOG << "Comm exception" << std::endl;
            }
            if(finish == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        assert(failed == true);
        assert(nb_except == 1);
        assert(faulty_idx == 0);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Device exception with non blocking write_attributes_reply --> OK" << std::endl;

        // Write an attribute to check timeout with polling and blocking with timeout

        id = device->write_attributes_asynch(send);

        // Check if attribute returned

        finish = false;
        failed = false;
        nb_except = 10;
        faulty_idx = 10;

        while(finish == false)
        {
            try
            {
                device->write_attributes_reply(id, 500);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Attribute not yet written" << std::endl;
            }
            catch(NamedDevFailedList &e)
            {
                finish = true;
                nb_except = e.err_list.size();
                faulty_idx = e.err_list[0].idx_in_call;
                if(strcmp(e.err_list[0].err_stack[0].reason, "aaa") == 0)
                {
                    failed = true;
                    TEST_LOG << "Server exception" << std::endl;
                }
            }
            catch(DevFailed &)
            {
                TEST_LOG << "Comm exception" << std::endl;
            }
        }
        assert(failed == true);
        assert(nb_except == 1);
        assert(faulty_idx == 0);

        TEST_LOG << "   Device exception with blocking write_attributes_reply with call timeout --> OK" << std::endl;

        // Write an attribute to check polling with blocking

        id = device->write_attributes_asynch(send);

        // Check if attribute returned

        failed = false;
        nb_except = 10;
        faulty_idx = 10;
        try
        {
            device->write_attributes_reply(id, 0);
            finish = true;
        }
        catch(NamedDevFailedList &e)
        {
            finish = true;
            nb_except = e.err_list.size();
            faulty_idx = e.err_list[0].idx_in_call;
            if(strcmp(e.err_list[0].err_stack[0].reason, "aaa") == 0)
            {
                failed = true;
                TEST_LOG << "Server exception" << std::endl;
            }
        }
        catch(DevFailed &)
        {
            TEST_LOG << "Comm exception" << std::endl;
        }

        assert(failed == true);
        assert(nb_except == 1);
        assert(faulty_idx == 0);

        TEST_LOG << "   Device exception with blocking write_attributes_reply --> OK" << std::endl;
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
