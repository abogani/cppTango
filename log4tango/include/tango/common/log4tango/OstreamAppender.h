//
// OstreamAppender.h
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

#ifndef _LOG4TANGO_OSTREAMAPPENDER_H
#define _LOG4TANGO_OSTREAMAPPENDER_H

#include <tango/common/log4tango/Portability.h>
#include <string>
#include <iostream>
#include <tango/common/log4tango/LayoutAppender.h>

namespace log4tango
{

//-----------------------------------------------------------------------------
// class : OstreamAppender (appends LoggingEvents to ostreams)
//-----------------------------------------------------------------------------
class OstreamAppender : public LayoutAppender
{
  public:
    OstreamAppender(const std::string &name, std::ostream *stream);
    ~OstreamAppender() override;

    bool reopen() override;
    void close() override;

  protected:
    int _append(const LoggingEvent &event) override;
    std::ostream *_stream;
};

} // namespace log4tango

#endif // _LOG4TANGO_OSTREAMAPPENDER_HH
