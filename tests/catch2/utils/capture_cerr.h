#pragma once

#include <sstream>
#include <iostream>

struct CaptureCerr
{
    CaptureCerr() :
        old_buf(std::cerr.rdbuf(capture_stream.rdbuf()))
    {
    }

    ~CaptureCerr()
    {
        // Restore the original buffer
        std::cerr.rdbuf(old_buf);
    }

    // Get everything that was streamed into std::cerr
    std::string str() const
    {
        return capture_stream.str();
    }

  private:
    std::ostringstream capture_stream;
    std::streambuf *old_buf;
};
