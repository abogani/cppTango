// NOLINTBEGIN(*)

#include "common.h"

class EventCallBack : public CountingCallBack<Tango::PipeEventData>
{
  public:
    std::string root_blob_name()
    {
        auto gaurd = lock();
        return m_root_blob_name;
    }

    size_t nb_data()
    {
        auto gaurd = lock();
        return m_nb_data;
    }

  private:
    bool process_event(Tango::PipeEventData *) override;

    std::string m_root_blob_name;
    size_t m_nb_data;
};

bool EventCallBack::process_event(Tango::PipeEventData *event_data)
{
    try
    {
        TEST_LOG << "EventCallBack::process_event(): called pipe " << event_data->pipe_name << " event "
                 << event_data->event << "\n";
        if(!event_data->err)
        {
            TEST_LOG << "Received pipe event for pipe " << event_data->pipe_name << std::endl;
            m_root_blob_name = event_data->pipe_value->get_root_blob_name();

            if(m_root_blob_name == "PipeEventCase4")
            {
                std::vector<Tango::DevLong> v_dl;
                (*(event_data->pipe_value))["Martes"] >> v_dl;
                m_nb_data = v_dl.size();
            }
            return false;
        }

        TEST_LOG << "Error sent to callback" << std::endl;
        return true;
    }
    catch(...)
    {
        TEST_LOG << "EventCallBack::process_event(): could not extract data !\n";
        return true;
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

        //
        // subscribe to a pipe event
        //

        int eve_id1 = device->subscribe_event("RWPipe", Tango::PIPE_EVENT, &cb);

        //
        // The callback should have been executed once
        //

        assert(cb.invocation_count() == 1);
        assert(cb.error_count() == 0);

        TEST_LOG << "   subscribe_event --> OK" << std::endl;

        //
        // Ask device to push a pipe event
        //

        Tango::DevShort ev_type = 0;
        Tango::DeviceData dd;
        dd << ev_type;

        device->command_inout("PushPipeEvent", dd);

        //
        // The callback should have been executed
        //

        cb.wait_for([&]() { return cb.invocation_count() >= 2; });

        assert(cb.invocation_count() == 2);
        assert(cb.error_count() == 0);
        assert(cb.root_blob_name() == "PipeEventCase0");

        //
        // Ask device to push a pipe event with another data
        //

        ev_type = 1;
        dd << ev_type;

        device->command_inout("PushPipeEvent", dd);

        //
        // The callback should have been executed
        //

        cb.wait_for([&] { return cb.invocation_count() >= 3; });

        assert(cb.invocation_count() == 3);
        assert(cb.error_count() == 0);
        assert(cb.root_blob_name() == "PipeEventCase1");

        TEST_LOG << "   received event --> OK" << std::endl;

        //
        // Ask device to push a pipe event when date is specified
        //

        ev_type = 2;
        dd << ev_type;

        device->command_inout("PushPipeEvent", dd);

        //
        // The callback should have been executed
        //

        cb.wait_for([&]() { return cb.invocation_count() >= 4; });

        assert(cb.invocation_count() == 4);
        assert(cb.error_count() == 0);
        assert(cb.root_blob_name() == "PipeEventCase2");

        TEST_LOG << "   received event (with specified date) --> OK" << std::endl;

        //
        // Ask device to push a pipe event with error
        //

        ev_type = 3;
        dd << ev_type;

        device->command_inout("PushPipeEvent", dd);

        //
        // The callback should have been executed
        //

        cb.wait_for([&]() { return cb.invocation_count() >= 5; });

        assert(cb.invocation_count() == 5);
        assert(cb.error_count() == 1);

        TEST_LOG << "   received event (with error) --> OK" << std::endl;

        //
        // Ask device to push a pipe event with enough data to trigger a no copy event sending
        //

        ev_type = 4;
        dd << ev_type;

        device->command_inout("PushPipeEvent", dd);

        //
        // The callback should have been executed
        //

        cb.wait_for([&]() { return cb.invocation_count() >= 6; });

        assert(cb.invocation_count() == 6);
        assert(cb.error_count() == 1);
        assert(cb.root_blob_name() == "PipeEventCase4");
        assert(cb.nb_data() == 3000);

        TEST_LOG << "   received event (no copy sending) --> OK" << std::endl;

        //
        // unsubscribe to the event
        //

        device->unsubscribe_event(eve_id1);

        TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

        //
        // subscribe to a another pipe
        //

        cb.reset_counts();

        DeviceData d_in;
        d_in << (short) 9;
        device->command_inout("SetPipeOutput", d_in);

        eve_id1 = device->subscribe_event("RPipe", Tango::PIPE_EVENT, &cb);

        cb.wait_for([&]() { return cb.invocation_count() >= 2; });

        assert(cb.invocation_count() == 2);
        assert(cb.error_count() == 0);

        DevicePipe pipe_data = device->read_pipe("rPipe");

        cb.wait_for([&]() { return cb.invocation_count() >= 3; });

        assert(cb.invocation_count() == 3);
        assert(cb.error_count() == 0);

        device->unsubscribe_event(eve_id1);

        TEST_LOG << "   read_pipe which trigger a push_pipe_event --> OK" << std::endl;
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
