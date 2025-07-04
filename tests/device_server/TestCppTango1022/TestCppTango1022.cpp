// NOLINTBEGIN(*)

/*----- PROTECTED REGION ID(TestCppTango1022.cpp) ENABLED START -----*/
/* clang-format on */
//=============================================================================
//
// file :        TestCppTango1022.cpp
//
// description : C++ source for the TestCppTango1022 class and its commands.
//               The class is derived from Device. It represents the
//               CORBA servant object which will be accessed from the
//               network. All commands which can be executed on the
//               TestCppTango1022 are implemented in this file.
//
//
//
// This file is part of Tango device class.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
//
//
// Copyright (C): 2022
//                European Synchrotron Radiation Facility
//                BP 220, Grenoble 38043
//                France
//
//=============================================================================
//                This file is generated by POGO
//        (Program Obviously used to Generate tango Object)
//=============================================================================

#include "TestCppTango1022.h"
#include "TestCppTango1022Class.h"

/* clang-format off */
/*----- PROTECTED REGION END -----*/	//	TestCppTango1022.cpp

/**
 *  TestCppTango1022 class description:
 *
 */

//================================================================
//  The following table gives the correspondence
//  between command and method names.
//
//  Command name  |  Method name
//================================================================
//  State         |  Inherited (no method)
//  Status        |  Inherited (no method)
//================================================================

//================================================================
//  Attributes managed is:
//================================================================
//================================================================

namespace TestCppTango1022_ns
{
/*----- PROTECTED REGION ID(TestCppTango1022::namespace_starting) ENABLED START -----*/
/* clang-format on */

//	static initializations
/* clang-format off */
/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::namespace_starting

//--------------------------------------------------------
/**
 *	Method      : TestCppTango1022::TestCppTango1022()
 * Description:  Constructors for a Tango device
 *                implementing the classTestCppTango1022
 */
//--------------------------------------------------------
TestCppTango1022::TestCppTango1022(Tango::DeviceClass *cl, std::string &s)
 : TANGO_BASE_CLASS(cl, s.c_str())
{
	/*----- PROTECTED REGION ID(TestCppTango1022::constructor_1) ENABLED START -----*/
    /* clang-format on */
    init_device();
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::constructor_1
}
//--------------------------------------------------------
TestCppTango1022::TestCppTango1022(Tango::DeviceClass *cl, const char *s)
 : TANGO_BASE_CLASS(cl, s)
{
	/*----- PROTECTED REGION ID(TestCppTango1022::constructor_2) ENABLED START -----*/
    /* clang-format on */
    init_device();
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::constructor_2
}
//--------------------------------------------------------
TestCppTango1022::TestCppTango1022(Tango::DeviceClass *cl, const char *s, const char *d)
 : TANGO_BASE_CLASS(cl, s, d)
{
	/*----- PROTECTED REGION ID(TestCppTango1022::constructor_3) ENABLED START -----*/
    /* clang-format on */
    init_device();
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::constructor_3
}
//--------------------------------------------------------
TestCppTango1022::~TestCppTango1022()
{
	delete_device();
}

//--------------------------------------------------------
/**
 *	Method      : TestCppTango1022::delete_device()
 * Description:  will be called at device destruction or at init command
 */
//--------------------------------------------------------
void TestCppTango1022::delete_device()
{
	DEBUG_STREAM << "TestCppTango1022::delete_device() " << device_name << std::endl;
	/*----- PROTECTED REGION ID(TestCppTango1022::delete_device) ENABLED START -----*/
    /* clang-format on */
    //	Delete device allocated objects
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::delete_device
}

//--------------------------------------------------------
/**
 *	Method      : TestCppTango1022::init_device()
 * Description:  will be called at device initialization.
 */
//--------------------------------------------------------
void TestCppTango1022::init_device()
{
	DEBUG_STREAM << "TestCppTango1022::init_device() create device " << device_name << std::endl;
	/*----- PROTECTED REGION ID(TestCppTango1022::init_device_before) ENABLED START -----*/
    /* clang-format on */
    //	Initialization before get_device_property() call
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::init_device_before

	//	No device property to be read from database

	/*----- PROTECTED REGION ID(TestCppTango1022::init_device) ENABLED START -----*/
    /* clang-format on */
    //	Initialize device
    set_state(Tango::ON);
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::init_device
}


//--------------------------------------------------------
/**
 *	Method      : TestCppTango1022::always_executed_hook()
 * Description:  method always executed before any command is executed
 */
//--------------------------------------------------------
void TestCppTango1022::always_executed_hook()
{
	DEBUG_STREAM << "TestCppTango1022::always_executed_hook()  " << device_name << std::endl;
	/*----- PROTECTED REGION ID(TestCppTango1022::always_executed_hook) ENABLED START -----*/
    /* clang-format on */
    //	code always executed before all requests
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::always_executed_hook
}

//--------------------------------------------------------
/**
 *	Method      : TestCppTango1022::read_attr_hardware()
 * Description:  Hardware acquisition for attributes
 */
//--------------------------------------------------------
void TestCppTango1022::read_attr_hardware(TANGO_UNUSED(std::vector<long> &attr_list))
{
	DEBUG_STREAM << "TestCppTango1022::read_attr_hardware(std::vector<long> &attr_list) entering... " << std::endl;
	/*----- PROTECTED REGION ID(TestCppTango1022::read_attr_hardware) ENABLED START -----*/
    /* clang-format on */
    //	Add your own code
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::read_attr_hardware
}


//--------------------------------------------------------
/**
 *	Read attribute DoubleAttr related method
 *
 *
 *	Data type:	Tango::DevDouble
 *	Attr type:	Scalar
 */
//--------------------------------------------------------
void TestCppTango1022::read_DoubleAttr(Tango::Attribute &attr)
{
	DEBUG_STREAM << "TestCppTango1022::read_DoubleAttr(Tango::Attribute &attr) entering... " << std::endl;
	Tango::DevDouble	*att_value = get_DoubleAttr_data_ptr(attr.get_name());
	/*----- PROTECTED REGION ID(TestCppTango1022::read_DoubleAttr) ENABLED START -----*/
    /* clang-format on */
    //	Set the attribute value
    attr.set_value(att_value);
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::read_DoubleAttr
}
//--------------------------------------------------------
/**
 *	Write attribute DoubleAttr related method
 *
 *
 *	Data type:	Tango::DevDouble
 *	Attr type:	Scalar
 */
//--------------------------------------------------------
void TestCppTango1022::write_DoubleAttr(Tango::WAttribute &attr)
{
	DEBUG_STREAM << "TestCppTango1022::write_DoubleAttr(Tango::WAttribute &attr) entering... " << std::endl;
	//	Retrieve write value
	Tango::DevDouble	w_val;
	attr.get_write_value(w_val);
	/*----- PROTECTED REGION ID(TestCppTango1022::write_DoubleAttr) ENABLED START -----*/
    /* clang-format on */
    //	Add your own code
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::write_DoubleAttr
}
//--------------------------------------------------------
/**
 *	Method      : TestCppTango1022::add_dynamic_attributes()
 * Description:  Create the dynamic attributes if any
 *                for specified device.
 */
//--------------------------------------------------------
void TestCppTango1022::add_dynamic_attributes()
{
	//	Example to add dynamic attribute:
	//	Copy inside the following protected area to create instance(s) at startup.
	//	add_DoubleAttr_dynamic_attribute("MyDoubleAttrAttribute");

	/*----- PROTECTED REGION ID(TestCppTango1022::add_dynamic_attributes) ENABLED START -----*/
    /* clang-format on */
    //	Add your own code to create and add dynamic attributes if any
    try
    {
        std::cout << device_name << ": Adding Attr1" << std::endl;
        add_DoubleAttr_dynamic_attribute("Attr1");
    }
    catch(const Tango::DevFailed &e)
    {
        ERROR_STREAM << device_name << ": Exception while adding Attr1 attribute:" << std::endl;
        Tango::Except::print_exception(e);
    }
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::add_dynamic_attributes
}

//--------------------------------------------------------
/**
 *	Method      : TestCppTango1022::add_dynamic_commands()
 * Description:  Create the dynamic commands if any
 *                for specified device.
 */
//--------------------------------------------------------
void TestCppTango1022::add_dynamic_commands()
{
	/*----- PROTECTED REGION ID(TestCppTango1022::add_dynamic_commands) ENABLED START -----*/
    /* clang-format on */
    //	Add your own code to create and add dynamic commands if any
    /* clang-format off */
	/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::add_dynamic_commands
}

/*----- PROTECTED REGION ID(TestCppTango1022::namespace_ending) ENABLED START -----*/
/* clang-format on */
//	Additional Methods
/* clang-format off */
/*----- PROTECTED REGION END -----*/	//	TestCppTango1022::namespace_ending
} //	namespace

 // NOLINTEND(*)
