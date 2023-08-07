#include "common.h"

int main(int argc, char **argv)
{
	DeviceProxy *device;

	if (argc != 4)
	{
		TEST_LOG << "usage: copy_devproxy <device1> <device2> <device3>" << endl;
		exit(-1);
	}

	string device1_name(argv[1]);
	string device2_name(argv[2]);
	string device3_name(argv[3]);

	try
	{
		device = new DeviceProxy(device1_name);
	}
	catch (CORBA::Exception &e)
	{
		Except::print_exception(e);
		exit(1);
	}

	TEST_LOG << endl << "new DeviceProxy(" << device->name() << ") returned" << endl << endl;

	try
	{

// Test copy constructor

		DeviceProxy dev2(*device);

		assert (dev2.name() == device1_name);
#ifndef COMPAT
		assert (dev2.get_idl_version() == 5);
#endif

	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}

	TEST_LOG << "   Copy constructor --> OK" << endl;

// Test assignement operator

	DeviceProxy dev2(device2_name);
	DeviceProxy dev3(device3_name);

	dev3 = *device;

	assert (dev3.name() == device1_name);
#ifndef COMPAT
	assert (dev3.get_idl_version() == 5);
#endif

	TEST_LOG << "   Assignement operator --> OK" << endl;

	delete device;
	return 0;
}
