//
// LoggingEvent.hh
//
// Copyright (C) :  2000 - 2002
//					LifeLine Networks BV (www.lifeline.nl). All rights reserved.
//					Bastiaan Bakker. All rights reserved.
//
//					2004,2005,2006,2007,2008,2009,2010,2011,2012
//					Synchrotron SOLEIL
//                	L'Orme des Merisiers
//                	Saint-Aubin - BP 48 - France
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

#ifndef _LOG4TANGO_LOGGINGEVENT_H
#define _LOG4TANGO_LOGGINGEVENT_H

#include <tango/common/log4tango/Portability.hh>
#include <string>
#include <thread>
#include <chrono>

#include <tango/common/log4tango/Level.hh>

namespace log4tango {

/**
 * The internal representation of logging events. When a affirmative
 * logging decision is made a <code>LoggingEvent</code> instance is
 * created. This instance is passed around the different log4tango
 * components.
 *
 * <p>This class is of concern to those wishing to extend log4tango.
 **/
//-----------------------------------------------------------------------------
// struct : LoggingEvent
//-----------------------------------------------------------------------------
struct LoggingEvent
{
public:
  /**
   * Instantiate a LoggingEvent from the supplied parameters.
   *
   * <p>Except <code>timeStamp</code> all the other fields of
   * <code>LoggingEvent</code> are filled when actually needed.
   * <p>
   * @param logger The logger of this event.
   * @param message  The message of this event.
   * @param level The level of this event.
   * @param file_path The file path where this event was generated.
   * @param line_number The line number where this event was generated.
   **/
  LoggingEvent(const std::string& logger,
               const std::string& message,
               Level::Value level,
               const std::string& file_path,
               int line_number);

  /** Copy constructor */
  LoggingEvent(const LoggingEvent& event);

  /** The logger name. */
  const std::string logger_name;

  /** The application supplied message of logging event. */
  const std::string message;


  /** Level of logging event. */
  Level::Value level;

  /** Name of thread in which this logging event was generated */
  std::string thread_name;

  /** id of thread in which this logging event was generated */
  std::thread::id thread_id;

  /** The number of seconds elapsed since the epoch
      (1/1/1970 00:00:00 UTC) until logging event was created. */
  std::chrono::system_clock::time_point timestamp;

  /** File path where the event was generated */
  const std::string file_path;

  /** Line number where the event was generated */
  const int line_number;

private:
  /** Prevent implicit copy */
  const LoggingEvent& operator= (const LoggingEvent&);
};

} // namespace log4tango

#endif // _LOG4TANGO_LOGGINGEVENT_H
