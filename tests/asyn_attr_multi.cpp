#include "common.h"

int main(int argc, char **argv)
{
	DeviceProxy *device;

	if (argc == 1)
	{
		TEST_LOG << "usage: %s device" << std::endl;
		exit(-1);
	}

	std::string device_name = argv[1];

	try
	{
		device = new DeviceProxy(device_name);
	}
        catch (CORBA::Exception &e)
        {
              	Except::print_exception(e);
		exit(1);
        }

	TEST_LOG << std::endl << "new DeviceProxy(" << device->name() << ") returned" << std::endl << std::endl;

	try
	{

// Change timeout (with a useless name as a workaround for ORBacus bug)

//		device->set_timeout_millis(6000);

// Read one attribute

		long id;
		std::vector<DeviceAttribute> *received = nullptr;
		std::vector<std::string> names;
		names.push_back("attr_asyn");
		names.push_back("Short_attr");
		id = device->read_attributes_asynch(names);

// Check if attribute returned

		bool finish = false;
		long nb_not_arrived = 0;
		while (finish == false)
		{
			try
			{
				received = device->read_attributes_reply(id);
				double db;
				short sh;
				(*received)[0] >> db;
				assert( db == 5.55 );
				(*received)[1] >> sh;
				assert( sh == 12 );
				finish = true;
			}
			catch (AsynReplyNotArrived&)
			{
				finish = false;
				TEST_LOG << "Attribute not yet read" << std::endl;
				nb_not_arrived++;
			}
			if (finish == false)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		delete received;

		assert ( nb_not_arrived >= 2);

		TEST_LOG << "   Asynchronous read_attributes in polling mode --> OK" << std::endl;

// Read attribute to check polling with blocking with timeout

		id = device->read_attributes_asynch(names);
//		assert( id == 2);

// Check if attribute returned

		finish = false;
		nb_not_arrived = 0;
		while (finish == false)
		{
			try
			{
				received = device->read_attributes_reply(id,200);
				double l;
				short s;
				(*received)[0] >> l;
				assert( l == 5.55 );
				(*received)[1] >> s;
				assert( s == 12 );
				finish = true;
			}
			catch (AsynReplyNotArrived&)
			{
				TEST_LOG << "Attribute not yet read" << std::endl;
				nb_not_arrived++;
			}
		}
		delete received;

		assert ( nb_not_arrived >= 4);

		TEST_LOG << "   Asynchronous read_attributes in blocking mode with call timeout --> OK" << std::endl;

// Send a command to check polling with blocking

		id = device->read_attributes_asynch(names);
//		assert( id == 3);

// Check if command returned

		received = device->read_attributes_reply(id,0);
		double l;
		(*received)[0] >> l;
		short s1;
		(*received)[1] >> s1;
		delete received;

		assert( l == 5.55 );
		assert( s1 == 12 );

		TEST_LOG << "   Asynchronous read_attributes in blocking mode --> OK" << std::endl;

//---------------------------------------------------------------------------
//
//			Now test Timeout exception and asynchronous calls
//
//---------------------------------------------------------------------------

// Change timeout in order to test asynchronous calls and timeout

//		device->set_timeout_millis(2000);

// Read an attribute

		names[0] = "attr_asyn_to";
		id = device->read_attributes_asynch(names);

// Check if attribute returned

		finish = false;
		bool to = false;
		nb_not_arrived = 0;
		while (finish == false)
		{
			try
			{
				received = device->read_attributes_reply(id);
				finish = true;
				delete received;
			}
			catch (AsynReplyNotArrived&)
			{
				finish = false;
				nb_not_arrived++;
				TEST_LOG << "Attributes not yet read" << std::endl;
			}
			catch (CommunicationFailed &e)
			{
				finish = true;
				if (strcmp(e.errors[1].reason,API_DeviceTimedOut) == 0)
				{
					to = true;
					TEST_LOG << "Timeout exception" << std::endl;
				}
				else
					TEST_LOG << "Comm exception" << std::endl;
			}
			if (finish == false)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		assert ( to == true );
		assert ( nb_not_arrived >= 2 );

		TEST_LOG << "   Device timeout exception with non blocking read_attributes_reply --> OK" << std::endl;

// Read an attribute to check timeout with polling and blocking with timeout

		id = device->read_attributes_asynch(names);

// Check if command returned

		finish = false;
		to = false;
		nb_not_arrived = 0;
		while (finish == false)
		{
			try
			{
				received = device->read_attributes_reply(id,500);
				finish = true;
				delete received;
			}
			catch (AsynReplyNotArrived&)
			{
				TEST_LOG << "Attributes not yet read" << std::endl;
				nb_not_arrived++;
			}
			catch (CommunicationFailed &e)
			{
				finish = true;
				if (strcmp(e.errors[1].reason,API_DeviceTimedOut) == 0)
				{
					to = true;
					TEST_LOG << "Timeout exception" << std::endl;
				}
				else
					TEST_LOG << "Comm exception" << std::endl;
			}
		}
		assert( to == true );
		assert( nb_not_arrived >= 2);

		TEST_LOG << "   Device timeout with blocking read_attributes_reply with call timeout --> OK" << std::endl;

// Read an attribute to check polling with blocking

		id = device->read_attributes_asynch(names);

// Check if attribute returned

		to = false;
		try
		{
			received = device->read_attributes_reply(id,0);
			finish = true;
			delete received;
		}
		catch (CommunicationFailed &e)
		{
			if (strcmp(e.errors[1].reason,API_DeviceTimedOut) == 0)
			{
				to = true;
				TEST_LOG << "Timeout exception" << std::endl;
			}
			else
				TEST_LOG << "Comm exception" << std::endl;
		}
		assert(to == true );

		TEST_LOG << "   Device timeout with blocking read_attributes_reply --> OK" << std::endl;

//---------------------------------------------------------------------------
//
//			Now test DevFailed exception sent by server
//
//---------------------------------------------------------------------------

		TEST_LOG << "   Waiting for server to execute all previous requests" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(5));

// Change timeout in order to test asynchronous calls and DevFailed exception

//		device->set_timeout_millis(5000);

// Read attribute

		names[0] = "attr_asyn_except";
		id = device->read_attributes_asynch(names);

// Check if attribute returned

		finish = false;
		bool failed = false;
		bool strange_err = false;
		nb_not_arrived = 0;
		short sh;
		while (finish == false)
		{
			try
			{
				received = device->read_attributes_reply(id);
				finish = true;
				if ((*received)[0].has_failed() == true)
				{
					DevErrorList err = (*received)[0].get_err_stack();
					if (strcmp(err[0].reason,"aaa") == 0)
					{
						failed = true;
						TEST_LOG << "Server exception" << std::endl;
					}
				}
				if ((*received)[1].has_failed() == true)
					strange_err = true;
				else
					(*received)[1] >> sh;
				delete received;
			}
			catch (AsynReplyNotArrived&)
			{
				finish = false;
				nb_not_arrived++;
				TEST_LOG << "Attributes not yet read" << std::endl;
			}
			catch (DevFailed&)
			{
				TEST_LOG << "Comm exception" << std::endl;
			}
			if (finish == false)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		assert ( failed == true );
		assert ( sh == 12 );
		assert (strange_err == false );
		assert ( nb_not_arrived >= 2 );

		TEST_LOG << "   Device exception with non blocking read_attributes_reply --> OK" << std::endl;

// Read an attribute to check timeout with polling and blocking with timeout

		id = device->read_attributes_asynch(names);

// Check if attribute returned

		finish = false;
		failed = false;
		while (finish == false)
		{
			try
			{
				received = device->read_attributes_reply(id,500);
				finish = true;
			}
			catch (AsynReplyNotArrived&)
			{
				TEST_LOG << "Attributes not yet read" << std::endl;
			}
			catch (DevFailed&)
			{
				TEST_LOG << "Comm exception" << std::endl;
			}
		}
		bool all_extracted = false;
		int i;
		long nb_extracted;
		sh = 0;

		while (all_extracted == false)
		{
			nb_extracted = 0;
			try
			{
				double db;
				nb_extracted++;
				(*received)[0] >> db;
				nb_extracted++;
				(*received)[1] >> sh;
				delete received;
				all_extracted = true;
			}
			catch (DevFailed &e)
			{
				for (i = 0;i < nb_extracted;i++)
					(*received)[i].reset_exceptions(DeviceAttribute::failed_flag);
				if (strcmp(e.errors[0].reason,"aaa") == 0)
				{
					failed = true;
					TEST_LOG << "Server exception" << std::endl;
				}
				else
					TEST_LOG << "Comm exception" << std::endl;
			}
		}

		assert( failed == true );
		assert( sh == 12 );

		TEST_LOG << "   Device exception with blocking read_attributes_reply with call timeout --> OK" << std::endl;

// Read an attribute to check polling with blocking

		id = device->read_attributes_asynch(names);

// Check if attribute returned

		failed = false;
		sh = 0;
		try
		{
			received = device->read_attributes_reply(id,0);
			finish = true;
			for (unsigned int j = 0;j < received->size();j++)
				(*received)[j].reset_exceptions(DeviceAttribute::failed_flag);

			double db;
			if (((*received)[0] >> db) == false)
			{
				DevErrorList err = (*received)[0].get_err_stack();
				if (strcmp(err[0].reason,"aaa") == 0)
				{
					failed = true;
					TEST_LOG << "Server exception" << std::endl;
				}
			}
			if (((*received)[1] >> sh) == false)
				strange_err = true;

			delete received;
		}
		catch (DevFailed&)
		{
			TEST_LOG << "Comm exception" << std::endl;
		}


		assert( failed == true );
		assert( sh == 12 );

		TEST_LOG << "   Device exception with blocking read_attributes_reply --> OK" << std::endl;

	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
	catch (CORBA::Exception &ex)
	{
		Except::print_exception(ex);
		exit(-1);
	}

	delete device;

	return 0;
}
