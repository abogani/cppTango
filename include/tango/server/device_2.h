//=============================================================================
//
// file :		Device.h
//
// description :	Include for the Device root classes.
//			Three classes are declared in this file :
//				The Device class
//				The DeviceClass class
//
// project :		TANGO
//
// author(s) :		A.Gotz + E.Taurel
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//						European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
//
// $Revision$
//
//=============================================================================

#ifndef _DEVICE_2_H
#define _DEVICE_2_H

#include <tango.h>

namespace Tango
{

class DeviceClass;

//=============================================================================
//
//			The Device_2Impl class
//
//
// description :	This class is derived directly from the Tango::Device_skel
//			class generated by CORBA. It represents the CORBA
//			servant which will be accessed by the client.
//			It implements all the methods
//			and attributes defined in the IDL interface for Device.
//
//=============================================================================

/**
 * Base class for all TANGO device since version 2.
 *
 * This class inherits from DeviceImpl class which itself inherits from
 * CORBA classes where all the network layer is implemented.
 * This class has been created since release 2 of Tango library where the IDL
 * Tango module has been modified in order to create a Device_2 interface
 * which inherits from the original Device interface
 *
 * $Author$
 * $Revision$
 *
 * @headerfile tango.h
 * @ingroup Server
 */

class Device_2Impl : public virtual POA_Tango::Device_2,
		     public DeviceImpl
{
public:

/**@name Constructors
 * Miscellaneous constructors */
//@{
/**
 * Constructs a newly allocated Device_2Impl object from its name.
 *
 * The device description field is set to <i>A Tango device</i>. The device
 * state is set to unknown and the device status is set to
 * <b>Not Initialised</b>
 *
 * @param 	device_class	Pointer to the device class object
 * @param	dev_name	The device name
 *
 */
	Device_2Impl(DeviceClass *device_class,string &dev_name);

/**
 * Constructs a newly allocated Device_2Impl object from its name and its description.
 *
 * The device
 * state is set to unknown and the device status is set to
 * <i>Not Initialised</i>
 *
 * @param 	device_class	Pointer to the device class object
 * @param	dev_name	The device name
 * @param	desc	The device description
 *
 */
	Device_2Impl(DeviceClass *device_class,string &dev_name,string &desc);

/**
 * Constructs a newly allocated Device_2Impl object from all its creation
 * parameters.
 *
 * The device is constructed from its name, its description, an original state
 * and status
 *
 * @param 	device_class	Pointer to the device class object
 * @param	dev_name	The device name
 * @param	desc 		The device description
 * @param	dev_state 	The device initial state
 * @param	dev_status	The device initial status
 *
 */
	Device_2Impl(DeviceClass *device_class,
	           string &dev_name,string &desc,
	           Tango::DevState dev_state,string &dev_status);

/**
 * Constructs a newly allocated Device_2Impl object from all its creation
 * parameters with some default values.
 *
 * The device is constructed from its name, its description, an original state
 * and status. This constructor defined default values for the description,
 * state and status parameters. The default device description is <i>A TANGO device</i>.
 * The default device state is <i>UNKNOWN</i> and the default device status
 * is <i>Not initialised</i>.
 *
 * @param 	device_class	Pointer to the device class object
 * @param	dev_name	The device name
 * @param	desc	The device desc
 * @param	dev_state 	The device initial state
 * @param	dev_status	The device initial status
 *
 */
	Device_2Impl(DeviceClass *device_class,
	           const char *dev_name,const char *desc = "A TANGO device",
	           Tango::DevState dev_state = Tango::UNKNOWN,
	           const char *dev_status = StatusNotSet);
//@}

/**@name Destructor
 * Only one desctructor is defined for this class */
//@{
/**
 * The device desctructor.
 */
	virtual ~Device_2Impl() {}
//@}


/**@name CORBA operation methods
 * Method defined to implement TANGO device CORBA operation */
//@{
/**
 * Execute a command.
 *
 * It's the master method executed when a "command_inout_2" CORBA operation is
 * requested by a client. It updates the device black-box, call the
 * TANGO command handler and returned the output Any
 *
 * @param in_cmd The command name
 * @param in_data The command input data packed in a CORBA Any
 * @param source The data source. This parameter is new in Tango release 2. It
 * allows a client to choose the data source between the device itself or the
 * data cache for polled command.
 * @return The command output data packed in a CORBA Any object
 * @exception DevFailed Re-throw of the exception thrown by the command_handler
 * method.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevFailed">here</a> to read
 * <b>DevFailed</b> exception specification
 */
	virtual CORBA::Any *command_inout_2(const char *in_cmd,
					    const CORBA::Any &in_data,
					    Tango::DevSource source);

/**
 * Get device command list.
 *
 * Invoked when the client request the command_list_query_2 CORBA operation.
 * It updates the device black box and returns an array of DevCmdInfo_2 object
 * with one object for each command.
 *
 * @return The device command list. One DevCmdInfo_2 is initialised for each
 * device command. Since Tango release 2, the command display level field has
 * been added to this structure
 */
	virtual Tango::DevCmdInfoList_2 *command_list_query_2();

/**
 * Get command info.
 *
 * Invoked when the client request the command_query_2 CORBA operation.
 * It updates the device black box and returns a DevCmdInfo_2 object for the
 * command with name passed
 * to the method as parameter.
 *
 * @param command The command name
 * @return A DevCmdInfo_2 initialised for the wanted command.
 * @exception DevFailed Thrown if the command does not exist.
 * Since Tango release 2, the command display level field has
 * been added to this structure.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevFailed">here</a> to read
 * <b>DevFailed</b> exception specification
 */
	virtual Tango::DevCmdInfo_2 *command_query_2(const char *command);

/**
 * Read attribute(s) value.
 *
 * Invoked when the client request the read_attributes_2 CORBA operation.
 * It returns to the client one AttributeValue structure for each wanted
 * attribute.
 *
 * @param names The attribute(s) name list
 * @param source The data source. This parameter is new in Tango release 2. It
 * allows a client to choose the data source between the device itself or the
 * data cache for polled attribute.
 * @return A sequence of AttributeValue structure. One structure is initialised
 * for each wanted attribute with the attribute value, the date and the attribute
 * value quality. Click <a href="../../../tango_idl/idl_html/_Tango.html#AttributeValue">here</a>
 * to read <b>AttributeValue</b> structure definition.
 * @exception DevFailed Thrown if the attribute does not exist.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevFailed">here</a> to read
 * <b>DevFailed</b> exception specification
 */
    	virtual Tango::AttributeValueList *read_attributes_2(const Tango::DevVarStringArray& names,
							     Tango::DevSource source);

/**
 * Get attribute(s) configuration.
 *
 * Invoked when the client request the get_attribute_config_2 CORBA operation.
 * It returns to the client one AttributeConfig_2 structure for each wanted
 * attribute. All the attribute properties value are returned in this
 * AttributeConfig_2 structure. Since Tango release 2, the attribute display
 * level field has been added to this structure.
 *
 * @param names The attribute(s) name list
 * @return A sequence of AttributeConfig_2 structure. One structure is initialised
 * for each wanted attribute. Click <a href="../../../tango_idl/idl_html/_Tango.html#AttributeConfig_2">here</a>
 * to read <b>AttributeConfig_2</b> structure specification.
 *
 * @exception DevFailed Thrown if the attribute does not exist.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevFailed">here</a> to read
 * <b>DevFailed</b> exception specification
 */
	virtual Tango::AttributeConfigList_2 *get_attribute_config_2(const Tango::DevVarStringArray& names);


/**
 * Read attribute value history.
 *
 * Invoked when the client request the read_attribute_history_2 CORBA operation.
 * This operation allows a client to retrieve attribute value history for
 * polled attribute. The depth of the history is limited to the depth of
 * the device server internal polling buffer.
 * It returns to the client one DevAttrHistory structure for each record.
 *
 * @param name The attribute name
 * @param n The record number.
 * @return A sequence of DevAttrHistory structure. One structure is initialised
 * for each record with the attribute value, the date and in case of the attribute
 * returns an error when it was read, the DevErrors data.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevAttrHistory">here</a>
 * to read <b>DevAttrHistory</b> structure definition.
 * @exception DevFailed Thrown if the attribute does not exist or is not polled.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevFailed">here</a> to read
 * <b>DevFailed</b> exception specification
 */
	virtual Tango::DevAttrHistoryList *read_attribute_history_2(const char* name,
								  DevLong n);

/**
 * Read command value history.
 *
 * Invoked when the client request the command_inout_history_2 CORBA operation.
 * This operation allows a client to retrieve command return value history for
 * polled command. The depth of the history is limited to the depth of
 * the device server internal polling buffer.
 * It returns to the client one DevCmdHistory structure for each record.
 *
 * @param command The command name
 * @param n The record number.
 * @return A sequence of DevCmdHistory structure. One structure is initialised
 * for each record with the command return value (in an Any), the date
 * and in case of the command returns an error when it was read, the
 * DevErrors data.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevCmdHistory">here</a>
 * to read <b>DevCmdHistory</b> structure definition.
 * @exception DevFailed Thrown if the attribute does not exist or is not polled.
 * Click <a href="../../../tango_idl/idl_html/_Tango.html#DevFailed">here</a> to read
 * <b>DevFailed</b> exception specification
 */

	virtual Tango::DevCmdHistoryList *command_inout_history_2(const char* command,
								DevLong n);
//@}

private:
	CORBA::Any *attr2cmd(AttributeValue_3 &,bool,bool);
	CORBA::Any *attr2cmd(AttributeValue_4 &,bool,bool);
	CORBA::Any *attr2cmd(AttributeValue_5 &,bool,bool);
	void Hist_32Hist(DevAttrHistoryList_3 *,DevAttrHistoryList *);
	void Polled_2_Live(long,Tango::AttrValUnion &,CORBA::Any &);
	void Polled_2_Live(long,CORBA::Any &,CORBA::Any &);

    class Device_2ImplExt
    {
    };

#ifdef HAS_UNIQUE_PTR
    unique_ptr<Device_2ImplExt>     ext_2;           // Class extension
#else
	Device_2ImplExt				    *ext_2;
#endif

	protected:
		Command *get_cmd_ptr(const string &cmd_name);
	};

} // End of Tango namespace

#endif // _DEVICE_H
