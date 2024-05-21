// NOLINTBEGIN(*)

#include "old_common.h"

class MyCallBack : public CallBack
{
  public:
    MyCallBack() :
        cb_executed(0),
        to(false),
        cmd_failed(false),
        attr_failed(false)
    {
    }

    virtual void cmd_ended(CmdDoneEvent *);
    virtual void attr_read(AttrReadEvent *);
    virtual void attr_written(AttrWrittenEvent *);

    long cb_executed;
    short l;
    double db;
    short sh;
    bool to;
    bool cmd_failed;
    bool attr_failed;
    long nb_attr;
};

void MyCallBack::cmd_ended(CmdDoneEvent *cmd)
{
    TEST_LOG << "In cmd_ended method for device " << cmd->device->dev_name() << std::endl;
    TEST_LOG << "Command = " << cmd->cmd_name << std::endl;

    if(cmd->errors.length() == 0)
    {
        cmd->argout >> l;
        TEST_LOG << "Command result = " << l << std::endl;
    }
    else
    {
        long nb_err = cmd->errors.length();
        TEST_LOG << "Command returns error" << std::endl;
        TEST_LOG << "error length = " << nb_err << std::endl;
        for(int i = 0; i < nb_err; i++)
        {
            TEST_LOG << "error[" << i << "].reason = " << cmd->errors[i].reason.in() << std::endl;
        }
        if(strcmp(cmd->errors[nb_err - 1].reason, API_DeviceTimedOut) == 0)
        {
            to = true;
            TEST_LOG << "Timeout error" << std::endl;
        }
        else if(strcmp(cmd->errors[nb_err - 1].reason, API_CommandFailed) == 0)
        {
            cmd_failed = true;
            TEST_LOG << "Command failed error" << std::endl;
        }
        else
        {
            TEST_LOG << "Unknown error" << std::endl;
        }
    }

    cb_executed++;
}

void MyCallBack::attr_read(AttrReadEvent *att)
{
    TEST_LOG << "In attr_read method for device " << att->device->dev_name() << std::endl;
    for(unsigned int i = 0; i < att->attr_names.size(); i++)
    {
        TEST_LOG << "Attribute read = " << att->attr_names[i] << std::endl;
    }

    nb_attr = att->attr_names.size();

    if(att->err == false)
    {
        (*(att->argout))[0] >> db;
        if(nb_attr == 2)
        {
            (*(att->argout))[1] >> sh;
        }
    }
    else
    {
        long nb_err = att->errors.length();
        TEST_LOG << "read_attributes returns error" << std::endl;
        TEST_LOG << "error length = " << nb_err << std::endl;
        for(int i = 0; i < nb_err; i++)
        {
            TEST_LOG << "error[" << i << "].reason = " << att->errors[i].reason.in() << std::endl;
        }
        if(strcmp(att->errors[nb_err - 1].reason, API_DeviceTimedOut) == 0)
        {
            to = true;
            TEST_LOG << "Timeout error" << std::endl;
        }
        else if(strcmp(att->errors[nb_err - 1].reason, API_AttributeFailed) == 0)
        {
            attr_failed = true;
            TEST_LOG << "Read attributes failed error" << std::endl;
        }
        else
        {
            TEST_LOG << "Unknown error" << std::endl;
        }
    }

    cb_executed++;
    delete att->argout;
}

void MyCallBack::attr_written(AttrWrittenEvent *att)
{
    TEST_LOG << "In attr_written method for device " << att->device->dev_name() << std::endl;
    for(unsigned int i = 0; i < att->attr_names.size(); i++)
    {
        TEST_LOG << "Attribute written = " << att->attr_names[i] << std::endl;
    }

    nb_attr = att->attr_names.size();

    if(att->err == true)
    {
        long nb_err = att->errors.get_faulty_attr_nb();
        TEST_LOG << "write_attributes returns error" << std::endl;
        TEST_LOG << "error length = " << nb_err << std::endl;
        for(long i = 0; i < nb_err; i++)
        {
            TEST_LOG << "error[" << i << "].reason = " << att->errors.err_list[i].err_stack[0].reason.in() << std::endl;
        }
        if(strcmp(att->errors.err_list[nb_err - 1].err_stack[0].reason.in(), API_DeviceTimedOut) == 0)
        {
            to = true;
            TEST_LOG << "Timeout error" << std::endl;
        }
        else
        {
            TEST_LOG << "Unknown error" << std::endl;
        }
    }

    cb_executed++;
}

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
        // Set the automatic callback mode

        ApiUtil::instance()->set_asynch_cb_sub_model(PUSH_CALLBACK);

        // Send a command to check asynchronous callback

        long nb_not_arrived = 0;

        DeviceData din, dout;
        std::vector<short> send;
        send.push_back(4);
        send.push_back(2);
        din << send;

        MyCallBack cb;

        device->command_inout_asynch("IOShortSleep", din, cb);

        // Check if command returned

        while(cb.cb_executed == 0)
        {
            TEST_LOG << "Command not yet arrived" << std::endl;
            nb_not_arrived++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        assert(nb_not_arrived >= 2);
        assert(cb.cb_executed == 1);
        assert(cb.l == 8);

        TEST_LOG << "   Aynchronous command_inout in callback mode --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test Timeout exception and asynchronous calls
        //
        //---------------------------------------------------------------------------

        // Send a new command

        std::vector<short> in;
        in.push_back(2);
        in.push_back(6);
        din << in;
        cb.l = 0;
        cb.cb_executed = 0;

        device->command_inout_asynch("IOShortSleep", din, cb);

        // Check if command returned

        nb_not_arrived = 0;
        while(cb.cb_executed == 0)
        {
            nb_not_arrived++;
            TEST_LOG << "Command not yet arrived" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        assert(cb.to == true);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Device timeout exception with non blocking get_asynch_replies --> OK" << std::endl;

        //---------------------------------------------------------------------------
        //
        //            Now test DevFailed exception sent by server
        //
        //---------------------------------------------------------------------------

        TEST_LOG << "   Waiting for server to execute all previous requests" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Send a new command

        short in_e = 2;
        din << in_e;
        cb.l = 0;
        cb.cb_executed = 0;
        cb.cmd_failed = false;

        device->command_inout_asynch("IOSleepExcept", din, cb);

        // Check if command returned

        nb_not_arrived = 0;
        while(cb.cb_executed == 0)
        {
            nb_not_arrived++;
            TEST_LOG << "Command not yet arrived" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        assert(cb.cmd_failed == true);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Device exception with non blocking get_asynch_replies --> OK" << std::endl;

        //----------------------------------------------------------------------------
        //
        //             Now, test writing attributes
        //
        //----------------------------------------------------------------------------

        DeviceAttribute send_w;

        send_w.set_name("attr_asyn_write");
        DevLong lg = 222;
        send_w << lg;

        cb.cb_executed = 0;
        cb.cmd_failed = false;

        device->write_attribute_asynch(send_w, cb);

        // Check if attribute returned

        nb_not_arrived = 0;
        while(cb.cb_executed == 0)
        {
            TEST_LOG << "Attribute not written yet" << std::endl;
            nb_not_arrived++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        assert(cb.cb_executed == 1);
        assert(nb_not_arrived >= 2);
        assert(cb.nb_attr == 1);

        TEST_LOG << "   Asynchronous write_attribute in callback mode --> OK" << std::endl;

        //----------------------------------------------------------------------------
        //
        //             Now, test reading attributes
        //
        //----------------------------------------------------------------------------

        // Read one attribute

        cb.cb_executed = 0;

        device->read_attribute_asynch("attr_asyn", cb);

        // Check if attribute returned

        nb_not_arrived = 0;
        while(cb.cb_executed == 0)
        {
            TEST_LOG << "Attribute not read yet" << std::endl;
            nb_not_arrived++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        assert(cb.db == 5.55);
        assert(nb_not_arrived >= 2);

        TEST_LOG << "   Asynchronous read_attribute in callback mode --> OK" << std::endl;

        //----------------------------------------------------------------------------
        //
        //             Now, all in one go
        //
        //----------------------------------------------------------------------------

        cb.cb_executed = 0;
        send.clear();
        send.push_back(4);
        send.push_back(1);
        din << send;
        send_w.set_name("Long_attr_w");

        device->command_inout_asynch("IOShortSleep", din, cb);
        device->read_attribute_asynch("Double_attr", cb);
        device->write_attribute_asynch(send_w, cb);

        // Check if attribute returned

        long sleep_loop = 3;
        while(sleep_loop > 0)
        {
            TEST_LOG << "Not ready" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            sleep_loop--;
        }

        assert(cb.cb_executed == 3);

        TEST_LOG << "   All in one go (callback mode) --> OK" << std::endl;
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
