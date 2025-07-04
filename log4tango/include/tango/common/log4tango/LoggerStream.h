//
// LoggerStream.h
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

#ifndef _LOG4TANGO_LOGGER_STREAM_H
#define _LOG4TANGO_LOGGER_STREAM_H

#include <tango/common/log4tango/Portability.h>
#include <tango/common/log4tango/Level.h>
#include <sstream>
#include <tango/common/log4tango/LogSeparator.h>

namespace log4tango
{

//-----------------------------------------------------------------------------
// FORWARD DECLARATION
//-----------------------------------------------------------------------------
class Logger;
class LoggerStream;

//-----------------------------------------------------------------------------
// DEFINE WHAT IS A LS_TERMINATOR
//-----------------------------------------------------------------------------
typedef LoggerStream &(*ls_terminator)(LoggerStream &);

typedef class std::ios_base &(*StdIosFlag)(class std::ios_base &);

//-----------------------------------------------------------------------------
// class : LoggerStream
//-----------------------------------------------------------------------------
class LoggerStream
{
  public:
    struct SourceLocation
    {
        const char *file;
        int line;
    };

    /**
     * Construct a LoggerStream for given Logger with given level.
     * @param logger The logger this stream will send log messages to.
     * @param level The level the log messages will get or
     * Level::NOTSET to silently discard any streamed in messages.
     * @param filter The filter flag
     **/
    LoggerStream(Logger &logger, Level::Value level, bool filter = true);

    LoggerStream(const LoggerStream &) = delete;
    LoggerStream(LoggerStream &&);

    LoggerStream &operator=(const LoggerStream &) = delete;
    LoggerStream &operator=(LoggerStream &&) = delete;

    /**
     * Destructor for LoggerStream&)
     **/
    ~LoggerStream();

    /**
     * Returns the destination Logger for this stream.
     * @returns The Logger.
     **/
    Logger &get_logger() const
    {
        return _logger;
    }

    /**
     * Returns the level for this stream.
     * @returns The level.
     **/
    Level::Value get_level() const
    {
        return _level;
    }

    /**
     * Streams in a Initiator. Just a trick to return a ref to self.
     *
     * @returns A reference to itself.
     **/
    LoggerStream &operator<<(LOG4TANGO_UNUSED(const LogInitiator &i))
    {
        return *this;
    }

    /**
     * Streams in a Separator.Sends the contents of the stream buffer
     * to the Logger with set level and empties the buffer.
     *
     * @returns A reference to itself.
     **/
    LoggerStream &operator<<(LOG4TANGO_UNUSED(const LogSeparator &s))
    {
        flush();
        return *this;
    }

    /**
     * Streams in a ls_manipulator. Sends the contents of the stream buffer
     * to the Logger with set level and empties the buffer.
     *
     * @returns A reference to itself.
     **/
    LoggerStream &operator<<(LOG4TANGO_UNUSED(ls_terminator endoflog))
    {
        flush();
        return *this;
    }

    /**
     * Streams in a source code location.
     *
     * Source code location is stored internally and attached to the log event
     * when one is generated.
     *
     * @returns A reference to itself.
     **/
    LoggerStream &operator<<(SourceLocation source_location)
    {
        _source_location = source_location;
        return *this;
    }

    /**
     * Flush the contents of the stream buffer to the Logger and
     * empties the buffer.
     **/
    void flush();

    /**
     * Streams in a std stream manipulator.
     * @param _F the manipulator function
     * @returns a reference to self.
     **/
#ifdef WIN32
    inline LoggerStream &operator<<(std::ios_base &(_cdecl *_F)(std::ios_base &) )
    {
#else
    LoggerStream &operator<<(std::ios_base &(*_F)(std::ios_base &) )
    {
#endif
        if(_buffer != nullptr)
        {
            (*_F)(*(std::ios_base *) (_buffer));
        }
        return *this;
    }

    /**
     * Stream in arbitrary types and objects.
     * @param t The value or object to stream in.
     * @returns A reference to itself.
     **/
    template <typename T>
    LoggerStream &operator<<(const T &t)
    {
        if(_level != Level::OFF)
        {
            if(_buffer)
            {
                (*_buffer) << t;
            }
        }
        return *this;
    }

  private:
    Logger &_logger;

    Level::Value _level;

    bool _filter;

    std::ostringstream *_buffer;

    SourceLocation _source_location;
};

} // namespace log4tango

namespace std
{
//-- A dummy <ls_terminator>
inline log4tango::LoggerStream &endl(log4tango::LoggerStream &ls)
{
    return ls;
}
} // namespace std

#endif // _LOG4TANGO_LOGGER_STREAM_H
