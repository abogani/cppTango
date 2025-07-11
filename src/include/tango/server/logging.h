//+=============================================================================
//
// file :   Logging.h
//
// description :  TLS helper class (pseudo-singleton)
//
// project :    TANGO
//
// author(s) :    N.Leclercq - SOLEIL
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
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
//
//-=============================================================================

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <cstring>
#include <tango/server/tango_current_function.h>
#include <tango/common/tango_const.h>
#include <tango/server/tango_config.h>
#include <tango/common/log4tango/Logger.h>

namespace Tango::logging_detail
{
#ifdef _TG_WINDOWS_
constexpr auto PathSeparator = '\\';
#else
constexpr auto PathSeparator = '/';
#endif
inline const char *basename(const char *path)
{
    const auto *last_dir_sep = std::strrchr(path, PathSeparator);
    if(last_dir_sep == nullptr)
    {
        // No separator, the path does not contains directory components.
        return path;
    }

    // Return next character after the directory separator. Incrementing this
    // pointer is safe (even if the path ends with the separator character)
    // as long as the path is null-terminated.
    return last_dir_sep + 1;
}
} // namespace Tango::logging_detail

// A shortcut to the core logger ------------------------------
#define API_LOGGER Tango::Logging::get_core_logger()

#ifdef _TANGO_LIB //== compiling TANGO lib ====================

  #define TANGO_LOG                                                               \
      if(API_LOGGER)                                                              \
          API_LOGGER->get_stream(log4tango::Level::INFO, false)                   \
              << log4tango::_begin_log << log4tango::LoggerStream::SourceLocation \
          {                                                                       \
              ::Tango::logging_detail::basename(__FILE__), __LINE__               \
          }

#else

  #define TANGO_LOG std::cout

#endif //== compiling TANGO lib ===============================

// Map. TANGO_LOG_FATAL to FATAL level --------------------------------
#define TANGO_LOG_FATAL                                                                                \
    if(API_LOGGER && API_LOGGER->is_fatal_enabled())                                                   \
        API_LOGGER->fatal_stream() << log4tango::_begin_log << log4tango::LoggerStream::SourceLocation \
        {                                                                                              \
            ::Tango::logging_detail::basename(__FILE__), __LINE__                                      \
        }

// Map. TANGO_LOG_ERROR to ERROR level --------------------------------
#define TANGO_LOG_ERROR                                                                                \
    if(API_LOGGER && API_LOGGER->is_error_enabled())                                                   \
        API_LOGGER->error_stream() << log4tango::_begin_log << log4tango::LoggerStream::SourceLocation \
        {                                                                                              \
            ::Tango::logging_detail::basename(__FILE__), __LINE__                                      \
        }

// Map. TANGO_LOG_WARN to WARN level --------------------------------
#define TANGO_LOG_WARN                                                                                \
    if(API_LOGGER && API_LOGGER->is_warn_enabled())                                                   \
        API_LOGGER->warn_stream() << log4tango::_begin_log << log4tango::LoggerStream::SourceLocation \
        {                                                                                             \
            ::Tango::logging_detail::basename(__FILE__), __LINE__                                     \
        }

// Map. TANGO_LOG_INFO to INFO level --------------------------------
#define TANGO_LOG_INFO                                                                                \
    if(API_LOGGER && API_LOGGER->is_info_enabled())                                                   \
        API_LOGGER->info_stream() << log4tango::_begin_log << log4tango::LoggerStream::SourceLocation \
        {                                                                                             \
            ::Tango::logging_detail::basename(__FILE__), __LINE__                                     \
        }

// Map. TANGO_LOG_DEBUG to DEBUG level -------------------------------
#define TANGO_LOG_DEBUG                                                                                \
    if(API_LOGGER && API_LOGGER->is_debug_enabled())                                                   \
        API_LOGGER->debug_stream() << log4tango::_begin_log << log4tango::LoggerStream::SourceLocation \
        {                                                                                              \
            ::Tango::logging_detail::basename(__FILE__), __LINE__                                      \
        }

namespace Tango
{

class Util;
class Database;

TANGO_IMP extern log4tango::Logger *_core_logger;

class Logging
{
  public:
    /**
     * Initializes the Tango Logging service (TLS)
     **/
    static void init(const std::string &ds_name, int cmd_line_level, bool use_db, Database *db, Util *tg);
    /**
     * Shutdown the Tango Logging service
     **/
    static void cleanup();

    /**
     * Returns the core logger substitute
     **/
    static log4tango::Logger *get_core_logger();

    /**
     * Implementation of the DServer AddLoggingTarget Tango command
     **/
    static void add_logging_target(const DevVarStringArray *argin);

    /**
     * Implementation of the DServer AddLoggingTarget Tango command
     **/
    static void add_logging_target(log4tango::Logger *logger,
                                   const std::string &tg_type,
                                   const std::string &tg_name,
                                   int throw_exception = 1);

    /**
     * Implementation of the DServer AddLoggingTarget Tango command
     **/
    static void add_logging_target(log4tango::Logger *logger, const std::string &tg_type_name, int throw_exception = 1);

    /**
     * Implementation of the DServer RemoveLoggingTarget Tango command
     **/
    static void remove_logging_target(const DevVarStringArray *argin);

    /**
     * Implementation of the DServer GetLoggingTarget Tango command
     **/
    static DevVarStringArray *get_logging_target(const std::string &dev_name);

    /**
     * Implementation of the DServer SetLoggingLevel Tango command
     **/
    static void set_logging_level(const DevVarLongStringArray *argin);

    /**
     * Implementation of the DServer GetLoggingLevel Tango command
     **/
    static DevVarLongStringArray *get_logging_level(const DevVarStringArray *argin);

    /**
     * Implementation of the DServer StartLogging Tango command
     **/
    static void start_logging();

    /**
     * Implementation of the DServer StopLogging Tango command
     **/
    static void stop_logging();

    /**
     * Converts a Tango logging level into a log4tango level
     **/
    static log4tango::Level::Value tango_to_log4tango_level(Tango::LogLevel tango_level, bool throw_exception = true);

    /**
     * Converts a Tango logging level into a log4tango level
     **/
    static log4tango::Level::Value tango_to_log4tango_level(const std::string &tango_level,
                                                            bool throw_exception = true);

    /**
     * Converts a log4tango level into a Tango logging level
     **/
    static Tango::LogLevel log4tango_to_tango_level(log4tango::Level::Value log4tango_level);

    /**
     * Modify the rolling file threshold of the given Logger
     **/
    static void set_rolling_file_threshold(log4tango::Logger *logger, size_t rtf);

  private:
    /**
     *
     **/
    static void kill_zombie_appenders();

    /**
     *
     **/
    static std::string dev_to_file_name(const std::string &_dev_name);

    /**
     *
     **/
    static int get_target_type_and_name(const std::string &input, std::string &type, std::string &name);

    /**
     *
     **/
    static int create_log_dir(const std::string &full_path);

    /**
     * Protect the Logging class against instanciation and copy
     **/
    Logging(const Logging &);
    ~Logging();
    const Logging &operator=(const Logging &);

    /**
     *
     **/
    static std::string _log_path;

    /**
     *
     **/
    static size_t _rft;

    /**
     *
     **/
    static int _cmd_line_level;
};

} // namespace Tango

#endif // _LOGGING_H_
