//=============================================================================
//
// file :               SeqVec.cpp
//
// description :        Dource file for CORBA sequence printing functions.
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010
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
// $Log$
// Revision 3.10  2009/01/21 12:49:03  taurel
// - Change CopyRights for 2009
//
// Revision 3.9  2008/10/06 15:01:36  taurel
// - Changed the licensing info from GPL to LGPL
//
// Revision 3.8  2008/10/03 06:53:09  taurel
// - Add some licensing info in each files
//
// Revision 3.7  2008/06/10 07:52:15  taurel
// - Add code for the DevEncoded attribute data type
//
// Revision 3.6  2008/05/20 12:44:14  taurel
// - Commit after merge with release 7 branch
//
// Revision 3.5.2.1  2008/05/20 06:17:46  taurel
// - Last commit before merge with trunk
// (start the implementation of the new DevEncoded data type)
//
// Revision 3.5  2007/04/16 14:57:42  taurel
// - Added 3 new attributes data types (DevULong, DevULong64 and DevState)
// - Ported to omniORB4.1
// - Increased the MAX_TRANSFER_SIZE to 256 MBytes
// - Added a new filterable field in the archive event
//
// Revision 3.4  2007/03/06 08:19:03  taurel
// - Added 64 bits data types for 64 bits computer...
//
// Revision 3.3  2005/01/13 08:30:00  taurel
// - Merge trunk with Release_5_0 from brach Release_5_branch
//
// Revision 3.2.4.1  2004/09/15 06:47:17  taurel
// - Added four new types for attributes (boolean, float, unsigned short and unsigned char)
// - It is also possible to read state and status as attributes
// - Fix bug in Database::get_class_property() method (missing ends insertion)
// - Fix bug in admin device DevRestart command (device name case problem)
//
// Revision 3.2  2003/05/28 14:55:10  taurel
// Add the include (conditionally) of the include files generated by autoconf
//
// Revision 3.1  2003/04/03 15:24:10  taurel
// Added methods to print DeviceData, DeviceAttribute, DeviceDataHistory
// and DeviceAttributeHistory instance
//
// Revision 3.0  2003/03/25 16:44:07  taurel
// Many changes for Tango release 3.0 including
// - Added full logging features
// - Added asynchronous calls
// - Host name of clients now stored in black-box
// - Three serialization model in DS
// - Fix miscellaneous bugs
// - Ported to gcc 3.2
// - Added ApiUtil::cleanup() and destructor methods
// - Some internal cleanups
// - Change the way how TangoMonitor class is implemented. It's a recursive
//   mutex
//
// Revision 2.9  2003/03/11 17:55:57  nleclercq
// Switch from log4cpp to log4tango
//
// Revision 2.8  2002/12/16 12:07:32  taurel
// No change in code at all but only forgot th emost important line in
// list of updates in the previous release :
// - Change underlying ORB from ORBacus to omniORB
//
// Revision 2.7  2002/12/16 10:16:23  taurel
// - New method get_device_list() in Util class
// - Util::get_class_list takes DServer device into account
// - Util::get_device_by_name() takes DServer device into account
// - Util::get_device_list_by_class() takes DServer device into account
// - New parameter to the attribute::set_value() method to enable CORBA to free
// memory allocated for the attribute
//
// Revision 2.6  2002/10/17 07:43:07  taurel
// Fix bug in history stored by the polling thread :
// - We need one copy of the attribute data to build an history!!! It is true
// also for command which return data created by the DeviceImpl::create_xxx
// methods. Chnage in pollring.cpp/pollring.h/dserverpoll.cpp/pollobj.cpp
// and pollobj.h
//
// Revision 2.5  2002/10/15 11:27:20  taurel
// Fix bugs in device.cpp file :
// - Protect the state and status CORBA attribute with the device monitor
// Add the "TgLibVers" string as a #define in tango_config.h
//
// Revision 2.4  2002/08/12 15:06:55  taurel
// Several big fixes and changes
//   - Remove HP-UX specific code
//   - Fix bug in polling alogorithm which cause the thread to enter an infinite
//     loop (pollthread.cpp)
//   - For bug for Win32 device when trying to set attribute config
//     (attribute.cpp)
//
// Revision 2.3  2002/07/02 15:22:25  taurel
// Miscellaneous small changes/bug fixes for Tango CPP release 2.1.0
//     - classes reference documentation now generated using doxygen instead of doc++
//     - A little file added to the library which summarizes version number.
//       The RCS/CVS "ident" command will now tells you that release library x.y.z is composed
//       by C++ client classes set release a.b and C++ server classes set release c.d
//     - Fix incorrect field setting for DevFailed exception re-thrown from a CORBA exception
//     - It's now not possible to poll the Init command
//     - It's now possible to define a default class doc. per control system
//       instance (using property)
//     - The test done to check if attribute value has been set before it is
//       returned to caller is done only if the attribute quality is set to VALID
//     - The JTCInitialize object is now stored in the Util
//     - Windows specific : The tango.h file now also include winsock.h
//
// Revision 2.2  2002/04/30 10:50:42  taurel
// Don't check alarm on attribute if attribute quality factor is INVALID
//
// Revision 2.1  2002/04/29 12:24:04  taurel
// Fix bug in attribute::set_value method and on the check against min and max value when writing attributes
//
// Revision 2.0  2002/04/09 14:45:11  taurel
// See Tango WEB pages for list of changes
//
// Revision 1.6  2001/10/08 09:03:14  taurel
// See tango WEB pages for list of changes
//
// Revision 1.1  2001/07/04 12:27:11  taurel
// New methods re_throw_exception(). Read_attributes supports AllAttr mnemonic A new add_attribute()method in DeviceImpl class New way to define attribute properties New pattern to prevent full re-compile For multi-classes DS, it is now possible to use the Util::get_device_by_name() method in device constructor Adding << operator ovebloading Fix devie CORBA ref. number when device constructor sends an excep.
//
//
//=============================================================================

#if HAVE_CONFIG_H
#include <ac_config.h>
#endif

#include <seqvec.h>

namespace Tango
{

//
// These functions are not defined within the tango namespace because the VC++
// is not able to understand that the operator overloading functions is defined
// within a namespace x from th eoerand namespace (c++ or aCC is able to
// do it)
//

//=============================================================================
//
//			The sequence print facilities functions
//
// description :	These methods allow an easy way to print all sequence
//			element using the following syntax
//				cout << seq << endl;
//
//=============================================================================


ostream &operator<<(ostream &o,const DevVarCharArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << (short)v[i] << dec;
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarShortArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarLongArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarLong64Array &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarFloatArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarDoubleArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarBooleanArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = ";
		if (v[i] == true)
			o << "true";
		else
			o << "false";
			
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarUShortArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarULongArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarULong64Array &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarStringArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i].in();
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarStateArray &v)
{
	long nb_elt = v.length();
	for (long i = 0;i < nb_elt;i++)
	{
		o << "Element number [" << i << "] = " << v[i];
		if (i < (nb_elt - 1))
			o << '\n';
	}
	return o;
}

ostream &operator<<(ostream &o,const DevVarEncodedArray &v)
{
	long nb_elt = v.length();
	for (long loop = 0;loop < nb_elt;loop++)
	{
		o << "Encoding string = " << v[loop].encoded_format << endl;
		long nb_data_elt = v[loop].encoded_data.length();
		for (long i = 0;i < nb_data_elt;i++)
		{
			o << "Data element number [" << i << "] = " << (int)v[loop].encoded_data[i];
			if (i < (nb_data_elt - 1))
				o << '\n';
		}
		if (loop < (nb_elt - 1))
			o << '\n';
	}
	return o;
}
} // End of Tango namespace
