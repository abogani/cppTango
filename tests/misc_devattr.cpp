#include "common.h"

int main()
{

// Test empty

	DeviceAttribute da;
	long lo;
	bitset<DeviceAttribute::numFlags> flags;
	flags.reset(DeviceAttribute::isempty_flag);
	da.exceptions(flags);

	bool ret = da >> lo;
	assert ( ret == false );

	TEST_LOG << "   Extraction from empty object --> OK" << endl;

	flags.set(DeviceAttribute::isempty_flag);
	da.exceptions(flags);

	try
	{
		da >> lo;
		assert(false);
	}
	catch (Tango::DevFailed &)
	{
	}

	TEST_LOG << "   Extraction from empty object (exception) --> OK" << endl;
	flags.reset();

// Test wrong type

	DeviceAttribute db;
	long l = 2;
	db << l;

	short fl;

	ret = db >> fl;
	assert ( ret == false );

	TEST_LOG << "   Extraction with wrong type --> OK" << endl;

	flags.set(DeviceAttribute::wrongtype_flag);
	db.exceptions(flags);

	try
	{
		db >> fl;
		assert(false);
	}
	catch (Tango::DevFailed &)
	{
	}

	TEST_LOG << "   Extraction with wrong type (exception) --> OK" << endl;

// Test assignement operator

	DeviceAttribute dd,dd_c;
	string v_str("abc");

	dd << v_str;
	dd_c = dd;

	string out;
	dd_c >> out;

	assert( out == "abc");

	TEST_LOG << "   assignement operator --> OK" << endl;

// Test copy constructor

	DeviceAttribute d;
	double db2 = 3.45;
	d << db2;

	DeviceAttribute dc(d);

	double db_out;
	dc >> db_out;

	assert( db_out == db2 );

	TEST_LOG << "   Copy constructor --> OK" << endl;

// Test move assignement

	DeviceAttribute d_ma;
	float fl_move = 2.0;
	d_ma << fl_move;

	DeviceAttribute d_ma_out;
	d_ma_out = std::move(d_ma);

	float fl_move_out;
	d_ma_out >> fl_move_out;

	assert (fl_move == fl_move_out );

	TEST_LOG << "   Move assignement --> OK" << endl;

	return 0;
}
