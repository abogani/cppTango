#include "utils/utils.h"

#include <catch2/catch_translate_exception.hpp>
#include <tango/tango.h>

#include <sstream>

CATCH_TRANSLATE_EXCEPTION(const Tango::DevFailed &ex)
{
    std::stringstream ss;
    Tango::Except::print_exception(ex, ss);
    return ss.str();
}
