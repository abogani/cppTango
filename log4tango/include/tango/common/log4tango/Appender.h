//
// Appender.h
//
// Copyright (C) :  2000 - 2002
//                    LifeLine Networks BV (www.lifeline.nl). All rights reserved.
//                    Bastiaan Bakker. All rights reserved.
//
//                    2004,2005,2006,2007,2008,2009,2010,2011,2012
//                    Synchrotron SOLEIL
//                    L'Orme des Merisiers
//                    Saint-Aubin - BP 48 - France
//
// This file is part of log4tango.
//
// Log4ango is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Log4tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Log4Tango.  If not, see <http://www.gnu.org/licenses/>.

#include <tango/common/log4tango/Portability.h>
#include <string>
#include <map>
#include <set>
#include <tango/common/log4tango/Level.h>
#include <tango/common/log4tango/Layout.h>
#include <tango/common/log4tango/LoggingEvent.h>

#ifndef _LOG4TANGO_APPENDER_H
  #define _LOG4TANGO_APPENDER_H

namespace log4tango
{

//-----------------------------------------------------------------------------
// class : Appender
//-----------------------------------------------------------------------------
class Appender
{
    friend class Logger;

  protected:
    /**
     * Constructor for Appender. Will only be used in
     * getAppender() (and in derived classes of course).
     * @param name The name of this Appender.
     **/
    Appender(const std::string &name);

    /**
     * Inform an appender that its Logger's logging level has changed.
     * The default implementation does nothing.
     *
     * @param new_level The new Logger's level.
     **/
    virtual void level_changed(Level::Value new_level);

  public:
    /**
     * Destructor for Appender.
     **/
    virtual ~Appender();

    /**
     * Log in Appender specific way. Returns -1 on error, 0 otherwise.
     * @param event  The LoggingEvent to log.
     **/
  #if defined(APPENDERS_HAVE_LEVEL_THRESHOLD) || defined(APPENDERS_HAVE_FILTERS)
    int append(const LoggingEvent &event);
  #else
    int append(const LoggingEvent &event)
    {
        return _append(event);
    }
  #endif

    /**
     * Reopens the output destination of this Appender, e.g. the logfile
     * or TCP socket.
     * @returns false if an error occurred during reopening, true otherwise.
     **/
    virtual bool reopen();

    /**
     * Release any resources allocated within the appender such as file
     * handles, network connections, etc.
     **/
    virtual void close() = 0;

    /**
     * Check if the appender uses a layout.
     *
     * @returns true if the appender implementation requires a layout.
     **/
    virtual bool requires_layout() const = 0;

    /**
     * Change the layout.
     **/
    virtual void set_layout(Layout *layout = nullptr) = 0;

    /**
     * Returns this appender name.
     **/
    const std::string &get_name() const
    {
        return _name;
    }

    /**
     * Check if the appender is valid (for instance the underlying connection is ok)
     * This default implementation always return true. Overload to define your own
     * behaviour.
     *
     * @returns true if the appender is valid, false otherwise.
     **/
    virtual bool is_valid() const;

  #ifdef APPENDERS_HAVE_LEVEL_THRESHOLD
    /**
     * Set the threshold level of this Appender. The Appender will not
     * appender LoggingEvents with a level lower than the threshold.
     * Use Level::NOTSET to disable level checking.
     * @param level The level to set.
     **/
    void set_level(Level::Value level);

    /**
     * Get the threshold level of this Appender.
     * @returns the threshold
     **/
    Level::Value get_level(void) const;
  #endif // APPENDERS_HAVE_LEVEL_THRESHOLD

  #ifdef APPENDERS_HAVE_FILTERS
    /**
     * Set a Filter for this appender.
     **/
    virtual void set_filter(Filter *filter);

    /**
     * Get the Filter for this appender.
     * @returns the filter, or nullptr if no filter has been set.
     **/
    virtual Filter *get_filter(void);
  #endif // APPENDERS_HAVE_FILTERS

  protected:
    /**
     * Log in Appender specific way. Subclasses of Appender should
     * implement this method to perform actual logging.
     * @param event  The LoggingEvent to log.
     **/
    virtual int _append(const LoggingEvent &event) = 0;

  private:
    /**
     * The appender name
     **/
    const std::string _name;

  #ifdef APPENDERS_HAVE_LEVEL_THRESHOLD
    /**
     * The appender logging level
     **/
    Level::Value _level;
  #endif

  #ifdef APPENDERS_HAVE_FILTERS
    /**
     * The appender filter list
     **/
    Filter *_filter;
  #endif
};

} // namespace log4tango

#endif // _LOG4TANGO_APPENDER_H
