#ifndef TANGO_COMMON_TELEMETRY_CONFIGURATION_H
#define TANGO_COMMON_TELEMETRY_CONFIGURATION_H

#include <tango/common/tango_version.h>

#if defined(TANGO_USE_TELEMETRY)

  #include <string>
  #include <variant>

namespace Tango::telemetry
{

//-----------------------------------------------------------------------------------------------------------------
//! The telemetry configuration class
//-----------------------------------------------------------------------------------------------------------------
class Configuration
{
  public:
    //-----------------------------------------------------------------------------------------------------------------
    //  CONFIGURATION
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! The kinds of configuration.
    //-----------------------------------------------------------------------------------------------------------------
    enum class Kind
    {
        //- telemetry for pure Tango clients.
        Client,
        //- telemetry for Tango servers (i.e., Tango devices).
        Server
    };

    //-----------------------------------------------------------------------------------------------------------------
    //! Some specific configuration info for clients.
    //-----------------------------------------------------------------------------------------------------------------
    struct Client
    {
        // tell the world that this struct relies on default ctors and assignment operators
        Client() = default;
        Client(const Client &) = default;
        Client &operator=(const Client &) = default;
        Client &operator=(Client &&) = default;

        //- the pure Tango client name: used as the "service name" (opentelemetry semantic convention)
        std::string name{"undefined Tango client name"};
    };

    //-----------------------------------------------------------------------------------------------------------------
    //! Some specific configuration info for servers (i.e. Tango devices).
    //-----------------------------------------------------------------------------------------------------------------
    struct Server
    {
        // tell the world that this struct relies on default ctors and assignment operators
        Server() = default;
        Server(const Server &) = default;
        Server &operator=(const Server &) = default;
        Server &operator=(Server &&) = default;

        //- the Tango class name: used as the "service.name" (opentelemetry semantic convention)
        std::string class_name{"undefined Tango class name"};
        //- the Tango device name: used as the "service.instance.id" (opentelemetry semantic convention)
        std::string device_name{"undefined Tango device name"};
    };

    using ServerClientDetails = std::variant<Server, Client>;

    ///! Available exporter types for logs and traces
    ///!
    ///! \see Configuration::get_exporter_from_env
    enum class Exporter
    {
        grpc,
        http,
        console,
        none
    };

    Configuration(std::string id, std::string name_space, ServerClientDetails details);

    // default configuration
    Configuration();

    // tell the world that this struct relies on default copy and assignment operators
    Configuration(const Configuration &) = default;
    Configuration &operator=(const Configuration &) = default;
    Configuration &operator=(Configuration &&) = default;

    //! The batch size for traces
    std::size_t traces_batch_size{Configuration::DEFAULT_TRACES_BATCH_SIZE};

    //! The batch size for logs
    std::size_t logs_batch_size{Configuration::DEFAULT_LOGS_BATCH_SIZE};

    //! The max queue size for traces and logs - threshold above witch signals are dropped
    std::size_t max_batch_queue_size{Configuration::DEFAULT_MAX_BATCH_QUEUE_SIZE};

    //! The delay (in ms) after which a batch processing is scheduled whatever is the number of pending signals in the
    //! queue
    std::size_t batch_schedule_delay_in_milliseconds{Configuration::DEFAULT_BATCH_SCHEDULE_DELAY};

    //! Get the 'kind' of the configuration.
    //! \see Configuration::Kind.
    Configuration::Kind get_kind() const noexcept;

    //! Check the 'kind' of the configuration.
    //! Returns true if the Configuration is of the specified Configuration::Kind, returns false otherwise.
    //!
    //! @param kind The Configuration::Kind to be compared to the actual kind of the configuration.
    //!
    //! \see Configuration::Kind.
    bool is_a(const Configuration::Kind &kind) const noexcept;

    //! Extract "host:port" from specified grpc endpoint.
    //! An empty string is returned if the specified endpoint is invalid.
    //! A valid telemetry grpc data collector endpoint is of the form: "grpc://addr:port".
    //!
    //! @param endpoint The telemetry endpoint from which the 'host:port' substring is to extracted.
    std::string extract_grpc_host_port(const std::string &endpoint) noexcept;

    //! Set to true (the default) to enable to the interface at instantiation.
    //! A disabled interface acts as a "no-op" one (i.e. signals, like traces, are silently dropped).
    bool enabled;

    //! Set to true to enable some of the kernel traces that are hidden by default.
    bool kernel_traces_enabled;

    //! An optional telemetry interface name (identifier)
    std::string id;

    //! The "name space" or "subsystem name" to which the Interface owner belongs to (for logical organisation of
    //! the telemetry signals). Things like "diagnostics" or "power-supplies" are examples of name spaces.
    std::string name_space;

    //! The client or server details.
    //! \see Configuration::Client and Configuration::Server.
    ServerClientDetails details;

    //! The telemetry data collector endpoint for traces/logs - a string of the form: "http://addr:port/..." or
    //! "grpc://addr:port"
    std::string traces_endpoint, logs_endpoint;
    Exporter traces_exporter, logs_exporter;

  private:
    //! Get the traces endpoint from the dedicated env. variable and the given exporter type
    //! Uses a defaults value in case the env. variable is undefined.
    //! A Tango::DevFailed exception will thrown in case the specified endpoint (i.e. the content of the
    //! env. variable) is (syntactically) invalid. A valid telemetry data collector endpoint is of the
    //! form: "http://addr:port/..." or "grpc://addr:port".
    //!
    //! \returns A std::string containing the traces endpoint.
    //!
    //! \see Interface::kEnvVarTelemetryTracesEndPoint.
    //! \see Configuration::DEFAULT_GRPC_TRACES_ENDPOINT and Configuration::DEFAULT_HTTP_TRACES_ENDPOINT.
    std::string get_traces_endpoint_from_env(Exporter exporter_type);

    //! Get the traces endpoint from the dedicated env. variable and the given exporter type
    //!
    //! Uses a defaults value in case the env. variable is undefined.
    //! A Tango::DevFailed exception will thrown in case the specified endpoint
    //! (i.e. the content of the env. variable) is (syntactically) invalid. A valid
    //! telemetry data collector endpoint is of the form: "http://addr:port/..." or "grpc://addr:port".
    //!
    //! \returns A std::string containing the logs endpoint.
    //!
    //! \see Interface::kEnvVarTelemetryLogsEndPoint.
    //! \see Configuration::DEFAULT_GRPC_LOGS_ENDPOINT and Configuration::DEFAULT_HTTP_LOGS_ENDPOINT.
    std::string get_logs_endpoint_from_env(Exporter exporter_type);

    //! Fetch the exporter type from the given env. variable
    //!
    //! Defaults to Configuration::kDefaultExporter
    Exporter get_exporter_from_env(const char *env_var);

    //! Check that endpoint describes a valid endpoint for the given exporter type, throws on error
    void ensure_valid_endpoint(const char *env_var, Configuration::Exporter exporter_type, const std::string &endpoint);

    //! Check the syntactic validity of the specified endpoint.
    //! Returns true if the specified string contains a syntactically valid http endpoint, returns false otherwise.
    //! A valid telemetry http data collector endpoint is of the form: "http://addr:port/...".
    //!
    //! @param endpoint The telemetry endpoint to be checked.
    bool is_valid_http_endpoint(const std::string &endpoint) noexcept;

    //! Check the syntactic validity of the specified endpoint.
    //! Returns true if the specified string contains a syntactically grpc endpoint, returns false otherwise.
    //! A valid telemetry grpc data collector endpoint is of the form: "grpc://addr:port".
    //!
    //! @param endpoint The telemetry endpoint to be checked.
    bool is_valid_grpc_endpoint(const std::string &endpoint) noexcept;

    //! Check the syntactic validity of the specified endpoint.
    //! Returns true if the specified string contains a syntactically console endpoint, returns false otherwise.
    //! A valid telemetry console data collector endpoint is of the form "cout" or "cerr"
    //!
    //! @param endpoint The telemetry endpoint to be checked.
    bool is_valid_console_endpoint(const std::string &endpoint) noexcept;
    static const Exporter kDefaultExporter{Exporter::console};

    //! Parse the given string as Exporter, throws on error
    Exporter to_exporter(std::string_view str);

    //-----------------------------------------------------------------------------------------------------------------
    //! The default gRPC endpoint to which the telemetry data is exported: grpc://localhost:4317
    //-----------------------------------------------------------------------------------------------------------------
    static const std::string DEFAULT_GRPC_TRACES_ENDPOINT;

    //-----------------------------------------------------------------------------------------------------------------
    //! The default HTTP endpoint to which the telemetry data is exported: http://localhost:4318/v1/traces
    //-----------------------------------------------------------------------------------------------------------------
    static const std::string DEFAULT_HTTP_TRACES_ENDPOINT;

    //-----------------------------------------------------------------------------------------------------------------
    //! The default console endpoint to which the telemetry data is exported: cout
    //-----------------------------------------------------------------------------------------------------------------
    static const std::string DEFAULT_CONSOLE_TRACES_ENDPOINT;

    //-------------------------------------------------------------------------------------------------
    //! The default endpoint to which logs are exported
    //-------------------------------------------------------------------------------------------------
    static const std::string DEFAULT_GRPC_LOGS_ENDPOINT;

    //-------------------------------------------------------------------------------------------------
    //! The default endpoint to which logs are exported
    //-------------------------------------------------------------------------------------------------
    static const std::string DEFAULT_HTTP_LOGS_ENDPOINT;

    //-----------------------------------------------------------------------------------------------------------------
    //! The default console endpoint to which the telemetry data is exported
    //-----------------------------------------------------------------------------------------------------------------
    static const std::string DEFAULT_CONSOLE_LOGS_ENDPOINT;

    //-----------------------------------------------------------------------------------------------------------------
    //! The default batch size for traces
    //-----------------------------------------------------------------------------------------------------------------
    static const std::size_t DEFAULT_TRACES_BATCH_SIZE;

    //-----------------------------------------------------------------------------------------------------------------
    //! The default batch size for logs
    //-----------------------------------------------------------------------------------------------------------------
    static const std::size_t DEFAULT_LOGS_BATCH_SIZE;

    //--------------------------------------------------------s---------------------------------------------------------
    //! The default max batch queue size (threshold above which signals are dropped - common to traces and logs)
    //-----------------------------------------------------------------------------------------------------------------
    static const std::size_t DEFAULT_MAX_BATCH_QUEUE_SIZE;

    //-----------------------------------------------------------------------------------------------------------------
    //! The default delay (in ms) after which a batch processing is scheduled whatever is the number of pending signals
    //! in the queue (common to traces and logs)
    //-----------------------------------------------------------------------------------------------------------------
    static const std::size_t DEFAULT_BATCH_SCHEDULE_DELAY;
};

std::string to_string(Configuration::Exporter exporter_type);

} // namespace Tango::telemetry

#endif // TANGO_USE_TELEMETRY

#endif // TANGO_COMMON_TELEMETRY_CONFIGURATION_H
