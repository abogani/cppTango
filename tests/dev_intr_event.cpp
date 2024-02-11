// NOLINTBEGIN(*)

#include "common.h"

class EventCallBack : public Tango::CallBack
{
    void push_event(Tango::DevIntrChangeEventData *);

  public:
    int cb_executed;
    int cb_err;
    size_t nb_cmd;
    size_t nb_att;
    bool dev_start;
};

void EventCallBack::push_event(Tango::DevIntrChangeEventData *event_data)
{
    cb_executed++;

    try
    {
        TEST_LOG << "EventCallBack::push_event(): called device " << event_data->device_name << " event "
                 << event_data->event << "\n";
        if(!event_data->err)
        {
            TEST_LOG << "Received device interface change event for device " << event_data->device_name << std::endl;
            nb_cmd = event_data->cmd_list.size();
            nb_att = event_data->att_list.size();
            dev_start = event_data->dev_started;
        }
        else
        {
            TEST_LOG << "Error send to callback" << std::endl;
            Tango::Except::print_error_stack(event_data->errors);
            cb_err++;
        }
    }
    catch(...)
    {
        TEST_LOG << "EventCallBack::push_event(): could not extract data !\n";
    }
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

    TEST_LOG << std::endl << "new DeviceProxy(" << device->name() << ") returned" << std::endl << std::endl;

    try
    {
        EventCallBack cb;
        cb.cb_executed = 0;
        cb.cb_err = 0;

        //
        // subscribe to a interface change event
        //

        int eve_id1 = device->subscribe_event(Tango::INTERFACE_CHANGE_EVENT, &cb);

        //
        // The callback should have been executed once
        //

        assert(cb.cb_executed == 1);
        assert(cb.cb_err == 0);
        assert(cb.dev_start == true);

        size_t old_cmd_nb = cb.nb_cmd;
        size_t old_att_nb = cb.nb_att;
        cb.dev_start = false;

        TEST_LOG << "   subscribe_event --> OK" << std::endl;

        //
        // Add a dynamic command -> Should generate a device interface change event
        //

        Tango::DevLong in = 0;
        DeviceData d_in;
        d_in << in;
        device->command_inout("IOAddCommand", d_in);

        std::this_thread::sleep_for(std::chrono::seconds(1));

        assert(cb.cb_executed == 2);
        assert(cb.cb_err == 0);
        assert(cb.nb_cmd == old_cmd_nb + 1);
        assert(cb.nb_att == old_att_nb);

        //
        // Remove the dynamic command -> Should generate a device interface change event
        //

        device->command_inout("IORemoveCommand", d_in);

        std::this_thread::sleep_for(std::chrono::seconds(1));

        assert(cb.cb_executed == 3);
        assert(cb.cb_err == 0);
        assert(cb.nb_cmd == old_cmd_nb);
        assert(cb.nb_att == old_att_nb);

        //
        // Add dynamic commands in loop -> 1 event
        //

        in = 2;
        d_in << in;
        device->command_inout("IOAddCommand", d_in);

        std::this_thread::sleep_for(std::chrono::seconds(1));

        assert(cb.cb_executed == 4);
        assert(cb.cb_err == 0);
        assert(cb.nb_cmd == old_cmd_nb + 3);
        assert(cb.nb_att == old_att_nb);

        TEST_LOG << "   Dev. interface change event after add/remove dynamic command --> OK" << std::endl;

        //
        // Init and DevRestart command without any dev. interface change -> No event
        //

        device->command_inout("Init");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        assert(cb.cb_executed == 4);
        assert(cb.cb_err == 0);
        std::string adm_name = device->adm_name();
        DeviceProxy adm(adm_name);

        Tango::DeviceData d_in_restart;
        d_in_restart << device_name;
        adm.command_inout("DevRestart", d_in_restart);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        assert(cb.cb_executed == 5);
        assert(cb.cb_err == 0);
        assert(cb.nb_cmd == old_cmd_nb);
        assert(cb.nb_att == old_att_nb);

        TEST_LOG << "   Dev. interface change event after Init or DevRestart command --> OK" << std::endl;

        //
        // Restart server cmd -> event generated
        //

        in = 0;
        d_in << in;
        device->command_inout("IOAddCommand", d_in);

        std::this_thread::sleep_for(std::chrono::seconds(1));

        assert(cb.cb_executed == 6);
        assert(cb.cb_err == 0);
        assert(cb.nb_cmd == old_cmd_nb + 1);
        assert(cb.nb_att == old_att_nb);

        adm.command_inout("RestartServer");
        std::this_thread::sleep_for(std::chrono::seconds(5));

        assert(cb.cb_executed == 7);
        assert(cb.cb_err == 0);
        assert(cb.nb_cmd == old_cmd_nb);
        assert(cb.nb_att == old_att_nb);
        assert(cb.dev_start == false);

        TEST_LOG << "   Event sent after RestartServer command --> OK" << std::endl;

        //
        // unsubscribe to the event
        //

        device->unsubscribe_event(eve_id1);

        TEST_LOG << "   unsubscribe_event --> OK" << std::endl;
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
