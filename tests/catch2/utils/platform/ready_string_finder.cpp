#include "utils/platform/ready_string_finder.h"

#include <sstream>
#include <cstddef>

namespace TangoTest::platform
{
ReadyStringFinder::ReadyStringFinder(const std::string &filename) :
    m_file{filename}
{
    if(m_file.bad())
    {
        std::stringstream ss;
        ss << "failed to open \"" << filename << "\"";
        throw std::runtime_error(ss.str());
    }
}

bool ReadyStringFinder::check_for_ready_string(const std::string &ready_string)
{
    while(true)
    {
        std::string line;
        std::getline(m_file, line);

        // Didn't hit a \n, so "put the line back" so we can try again
        // next time.
        if(m_file.eof())
        {
            auto offset = -static_cast<ptrdiff_t>(line.size());
            // clear needs to happen here.
            // The reason being that on macOS seekg fails when
            // fail bit is set. We hope that it is a libc++ thing
            // and not a implementation detail that could bite us
            // again when implementing the Windows version.
            m_file.clear();
            m_file.seekg(offset, std::ios_base::cur);
            return false;
        }

        if(m_file.fail())
        {
            throw std::runtime_error("getline() failed");
        }

        if(line.find(ready_string) != std::string::npos)
        {
            return true;
        }
    }
}
} // namespace TangoTest::platform
