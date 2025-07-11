// NOLINTBEGIN(*)

#include "old_common.h"

/*
 * Small utility program to help testing locking features.
 *
 * Possible return code:
 *  -1 : major error
 *   0 : success
 *   1 : Exception API_DeviceLocked
 *   2 : All other exceptions
 *   3 : State or Status command failed
 */

int main(int argc, char **argv)
{
    std::unique_ptr<DeviceProxy> device;

    if((argc == 1) || (argc > 3))
    {
        TEST_LOG << "usage: %s device" << endl;
        exit(-1);
    }

    string device_name = argv[1];

    try
    {
        device = std::make_unique<DeviceProxy>(device_name);
    }
    catch(CORBA::Exception &)
    {
        //        Except::print_exception(e);
        exit(-1);
    }

    // Try state or status on a locked device

    try
    {
        device->command_inout("State");
        device->command_inout("Status");
    }
    catch(Tango::DevFailed &)
    {
        return 3;
    }

    // Try a command on the device

    DeviceData din, dout;
    din << (short) 2;

    try
    {
        dout = device->command_inout("IOShort", din);
    }
    catch(Tango::DevFailed &e)
    {
        //        Except::print_exception(e);
        if(::strcmp(e.errors[0].reason.in(), API_DeviceLocked) != 0)
        {
            return 2;
        }
    }

    //  Try a command asynchronously

    long id;
    id = device->command_inout_asynch("IOShort", din);

    bool finish = false;
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
            TEST_LOG << "Command not yet arrived" << endl;
        }
        catch(DevFailed &e)
        {
            //            Except::print_exception(e);
            if(::strcmp(e.errors[0].reason.in(), API_DeviceLocked) != 0)
            {
                return 2;
            }
            else
            {
                finish = true;
            }
        }

        if(finish == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    //    Try a write attribute

    try
    {
        DeviceAttribute da("Long64_attr_rw", (DevLong64) 10);
        device->write_attribute(da);
    }
    catch(Tango::DevFailed &e)
    {
        //        Except::print_exception(e);
        if(::strcmp(e.errors[0].reason.in(), API_DeviceLocked) != 0)
        {
            return 2;
        }
    }

    //    Try a write attribute asynchronously

    DeviceAttribute da("Long64_attr_rw", (DevLong64) 10);
    id = device->write_attribute_asynch(da);

    finish = false;
    while(finish == false)
    {
        try
        {
            device->write_attribute_reply(id);
            finish = true;
        }
        catch(AsynReplyNotArrived &)
        {
            finish = false;
            TEST_LOG << "Attribute not yet written" << endl;
        }
        catch(DevFailed &e)
        {
            //            Except::print_exception(e);
            if(::strcmp(e.errors[0].reason.in(), API_DeviceLocked) != 0)
            {
                return 2;
            }
            else
            {
                finish = true;
            }
        }

        if(finish == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // Try a attribute set_config

    try
    {
        string att_name("Long64_attr_rw");
        AttributeInfoEx ai = device->get_attribute_config(att_name);
        AttributeInfoListEx ail;
        ail.push_back(ai);
        device->set_attribute_config(ail);
    }
    catch(Tango::DevFailed &e)
    {
        //        Except::print_exception(e);
        if(::strcmp(e.errors[0].reason.in(), API_DeviceLocked) != 0)
        {
            return 2;
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

// NOLINTEND(*)
