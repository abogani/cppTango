#include "common.h"

int main(int argc, char **argv)
{
	DeviceProxy *device;

	if (argc != 2)
	{
		TEST_LOG << "usage: %s device" << endl;
		exit(-1);
	}

	string device_name = argv[1];

	try
	{
		device = new DeviceProxy(device_name);

		TEST_LOG << "new DeviceProxy does not throw exception" << endl;
	}
        catch (CORBA::Exception &e)
        {
              	Except::print_exception(e);
		exit(1);
        }

	TEST_LOG << endl << "Device IDL version : " << device->get_idl_version() << endl;

	try
	{
//		device->ping();
		device->command_inout("Status");
	}
	catch (CORBA::Exception &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
	TEST_LOG << "Ping successfull" << endl;

//	sleep(1000);



}
