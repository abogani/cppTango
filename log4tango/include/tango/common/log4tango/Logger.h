//
// Logger.h
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

#ifndef _LOG4TANGO_LOGGER_H
#define _LOG4TANGO_LOGGER_H

#include <tango/common/log4tango/Portability.h>
#include <tango/common/log4tango/AppenderAttachable.h>
#include <tango/common/log4tango/LoggingEvent.h>
#include <tango/common/log4tango/Level.h>
#include <tango/common/log4tango/LoggerStream.h>

namespace log4tango
{

//-----------------------------------------------------------------------------
// class : Logger
//-----------------------------------------------------------------------------
class Logger : public AppenderAttachable
{
  public:
    /**
     * Constructor
     * @param name the fully qualified name of this Logger
     * @param level the level for this Logger. Defaults to Level::OFF
     **/
    Logger(const std::string &name, Level::Value level = Level::OFF);

    /**
     * Destructor
     **/
    ~Logger() override;

    /**
     * Return the logger name.
     * @returns The logger name.
     */
    const std::string &get_name() const
    {
        return _name;
    }

    /**
     * Set the level of this Logger (silently ignores invalid values)
     * @param level The level to set.
     **/
    void set_level(Level::Value level);

    /**
     * Returns the assigned Level, if any, for this Logger.
     * @return Level - the assigned Level, can be Level::NOTSET
     **/
    Level::Value get_level() const
    {
        return _level;
    }

    /**
     * Returns true if the level of the Logger is equal to
     * or higher than given level.
     * @param level The level to compare with.
     * @returns whether logging is enable for this level.
     **/
    bool is_level_enabled(Level::Value level) const
    {
        return _level >= level;
    }

    /**
     * Log a message with the specified level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param level The level of this log message.
     * @param string_format Format specifier for the log .
     * @param ... The arguments for string_format
     **/
    void log(const std::string &file, int line, Level::Value level, const char *string_format, ...);

    /**
     * Log a message with the specified level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param level The level of this log message.
     * @param message string to write in the log file
     **/
    void log(const std::string &file, int line, Level::Value level, const std::string &message)
    {
        if(is_level_enabled(level))
        {
            log_unconditionally(file, line, level, message);
        }
    }

    /**
     * Log a message with the specified level without level checking.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param level The level of this log message.
     * @param string_format Format specifier for the log .
     * @param ... The arguments for string_format
     **/
    void log_unconditionally(const std::string &file, int line, Level::Value level, const char *string_format, ...);

    /**
     * Log a message with the specified level without level checking.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param level The level of this log message.
     * @param message string to write in the log file
     **/
    void log_unconditionally(const std::string &file, int line, Level::Value level, const std::string &message);

    /**
     * Log a message with debug level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param string_format Format specifier for the log.
     * @param ... The arguments for string_format
     **/
    void debug(const std::string &file, int line, const char *string_format, ...);

    /**
     * Log a message with debug level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param message string to write in the log file
     **/
    void debug(const std::string &file, int line, const std::string &message)
    {
        if(is_level_enabled(Level::DEBUG))
        {
            log_unconditionally(file, line, Level::DEBUG, message);
        }
    }

    /**
     * Return true if the Logger will log messages with level DEBUG.
     * @returns Whether the Logger will log.
     **/
    bool is_debug_enabled() const
    {
        return is_level_enabled(Level::DEBUG);
    }

    /**
     * Return a LoggerStream with level DEBUG.
     * @returns The LoggerStream.
     **/
    LoggerStream debug_stream()
    {
        return LoggerStream(*this, Level::DEBUG, true);
    }

    /**
     * Log a message with info level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param string_format Format specifier for the log.
     * @param ... The arguments for string_format
     **/
    void info(const std::string &file, int line, const char *string_format, ...);

    /**
     * Log a message with info level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param message string to write in the log file
     **/
    void info(const std::string &file, int line, const std::string &message)
    {
        if(is_level_enabled(Level::INFO))
        {
            log_unconditionally(file, line, Level::INFO, message);
        }
    }

    /**
     * Return true if the Logger will log messages with level INFO.
     * @returns Whether the Logger will log.
     **/
    bool is_info_enabled() const
    {
        return is_level_enabled(Level::INFO);
    }

    /**
     * Return a LoggerStream with level INFO.
     * @returns The LoggerStream.
     **/
    LoggerStream info_stream()
    {
        return LoggerStream(*this, Level::INFO, true);
    }

    /**
     * Log a message with warn level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param string_format Format specifier for the log.
     * @param ... The arguments for string_format
     **/
    void warn(const std::string &file, int line, const char *string_format, ...);

    /**
     * Log a message with warn level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param message string to write in the log file
     **/
    void warn(const std::string &file, int line, const std::string &message)
    {
        if(is_level_enabled(Level::WARN))
        {
            log_unconditionally(file, line, Level::WARN, message);
        }
    }

    /**
     * Return true if the Logger will log messages with level WARN.
     * @returns Whether the Logger will log.
     **/
    bool is_warn_enabled() const
    {
        return is_level_enabled(Level::WARN);
    }

    /**
     * Return a LoggerStream with level WARN.
     * @returns The LoggerStream.
     **/
    LoggerStream warn_stream()
    {
        return LoggerStream(*this, Level::WARN, true);
    }

    /**
     * Log a message with error level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param string_format Format specifier for the log.
     * @param ... The arguments for string_format
     **/
    void error(const std::string &file, int line, const char *string_format, ...);

    /**
     * Log a message with error level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param message string to write in the log file
     **/
    void error(const std::string &file, int line, const std::string &message)
    {
        if(is_level_enabled(Level::ERROR))
        {
            log_unconditionally(file, line, Level::ERROR, message);
        }
    }

    /**
     * Return true if the Logger will log messages with level ERROR.
     * @returns Whether the Logger will log.
     **/
    bool is_error_enabled() const
    {
        return is_level_enabled(Level::ERROR);
    }

    /**
     * Return a LoggerStream with level ERROR.
     * @returns The LoggerStream.
     **/
    LoggerStream error_stream()
    {
        return LoggerStream(*this, Level::ERROR, true);
    }

    /**
     * Log a message with fatal level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param string_format Format specifier for the log.
     * @param ... The arguments for string_format
     **/
    void fatal(const std::string &file, int line, const char *string_format, ...);

    /**
     * Log a message with fatal level.
     * @param file File path of this log message.
     * @param line Line number of this log message.
     * @param message string to write in the log file
     **/
    void fatal(const std::string &file, int line, const std::string &message)
    {
        if(is_level_enabled(Level::FATAL))
        {
            log_unconditionally(file, line, Level::FATAL, message);
        }
    }

    /**
     * Return true if the Logger will log messages with level FATAL.
     * @returns Whether the Logger will log.
     **/
    bool is_fatal_enabled() const
    {
        return is_level_enabled(Level::FATAL);
    }

    /**
     * Return a LoggerStream with level FATAL.
     * @returns The LoggerStream.
     **/
    LoggerStream fatal_stream()
    {
        return LoggerStream(*this, Level::FATAL, true);
    }

    /**
     * Return a LoggerStream with given Level.
     * @param level The Level of the LoggerStream.
     * @param filter The filter flag
     * @returns The requested LoggerStream.
     **/
    LoggerStream get_stream(Level::Value level, bool filter = true)
    {
        return LoggerStream(*this, level, filter);
    }

  protected:
    /**
     * Call the appenders.
     *
     * @param event the LogginEvent to log.
     **/
    void call_appenders(const LoggingEvent &event);

  private:
    /** The name of this logger. */
    const std::string _name;

    /** The assigned level of this logger. */
    Level::Value _level;

    /* prevent copying and assignment */
    Logger(const Logger &);
    Logger &operator=(const Logger &);
};

} // namespace log4tango

#endif // _LOG4TANGO_LOGGER_H
