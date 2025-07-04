//
// cpp     - C++ source code file for TANGO dbapi class Database
//
// programmer     - Andy Gotz (goetz@esrf.fr)
//
// original     - September 2000
//
// Copyright (C) :      2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
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
//
//

#include <tango/server/seqvec.h>
#include <tango/client/accessproxy.h>
#include <tango/client/Database.h>
#include <tango/internal/net.h>

#include <memory>

using namespace CORBA;

namespace Tango
{

//-----------------------------------------------------------------------------
//
// Database::Database() - constructor to create connection to TANGO Database
//                        without arguments, host and port specified by
//                        environment variable TANGO_HOST
//              Rem : For Windows, when the server is used as a
//                service, the TANGO_HOST environment
//                variable must be retrieved from the registry
//                from the device server exec name and instance
//                name
//
//-----------------------------------------------------------------------------

Database::Database(CORBA::ORB_var orb_in) :
    Connection(orb_in),
    ext(new DatabaseExt),
    access_proxy(nullptr),
    access_checked(false),
    access_service_defined(false),
    db_tg(nullptr)
{
    //
    // get host and port from environment variable TANGO_HOST
    //

    std::string tango_host_env_var;
    int ret;
    filedb = nullptr;
    serv_version = 0;

    ret = get_env_var(EnvVariable, tango_host_env_var);

    if(ret == -1)
    {
        TangoSys_MemStream desc;
        desc << "TANGO_HOST env. variable not set, set it and retry (e.g. TANGO_HOST=<host>:<port>)" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
    }

    check_tango_host(tango_host_env_var.c_str());

    TANGO_LOG_DEBUG << "Database::Database(): TANGO host " << host << " port " << port << std::endl;

    build_connection();

    set_server_release();

    dev_name();
}

#ifdef _TG_WINDOWS_
Database::Database(CORBA::ORB_var orb_in, std::string &ds_exec_name, std::string &ds_inst_name) :
    Connection(orb_in),
    ext(new DatabaseExt),
    access_proxy(nullptr),
    access_checked(false),
    access_service_defined(false),
    db_tg(nullptr)
{
    //
    // get host and port from environment variable TANGO_HOST
    //
    char *tango_host_env_c_str;
    filedb = nullptr;
    serv_version = 0;

    if(get_tango_host_from_reg(&tango_host_env_c_str, ds_exec_name, ds_inst_name) == -1)
    {
        tango_host_env_c_str = nullptr;
    }

    if(tango_host_env_c_str == nullptr)
    {
        TangoSys_MemStream desc;
        desc << "TANGO_HOST env. variable not set, set it and retry (e.g. TANGO_HOST=<host>:<port>)" << std::ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
    }

    check_tango_host(tango_host_env_c_str);

    delete[] tango_host_env_c_str;

    TANGO_LOG_DEBUG << "Database::Database(): TANGO host " << host << " port " << port << std::endl;

    build_connection();

    set_server_release();

    dev_name();
}
#endif

//+----------------------------------------------------------------------------
//
// method :         Database::check_tango_host()
//
// description :     Check the TANGO_HOST environment variable syntax and
//            extract database server host(s) and port(s) from
//            it
//
// in :    tango_host_env_c_str : The TANGO_HOST env. variable value
//
//-----------------------------------------------------------------------------

void Database::check_tango_host(const char *tango_host_env_c_str)
{
    filedb = nullptr;
    std::string tango_host_env(tango_host_env_c_str);
    std::string::size_type separator;

    db_multi_svc = false;
    separator = tango_host_env.find(',');
    if(separator != std::string::npos)
    {
        //
        // It is a multi db server system, extract each host and port and check
        // syntax validity (<host>:<port>,<host>:<port>)
        //

        db_multi_svc = true;
        separator = 0;
        std::string::size_type old_sep = separator;
        std::string::size_type host_sep;
        while((separator = tango_host_env.find(',', separator + 1)) != std::string::npos)
        {
            std::string sub = tango_host_env.substr(old_sep, separator - old_sep);
            old_sep = separator + 1;
            host_sep = sub.find(':');
            if(host_sep != std::string::npos)
            {
                if((host_sep == sub.size() - 1) || (host_sep == 0))
                {
                    TangoSys_MemStream desc;
                    desc << "TANGO_HOST env. variable syntax incorrect (e.g. TANGO_HOST=<host>:<port>,<host>:<port>)"
                         << std::ends;
                    TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
                }
                multi_db_port.push_back(sub.substr(host_sep + 1));
            }
            else
            {
                TangoSys_MemStream desc;
                desc << "TANGO_HOST env. variable syntax incorrect (e.g. TANGO_HOST=<host>:<port>,<host>:<port>)"
                     << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
            }
            std::string tmp_host(sub.substr(0, host_sep));
            if(tmp_host.find('.') == std::string::npos)
            {
                get_fqdn(tmp_host);
            }
            multi_db_host.push_back(tmp_host);
        }
        std::string last = tango_host_env.substr(old_sep);
        host_sep = last.find(':');
        if(host_sep != std::string::npos)
        {
            if((host_sep == last.size() - 1) || (host_sep == 0))
            {
                TangoSys_MemStream desc;
                desc << "TANGO_HOST env. variable syntax incorrect (e.g. TANGO_HOST=<host>:<port>,<host>:<port>)"
                     << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
            }
            multi_db_port.push_back(last.substr(host_sep + 1));
        }
        else
        {
            TangoSys_MemStream desc;
            desc << "TANGO_HOST env. variable syntax incorrect (e.g. TANGO_HOST=<host>:<port>,<host>:<port>)"
                 << std::ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
        }
        std::string tmp_host(last.substr(0, host_sep));
        if(tmp_host.find('.') == std::string::npos)
        {
            get_fqdn(tmp_host);
        }
        multi_db_host.push_back(tmp_host);

        db_port = multi_db_port[0];
        db_host = multi_db_host[0];
        db_port_num = atoi(db_port.c_str());
    }
    else
    {
        //
        // Single database server case
        //

        separator = tango_host_env.find(':');
        if(separator != std::string::npos)
        {
            if((separator == tango_host_env.size() - 1) || (separator == 0))
            {
                TangoSys_MemStream desc;
                desc << "TANGO_HOST env. variable syntax incorrect (e.g. TANGO_HOST=<host>:<port>)" << std::ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
            }
            db_port = tango_host_env.substr(separator + 1);
            db_port_num = atoi(db_port.c_str());
        }
        else
        {
            TangoSys_MemStream desc;
            desc << "TANGO_HOST env. variable syntax incorrect (e.g. TANGO_HOST=<host>:<port>)" << std::ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_TangoHostNotSet, desc.str());
        }
        db_host = tango_host_env.substr(0, separator);

        //
        // If localhost is used as host name, try to replace it by the real host name
        //

        std::string db_host_lower(db_host);
        std::transform(db_host_lower.begin(), db_host_lower.end(), db_host_lower.begin(), ::tolower);

        if(db_host_lower == "localhost")
        {
            char h_name[Tango::detail::TANGO_MAX_HOSTNAME_LEN];
            int res = gethostname(h_name, Tango::detail::TANGO_MAX_HOSTNAME_LEN);
            if(res == 0)
            {
                db_host = h_name;
                tango_host_localhost = true;
            }
        }

        //
        // Get FQDN but store original TANGO_HOST (for event in case of alias used in TANGO_HOST)
        //

        ext->orig_tango_host = db_host;
        if(db_host.find('.') == std::string::npos)
        {
            get_fqdn(db_host);
            std::string::size_type pos = db_host.find('.');
            if(pos != std::string::npos)
            {
                std::string fq = db_host.substr(pos);
                ext->orig_tango_host = ext->orig_tango_host + fq;
            }
        }
    }

    host = db_host;
    port = db_port;
    port_num = db_port_num;

    dbase_used = true;
    from_env_var = true;
}

//-----------------------------------------------------------------------------
//
// Database::Database(std::stringhost, int port, CORBA::ORB_var orb) - constructor to
//           create connection to TANGO Database with host, port, orb specified.
//
//-----------------------------------------------------------------------------

Database::Database(const std::string &in_host, int in_port, CORBA::ORB_var orb_in) :
    Connection(orb_in),
    ext(new DatabaseExt),
    access_proxy(nullptr),
    access_checked(false),
    access_service_defined(false),
    db_tg(nullptr)
{
    filedb = nullptr;
    serv_version = 0;
    db_multi_svc = false;

    host = in_host;
    db_host = host;

    TangoSys_MemStream o;
    o << in_port << std::ends;

    std::string st = o.str();
    port = st.c_str();

    db_port = port;

    db_port_num = in_port;
    port_num = in_port;
    from_env_var = false;
    dbase_used = true;

    build_connection();

    set_server_release();

    dev_name();
}

Database::Database(const std::string &name) :
    Connection(true),
    ext(new DatabaseExt),
    access_proxy(nullptr),
    access_checked(false),
    access_service_defined(false),
    db_tg(nullptr)
{
    file_name = name;
    filedb = new FileDatabase(file_name);
    serv_version = 230;

    check_acc = false;
}

//-----------------------------------------------------------------------------
//
// Database::Database() - copy constructor
//
//-----------------------------------------------------------------------------

Database::Database(const Database &sou) :
    Connection(sou),
    ext(nullptr)
{
    //
    // Copy Database members
    //

    db_multi_svc = sou.db_multi_svc;
    multi_db_port = sou.multi_db_port;
    multi_db_host = sou.multi_db_host;
    file_name = sou.file_name;
    if(sou.filedb == nullptr)
    {
        filedb = nullptr;
    }
    else
    {
        filedb = new FileDatabase(file_name);
    }
    serv_version = sou.serv_version;

    if(sou.access_proxy == nullptr)
    {
        access_proxy = nullptr;
    }
    else
    {
        access_proxy = new AccessProxy(sou.access_proxy->name().c_str());
    }
    access_checked = sou.access_checked;
    access_except_errors = sou.access_except_errors;

    dev_class_cache = sou.dev_class_cache;
    db_device_name = sou.db_device_name;

    access_service_defined = sou.access_service_defined;
    db_tg = sou.db_tg;

    //
    // Copy extension class
    //

    if(sou.ext != nullptr)
    {
        ext = std::make_unique<DatabaseExt>();
    }
}

//-----------------------------------------------------------------------------
//
// Database::Database() - assignement operator
//
//-----------------------------------------------------------------------------

Database &Database::operator=(const Database &rval)
{
    if(this != &rval)
    {
        this->Connection::operator=(rval);

        //
        // Now Database members
        //

        db_multi_svc = rval.db_multi_svc;
        multi_db_port = rval.multi_db_port;
        multi_db_host = rval.multi_db_host;
        file_name = rval.file_name;
        serv_version = rval.serv_version;

        delete filedb;
        if(rval.filedb == nullptr)
        {
            filedb = nullptr;
        }
        else
        {
            filedb = new FileDatabase(file_name);
        }

        delete access_proxy;
        if(rval.access_proxy == nullptr)
        {
            access_proxy = nullptr;
        }
        else
        {
            access_proxy = new AccessProxy(rval.access_proxy->name().c_str());
        }
        access_checked = rval.access_checked;
        access_except_errors = rval.access_except_errors;

        dev_class_cache = rval.dev_class_cache;
        db_device_name = rval.db_device_name;

        access_service_defined = rval.access_service_defined;
        db_tg = rval.db_tg;

        if(rval.ext != nullptr)
        {
            ext = std::make_unique<DatabaseExt>();
        }
        else
        {
            ext.reset();
        }
    }

    return *this;
}

//-----------------------------------------------------------------------------
//
// Database::check_access_and_get() - Check if the access has been retrieved
// and get it if not.
// Protect this code using a TangoMonitor defined in the Connection class
//
//-----------------------------------------------------------------------------

void Database::check_access_and_get()
{
    bool local_access_checked;
    {
        ReaderLock guard(con_to_mon);
        local_access_checked = access_checked;
    }
    if(!local_access_checked)
    {
        WriterLock guard(con_to_mon);
        if(!access_checked)
        {
            check_access();
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::set_server_release() - Check which is the database server release
//
//-----------------------------------------------------------------------------

void Database::set_server_release()
{
    try
    {
        DevCmdInfo *cmd_ptr = device->command_query("DbDeleteAllDeviceAttributeProperty");
        serv_version = 400;
        delete cmd_ptr;
    }
    catch(Tango::DevFailed &e)
    {
        if(::strcmp(e.errors[0].reason.in(), API_CommandNotFound) == 0)
        {
            try
            {
                DevCmdInfo *cmd_ptr = device->command_query("DbGetDeviceAttributeProperty2");
                serv_version = 230;
                delete cmd_ptr;
            }
            catch(Tango::DevFailed &)
            {
                serv_version = 210;
            }
        }
        else
        {
            serv_version = 210;
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_file_name() - Get file name when the database is a file
//
//-----------------------------------------------------------------------------

const std::string &Database::get_file_name()
{
    if(filedb == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, "The database is not a file-based database");
    }

    return file_name;
}

//-----------------------------------------------------------------------------
//
// Database::~Database() - destructor to destroy connection to TANGO Database
//
//-----------------------------------------------------------------------------

Database::~Database()
{
    if(filedb != nullptr)
    {
        write_filedatabase();
        delete filedb;
    }

    delete access_proxy;
}

#ifdef _TG_WINDOWS_
//+----------------------------------------------------------------------------
//
// method :         Util::get_tango_host_from_reg()
//
// description :     When the server is a Win32 service, it is not
//            possible to get the env. variable TANGO_HOST
//            value. In this case, the TANGO_HOST value is
//            stored in the WIN32 registry when the service is
//            installed and retrieved by this method.
//
//-----------------------------------------------------------------------------

long Database::get_tango_host_from_reg(char **buffer, std::string &ds_exec_name, std::string &ds_instance_name)
{
    //
    // Build the key name
    //

    HKEY regHandle = HKEY_LOCAL_MACHINE;
    std::string keyName("SYSTEM\\CurrentControlSet\\Services\\");
    keyName = keyName + ds_exec_name + '_' + ds_instance_name;
    keyName = keyName + "\\Server";

    //
    // Open the key.
    //

    HKEY keyHandle;
    if(::RegOpenKeyEx(regHandle, keyName.c_str(), 0, KEY_ALL_ACCESS, &keyHandle) != ERROR_SUCCESS)
    {
        return -1;
    }

    //
    // Read the value
    //

    DWORD type, size;
    char tmp_buf[128];
    size = 127;
    if(::RegQueryValueEx(keyHandle, "TangoHost", nullptr, &type, (unsigned char *) tmp_buf, &size) != ERROR_SUCCESS)
    {
        return -1;
    }

    //
    // Store answer in caller parameter
    //

    *buffer = new char[strlen(tmp_buf) + 1];
    strcpy(*buffer, tmp_buf);

    return 0;
}
#endif

//-----------------------------------------------------------------------------
//
// Database::connect() - method to create connection to TANGO Database
//                       specifying host and port
//
//-----------------------------------------------------------------------------

void Database::write_filedatabase()
{
    if(filedb != nullptr)
    {
        filedb->write_file();
    }
}

void Database::reread_filedatabase()
{
    delete filedb;
    filedb = new FileDatabase(file_name);
}

void Database::build_connection()
{
    // omniORB::setClientConnectTimeout(CORBA::ULong(DB_CONNECT_TIMEOUT));
    try
    {
        // we now use reconnect instead of connect
        // it allows us to know more about the device we are talking to
        // see Connection::reconnect for details
        reconnect(true);

        // omniORB::setClientConnectTimeout(0);
    }
    catch(Tango::DevFailed &)
    {
        // omniORB::setClientConnectTimeout(0);
        throw;
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_corba_name() - private method to return corba name
//
//-----------------------------------------------------------------------------

std::string Database::get_corba_name(TANGO_UNUSED(bool ch_acc))
{
    std::string db_corbaloc;
    if(db_multi_svc)
    {
        db_corbaloc = "corbaloc:iiop:1.2@";
        int nb_host = multi_db_host.size();
        for(int i = 0; i < nb_host; i++)
        {
            db_corbaloc = db_corbaloc + multi_db_host[i];
            db_corbaloc = db_corbaloc + ":" + multi_db_port[i];
            if(i != nb_host - 1)
            {
                db_corbaloc = db_corbaloc + ",iiop:1.2@";
            }
        }
        db_corbaloc = db_corbaloc + "/" + DbObjName;
    }
    else
    {
        db_corbaloc = "corbaloc:iiop:";
        if(tango_host_localhost)
        {
            db_corbaloc = db_corbaloc + "localhost:";
        }
        else
        {
            db_corbaloc = db_corbaloc + db_host + ":";
        }
        db_corbaloc = db_corbaloc + port + "/" + DbObjName;
    }

    return db_corbaloc;
}

//-----------------------------------------------------------------------------
//
// Database::post_reconnection() - do what is necessary after a db re-connection
//
//-----------------------------------------------------------------------------

void Database::post_reconnection()
{
    set_server_release();

    dev_name();
}

//-----------------------------------------------------------------------------
//
// Database::get_info() - public method to return info about the Database
//
//-----------------------------------------------------------------------------

std::string Database::get_info()
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    if(filedb != nullptr)
    {
        received = filedb->DbInfo(send);
    }
    else
    {
        CALL_DB_SERVER("DbInfo", send, received);
    }
    const DevVarStringArray *db_info_list = nullptr;
    received.inout() >>= db_info_list;

    std::ostringstream ostream;

    for(unsigned int i = 0; i < db_info_list->length(); i++)
    {
        ostream << (*db_info_list)[i].in() << std::endl;
    }

    std::string ret_str = ostream.str();

    return (ret_str);
}

//-----------------------------------------------------------------------------
//
// Database::import_device() - public method to return import info for a device
//
//-----------------------------------------------------------------------------

DbDevImportInfo Database::import_device(const std::string &dev)
{
    Any send;
    DeviceData received_cmd;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    const DevVarLongStringArray *dev_import_list = nullptr;

    //
    // Import device is allways possible whatever access rights are
    //

    DbDevImportInfo dev_import;
    {
        WriterLock guard(con_to_mon);

        AccessControlType tmp_access = access;
        access = ACCESS_WRITE;

        send <<= dev.c_str();
        bool imported_from_cache = false;

        try
        {
            if(filedb != nullptr)
            {
                received = filedb->DbImportDevice(send);
                received.inout() >>= dev_import_list;
            }
            else
            {
                //
                // If we are in a server with a valid db_cache (meaning only during
                // device server startup sequence) and if
                // the device to be imported is the TAC device, do the
                // import from the cache.
                // All devices imported in a DS during its startup sequence will
                // be searched first from the cache. This will slow down a litle
                // bit but with this code the TAC device is really imported from the
                // cache instead of from DB itself.
                //

                ApiUtil *au = ApiUtil::instance();
                if(au->in_server())
                {
                    if(db_tg != nullptr)
                    {
                        try
                        {
                            std::shared_ptr<DbServerCache> dsc = db_tg->get_db_cache();
                            if(dsc)
                            {
                                dev_import_list = dsc->import_tac_dev(dev);
                                imported_from_cache = true;
                            }
                        }
                        catch(Tango::DevFailed &)
                        {
                        }
                    }
                }

                if(!imported_from_cache)
                {
                    DeviceData send_name;
                    send_name << dev;
                    CALL_DB_SERVER("DbImportDevice", send_name, received_cmd);
                    received_cmd >> dev_import_list;
                }
            }
        }
        catch(Tango::DevFailed &)
        {
            access = tmp_access;
            throw;
        }

        dev_import.exported = 0;
        dev_import.name = dev;
        dev_import.ior = std::string((dev_import_list->svalue)[1]);
        dev_import.version = std::string((dev_import_list->svalue)[2]);
        dev_import.exported = dev_import_list->lvalue[0];

        //
        // If the db server returns the device class,
        // store device class in cache if not already there
        //

        if(dev_import_list->svalue.length() == 6)
        {
            omni_mutex_lock guard(map_mutex);

            auto pos = dev_class_cache.find(dev);
            if(pos == dev_class_cache.end())
            {
                std::string dev_class((dev_import_list->svalue)[5]);
                std::pair<std::map<std::string, std::string>::iterator, bool> status;
                status = dev_class_cache.insert(std::make_pair(dev, dev_class));
                if(!status.second)
                {
                    TangoSys_OMemStream o;
                    o << "Can't insert device class for device " << dev << " in device class cache" << std::ends;
                    TANGO_THROW_EXCEPTION(API_CantStoreDeviceClass, o.str());
                }
            }
        }

        access = tmp_access;
    }

    return (dev_import);
}

//-----------------------------------------------------------------------------
//
// Database::export_device() - public method to export device to the Database
//
//-----------------------------------------------------------------------------

void Database::export_device(const DbDevExportInfo &dev_export)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *dev_export_list = new DevVarStringArray;
    dev_export_list->length(5);
    (*dev_export_list)[0] = string_dup(dev_export.name.c_str());
    (*dev_export_list)[1] = string_dup(dev_export.ior.c_str());
    (*dev_export_list)[2] = string_dup(dev_export.host.c_str());

    std::ostringstream ostream;

    ostream << dev_export.pid << std::ends;
    std::string st = ostream.str();

    (*dev_export_list)[3] = string_dup(st.c_str());
    (*dev_export_list)[4] = string_dup(dev_export.version.c_str());

    send <<= dev_export_list;

    if(filedb != nullptr)
    {
        filedb->DbExportDevice(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbExportDevice", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::unexport_device() - public method to unexport a device from the Database
//
//-----------------------------------------------------------------------------

void Database::unexport_device(std::string dev)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= string_dup(dev.c_str());
    if(filedb != nullptr)
    {
        filedb->DbUnExportDevice(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbUnExportDevice", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::add_device() - public method to add a device to the Database
//
//-----------------------------------------------------------------------------

void Database::add_device(const DbDevInfo &dev_info)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *dev_info_list = new DevVarStringArray;
    dev_info_list->length(3);
    (*dev_info_list)[0] = string_dup(dev_info.server.c_str());
    (*dev_info_list)[1] = string_dup(dev_info.name.c_str());
    (*dev_info_list)[2] = string_dup(dev_info._class.c_str());
    send <<= dev_info_list;
    if(filedb != nullptr)
    {
        filedb->DbAddDevice(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbAddDevice", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::delete_device() - public method to delete a device from the Database
//
//-----------------------------------------------------------------------------

void Database::delete_device(std::string dev)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= dev.c_str();
    if(filedb != nullptr)
    {
        filedb->DbDeleteDevice(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteDevice", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::add_server() - public method to add a server to the Database
//
//-----------------------------------------------------------------------------

void Database::add_server(const std::string &server, const DbDevInfos &dev_infos)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *dev_info_list = new DevVarStringArray;
    dev_info_list->length((2 * dev_infos.size()) + 1);
    (*dev_info_list)[0] = string_dup(server.c_str());
    for(unsigned int i = 0; i < dev_infos.size(); i++)
    {
        (*dev_info_list)[(i * 2) + 1] = string_dup(dev_infos[i].name.c_str());
        (*dev_info_list)[(i * 2) + 2] = string_dup(dev_infos[i]._class.c_str());
    }
    send <<= dev_info_list;
    if(filedb != nullptr)
    {
        filedb->DbAddServer(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbAddServer", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::delete_server() - public method to delete a server from the Database
//
//-----------------------------------------------------------------------------

void Database::delete_server(const std::string &server)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= string_dup(server.c_str());
    if(filedb != nullptr)
    {
        filedb->DbDeleteServer(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteServer", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::export_server() - public method to export a server to the Database
//
//-----------------------------------------------------------------------------

void Database::export_server(const DbDevExportInfos &dev_export)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *dev_export_list = new DevVarStringArray;
    dev_export_list->length(5 * dev_export.size());

    std::ostringstream ostream;

    for(unsigned int i = 0; i < dev_export.size(); i++)
    {
        (*dev_export_list)[i * 5] = string_dup(dev_export[i].name.c_str());
        (*dev_export_list)[(i * 5) + 1] = string_dup(dev_export[i].ior.c_str());
        (*dev_export_list)[(i * 5) + 2] = string_dup(dev_export[i].host.c_str());
        ostream << dev_export[i].pid << std::ends;

        std::string st = ostream.str();
        (*dev_export_list)[(i * 5) + 3] = string_dup(st.c_str());
        (*dev_export_list)[(i * 5) + 4] = string_dup(dev_export[i].version.c_str());
    }
    send <<= dev_export_list;

    if(filedb != nullptr)
    {
        filedb->DbExportServer(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbExportServer", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::unexport_server() - public method to unexport a server from the Database
//
//-----------------------------------------------------------------------------

void Database::unexport_server(const std::string &server)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= server.c_str();
    if(filedb != nullptr)
    {
        filedb->DbUnExportServer(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbUnExportServer", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_server_info() - public method to return info for a server
//
//-----------------------------------------------------------------------------

DbServerInfo Database::get_server_info(const std::string &server)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= server.c_str();
    if(filedb != nullptr)
    {
        received = filedb->DbGetServerInfo(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetServerInfo", send, received);
    }

    const DevVarStringArray *server_info_list = nullptr;
    received.inout() >>= server_info_list;

    DbServerInfo server_info;
    if(server_info_list != nullptr)
    {
        server_info.name = std::string((*server_info_list)[0]);
        server_info.host = std::string((*server_info_list)[1]);
        server_info.mode = atoi((*server_info_list)[2]);
        server_info.level = atoi((*server_info_list)[3]);
    }
    else
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }

    return (server_info);
}

//-----------------------------------------------------------------------------
//
// Database::get_device_property() - public method to get a device property from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::get_device_property(std::string dev, DbData &db_data, std::shared_ptr<DbServerCache> db_cache)
{
    unsigned int i;
    Any_var received;
    const DevVarStringArray *property_values = nullptr;

    check_access_and_get();

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(dev.c_str());
    for(i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }

    if(!db_cache)
    {
        //
        // Get property(ies) from DB server
        //

        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
        Any send;

        send <<= property_names;

        if(filedb != nullptr)
        {
            received = filedb->DbGetDeviceProperty(send);
        }
        else
        {
            CALL_DB_SERVER("DbGetDeviceProperty", send, received);
        }

        received.inout() >>= property_values;
    }
    else
    {
        //
        // Try to get property(ies) from cache
        //

        try
        {
            property_values = db_cache->get_dev_property(property_names);
            delete property_names;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), DB_DeviceNotFoundInCache) == 0)
            {
                //
                // The device is not defined in the cache, call DB server
                //

                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
                Any send;

                send <<= property_names;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetDeviceProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetDeviceProperty", send, received);
                }
                received.inout() >>= property_values;
            }
            else
            {
                delete property_names;
                throw;
            }
        }
    }

    unsigned int n_props, index;
    std::stringstream iostream;

    iostream << (*property_values)[1].in() << std::ends;
    iostream >> n_props;
    index = 2;
    for(i = 0; i < n_props; i++)
    {
        int n_values;
        db_data[i].name = (*property_values)[index].in();
        index++;
        iostream.seekp(0);
        iostream.seekg(0);
        iostream.clear();
        iostream << (*property_values)[index].in() << std::ends;
        iostream >> n_values;
        index++;

        db_data[i].value_string.resize(n_values);
        if(n_values == 0)
        {
            index++; // skip dummy returned value " "
        }
        else
        {
            for(int j = 0; j < n_values; j++)
            {
                db_data[i].value_string[j] = (*property_values)[index].in();
                index++;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::put_device_property() - public method to put a device property from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::put_device_property(std::string dev, const DbData &db_data)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    int index;

    check_access_and_get();

    DevVarStringArray *property_values = new DevVarStringArray;
    property_values->length(2);
    index = 2;
    (*property_values)[0] = string_dup(dev.c_str());
    ostream << db_data.size();

    std::string st = ostream.str();
    (*property_values)[1] = string_dup(st.c_str());

    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        index++;
        property_values->length(index);
        (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
        ostream.seekp(0);
        ostream.clear();
        ostream << db_data[i].size() << std::ends;
        index++;
        property_values->length(index);

        std::string st = ostream.str();
        (*property_values)[index - 1] = string_dup(st.c_str());

        for(size_t j = 0; j < db_data[i].size(); j++)
        {
            index++;
            property_values->length(index);
            (*property_values)[index - 1] = string_dup(db_data[i].value_string[j].c_str());
        }
    }
    send <<= property_values;

    if(filedb != nullptr)
    {
        CORBA::Any_var the_any;
        the_any = filedb->DbPutDeviceProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbPutDeviceProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::delete_device_property() - public method to delete device properties
//                      from the Database
//
//-----------------------------------------------------------------------------

void Database::delete_device_property(std::string dev, const DbData &db_data)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(dev.c_str());
    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }
    send <<= property_names;

    if(filedb != nullptr)
    {
        filedb->DbDeleteDeviceProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteDeviceProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_device_attribute_property() - public method to get device
//                    attribute properties from the Database
//
//-----------------------------------------------------------------------------

void Database::get_device_attribute_property(std::string dev, DbData &db_data, std::shared_ptr<DbServerCache> db_cache)
{
    unsigned int i, j;
    Any_var received;
    const DevVarStringArray *property_values = nullptr;

    check_access_and_get();

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(dev.c_str());
    for(i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }

    if(!db_cache)
    {
        //
        // Get propery(ies) from DB server
        //

        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
        Any send;

        send <<= property_names;

        if(filedb != nullptr)
        {
            received = filedb->DbGetDeviceAttributeProperty(send);
        }
        else
        {
            try
            {
                if(serv_version >= 230)
                {
                    CALL_DB_SERVER("DbGetDeviceAttributeProperty2", send, received);
                }
                else
                {
                    CALL_DB_SERVER("DbGetDeviceAttributeProperty", send, received);
                }
            }
            catch(Tango::DevFailed &)
            {
                throw;
            }
        }
        received.inout() >>= property_values;
    }
    else
    {
        //
        // Try  to get property(ies) from cache
        //

        try
        {
            property_values = db_cache->get_dev_att_property(property_names);
            delete property_names;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), DB_DeviceNotFoundInCache) == 0)
            {
                //
                // The device is not in cache, ask the db server
                //

                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
                Any send;

                send <<= property_names;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetDeviceAttributeProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetDeviceAttributeProperty2", send, received);
                }
                received.inout() >>= property_values;
            }
            else
            {
                delete property_names;
                throw;
            }
        }
    }

    unsigned int n_attribs, index;
    int i_total_props;
    std::stringstream iostream;

    iostream << (*property_values)[1].in() << std::ends;
    iostream >> n_attribs;
    index = 2;
    if(serv_version < 230)
    {
        db_data.resize((property_values->length() / 2) - 1);
    }

    i_total_props = 0;

    if(serv_version < 230)
    {
        for(i = 0; i < n_attribs; i++)
        {
            unsigned short n_props;
            db_data[i_total_props].name = (*property_values)[index];
            index++;
            iostream.seekp(0);
            iostream.seekg(0);
            iostream.clear();
            iostream << (*property_values)[index].in() << std::ends;
            iostream >> n_props;
            db_data[i_total_props] << n_props;
            i_total_props++;
            index++;
            for(j = 0; j < n_props; j++)
            {
                db_data[i_total_props].name = (*property_values)[index];
                index++;
                db_data[i_total_props].value_string.resize(1);
                db_data[i_total_props].value_string[0] = (*property_values)[index];
                index++;
                i_total_props++;
            }
        }
    }
    else
    {
        long old_size = 0;
        for(i = 0; i < n_attribs; i++)
        {
            old_size++;
            db_data.resize(old_size);
            unsigned short n_props;
            db_data[i_total_props].name = (*property_values)[index];
            index++;
            iostream.seekp(0);
            iostream.seekg(0);
            iostream.clear();
            iostream << (*property_values)[index].in() << std::ends;
            iostream >> n_props;
            db_data[i_total_props] << n_props;
            db_data.resize(old_size + n_props);
            old_size = old_size + n_props;
            i_total_props++;
            index++;
            for(j = 0; j < n_props; j++)
            {
                db_data[i_total_props].name = (*property_values)[index];
                index++;
                int n_values;
                iostream.seekp(0);
                iostream.seekg(0);
                iostream.clear();
                iostream << (*property_values)[index].in() << std::ends;
                iostream >> n_values;
                index++;

                db_data[i_total_props].value_string.resize(n_values);
                if(n_values == 0)
                {
                    index++; // skip dummy returned value " "
                }
                else
                {
                    for(int k = 0; k < n_values; k++)
                    {
                        db_data[i_total_props].value_string[k] = (*property_values)[index].in();
                        index++;
                    }
                }
                i_total_props++;
            }
        }
    }

    TANGO_LOG_DEBUG << "Leaving get_device_attribute_property" << std::endl;
}

//-----------------------------------------------------------------------------
//
// Database::put_device_attribute_property() - public method to put device
//         attribute properties into the Database
//
//-----------------------------------------------------------------------------

void Database::put_device_attribute_property(std::string dev, const DbData &db_data)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    int index, n_attribs;
    bool retry = true;

    check_access_and_get();

    while(retry)
    {
        DevVarStringArray *property_values = new DevVarStringArray;
        property_values->length(2);
        index = 2;
        (*property_values)[0] = string_dup(dev.c_str());
        n_attribs = 0;

        if(serv_version >= 230)
        {
            for(unsigned int i = 0; i < db_data.size();)
            {
                short n_props = 0;
                index++;
                property_values->length(index);
                (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
                db_data[i] >> n_props;
                ostream.seekp(0);
                ostream.clear();
                ostream << n_props << std::ends;
                index++;
                property_values->length(index);

                std::string st = ostream.str();
                (*property_values)[index - 1] = string_dup(st.c_str());

                for(int j = 0; j < n_props; j++)
                {
                    index++;
                    property_values->length(index);
                    (*property_values)[index - 1] = string_dup(db_data[i + j + 1].name.c_str());
                    index++;
                    property_values->length(index);
                    int prop_size = db_data[i + j + 1].size();
                    ostream.seekp(0);
                    ostream.clear();
                    ostream << prop_size << std::ends;

                    std::string st = ostream.str();
                    (*property_values)[index - 1] = string_dup(st.c_str());

                    property_values->length(index + prop_size);
                    for(int q = 0; q < prop_size; q++)
                    {
                        (*property_values)[index + q] = string_dup(db_data[i + j + 1].value_string[q].c_str());
                    }
                    index = index + prop_size;
                }
                i = i + n_props + 1;
                n_attribs++;
            }

            ostream.seekp(0);
            ostream.clear();
            ostream << n_attribs << std::ends;

            std::string st = ostream.str();
            (*property_values)[1] = string_dup(st.c_str());
        }
        else
        {
            for(unsigned int i = 0; i < db_data.size();)
            {
                short n_props = 0;
                index++;
                property_values->length(index);
                (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
                db_data[i] >> n_props;
                ostream.seekp(0);
                ostream.clear();
                ostream << n_props << std::ends;
                index++;
                property_values->length(index);

                std::string st = ostream.str();
                (*property_values)[index - 1] = string_dup(st.c_str());

                for(int j = 0; j < n_props; j++)
                {
                    index++;
                    property_values->length(index);
                    (*property_values)[index - 1] = string_dup(db_data[i + j + 1].name.c_str());
                    index++;
                    property_values->length(index);
                    (*property_values)[index - 1] = string_dup(db_data[i + j + 1].value_string[0].c_str());
                }
                i = i + n_props + 1;
                n_attribs++;
            }
            ostream.seekp(0);
            ostream.clear();
            ostream << n_attribs << std::ends;

            std::string st = ostream.str();
            (*property_values)[1] = string_dup(st.c_str());
        }
        send <<= property_values;

        if(filedb != nullptr)
        {
            filedb->DbPutDeviceAttributeProperty(send);
            retry = false;
        }
        else
        {
            try
            {
                if(serv_version >= 230)
                {
                    CALL_DB_SERVER_NO_RET("DbPutDeviceAttributeProperty2", send);
                }
                else
                {
                    CALL_DB_SERVER_NO_RET("DbPutDeviceAttributeProperty", send);
                }
                retry = false;
            }
            catch(Tango::DevFailed &)
            {
                throw;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::delete_device_attribute_property() - public method to delete device
//         attribute properties from the Database
//
//-----------------------------------------------------------------------------

void Database::delete_device_attribute_property(std::string dev, const DbData &db_data)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *property_values = new DevVarStringArray;
    unsigned int nb_prop = db_data.size() - 1;
    property_values->length(nb_prop + 2);
    (*property_values)[0] = string_dup(dev.c_str());
    (*property_values)[1] = string_dup(db_data[0].name.c_str());
    for(unsigned int i = 0; i < nb_prop; i++)
    {
        (*property_values)[i + 2] = string_dup(db_data[i + 1].name.c_str());
    }

    send <<= property_values;

    if(filedb != nullptr)
    {
        filedb->DbDeleteDeviceAttributeProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteDeviceAttributeProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_class_property() - public method to get class properties from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::get_class_property(std::string device_class, DbData &db_data, std::shared_ptr<DbServerCache> db_cache)
{
    unsigned int i;
    const DevVarStringArray *property_values = nullptr;
    Any_var received;

    //
    // Parameters for db server call
    //

    DevVarStringArray *property_names = new DevVarStringArray;

    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(device_class.c_str());
    for(i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }

    //
    // Call db server or get data from cache
    //

    if(!db_cache)
    {
        //
        // Call DB server
        //

        Any send;
        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

        send <<= property_names;

        if(filedb != nullptr)
        {
            received = filedb->DbGetClassProperty(send);
        }
        else
        {
            CALL_DB_SERVER("DbGetClassProperty", send, received);
        }

        received.inout() >>= property_values;
    }
    else
    {
        //
        // Try to get property(ies) from cache
        //

        try
        {
            property_values = db_cache->get_class_property(property_names);
            delete property_names;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), DB_ClassNotFoundInCache) == 0)
            {
                //
                // The class is not defined in cache, try in DB server
                //

                Any send;
                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

                send <<= property_names;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetClassProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetClassProperty", send, received);
                }

                received.inout() >>= property_values;
            }
            else
            {
                delete property_names;
                throw;
            }
        }
    }

    //
    // Build data returned to caller from those received from db server or from cache
    //

    unsigned int n_props, index;
    std::stringstream iostream;

    iostream << (*property_values)[1].in() << std::ends;
    iostream >> n_props;
    index = 2;
    for(i = 0; i < n_props; i++)
    {
        int n_values;
        db_data[i].name = (*property_values)[index];
        index++;
        iostream.seekp(0);
        iostream.seekg(0);
        iostream.clear();
        iostream << (*property_values)[index].in() << std::ends;
        index++;
        iostream >> n_values;

        db_data[i].value_string.resize(n_values);
        for(int j = 0; j < n_values; j++)
        {
            db_data[i].value_string[j] = (*property_values)[index];
            index++;
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::put_class_property() - public method to put class properties from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::put_class_property(std::string device_class, const DbData &db_data)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    int index;

    check_access_and_get();

    DevVarStringArray *property_values = new DevVarStringArray;
    property_values->length(2);
    index = 2;
    (*property_values)[0] = string_dup(device_class.c_str());
    ostream << db_data.size() << std::ends;

    std::string st = ostream.str();
    (*property_values)[1] = string_dup(st.c_str());

    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        index++;
        property_values->length(index);
        (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
        ostream.seekp(0);
        ostream.clear();
        ostream << db_data[i].size() << std::ends;
        index++;
        property_values->length(index);

        std::string st = ostream.str();
        (*property_values)[index - 1] = string_dup(st.c_str());

        for(size_t j = 0; j < db_data[i].size(); j++)
        {
            index++;
            property_values->length(index);
            (*property_values)[index - 1] = string_dup(db_data[i].value_string[j].c_str());
        }
    }
    send <<= property_values;

    if(filedb != nullptr)
    {
        CORBA::Any_var the_any;
        the_any = filedb->DbPutClassProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbPutClassProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::delete_class_property() - public method to delete class properties from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::delete_class_property(std::string device_class, const DbData &db_data)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(device_class.c_str());
    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }
    send <<= property_names;

    if(filedb != nullptr)
    {
        filedb->DbDeleteClassProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteClassProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_class_attribute_property() - public method to get class attribute
//         properties from the Database
//
//-----------------------------------------------------------------------------

void Database::get_class_attribute_property(std::string device_class,
                                            DbData &db_data,
                                            std::shared_ptr<DbServerCache> db_cache)
{
    unsigned int i;
    Any_var received;
    const DevVarStringArray *property_values = nullptr;

    check_access_and_get();

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(device_class.c_str());
    for(i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }

    if(!db_cache)
    {
        //
        // Get property(ies) from DB server
        //

        Any send;
        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

        send <<= property_names;

        if(filedb != nullptr)
        {
            received = filedb->DbGetClassAttributeProperty(send);
        }
        else
        {
            try
            {
                if(serv_version >= 230)
                {
                    CALL_DB_SERVER("DbGetClassAttributeProperty2", send, received);
                }
                else
                {
                    CALL_DB_SERVER("DbGetClassAttributeProperty", send, received);
                }
            }
            catch(Tango::DevFailed &)
            {
                throw;
            }
        }
        received.inout() >>= property_values;
    }
    else
    {
        //
        // Try to get property(ies) from cache
        //

        try
        {
            property_values = db_cache->get_class_att_property(property_names);
            delete property_names;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), DB_ClassNotFoundInCache) == 0)
            {
                //
                // The class is not defined in cache, get property(ies) from DB server
                //

                Any send;
                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

                send <<= property_names;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetClassAttributeProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetClassAttributeProperty2", send, received);
                }
                received.inout() >>= property_values;
            }
            else
            {
                delete property_names;
                throw;
            }
        }
    }

    unsigned int n_attribs, index;
    int i_total_props;
    std::stringstream iostream;

    iostream << (*property_values)[1].in() << std::ends;
    iostream >> n_attribs;
    index = 2;
    if(serv_version < 230)
    {
        db_data.resize((property_values->length() / 2) - 1);
    }
    i_total_props = 0;

    if(serv_version < 230)
    {
        for(i = 0; i < n_attribs; i++)
        {
            short n_props;
            db_data[i_total_props].name = (*property_values)[index];
            index++;
            iostream.seekp(0);
            iostream.seekg(0);
            iostream.clear();
            iostream << (*property_values)[index].in() << std::ends;
            index++;
            iostream >> n_props;
            db_data[i_total_props] << n_props;
            i_total_props++;
            for(int j = 0; j < n_props; j++)
            {
                db_data[i_total_props].name = (*property_values)[index];
                index++;
                db_data[i_total_props].value_string.resize(1);
                db_data[i_total_props].value_string[0] = (*property_values)[index];
                index++;
                i_total_props++;
            }
        }
    }
    else
    {
        long old_size = 0;
        for(i = 0; i < n_attribs; i++)
        {
            old_size++;
            db_data.resize(old_size);
            short n_props;
            db_data[i_total_props].name = (*property_values)[index];
            index++;
            iostream.seekp(0);
            iostream.seekg(0);
            iostream.clear();
            iostream << (*property_values)[index].in() << std::ends;
            iostream >> n_props;
            db_data[i_total_props] << n_props;
            db_data.resize(old_size + n_props);
            old_size = old_size + n_props;
            i_total_props++;
            index++;
            for(int j = 0; j < n_props; j++)
            {
                db_data[i_total_props].name = (*property_values)[index];
                index++;
                int n_values;
                iostream.seekp(0);
                iostream.seekg(0);
                iostream.clear();
                iostream << (*property_values)[index].in() << std::ends;
                iostream >> n_values;
                index++;

                db_data[i_total_props].value_string.resize(n_values);
                if(n_values == 0)
                {
                    index++; // skip dummy returned value ""
                }
                else
                {
                    for(int k = 0; k < n_values; k++)
                    {
                        db_data[i_total_props].value_string[k] = (*property_values)[index].in();
                        index++;
                    }
                }
                i_total_props++;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::put_class_attribute_property() - public method to put class
//         attribute properties into the Database
//
//-----------------------------------------------------------------------------

void Database::put_class_attribute_property(std::string device_class, const DbData &db_data)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    int index, n_attribs;
    bool retry = true;

    check_access_and_get();

    while(retry)
    {
        DevVarStringArray *property_values = new DevVarStringArray;
        property_values->length(2);
        index = 2;
        (*property_values)[0] = string_dup(device_class.c_str());
        n_attribs = 0;

        if(serv_version >= 230)
        {
            for(unsigned int i = 0; i < db_data.size();)
            {
                short n_props = 0;
                index++;
                property_values->length(index);
                (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
                db_data[i] >> n_props;
                ostream.seekp(0);
                ostream.clear();
                ostream << n_props << std::ends;
                index++;
                property_values->length(index);

                std::string st = ostream.str();
                (*property_values)[index - 1] = string_dup(st.c_str());

                for(int j = 0; j < n_props; j++)
                {
                    index++;
                    property_values->length(index);
                    (*property_values)[index - 1] = string_dup(db_data[i + j + 1].name.c_str());
                    index++;
                    property_values->length(index);
                    int prop_size = db_data[i + j + 1].size();
                    ostream.seekp(0);
                    ostream.clear();
                    ostream << prop_size << std::ends;

                    std::string st = ostream.str();
                    (*property_values)[index - 1] = string_dup(st.c_str());

                    property_values->length(index + prop_size);
                    for(int q = 0; q < prop_size; q++)
                    {
                        (*property_values)[index + q] = string_dup(db_data[i + j + 1].value_string[q].c_str());
                    }
                    index = index + prop_size;
                }
                i = i + n_props + 1;
                n_attribs++;
            }

            ostream.seekp(0);
            ostream.clear();
            ostream << n_attribs << std::ends;

            std::string st = ostream.str();
            (*property_values)[1] = string_dup(st.c_str());
        }
        else
        {
            for(unsigned int i = 0; i < db_data.size();)
            {
                short n_props = 0;
                index++;
                property_values->length(index);
                (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
                db_data[i] >> n_props;
                ostream.seekp(0);
                ostream.clear();
                ostream << n_props << std::ends;
                index++;
                property_values->length(index);

                std::string st = ostream.str();
                (*property_values)[index - 1] = string_dup(st.c_str());

                for(int j = 0; j < n_props; j++)
                {
                    index++;
                    property_values->length(index);
                    (*property_values)[index - 1] = string_dup(db_data[i + j + 1].name.c_str());
                    index++;
                    property_values->length(index);
                    (*property_values)[index - 1] = string_dup(db_data[i + j + 1].value_string[0].c_str());
                }
                i = i + n_props + 1;
                n_attribs++;
            }
            ostream.seekp(0);
            ostream.clear();
            ostream << n_attribs << std::ends;

            std::string st = ostream.str();
            (*property_values)[1] = string_dup(st.c_str());
        }
        send <<= property_values;

        if(filedb != nullptr)
        {
            filedb->DbPutClassAttributeProperty(send);
            retry = false;
        }
        else
        {
            if(serv_version >= 230)
            {
                CALL_DB_SERVER_NO_RET("DbPutClassAttributeProperty2", send);
            }
            else
            {
                CALL_DB_SERVER_NO_RET("DbPutClassAttributeProperty", send);
            }
            retry = false;
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::delete_class_attribute_property() - public method to delete class
//         attribute properties from the Database
//
//-----------------------------------------------------------------------------

void Database::delete_class_attribute_property(std::string device_class, const DbData &db_data)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *property_values = new DevVarStringArray;
    unsigned int nb_prop = db_data.size() - 1;
    property_values->length(nb_prop + 2);
    (*property_values)[0] = string_dup(device_class.c_str());
    (*property_values)[1] = string_dup(db_data[0].name.c_str());
    for(unsigned int i = 0; i < nb_prop; i++)
    {
        (*property_values)[i + 2] = string_dup(db_data[i + 1].name.c_str());
    }

    send <<= property_values;

    if(filedb != nullptr)
    {
        filedb->DbDeleteClassAttributeProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteClassAttributeProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_device_name() - public method to get device names from
//                               the Database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_name(const std::string &d_server, const std::string &d_class)
{
    return get_device_name(d_server, d_class, nullptr);
}

DbDatum Database::get_device_name(const std::string &device_server,
                                  const std::string &device_class,
                                  std::shared_ptr<DbServerCache> db_cache)
{
    Any_var received;
    const DevVarStringArray *device_names = nullptr;

    check_access_and_get();

    DevVarStringArray *device_server_class = new DevVarStringArray;
    device_server_class->length(2);
    (*device_server_class)[0] = string_dup(device_server.c_str());
    (*device_server_class)[1] = string_dup(device_class.c_str());

    if(!db_cache)
    {
        Any send;
        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

        send <<= device_server_class;

        if(filedb != nullptr)
        {
            received = filedb->DbGetDeviceList(send);
        }
        else
        {
            CALL_DB_SERVER("DbGetDeviceList", send, received);
        }
        received.inout() >>= device_names;
    }
    else
    {
        device_names = db_cache->get_dev_list(device_server_class);
        delete device_server_class;
    }

    DbDatum db_datum;
    int n_devices;
    n_devices = device_names->length();
    db_datum.name = device_server;
    db_datum.value_string.resize(n_devices);
    for(int i = 0; i < n_devices; i++)
    {
        db_datum.value_string[i] = (*device_names)[i];
    }

    return db_datum;
}

namespace
{
// Implementation of get_device_* functions which all return a
// std::vector<std::string> in their DbDatum.
//
// db_function should have the following signature:
//
//      void db_function(Any &, Any_var &)
//
//  and should invoke the required command on either filedb or the real db
//  server.
template <typename Func>
DbDatum get_device_list_impl(const std::string &filter, Func db_function)
{
    Any send;
    Any_var received;

    send <<= filter.c_str();
    db_function(send, received);

    const DevVarStringArray *device_names = nullptr;
    received.inout() >>= device_names;

    DbDatum db_datum;
    if(device_names == nullptr)
    {
        TANGO_THROW_EXCEPTION(
            API_IncoherentDbData,
            "Database response could not be parsed into DevVarStringArray. Check database consistency and transport.");
    }
    else
    {
        const ULong n_devices = device_names->length();
        db_datum.name = filter;
        db_datum.value_string.resize(n_devices);
        for(ULong i = 0; i < n_devices; i++)
        {
            db_datum.value_string[i] = (*device_names)[i];
        }
    }

    return db_datum;
}
} // namespace

//-----------------------------------------------------------------------------
//
// Database::get_device_exported() - public method to get exported device names from
//                               the Database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_exported(const std::string &filter)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    return get_device_list_impl(filter,
                                [this](Any &send, Any_var &received)
                                {
                                    if(filedb != nullptr)
                                    {
                                        received = filedb->DbGetDeviceExportedList(send);
                                    }
                                    else
                                    {
                                        CALL_DB_SERVER("DbGetDeviceExportedList", send, received);
                                    }
                                });
}

//-----------------------------------------------------------------------------
//
// Database::get_device_defined() - public method to get all matching device names
//                               from the Database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_defined(const std::string &filter)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    return get_device_list_impl(filter,
                                [this](Any &send, Any_var &received)
                                {
                                    if(filedb != nullptr)
                                    {
                                        received = filedb->DbGetDeviceWideList(send);
                                    }
                                    else
                                    {
                                        CALL_DB_SERVER("DbGetDeviceWideList", send, received);
                                    }
                                });
}

//-----------------------------------------------------------------------------
//
// Database::get_device_member() - public method to get device members from
//                               the Database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_member(const std::string &wildcard)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    return get_device_list_impl(wildcard,
                                [this](Any &send, Any_var &received)
                                {
                                    if(filedb != nullptr)
                                    {
                                        received = filedb->DbGetDeviceMemberList(send);
                                    }
                                    else
                                    {
                                        CALL_DB_SERVER("DbGetDeviceMemberList", send, received);
                                    }
                                });
}

//-----------------------------------------------------------------------------
//
// Database::get_device_family() - public method to get device familys from
//                               the Database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_family(const std::string &wildcard)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    return get_device_list_impl(wildcard,
                                [this](Any &send, Any_var &received)
                                {
                                    if(filedb != nullptr)
                                    {
                                        received = filedb->DbGetDeviceFamilyList(send);
                                    }
                                    else
                                    {
                                        CALL_DB_SERVER("DbGetDeviceFamilyList", send, received);
                                    }
                                });
}

//-----------------------------------------------------------------------------
//
// Database::get_device_domain() - public method to get device domains from
//                               the Database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_domain(const std::string &wildcard)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    return get_device_list_impl(wildcard,
                                [this](Any &send, Any_var &received)
                                {
                                    if(filedb != nullptr)
                                    {
                                        received = filedb->DbGetDeviceDomainList(send);
                                    }
                                    else
                                    {
                                        CALL_DB_SERVER("DbGetDeviceDomainList", send, received);
                                    }
                                });
}

//-----------------------------------------------------------------------------
//
// Database::get_property() - public method to get object property from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::get_property_forced(std::string obj, DbData &db_data, std::shared_ptr<DbServerCache> dsc)
{
    WriterLock guard(con_to_mon);

    AccessControlType tmp_access = access;
    access = ACCESS_WRITE;
    try
    {
        get_property(obj, db_data, dsc);
    }
    catch(Tango::DevFailed &)
    {
    }
    access = tmp_access;
}

void Database::get_property(std::string obj, DbData &db_data, std::shared_ptr<DbServerCache> db_cache)
{
    unsigned int i;
    Any_var received;
    const DevVarStringArray *property_values = nullptr;

    {
        WriterLock guard(con_to_mon);

        if((access == ACCESS_READ) && (!access_checked))
        {
            check_access();
        }
    }

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(obj.c_str());
    for(i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }

    if(!db_cache)
    {
        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
        Any send;

        send <<= property_names;

        if(filedb != nullptr)
        {
            received = filedb->DbGetProperty(send);
        }
        else
        {
            CALL_DB_SERVER("DbGetProperty", send, received);
        }

        received.inout() >>= property_values;
    }
    else
    {
        //
        // Try to get property(ies) from cache
        //

        try
        {
            property_values = db_cache->get_obj_property(property_names);
            delete property_names;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), DB_DeviceNotFoundInCache) == 0)
            {
                //
                // The object is not defined in the cache, call DB server
                //

                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
                Any send;

                send <<= property_names;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetDeviceProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetDeviceProperty", send, received);
                }
                received.inout() >>= property_values;
            }
            else
            {
                delete property_names;
                throw;
            }
        }
    }

    unsigned int n_props, index;
    std::stringstream iostream;

    iostream << (*property_values)[1].in() << std::ends;
    iostream >> n_props;
    index = 2;
    for(i = 0; i < n_props; i++)
    {
        int n_values;
        db_data[i].name = (*property_values)[index];
        index++;
        iostream.seekp(0);
        iostream.seekg(0);
        iostream.clear();
        iostream << (*property_values)[index].in() << std::ends;
        iostream >> n_values;
        index++;

        db_data[i].value_string.resize(n_values);
        if(n_values == 0)
        {
            index++; // skip dummy returned value " "
        }
        else
        {
            for(int j = 0; j < n_values; j++)
            {
                db_data[i].value_string[j] = (*property_values)[index];
                index++;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::put_property() - public method to put object property from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::put_property(std::string obj, const DbData &db_data)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    int index;

    check_access_and_get();

    DevVarStringArray *property_values = new DevVarStringArray;
    property_values->length(2);
    index = 2;
    (*property_values)[0] = string_dup(obj.c_str());
    ostream << db_data.size();

    std::string st = ostream.str();
    (*property_values)[1] = string_dup(st.c_str());

    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        index++;
        property_values->length(index);
        (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
        ostream.seekp(0);
        ostream.clear();
        ostream << db_data[i].size() << std::ends;
        index++;
        property_values->length(index);

        std::string st = ostream.str();
        (*property_values)[index - 1] = string_dup(st.c_str());

        for(size_t j = 0; j < db_data[i].size(); j++)
        {
            index++;
            property_values->length(index);
            (*property_values)[index - 1] = string_dup(db_data[i].value_string[j].c_str());
        }
    }
    send <<= property_values;

    if(filedb != nullptr)
    {
        filedb->DbPutProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbPutProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::delete_property() - public method to delete object properties from
//                                   the Database
//
//-----------------------------------------------------------------------------

void Database::delete_property(std::string obj, const DbData &db_data)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(obj.c_str());
    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }
    send <<= property_names;

    if(filedb != nullptr)
    {
        filedb->DbDeleteProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_device_alias() - public method to get device name from
//                               its alias
//
//-----------------------------------------------------------------------------

void Database::get_device_alias(std::string alias, std::string &dev_name)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= alias.c_str();

    if(filedb != nullptr)
    {
        received = filedb->DbGetAliasDevice(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetAliasDevice", send, received);
    }
    const char *dev_name_tmp = nullptr;
    received.inout() >>= dev_name_tmp;
    dev_name = dev_name_tmp;
}

//-----------------------------------------------------------------------------
//
// Database::get_alias() - public method to get alias name from
//                         device name
//
//-----------------------------------------------------------------------------

void Database::get_alias(std::string dev_name, std::string &alias_name)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= dev_name.c_str();

    if(filedb != nullptr)
    {
        received = filedb->DbGetDeviceAlias(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetDeviceAlias", send, received);
    }
    const char *dev_name_tmp = nullptr;
    received.inout() >>= dev_name_tmp;
    alias_name = dev_name_tmp;
}

//-----------------------------------------------------------------------------
//
// Database::get_attribute_alias() - public method to get attribute name from
//                                   its alias
//
//-----------------------------------------------------------------------------

void Database::get_attribute_alias(std::string attr_alias, std::string &attr_name)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= attr_alias.c_str();

    if(filedb != nullptr)
    {
        received = filedb->DbGetAttributeAlias(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetAttributeAlias", send, received);
    }
    const char *attr_name_tmp = nullptr;
    received.inout() >>= attr_name_tmp;

    if(attr_name_tmp == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }
    else
    {
        attr_name = attr_name_tmp;
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_device_alias_list() - public method to get device alias list.
//                       Wildcard (*) is suported
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_alias_list(const std::string &alias)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    send <<= alias.c_str();

    if(filedb != nullptr)
    {
        received = filedb->DbGetDeviceAliasList(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetDeviceAliasList", send, received);
    }
    const DevVarStringArray *alias_array = nullptr;
    received.inout() >>= alias_array;

    DbDatum db_datum;

    if(alias_array == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }
    else
    {
        int n_aliases;
        n_aliases = alias_array->length();
        db_datum.name = alias;
        db_datum.value_string.resize(n_aliases);
        for(int i = 0; i < n_aliases; i++)
        {
            db_datum.value_string[i] = (*alias_array)[i];
        }
    }

    return db_datum;
}

//-----------------------------------------------------------------------------
//
// Database::get_attribute_alias_list() - public method to get attribute alias list
//                          Wildcard (*) is suported
//
//-----------------------------------------------------------------------------

DbDatum Database::get_attribute_alias_list(const std::string &alias)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    send <<= alias.c_str();

    if(filedb != nullptr)
    {
        received = filedb->DbGetAttributeAliasList(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetAttributeAliasList", send, received);
    }
    const DevVarStringArray *alias_array = nullptr;
    received.inout() >>= alias_array;

    DbDatum db_datum;

    if(alias_array == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }
    else
    {
        int n_aliases;
        n_aliases = alias_array->length();
        db_datum.name = alias;
        db_datum.value_string.resize(n_aliases);
        for(int i = 0; i < n_aliases; i++)
        {
            db_datum.value_string[i] = (*alias_array)[i];
        }
    }

    return db_datum;
}

//-----------------------------------------------------------------------------
//
// Database::make_string_array() - Build a string array DbDatum from received
//
//-----------------------------------------------------------------------------

DbDatum Database::make_string_array(std::string name, Any_var &received)
{
    const DevVarStringArray *prop_list = nullptr;
    DbDatum db_datum;

    received.inout() >>= prop_list;
    if(prop_list == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }
    else
    {
        int n_props;

        n_props = prop_list->length();
        db_datum.name = name;
        db_datum.value_string.resize(n_props);
        for(int i = 0; i < n_props; i++)
        {
            db_datum.value_string[i] = (*prop_list)[i];
        }
    }

    return db_datum;
}

//-----------------------------------------------------------------------------
//
// Database::get_device_property_list() - public method to get a device property
//                      list from the Database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_property_list(const std::string &dev, const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *sent_names = new DevVarStringArray;
    sent_names->length(2);
    (*sent_names)[0] = string_dup(dev.c_str());
    (*sent_names)[1] = string_dup(wildcard.c_str());

    send <<= sent_names;

    CALL_DB_SERVER("DbGetDevicePropertyList", send, received);

    return make_string_array(dev, received);
}

void Database::get_device_property_list(std::string &dev,
                                        const std::string &wildcard,
                                        std::vector<std::string> &prop_list,
                                        std::shared_ptr<DbServerCache> db_cache)
{
    if(!db_cache)
    {
        DbDatum db = get_device_property_list(dev, const_cast<std::string &>(wildcard));
        prop_list = db.value_string;
    }
    else
    {
        //
        // Try to get property names from cache
        //

        DevVarStringArray send_seq;
        send_seq.length(2);
        send_seq[0] = Tango::string_dup(dev.c_str());
        send_seq[1] = Tango::string_dup(wildcard.c_str());

        try
        {
            const DevVarStringArray *recev;
            recev = db_cache->get_device_property_list(&send_seq);
            prop_list << *recev;
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), DB_DeviceNotFoundInCache) == 0)
            {
                //
                // The object is not defined in the cache, call DB server
                //

                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
                Any send;
                Any_var received;

                send <<= send_seq;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetDeviceProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetDevicePropertyList", send, received);
                }

                DbDatum db_d = make_string_array(dev, received);
                prop_list = db_d.value_string;
            }
            else
            {
                throw;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_host_list() - Query the database for a list of host registered.
//
//-----------------------------------------------------------------------------

DbDatum Database::get_host_list()
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= "*";

    CALL_DB_SERVER("DbGetHostList", send, received);

    return make_string_array(std::string("host"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_host_list() - Query the database for a list of host registred.
//
//-----------------------------------------------------------------------------

DbDatum Database::get_host_list(const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= wildcard.c_str();

    CALL_DB_SERVER("DbGetHostList", send, received);

    return make_string_array(std::string("host"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_server_class_list() - Query the database for a list of classes
//                                     instancied for a server
//
//-----------------------------------------------------------------------------

DbDatum Database::get_server_class_list(const std::string &servname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= servname.c_str();

    CALL_DB_SERVER("DbGetDeviceServerClassList", send, received);

    const DevVarStringArray *class_list = nullptr;
    received.inout() >>= class_list;

    if(class_list == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }

    // Extract the DServer class (if there is)
    DbDatum db_datum{servname};
    CORBA::ULong nb_classes = class_list->length();

    for(CORBA::ULong i = 0, j = 0; i < nb_classes; i++)
    {
        if(TG_strcasecmp((*class_list)[i], "DServer") != 0)
        {
            db_datum.value_string.resize(j + 1);
            db_datum.value_string[j] = (*class_list)[i];
            j++;
        }
    }

    return db_datum;
}

//-----------------------------------------------------------------------------
//
// Database::get_server_name_list() - Query the database for a list of server
//                                    names registred in the database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_server_name_list()
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= "*";

    CALL_DB_SERVER("DbGetServerNameList", send, received);

    return make_string_array(std::string("server"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_instance_name_list() - Query the database for a list of instance
//                                      names registred for specified server name
//
//-----------------------------------------------------------------------------

DbDatum Database::get_instance_name_list(const std::string &servname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= servname.c_str();

    CALL_DB_SERVER("DbGetInstanceNameList", send, received);

    return make_string_array(servname, received);
}

//-----------------------------------------------------------------------------
//
// Database::get_server_list() - Query the database for a list of servers registred
//                               in the database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_server_list()
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= "*";

    CALL_DB_SERVER("DbGetServerList", send, received);

    return make_string_array(std::string("server"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_server_list() - Query the database for a list of servers registred
//                               in the database
//
//-----------------------------------------------------------------------------

DbDatum Database::get_server_list(const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= wildcard.c_str();

    CALL_DB_SERVER("DbGetServerList", send, received);

    return make_string_array(std::string("server"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_host_server_list() - Query the database for a list of servers
//                                    registred on the specified host
//
//-----------------------------------------------------------------------------

DbDatum Database::get_host_server_list(const std::string &hostname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= hostname.c_str();

    CALL_DB_SERVER("DbGetHostServerList", send, received);

    return make_string_array(std::string("server"), received);
}

//-----------------------------------------------------------------------------
//
// Database::put_server_info() - Add/update server information in databse
//
//-----------------------------------------------------------------------------

void Database::put_server_info(const DbServerInfo &info)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    DevVarStringArray *serv_info = new DevVarStringArray;

    check_access_and_get();

    serv_info->length(4);
    (*serv_info)[0] = string_dup(info.name.c_str());
    (*serv_info)[1] = string_dup(info.host.c_str());

    ostream.seekp(0);
    ostream.clear();
    ostream << info.mode << std::ends;

    std::string st = ostream.str();
    (*serv_info)[2] = string_dup(st.c_str());

    ostream.seekp(0);
    ostream.clear();
    ostream << info.level << std::ends;

    st = ostream.str();
    (*serv_info)[3] = string_dup(st.c_str());

    send <<= serv_info;

    CALL_DB_SERVER_NO_RET("DbPutServerInfo", send);
}

//-----------------------------------------------------------------------------
//
// Database::delete_server_info() - Delete server information in databse
//
//-----------------------------------------------------------------------------

void Database::delete_server_info(const std::string &servname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= servname.c_str();

    CALL_DB_SERVER_NO_RET("DbDeleteServerInfo", send);
}

//-----------------------------------------------------------------------------
//
// Database::get_device_class_list() - Query the database for server devices
//                                     and classes.
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_class_list(const std::string &servname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= servname.c_str();

    CALL_DB_SERVER("DbGetDeviceClassList", send, received);

    return make_string_array(std::string("server"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_object_list() - Query the database for a list of object
//                               (ie non-device) for which properties are defined
//
//-----------------------------------------------------------------------------

DbDatum Database::get_object_list(const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= wildcard.c_str();

    CALL_DB_SERVER("DbGetObjectList", send, received);

    return make_string_array(std::string("object"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_object_property_list() - Query the database for a list of
//                                        properties defined for the specified
//                                        object.
//
//-----------------------------------------------------------------------------

DbDatum Database::get_object_property_list(const std::string &objname, const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *obj_info = new DevVarStringArray;

    obj_info->length(2);
    (*obj_info)[0] = string_dup(objname.c_str());
    (*obj_info)[1] = string_dup(wildcard.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetPropertyList", send, received);

    return make_string_array(std::string("object"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_class_property_list() - Query the database for a list of
//                                       properties defined for the specified
//                                       class.
//
//-----------------------------------------------------------------------------

DbDatum Database::get_class_property_list(const std::string &classname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= classname.c_str();

    CALL_DB_SERVER("DbGetClassPropertyList", send, received);

    return make_string_array(std::string("class"), received);
}

DbDatum Database::get_class_property_list(const std::string &classname, const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *sent_names = new DevVarStringArray;
    sent_names->length(2);
    (*sent_names)[0] = string_dup(classname.c_str());
    (*sent_names)[1] = string_dup(wildcard.c_str());

    send <<= sent_names;

    CALL_DB_SERVER("DbGetClassPropertyListWildcard", send, received);

    return make_string_array(std::string("class"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_class_for_device() - Return the class of the specified device
//
//-----------------------------------------------------------------------------

std::string Database::get_class_for_device(const std::string &devname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    //
    // First check if the device class is not already in the device class cache
    // If yes, simply returns it otherwise get class name from Db server
    // and stores it in device class cache
    //

    std::string ret_str;

    {
        omni_mutex_lock guard(map_mutex);

        auto pos = dev_class_cache.find(devname);
        if(pos == dev_class_cache.end())
        {
            send <<= devname.c_str();

            CALL_DB_SERVER("DbGetClassforDevice", send, received);

            const char *classname = nullptr;
            received.inout() >>= classname;
            ret_str = classname;

            std::pair<std::map<std::string, std::string>::iterator, bool> status;

            status = dev_class_cache.insert(std::make_pair(devname, ret_str));
            if(!status.second)
            {
                TangoSys_OMemStream o;
                o << "Can't insert device class for device " << devname << " in device class cache" << std::ends;
                TANGO_THROW_EXCEPTION(API_CantStoreDeviceClass, o.str());
            }
        }
        else
        {
            ret_str = pos->second;
        }
    }

    return ret_str;
}

//-----------------------------------------------------------------------------
//
// Database::get_class_inheritance_for_device() - Return the class inheritance
//                                                scheme of the specified device
//
//-----------------------------------------------------------------------------

DbDatum Database::get_class_inheritance_for_device(const std::string &devname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= devname.c_str();

    try
    {
        CALL_DB_SERVER("DbGetClassInheritanceForDevice", send, received);
    }
    catch(DevFailed &e)
    {
        // Check if an old API else re-throw
        if(strcmp(e.errors[0].reason.in(), API_CommandNotFound) != 0)
        {
            throw;
        }
        else
        {
            DbDatum db_datum;
            db_datum.name = "class";
            db_datum.value_string.resize(1);
            db_datum.value_string[0] = "Device_3Impl";
            return db_datum;
        }
    }

    return make_string_array(std::string("class"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_class_list() - Query the database for a list of classes
//
//-----------------------------------------------------------------------------

DbDatum Database::get_class_list(const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= wildcard.c_str();

    CALL_DB_SERVER("DbGetClassList", send, received);

    return make_string_array(std::string("class"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_class_attribute_list() - Query the database for a list of
//                                        attributes defined for the specified
//                                        class.
//
//-----------------------------------------------------------------------------

DbDatum Database::get_class_attribute_list(const std::string &classname, const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *class_info = new DevVarStringArray;

    class_info->length(2);
    (*class_info)[0] = string_dup(classname.c_str());
    (*class_info)[1] = string_dup(wildcard.c_str());

    send <<= class_info;

    CALL_DB_SERVER("DbGetClassAttributeList", send, received);

    return make_string_array(std::string("class"), received);
}

//-----------------------------------------------------------------------------
//
// Database::get_device_exported_for_class() - Query database for list of exported
//                                             devices for the specified class
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_exported_for_class(const std::string &classname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= classname.c_str();

    CALL_DB_SERVER("DbGetExportdDeviceListForClass", send, received);

    return make_string_array(std::string("device"), received);
}

//-----------------------------------------------------------------------------
//
// Database::put_device_alias() - Set an alias for a device
//
//-----------------------------------------------------------------------------

void Database::put_device_alias(const std::string &devname, const std::string &aliasname)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    DevVarStringArray *alias_info = new DevVarStringArray;

    alias_info->length(2);
    (*alias_info)[0] = string_dup(devname.c_str());
    (*alias_info)[1] = string_dup(aliasname.c_str());

    send <<= alias_info;

    CALL_DB_SERVER_NO_RET("DbPutDeviceAlias", send);
}

//-----------------------------------------------------------------------------
//
// Database::delete_device_alias() - Delete a device alias from the database
//
//-----------------------------------------------------------------------------

void Database::delete_device_alias(const std::string &aliasname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= aliasname.c_str();

    CALL_DB_SERVER_NO_RET("DbDeleteDeviceAlias", send);
}

//-----------------------------------------------------------------------------
//
// Database::put_attribute_alias() - Set an alias for an attribute
//
//-----------------------------------------------------------------------------

void Database::put_attribute_alias(std::string &attname, const std::string &aliasname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    DevVarStringArray *alias_info = new DevVarStringArray;

    alias_info->length(2);
    (*alias_info)[0] = string_dup(attname.c_str());
    (*alias_info)[1] = string_dup(aliasname.c_str());

    send <<= alias_info;

    CALL_DB_SERVER_NO_RET("DbPutAttributeAlias", send);
}

//-----------------------------------------------------------------------------
//
// Database::delete_attribute_alias() - Delete an attribute alias from the database
//
//-----------------------------------------------------------------------------

void Database::delete_attribute_alias(const std::string &aliasname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= aliasname.c_str();

    CALL_DB_SERVER("DbDeleteAttributeAlias", send, received);
}

//-----------------------------------------------------------------------------
//
// Database::make_history_array() - Convert the result of a DbGet...PropertyHist
//                                  command.
//
//-----------------------------------------------------------------------------

std::vector<DbHistory> Database::make_history_array(bool is_attribute, Any_var &received)
{
    const DevVarStringArray *ret = nullptr;
    received.inout() >>= ret;

    std::vector<DbHistory> v;
    if(ret == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }
    else
    {
        unsigned int i = 0;
        int count = 0;
        int offset;
        std::string aName;
        std::string pName;
        std::string pDate;
        std::string pCount;

        while(i < ret->length())
        {
            if(is_attribute)
            {
                aName = (*ret)[i];
                pName = (*ret)[i + 1];
                pDate = (*ret)[i + 2];
                pCount = (*ret)[i + 3];
                offset = 4;
            }
            else
            {
                pName = (*ret)[i];
                pDate = (*ret)[i + 1];
                pCount = (*ret)[i + 2];
                offset = 3;
            }

            std::istringstream istream(pCount);
            istream >> count;
            if(!istream)
            {
                TANGO_THROW_EXCEPTION(API_HistoryInvalid, "History format is invalid");
            }
            std::vector<std::string> value;
            for(int j = 0; j < count; j++)
            {
                value.emplace_back((*ret)[i + offset + j]);
            }

            if(is_attribute)
            {
                v.emplace_back(pName, aName, pDate, value);
            }
            else
            {
                v.emplace_back(pName, pDate, value);
            }

            i += (count + offset);
        }
    }

    return v;
}

//-----------------------------------------------------------------------------
//
// Database::get_property_history() - Returns the history of the specified object
//                                    property
//
//-----------------------------------------------------------------------------

std::vector<DbHistory> Database::get_property_history(const std::string &objname, const std::string &propname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *obj_info = new DevVarStringArray;

    obj_info->length(2);
    (*obj_info)[0] = string_dup(objname.c_str());
    (*obj_info)[1] = string_dup(propname.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetPropertyHist", send, received);

    return make_history_array(false, received);
}

//-----------------------------------------------------------------------------
//
// Database::get_device_property_history() - Returns the history of the specified
//                                           device property
//
//-----------------------------------------------------------------------------

std::vector<DbHistory> Database::get_device_property_history(const std::string &devname, const std::string &propname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *obj_info = new DevVarStringArray;

    obj_info->length(2);
    (*obj_info)[0] = string_dup(devname.c_str());
    (*obj_info)[1] = string_dup(propname.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetDevicePropertyHist", send, received);

    return make_history_array(false, received);
}

//-----------------------------------------------------------------------------
//
// Database::get_device_attribute_property_history() - Returns the history of
//                                                     the specified device
//                                                     attribute property
//
//-----------------------------------------------------------------------------

std::vector<DbHistory> Database::get_device_attribute_property_history(const std::string &devname,
                                                                       const std::string &attname,
                                                                       const std::string &propname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *obj_info = new DevVarStringArray;

    obj_info->length(3);
    (*obj_info)[0] = string_dup(devname.c_str());
    (*obj_info)[1] = string_dup(attname.c_str());
    (*obj_info)[2] = string_dup(propname.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetDeviceAttributePropertyHist", send, received);

    return make_history_array(true, received);
}

//-----------------------------------------------------------------------------
//
// Database::get_class_property_history() - Returns the history of the specified
//                                          class property
//
//-----------------------------------------------------------------------------

std::vector<DbHistory> Database::get_class_property_history(const std::string &classname, const std::string &propname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    DevVarStringArray *obj_info = new DevVarStringArray;

    check_access_and_get();

    obj_info->length(2);
    (*obj_info)[0] = string_dup(classname.c_str());
    (*obj_info)[1] = string_dup(propname.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetClassPropertyHist", send, received);

    return make_history_array(false, received);
}

//-----------------------------------------------------------------------------
//
// Database::get_class_attribute_property_history() - Returns the history of
//                                                    the specified class
//                                                    attribute property
//
//-----------------------------------------------------------------------------

std::vector<DbHistory> Database::get_class_attribute_property_history(const std::string &classname,
                                                                      const std::string &attname,
                                                                      const std::string &propname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    DevVarStringArray *obj_info = new DevVarStringArray;

    check_access_and_get();

    obj_info->length(3);
    (*obj_info)[0] = string_dup(classname.c_str());
    (*obj_info)[1] = string_dup(attname.c_str());
    (*obj_info)[2] = string_dup(propname.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetClassAttributePropertyHist", send, received);

    return make_history_array(true, received);
}

//-----------------------------------------------------------------------------
//
// Database::get_services() - Query database for specified services
//
//-----------------------------------------------------------------------------

DbDatum Database::get_services(const std::string &servname, const std::string &instname)
{
    DbData data;
    DbDatum db_datum;
    std::vector<std::string> services;
    std::vector<std::string> filter_services;

    // Get list of services

    ApiUtil *au = ApiUtil::instance();
    std::shared_ptr<DbServerCache> dsc;
    if(au->in_server())
    {
        if(!from_env_var)
        {
            dsc = nullptr;
        }
        else
        {
            try
            {
                Tango::Util *tg = Tango::Util::instance(false);
                dsc = tg->get_db_cache();
            }
            catch(Tango::DevFailed &e)
            {
                std::string reason = e.errors[0].reason.in();
                if(reason == API_UtilSingletonNotCreated && db_tg != nullptr)
                {
                    dsc = db_tg->get_db_cache();
                }
                else
                {
                    dsc = nullptr;
                }
            }
        }
    }
    else
    {
        dsc = nullptr;
    }

    DbDatum db_d(SERVICE_PROP_NAME);
    data.push_back(db_d);
    get_property_forced(CONTROL_SYSTEM, data, dsc);
    data[0] >> services;

    // Filter

    std::string filter = servname + "/";
    if(strcmp(instname.c_str(), "*") != 0)
    {
        filter += instname + ":";
    }

    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    for(unsigned int i = 0; i < services.size(); i++)
    {
        std::transform(services[i].begin(), services[i].end(), services[i].begin(), ::tolower);
        if(strncmp(services[i].c_str(), filter.c_str(), filter.length()) == 0)
        {
            std::string::size_type pos;
            pos = services[i].find(':');
            if(pos != std::string::npos)
            {
                filter_services.push_back(services[i].substr(pos + 1));
            }
        }
    }

    // Build return value

    db_datum.name = "services";
    db_datum.value_string.resize(filter_services.size());
    for(unsigned int i = 0; i < filter_services.size(); i++)
    {
        db_datum.value_string[i] = filter_services[i];
    }

    return db_datum;
}

//-----------------------------------------------------------------------------
//
// Database::get_device_service_list() - Query database for all devices for all
//                                         instances of a specified service
//
//-----------------------------------------------------------------------------

DbDatum Database::get_device_service_list(const std::string &servname)
{
    DbData data;
    DbDatum db_datum;
    std::vector<std::string> services;
    std::vector<std::string> filter_services;

    //
    // Get list of services
    //

    ApiUtil *au = ApiUtil::instance();
    std::shared_ptr<DbServerCache> dsc;
    if(au->in_server())
    {
        if(!from_env_var)
        {
            dsc = nullptr;
        }
        else
        {
            try
            {
                Tango::Util *tg = Tango::Util::instance(false);
                dsc = tg->get_db_cache();
            }
            catch(Tango::DevFailed &e)
            {
                std::string reason = e.errors[0].reason.in();
                if(reason == API_UtilSingletonNotCreated && db_tg != nullptr)
                {
                    dsc = db_tg->get_db_cache();
                }
                else
                {
                    dsc = nullptr;
                }
            }
        }
    }
    else
    {
        dsc = nullptr;
    }

    DbDatum db_d(SERVICE_PROP_NAME);
    data.push_back(db_d);
    get_property_forced(CONTROL_SYSTEM, data, dsc);
    data[0] >> services;

    //
    // Filter the required service
    //

    std::string filter = servname + "/";

    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    for(unsigned int i = 0; i < services.size(); i++)
    {
        std::transform(services[i].begin(), services[i].end(), services[i].begin(), ::tolower);
        if(strncmp(services[i].c_str(), filter.c_str(), filter.length()) == 0)
        {
            std::string::size_type pos, pos_end;
            pos = services[i].find('/');
            if(pos != std::string::npos)
            {
                pos_end = services[i].find(':');
                if(pos != std::string::npos)
                {
                    filter_services.push_back(services[i].substr(pos + 1, pos_end - pos - 1));
                    filter_services.push_back(services[i].substr(pos_end + 1));
                }
            }
        }
    }

    //
    // Build return value
    //

    db_datum.name = "services";
    db_datum.value_string.resize(filter_services.size());
    for(unsigned int i = 0; i < filter_services.size(); i++)
    {
        db_datum.value_string[i] = filter_services[i];
    }

    return db_datum;
}

//-----------------------------------------------------------------------------
//
// Database::register_service() - Register a new service
//
//-----------------------------------------------------------------------------

void Database::register_service(const std::string &servname, const std::string &instname, const std::string &devname)
{
    DbData data;
    std::vector<std::string> services;
    std::vector<std::string> new_services;
    bool service_exists = false;

    // Get list of services

    data.emplace_back(SERVICE_PROP_NAME);
    get_property(CONTROL_SYSTEM, data);
    data[0] >> services;

    std::string full_name = servname + "/" + instname + ":";

    std::transform(full_name.begin(), full_name.end(), full_name.begin(), ::tolower);

    // Update service list

    for(unsigned int i = 0; i < services.size(); i++)
    {
        std::string service_lower = services[i];
        std::transform(service_lower.begin(), service_lower.end(), service_lower.begin(), ::tolower);
        if(strncmp(service_lower.c_str(), full_name.c_str(), full_name.length()) == 0)
        {
            // The service already exists, update the device name

            std::string::size_type pos;
            pos = services[i].find(':');
            if(pos != std::string::npos)
            {
                new_services.push_back(services[i].substr(0, pos) + ":" + devname);
                service_exists = true;
            }
        }
        else
        {
            new_services.push_back(services[i]);
        }
    }

    if(!service_exists)
    {
        // Add the new service
        new_services.push_back(servname + "/" + instname + ":" + devname);
    }

    // Update the property

    data[0] << new_services;
    put_property(CONTROL_SYSTEM, data);
}

//-----------------------------------------------------------------------------
//
// Database::unregister_service() - Unregister a service
//
//-----------------------------------------------------------------------------

void Database::unregister_service(const std::string &servname, const std::string &instname)
{
    DbData data;
    std::vector<std::string> services;
    std::vector<std::string> new_services;
    bool service_deleted = false;

    // Get list of services

    data.emplace_back(SERVICE_PROP_NAME);
    get_property(CONTROL_SYSTEM, data);
    data[0] >> services;

    std::string full_name = servname + "/" + instname + ":";
    std::transform(full_name.begin(), full_name.end(), full_name.begin(), ::tolower);

    // Update service list

    for(unsigned int i = 0; i < services.size(); i++)
    {
        std::string service_lower = services[i];
        std::transform(service_lower.begin(), service_lower.end(), service_lower.begin(), ::tolower);
        if(strncmp(service_lower.c_str(), full_name.c_str(), full_name.length()) != 0)
        {
            new_services.push_back(services[i]);
        }
        else
        {
            service_deleted = true;
        }
    }

    if(service_deleted)
    {
        // Update the property

        data[0] << new_services;
        put_property(CONTROL_SYSTEM, data);
    }
}

//-----------------------------------------------------------------------------
//
// Database::export_event() - public method to export event to the Database
//
//-----------------------------------------------------------------------------

void Database::export_event(DevVarStringArray *eve_export)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    send <<= eve_export;

    CALL_DB_SERVER_NO_RET("DbExportEvent", send);
}

//-----------------------------------------------------------------------------
//
// Database::unexport_event() - public method to unexport an event from the Database
//
//-----------------------------------------------------------------------------

void Database::unexport_event(const std::string &event)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    send <<= event.c_str();

    CALL_DB_SERVER_NO_RET("DbUnExportEvent", send);
}

//-----------------------------------------------------------------------------
//
// Database::import_event() - public method to return import info for an event
//
//-----------------------------------------------------------------------------

CORBA::Any *Database::import_event(const std::string &event)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    //
    // Import device is allways possible whatever access rights are
    //

    {
        WriterLock guard(con_to_mon);

        AccessControlType tmp_access = access;
        access = ACCESS_WRITE;

        send <<= event.c_str();

        try
        {
            CALL_DB_SERVER("DbImportEvent", send, received);
            access = tmp_access;
        }
        catch(Tango::DevFailed &)
        {
            access = tmp_access;
            throw;
        }
    }

    return received._retn();
}

//-----------------------------------------------------------------------------
//
// Database::fill_server_cache() - Call Db server to get DS process cache data
//
//-----------------------------------------------------------------------------

CORBA::Any *Database::fill_server_cache(const std::string &ds_name, const std::string &loc_host)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    //
    // Some check on args.
    //

    std::string::size_type pos;
    pos = ds_name.find('/');
    if(pos == std::string::npos)
    {
        TANGO_THROW_EXCEPTION(API_MethodArgument,
                              "The device server name parameter is incorrect. Should be: <ds_exec_name>/<inst_name>");
    }

    //
    // Read Db device "StoredProcedureRelease" attribute. In case the stored procedure release is >= 1.9,
    // add Tango major release at the end the host name. This is required to manage compatibility between different
    // Tango releases used by device server(s) process when stored procedure supporting pipes is installed
    // (release 1.9 or higher)
    // To read the attribute, we do not have a DeviceProxy.
    //

    std::string ds_host(loc_host);
    int db_proc_release = 0;

    AttributeValueList_3 *attr_value_list_3 = nullptr;
    AttributeValueList_4 *attr_value_list_4 = nullptr;
    AttributeValueList_5 *attr_value_list_5 = nullptr;

    DevVarStringArray attr_list;
    attr_list.length(1);
    attr_list[0] = Tango::string_dup("StoredProcedureRelease");
    DeviceAttribute da;

    try
    {
        if(version >= 5)
        {
            Device_5_var dev = Device_5::_duplicate(device_5);
            attr_value_list_5 = dev->read_attributes_5(attr_list, DEV, get_client_identification());

            ApiUtil::attr_to_device(&((*attr_value_list_5)[0]), version, &da);
            delete attr_value_list_5;
        }
        else if(version == 4)
        {
            Device_4_var dev = Device_4::_duplicate(device_4);
            attr_value_list_4 = dev->read_attributes_4(attr_list, DEV, get_client_identification());

            ApiUtil::attr_to_device(&((*attr_value_list_4)[0]), version, &da);
            delete attr_value_list_4;
        }
        else
        {
            Device_3_var dev = Device_3::_duplicate(device_3);
            attr_value_list_3 = dev->read_attributes_3(attr_list, DEV);

            ApiUtil::attr_to_device(nullptr, &((*attr_value_list_3)[0]), version, &da);
            delete attr_value_list_3;
        }

        std::string sp_rel;
        da >> sp_rel;

        std::string rel = sp_rel.substr(8);
        std::string::size_type pos = rel.find('.');
        if(pos != std::string::npos)
        {
            std::string maj_str = rel.substr(0, pos);
            std::string min_str = rel.substr(pos + 1);

            int maj = atoi(maj_str.c_str());
            int min = atoi(min_str.c_str());

            db_proc_release = (maj * 100) + min;
        }
    }
    catch(DevFailed &)
    {
    }

    if(db_proc_release >= 109)
    {
        ds_host = ds_host + "%%" + TgLibMajorVers;
    }

    //
    // Filling the cache is always possible whatever access rights are
    //

    Any_var received;
    {
        WriterLock guard(con_to_mon);

        AccessControlType tmp_access = access;
        access = ACCESS_WRITE;

        //
        // Call the db server
        //

        Any send;
        DevVarStringArray *sent_info = new DevVarStringArray;

        sent_info->length(2);
        (*sent_info)[0] = string_dup(ds_name.c_str());
        (*sent_info)[1] = string_dup(ds_host.c_str());
        send <<= sent_info;

        try
        {
            CALL_DB_SERVER("DbGetDataForServerCache", send, received);
            access = tmp_access;
        }
        catch(Tango::DevFailed &)
        {
            access = tmp_access;
            throw;
        }
    }

    return received._retn();
}

//-----------------------------------------------------------------------------
//
// Database::delete_all_device_attribute_property() - Call Db server to
// delete all the property(ies) belonging to the specified device
// attribute(s)
//
//-----------------------------------------------------------------------------

void Database::delete_all_device_attribute_property(std::string dev_name, const DbData &db_data)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    Any send;

    DevVarStringArray *att_names = new DevVarStringArray;
    att_names->length(db_data.size() + 1);
    (*att_names)[0] = string_dup(dev_name.c_str());
    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        (*att_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }
    send <<= att_names;

    if(filedb != nullptr)
    {
        TANGO_THROW_EXCEPTION(API_NotSupportedFeature,
                              "The underlying database command is not implemented when the database is a file");
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteAllDeviceAttributeProperty", send);
    }
}

//-----------------------------------------------------------------------------
//
// Database::check_access() -
//
//-----------------------------------------------------------------------------

void Database::check_access()
{
    if((check_acc) && (!access_checked))
    {
        access = check_access_control(db_device_name);
        access_checked = true;
    }
}

//-----------------------------------------------------------------------------
//
// Database::check_access_control() -
//
//-----------------------------------------------------------------------------

AccessControlType Database::check_access_control(const std::string &devname)
{
    //
    // For DB device
    //

    if((access_checked) && (devname == db_device_name))
    {
        return access;
    }

    AccessControlType local_access = ACCESS_WRITE;

    try
    {
        if((!access_checked) && (access_proxy == nullptr))
        {
            //
            // Try to get Access Service device name from an environment
            // variable (for test purpose) or from the AccessControl service
            //

            std::string access_devname_str;

            int ret = get_env_var("ACCESS_DEVNAME", access_devname_str);
            if(ret == -1)
            {
                std::string service_name(ACCESS_SERVICE);
                std::string service_inst_name("*");

                DbDatum db_serv = get_services(service_name, service_inst_name);
                std::vector<std::string> serv_dev_list;
                db_serv >> serv_dev_list;
                if(!serv_dev_list.empty())
                {
                    access_devname_str = serv_dev_list[0];
                    access_service_defined = true;
                }
                else
                {
                    //
                    // No access service defined, give WRITE ACCESS to everyone also on the
                    // database device
                    //

                    //                    cerr << "No access service found" << endl;
                    access = ACCESS_WRITE;
                    return ACCESS_WRITE;
                }
            }

            //
            // Build the local AccessProxy instance
            // If the database has been built for a device defined with a FQDN,
            // don't forget to add db_host and db_port on the TAC device name
            // but only if FQDN infos are not laredy defined in the service
            // device name
            //

            if(!from_env_var)
            {
                int num = 0;
                num = count(access_devname_str.begin(), access_devname_str.end(), '/');

                if(num == 2)
                {
                    std::string fqdn("tango://");
                    fqdn = fqdn + db_host + ":" + db_port + "/";
                    access_devname_str.insert(0, fqdn);
                }
            }
            access_proxy = new AccessProxy(access_devname_str);
        }

        //
        // Get access rights
        //

        if(access_proxy != nullptr)
        {
            local_access = access_proxy->check_access_control(devname);
        }
        else
        {
            if(!access_service_defined)
            {
                local_access = ACCESS_WRITE;
            }
            else
            {
                local_access = ACCESS_READ;
            }
        }

        access_except_errors.length(0);
    }
    catch(Tango::DevFailed &e)
    {
        if(::strcmp(e.errors[0].reason.in(), API_DeviceNotExported) == 0 ||
           ::strcmp(e.errors[0].reason.in(), DB_DeviceNotDefined) == 0)
        {
            std::string tmp_err_desc(e.errors[0].desc.in());
            tmp_err_desc =
                tmp_err_desc +
                "\nControlled access service defined in Db but unreachable --> Read access given to all devices...";
            e.errors[0].desc = Tango::string_dup(tmp_err_desc.c_str());
        }
        access_except_errors = e.errors;
        local_access = ACCESS_READ;
    }

    return local_access;
}

//-----------------------------------------------------------------------------
//
// Database::is_command_allowed() -
//
//-----------------------------------------------------------------------------

bool Database::is_command_allowed(const std::string &devname, const std::string &cmd)
{
    WriterLock guard(con_to_mon);

    bool ret;

    if(access_proxy == nullptr)
    {
        AccessControlType acc = check_access_control(devname);

        if(access_proxy == nullptr)
        {
            //            ret = !check_acc;
            ret = acc != ACCESS_READ;
            return ret;
        }
        else
        {
            access = acc;
            clear_access_except_errors();
        }
    }

    if(devname == db_device_name)
    {
        //
        // In case of Database object, the first command uses the default access right (READ)
        // Therefore, this is_command_allowed method is called and the access are checked by the
        // check_access_control() call upper in this method. If the access is WRITE, force
        // true for the retrun value of this method
        //

        if(access == ACCESS_READ)
        {
            std::string db_class("Database");
            ret = access_proxy->is_command_allowed(db_class, cmd);
        }
        else
        {
            ret = true;
        }
    }
    else
    {
        //
        // Get device class
        //

        std::string dev_class = get_class_for_device(devname);
        ret = access_proxy->is_command_allowed(dev_class, cmd);
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// method :            Database::write_event_channel_ior_filedatabase() -
//
// description :     Method to connect write the event channel ior to the file
//                    used as database
//
// argument : in :    ec_ior : The event channel IOR
//
//-----------------------------------------------------------------------------

void Database::write_event_channel_ior_filedatabase(const std::string &ec_ior)
{
    if(filedb == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_NotSupportedFeature, "This call is supported only when the database is a file");
    }

    filedb->write_event_channel_ior(ec_ior);
}

//-----------------------------------------------------------------------------
//
// Database::get_device_info() - public method to get device information
//
//-----------------------------------------------------------------------------

DbDevFullInfo Database::get_device_info(const std::string &dev)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= dev.c_str();

    CALL_DB_SERVER("DbGetDeviceInfo", send, received);

    const DevVarLongStringArray *dev_info_db = nullptr;
    received.inout() >>= dev_info_db;

    DbDevFullInfo dev_info;
    if(dev_info_db != nullptr)
    {
        dev_info.name = std::string(dev_info_db->svalue[0]);
        dev_info.ior = std::string(dev_info_db->svalue[1]);
        dev_info.version = std::string(dev_info_db->svalue[2]);
        dev_info.ds_full_name = std::string(dev_info_db->svalue[3]);
        dev_info.host = std::string(dev_info_db->svalue[4]);
        if(::strlen(dev_info_db->svalue[5]) != 1)
        {
            dev_info.started_date = std::string(dev_info_db->svalue[5]);
        }
        if(::strlen(dev_info_db->svalue[6]) != 1)
        {
            dev_info.stopped_date = std::string(dev_info_db->svalue[6]);
        }

        if(dev_info_db->svalue.length() > 7)
        {
            dev_info.class_name = std::string(dev_info_db->svalue[7]);
        }
        else
        {
            try
            {
                dev_info.class_name = get_class_for_device(dev);
            }
            catch(...)
            {
            }
        }

        dev_info.exported = dev_info_db->lvalue[0];
        dev_info.pid = dev_info_db->lvalue[1];
    }
    else
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }

    return (dev_info);
}

//-----------------------------------------------------------------------------
//
// Database::get_device_from_alias() - Get device name from an alias
//
//-----------------------------------------------------------------------------
void Database::get_device_from_alias(std::string alias_name, std::string &dev_name)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= alias_name.c_str();

    if(filedb != nullptr)
    {
        received = filedb->DbGetAliasDevice(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetAliasDevice", send, received);
    }
    const char *dev_name_tmp = nullptr;
    received.inout() >>= dev_name_tmp;
    dev_name = dev_name_tmp;
}

//-----------------------------------------------------------------------------
//
// Database::get_alias_from_device() - Get alias name from a device name
//
//-----------------------------------------------------------------------------
void Database::get_alias_from_device(std::string dev_name, std::string &alias_name)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= dev_name.c_str();

    if(filedb != nullptr)
    {
        received = filedb->DbGetDeviceAlias(send);
    }
    else
    {
        CALL_DB_SERVER("DbGetDeviceAlias", send, received);
    }
    const char *dev_name_tmp = nullptr;
    received.inout() >>= dev_name_tmp;
    alias_name = dev_name_tmp;
}

//-----------------------------------------------------------------------------
//
// Database::get_attribute_from_alias() - Get attribute name from an alias
//
//-----------------------------------------------------------------------------
void Database::get_attribute_from_alias(std::string attr_alias, std::string &attr_name)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= attr_alias.c_str();

    /*    if (filedb != 0)
            received = filedb->DbGetAliasAttribute(send);
        else*/
    CALL_DB_SERVER("DbGetAliasAttribute", send, received);
    const char *attr_name_tmp = nullptr;
    received.inout() >>= attr_name_tmp;

    if(attr_name_tmp == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }
    else
    {
        attr_name = attr_name_tmp;
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_alias_from_attribute() - Get alias name from an attribute name
//
//-----------------------------------------------------------------------------
void Database::get_alias_from_attribute(std::string attr_name, std::string &attr_alias)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    send <<= attr_name.c_str();

    /*    if (filedb != 0)
            received = filedb->DbGetAttributeAlias2(send);
        else*/
    CALL_DB_SERVER("DbGetAttributeAlias2", send, received);
    const char *attr_alias_tmp = nullptr;
    received.inout() >>= attr_alias_tmp;

    if(attr_alias_tmp == nullptr)
    {
        TANGO_THROW_EXCEPTION(API_IncoherentDbData, "Incoherent data received from database");
    }
    else
    {
        attr_alias = attr_alias_tmp;
    }
}

//-----------------------------------------------------------------------------
//
// Database::get_device_attribute_list() - Get list of attributes with data in db
// for a specified device
//
//-----------------------------------------------------------------------------
void Database::get_device_attribute_list(const std::string &dev_name, std::vector<std::string> &att_list)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *sent_names = new DevVarStringArray;
    sent_names->length(2);
    (*sent_names)[0] = string_dup(dev_name.c_str());
    (*sent_names)[1] = string_dup("*");

    send <<= sent_names;

    CALL_DB_SERVER("DbGetDeviceAttributeList", send, received);

    const DevVarStringArray *recv_names = nullptr;
    received.inout() >>= recv_names;

    att_list << *recv_names;
}

//-----------------------------------------------------------------------------
//
// Database::rename_server() - Rename a device server process
//
//-----------------------------------------------------------------------------
void Database::rename_server(const std::string &old_ds_name, const std::string &new_ds_name)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *sent_names = new DevVarStringArray;
    sent_names->length(2);
    (*sent_names)[0] = string_dup(old_ds_name.c_str());
    (*sent_names)[1] = string_dup(new_ds_name.c_str());

    send <<= sent_names;

    CALL_DB_SERVER_NO_RET("DbRenameServer", send);
}

//-----------------------------------------------------------------------------------------------------------------
//
// Database::get_class_pipe_property() - public method to get class pipe properties from the Database
//
//-----------------------------------------------------------------------------------------------------------------

void Database::get_class_pipe_property(std::string device_class,
                                       DbData &db_data,
                                       std::shared_ptr<DbServerCache> db_cache)
{
    unsigned int i;
    Any_var received;
    const DevVarStringArray *property_values = nullptr;

    check_access_and_get();

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(device_class.c_str());
    for(i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }

    if(!db_cache)
    {
        //
        // Get property(ies) from DB server
        //

        Any send;
        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

        send <<= property_names;

        if(filedb != nullptr)
        {
            received = filedb->DbGetClassPipeProperty(send);
        }
        else
        {
            try
            {
                CALL_DB_SERVER("DbGetClassPipeProperty", send, received);
            }
            catch(Tango::DevFailed &)
            {
                throw;
            }
        }
        received.inout() >>= property_values;
    }
    else
    {
        //
        // Try to get property(ies) from cache
        //

        try
        {
            property_values = db_cache->get_class_pipe_property(property_names);
            delete property_names;
        }
        catch(Tango::DevFailed &e)
        {
            std::string err_reason(e.errors[0].reason.in());

            if(err_reason == DB_ClassNotFoundInCache || err_reason == DB_TooOldStoredProc)
            {
                if(err_reason == DB_TooOldStoredProc)
                {
                    TANGO_LOG << "WARNING: You database stored procedure is too old to support device pipe"
                              << std::endl;
                    TANGO_LOG << "Please, update to stored procedure release 1.9 or more" << std::endl;
                    TANGO_LOG << "Trying direct Db access" << std::endl;
                }

                //
                // The class is not defined in cache, get property(ies) from DB server
                //

                Any send;
                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

                send <<= property_names;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetClassPipeProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetClassPipeProperty", send, received);
                }
                received.inout() >>= property_values;
            }
            else
            {
                delete property_names;
                throw;
            }
        }
    }

    unsigned int n_pipes, index;
    int i_total_props;
    std::stringstream iostream;

    iostream << (*property_values)[1].in() << std::ends;
    iostream >> n_pipes;
    index = 2;
    i_total_props = 0;

    long old_size = 0;
    for(i = 0; i < n_pipes; i++)
    {
        old_size++;
        db_data.resize(old_size);
        short n_props;
        db_data[i_total_props].name = (*property_values)[index];
        index++;
        iostream.seekp(0);
        iostream.seekg(0);
        iostream.clear();
        iostream << (*property_values)[index].in() << std::ends;
        iostream >> n_props;
        db_data[i_total_props] << n_props;
        db_data.resize(old_size + n_props);
        old_size = old_size + n_props;
        i_total_props++;
        index++;
        for(int j = 0; j < n_props; j++)
        {
            db_data[i_total_props].name = (*property_values)[index];
            index++;
            int n_values;
            iostream.seekp(0);
            iostream.seekg(0);
            iostream.clear();
            iostream << (*property_values)[index].in() << std::ends;
            iostream >> n_values;
            index++;

            db_data[i_total_props].value_string.resize(n_values);
            if(n_values == 0)
            {
                index++; // skip dummy returned value ""
            }
            else
            {
                for(int k = 0; k < n_values; k++)
                {
                    db_data[i_total_props].value_string[k] = (*property_values)[index].in();
                    index++;
                }
            }
            i_total_props++;
        }
    }
}

//------------------------------------------------------------------------------------------------------------------
//
// Database::get_device_pipe_property() - public method to get device pipe properties from the Database
//
//------------------------------------------------------------------------------------------------------------------

void Database::get_device_pipe_property(std::string dev, DbData &db_data, std::shared_ptr<DbServerCache> db_cache)
{
    unsigned int i, j;
    Any_var received;
    const DevVarStringArray *property_values = nullptr;

    check_access_and_get();

    DevVarStringArray *property_names = new DevVarStringArray;
    property_names->length(db_data.size() + 1);
    (*property_names)[0] = string_dup(dev.c_str());
    for(i = 0; i < db_data.size(); i++)
    {
        (*property_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }

    if(!db_cache)
    {
        //
        // Get propery(ies) from DB server
        //

        AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
        Any send;

        send <<= property_names;

        if(filedb != nullptr)
        {
            received = filedb->DbGetDevicePipeProperty(send);
        }
        else
        {
            try
            {
                CALL_DB_SERVER("DbGetDevicePipeProperty", send, received);
            }
            catch(Tango::DevFailed &)
            {
                throw;
            }
        }
        received.inout() >>= property_values;
    }
    else
    {
        //
        // Try  to get property(ies) from cache
        //

        try
        {
            property_values = db_cache->get_dev_pipe_property(property_names);
            delete property_names;
        }
        catch(Tango::DevFailed &e)
        {
            std::string err_reason(e.errors[0].reason.in());

            if(err_reason == DB_DeviceNotFoundInCache || err_reason == DB_TooOldStoredProc)
            {
                if(err_reason == DB_TooOldStoredProc)
                {
                    TANGO_LOG << "WARNING: You database stored procedure is too old to support device pipe"
                              << std::endl;
                    TANGO_LOG << "Please, update to stored procedure release 1.9 or more" << std::endl;
                    TANGO_LOG << "Trying direct Db access" << std::endl;
                }

                //
                // The device is not in cache, ask the db server
                //

                AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
                Any send;

                send <<= property_names;

                if(filedb != nullptr)
                {
                    received = filedb->DbGetDeviceAttributeProperty(send);
                }
                else
                {
                    CALL_DB_SERVER("DbGetDevicePipeProperty", send, received);
                }
                received.inout() >>= property_values;
            }
            else
            {
                delete property_names;
                throw;
            }
        }
    }

    unsigned int n_pipes, index;
    int i_total_props;
    std::stringstream iostream;

    iostream << (*property_values)[1].in() << std::ends;
    iostream >> n_pipes;
    index = 2;

    i_total_props = 0;

    long old_size = 0;
    for(i = 0; i < n_pipes; i++)
    {
        old_size++;
        db_data.resize(old_size);
        unsigned short n_props;
        db_data[i_total_props].name = (*property_values)[index];
        index++;
        iostream.seekp(0);
        iostream.seekg(0);
        iostream.clear();
        iostream << (*property_values)[index].in() << std::ends;
        iostream >> n_props;
        db_data[i_total_props] << n_props;
        db_data.resize(old_size + n_props);
        old_size = old_size + n_props;
        i_total_props++;
        index++;
        for(j = 0; j < n_props; j++)
        {
            db_data[i_total_props].name = (*property_values)[index];
            index++;
            int n_values;
            iostream.seekp(0);
            iostream.seekg(0);
            iostream.clear();
            iostream << (*property_values)[index].in() << std::ends;
            iostream >> n_values;
            index++;

            db_data[i_total_props].value_string.resize(n_values);
            if(n_values == 0)
            {
                index++; // skip dummy returned value " "
            }
            else
            {
                for(int k = 0; k < n_values; k++)
                {
                    db_data[i_total_props].value_string[k] = (*property_values)[index].in();
                    index++;
                }
            }
            i_total_props++;
        }
    }

    TANGO_LOG_DEBUG << "Leaving get_device_pipe_property" << std::endl;
}

//------------------------------------------------------------------------------------------------------------------
//
// Database::delete_class_pipe_property() - public method to delete class pipe properties from the Database
//
//------------------------------------------------------------------------------------------------------------------

void Database::delete_class_pipe_property(std::string device_class, const DbData &db_data)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *property_values = new DevVarStringArray;
    unsigned int nb_prop = db_data.size() - 1;
    property_values->length(nb_prop + 2);
    (*property_values)[0] = string_dup(device_class.c_str());
    (*property_values)[1] = string_dup(db_data[0].name.c_str());
    for(unsigned int i = 0; i < nb_prop; i++)
    {
        (*property_values)[i + 2] = string_dup(db_data[i + 1].name.c_str());
    }

    send <<= property_values;

    if(filedb != nullptr)
    {
        filedb->DbDeleteClassPipeProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteClassPipeProperty", send);
    }
}

//--------------------------------------------------------------------------------------------------------------------
//
// Database::delete_device_pipe_property() - public method to delete device pipe properties from the Database
//
//--------------------------------------------------------------------------------------------------------------------

void Database::delete_device_pipe_property(std::string dev, const DbData &db_data)
{
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *property_values = new DevVarStringArray;
    unsigned int nb_prop = db_data.size() - 1;
    property_values->length(nb_prop + 2);
    (*property_values)[0] = string_dup(dev.c_str());
    (*property_values)[1] = string_dup(db_data[0].name.c_str());
    for(unsigned int i = 0; i < nb_prop; i++)
    {
        (*property_values)[i + 2] = string_dup(db_data[i + 1].name.c_str());
    }

    send <<= property_values;

    if(filedb != nullptr)
    {
        filedb->DbDeleteDevicePipeProperty(send);
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteDevicePipeProperty", send);
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// Database::get_class_pipe_list() - Query the database for a list of pipes defined for the specified class.
//
//-------------------------------------------------------------------------------------------------------------------

DbDatum Database::get_class_pipe_list(const std::string &classname, const std::string &wildcard)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *class_info = new DevVarStringArray;

    class_info->length(2);
    (*class_info)[0] = string_dup(classname.c_str());
    (*class_info)[1] = string_dup(wildcard.c_str());

    send <<= class_info;

    CALL_DB_SERVER("DbGetClassPipeList", send, received);

    return make_string_array(std::string("class"), received);
}

//-------------------------------------------------------------------------------------------------------------------
//
// Database::get_device_pipe_list() - Get list of pipes with data in db for a specified device
//
//-------------------------------------------------------------------------------------------------------------------
void Database::get_device_pipe_list(const std::string &dev_name, std::vector<std::string> &pipe_list)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *sent_names = new DevVarStringArray;
    sent_names->length(2);
    (*sent_names)[0] = string_dup(dev_name.c_str());
    (*sent_names)[1] = string_dup("*");

    send <<= sent_names;

    CALL_DB_SERVER("DbGetDevicePipeList", send, received);

    const DevVarStringArray *recv_names = nullptr;
    received.inout() >>= recv_names;

    pipe_list << *recv_names;
}

//----------------------------------------------------------------------------------------------------------------
//
// Database::delete_all_device_pipe_property() - Call Db server to delete all the property(ies) belonging to the
// specified device pipe(s)
//
//----------------------------------------------------------------------------------------------------------------

void Database::delete_all_device_pipe_property(std::string dev_name, const DbData &db_data)
{
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    Any send;

    DevVarStringArray *att_names = new DevVarStringArray;
    att_names->length(db_data.size() + 1);
    (*att_names)[0] = string_dup(dev_name.c_str());
    for(unsigned int i = 0; i < db_data.size(); i++)
    {
        (*att_names)[i + 1] = string_dup(db_data[i].name.c_str());
    }
    send <<= att_names;

    if(filedb != nullptr)
    {
        TANGO_THROW_EXCEPTION(API_NotSupportedFeature,
                              "The underlying database command is not implemented when the database is a file");
    }
    else
    {
        CALL_DB_SERVER_NO_RET("DbDeleteAllDevicePipeProperty", send);
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// Database::put_class_pipe_property() - public method to put class pipe properties into the Database
//
//-------------------------------------------------------------------------------------------------------------------

void Database::put_class_pipe_property(std::string device_class, const DbData &db_data)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    int index, n_pipes;
    bool retry = true;

    check_access_and_get();

    while(retry)
    {
        DevVarStringArray *property_values = new DevVarStringArray;
        property_values->length(2);
        index = 2;
        (*property_values)[0] = string_dup(device_class.c_str());
        n_pipes = 0;

        for(unsigned int i = 0; i < db_data.size();)
        {
            short n_props = 0;
            index++;
            property_values->length(index);
            (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
            db_data[i] >> n_props;
            ostream.seekp(0);
            ostream.clear();
            ostream << n_props << std::ends;
            index++;
            property_values->length(index);

            std::string st = ostream.str();
            (*property_values)[index - 1] = string_dup(st.c_str());

            for(int j = 0; j < n_props; j++)
            {
                index++;
                property_values->length(index);
                (*property_values)[index - 1] = string_dup(db_data[i + j + 1].name.c_str());
                index++;
                property_values->length(index);
                int prop_size = db_data[i + j + 1].size();
                ostream.seekp(0);
                ostream.clear();
                ostream << prop_size << std::ends;

                std::string st = ostream.str();
                (*property_values)[index - 1] = string_dup(st.c_str());

                property_values->length(index + prop_size);
                for(int q = 0; q < prop_size; q++)
                {
                    (*property_values)[index + q] = string_dup(db_data[i + j + 1].value_string[q].c_str());
                }
                index = index + prop_size;
            }
            i = i + n_props + 1;
            n_pipes++;
        }

        ostream.seekp(0);
        ostream.clear();
        ostream << n_pipes << std::ends;

        std::string st = ostream.str();
        (*property_values)[1] = string_dup(st.c_str());

        send <<= property_values;

        if(filedb != nullptr)
        {
            filedb->DbPutClassPipeProperty(send);
            retry = false;
        }
        else
        {
            CALL_DB_SERVER_NO_RET("DbPutClassPipeProperty", send);
            retry = false;
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//
// Database::put_device_pipe_property() - public method to put device pipe properties into the Database
//
//-------------------------------------------------------------------------------------------------------------------

void Database::put_device_pipe_property(std::string dev, const DbData &db_data)
{
    std::ostringstream ostream;
    Any send;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    int index, n_pipes;
    bool retry = true;

    check_access_and_get();

    while(retry)
    {
        DevVarStringArray *property_values = new DevVarStringArray;
        property_values->length(2);
        index = 2;
        (*property_values)[0] = string_dup(dev.c_str());
        n_pipes = 0;

        for(unsigned int i = 0; i < db_data.size();)
        {
            short n_props = 0;
            index++;
            property_values->length(index);
            (*property_values)[index - 1] = string_dup(db_data[i].name.c_str());
            db_data[i] >> n_props;
            ostream.seekp(0);
            ostream.clear();
            ostream << n_props << std::ends;
            index++;
            property_values->length(index);

            std::string st = ostream.str();
            (*property_values)[index - 1] = string_dup(st.c_str());

            for(int j = 0; j < n_props; j++)
            {
                index++;
                property_values->length(index);
                (*property_values)[index - 1] = string_dup(db_data[i + j + 1].name.c_str());
                index++;
                property_values->length(index);
                int prop_size = db_data[i + j + 1].size();
                ostream.seekp(0);
                ostream.clear();
                ostream << prop_size << std::ends;

                std::string st = ostream.str();
                (*property_values)[index - 1] = string_dup(st.c_str());

                property_values->length(index + prop_size);
                for(int q = 0; q < prop_size; q++)
                {
                    (*property_values)[index + q] = string_dup(db_data[i + j + 1].value_string[q].c_str());
                }
                index = index + prop_size;
            }
            i = i + n_props + 1;
            n_pipes++;
        }

        ostream.seekp(0);
        ostream.clear();
        ostream << n_pipes << std::ends;

        std::string st = ostream.str();
        (*property_values)[1] = string_dup(st.c_str());

        send <<= property_values;

        if(filedb != nullptr)
        {
            filedb->DbPutDevicePipeProperty(send);
            retry = false;
        }
        else
        {
            try
            {
                CALL_DB_SERVER_NO_RET("DbPutDevicePipeProperty", send);
                retry = false;
            }
            catch(Tango::DevFailed &)
            {
                throw;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------
//
// Database::get_class_pipe_property_history() - Returns the history of the specified class pipe property
//
//------------------------------------------------------------------------------------------------------------------

std::vector<DbHistory> Database::get_class_pipe_property_history(const std::string &classname,
                                                                 const std::string &pipename,
                                                                 const std::string &propname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);
    DevVarStringArray *obj_info = new DevVarStringArray;

    check_access_and_get();

    obj_info->length(3);
    (*obj_info)[0] = string_dup(classname.c_str());
    (*obj_info)[1] = string_dup(pipename.c_str());
    (*obj_info)[2] = string_dup(propname.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetClassPipePropertyHist", send, received);

    return make_history_array(true, received);
}

//-----------------------------------------------------------------------------------------------------------------
//
// Database::get_device_pipe_property_history() - Returns the history of the specified device pipe property
//
//-----------------------------------------------------------------------------------------------------------------

std::vector<DbHistory> Database::get_device_pipe_property_history(const std::string &devname,
                                                                  const std::string &pipename,
                                                                  const std::string &propname)
{
    Any send;
    Any_var received;
    AutoConnectTimeout act(DB_RECONNECT_TIMEOUT);

    check_access_and_get();

    DevVarStringArray *obj_info = new DevVarStringArray;

    obj_info->length(3);
    (*obj_info)[0] = string_dup(devname.c_str());
    (*obj_info)[1] = string_dup(pipename.c_str());
    (*obj_info)[2] = string_dup(propname.c_str());

    send <<= obj_info;

    CALL_DB_SERVER("DbGetDevicePipePropertyHist", send, received);

    return make_history_array(true, received);
}

} // namespace Tango
