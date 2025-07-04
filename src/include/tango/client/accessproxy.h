//
// AccessProxy.h -    include file for TANGO AccessProxy class
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

#ifndef _ACCESSPROXY_H
#define _ACCESSPROXY_H

#include <tango/client/DeviceProxy.h>
#include <tango/common/tango_const.h>

#include <string>
#include <vector>
#include <map>

namespace Tango
{

#define __AC_BUFFER_SIZE 1024

class AccessProxy : public Tango::DeviceProxy
{
  public:
    AccessProxy(const std::string &);
    AccessProxy(const char *);

    ~AccessProxy() override { }

    AccessControlType check_access_control(const std::string &);
    bool is_command_allowed(std::string &, const std::string &);

  protected:
    std::string user;
    std::vector<std::string> host_ips;
    bool forced;
    std::map<std::string, std::vector<std::string>> allowed_cmd_table;
    omni_mutex only_one;

    void get_allowed_commands(const std::string &, std::vector<std::string> &);

  private:
    void real_ctor();
};

} // namespace Tango

#endif /* _ACCESSPROXY_H */
