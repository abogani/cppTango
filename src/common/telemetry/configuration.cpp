#include <tango/common/telemetry/configuration.h>

#include <tango/internal/utils.h>

#include <tango/tango.h>

#include <string>
#include <variant>

namespace Tango::telemetry
{

std::string to_string(Configuration::Exporter exporter_type)
{
    switch(exporter_type)
    {
    case Configuration::Exporter::grpc:
        return "grpc";
    case Configuration::Exporter::http:
        return "http";
    case Configuration::Exporter::console:
        return "console";
    default:
        using ut = std::underlying_type_t<Configuration::Exporter>;
        return std::to_string(static_cast<ut>(exporter_type));
    }
}

//-----------------------------------------------------------------------------------------
//! The default endpoint to which traces are exported
//-----------------------------------------------------------------------------------------
const std::string Configuration::DEFAULT_GRPC_TRACES_ENDPOINT{"grpc://localhost:4317"};

//-----------------------------------------------------------------------------------------
//! The default endpoint to which traces are exported
//-----------------------------------------------------------------------------------------
const std::string Configuration::DEFAULT_HTTP_TRACES_ENDPOINT{"http://localhost:4318/v1/traces"};

const std::string Configuration::DEFAULT_CONSOLE_TRACES_ENDPOINT{"cout"};

//-----------------------------------------------------------------------------------------
//! The default endpoint to which logs are exported
//-----------------------------------------------------------------------------------------
const std::string Configuration::DEFAULT_GRPC_LOGS_ENDPOINT{"grpc://localhost:4317"};

//-----------------------------------------------------------------------------------------
//! The default endpoint to which logs are exported
//-----------------------------------------------------------------------------------------
const std::string Configuration::DEFAULT_HTTP_LOGS_ENDPOINT{"http://localhost:4318/v1/logs"};

const std::string Configuration::DEFAULT_CONSOLE_LOGS_ENDPOINT{"cout"};

//-----------------------------------------------------------------------------------------
//! The default batch size for traces
//-----------------------------------------------------------------------------------------
const std::size_t Configuration::DEFAULT_TRACES_BATCH_SIZE = 512;

//-----------------------------------------------------------------------------------------
//! The default batch size for logs
//-----------------------------------------------------------------------------------------
const std::size_t Configuration::DEFAULT_LOGS_BATCH_SIZE = 512;

//-----------------------------------------------------------------------------------------
//! The default max batch queue size (threshold above which signals are dropped)
//-----------------------------------------------------------------------------------------
const std::size_t Configuration::DEFAULT_MAX_BATCH_QUEUE_SIZE = 2048;

//-----------------------------------------------------------------------------------------
//! The default delay (in ms) after which a batch processing is scheduled whatever is the
//! number of pending signals in the queue: 2500
//-----------------------------------------------------------------------------------------
const std::size_t Configuration::DEFAULT_BATCH_SCHEDULE_DELAY = 2500;

// TODO: offer a way to specify the endpoint by Tango property (only env. var. so far)
Configuration::Configuration(std::string id, std::string name_space, ServerClientDetails details) :
    id(id),
    name_space(name_space),
    details(details)
{
    auto *detailsServer = std::get_if<Configuration::Server>(&details);

    if(detailsServer != nullptr && detailsServer->class_name == "DServer")
    {
        enabled = false;
    }
    else
    {
        enabled = detail::get_boolean_env_var(Tango::telemetry::kEnvVarTelemetryEnable, false);
    }

    kernel_traces_enabled = detail::get_boolean_env_var(Tango::telemetry::kEnvVarTelemetryKernelEnable, false);

    traces_endpoint = get_traces_endpoint_from_env(Tango::telemetry::Configuration::Exporter::console);
    logs_endpoint = get_logs_endpoint_from_env(Tango::telemetry::Configuration::Exporter::console);

    traces_exporter = get_exporter_from_env(telemetry::kEnvVarTelemetryTracesExporter);
    logs_exporter = get_exporter_from_env(telemetry::kEnvVarTelemetryLogsExporter);
}

Configuration::Kind Configuration::get_kind() const noexcept
{
    if(std::get_if<Configuration::Server>(&details) != nullptr)
    {
        return Configuration::Kind::Server;
    }
    return Configuration::Kind::Client;
}

bool Configuration::is_a(const Configuration::Kind &kind) const noexcept
{
    return get_kind() == kind;
}

bool Configuration::is_valid_http_endpoint(const std::string &endpoint) noexcept
{
    const std::regex pattern("^(http|https)://[^/]+:\\d+(/.*)?$");
    return std::regex_match(endpoint, pattern);
}

bool Configuration::is_valid_console_endpoint(const std::string &endpoint) noexcept
{
    const std::regex pattern("^(cout|cerr)$");
    return std::regex_match(endpoint, pattern);
}

bool Configuration::is_valid_grpc_endpoint(const std::string &endpoint) noexcept
{
    // regex pattern to match 'host:port'
    const std::regex pattern("^grpc://[^/]+:\\d+$");
    return std::regex_match(endpoint, pattern);
}

std::string Configuration::extract_grpc_host_port(const std::string &endpoint) noexcept
{
    // regex pattern to match and capture 'host:port' from 'grpc://host:port'
    const std::regex pattern("^(?:grpc://)?([^/]+:\\d+)$");
    std::smatch matches;
    if(std::regex_search(endpoint, matches, pattern) && matches.size() > 1)
    {
        // valid endpoint return: host:port
        return matches[1].str();
    }
    else
    {
        // invalid endpoint return: empty string
        return "";
    }
}

/// @throw Tango::API_InvalidArgs
Configuration::Exporter Configuration::to_exporter(std::string_view str)
{
    if(str == "grpc")
    {
        return Exporter::grpc;
    }
    else if(str == "http")
    {
        return Exporter::http;
    }
    else if(str == "console")
    {
        return Exporter::console;
    }

    std::stringstream sstr;
    sstr << "Can not parse " << str << " as Exporter enum class.";
    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, sstr.str());
}

/// @throw Tango::API_InvalidArgs
Configuration::Exporter Configuration::get_exporter_from_env(const char *env_var)
{
    std::string exp;
    int ret = ApiUtil::instance()->get_env_var(env_var, exp);

    Exporter exporter_type = ret != 0 ? kDefaultExporter : to_exporter(detail::to_lower(exp));

    switch(exporter_type)
    {
    case Exporter::grpc:
#if !defined(TANGO_TELEMETRY_USE_GRPC)
        TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs,
                              "Requested grpc trace exporter, but compiled without GRPC support.");
#else
        break;
#endif
    case Exporter::http:
#if !defined(TANGO_TELEMETRY_USE_HTTP)
        TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs,
                              "Requested http trace exporter, but compiled without HTTP support.");
#else
        break;
#endif
    case Exporter::console:
        // nothing to check
        break;
    default:
        TANGO_ASSERT_ON_DEFAULT(exporter_type);
    }

    return exporter_type;
}

/// @throw Tango::API_InvalidArgs
void Configuration::ensure_valid_endpoint(const char *env_var,
                                          Configuration::Exporter exporter_type,
                                          const std::string &endpoint)
{
    switch(exporter_type)
    {
    case Exporter::grpc:
        if(!Configuration::is_valid_grpc_endpoint(endpoint))
        {
            std::stringstream err;
            err << "the specified telemetry endpoint '" << endpoint << "' is invalid - ";
            err << "check the " << env_var << " env. var. - ";
            err << "expecting a valid gRPC endpoint - e.g., grpc://localhost:4318";
            TANGO_LOG << err.str() << std::endl;
            TANGO_THROW_EXCEPTION(API_InvalidArgs, err.str());
        }
        break;
    case Exporter::http:
        if(!Configuration::is_valid_http_endpoint(endpoint))
        {
            std::stringstream err;
            err << "the specified telemetry endpoint '" << endpoint << "' is invalid - ";
            err << "check the " << env_var << " env. var. - ";
            err << "expecting a valid http[s]:// url - e.g., http://localhost:4317/v1/traces";
            TANGO_LOG << err.str() << std::endl;
            TANGO_THROW_EXCEPTION(API_InvalidArgs, err.str());
        }
        break;
    case Exporter::console:
        if(!Configuration::is_valid_console_endpoint(endpoint))
        {
            std::stringstream err;
            err << "the specified telemetry endpoint '" << endpoint << "' is invalid - ";
            err << "check the " << env_var << " env. var. - ";
            err << R"(expecting "cout" or "cerr")";
            TANGO_LOG << err.str() << std::endl;
            TANGO_THROW_EXCEPTION(API_InvalidArgs, err.str());
        }
        break;
    default:
        TANGO_ASSERT_ON_DEFAULT(exporter_type);
    }
}

std::string Configuration::get_traces_endpoint_from_env(Exporter exporter_type)
{
    std::string endpoint;

    // get traces endpoint from env. variable.
    int ret = ApiUtil::instance()->get_env_var(kEnvVarTelemetryTracesEndPoint, endpoint);

    // use default endpoint if none provided
    if(ret != 0)
    {
        switch(exporter_type)
        {
        case Exporter::grpc:
            endpoint = Configuration::DEFAULT_GRPC_TRACES_ENDPOINT;
            break;
        case Exporter::http:
            endpoint = Configuration::DEFAULT_HTTP_TRACES_ENDPOINT;
            break;
        case Exporter::console:
            endpoint = Configuration::DEFAULT_CONSOLE_TRACES_ENDPOINT;
            break;
        default:
            TANGO_ASSERT_ON_DEFAULT(exporter_type);
        }
    }

    ensure_valid_endpoint(kEnvVarTelemetryTracesEndPoint, exporter_type, endpoint);

    return endpoint;
}

std::string Configuration::get_logs_endpoint_from_env(Exporter exporter_type)
{
    std::string endpoint;

    // get logs endpoint from env. variable.
    int ret = ApiUtil::instance()->get_env_var(kEnvVarTelemetryLogsEndPoint, endpoint);

    // use default endpoint if none provided
    if(ret != 0)
    {
        switch(exporter_type)
        {
        case Exporter::grpc:
            endpoint = Configuration::DEFAULT_GRPC_LOGS_ENDPOINT;
            break;
        case Exporter::http:
            endpoint = Configuration::DEFAULT_HTTP_LOGS_ENDPOINT;
            break;
        case Exporter::console:
            endpoint = Configuration::DEFAULT_CONSOLE_LOGS_ENDPOINT;
            break;
        default:
            TANGO_ASSERT_ON_DEFAULT(exporter_type);
        }
    }

    ensure_valid_endpoint(kEnvVarTelemetryLogsEndPoint, exporter_type, endpoint);

    return endpoint;
}

} // namespace Tango::telemetry
