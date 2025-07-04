//===================================================================================================================
//
// file :        pipedesc.h
//
// description :    Include file for the Pipe user configuration
//
// project :        TANGO
//
// author(s) :        E.Taurel
//
// Copyright (C) :      2014
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//
//===================================================================================================================

#ifndef _PIPEDESC_H
#define _PIPEDESC_H

#include <string>
#include <memory>

namespace Tango
{

/**
 * User class to set pipe default properties.
 *
 * This class is used to set pipe default properties. Three levels of
 * pipes properties setting are implemented within Tango. The highest
 * property setting level is the database. Then the user default (set using
 * this UserDefaultPipeProp class) and finally a Tango library default
 * value
 *
 *
 * @headerfile tango.h
 * @ingroup Server
 */

class UserDefaultPipeProp
{
  public:
    /**@name Constructor
     * Only one constructor is defined for this class */
    //@{
    /**
     * Constructs a newly allocated UserDefaultPipeProp object.
     */
    UserDefaultPipeProp() :
        ext(nullptr)
    {
    }

    //@}

    /**@name Set default property methods */
    //@{
    /**
     * Set default label property
     *
     * @param    def_label    The user default label property
     */
    void set_label(const std::string &def_label)
    {
        label = def_label;
    }

    /**
     * Set default description property
     *
     * @param    def_desc    The user default description property
     */
    void set_description(const std::string &def_desc)
    {
        description = def_desc;
    }

    //@}

    /// @privatesection
    ~UserDefaultPipeProp() { }

    std::string label;
    std::string description;

  private:
    class UserDefaultPipePropExt
    {
    };

    std::unique_ptr<UserDefaultPipePropExt> ext; // Class extension
};

} // namespace Tango

#endif /* _PIPEDESC_H */
