//====================================================================================================================
//
// file :               PollExt.tpp
//
// description :        Template method decleration for classes used by the method dedicated
//			   			to fill the polling buffer for command or attributes.
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2013,2014,2015
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
//===================================================================================================================

#ifndef _POLLEXT_TPP
#define _POLLEXT_TPP

#include <tango/tango.h>

#ifdef _TG_WINDOWS_
#include <sys/types.h>
#endif /* _TG_WINDOWS_ */

namespace Tango
{

//=============================================================================
//
//			Attribute related class
//
// description :	These classes are used when the user want to fill
//		       attribute polling buffer
//
//=============================================================================

//=============================================================================
//
//			The AttrData class
//
//
// description : 	This class is used to store all the data needed to build
//			an attribute value.
//
//=============================================================================


template <typename T>
inline AttrData<T>::AttrData(const T *p): ptr(p),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	qual = Tango::ATTR_VALID;
	x = 1;
	y = 0;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,Tango::AttrQuality q): ptr(p),qual(q),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	x = 1;
	y = 0;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,Tango::AttrQuality q,bool rel): ptr(p),qual(q),release(rel),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	x = 1;
	y = 0;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,const T *wr_p): ptr(p),wr_y(0),wr_ptr(wr_p)
{
	qual = Tango::ATTR_VALID;
	x = 1;
	y = 0;
	wr_x =1 ;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,const T *wr_p,Tango::AttrQuality q): ptr(p),qual(q),wr_y(0),wr_ptr(wr_p)
{
	x = 1;
	y = 0;
	wr_x = 1;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,const T *wr_p,Tango::AttrQuality q,bool rel): ptr(p),qual(q),release(rel),wr_y(0),wr_ptr(wr_p)
{
	x = 1;
	y = 0;
	wr_x = 1;
}

// For spectrum

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb): ptr(p),x(nb),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	__CHECK_DIM_X();
	qual = Tango::ATTR_VALID;
	y = 0;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,Tango::AttrQuality q): ptr(p),qual(q),x(nb),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	__CHECK_DIM_X();
	y = 0;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,Tango::AttrQuality q,bool rel): ptr(p),qual(q),x(nb),release(rel),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	__CHECK_DIM_X();
	y = 0;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,const T *wr_p,long wr_nb): ptr(p),x(nb),wr_x(wr_nb),wr_y(0),wr_ptr(wr_p)
{
	__CHECK_DIM_X();
	qual = Tango::ATTR_VALID;
	y = 0;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,const T *wr_p,long wr_nb,Tango::AttrQuality q): ptr(p),qual(q),x(nb),wr_x(wr_nb),wr_y(0),wr_ptr(wr_p)
{
	__CHECK_DIM_X();
	y = 0;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,const T *wr_p,long wr_nb,Tango::AttrQuality q,bool rel): ptr(p),qual(q),x(nb),release(rel),wr_x(wr_nb),wr_y(0),wr_ptr(wr_p)
{
	__CHECK_DIM_X();
	y = 0;
}

// For image

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,long nb2): ptr(p),x(nb),y(nb2),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	__CHECK_DIM();
	qual = Tango::ATTR_VALID;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,long nb2,Tango::AttrQuality q): ptr(p),qual(q),x(nb),y(nb2),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	__CHECK_DIM();
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,long nb2,Tango::AttrQuality q,bool rel): ptr(p),qual(q),
									     x(nb),y(nb2),release(rel),wr_x(0),wr_y(0),wr_ptr(NULL)
{
	__CHECK_DIM();
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,long nb2,const T *wr_p,long wr_nb,long wr_nb2): ptr(p),x(nb),y(nb2),wr_x(wr_nb),wr_y(wr_nb2),wr_ptr(wr_p)
{
	__CHECK_DIM();
	qual = Tango::ATTR_VALID;
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,long nb2,const T *wr_p,long wr_nb,long wr_nb2,Tango::AttrQuality q): ptr(p),qual(q),x(nb),y(nb2),wr_x(wr_nb),wr_y(wr_nb2),wr_ptr(wr_p)
{
	__CHECK_DIM();
	release = false;
}

template <typename T>
inline AttrData<T>::AttrData(const T *p,long nb,long nb2,const T *wr_p,long wr_nb,long wr_nb2,Tango::AttrQuality q,bool rel): ptr(p),qual(q),
									     x(nb),y(nb2),release(rel),wr_x(wr_nb),wr_y(wr_nb2),wr_ptr(wr_p)
{
	__CHECK_DIM();
}

//=============================================================================
//
//			The TimedAttrData class
//
//
// description : 	This class inherits from the AttrData class and adds
//			a date to all the data contains in the AttrData
//			class
//
//=============================================================================

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,const TangoTimestamp &when): AttrData<T>(p_data),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,const T *p_wr_data,const TangoTimestamp &when): AttrData<T>(p_data,p_wr_data),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,Tango::AttrQuality qual,const TangoTimestamp &when): AttrData<T>(p_data,qual),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,TANGO_UNUSED(const T *p_wr_data),Tango::AttrQuality qual,const TangoTimestamp &when): AttrData<T>(p_data,qual),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when): AttrData<T>(p_data,qual,rel),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,const T *p_wr_data,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when): AttrData<T>(p_data,p_wr_data,qual,rel),tp(when)
{}

// For spectrum
template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,const TangoTimestamp &when): AttrData<T>(p_data,x),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,const T *p_wr_data,long x_wr,const TangoTimestamp &when): AttrData<T>(p_data,x,p_wr_data,x_wr),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,Tango::AttrQuality qual,const TangoTimestamp &when): AttrData<T>(p_data,x,qual),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,const T *p_wr_data,long x_wr,Tango::AttrQuality qual,const TangoTimestamp &when): AttrData<T>(p_data,x,p_wr_data,x_wr,qual),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when): AttrData<T>(p_data,x,qual,rel),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,const T *p_wr_data,long x_wr,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when): AttrData<T>(p_data,x,p_wr_data,x_wr,qual,rel),tp(when)
{}

// For image

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,long y,const TangoTimestamp &when): AttrData<T>(p_data,x,y),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,long y,const T *p_wr_data,long x_wr,long y_wr,const TangoTimestamp &when): AttrData<T>(p_data,x,y,p_wr_data,x_wr,y_wr),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,long y,Tango::AttrQuality qual,const TangoTimestamp &when): AttrData<T>(p_data,x,y,qual),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,long y,const T *p_wr_data,long x_wr,long y_wr,Tango::AttrQuality qual,const TangoTimestamp &when): AttrData<T>(p_data,x,y,p_wr_data,x_wr,y_wr,qual),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,long y,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when): AttrData<T>(p_data,x,y,qual,rel),tp(when)
{}

template <typename T>
inline TimedAttrData<T>::TimedAttrData(const T *p_data,long x,long y,const T *p_wr_data,long x_wr,long y_wr,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when): AttrData<T>(p_data,x,y,p_wr_data,x_wr,y_wr,qual,rel),tp(when)
{}

// For error

template <typename T>
inline TimedAttrData<T>::TimedAttrData(DevErrorList &errs,const TangoTimestamp &when): AttrData<T>(errs),tp(when)
{}

//=============================================================================
//
//			The AttrHistoryStack class
//
//
// description : 	This class is simply a wrapper above a vector of
//			TimedAttrData class. It is  used to pass an attribute
//			value history which will be stored in the polling
//			buffer
//
//=============================================================================



template <typename T>
inline void AttrHistoryStack<T>::push(TimedAttrData<T> const &elt)
{
	hist.push_back(elt);
}

template <typename T>
inline std::vector<TimedAttrData<T> > &AttrHistoryStack<T>::get_data()
{
	return hist;
}


//=============================================================================
//
//			Command related class
//
// description :	These classes are used when the user want to fill
//		       command polling buffer
//
//=============================================================================


//=============================================================================
//
//			The TimedCmdData class
//
//
// description : 	This class is used to store all the data needed to build
//			a command value plus a date.
//
//=============================================================================

template <typename T>
inline TimedCmdData<T>::TimedCmdData(T *p_data,const TangoTimestamp &when): ptr(p_data),tp(when),release(false)
{}

template <typename T>
inline TimedCmdData<T>::TimedCmdData(T *p_data,bool rel,const TangoTimestamp &when): ptr(p_data),tp(when),release(rel)
{}

//=============================================================================
//
//			The CmdHistoryStack class
//
//
// description : 	This class is simply a wrapper above a vector of
//			TimedCmdData class. It is  used to pass a command
//			value history which will be stored in the polling
//			buffer
//
//=============================================================================


template <typename T>
inline void CmdHistoryStack<T>::push(Tango::TimedCmdData<T> const &elt)
{
	hist.push_back(elt);
}

template <typename T>
inline std::vector<TimedCmdData<T> > & CmdHistoryStack<T>::get_data()
{
	return hist;
}


} // End of Tango namespace

#endif /* _POLLOBJ_ */
