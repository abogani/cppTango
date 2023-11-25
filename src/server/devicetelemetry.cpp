//------------------------------------------------------------------------------------------------
//         THE FOLLOWING CODE EXTENDS THE 'DeviceImpl' CLASS IN A DEDICATED .CPP FILE
//      WE SIMPLY REUSE THE APPROACH USED FOR THE LOGGING SERVICE FOR CONSISTENCY REASONS
//------------------------------------------------------------------------------------------------

#include <tango/tango.h>

namespace Tango
{

#if defined(TELEMETRY_ENABLED)

//-------------------------------------------------------------------------------------------------
//  method : DeviceImpl::initialize_telemetry_interface
//-------------------------------------------------------------------------------------------------
void DeviceImpl::initialize_telemetry_interface() noexcept
{
    // std::cout << "DeviceImpl::initialize_telemetry_interface: " << device_name << "@" << std::hex << this << std::dec
    // << " << " << std::endl;

    // TOD: ill-formatted endpint causes crash in otel - e.g., localhost::4317 (two ':')
    std::string endpoint{"localhost:4317"};
    ApiUtil::instance()->get_env_var("TANGO_TELEMETRY_ENDPOINT", endpoint);

    Tango::telemetry::Interface::Configuration cfg{
        device_name, "tango", Tango::telemetry::Interface::Server{device_class->get_name(), device_name}, endpoint};

    telemetry_interface = Tango::telemetry::InterfaceFactory::create(cfg);

    // std::cout << "DeviceImpl::initialize_telemetry_interface: " << device_name << "@" << std::hex << this << std::dec
    // << " << " << std::endl;
}

//-------------------------------------------------------------------------------------------------
//  method : DeviceImpl::cleanup_telemetry_interface
//-------------------------------------------------------------------------------------------------
void DeviceImpl::cleanup_telemetry_interface() noexcept
{
    // std::cout << "DeviceImpl::cleanup_telemetry_interface <<" << std::endl;

    if(telemetry_interface)
    {
        telemetry_interface->terminate();
    }

    // std::cout << "DeviceImpl::cleanup_telemetry_interface >>" << std::endl;
}

#endif // #if defined(TELEMETRY_ENABLED)

} // namespace Tango
