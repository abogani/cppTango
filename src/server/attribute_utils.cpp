#include <tango/tango.h>
#include <tango/internal/server/attribute_utils.h>

namespace Tango::detail
{
void upd_att_prop_db(const Tango::Attr_CheckVal &new_value,
                     const char *prop_name,
                     const std::string &d_name,
                     const std::string &name,
                     long data_type)
{
    TANGO_LOG_DEBUG << "Entering upd_att_prop_db method for attribute " << name << ", property = " << prop_name
                    << std::endl;

    //
    // Build the data sent to database
    //

    Tango::DbData db_data;
    Tango::DbDatum att(name), prop(prop_name);
    att << (short) 1;

    switch(data_type)
    {
    case Tango::DEV_SHORT:
    case Tango::DEV_ENUM:
        prop << new_value.sh;
        break;

    case Tango::DEV_LONG:
        prop << new_value.lg;
        break;

    case Tango::DEV_LONG64:
        prop << new_value.lg64;
        break;

    case Tango::DEV_DOUBLE:
        prop << new_value.db;
        break;

    case Tango::DEV_FLOAT:
        prop << new_value.fl;
        break;

    case Tango::DEV_USHORT:
        prop << new_value.ush;
        break;

    case Tango::DEV_UCHAR:
        prop << new_value.uch;
        break;

    case Tango::DEV_ULONG:
        prop << new_value.ulg;
        break;

    case Tango::DEV_ULONG64:
        prop << new_value.ulg64;
        break;

    case Tango::DEV_STATE:
        prop << (short) new_value.d_sta;
        break;
    }

    db_data.push_back(att);
    db_data.push_back(prop);

    //
    // Implement a reconnection schema. The first exception received if the db
    // server is down is a COMM_FAILURE exception. Following exception received
    // from following calls are TRANSIENT exception
    //

    Tango::Util *tg = Tango::Util::instance();
    bool retry = true;
    while(retry)
    {
        try
        {
            tg->get_database()->put_device_attribute_property(d_name, db_data);
            retry = false;
        }
        catch(CORBA::COMM_FAILURE &)
        {
            tg->get_database()->reconnect(true);
        }
    }
}

void delete_startup_exception(Tango::Attribute &attr,
                              std::string prop_name,
                              std::string dev_name,
                              bool &check_startup_exceptions,
                              std::map<std::string, Tango::DevFailed> &startup_exceptions)
{
    if(attr.is_startup_exception())
    {
        auto it = startup_exceptions.find(prop_name);
        if(it != startup_exceptions.end())
        {
            startup_exceptions.erase(it);
        }
        if(startup_exceptions.empty())
        {
            check_startup_exceptions = false;
        }

        Tango::Util *tg = Tango::Util::instance();
        bool dev_restart = false;
        if(dev_name != "None")
        {
            dev_restart = tg->is_device_restarting(dev_name);
        }
        if(!tg->is_svr_starting() && !dev_restart)
        {
            Tango::DeviceImpl *dev = attr.get_att_device();
            dev->set_run_att_conf_loop(true);
        }
    }
}

void throw_err_data_type(const char *prop_name,
                         const std::string &dev_name,
                         const std::string &name,
                         const char *origin)
{
    TangoSys_OMemStream o;

    o << "Device " << dev_name << "-> Attribute : " << name;
    o << "\nThe property " << prop_name << " is not settable for the attribute data type" << std::ends;
    Tango::Except::throw_exception(Tango::API_AttrOptProp, o.str(), origin);
}

void throw_incoherent_val_err(const char *min_prop,
                              const char *max_prop,
                              const std::string &dev_name,
                              const std::string &name,
                              const char *origin)
{
    TangoSys_OMemStream o;

    o << "Device " << dev_name << "-> Attribute : " << name;
    o << "\nValue of " << min_prop << " is greater than or equal to " << max_prop << std::ends;
    Tango::Except::throw_exception(Tango::API_IncoherentValues, o.str(), origin);
}

void throw_err_format(const char *prop_name, const std::string &dev_name, const std::string &name, const char *origin)
{
    TangoSys_OMemStream o;

    o << "Device " << dev_name << "-> Attribute : " << name;
    o << "\nThe property " << prop_name << " is defined in an unsupported format" << std::ends;
    Except::throw_exception(API_AttrOptProp, o.str(), origin);
}

void avns_in_att(Tango::Attribute &attr, const std::string &d_name, bool &check_value, std::string &value_str)
{
    Tango::Util *tg = Tango::Util::instance();
    Tango::TangoMonitor *mon_ptr = nullptr;
    if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
    {
        mon_ptr = &(attr.get_att_device()->get_att_conf_monitor());
    }

    {
        Tango::AutoTangoMonitor sync1(mon_ptr);

        check_value = false;
        value_str = AlrmValueNotSpec;

        if(!tg->is_svr_starting() && !tg->is_device_restarting(d_name))
        {
            attr.get_att_device()->push_att_conf_event(&attr);
        }
    }
}

void avns_in_db(const char *prop_name, const std::string &name, const std::string &dev_name)
{
    Tango::Util *tg = Tango::Util::instance();

    if(Tango::Util::instance()->use_db())
    {
        Tango::DbDatum attr_dd(name), prop_dd(prop_name);
        attr_dd << 1L;
        prop_dd << AlrmValueNotSpec;
        Tango::DbData db_data;
        db_data.push_back(attr_dd);
        db_data.push_back(prop_dd);

        bool retry = true;
        while(retry)
        {
            try
            {
                tg->get_database()->put_device_attribute_property(dev_name, db_data);
                retry = false;
            }
            catch(CORBA::COMM_FAILURE &)
            {
                tg->get_database()->reconnect(true);
            }
        }
    }
}

bool prop_in_list(const char *prop_name, std::string &prop_str, const std::vector<AttrProperty> &list)
{
    for(const auto &prop : list)
    {
        if(prop.get_name() == prop_name)
        {
            prop_str = prop.get_value();
            return true;
        }
    }
    return false;
}

} // namespace Tango::detail
