//
// Filter.h
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

#ifndef _LOG4TANGO_FILTER_HH
#define _LOG4TANGO_FILTER_HH

#ifdef APPENDERS_HAVE_FILTERS

  #include <tango/common/log4tango/Portability.h>
  #include <tango/common/log4tango/LoggingEvent.h>

namespace log4tango
{

/**
 Users should extend this class to implement customized logging
 event filtering. Note that {@link log4tango::Logger} and {@link
 log4tango::Appender} have built-in filtering rules. It is suggested
 that you first use and understand the built-in rules before rushing
 to write your own custom filters.

 <p>This abstract class assumes and also imposes that filters be
 organized in a linear chain. The <code>decide(LoggingEvent)</code>
 method of each filter is called sequentially, in the order of their
 addition to the chain.

 <p>The <code>decide(LoggingEvent)</code> method must return a
 Decision value, either DENY, NEUTRAL or ACCCEPT.

 <p>If the value DENY is returned, then the log event is
 dropped immediately without consulting with the remaining
 filters.

 <p>If the value NEUTRAL is returned, then the next filter
 in the chain is consulted. If there are no more filters in the
 chain, then the log event is logged. Thus, in the presence of no
 filters, the default behaviour is to log all logging events.

 <p>If the value ACCEPT is returned, then the log
 event is logged without consulting the remaining filters.

 <p>The philosophy of log4tango filters is largely inspired from the
 Linux ipchains.
**/

//-----------------------------------------------------------------------------
// class : Filter
//-----------------------------------------------------------------------------
class Filter
{
  public:
    typedef enum
    {
        DENY = -1,
        NEUTRAL = 0,
        ACCEPT = 1
    } Decision;

    /**
     * Default Constructor for Filter
     **/
    Filter();

    /**
     * Destructor for Filter
     **/
    virtual ~Filter();

    /**
     * Set the next Filter in the Filter chain
     * @param filter The filter to chain
     **/
    void set_chained_filter(Filter *filter);

    /**
     * Get the next Filter in the Filter chain
     * @return The next Filter or nullptr if the current filter
     * is the last in the chain
     **/
    inline Filter *Filter::get_chained_filter(void)
    {
        return _chain;
    }

    /**
     * Get the last Filter in the Filter chain
     * @return The last Filter in the Filter chain
     **/
    virtual Filter *get_end_of_chain(void);

    /**
     * Add a Filter to the end of the Filter chain. Convience method for
     * getEndOfChain()->set_chained_filter(filter).
     * @param filter The filter to add to the end of the chain.
     **/
    virtual void append_chained_filter(Filter *filter);

    /**
     * Decide whether to accept or deny a LoggingEvent. This method will
     * walk the entire chain until a non neutral decision has been made
     * or the end of the chain has been reached.
     * @param event The LoggingEvent to decide on.
     * @return The Decision
     **/
    virtual Decision decide(const LoggingEvent &event);

  protected:
    /**
     * Decide whether <b>this</b> Filter accepts or denies the given
     * LoggingEvent. Actual implementation of Filter should override this
     * method and not <code>decide(LoggingEvent&)</code>.
     * @param event The LoggingEvent to decide on.
     * @return The Decision
     **/
    virtual Decision _decide(const LoggingEvent &event) = 0;

  private:
    Filter *_chain;
};

} // namespace log4tango

#endif // APPENDERS_HAVE_FILTERS

#endif // _LOG4TANGO_FILTER_HH
