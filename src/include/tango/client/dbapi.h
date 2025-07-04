
//+==================================================================================================================
//
// dbapi.h -    include file for TANGO database api
//
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
//+==================================================================================================================

#ifndef _DBAPI_H
#define _DBAPI_H

#include <vector>
#include <cerrno>
#include <tango/client/DbDatum.h>
#include <tango/client/devapi.h>
#include <tango/client/DeviceProxy.h>

/** @defgroup DBase Database Client classes
 * @ingroup Client
 */

#include <tango/common/tango_const.h>

namespace Tango
{
class Database;

/**********************************************************************
 *                                                                    *
 *                  DbServerInfo                                      *
 *                                                                    *
 **********************************************************************/

class DbServerInfo
{
  public:
    std::string name;
    std::string host;
    int mode;
    int level;
};

///
/// Some typedef and define
///

#define POGO_DESC "Description"
#define POGO_TITLE "ProjectTitle"

///
/// Classes declaration
///

class DbProperty
{
  public:
    DbProperty(std::string);
    ~DbProperty();
    //
    // methods
    //
    void get(DbData &);
    void put(DbData &);
    void delete_(DbData &);
};

/**********************************************************************
 *                                                                    *
 *  DbAttribute - A database object for accessing attribute related   *
 *               information in the database                          *
 *                                                                    *
 **********************************************************************/

class DbAttribute
{
  private:
    std::string name;
    std::string device_name;
    Database *dbase;
    int db_ind;
    bool ext_dbase;

  public:
    DbAttribute(const std::string &, const std::string &);
    DbAttribute(const std::string &, const std::string &, Database *);
    DbAttribute(const std::string &, const std::string &, const std::string &, const std::string &);
    ~DbAttribute();
    //
    // methods
    //
    void get_property(DbData &);
    void put_property(const DbData &);
    void delete_property(const DbData &);
};

/**********************************************************************
 *                                                                    *
 *                  DbDevInfo                                         *
 *                                                                    *
 **********************************************************************/

/**
 * Device information for Database device creation
 *
 * @headerfile tango.h
 * @ingroup DBase
 */

class DbDevInfo
{
  public:
    std::string name;   ///< The device name
    std::string _class; ///< The device class name
    std::string server; ///< The full device server process name
};

typedef std::vector<DbDevInfo> DbDevInfos;

/**********************************************************************
 *                                                                    *
 *                  DbDevExportInfo                                   *
 *                                                                    *
 **********************************************************************/

/**
 * Device export information to the database
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbDevExportInfo
{
  public:
    std::string name;    ///< The device name
    std::string ior;     ///< The device IOR
    std::string host;    ///< The host name where the device server process runs
    std::string version; ///< The device version
    int pid;             ///< The device server process PID
};

typedef std::vector<DbDevExportInfo> DbDevExportInfos;

/**********************************************************************
 *                                                                    *
 *  DbServer - A database object for accessing server related         *
 *               information in the database                          *
 *                                                                    *
 **********************************************************************/

/**
 * A database object for a device server which can be used to query or modify server database information.
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbServer
{
  private:
    std::string name;
    Database *dbase;
    int db_ind;
    bool ext_dbase;

    class DbServerExt
    {
      public:
        DbServerExt() { }
    };

    std::unique_ptr<DbServerExt> ext;

  public:
    /**@name Constructors */
    //@{
    /**
     * Create a DbServer instance.
     *
     * A constructor for a DbServer object for a server in the TANGO database specified by the TANGO_HOST
     * environment variable.
     *
     * @param [in] server_name    The device server name
     *
     */
    DbServer(std::string server_name);
    /**
     * Create a DbServer instance using a specified database
     *
     * A constructor for a DbServer object for the server in the specified database. This method reuses the
     * Database supplied by the programmer
     *
     * @param [in] server_name    The device server name
     * @param [in] db The database object
     *
     */
    DbServer(std::string server_name, Database *db);
    //@}
    //
    // methods
    //
    /**@name Server oriented methods */
    //@{
    /**
     * Add a device server process into the database
     *
     * Add a group of devices to the database. The device names, server names and classes are specified in the
     * vector of DbDevInfo structures
     *
     * @param [in] serv Device server process data
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void add_server(DbDevInfos &serv);
    /**
     * Delete the device server from database
     *
     * Delete the device server and its associated devices from the database.
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void delete_server();
    /**
     * Export all device server devices in database
     *
     * Export a group of device to the database. The device names, IOR, class, server name, pid etc. are specified
     * in the vector of DbDevExportInfo structures.
     *
     * @param [in] serv Devices information
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void export_server(DbDevExportInfos &serv);
    /**
     * Mark all devices belonging to the device server as un-exported
     *
     * Mark all the devices exported by the server as un-exported.
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void unexport_server();
    //@}

    /// @privatesection
    ~DbServer();

    DbServerInfo get_server_info();
};

/**********************************************************************
 *                                                                    *
 *  DbClass - A database object for accessing class related           *
 *               information in the database                          *
 *                                                                    *
 **********************************************************************/

/**
 * A database object for a class which can be used to query or modify class properties
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbClass
{
  private:
    std::string name;
    Database *dbase;
    int db_ind;
    bool ext_dbase;

    class DbClassExt
    {
      public:
        DbClassExt() { }
    };

    std::unique_ptr<DbClassExt> ext;

  public:
    /**@name Constructors */
    //@{
    /**
     * Create a DbClass instance.
     *
     * A constructor for a DbClass object for a class in the TANGO database specified by the TANGO_HOST
     * environment variable
     *
     * @param [in] class_name    The Tango class name
     *
     */
    DbClass(std::string class_name);
    /**
     * Create a DbClass instance using a specified database
     *
     * A constructor for a DbClass object for the Tango class in the specified database. This method reuses the
     * Database supplied by the programmer.
     *
     * @param [in] class_name    The Tango class name
     * @param [in] db The database object
     *
     */
    DbClass(std::string class_name, Database *db);
    //@}

    //
    // methods
    //
    /**@name Property oriented methods */
    //@{
    /**
     * Get class property from database
     *
     * Query the database for the list of properties of this class. See Database::get_class_property() for an example
     * of how to specify and retrieve the properties.
     *
     * @param [in,out] db Property name(s) and value
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void get_property(DbData &db);
    /**
     * Update class property in database
     *
     * Update the list of properties for this class in the database. See Database::put_class_property() for an example
     * of how to specify the properties.
     *
     * @param [in] db Property name(s) and value
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void put_property(const DbData &db);
    /**
     * Remove class property from database
     *
     * Delete the list of specified properties for this class in the database. See Database::delete_property() for an
     * example of how to specify the properties.
     *
     * @param [in] db Property name(s)
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void delete_property(const DbData &db);
    /**
     * Get class attribute property from database
     *
     * Query the database for the list of attribute properties of this class. See
     * Database::get_class_attribute_property() for an example of how to specify and retrieve the properties.
     *
     * @param [in,out] db Property name(s) and value
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void get_attribute_property(DbData &db);
    /**
     * Update class attribute property in database
     *
     * Update the list of attribute properties for this class in the database. See
     * Database::put_class_attribute_property() for an example of how to specify the properties.
     *
     * @param [in] db Property name(s) and value
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void put_attribute_property(const DbData &db);
    /**
     * Remove class attribute property from database
     *
     * Delete all properties for the list of specified attributes for this class in the database. See
     * Database::delete_class_attribute_property() for an example of how to specify the properties.
     *
     * @param [in] db Property name(s)
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void delete_attribute_property(const DbData &db);
    /**
     * Get class pipe property from database
     *
     * Query the database for the list of pipe properties of this class. See Database::get_class_pipe_property()
     * for an example of how to specify and retrieve the properties.
     *
     * @param [in,out] db Property name(s) and value
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void get_pipe_property(DbData &db);
    /**
     * Update class pipe property in database
     *
     * Update the list of pipe properties for this class in the database. See Database::put_class_pipe_property()
     * for an example of how to specify the properties.
     *
     * @param [in] db Property name(s) and value
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void put_pipe_property(const DbData &db);
    /**
     * Remove class pipe property from database
     *
     * Delete all properties for the list of specified pipes for this class in the database. See
     * Database::delete_class_pipe_property() for an example of how to specify the properties.
     *
     * @param [in] db Property name(s)
     *
     * @exception ConnectionFailed, CommunnicationFailed, DevFailed from device
     */
    void delete_pipe_property(const DbData &db);
    //@}

    /// @privatesection
    ~DbClass();
};

/**********************************************************************
 *                                                                    *
 *  DbHistory - A data object for receiving data history from the     *
 *               Tango database                                       *
 *                                                                    *
 **********************************************************************/

/**
 * Class used to retrieve database object history
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbHistory
{
  public:
    //
    // constructor methods
    //

    DbHistory(std::string, std::string, const std::vector<std::string> &);
    DbHistory(std::string, std::string, std::string, const std::vector<std::string> &);

    //
    // getter methods
    //

    /**
     * Get property name
     *
     * @return The property name
     */
    std::string get_name();
    /**
     * Get attribute name
     *
     * @return The attribute name
     */
    std::string get_attribute_name();
    /**
     * Get change date
     *
     * @return The date
     */
    std::string get_date();
    /**
     * Get value
     *
     * @return The property value
     */
    DbDatum get_value();
    /**
     * Get property deleted flag
     *
     * @return The property deleted flag
     */
    bool is_deleted();

  private:
    std::string propname; // Property name
    std::string attname;  // Attribute name (Not used for device/pipe properties)
    DbDatum value;        // Property value
    std::string date;     // Update date
    bool deleted;         // Deleted flag

    std::string format_mysql_date(std::string);
    void make_db_datum(const std::vector<std::string> &);
};

/**********************************************************************
 *                                                                    *
 *                  DbDevImportInfo                                   *
 *                                                                    *
 **********************************************************************/

/**
 * Device import information from the database
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbDevImportInfo
{
  public:
    std::string name;    ///< The device name
    long exported;       ///< The exported flag
    std::string ior;     ///< The device IOR
    std::string version; ///< The device version (as a std::string)
};

typedef std::vector<DbDevImportInfo> DbDevImportInfos;

/****************************************************************
 *                                                              *
 *                  DbDevFullInfo                               *
 *                                                              *
 ****************************************************************/

/**
 * Device information from the database
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbDevFullInfo : public DbDevImportInfo
{
  public:
    std::string class_name;   ///< The device class name
    std::string ds_full_name; ///< The full device server process name
    std::string host;         ///< The host name where the device server process is running
    std::string started_date; ///< Date of the last device export (empty if not set in DB)
    std::string stopped_date; ///< Date of the last device un-export (empty if not set in DB)
    long pid;                 ///< The device server process PID (-1 if not set in DB)
};

/****************************************************************************************
 *                                                                                         *
 *                     The DbServerCache class                                                *
 *                     ------------------                                                    *
 *                                                                                         *
 ***************************************************************************************/

//
// DbServerCache data object to implement a DB cache
// used during the DS startup sequence
//

class DbServerCache
{
  public:
    typedef struct
    {
        int first_idx;
        int last_idx;
    } EltIdx;

    typedef struct
    {
        int first_idx;
        int last_idx;
        int prop_nb;
        int *props_idx;
    } PropEltIdx;

    typedef struct
    {
        int first_idx;
        int last_idx;
        int att_nb;
        int *atts_idx;
    } AttPropEltIdx;

    typedef struct
    {
        PropEltIdx dev_prop;
        AttPropEltIdx dev_att_prop;
        AttPropEltIdx dev_pipe_prop;
    } DevEltIdx;

    typedef struct
    {
        PropEltIdx class_prop;
        AttPropEltIdx class_att_prop;
        AttPropEltIdx class_pipe_prop;
        EltIdx dev_list;
        int dev_nb;
        DevEltIdx *devs_idx;
    } ClassEltIdx;

    DbServerCache(Database *, const std::string &, const std::string &);
    ~DbServerCache();

    const DevVarLongStringArray *import_adm_dev();
    const DevVarLongStringArray *import_notifd_event();
    const DevVarLongStringArray *import_adm_event();
    const DevVarStringArray *get_class_property(DevVarStringArray *);
    const DevVarStringArray *get_dev_property(DevVarStringArray *);
    const DevVarStringArray *get_dev_list(DevVarStringArray *);
    const DevVarStringArray *get_class_att_property(DevVarStringArray *);
    const DevVarStringArray *get_dev_att_property(DevVarStringArray *);
    const DevVarStringArray *get_obj_property(DevVarStringArray *);
    const DevVarStringArray *get_device_property_list(DevVarStringArray *);
    const DevVarStringArray *get_class_pipe_property(DevVarStringArray *);
    const DevVarStringArray *get_dev_pipe_property(DevVarStringArray *);
    const DevVarLongStringArray *import_tac_dev(const std::string &);

    const EltIdx &get_imp_dat()
    {
        return imp_adm;
    }

    const EltIdx &get_imp_notifd_event()
    {
        return imp_notifd_event;
    }

    const EltIdx &get_imp_adm_event()
    {
        return imp_adm_event;
    }

    const PropEltIdx &get_DServer_class_prop()
    {
        return DServer_class_prop;
    }

    const PropEltIdx &get_Default_prop()
    {
        return Default_prop;
    }

    const PropEltIdx &get_adm_dev_prop()
    {
        return adm_dev_prop;
    }

    const PropEltIdx &get_ctrl_serv_prop()
    {
        return ctrl_serv_prop;
    }

    int get_class_nb()
    {
        return class_nb;
    }

    const ClassEltIdx *get_classes_elt()
    {
        return classes_idx;
    }

    int get_data_nb()
    {
        return n_data;
    }

  private:
    void prop_indexes(int &, int &, PropEltIdx &, const DevVarStringArray *);
    void prop_att_indexes(int &, int &, AttPropEltIdx &, const DevVarStringArray *);
    void prop_pipe_indexes(int &, int &, AttPropEltIdx &, const DevVarStringArray *);
    void get_obj_prop(DevVarStringArray *, PropEltIdx &, bool dev_prop = false);
    int find_class(DevString);
    int find_dev_att(DevString, int &, int &);
    int find_obj(DevString obj_name, int &);
    void get_obj_prop_list(DevVarStringArray *, PropEltIdx &);

    CORBA::Any_var received;
    const DevVarStringArray *data_list;
    int n_data;
    int proc_release;

    EltIdx imp_adm;
    EltIdx imp_notifd_event;
    EltIdx imp_adm_event;
    EltIdx imp_tac;
    PropEltIdx ctrl_serv_prop;
    PropEltIdx DServer_class_prop;
    PropEltIdx Default_prop;
    PropEltIdx adm_dev_prop;
    int class_nb;
    ClassEltIdx *classes_idx;

    DevVarLongStringArray imp_adm_data;
    DevVarLongStringArray imp_notifd_event_data;
    DevVarLongStringArray imp_adm_event_data;
    DevVarLongStringArray imp_tac_data;
    DevVarStringArray ret_obj_prop;
    DevVarStringArray ret_dev_list;
    DevVarStringArray ret_obj_att_prop;
    DevVarStringArray ret_obj_pipe_prop;
    DevVarStringArray ret_prop_list;
};

/****************************************************************************************
 *                                                                                         *
 *                     The DbServerData class                                                *
 *                     ----------------                                                    *
 *                                                                                         *
 ***************************************************************************************/

//
// DbServerData object to implement the features required to move a complete device server proces
// configuration from one database to another one
//

/**
 * Class used to move/copy a complete device server process database configuration
 * from one Tango host to another
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbServerData
{
  private:
    struct TangoProperty
    {
        std::string name;
        std::vector<std::string> values;

        TangoProperty(std::string &na, std::vector<std::string> &val) :
            name(na),
            values(val)
        {
        }
    };

    struct TangoAttribute : std::vector<TangoProperty>
    {
        std::string name;

        TangoAttribute(std::string na) :
            name(na)
        {
        }
    };

    struct TangoPipe : std::vector<TangoProperty>
    {
        std::string name;

        TangoPipe(std::string na) :
            name(na)
        {
        }
    };

    struct TangoDevice : DeviceProxy
    {
        std::string name;
        std::vector<TangoProperty> properties;
        std::vector<TangoAttribute> attributes;
        std::vector<TangoPipe> pipes;

        TangoDevice(const std::string &);

        std::string get_name()
        {
            return name;
        }

        std::vector<TangoProperty> &get_properties()
        {
            return properties;
        }

        std::vector<TangoAttribute> &get_attributes()
        {
            return attributes;
        }

        std::vector<TangoPipe> &get_pipes()
        {
            return pipes;
        }

        void put_properties(Database *);
        void put_attribute_properties(Database *);
        void put_pipe_properties(Database *);
    };

    struct TangoClass : std::vector<TangoDevice>
    {
        std::string name;
        std::vector<TangoProperty> properties;
        std::vector<TangoAttribute> attributes;
        std::vector<TangoPipe> pipes;

        TangoClass(const std::string &, const std::string &, Database *);

        std::string get_name()
        {
            return name;
        }

        std::vector<TangoProperty> &get_properties()
        {
            return properties;
        }

        std::vector<TangoAttribute> &get_attributes()
        {
            return attributes;
        }

        std::vector<TangoPipe> &get_pipes()
        {
            return pipes;
        }

        void put_properties(Database *);
        void put_attribute_properties(Database *);
        void put_pipe_properties(Database *);
        void remove_properties(Database *);
    };

    void create_server(Database *);
    void put_properties(Database *);

    std::string full_server_name;
    std::vector<TangoClass> classes;

  public:
    /**
     * Create a DbServerData instance.
     *
     * A constructor for a DbServerData object for a device server process defined in the TANGO database specified by
     * the TANGO_HOST environment variable
     *
     * @param [in] ds_exec_name    The device server process executable name
     * @param [in] ds_inst_name The device server process instance name
     *
     */
    DbServerData(const std::string &ds_exec_name, const std::string &ds_inst_name);
    /**
     * Check if device(s) already defined
     *
     * Return true if one of the device defined in the device server process with configuration in this instance is
     * already defined in the database defined by the Tango host given as parameter
     *
     * @param [in] tg_host The tango host
     * @return Boolean set to true if one of the device server device is defined in the Tango host given as parameter
     */
    bool already_exist(const std::string &tg_host);
    /**
     * Put device server database configuration in new Tango host
     *
     * Store the device server process database configuration in the database specified by the given Tango host
     * parameter
     *
     * @param [in] tg_host The tango host
     */
    void put_in_database(const std::string &tg_host);
    /**
     * Remove device server database configuration from Tango host
     *
     * Remove the device server process database configuration from the database specified by the given Tango host
     * parameter
     *
     * @param [in] tg_host The tango host
     */
    void remove(const std::string &tg_host);
    /**
     * Remove device server database configuration from default Tango host
     *
     * Remove the device server process database configuration from the database specified by the environment variable
     * TANGO_HOST
     *
     */
    void remove();

    ///@privatesection
    ~DbServerData() { }

    const std::string &get_name()
    {
        return full_server_name;
    }

    std::vector<TangoClass> &get_classes()
    {
        return classes;
    }
};

/*
// Some macros to call the Db server
// These macros will do some retries in case of
// timeout while calling the DB device
// This is necessary in case of massive DS
// startup (after a power cut for instance)
// when the Db is over-loaded
*/

#define MANAGE_EXCEPT(NAME)                                                \
    catch(Tango::CommunicationFailed & e)                                  \
    {                                                                      \
        if(e.errors.length() >= 2)                                         \
        {                                                                  \
            if(::strcmp(e.errors[1].reason.in(), API_DeviceTimedOut) == 0) \
            {                                                              \
                if(db_retries != 0)                                        \
                {                                                          \
                    db_retries--;                                          \
                    if(db_retries == 0)                                    \
                        throw;                                             \
                }                                                          \
                else                                                       \
                    throw;                                                 \
            }                                                              \
            else                                                           \
                throw;                                                     \
        }                                                                  \
        else                                                               \
            throw;                                                         \
    }

#define CALL_DB_SERVER_NO_RET(NAME, SEND)            \
    {                                                \
        bool retry_mac = true;                       \
        long db_retries = 0;                         \
        if(db_tg != nullptr)                         \
        {                                            \
            if(db_tg->is_svr_starting() == true)     \
                db_retries = DB_START_PHASE_RETRIES; \
        }                                            \
        while(retry_mac == true)                     \
        {                                            \
            try                                      \
            {                                        \
                command_inout(NAME, SEND);           \
                retry_mac = false;                   \
            }                                        \
            MANAGE_EXCEPT(NAME)                      \
        }                                            \
    }

#define CALL_DB_SERVER(NAME, SEND, RET)              \
    {                                                \
        bool retry_mac = true;                       \
        long db_retries = 0;                         \
        if(db_tg != nullptr)                         \
        {                                            \
            if(db_tg->is_svr_starting() == true)     \
                db_retries = DB_START_PHASE_RETRIES; \
        }                                            \
        while(retry_mac == true)                     \
        {                                            \
            try                                      \
            {                                        \
                RET = command_inout(NAME, SEND);     \
                retry_mac = false;                   \
            }                                        \
            MANAGE_EXCEPT(NAME)                      \
        }                                            \
    }

} // namespace Tango

#endif /* _DBAPI_H */
