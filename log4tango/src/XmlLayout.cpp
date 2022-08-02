//
// XMLLayout.cpp
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

#include <tango/common/log4tango/Portability.hh>
#include <chrono>
#include <tango/common/log4tango/LoggingEvent.hh>
#include <tango/common/log4tango/XmlLayout.hh>

#include <stdio.h>
#include <sstream>
#include <string>

namespace log4tango {

  XMLLayout::XMLLayout()
  {
    // no-op
  }

  XMLLayout::~XMLLayout()
  {
    // no-op
  }

  std::string XMLLayout::format (const LoggingEvent& event)
  {
    auto id = []() -> std::string {
      std::ostringstream out;
      out << std::this_thread::get_id();
      return out.str();
    };

    auto ts = [](const LoggingEvent& event) -> std::string {
      std::ostringstream out;
      auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.timestamp.time_since_epoch()).count();
      out << ts_ms;
      return out.str();
    };

    std::string buf;
    buf.append("<log4j:event logger=\"");
    buf.append(event.logger_name);
    buf.append("\" timestamp=\"");
    buf.append(ts(event));
    buf.append("\" level=\"");
    buf.append(log4tango::Level::get_name(event.level));
    buf.append("\" thread=\"");
    buf.append(id());
    buf.append("\">\r\n");
    buf.append("<log4j:message><![CDATA[");
    buf.append(event.message);
    buf.append("]]></log4j:message>\r\n");
    buf.append("<log4j:NDC><![CDATA[");
    buf.append("]]></log4j:NDC>\r\n");
    buf.append("</log4j:event>\r\n\r\n");
    return buf;
  }

} // namespace log4tango
