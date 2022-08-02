//=============================================================================
//
// file :               PollExt.h
//
// description :        Include for classes used by the method dedicated
//			   to fill the polling buffer for command or
//			   attributes.
//
// project :            TANGO
//
// author(s) :          E.Taurel
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
// $Revision: 15556
//
//=============================================================================

#ifndef _POLLEXT_H
#define _POLLEXT_H

#include <tango/tango.h>

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

#define __CHECK_DIM() \
	if ((x == 0) || (y == 0)) \
	{ \
		TANGO_THROW_EXCEPTION(API_AttrOptProp, "X or Y dimension cannot be 0 for image attribute"); \
	} \
	else \
		(void)0

#define __CHECK_DIM_X() \
	if (x == 0) \
	{ \
		TANGO_THROW_EXCEPTION(API_AttrOptProp, "X dimension cannot be 0 for spectrum or image attribute"); \
	} \
	else \
		(void)0

template <typename T>
class AttrData
{
public:
	const T 			*ptr;
	Tango::AttrQuality	qual;
	long				x;
	long				y;
	bool				release;
	DevErrorList		err;

	long				wr_x;
	long				wr_y;
	const T 			*wr_ptr;

	AttrData(const T *);
	AttrData(const T *,Tango::AttrQuality);
	AttrData(const T *,Tango::AttrQuality,bool);
	AttrData(const T *,const T *);
	AttrData(const T *,const T *,Tango::AttrQuality);
	AttrData(const T *,const T *,Tango::AttrQuality,bool);

// For spectrum

	AttrData(const T *,long);
	AttrData(const T *,long,Tango::AttrQuality);
	AttrData(const T *,long,Tango::AttrQuality,bool);
	AttrData(const T *,long,const T *,long);
	AttrData(const T *,long,const T *,long,Tango::AttrQuality);
	AttrData(const T *,long,const T *,long,Tango::AttrQuality,bool);

// For image

	AttrData(const T *,long,long);
	AttrData(const T *,long,long,Tango::AttrQuality);
	AttrData(const T *,long,long,Tango::AttrQuality,bool);
	AttrData(const T *,long,long,const T *,long,long);
	AttrData(const T *,long,long,const T *,long,long,Tango::AttrQuality);
	AttrData(const T *,long,long,const T *,long,long,Tango::AttrQuality,bool);

// For error

	AttrData(DevErrorList &e): ptr(NULL),x(0),y(0),release(false),err(e),wr_x(0),wr_y(0),wr_ptr(NULL) {}
};


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

/**
 * This class is used to store one element of an attribute history stack.
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

template <typename T>
class TimedAttrData:public Tango::AttrData<T>
{
public:

/**@name Miscellaneous constructors for scalar attribute
 */
//@{
/**
 * Create a new TimedAttrData object.
 *
 * The memory pointed to by the <i>p_data</i> parameter will not be freed
 * The attribute quality factor will be set to ATTR_VALID
 *
 * @param p_data Pointer to the attribute value
 * @param when The date
 */
	TimedAttrData(const T *p_data,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * The memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters will not be freed
 * The attribute quality factor will be set to ATTR_VALID
 *
 * @param p_data Pointer to the attribute value
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param when The date
 */
	TimedAttrData(const T *p_data,const T *p_wr_data,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object.
 *
 * The memory pointed to by the <i>p_data</i> parameter will not be freed
 *
 * @param p_data Pointer to the attribute value
 * @param qual The attribute quality factor
 * @param when The date
 */
	TimedAttrData(const T *p_data,Tango::AttrQuality qual,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * The memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters will not be freed
 *
 * @param p_data Pointer to the attribute value
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param qual The attribute quality factor
 * @param when The date
 */
	TimedAttrData(const T *p_data,const T *p_wr_data,Tango::AttrQuality qual,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object.
 *
 * @param p_data Pointer to the attribute value
 * @param qual The attribute quality factor
 * @param rel Set to true if the memory pointed to by the <i>p_data</i> parameter must be freed
 * @param when The date
 */
	TimedAttrData(const T *p_data,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * @param p_data Pointer to the attribute value
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param qual The attribute quality factor
 * @param rel Set to true if the memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters must be freed
 * @param when The date
 */
	TimedAttrData(const T *p_data,const T *p_wr_data,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when);
//@}


// For spectrum


/**@name Miscellaneous constructors for spectrum attribute
 */
//@{
/**
 * Create a new TimedAttrData object.
 *
 * The memory pointed to by the <i>p_data</i> parameter will not be freed
 * The attribute quality factor will be set to ATTR_VALID
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * The memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters will not be freed
 * The attribute quality factor will be set to ATTR_VALID
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param x_wr The attribute written part x length
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,const T *p_wr_data,long x_wr,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object.
 *
 * The memory pointed to by the <i>p_data</i> parameter will not be freed
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param qual The attribute quality factor
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,Tango::AttrQuality qual,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * The memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters will not be freed
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param x_wr The attribute written part x length
 * @param qual The attribute quality factor
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,const T *p_wr_data,long x_wr,Tango::AttrQuality qual,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object.
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param qual The attribute quality factor
 * @param rel Set to true if the memory pointed to by the <i>p_data</i> parameter must be freed
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param x_wr The attribute written part x length
 * @param qual The attribute quality factor
 * @param rel Set to true if the memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters must be freed
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,const T *p_wr_data,long x_wr,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when);

//@}


// For image


/**@name Miscellaneous constructors for image attribute
 */
//@{
/**
 * Create a new TimedAttrData object.
 *
 * The memory pointed to by the <i>p_data</i> parameter will not be freed
 * The attribute quality factor will be set to ATTR_VALID
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param y The attribute y length
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,long y,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * The memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters will not be freed
 * The attribute quality factor will be set to ATTR_VALID
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param y The attribute y length
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param x_wr The attribute written part x length
 * @param y_wr The attribute written part y length
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,long y,const T *p_wr_data,long x_wr,long y_wr,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object.
 *
 * The memory pointed to by the <i>p_data</i> parameter will not be freed
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param y The attribute y length
 * @param qual The attribute quality factor
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,long y,Tango::AttrQuality qual,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * The memory pointed to by the <i>p_data</i> and <i>p_wr_data</i> parameters will not be freed
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param y The attribute y length
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param x_wr The attribute written part x length
 * @param y_wr The attribute written part y length
 * @param qual The attribute quality factor
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,long y,const T *p_wr_data,long x_wr,long y_wr,Tango::AttrQuality qual,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object.
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param y The attribute y length
 * @param qual The attribute quality factor
 * @param rel Set to true if the memory pointed to by the <i>p_data</i> parameter must be freed
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,long y,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when);

/**
 * Create a new TimedAttrData object for a R/W attribute.
 *
 * @param p_data Pointer to the attribute value
 * @param x The attribute x length
 * @param y The attribute y length
 * @param p_wr_data Pointer to the written part of the attribute value
 * @param x_wr The attribute written part x length
 * @param y_wr The attribute written part y length
 * @param qual The attribute quality factor
 * @param rel Set to true if the memory pointed to by the <i>p_data</i> abd <i>p_wr_data</i> parameters must be freed
 * @param when The date
 */
	TimedAttrData(const T *p_data,long x,long y,const T *p_wr_data,long x_wr,long y_wr,Tango::AttrQuality qual,bool rel,const TangoTimestamp &when);
//@}



// For error


/**@name Miscellaneous constructors for errors
 */
//@{
/**
 * Create a new TimedAttrData object for errors.
 *
 * The created TimedAttrData is used to store attribute errors
 * in the attribute history stack
 *
 * @param errs The error stack
 * @param when The date
 */
	TimedAttrData(DevErrorList &errs,const TangoTimestamp &when);
//@}

/// @privatesection
	std::chrono::system_clock::time_point tp;

};


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

/**
 * This class is a used to pass an attribute value history when the user
 * directly fills the attribute polling buffer. Each element in this stack
 * will be used to store one element of the attribute polling buffer
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

template <typename T>
class AttrHistoryStack
{
public:

/**
 * Store a new element in the stack
 *
 * This method stores a new element in the stack
 *
 * @param elt The new element
 */
	void push(TimedAttrData<T> const &elt);

/**
 * Get stack depth
 *
 * @return The stack depth
 */
	size_t length() {return hist.size();}

/**
 * Reserve memory for stack elements
 *
 * @param nb The stack element number
 */
	void length(long nb) {hist.reserve(nb);}

/**
 * Clear the stack
 */
	void clear() {hist.clear();}

/**
 * Get stack data
 *
 * @return The stack itself
 */
	std::vector<TimedAttrData<T> > &get_data();

/// @privatesection
	AttrHistoryStack() {}

	std::vector<Tango::TimedAttrData<T> >	hist;
};


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

/**
 * This class is used to store one element of a command history stack.
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

template <typename T>
class TimedCmdData
{
public:

/**@name Constructors
 * Miscellaneous constructors
 */
//@{
/**
 * Create a new TimedCmdData object.
 *
 * The memory pointed to by the <i>p_data</i> parameter will not be freed
 *
 * @param p_data Pointer to the command result data
 * @param when The date
 */
	TimedCmdData(T *p_data,const TangoTimestamp &when);

/**
 * Create a new TimedCmdData object with memory management.
 *
 * @param p_data Pointer to the command result data
 * @param rel Set to true if the memory pointed to by the <i>p_data</i> parameter must be freed
 * @param when The date
 */
	TimedCmdData(T *p_data,bool rel,const TangoTimestamp &when);

/**
 * Create a new TimedCmdData object for errors.
 *
 * The created TimedCmdData is used to store command errors
 * in the command history stack
 *
 * @param errs The error stack
 * @param when The date
 */
	TimedCmdData(DevErrorList errs,const TangoTimestamp &when): ptr(NULL),err(errs),tp(when),release(false) {}
//@}

/// @privatesection

	T		        *ptr;
	DevErrorList	err;
	std::chrono::system_clock::time_point tp;
	bool		    release;

};


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


/**
 * This class is a used to pass a command result history when the user
 * directly fills the command polling buffer. Each element in this stack
 * will be used to store one element of the command polling buffer
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

template <typename T>
class CmdHistoryStack
{
public:

/**
 * Store a new element in the stack
 *
 * This method stores a new element in the stack
 *
 * @param elt The new element
 */
	void push(Tango::TimedCmdData<T> const &elt);

/**
 * Get stack depth
 *
 * @return The stack depth
 */
	size_t length() {return hist.size();}

/**
 * Reserve memory for stack elements
 *
 * @param nb The stack element number
 */
	void length(long nb) {hist.reserve(nb);}

/**
 * Clear the stack
 */
	void clear() {hist.clear();}

/**
 * Get stack data
 *
 * @return The stack itself
 */
	std::vector<TimedCmdData<T> > &get_data();

/// @privatesection

	CmdHistoryStack() {}

	std::vector<Tango::TimedCmdData<T> >	hist;
};


} // End of Tango namespace

#endif /* _POLLOBJ_ */
