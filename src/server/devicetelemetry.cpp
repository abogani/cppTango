//------------------------------------------------------------------------------------------------
//         THE FOLLOWING CODE EXTENDS THE 'DeviceImpl' CLASS IN A DEDICATED .CPP FILE
//      WE SIMPLY REUSE THE APPROACH USED FOR THE LOGGING SERVICE FOR CONSISTENCY REASONS
//------------------------------------------------------------------------------------------------

#include <tango/tango.h>
#include <tango/internal/utils.h>

namespace Tango
{

#if defined(TANGO_USE_TELEMETRY)

//-------------------------------------------------------------------------------------------------
//  method : DeviceImpl::initialize_telemetry_interface
//-------------------------------------------------------------------------------------------------
void DeviceImpl::initialize_telemetry_interface()
{
    // TODO: DServer tracing?
    bool telemetry_enabled = device_class->get_name() != "DServer"
                                 ? detail::get_boolean_env_var(Tango::telemetry::kEnvVarTelemetryEnable, false)
                                 : false;

    bool kernel_traces_enabled = detail::get_boolean_env_var(Tango::telemetry::kEnvVarTelemetryKernelEnable, false);

    // configure the telemetry
    // TODO: offer a way to specify the endpoint by Tango property (only env. var. so far)
    // TODO: it means that, so far, any endpoint specified through Interface::Configuration
    // TODO: will be ignored - it here there for (near) future use- we simple pass an empty
    // TODO: string till we provide the ability to get the endpoint using a Tango property.
    Tango::telemetry::Configuration cfg{telemetry_enabled,
                                        kernel_traces_enabled,
                                        device_name,
                                        "tango",
                                        Tango::telemetry::Configuration::Server{device_class->get_name(), device_name},
                                        ""};

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
    telemetry_interface.reset();
}

#endif // #if defined(TANGO_USE_TELEMETRY)

} // namespace Tango
