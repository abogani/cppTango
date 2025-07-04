#include <tango/common/utils/assert.h>
#include <tango/server/logging.h>

#include <iostream>
#include <exception>

namespace Tango::detail
{
void assertion_failure(const char *file, int line, const char *func, const char *msg)
{
    // When called from the TANGO_ASSERT macro these nullptr checks never
    // fire, however, on the off chance that someone calls this function in a
    // different context, we ensure that this function does not crash and at least
    // reports something before we terminate.

    if(file == nullptr)
    {
        file = "unknown";
    }

    if(func == nullptr)
    {
        func = "unknown";
    }

    if(msg == nullptr)
    {
        msg = "";
    }

    const char *basename = ::Tango::logging_detail::basename(file);

    if(API_LOGGER && API_LOGGER->is_fatal_enabled())
    {
        using SourceLocation = log4tango::LoggerStream::SourceLocation;

        API_LOGGER->fatal_stream() << log4tango::_begin_log << SourceLocation{basename, line} << func << ": " << msg
                                   << log4tango::_end_log;
    }

    std::cerr << basename << ":" << line << " " << func << ": " << msg << "\n";

    std::terminate();
}
} // namespace Tango::detail
