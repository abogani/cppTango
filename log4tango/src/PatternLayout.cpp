//
// PatternLayout.cpp
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

#include <tango/common/log4tango/PatternLayout.h>
#include <tango/common/log4tango/Level.h>

#include <sstream>

#include <iomanip>
#include <ctime>
#include <cmath>
#include <chrono>

namespace
{
const auto LogStartTime = std::chrono::system_clock::now();
}

namespace log4tango
{

struct StringLiteralComponent : public PatternLayout::PatternComponent
{
    StringLiteralComponent(const std::string &literal) :
        _literal(literal)
    {
        // no-op
    }

    void append(std::ostringstream &out, const LoggingEvent & /*event*/) override
    {
        out << _literal;
    }

  private:
    std::string _literal;
};

struct LoggerNameComponent : public PatternLayout::PatternComponent
{
    LoggerNameComponent(std::string specifier)
    {
        if(specifier == "")
        {
            _precision = -1;
        }
        else
        {
            std::istringstream s(specifier);
            s >> _precision;
        }
    }

    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        if(_precision == -1)
        {
            out << event.logger_name;
        }
        else
        {
            std::string::size_type begin = std::string::npos;
            for(int i = 0; i < _precision; i++)
            {
                begin = event.logger_name.rfind('.', begin - 2);
                if(begin == std::string::npos)
                {
                    begin = 0;
                    break;
                }
                begin++;
            }
            out << event.logger_name.substr(begin);
        }
    }

  private:
    int _precision;
};

struct MessageComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        out << event.message;
    }
};

struct LevelComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        out << Level::get_name(event.level);
    }
};

struct ThreadNameComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        out << event.thread_name;
    }
};

struct ThreadIdComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        out << event.thread_id;
    }
};

struct ProcessorTimeComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent & /*event*/) override
    {
        out << ::clock();
    }
};

struct FilePathComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        out << event.file_path;
    }
};

struct LineNumberComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        out << event.line_number;
    }
};

struct TimeStampComponent : public PatternLayout::PatternComponent
{
    static const char *const FORMAT_ISO8601;
    static const char *const FORMAT_ABSOLUTE;
    static const char *const FORMAT_DATE;

    TimeStampComponent(std::string timeFormat)
    {
        if((timeFormat == "") || (timeFormat == "ISO8601"))
        {
            timeFormat = FORMAT_ISO8601;
        }
        else if(timeFormat == "ABSOLUTE")
        {
            timeFormat = FORMAT_ABSOLUTE;
        }
        else if(timeFormat == "DATE")
        {
            timeFormat = FORMAT_DATE;
        }
        std::string::size_type pos = timeFormat.find("%l");
        if(pos == std::string::npos)
        {
            _printFraction = false;
            _timeFormat1 = timeFormat;
        }
        else
        {
            _printFraction = true;
            _timeFormat1 = timeFormat.substr(0, pos);
            _timeFormat2 = timeFormat.substr(pos + 2);
        }
    }

    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        std::string timeFormat;
        if(_printFraction)
        {
            constexpr int precision = 6;
            using Duration = std::chrono::microseconds;
            auto fraction =
                std::chrono::duration_cast<Duration>(event.timestamp.time_since_epoch()) % std::chrono::seconds(1);
            std::ostringstream formatStream;
            formatStream << _timeFormat1 << std::setw(precision) << std::setfill('0') << fraction.count()
                         << _timeFormat2;
            timeFormat = formatStream.str();
        }
        else
        {
            timeFormat = _timeFormat1;
        }
        char formatted[100];
        std::time_t t = std::chrono::system_clock::to_time_t(event.timestamp);
        std::tm *currentTime = std::localtime(&t);
        std::strftime(formatted, sizeof(formatted), timeFormat.c_str(), currentTime);
        out << formatted;
    }

  private:
    std::string _timeFormat1;
    std::string _timeFormat2;
    bool _printFraction;
};

const char *const TimeStampComponent::FORMAT_ISO8601 = "%Y-%m-%dT%H:%M:%S,%l%z";
const char *const TimeStampComponent::FORMAT_ABSOLUTE = "%H:%M:%S,%l";
const char *const TimeStampComponent::FORMAT_DATE = "%d %b %Y %H:%M:%S,%l";

struct SecondsSinceEpochComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        out << std::chrono::duration_cast<std::chrono::seconds>(event.timestamp.time_since_epoch()).count();
    }
};

struct MillisSinceEpochComponent : public PatternLayout::PatternComponent
{
    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        auto delta = event.timestamp - LogStartTime;
        out << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
    }
};

struct FormatModifierComponent : public PatternLayout::PatternComponent
{
    FormatModifierComponent(PatternLayout::PatternComponent *component, int minWidth, int maxWidth) :
        _component(component),
        _minWidth(minWidth < 0 ? -minWidth : minWidth),
        _maxWidth(maxWidth),
        _alignLeft(minWidth < 0)
    {
    }

    ~FormatModifierComponent() override
    {
        delete _component;
    }

    void append(std::ostringstream &out, const LoggingEvent &event) override
    {
        std::ostringstream s;
        _component->append(s, event);
        std::string msg = s.str();
        if(_maxWidth > 0)
        {
            msg.erase(_maxWidth);
        }
        auto fillCount = _minWidth - msg.length();
        if(fillCount > 0)
        {
            if(_alignLeft)
            {
                out << msg << std::string(fillCount, ' ');
            }
            else
            {
                out << std::string(fillCount, ' ') << msg;
            }
        }
        else
        {
            out << msg;
        }
    }

  private:
    PatternLayout::PatternComponent *_component;
    int _minWidth;
    int _maxWidth;
    bool _alignLeft;
};

const char *PatternLayout::BASIC_CONVERSION_PATTERN = "%d %p (%F:%L) %c %m";

PatternLayout::PatternLayout()
{
    set_conversion_pattern(BASIC_CONVERSION_PATTERN);
}

PatternLayout::~PatternLayout()
{
    clear_conversion_pattern();
}

void PatternLayout::clear_conversion_pattern()
{
    for(auto i = _components.begin(); i != _components.end(); ++i)
    {
        delete(*i);
    }
    _components.clear();
    _conversionPattern = "";
}

int PatternLayout::set_conversion_pattern(const std::string &conversionPattern)
{
    std::istringstream conversionStream(conversionPattern);
    std::string literal;

    char ch;
    PatternLayout::PatternComponent *component = nullptr;
    int minWidth = 0;
    int maxWidth = 0;
    clear_conversion_pattern();
    while(conversionStream.get(ch))
    {
        if(ch == '%')
        {
            // readPrefix;
            {
                char ch2;
                conversionStream.get(ch2);
                if((ch2 == '-') || ((ch2 >= '0') && (ch2 <= '9')))
                {
                    conversionStream.putback(ch2);
                    conversionStream >> minWidth;
                    conversionStream.get(ch2);
                }
                if(ch2 == '.')
                {
                    conversionStream >> maxWidth;
                }
                else
                {
                    conversionStream.putback(ch2);
                }
            }
            if(!conversionStream.get(ch))
            {
                return -1;
            }
            std::string specPostfix;
            // read postfix
            {
                char ch2;
                if(conversionStream.get(ch2))
                {
                    if(ch2 == '{')
                    {
                        while(conversionStream.get(ch2) && (ch2 != '}'))
                        {
                            specPostfix += ch2;
                        }
                    }
                    else
                    {
                        conversionStream.putback(ch2);
                    }
                }
            }
            switch(ch)
            {
            case '%':
                literal += ch;
                break;
            case 'm':
                component = new MessageComponent();
                break;
            case 'n':
            {
                std::ostringstream endline;
                endline << std::endl;
                literal += endline.str();
            }
            break;
            case 'c':
                component = new LoggerNameComponent(specPostfix);
                break;
            case 'd':
                component = new TimeStampComponent(specPostfix);
                break;
            case 'p':
                component = new LevelComponent();
                break;
            case 't':
                component = new ThreadIdComponent();
                break;
            case 'T':
                component = new ThreadNameComponent();
                break;
            case 'r':
                component = new MillisSinceEpochComponent();
                break;
            case 'R':
                component = new SecondsSinceEpochComponent();
                break;
            case 'u':
                component = new ProcessorTimeComponent();
                break;
            case 'F':
                component = new FilePathComponent();
                break;
            case 'L':
                component = new LineNumberComponent();
                break;
            default:
                return -1;
            }
            if(component != nullptr)
            {
                if(!literal.empty())
                {
                    _components.push_back(new StringLiteralComponent(literal));
                    literal = "";
                }
                if((minWidth != 0) || (maxWidth != 0))
                {
                    component = new FormatModifierComponent(component, minWidth, maxWidth);
                    minWidth = maxWidth = 0;
                }
                _components.push_back(component);
                component = nullptr;
            }
        }
        else
        {
            literal += ch;
        }
    }
    if(!literal.empty())
    {
        _components.push_back(new StringLiteralComponent(literal));
    }

    _conversionPattern = conversionPattern;

    return 0;
}

std::string PatternLayout::get_conversion_pattern() const
{
    return _conversionPattern;
}

std::string PatternLayout::format(const LoggingEvent &event)
{
    std::ostringstream message;

    for(auto i = _components.begin(); i != _components.end(); ++i)
    {
        (*i)->append(message, event);
    }

    return message.str();
}

} // namespace log4tango
