#include "common.h"

#ifdef _TG_WINDOWS_
#include <sys/types.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void set_user_device_right(DeviceProxy *,const char *,const char *,const char *);
void set_user_address(DeviceProxy *,const char *,const char *);
void remove_user(DeviceProxy *,string &);
void check_device_access(DeviceProxy *,bool,bool);
void check_device_access(string &,bool,bool);
void call_devices_ds_off(DeviceProxy *,DeviceProxy *,bool);
TangoSys_Pid start_ds(const std::string& path, const std::string& name, const std::string& inst);

int main(int argc, char **argv)
{	
	if ((argc < 6) || (argc > 7))
	{
		TEST_LOG << "usage: acc_right user host device1 device2 admin_device" << endl;
		exit(-1);
	}

	string user = argv[1];
	string host = argv[2];
	string device = argv[3];
	string another_device(argv[4]);
	string admin_device(argv[5]);

	struct hostent *my_addr;
	if ((my_addr = gethostbyname(host.c_str())) == NULL)
	{
		cerr << "Can't get IP address for given host!" << endl;
		exit(-1);
	}

	struct in_addr **addr_list;		
	addr_list = (struct in_addr **)my_addr->h_addr_list;
	char *host_ptr = inet_ntoa(*addr_list[0]);

	string host_ip(host_ptr);

	TEST_LOG << "Host IP address: " << host_ip << endl;

//
// Get info on control access device
//

	Database *db = new Database();

	string serv_name("AccessControl");
	string serv_inst("tango");

	DbDatum db_serv = db->get_services(serv_name,serv_inst);

	string serv_dev_name;
	vector<string> serv_dev_list;
	db_serv >> serv_dev_list;
	if (serv_dev_list.size() > 0)
		serv_dev_name = serv_dev_list[0];
	else
	{
		cerr << "Control access not defined in your control system" << endl;
		exit(-1);
	}

	TEST_LOG << "Control access dev name = " << serv_dev_name << endl;

//
// Connect to the control access service
//

	DeviceProxy *acc_device;
	try 
	{
		acc_device = new DeviceProxy(serv_dev_name);
	}
	catch (CORBA::Exception &e)
	{
		Except::print_exception(e);
		exit(1);
	}

	TEST_LOG << "Connected to Controlled access service" << endl;

//
// Add some allowed commands on the controlled access device to do our tests
//

	try
	{
		DbData db_data_prop;
		db_data_prop.push_back(DbDatum("AllowedAccessCmd"));

		db->get_class_property("TangoAccessControl", db_data_prop);

		vector<string> allo_cmds;
		db_data_prop[0] >> allo_cmds;
		int old_cmd_nb = allo_cmds.size();

		if (allo_cmds.size() == 0)
		{
			TEST_LOG << "Your control system is not configured property to run the controlled access!" << endl;
			exit(-1);
		}

		if (find(allo_cmds.begin(),allo_cmds.end(),"AddDeviceForUser") == allo_cmds.end())
			allo_cmds.push_back("AddDeviceForUser");

		if (find(allo_cmds.begin(),allo_cmds.end(),"RemoveUser") == allo_cmds.end())
			allo_cmds.push_back("RemoveUser");

		if (find(allo_cmds.begin(),allo_cmds.end(),"RemoveDeviceForUser") == allo_cmds.end())
			allo_cmds.push_back("RemoveDeviceForUser");

		if (find(allo_cmds.begin(),allo_cmds.end(),"AddAddressForUser") == allo_cmds.end())
			allo_cmds.push_back("AddAddressForUser");

		int new_cmd_nb = allo_cmds.size();

		if (new_cmd_nb != old_cmd_nb)
		{
			DbData db_put;
			DbDatum db_tum("AllowedAccessCmd");
			db_tum << allo_cmds;
			db_put.push_back(db_tum);

			db->put_class_property("TangoAccessControl", db_put);
		}
	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}

//--------------- BASIC FEATURES TEST--------------------

//
// Set default user address to "*.*.*.*" and right to READ
// and remove the given user from access tables
//

	set_user_device_right(acc_device,"*","*/*/*","write");
	remove_user(acc_device,user);
	set_user_address(acc_device,"*","*.*.*.*");
	set_user_device_right(acc_device,"*","*/*/*","read");

	check_device_access(device,false,false);

	TEST_LOG << "  Access allowed/blocked for first config (user not defined, * read) --> OK" << endl;

//
// Set default user right to WRITE
//

	set_user_device_right(acc_device,"*","*/*/*","write");

	check_device_access(device,true,false);

	TEST_LOG << "  Access allowed/blocked for second config (user not defined, * WRITE) --> OK" << endl;

//
// Set default user right to READ
// Set cmd line user right to WRITE on the command line device
//

	set_user_device_right(acc_device,"*","*/*/*","read");
	set_user_device_right(acc_device,user.c_str(),device.c_str(),"write");

	check_device_access(device,true,false);

	TEST_LOG << "  Access allowed/blocked for third config (* READ, user defined with device WRITE) --> OK" << endl;

//
// Set cmd line user right to READ on the command line device and WRITE on all other device
//

	set_user_device_right(acc_device,user.c_str(),device.c_str(),"read");
	set_user_device_right(acc_device,user.c_str(),"*/*/*","write");


	string another_dev(another_device);

	check_device_access(device,false,false);
	check_device_access(another_dev,true,false);

	TEST_LOG << "  Access allowed/blocked for forth config (* READ, user defined with default device WRITE, cmd line device READ) --> OK" << endl;

//
// Set cmd line user right to READ on the command line device and WRITE on all other device
//

	set_user_address(acc_device,user.c_str(),"160.103.99.99");
	set_user_device_right(acc_device,user.c_str(),device.c_str(),"write");

	check_device_access(device,false,false);

	TEST_LOG << "  Access allowed/blocked for fifth config (cmd line device WRITE + allowed from wrong address) --> OK" << endl;

//
// Set cmd line user right to READ on the command line device and WRITE on all other device
//

	set_user_address(acc_device,user.c_str(),host_ip.c_str());

	check_device_access(device,true,false);

	TEST_LOG << "  Access allowed/blocked for sixth config (cmd line device WRITE + allowed from user address) --> OK" << endl;

//--------------- CONTROLLED ACCESS AND CONTROL SYSTEM CONFIG TEST--------------------

//
// Remove the Tango Control service from the services list
// and remove everything to simulate an appli start-up
//

	set_user_device_right(acc_device,user.c_str(),device.c_str(),"read");

	db->unregister_service(serv_name,serv_inst);

	delete acc_device;
	delete db;

	ApiUtil::cleanup();

//
// All devices have write access
//

	check_device_access(device,true,false);

	TEST_LOG << "  Write access if no access service defined in DB --> OK" << endl;

//
// Re-create CtrlSystem property
//

	db = new Database();
	db->register_service(serv_name,serv_inst,serv_dev_name);

//
// Kill the Controlled access service and clean-up another time
//

	DeviceProxy *ca_admin = new DeviceProxy("dserver/tangoaccesscontrol/1");
	ca_admin->command_inout("Kill");
	std::this_thread::sleep_for(std::chrono::seconds(1));

	delete ca_admin;
	delete db;

	ApiUtil::cleanup();

//
// All devices have read access
//

	check_device_access(device,false,true);

	TEST_LOG << "  Read access if access service defined in DB but is not running (even for allowed commands) --> OK" << endl;

//
// Start the controlled access service
//

	string cmd_line("export MYSQL_USER=root;export MYSQL_PASSWORD=root;export SUPER_TANGO=true;/home/taurel/tango/cppserver/TangoAccessControl/bin/ubuntu810/TangoAccessControl 1 2>/dev/null 1>/dev/null &");
	system(cmd_line.c_str());

	std::this_thread::sleep_for(std::chrono::seconds(2));
	ApiUtil::cleanup();

	try 
	{
		acc_device = new DeviceProxy(serv_dev_name);
	}
	catch (CORBA::Exception &e)
	{
		Except::print_exception(e);
		exit(1);
	}

	set_user_device_right(acc_device,user.c_str(),device.c_str(),"write");

	check_device_access(device,true,false);

	TEST_LOG << "  Controlled access running after service re-start --> OK" << endl;

//--------------- DEVICE RE_CONNECTION TEST--------------------

	set_user_device_right(acc_device,user.c_str(),another_dev.c_str(),"read");

	DeviceProxy *dev_dp;
	DeviceProxy *another_dev_dp;
	TangoSys_Pid ds_pid;

	try
	{
		dev_dp = new  DeviceProxy(device);
		another_dev_dp = new DeviceProxy(another_dev);
		
		check_device_access(dev_dp,true,false);
		check_device_access(another_dev_dp,false,false);

		TEST_LOG << "Access before killing the DS --> OK" << endl;

		DeviceProxy *ds_admin = new DeviceProxy(admin_device);
		ds_admin->command_inout("Kill");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		ds_pid = start_ds("/home/taurel/tango/cppapi_tst_ds/bin/ubuntu904/devTest","devTest","api");
		TEST_LOG << "DS pid = " << ds_pid << endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "DS killed and re-started" << endl;

		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access after DS restart for " << device << " OK" << endl;
		check_device_access(another_dev_dp,false,false);
		TEST_LOG << "Access after DS restart for " << another_dev << " OK" << endl;
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
		exit(-1);
	}

	TEST_LOG << "  Device rights after re-connection (without calls between DS kill/start) --> OK" << endl;

	try
	{
		
		check_device_access(dev_dp,true,false);
		check_device_access(another_dev_dp,false,false);

		TEST_LOG << "Access before killing the DS --> OK" << endl;

		DeviceProxy *ds_admin = new DeviceProxy(admin_device);
		ds_admin->command_inout("Kill");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		TEST_LOG << "DS killed" << endl;

		call_devices_ds_off(dev_dp,another_dev_dp,false);

		ds_pid = start_ds("/home/taurel/tango/cppapi_tst_ds/bin/ubuntu904/devTest","devTest","api");
		TEST_LOG << "DS pid = " << ds_pid << endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "DS re-started" << endl;

		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access after DS restart for " << device << " OK" << endl;
		check_device_access(another_dev_dp,false,false);
		TEST_LOG << "Access after DS restart for " << another_dev << " OK" << endl;
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
		exit(-1);
	}

	TEST_LOG << "  Device rights after re-connection (with calls between DS kill/start) --> OK" << endl;

	TangoSys_MemStream cmd_stream;
	cmd_stream << "kill -9 " << ds_pid;
	TEST_LOG << "Kill cmd = " << cmd_stream.str() << endl;

	try
	{
		system(cmd_stream.str().c_str());

		std::this_thread::sleep_for(std::chrono::seconds(1));

		ds_pid = start_ds("/home/taurel/tango/cppapi_tst_ds/bin/ubuntu904/devTest","devTest","api");
		TEST_LOG << "DS pid = " << ds_pid << endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "DS awfully killed and re-started" << endl;

		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access after DS restart for " << device << " OK" << endl;
		check_device_access(another_dev_dp,false,false);
		TEST_LOG << "Access after DS restart for " << another_dev << " OK" << endl;
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
		exit(-1);
	}

	TEST_LOG << "  Device rights after re-connection (kill -9 and without calls between DS kill/start) --> OK" << endl;

	cmd_stream.str("");
	cmd_stream << "kill -9 " << ds_pid;
	TEST_LOG << "Second kill cmd = " << cmd_stream.str() << endl;

	try
	{
		system(cmd_stream.str().c_str());

		std::this_thread::sleep_for(std::chrono::seconds(1));

		TEST_LOG << "DS killed" << endl;

		call_devices_ds_off(dev_dp,another_dev_dp,true);

		ds_pid = start_ds("/home/taurel/tango/cppapi_tst_ds/bin/ubuntu904/devTest","devTest","api");
		TEST_LOG << "DS pid = " << ds_pid << endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "DS re-started" << endl;

		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access after DS restart for " << device << " OK" << endl;
		check_device_access(another_dev_dp,false,false);
		TEST_LOG << "Access after DS restart for " << another_dev << " OK" << endl;
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
		exit(-1);
	}

	TEST_LOG << "  Device rights after re-connection (kill -9 and with calls between DS kill/start) --> OK" << endl;

	delete another_dev_dp;

//--------------- RE_CONNECTION TO CONTROLLED ACCESS SERVICE TEST--------------------

	TangoSys_Pid ca_pid;
	try
	{		
		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access before killing the Controlled Access service --> OK" << endl;

		DeviceProxy *ca_admin = new DeviceProxy("dserver/tangoaccesscontrol/1");
		ca_admin->command_inout("Kill");
		std::this_thread::sleep_for(std::chrono::seconds(1));

		delete ca_admin;

		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access for already created DeviceProxy after CA is killed --> OK" << endl;

		delete dev_dp;
		dev_dp = new DeviceProxy(device);

//		check_device_access(dev_dp,false,false);
		TEST_LOG << "Read access for DeviceProxy created while the CA is down --> OK" << endl;

		ca_pid = start_ds("/home/taurel/tango/cppserver/TangoAccessControl/bin/ubuntu810/TangoAccessControl","TangoAccessControl","1");
		TEST_LOG << "CA re-started" << endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		delete dev_dp;
		dev_dp = new DeviceProxy(device);

		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access after CA restart for " << device << " OK" << endl;
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
		exit(-1);
	}

	TEST_LOG << "  Device rights after re-start of Controlled Access Service --> OK" << endl;

	try
	{		
		cmd_stream.str("");
		cmd_stream << "kill -9 " << ca_pid;
		TEST_LOG << "Third kill cmd = " << cmd_stream.str() << endl;

		system(cmd_stream.str().c_str());
		std::this_thread::sleep_for(std::chrono::seconds(1));


		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access for already created DeviceProxy after CA is killed -9 --> OK" << endl;

		delete dev_dp;
		dev_dp = new DeviceProxy(device);

//		check_device_access(dev_dp,false,false);
		TEST_LOG << "Read access for DeviceProxy created while the CA is killed -9 --> OK" << endl;

		ca_pid = start_ds("/home/taurel/tango/cppserver/TangoAccessControl/bin/ubuntu810/TangoAccessControl","TangoAccessControl","1");
		TEST_LOG << "CA re-started" << endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		delete dev_dp;
		dev_dp = new DeviceProxy(device);

		check_device_access(dev_dp,true,false);
		TEST_LOG << "Access after CA restart for " << device << " OK" << endl;
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
		exit(-1);
	}

	TEST_LOG << "  Device rights after re-start of Controlled Access Service killed by -9 --> OK" << endl;

//
// Reset the TangoAccessControl class to its default config
//

	db = new Database();
	try
	{
		DbData db_data_prop_end;
		db_data_prop_end.push_back(DbDatum("AllowedAccessCmd"));

		db->get_class_property("TangoAccessControl", db_data_prop_end);

		vector<string> allo_cmds_end;
		db_data_prop_end[0] >> allo_cmds_end;

		vector<string>::iterator pos1;
		if ((pos1 = find(allo_cmds_end.begin(),allo_cmds_end.end(),"AddDeviceForUser")) != allo_cmds_end.end())
			allo_cmds_end.erase(pos1);

		if ((pos1 = find(allo_cmds_end.begin(),allo_cmds_end.end(),"RemoveUser")) != allo_cmds_end.end())
			allo_cmds_end.erase(pos1);

		if ((pos1 = find(allo_cmds_end.begin(),allo_cmds_end.end(),"RemoveDeviceForUser")) != allo_cmds_end.end())
			allo_cmds_end.erase(pos1);

		if ((pos1 = find(allo_cmds_end.begin(),allo_cmds_end.end(),"AddAddressForUser")) != allo_cmds_end.end())
			allo_cmds_end.erase(pos1);

		DbData db_put_end;
		DbDatum db_tum_end("AllowedAccessCmd");
		db_tum_end << allo_cmds_end;
		db_put_end.push_back(db_tum_end);

//		db->put_class_property("TangoAccessControl", db_put_end);
	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}

	return 0;
	
}

void set_user_address(DeviceProxy *dev,const char *user,const char *addr)
{
	try
	{
		DeviceData din,dout;
		din << user;

		dout = dev->command_inout("GetAddressByUser",din);

		vector<string> addresses;
		dout >> addresses;
		string addr_str(addr);

		if (find(addresses.begin(),addresses.end(),addr_str) == addresses.end())
		{
			string user_str(user);

			vector<string> in;
			in.push_back(user_str);
			in.push_back(addr_str);

			din << in;
			dev->command_inout("AddAddressForUser",din);
		}
	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
}

void set_user_device_right(DeviceProxy *acc_dev,const char *user,const char *dev,const char *right)
{
	try
	{
		DeviceData din,dout;
		din << user;

		dout = acc_dev->command_inout("GetDeviceByUser",din);

		vector<string> dev_list;
		dout >> dev_list;

		vector<string>::iterator pos;
		string dev_str(dev);
		string right_str(right);

		string user_str(user);

		vector<string> in;
		in.push_back(user_str);
		in.push_back(dev_str);

		if ((pos = find(dev_list.begin(),dev_list.end(),dev_str)) != dev_list.end())
		{
			pos++;
			in.push_back(*pos);
			din << in;
			acc_dev->command_inout("RemoveDeviceForUser",din);
			in.pop_back();
		}
	
		in.push_back(right_str);	
		din << in;
		acc_dev->command_inout("AddDeviceForUser",din);	
	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
}

void remove_user(DeviceProxy *dev,string &user)
{
	try
	{
		DeviceData din;
		din << user;

		dev->command_inout("RemoveUser",din);
	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
}

void check_device_access(string &device,bool allowed,bool all_cmd_not_allowed)
{

//
// Connect to the device
//

	try
	{
		DeviceProxy *dev = new DeviceProxy(device);
		check_device_access(dev,allowed,all_cmd_not_allowed);
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
		exit(-1);
	}
}

void check_device_access(DeviceProxy *dev,bool allowed,bool all_cmd_not_allowed)
{

	try
	{

//
// Read attribute is allowed
//

		DeviceAttribute att;
		att = dev->read_attribute("Float_attr");

//
// Write attribute is allowed
//

		bool read_only_except = false;
		try
		{
			DeviceAttribute attr("Float_attr_w",(float)3.0);
			dev->write_attribute(attr);
		}
		catch (Tango::DevFailed &e)
		{
//			Except::print_exception(e);
			if (::strcmp(e.errors[0].reason.in(),API_ReadOnlyMode) == 0)
				read_only_except = true;
		}

		if (allowed == true)
			assert (read_only_except == false);
		else
			assert (read_only_except == true);

//
// IOShort command is allowed
//

		read_only_except = false;
		short short_res;
		try
		{
			DeviceData din,dout;
			din << (short)33;

			dout = dev->command_inout("IOShort",din);
			dout >> short_res;
		}
		catch (Tango::DevFailed &e)
		{
//			Except::print_exception(e);
			if (::strcmp(e.errors[0].reason.in(),API_ReadOnlyMode) == 0)
				read_only_except = true;
		}

		if (allowed == false)
			assert (read_only_except == true);
		else
		{
			assert (read_only_except == false);
			assert (short_res == 66);
		}

//
// IOLong is allways an allowed commands except if the appli
// has been started with a defined controlled access service
// and no service running
//

		DevLong res;
		read_only_except = false;
		try
		{
			DeviceData din,dout;

			din << (DevLong)222;

			dout = dev->command_inout("IOLong",din);
			dout >> res;
		}
		catch (Tango::DevFailed &e)
		{
//			Except::print_exception(e);
			if (::strcmp(e.errors[0].reason.in(),API_ReadOnlyMode) == 0)
				read_only_except = true;
		}

		if (all_cmd_not_allowed == false)
		{
			assert (read_only_except == false);
			assert (res == 444);
		}
		else
			assert (read_only_except == true);
	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
}


void call_devices_ds_off(DeviceProxy *dev_dp,DeviceProxy *another_dev_dp,bool kill9)
{
	bool not_exported_except = false;
	bool cant_connect_except = false;
	try
	{
		dev_dp->command_inout("State");
	}
	catch (Tango::DevFailed &e)
	{
//		Except::print_exception(e);
		if (e.errors.length() == 2)
		{
			if (::strcmp(e.errors[0].reason.in(),API_DeviceNotExported) == 0)
				not_exported_except = true;
		}
		else if (e.errors.length() == 3)
		{
			if (::strcmp(e.errors[1].reason.in(),API_CantConnectToDevice) == 0)
				cant_connect_except = true;
		}
	}
	if (kill9 == true)
		assert (cant_connect_except == true);
	else
		assert (not_exported_except == true);
	TEST_LOG << "First State: OK" << endl;

	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	not_exported_except = false;
	cant_connect_except = false;
	try
	{
		dev_dp->command_inout("Status");
	}
	catch (Tango::DevFailed &e)
	{
//		Except::print_exception(e);
		if (e.errors.length() == 2)
		{
			if (::strcmp(e.errors[0].reason.in(),API_DeviceNotExported) == 0)
				not_exported_except = true;
		}
		else if (e.errors.length() == 3)
		{
			if (::strcmp(e.errors[1].reason.in(),API_CantConnectToDevice) == 0)
				cant_connect_except = true;
		}
	}
	if (kill9 == true)
		assert (cant_connect_except == true);
	else
		assert (not_exported_except == true);
	TEST_LOG << "First Status: OK" << endl;

	std::this_thread::sleep_for(std::chrono::seconds(1));

	not_exported_except = false;
	cant_connect_except = false;
	try
	{
		another_dev_dp->command_inout("State");
	}
	catch (Tango::DevFailed &e)
	{
//		Except::print_exception(e);
		if (e.errors.length() == 2)
		{
			if (::strcmp(e.errors[0].reason.in(),API_DeviceNotExported) == 0)
				not_exported_except = true;
		}
		else if (e.errors.length() == 3)
		{
			if (::strcmp(e.errors[1].reason.in(),API_CantConnectToDevice) == 0)
				cant_connect_except = true;
		}
	}
	if (kill9 == true)
		assert (cant_connect_except == true);
	else
		assert (not_exported_except == true);
	TEST_LOG << "Second State: OK" << endl;

	std::this_thread::sleep_for(std::chrono::seconds(1));

	not_exported_except = false;
	cant_connect_except = false;
	try
	{
		another_dev_dp->command_inout("Status");
	}
	catch (Tango::DevFailed &e)
	{
//		Except::print_exception(e);
		if (e.errors.length() == 2)
		{
			if (::strcmp(e.errors[0].reason.in(),API_DeviceNotExported) == 0)
				not_exported_except = true;
		}
		else if (e.errors.length() == 3)
		{
			if (::strcmp(e.errors[1].reason.in(),API_CantConnectToDevice) == 0)
				cant_connect_except = true;
		}
	}
	if (kill9 == true)
		assert (cant_connect_except == true);
	else
		assert (not_exported_except == true);
	TEST_LOG << "Second Status: OK" << endl;
}

TangoSys_Pid start_ds(const std::string& path, const std::string& name, const std::string& inst)
{
	TangoSys_Pid pi;
#ifdef _TG_WINDOWS_
        TS_FAIL("not implemented");

        std::string cmd_line = path + " " + inst;
        char* cmdline = new char[cmd_line.length() + 1];
        strcpy(cmdline, cmd_line.c_str());

        STARTUPINFO si;
        PROCESS_INFORMATION info;

        if(!CreateProcess(path.c_str(),
                    cmdline,
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &si,
                    &info)
                )
        {
            TEST_LOG << "Error !!!!!!!!!!" << GetLastError() << endl;
            exit(-1);
        }

        pi = info.dwProcessId;
        delete[] cmdline;
#else
	pi = fork();
	if (pi == 0)
	{
		if (close(1) ==  -1)
		{
			TEST_LOG << "Can't close 1" << endl;
			exit(-1);
		}

		int fd;
		if ((fd = open("/dev/null",O_WRONLY)) == -1)
		{
			TEST_LOG << "Can't open /dev/null" << endl;
			exit(-1);
		}

		if (close(2) == -1)
		{
			TEST_LOG << "Can't close 2" << endl;
			exit(-1);
		}

		if ((fd = open("/dev/null",O_WRONLY)) == -1)
		{
			TEST_LOG << "Cann't open /dev/null for 2" << endl;
			exit(-1);
		}
	
		char *args[] = {(char *)name.c_str(),(char *)inst.c_str(),(char *)0};
		execv(path.c_str(),args);
		TEST_LOG << "Error !!!!!!!!!!" << endl;
	}
	else if (pi < 0)
		pi = -1;
#endif
	return pi;
}
