//====================================================================================================================
//
// file :               BlackBox.h
//
// description :        Include for the BlackBox object. This class implements the black box objects which keep tracks
//						of all operation invoke on a device or attribute retrieved.
//						This black box is managed as a circular buffer
//
// project :            TANGO
//
// author(s) :          A.Gotz + E.Taurel
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//						European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//====================================================================================================================

#ifndef _BLACKBOX_H
#define _BLACKBOX_H

#include <tango/tango.h>
#ifdef _TG_WINDOWS_
#include <winsock.h>
#endif
#include <time.h>
#include <omniORB4/omniInterceptors.h>
#include <omniORB4/callDescriptor.h>
#include <chrono>

namespace Tango
{

#define		IP_ADDR_BUFFER_SIZE		80

// client call interceptor for remote calls 
// will be removed once omniORB 4.3 is adopted
CORBA::Boolean get_client_addr(omni::omniInterceptors::serverReceiveRequest_T::info_T &);

//==================================================================================================================
// client_call_interceptor: a omniORB server-side call interceptor dealing with client info & more
//==================================================================================================================
// Description:
// Calls can be intercepted on the server just before the upcall into application code. 
// This interceptor is registered with omniORB’s callDescriptor class, which is responsible for encapsulating 
// the state of a call. Unlike the transport-related serverReceiveRequest, serverSendReply and serverSendException 
// interceptors, the callDescriptor interceptor is invoked for all calls, even ones from colocated clients in the 
// same address space. The types used for the call interceptor are defined in include/omniORB4/callDescriptor.h. 
// The interceptor takes the form of a bare function with two parameters. The first parameter is a pointer to the 
// callDescriptor; the second is a pointer to omniServant, which is the base class of all servant classes. The 
// interceptor function must call the callDescriptor’s interceptedCall() method to pass on the call. This interception 
// point allows access to various parts of omniORB’s call machinery. The callDescriptor includes access to the operation 
// name and, if cast to the concrete subclass defined by the IDL compiler, the call arguments and return values too.
///==================================================================================================================
// Parameters:
// parameter d: a pointer to an omniORB callDescriptor (see include/omniORB4/callDescriptor.h for details)
// parameter s: a pointer an omniORB omniServant (base class of any Tango device dealing with CORBA low level stuffs)
///==================================================================================================================
// Comments:
// This interceptor works for both collocated and remote calls so that the client info is properly setup in any case.
// However, for the moment, we only use it for local calls. It will also be used for remote ones once we adopt ominitORB 
// 4.3. See also cppTango issue #865 for details.
///==================================================================================================================
void client_call_interceptor(omniCallDescriptor* d, omniServant* s);

class client_addr: public omni_thread::value_t
{
public:
    client_addr():client_ident(false),client_pid(0) {client_ip[0]='\0';::memset(java_ident,0,sizeof(DevULong64)<<1);}
	client_addr(const char *addr):client_ident(false),client_pid(0) {strcpy(client_ip,addr);}
	~client_addr() {}

	client_addr(const client_addr &);
	client_addr & operator=(const client_addr &);
	bool operator==(const client_addr &);
	bool operator!=(const client_addr &);

	bool				client_ident;
	char				client_ip[IP_ADDR_BUFFER_SIZE];
	LockerLanguage		client_lang;
	TangoSys_Pid		client_pid;
	std::string				java_main_class;
	DevULong64			java_ident[2];

	int client_ip_2_client_name(std::string &) const;
	friend std::ostream &operator<<(std::ostream &o_str,const client_addr &ca);
};

//==================================================================================================================
//
//			The BlackBoxElt class
//
// description :
//		Class to store all the necessary information which will be stored and returned to client on request
//
//==================================================================================================================

#define 	DEFAULT_ATTR_NB		10

enum BlackBoxElt_ReqType
{
	Req_Unknown,
	Req_Operation,
	Req_Attribute
};

enum BlackBoxElt_AttrType
{
	Attr_Unknown,
	Attr_Name,
	Attr_Description,
	Attr_State,
	Attr_Status,
	Attr_AdmName
};

enum BlackBoxElt_OpType
{
	Op_Unknown,
	Op_Command_inout,
	Op_BlackBox,
	Op_Ping,
	Op_Info,
	Op_Command_list,
	Op_Command,
	Op_Get_Attr_Config,
	Op_Set_Attr_Config,
	Op_Read_Attr,
	Op_Write_Attr,
	Op_Command_inout_2,
	Op_Command_list_2,
	Op_Command_2,
	Op_Get_Attr_Config_2,
	Op_Read_Attr_2,
	Op_Command_inout_history_2,
	Op_Read_Attr_history_2,
	Op_Read_Attr_3,
	Op_Write_Attr_3,
	Op_Read_Attr_history_3,
	Op_Info_3,
	Op_Get_Attr_Config_3,
	Op_Set_Attr_Config_3,
	Op_Read_Attr_history_4,
	Op_Command_inout_history_4,
	Op_Command_inout_4,
	Op_Write_Attr_4,
	Op_Read_Attr_4,
	Op_Set_Attr_Config_4,
	Op_Write_Read_Attributes_4,
	Op_Get_Attr_Config_5,
	Op_Set_Attr_Config_5,
	Op_Read_Attr_5,
	Op_Write_Read_Attributes_5,
	Op_Read_Attr_history_5,
	Op_Get_Pipe_Config_5,
	Op_Set_Pipe_Config_5,
	Op_Read_Pipe_5,
	Op_Write_Pipe_5,
	Op_Write_Read_Pipe_5
};

class BlackBoxElt
{
public:
	BlackBoxElt();
	~BlackBoxElt();

	BlackBoxElt_ReqType		req_type;
	BlackBoxElt_AttrType	attr_type;
	BlackBoxElt_OpType		op_type;
	std::string					cmd_name;
	std::vector<std::string>			attr_names;
	std::chrono::system_clock::time_point when;
	char					host_ip_str[IP_ADDR_BUFFER_SIZE];
	DevSource				source;

	bool					client_ident;
	LockerLanguage			client_lang;
	TangoSys_Pid			client_pid;
	std::string					java_main_class;
};

inline bool operator<(const BlackBoxElt &,const BlackBoxElt &)
{
	return true;
}

inline bool operator==(const BlackBoxElt &,const BlackBoxElt &)
{
	return true;
}

//==================================================================================================================
//
//			The BlackBox class
//
// description :
//		Class to implement the black box itself. This is mainly a vector of BlackBoxElt managed as a circular vector
//
//===================================================================================================================

class BlackBox
{
public:
	BlackBox();
	BlackBox(long);

	void insert_corba_attr(BlackBoxElt_AttrType);
	void insert_cmd(const char *,long vers=1,DevSource=Tango::DEV);
	void insert_attr(const Tango::DevVarStringArray &,long vers=1,DevSource=Tango::DEV);
	void insert_attr(const Tango::DevVarStringArray &,const ClntIdent &,long vers=1,DevSource=Tango::DEV);
	void insert_attr(const char *,const ClntIdent &,long);
	void insert_attr(const Tango::AttributeValueList &,long vers=1);
	void insert_attr(const Tango::AttributeValueList_4 &,const ClntIdent &,long vers);
	void insert_attr(const Tango::DevPipeData &,const ClntIdent &,long vers);
	void insert_wr_attr(const Tango::AttributeValueList_4 &,const Tango::DevVarStringArray &,const ClntIdent &,long vers);
	void insert_op(BlackBoxElt_OpType);
	void insert_op(BlackBoxElt_OpType,const ClntIdent &);

	void insert_cmd_nl(const char *,long,DevSource);
	void insert_cmd_cl_ident(const char *,const ClntIdent &,long vers=1,DevSource=Tango::DEV);
	void add_cl_ident(const ClntIdent &,client_addr *);
	void update_client_host(client_addr *);

	Tango::DevVarStringArray *read(long);

private:

	void inc_indexes();
	void get_client_host();
	void build_info_as_str(long);
	std::string timestamp_unix_to_str(const std::chrono::system_clock::time_point &);
	void add_source(long);
	void insert_op_nl(BlackBoxElt_OpType);
	void insert_attr_nl(const Tango::AttributeValueList &,long);
	void insert_attr_nl_4(const Tango::AttributeValueList_4 &);
	void insert_attr_wr_nl(const Tango::AttributeValueList_4 &,const Tango::DevVarStringArray &,long);

	std::vector<BlackBoxElt>	box;
	long				insert_elt;
	long				nb_elt;
	long				max_elt;

	omni_mutex			sync;

	std::string				elt_str;
};

} // End of Tango namespace

#endif /* _BLACKBOX_ */
