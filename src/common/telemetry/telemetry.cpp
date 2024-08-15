
#include <tango/tango.h>

#include <tango/common/git_revision.h>

#include <tango/internal/utils.h>

#include <tango/common/telemetry/configuration.h>

#include <iostream>
#include <regex>
#include <type_traits>

#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span_context.h>

#include <opentelemetry/sdk/version/version.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/ostream/span_exporter_factory.h>

#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>

#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>
#include <opentelemetry/sdk/logs/batch_log_record_processor_options.h>
#include <opentelemetry/sdk/logs/batch_log_record_processor_factory.h>

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/exporters/ostream/log_record_exporter.h>
#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_log_record_exporter_factory.h>

namespace Tango::telemetry
{

//-----------------------------------------------------------------------------------------
// The telemetry::Interface currently attached to the current thread  (thread_local - TSS)
//-----------------------------------------------------------------------------------------
thread_local InterfacePtr current_telemetry_interface{nullptr};

//-----------------------------------------------------------------------------------------
// Ptr to opentelemetry tracer
//-----------------------------------------------------------------------------------------
using TracerPtr = opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>;

//-----------------------------------------------------------------------------------------
//  SPAN-IMPLEMENTATION
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//  SpanImplementation
//-----------------------------------------------------------------------------------------
class SpanImplementation final
{
  public:
    // the actual/concrete opentelemetry span
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> otel_span;

    // the Span status (no way to retrieve the current status on the otel. span)
    Span::Status span_status{Span::Status::kUnset};

    //-------------------------------------------------------------------------------------
    // SpanImplementation::SpanImplementation
    //-------------------------------------------------------------------------------------
    explicit SpanImplementation(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span) :
        otel_span(std::move(span))
    {
    }

    //-------------------------------------------------------------------------------------
    // SpanImplementation::opentelemetry_span
    //-------------------------------------------------------------------------------------
    opentelemetry::trace::Span &opentelemetry_span() noexcept
    {
        return *otel_span;
    }

    //-------------------------------------------------------------------------------------
    // SpanImplementation::set_attribute
    //-------------------------------------------------------------------------------------
    void set_attribute(const std::string &key, const AttributeValue &value) noexcept
    {
        auto to_opentelemetry_attribute_value = [](auto &&arg) -> opentelemetry::common::AttributeValue
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr(std::is_same_v<T, std::string>)
            {
                // convert std::string to opentelemetry::nostd::string_view
                return opentelemetry::nostd::string_view(arg);
            }
            else
            {
                static_assert(detail::is_one_of<T, opentelemetry::common::AttributeValue>::value,
                              "Unsupported type in Tango::telemetry::AttributeValue");

                // direct mapping for other supported types
                return arg;
            }
        };

        if(otel_span)
        {
            otel_span->SetAttribute(key, std::visit(to_opentelemetry_attribute_value, value));
        }
    }

    //-------------------------------------------------------------------------------------
    // SpanImplementation::add_event
    //-------------------------------------------------------------------------------------
    void add_event(const std::string &name, const Attributes &attributes) noexcept
    {
        if(otel_span)
        {
            std::map<std::string, opentelemetry::common::AttributeValue> otel_attributes;

            for(const auto &attribute : attributes)
            {
                std::visit(
                    [&](auto &&arg)
                    {
                        // Convert each type to opentelemetry::trace::AttributeValue and add to convertedAttributes
                        otel_attributes[attribute.first] = opentelemetry::common::AttributeValue(arg);
                    },
                    attribute.second);
            }

            otel_span->AddEvent(name, otel_attributes);
        }
    }

    //-------------------------------------------------------------------------------------
    // SpanImplementation::set_status
    //-------------------------------------------------------------------------------------
    void set_status(const Span::Status &status, const std::string &description = "") noexcept
    {
        if(otel_span)
        {
            // see otel. specs on span status - description mandatory for status == Span::Status::kError
            // https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/trace/api.md#set-status

            span_status = status;

            auto to_opentelemetry_code = [](Span::Status code)
            {
                switch(code)
                {
                case Span::Status::kOk:
                    return opentelemetry::trace::StatusCode::kOk;
                case Span::Status::kError:
                    return opentelemetry::trace::StatusCode::kError;
                case Span::Status::kUnset:
                default:
                    return opentelemetry::trace::StatusCode::kUnset;
                }
            };

            otel_span->SetStatus(to_opentelemetry_code(status), description);
        }
    }

    //-------------------------------------------------------------------------------------
    // SpanImplementation::get_status
    //-------------------------------------------------------------------------------------
    Span::Status get_status() const noexcept
    {
        return span_status;
    }

    //-------------------------------------------------------------------------------------
    // SpanImplementation::end
    //-------------------------------------------------------------------------------------
    void end() noexcept
    {
        if(otel_span)
        {
            otel_span->End();
        }
    }

    //-------------------------------------------------------------------------------------
    // SpanImplementation::is_recording
    //-------------------------------------------------------------------------------------
    bool is_recording() const noexcept
    {
        return otel_span ? otel_span->IsRecording() : false;
    }
};

//-----------------------------------------------------------------------------------------
//  SPAN
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
// Span::~Span
//-----------------------------------------------------------------------------------------
Span::~Span() { }

//-----------------------------------------------------------------------------------------
// Span::set_attribute
//-----------------------------------------------------------------------------------------
void Span::set_attribute(const std::string &key, const AttributeValue &value) noexcept
{
    impl->set_attribute(key, value);
}

//-----------------------------------------------------------------------------------------
// Span::add_event
//-----------------------------------------------------------------------------------------
void Span::add_event(const std::string &name, const Attributes &attributes) noexcept
{
    impl->add_event(name, attributes);
}

//-----------------------------------------------------------------------------------------
// Span::set_status
//-----------------------------------------------------------------------------------------
void Span::set_status(const Span::Status &status, const std::string &description) noexcept
{
    impl->set_status(status, description);
}

//-----------------------------------------------------------------------------------------
// Span::get_status
//-----------------------------------------------------------------------------------------
Span::Status Span::get_status() const noexcept
{
    return impl->get_status();
}

//-----------------------------------------------------------------------------------------
// Span::end
//-----------------------------------------------------------------------------------------
void Span::end() noexcept
{
    impl->end();
}

//-----------------------------------------------------------------------------------------
// Span::is_recording
//-----------------------------------------------------------------------------------------
bool Span::is_recording() const noexcept
{
    return impl->is_recording();
}

//-----------------------------------------------------------------------------------------
// ScopeImplementation
//-----------------------------------------------------------------------------------------
class ScopeImplementation final
{
  public:
    ScopeImplementation(const SpanPtr &span) noexcept :
        token(opentelemetry::context::RuntimeContext::Attach(
            opentelemetry::context::RuntimeContext::GetCurrent().SetValue(opentelemetry::trace::kSpanKey,
                                                                          span->impl->otel_span)))
    {
    }

  private:
    opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> token;
};

//-----------------------------------------------------------------------------------------
//  SCOPE
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
// Scope::Scope
//-----------------------------------------------------------------------------------------
Scope::Scope(const SpanPtr &span) noexcept
{
    // an Scope without a valid "implementation" is invalid and can't be used.
    // but, we don't throw any exception if the instantiation failed! Yes, sir. See:
    // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f6-if-your-function-must-not-throw-declare-it-noexcep

    impl = std::make_unique<ScopeImplementation>(span);
}

//-----------------------------------------------------------------------------------------
// Scope::~Scope
//-----------------------------------------------------------------------------------------
Scope::~Scope() { }

//-----------------------------------------------------------------------------------------
// TANGO CARRIER
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
// TangoCarrier
//-----------------------------------------------------------------------------------------
// OpenTelemetry does not specify how the context is propagated. It simply provides a mechanism for
// injecting and extracting the context. This mechanism relies on a Propagator that itself delegates
// the actual I/O actions to a Carrier implementing a 'Set' (injection) and a 'Get' (extraction)
// method. We consequently have to provide a 'TangoCarrier' so that we will be able to inject/extract
// the the trace context from the data struct that carries it. So far, the context information is
// encapsulated into the ClntIdent data struct (of the CORBA IDL) passed by a client (the caller)
// to a server (the callee).
//-----------------------------------------------------------------------------------------
class TangoTextMapCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
  public:
    TangoTextMapCarrier() = default;

    //-------------------------------------------------------------------------------------
    // Given a key, returns the associated value or and empty if there's no such key
    //-------------------------------------------------------------------------------------
    opentelemetry::nostd::string_view Get(opentelemetry::nostd::string_view key) const noexcept override
    {
        auto it = headers.find(std::string(key));
        if(it != headers.end())
        {
            return it->second;
        }
        return "";
    }

    //-------------------------------------------------------------------------------------
    // Given a key, sets its associated value
    //-------------------------------------------------------------------------------------
    void Set(opentelemetry::nostd::string_view key, opentelemetry::nostd::string_view value) noexcept override
    {
        headers.insert(std::pair<std::string, std::string>(std::string(key), std::string(value)));
    }

  private:
    std::map<std::string, std::string> headers;
};

//-----------------------------------------------------------------------------------------
// helper function: to_opentelemetry_span_kind
//-----------------------------------------------------------------------------------------
inline static opentelemetry::trace::SpanKind to_opentelemetry_span_kind(const Span::Kind &kind)
{
    switch(kind)
    {
    case Span::Kind::kClient:
        return opentelemetry::trace::SpanKind::kClient;
    case Span::Kind::kServer:
        return opentelemetry::trace::SpanKind::kServer;
    case Span::Kind::kProducer:
        return opentelemetry::trace::SpanKind::kProducer;
    case Span::Kind::kConsumer:
        return opentelemetry::trace::SpanKind::kConsumer;
    case Span::Kind::kInternal:
    default:
        return opentelemetry::trace::SpanKind::kInternal;
    }
}

//-----------------------------------------------------------------------------------------
// INTERFACE-IMPLEMENTATION
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
// InterfaceImplementation
//-----------------------------------------------------------------------------------------
class InterfaceImplementation final
{
  public:
    //-------------------------------------------------------------------------------------
    // Ctor
    //-------------------------------------------------------------------------------------
    explicit InterfaceImplementation(const Configuration &config) :
        cfg{config}
    {
        init_tracer_provider();
        // init the global propagator
        init_global_propagator();
    }

    //-------------------------------------------------------------------------------------
    // terminate
    //-------------------------------------------------------------------------------------
    void terminate() noexcept
    {
        // flush traces
        cleanup_tracer_provider();
    }

    //-------------------------------------------------------------------------------------
    // trace provider initialization
    //-------------------------------------------------------------------------------------
    void init_tracer_provider()
    {
        // see the following link for details on tracer naming:
        // https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/trace/api.md#get-a-tracer
        //- the tracer name is the 'instrumentation library' - here, it's simply the cpp version of tango
        std::string tracer_name = "tango.cpp";
        //- the tracer version is the cppTango version
        std::string tracer_version = git_revision();

        if(!cfg.enabled)
        {
            using TracerProviderPtr = opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>;
            provider = TracerProviderPtr{new opentelemetry::trace::NoopTracerProvider};
            tracer = provider->GetTracer(tracer_name, tracer_version);

            return;
        }

        auto exporter_type = cfg.traces_exporter;
        auto endpoint = cfg.traces_endpoint;

        std::unique_ptr<opentelemetry::sdk::trace::SpanExporter> exporter;
        std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor> processor;

        // we now have a valid endpoint for the given exporter type,
        // and we also already have checked the compiled features grpc and http for the requested exporter type

        switch(exporter_type)
        {
        case Configuration::Exporter::grpc:
#if defined(TANGO_TELEMETRY_USE_GRPC)
        {
            opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
            opts.endpoint = cfg.extract_grpc_host_port(endpoint);
            opts.use_ssl_credentials = false;
            exporter = opentelemetry::exporter::otlp::OtlpGrpcExporterFactory::Create(opts);
        }
#endif
        break;
        case Configuration::Exporter::http:
#if defined(TANGO_TELEMETRY_USE_HTTP)
        {
            opentelemetry::exporter::otlp::OtlpHttpExporterOptions opts;
            opts.url = endpoint;
            exporter = opentelemetry::exporter::otlp::OtlpHttpExporterFactory::Create(opts);
        }
#endif
        break;
        case Configuration::Exporter::console:
            if(endpoint == "cout")
            {
                exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create(std::cout);
            }
            else if(endpoint == "cerr")
            {
                exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create(std::cerr);
            }
            else
            {
                TANGO_ASSERT(false);
            }
            break;
        default:
            TANGO_ASSERT_ON_DEFAULT(exporter_type);
        }

        TANGO_ASSERT(exporter);

        switch(exporter_type)
        {
        case Configuration::Exporter::grpc:
        case Configuration::Exporter::http:
        {
            opentelemetry::sdk::trace::BatchSpanProcessorOptions opts;
            opts.max_queue_size = cfg.max_batch_queue_size;
            opts.max_export_batch_size = cfg.traces_batch_size;
            opts.schedule_delay_millis = std::chrono::milliseconds(cfg.batch_schedule_delay_in_milliseconds);
            processor = opentelemetry::sdk::trace::BatchSpanProcessorFactory::Create(std::move(exporter), opts);
        }
        break;
        case Configuration::Exporter::console:
            // fix garbeled output with batch processing
            processor = opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(std::move(exporter));
            break;
        default:
            TANGO_ASSERT_ON_DEFAULT(exporter_type);
        }

        Tango::Util *util{nullptr};
        try
        {
            util = Tango::Util::instance(false);
        }
        catch(...)
        {
            // ok, we are starting up... not a big deal
        }

        auto *api_util = Tango::ApiUtil::instance();

        std::string tango_host;
        api_util->get_env_var("TANGO_HOST", tango_host);

        opentelemetry::sdk::resource::ResourceAttributes resource_attributes;

        // check interface configuration kind
        if(cfg.is_a(Configuration::Kind::Server))
        {
            // interface is instantiated for a server
            const Configuration::Server &srv_info = std::get<0>(cfg.details);
            resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{
                {"service.namespace", cfg.name_space.empty() ? "tango" : cfg.name_space},
                {"service.name", srv_info.class_name},         // naming convention of OpenTelemetry
                {"service.instance.id", srv_info.device_name}, // naming convention of OpenTelemetry
                {"tango.server.name",
                 util != nullptr ? util->get_ds_exec_name() + "/" + util->get_ds_inst_name() : "unknown"},
                {"tango.process.id", api_util->get_client_pid()},
                {"tango.process.kind", api_util->in_server() ? "server" : "client"},
                {"tango.host", tango_host}};
        }
        else
        {
            // interface is instantiated for a client
            const Configuration::Client &clt_info = std::get<1>(cfg.details);
            resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{
                {"service.namespace", cfg.name_space.empty() ? "tango" : cfg.name_space},
                {"service.name", clt_info.name}, // naming convention of OpenTelemetry
                {"tango.process.id", api_util->get_client_pid()},
                {"tango.process.kind", api_util->in_server() ? "server" : "client"},
                {"tango.host", tango_host}};
        }

        auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);

        TANGO_ASSERT(processor);
        provider = opentelemetry::sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);

        tracer = provider->GetTracer(tracer_name, tracer_version);
    }

    //-------------------------------------------------------------------------------------
    // trace provider cleanup
    //-------------------------------------------------------------------------------------
    void cleanup_tracer_provider() noexcept
    {
        if(provider)
        {
            auto *base_class = dynamic_cast<opentelemetry::sdk::trace::TracerProvider *>(provider.get());
            if(base_class != nullptr)
            {
                base_class->ForceFlush();
            }
        }
    }

    //-------------------------------------------------------------------------------------
    // global propagator initialization
    //-------------------------------------------------------------------------------------
    void init_global_propagator() noexcept
    {
        const std::lock_guard<std::mutex> lock(InterfaceImplementation::global_propagator_initialized_mutex);

        // no mutex need cause devices are created sequentially at startup and the first device is created
        // there's no more danger to have a race condition on the global_propagator_initialized
        if(!InterfaceImplementation::global_propagator_initialized)
        {
            opentelemetry::context::propagation::GlobalTextMapPropagator::SetGlobalPropagator(
                opentelemetry::nostd::shared_ptr<opentelemetry::context::propagation::TextMapPropagator>(
                    new(std::nothrow) opentelemetry::trace::propagation::HttpTraceContext()));

            InterfaceImplementation::global_propagator_initialized = true;
        }
    }

    //-------------------------------------------------------------------------------------
    // get_tracer
    //-------------------------------------------------------------------------------------
    TracerPtr get_tracer() const noexcept
    {
        return tracer;
    }

    //-------------------------------------------------------------------------------------
    // instantiate_span
    //-------------------------------------------------------------------------------------
    SpanPtr instantiate_span(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> otel_span) const noexcept
    {
        //----------------------------------------------------------------------------------------
        // the following code could look weird but it allows to obtain a Span class that can not be instantiated
        // externally (i.e., from the user space).
        //----------------------------------------------------------------------------------------
        // step-1: instantiate a span without any impl.
        // here we follow the iso-cpp guidelines: don't deal with bad_alloc cause nobody can recover from it! see:
        // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f6-if-your-function-must-not-throw-declare-it-noexcept
        auto *span = new(std::nothrow) Span();
        // step-2: instantiate the impl then attach it to the span
        span->impl = std::make_unique<SpanImplementation>(std::move(otel_span));
        // step-3: return the new span
        return std::unique_ptr<Span>(span);
    }

    //-------------------------------------------------------------------------------------
    // start_span
    //-------------------------------------------------------------------------------------
    SpanPtr start_span(const std::string &name, const Attributes &attributes, const Span::Kind &kind) noexcept
    {
        std::map<std::string, opentelemetry::common::AttributeValue> otel_attributes;

        for(const auto &attribute : attributes)
        {
            std::visit(
                [&](auto &&arg)
                {
                    // Convert each type to opentelemetry::trace::AttributeValue and add to convertedAttributes
                    otel_attributes[attribute.first] = opentelemetry::common::AttributeValue(arg);
                },
                attribute.second);
        }

        auto otel_span = get_tracer()->StartSpan(
            name,
            otel_attributes,
            {{}, {}, opentelemetry::trace::SpanContext(false, false), to_opentelemetry_span_kind(kind)});

        return instantiate_span(otel_span);
    }

    //-------------------------------------------------------------------------------------
    // start_span
    //-------------------------------------------------------------------------------------
    SpanPtr start_span(const std::string &name,
                       const Attributes &attributes,
                       const opentelemetry::trace::StartSpanOptions &options) noexcept
    {
        std::map<std::string, opentelemetry::common::AttributeValue> otel_attributes;

        for(const auto &attribute : attributes)
        {
            std::visit(
                [&](auto &&arg)
                {
                    // Convert each type to opentelemetry::trace::AttributeValue and add to convertedAttributes
                    otel_attributes[attribute.first] = opentelemetry::common::AttributeValue(arg);
                },
                attribute.second);
        }

        auto otel_span = get_tracer()->StartSpan(name, otel_attributes, options);

        return instantiate_span(otel_span);
    }

    //-------------------------------------------------------------------------------------
    // get_current_span
    //-------------------------------------------------------------------------------------
    SpanPtr get_current_span() const noexcept

    {
        auto current_otel_span = get_tracer()->GetCurrentSpan();

        return instantiate_span(current_otel_span);
    }

    //-------------------------------------------------------------------------------------
    // get_current_opentelemetry_span
    //-------------------------------------------------------------------------------------
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> get_current_opentelemetry_span() noexcept

    {
        return get_tracer()->GetCurrentSpan();
    }

    // Avoid lifetime issues with static storage duration objects
    opentelemetry::nostd::shared_ptr<const opentelemetry::context::RuntimeContextStorage> rcsKeep{
        opentelemetry::context::RuntimeContext::GetConstRuntimeContextStorage()};
    opentelemetry::nostd::shared_ptr<const opentelemetry::trace::TraceState> tsKeep{
        opentelemetry::trace::TraceState::GetDefault()};

    // default interface flag
    bool is_default_interface{false};

    // the interface configuration
    Configuration cfg;

    // the opentelemetry tracer/logger provider
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider> provider;
    opentelemetry::nostd::shared_ptr<opentelemetry::logs::LoggerProvider> logger_provider;

    // the actual opentelemetry tracer attached to this interface
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> tracer;

    // the global propagator initialization flag (singleton)
    static bool global_propagator_initialized;

    // the associated mutex protecting its instantiation against race conditions
    // required for pure clients cause in servers, devices are instantiated sequentially
    static std::mutex global_propagator_initialized_mutex;

    // the default interface - returned when none attached to the current thread
    static Tango::telemetry::InterfacePtr default_telemetry_interface;

    // the associated mutex protecting its instantiation against race conditions
    static std::mutex default_telemetry_interface_mutex;
};

//-----------------------------------------------------------------------------------------
// The default telemetry Interface - used in case none is attached to the current thread
//-----------------------------------------------------------------------------------------
Tango::telemetry::InterfacePtr InterfaceImplementation::default_telemetry_interface{nullptr};

//-----------------------------------------------------------------------------------------
// The mutex protecting the instantiation of the default interface against race conditions
//-----------------------------------------------------------------------------------------
std::mutex InterfaceImplementation::default_telemetry_interface_mutex;

//-----------------------------------------------------------------------------------------
// The "global_propagator_initialized" flag
//-----------------------------------------------------------------------------------------
bool InterfaceImplementation::global_propagator_initialized{false};

//-----------------------------------------------------------------------------------------
// The mutex protecting the instantiation of the global propagator against race conditions
//-----------------------------------------------------------------------------------------
std::mutex InterfaceImplementation::global_propagator_initialized_mutex;

//-----------------------------------------------------------------------------------------
// A pointer to an InterfaceImplementation
//-----------------------------------------------------------------------------------------
using InterfaceImplementationPtr = std::shared_ptr<InterfaceImplementation>;

//-----------------------------------------------------------------------------------------
// APPENDER: routes the Tango logs to the telemetry backend
//-----------------------------------------------------------------------------------------
class Appender : public log4tango::Appender
{
    // the interface config
    InterfaceImplementationPtr interface;
    // the logger name
    std::string logger_name;

  public:
    //-------------------------------------------------------------------------------------
    // Appender::ctor
    //-------------------------------------------------------------------------------------
    Appender(InterfaceImplementationPtr owner) :
        log4tango::Appender(kTelemetryLogAppenderName),
        interface(owner)
    {
        init_logger_provider();
    }

    //-------------------------------------------------------------------------------------
    // Appender::dtor
    //-------------------------------------------------------------------------------------
    ~Appender() override
    {
        cleanup_logger_provider();
    }

    //-------------------------------------------------------------------------------------
    // Appender::init_logger_provider
    //-------------------------------------------------------------------------------------
    void init_logger_provider()
    {
        if(!interface->cfg.enabled)
        {
            cleanup_logger_provider();
            return;
        }

        auto exporter_type = interface->cfg.logs_exporter;
        auto endpoint = interface->cfg.logs_endpoint;

        std::unique_ptr<opentelemetry::sdk::logs::LogRecordExporter> exporter;
        std::unique_ptr<opentelemetry::sdk::logs::LogRecordProcessor> processor;

        // we now have a valid endpoint for the given exporter type,
        // and we also already have checked the compiled features grpc and http for the requested exporter type

        switch(exporter_type)
        {
        case Configuration::Exporter::grpc:
#if defined(TANGO_TELEMETRY_USE_GRPC)
        {
  #if defined(TANGO_TELEMETRY_EXPORTER_OPTION_NEW)
            opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporterOptions opts;
  #else
            opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
  #endif
            opts.endpoint = interface->cfg.extract_grpc_host_port(endpoint);
            opts.use_ssl_credentials = false;
            exporter = opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporterFactory::Create(opts);
        }
#endif
        break;
        case Configuration::Exporter::http:
#if defined(TANGO_TELEMETRY_USE_HTTP)
        {
            opentelemetry::exporter::otlp::OtlpHttpLogRecordExporterOptions opts;
            opts.url = endpoint;
            exporter = opentelemetry::exporter::otlp::OtlpHttpLogRecordExporterFactory::Create(opts);
        }
#endif
        break;
        case Configuration::Exporter::console:
            if(endpoint == "cout")
            {
                exporter = opentelemetry::exporter::logs::OStreamLogRecordExporterFactory::Create(std::cout);
            }
            else if(endpoint == "cerr")
            {
                exporter = opentelemetry::exporter::logs::OStreamLogRecordExporterFactory::Create(std::cerr);
            }
            else
            {
                TANGO_ASSERT(false);
            }
            break;
        default:
            TANGO_ASSERT_ON_DEFAULT(exporter_type);
        }

        TANGO_ASSERT(exporter);

        switch(exporter_type)
        {
        case Configuration::Exporter::grpc:
        case Configuration::Exporter::http:
        {
            opentelemetry::sdk::logs::BatchLogRecordProcessorOptions opts;
            opts.max_queue_size = interface->cfg.max_batch_queue_size;
            opts.max_export_batch_size = interface->cfg.logs_batch_size;
            opts.schedule_delay_millis = std::chrono::milliseconds(interface->cfg.batch_schedule_delay_in_milliseconds);
            processor = opentelemetry::sdk::logs::BatchLogRecordProcessorFactory::Create(std::move(exporter), opts);
        }
        break;
        case Configuration::Exporter::console:
            // fix garbeled output with batch processing
            processor = opentelemetry::sdk::logs::SimpleLogRecordProcessorFactory::Create(std::move(exporter));
            break;
        default:
            TANGO_ASSERT_ON_DEFAULT(exporter_type);
        }

        Tango::Util *util{nullptr};
        try
        {
            util = Tango::Util::instance(false);
        }
        catch(...)
        {
            // ok, we are starting up... not a big deal
        }

        auto *api_util = Tango::ApiUtil::instance();

        std::string tango_host;
        api_util->get_env_var("TANGO_HOST", tango_host);

        opentelemetry::sdk::resource::ResourceAttributes resource_attributes;

        // check interface configuration kind
        if(interface->cfg.is_a(Configuration::Kind::Server))
        {
            // interface is instantiated for a server
            const Configuration::Server &srv_info = std::get<0>(interface->cfg.details);
            logger_name = srv_info.device_name;
            resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{
                {"service.namespace", interface->cfg.name_space.empty() ? "tango" : interface->cfg.name_space},
                {"service.name", srv_info.class_name},         // naming convention of OpenTelemetry
                {"service.instance.id", srv_info.device_name}, // naming convention of OpenTelemetry
                {"tango.server.name",
                 util != nullptr ? util->get_ds_exec_name() + "/" + util->get_ds_inst_name() : "unknown"},
                {"tango.process.id", api_util->get_client_pid()},
                {"tango.process.kind", api_util->in_server() ? "server" : "client"},
                {"tango.host", tango_host}};
        }
        else
        {
            // interface is instantiated for a client
            const Configuration::Client &clt_info = std::get<1>(interface->cfg.details);
            logger_name = clt_info.name;
            resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{
                {"service.namespace", interface->cfg.name_space.empty() ? "tango" : interface->cfg.name_space},
                {"service.name", clt_info.name}, // naming convention of OpenTelemetry
                {"tango.process.id", api_util->get_client_pid()},
                {"tango.process.kind", api_util->in_server() ? "server" : "client"},
                {"tango.host", tango_host}};
        }

        auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);

        TANGO_ASSERT(processor);
        interface->logger_provider =
            opentelemetry::sdk::logs::LoggerProviderFactory::Create(std::move(processor), resource);

        // set the global logger provider
        opentelemetry::logs::Provider::SetLoggerProvider(interface->logger_provider);
    }

    //-------------------------------------------------------------------------------------
    // Appender::cleanup_logger_provider
    //-------------------------------------------------------------------------------------
    void cleanup_logger_provider()
    {
        auto old_provider = opentelemetry::logs::Provider::GetLoggerProvider();
        auto *base_class = dynamic_cast<opentelemetry::sdk::logs::LoggerProvider *>(old_provider.get());
        if(base_class != nullptr)
        {
            base_class->ForceFlush();
        }

        using LoggerProviderPtr = opentelemetry::nostd::shared_ptr<opentelemetry::logs::LoggerProvider>;
        interface->logger_provider = LoggerProviderPtr(new opentelemetry::logs::NoopLoggerProvider);
        opentelemetry::logs::Provider::SetLoggerProvider(interface->logger_provider);
    }

    //-------------------------------------------------------------------------------------
    // Appender::get_logger
    //-------------------------------------------------------------------------------------
    opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> get_logger()
    {
        auto provider = opentelemetry::logs::Provider::GetLoggerProvider();
        return provider->GetLogger(logger_name, "cppTango", git_revision());
    }

    //-------------------------------------------------------------------------------------
    // Appender::requires_layout
    //-------------------------------------------------------------------------------------
    bool requires_layout() const override
    {
        return false;
    }

    //-------------------------------------------------------------------------------------
    // Appender::set_layout
    //-------------------------------------------------------------------------------------
    void set_layout([[maybe_unused]] log4tango::Layout *layout = nullptr) override { }

    //-------------------------------------------------------------------------------------
    // Appender::close
    //-------------------------------------------------------------------------------------
    void close() override
    {
        // noop
    }

    //-------------------------------------------------------------------------------------
    // Appender::reopen
    //-------------------------------------------------------------------------------------
    bool reopen() override
    {
        return true;
    }

    //-------------------------------------------------------------------------------------
    // Appender::is_valid
    //-------------------------------------------------------------------------------------
    bool is_valid() const override
    {
        return true;
    }

    //-------------------------------------------------------------------------------------
    // Appender::_append
    //-------------------------------------------------------------------------------------
    int _append(const log4tango::LoggingEvent &event) override
    {
        auto to_opentelemetry_level = [](const log4tango::Level::Value &level)
        {
            switch(level)
            {
            case log4tango::Level::FATAL:
                return opentelemetry::logs::Severity::kFatal;
            case log4tango::Level::ERROR:
                return opentelemetry::logs::Severity::kError;
            case log4tango::Level::WARN:
                return opentelemetry::logs::Severity::kWarn;
            case log4tango::Level::INFO:
                return opentelemetry::logs::Severity::kInfo;
            case log4tango::Level::DEBUG:
                return opentelemetry::logs::Severity::kDebug;
            case log4tango::Level::OFF:
            default:
                return opentelemetry::logs::Severity::kInvalid;
            }
        };

        auto ctx = interface->get_current_opentelemetry_span()->GetContext();

        get_logger()->EmitLogRecord(to_opentelemetry_level(event.level),
                                    event.message,
                                    ctx.trace_id(),
                                    ctx.span_id(),
                                    ctx.trace_flags(),
                                    opentelemetry::common::SystemTimestamp(event.timestamp),
                                    opentelemetry::common::MakeAttributes(
                                        {{"code.filepath", event.file_path}, {"code.lineno", event.line_number}}));

        return 0;
    }
};

//-----------------------------------------------------------------------------------------
// INTERFACE
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
// Interface::Interface
//-----------------------------------------------------------------------------------------
Interface::Interface(const Configuration &config)
{
    impl = std::make_shared<InterfaceImplementation>(config);
}

//-----------------------------------------------------------------------------------------
// Interface::~Interface
//-----------------------------------------------------------------------------------------
Interface::~Interface()
{
    impl->terminate();
}

//-----------------------------------------------------------------------------------------
//! Interface::get_configuration
//-----------------------------------------------------------------------------------------
const Configuration &Interface::get_configuration() const noexcept
{
    return impl->cfg;
}

//-----------------------------------------------------------------------------------------
//! Interface::get_appender
//-----------------------------------------------------------------------------------------
log4tango::Appender *Interface::get_appender() const noexcept
{
    return new(std::nothrow) Appender(impl);
}

//-----------------------------------------------------------------------------------------
// Interface::terminate
//-----------------------------------------------------------------------------------------
bool Interface::is_enabled() const noexcept
{
    return impl->cfg.enabled;
}

//-----------------------------------------------------------------------------------------
// Interface::enable
//-----------------------------------------------------------------------------------------
void Interface::enable() noexcept
{
    impl->cfg.enabled = true;
}

//-----------------------------------------------------------------------------------------
// Interface::disable
//-----------------------------------------------------------------------------------------
void Interface::disable() noexcept
{
    impl->cfg.enabled = false;
}

//-----------------------------------------------------------------------------------------
// Interface::are_kernel_traces_enabled
//-----------------------------------------------------------------------------------------
bool Interface::are_kernel_traces_enabled() const noexcept
{
    return impl->cfg.kernel_traces_enabled;
}

//-----------------------------------------------------------------------------------------
// Interface::are_kernel_traces_disabled
//-----------------------------------------------------------------------------------------
bool Interface::are_kernel_traces_disabled() const noexcept
{
    return !impl->cfg.kernel_traces_enabled;
}

//-----------------------------------------------------------------------------------------
// Interface::enable_kernel_traces
//-----------------------------------------------------------------------------------------
void Interface::enable_kernel_traces() noexcept
{
    impl->cfg.kernel_traces_enabled = true;
}

//-----------------------------------------------------------------------------------------
// Interface::disable_kernel_traces
//-----------------------------------------------------------------------------------------
void Interface::disable_kernel_traces() noexcept
{
    impl->cfg.kernel_traces_enabled = false;
}

//-----------------------------------------------------------------------------------------
// Interface::get_id
//-----------------------------------------------------------------------------------------
const std::string &Interface::get_id() const noexcept
{
    return impl->cfg.id;
}

//-----------------------------------------------------------------------------------------
//  start_span
//-----------------------------------------------------------------------------------------
SpanPtr Interface::start_span(const std::string &name, const Attributes &attributes, const Span::Kind &kind) noexcept
{
    return impl->start_span(name, attributes, kind);
}

//-----------------------------------------------------------------------------------------
//  get_current_span
//-----------------------------------------------------------------------------------------
SpanPtr Interface::get_current_span() const noexcept
{
    return impl->get_current_span();
}

//-----------------------------------------------------------------------------------------
// set_current_context: part of the trace context propagation - mutualizes the associated code
//-----------------------------------------------------------------------------------------
static opentelemetry::context::Context set_current_context(const std::string &trace_parent,
                                                           const std::string &trace_state) noexcept
{
    // inject the incoming W3C headers into a carrier
    TangoTextMapCarrier carrier;
    carrier.Set(opentelemetry::trace::propagation::kTraceParent, trace_parent);
    carrier.Set(opentelemetry::trace::propagation::kTraceState, trace_state);

    // get the current context
    auto current_context = opentelemetry::context::RuntimeContext::GetCurrent();

    // get the current propagator
    auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();

    //--------------------------------------------------------------------------------------
    // breakdown of following call to prop->Extract:
    //----------------------------------------------
    // 1. the propagator to extract the new context from the carrier -> returns SpanContext from W3C headers
    // 2. the SpanContext is used to instantiate a DefaultSpan ->  returns a DefaultSpan (shared ptr) [1]
    // 3. the DefaultSpan is attached to the "current_context" (becomes the active one) -> returns a Context
    // 4. the Context is returned to the caller - will be used as the parent of the next downstream spans
    //--------------------------------------------------------------------------------------
    // [1] the class DefaultSpan provides a non-operational Span that propagates the tracer context by
    // wrapping it inside the Span object.
    //--------------------------------------------------------------------------------------
    return prop->Extract(carrier, current_context);
}

//-----------------------------------------------------------------------------------------
// Interface::get_current_context: return the current telemetry context
//-----------------------------------------------------------------------------------------
// this method is used by the Tango::Connection (and its child classes - e.g., DeviceProxy) to
// propagate the trace context to the callee. It could also be used by pyTango. The trace context
// is obtained in its W3C format trough the two strings passed as arguments.
//-----------------------------------------------------------------------------------------
static void get_current_context(std::string &trace_parent, std::string &trace_state) noexcept
{
    // ask the propagator to inject the current context in the specified carrier
    TangoTextMapCarrier carrier;
    auto context = opentelemetry::context::RuntimeContext::GetCurrent();
    auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
    prop->Inject(carrier, context);

    // extract the W3C headers from carrier
    trace_parent = std::string(carrier.Get(opentelemetry::trace::propagation::kTraceParent));
    trace_state = std::string(carrier.Get(opentelemetry::trace::propagation::kTraceState));
}

//-----------------------------------------------------------------------------------------
// Interface::set_trace_context
//-----------------------------------------------------------------------------------------
// set_trace_context: context propagation. This method is used by the Tango::Device_[X]Impl to
// setup the trace context upon receipt of a remote call. This flavor of "set_trace_context" is
// used where the caller is using an IDL version >= 4 and propagates context information.
//-----------------------------------------------------------------------------------------
ScopePtr Interface::set_trace_context(const std::string &new_span_name,
                                      const Tango::telemetry::Attributes &span_attr,
                                      const Tango::ClntIdent &client_identification) noexcept
{
    //  get telemetry interface attached to the current thread
    auto current_interface = Tango::telemetry::Interface::get_current();

    Tango::LockerLanguage cl_lang = client_identification._d();

    if(cl_lang == Tango::CPP || cl_lang == Tango::JAVA)
    {
        return std::make_unique<Scope>(current_interface->start_span(new_span_name));
    }

    const TraceContext *trace_context{nullptr};

    // telemetry: get trace context from ClntIdent
    switch(cl_lang)
    {
    case Tango::CPP_6:
    {
        trace_context = &client_identification.cpp_clnt_6().trace_context;
    }
    break;
    case Tango::JAVA_6:
    {
        trace_context = &client_identification.java_clnt_6().trace_context;
    }
    break;
    default:
        // unknown IDL version: simply ignore it till someone upgrade the code
        break;
    }

    if(trace_context == nullptr)
    {
        // weird case...
        // deal with that just as if we didn't receive any trace context info from the client
        return std::make_unique<Scope>(current_interface->start_span(new_span_name));
    }

    // this is what we wnat to extract - see W3C trace context standard for info
    std::string trace_parent;
    std::string trace_state;

    switch(trace_context->_d())
    {
    case Tango::W3C_TC_V0:
    default:
    {
        const Tango::W3CTraceContextV0 &tc_data = trace_context->data();
        trace_parent = tc_data.trace_parent;
        trace_state = tc_data.trace_state;
    }
    }

    // make the incoming context the current "local" one (see set_current_context from details)
    auto new_context = set_current_context(trace_parent, trace_state);

    // create the server counterpart of the incoming client trace (i.e., make the link between caller and callee)
    opentelemetry::trace::StartSpanOptions options;
    // the active DefaultSpan of the "new_context" becomes the parent of the downstream spans (see set_current_context)
    options.parent = opentelemetry::trace::GetSpan(new_context)->GetContext();
    // make sure we associated the server span to the client span one (server counterpart - that is critical)
    options.kind = opentelemetry::trace::SpanKind::kServer;

    // ok, let's create and return the "local root span" of the distributed transaction in progress
    return std::make_unique<Scope>(current_interface->impl->start_span(new_span_name, span_attr, options));
}

//-----------------------------------------------------------------------------------------
// Interface::set_trace_context
//-----------------------------------------------------------------------------------------
// set_trace_context: context propagation. This method is used by the Tango::Device_[X]Impl to
// setup the trace context upon receipt of a remote call. This flavor of "set_trace_context" is
// used where the caller is using an IDL version < 4 and do not propagate any context information.
//-----------------------------------------------------------------------------------------
ScopePtr Interface::set_trace_context(const std::string &new_span_name,
                                      const Tango::telemetry::Attributes &span_attr) noexcept
{
    //  get telemetry interface attached to the current thread
    auto current_interface = Tango::telemetry::Interface::get_current();

    // no trace context to propagate
    opentelemetry::trace::StartSpanOptions options;
    options.kind = opentelemetry::trace::SpanKind::kServer;
    return std::make_unique<Scope>(current_interface->impl->start_span(new_span_name, span_attr, options));
}

//-----------------------------------------------------------------------------------------
// Interface::set_trace_context
//-----------------------------------------------------------------------------------------
// this is a helper function for pyTango: the python binding use the python native implementation
// of OpenTelemetry. All we have to do is to provide it with a way to get and set the current
// telemetry context when it call the kernel back (e.g., making use of a DeviceProxy). This
// method allows to set the current telemetry context (i.e., to propagate the python context to
// c++) using its W3C format trough the two strings passed as arguments.
//-----------------------------------------------------------------------------------------
ScopePtr Interface::set_trace_context(const std::string &new_span_name,
                                      const std::string &trace_parent,
                                      const std::string &trace_state,
                                      const Span::Kind &kind) noexcept
{
    //  get telemetry interface attached to the current thread
    auto current_interface = Tango::telemetry::Interface::get_current();

    // make the incoming context the current "local" one (see set_current_context from details)
    auto new_context = set_current_context(trace_parent, trace_state);

    // create the server counterpart of the incoming client trace (i.e., make the link between caller and callee)
    opentelemetry::trace::StartSpanOptions options;
    // the active DefaultSpan of the "new_context" becomes the parent of the downstream spans (see set_current_context)
    options.parent = opentelemetry::trace::GetSpan(new_context)->GetContext();
    // make sure we associated the server span to the client span one (server counterpart)
    options.kind = to_opentelemetry_span_kind(kind);

    // ok, let's create and return the "local root span" of the distributed transaction in progress
    return std::make_unique<Scope>(current_interface->impl->start_span(new_span_name, {}, options));
}

//-----------------------------------------------------------------------------------------
// Interface::get_current_context: return the current telemetry context
//-----------------------------------------------------------------------------------------
// this method is used by the Tango::Connection (and its child classes - e.g., DeviceProxy) to
// propagate the trace context to the callee. It could also be used by pyTango. The trace context
// is obtained in its W3C format trough the two strings passed as arguments.
//-----------------------------------------------------------------------------------------
void Interface::get_trace_context(std::string &trace_parent, std::string &trace_state) noexcept
{
    get_current_context(trace_parent, trace_state);
}

//-----------------------------------------------------------------------------------------
// Interface::get_default_interface: returns the default Tango::telemetry::Interface
//-----------------------------------------------------------------------------------------
Tango::telemetry::InterfacePtr Interface::get_default_interface()
{
    const std::lock_guard<std::mutex> lock(InterfaceImplementation::default_telemetry_interface_mutex);

    if(!InterfaceImplementation::default_telemetry_interface)
    {
        InterfaceImplementation::default_telemetry_interface =
            std::make_shared<Tango::telemetry::Interface>(Configuration{});

        // mark this interface as the default one
        InterfaceImplementation::default_telemetry_interface->impl->is_default_interface = true;
    }

    return InterfaceImplementation::default_telemetry_interface;
}

//-----------------------------------------------------------------------------------------
// Interface::is_default:
// returns true if the telemetry interface the default one, returns false otherwise.
//-----------------------------------------------------------------------------------------
bool Interface::is_default() const noexcept
{
    return impl->is_default_interface;
}

//-----------------------------------------------------------------------------------------
// Interface::trace_exception:
// a helper function that tries to extract an err. msg. from the current exception
//-----------------------------------------------------------------------------------------
std::string Interface::extract_exception_info(std::exception_ptr current_exception)
{
    // be sure we have a validate input
    if(!current_exception)
    {
        return "unknown exception caught (no details available)";
    }

    // identify the exception by rethrowing it
    std::stringstream err;
    try
    {
        std::rethrow_exception(current_exception);
    }
    catch(const Tango::DevFailed &tango_ex)
    {
        if(tango_ex.errors.length() > 0)
        {
            err << "EXCEPTION:Tango::DevFailed;REASON:" << tango_ex.errors[0].reason
                << ";DESC:" << tango_ex.errors[0].desc << ";ORIGIN:" << tango_ex.errors[0].origin;
        }
        else
        {
            err << "EXCEPTION:Tango::DevFailed;REASON:unknown;DESC:unknown;ORIGIN:unknown";
        }
    }
    catch(const std::exception &std_ex)
    {
        err << "EXCEPTION:std::exception;DESC:unknown" << std_ex.what();
    }
    catch(...)
    {
        err << "EXCEPTION:unknown;DESC:unknown";
    }

    // return exception info (err msg)...
    return err.str();
}

//-----------------------------------------------------------------------------------------
// Interface::extract_exception_info:
// a helper function that tries to extract an err. msg. from the current exception
//-----------------------------------------------------------------------------------------
void Interface::extract_exception_info(std::string &type, std::string &message)
{
    std::exception_ptr current_exception = std::current_exception();

    // be sure we have a validate input
    if(!current_exception)
    {
        type = "unknown";
        message = "there is currently no exception";
        return;
    }

    // identify the exception by rethrowing it
    try
    {
        std::rethrow_exception(current_exception);
    }
    catch(const Tango::DevFailed &tango_ex)
    {
        type = "Tango::DevFailed";
        std::stringstream err;
        if(tango_ex.errors.length() > 0)
        {
            err << "REASON:" << tango_ex.errors[0].reason << ";DESC:" << tango_ex.errors[0].desc
                << ";ORIGIN:" << tango_ex.errors[0].origin;
        }
        else
        {
            err << "REASON:unknown;DESC:unknown;ORIGIN:unknown";
        }
        message = err.str();
    }
    catch(const std::exception &std_ex)
    {
        type = "std::exception";
        message = std_ex.what();
    }
    catch(...)
    {
        type = "unknown";
        message = "unknown exception caught";
    }
}

//-----------------------------------------------------------------------------------------
// INTERFACE-FACTORY
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
// InterfaceFactory: a Tango::telemetry::Interface factory
//-----------------------------------------------------------------------------------------
Tango::telemetry::InterfacePtr InterfaceFactory::create(const Configuration &cfg)
{
    auto srv = std::make_shared<Tango::telemetry::Interface>(cfg);
    return srv;
}

} // namespace Tango::telemetry
