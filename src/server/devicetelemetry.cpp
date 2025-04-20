//------------------------------------------------------------------------------------------------
//         THE FOLLOWING CODE EXTENDS THE 'DeviceImpl' CLASS IN A DEDICATED .CPP FILE
//      WE SIMPLY REUSE THE APPROACH USED FOR THE LOGGING SERVICE FOR CONSISTENCY REASONS
//------------------------------------------------------------------------------------------------

#include <tango/internal/utils.h>
#include <tango/server/device.h>

namespace Tango
{

#if defined(TANGO_USE_TELEMETRY)

//-------------------------------------------------------------------------------------------------
//  method : DeviceImpl::initialize_telemetry_interface
//-------------------------------------------------------------------------------------------------
void DeviceImpl::initialize_telemetry_interface()
{
    auto details = telemetry::Configuration::Server{device_class->get_name(), device_name};
    telemetry::Configuration cfg{device_name, "tango", details};

    // this might throw an exception in case there's no valid endpoint defined
    telemetry_interface = Tango::telemetry::InterfaceFactory::create(cfg);

    // attach a telemetry appender to the logger
    log4tango::Appender *appender = telemetry_interface->get_appender();
    get_logger()->add_appender(appender);
}

//-------------------------------------------------------------------------------------------------
//  method : DeviceImpl::cleanup_telemetry_interface
//-------------------------------------------------------------------------------------------------
void DeviceImpl::cleanup_telemetry_interface() noexcept
{
    get_logger()->remove_appender(Tango::telemetry::kTelemetryLogAppenderName);

    telemetry_interface.reset();
}

#endif // #if defined(TANGO_USE_TELEMETRY)

} // namespace Tango
