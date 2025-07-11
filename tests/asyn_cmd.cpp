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

    try
    {
        // Send a command to check polling without blocking

        DeviceData din, dout;
        long id;
        std::vector<short> send;
        send.push_back(4);
        send.push_back(2);
        din << send;

        id = device->command_inout_asynch("IOShortSleep", din);

        // Check if command returned

        bool finish = false;
        long nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                dout = device->command_inout_reply(id);
                short l;
                dout >> l;
                assert(l == 8);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                TEST_LOG << "Command not yet arrived" << std::endl;
                nb_not_arrived++;
            }
            if(finish == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Aynchronous command_inout in polling mode --> OK" << std::endl;

        // Send a command to check polling with blocking with timeout

        id = device->command_inout_asynch("IOShortSleep", din);
        //        assert( id == 2);

        // Check if command returned

        finish = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                dout = device->command_inout_reply(id, 200);
                short l;
                dout >> l;
                assert(l == 8);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Command not yet arrived" << std::endl;
                nb_not_arrived++;
            }
        }
        assert(nb_not_arrived >= 4);

        TEST_LOG << "   Aynchronous command_inout in blocking mode with call timeout --> OK" << std::endl;

        // Send a command to check polling with blocking

        id = device->command_inout_asynch("IOShortSleep", din);
        //        assert( id == 3);

        // Check if command returned

        dout = device->command_inout_reply(id, 0);
        short l;
        dout >> l;

        assert(l == 8);

        TEST_LOG << "   Aynchronous command_inout in blocking mode --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test Timeout exception and asynchronous calls
        //
        //---------------------------------------------------------------------------

        // Change timeout in order to test asynchronous calls and timeout

        /*        device->set_timeout_millis(2000);*/

        // Send a new command

        std::vector<short> in;
        in.push_back(2);
        in.push_back(6);
        din << in;
        id = device->command_inout_asynch("IOShortSleep", din);

        // Check if command returned

        finish = false;
        bool to = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                dout = device->command_inout_reply(id);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                nb_not_arrived++;
                TEST_LOG << "Command not yet arrived" << std::endl;
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

        // Send a command to check timeout with polling and blocking with timeout

        id = device->command_inout_asynch("IOShortSleep", din);

        // Check if command returned

        finish = false;
        to = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                dout = device->command_inout_reply(id, 500);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Command not yet arrived" << std::endl;
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

        // Send a command to check polling with blocking

        id = device->command_inout_asynch("IOShortSleep", din);

        // Check if command returned

        to = false;

        try
        {
            dout = device->command_inout_reply(id, 0);
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

        TEST_LOG << "   Device timeout with blocking command_inout_reply --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test DevFailed exception sent by server
        //
        //---------------------------------------------------------------------------

        TEST_LOG << "   Waiting for server to execute all previous requests" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(4));

        // Change timeout in order to test asynchronous calls and DevFailed exception

        //        device->set_timeout_millis(5000);

        // Send a new command

        short in_e = 2;
        din << in_e;
        id = device->command_inout_asynch("IOSleepExcept", din);

        // Check if command returned

        finish = false;
        bool failed = false;
        nb_not_arrived = 0;
        while(finish == false)
        {
            try
            {
                dout = device->command_inout_reply(id);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                finish = false;
                nb_not_arrived++;
                TEST_LOG << "Command not yet arrived" << std::endl;
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
        assert(failed == true);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Device exception with non blocking command_inout_reply --> OK" << std::endl;

        // Send a command to check timeout with polling and blocking with timeout

        id = device->command_inout_asynch("IOSleepExcept", din);

        // Check if command returned

        finish = false;
        failed = false;
        while(finish == false)
        {
            try
            {
                dout = device->command_inout_reply(id, 500);
                finish = true;
            }
            catch(AsynReplyNotArrived &)
            {
                TEST_LOG << "Command not yet arrived" << std::endl;
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
        assert(failed == true);

        TEST_LOG << "   Device exception with blocking command_inout_reply with call timeout --> OK" << std::endl;

        // Send a command to check polling with blocking

        id = device->command_inout_asynch("IOSleepExcept", din);

        // Check if command returned

        failed = false;
        try
        {
            dout = device->command_inout_reply(id, 0);
            finish = true;
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
        assert(failed == true);

        TEST_LOG << "   Device exception with blocking command_inout_reply --> OK" << std::endl;
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
