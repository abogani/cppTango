#if defined(TELEMETRY_ENABLED)

  #include <grpcpp/grpcpp.h>
  #include <iostream>
  #include <memory>
  #include <iostream>
  #include <opentelemetry/sdk/version/version.h>
  #include <opentelemetry/context/propagation/global_propagator.h>
  #include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>
  #include <opentelemetry/exporters/ostream/span_exporter_factory.h>
  #include <opentelemetry/sdk/resource/resource.h>
  #include <opentelemetry/sdk/trace/processor.h>
  #include <opentelemetry/sdk/trace/simple_processor_factory.h>
  #include <opentelemetry/sdk/trace/tracer_provider.h>
  #include <opentelemetry/sdk/trace/tracer_provider_factory.h>
  #include <opentelemetry/trace/provider.h>
  #include <tango/tango.h>
  #include <tango/common/telemetry/telemetry.h>

namespace Tango::telemetry
{
//-------------------------------------------------------------------------------------------------
// The default endpoint to which traces are exported (default value)
//-------------------------------------------------------------------------------------------------
// TODO: default endpoint port
const std::string Interface::DEFAULT_COLLECTOR_ENDPOINT = "localhost:4317";

//-------------------------------------------------------------------------------------------------
// TangoCarrier::Set - trace context propagation - input
//-------------------------------------------------------------------------------------------------
void TangoCarrier::Set(opentelemetry::nostd::string_view key, opentelemetry::nostd::string_view value) noexcept
{
    // Set: implementation to set the key-value pair in the carrier
    // For example, we might store it in a map or another data structure.
    // Here, we are printing the values for demonstration purposes.

    std::cout << "TangoCarrier::Set::key: " << key.data() << ", value: " << value.data() << std::endl;
}

//-------------------------------------------------------------------------------------------------
// TangoCarrier::Get - trace context propagation - ouput
//-------------------------------------------------------------------------------------------------
opentelemetry::nostd::string_view TangoCarrier::Get(opentelemetry::nostd::string_view key) const noexcept
{
    // Get: implementation to retrieve the value for the given key from the
    // carrier For example, we might look up the value in a map or another data
    // structure. Here, we return a dummy string for demonstration purposes.

    std::cout << "TangoCarrier::Get::key: " << key.data() << std::endl;

    return opentelemetry::nostd::string_view("some-trace-context-data");
}

//-------------------------------------------------------------------------------------------------
// Tracer::Tracer
//-------------------------------------------------------------------------------------------------
Tracer::Tracer(const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> &t) :
    otel_tracer(t)
{
}

//-------------------------------------------------------------------------------------------------
// Tracer::start_span
//-------------------------------------------------------------------------------------------------
std::shared_ptr<Tango::telemetry::Span> Tracer::start_span(const std::string &name,
                                                           const Tango::telemetry::Attributes &attributes,
                                                           const Tango::telemetry::Span::Kind &kind) noexcept
{
    opentelemetry::trace::StartSpanOptions start_span_opts;

    switch(kind)
    {
    case Span::Kind::Server:
        start_span_opts.kind = opentelemetry::trace::SpanKind::kServer;
        break;
    case Span::Kind::Client:
        start_span_opts.kind = opentelemetry::trace::SpanKind::kClient;
        break;
    case Span::Kind::Producer:
        start_span_opts.kind = opentelemetry::trace::SpanKind::kProducer;
        break;
    case Span::Kind::Consumer:
        start_span_opts.kind = opentelemetry::trace::SpanKind::kConsumer;
        break;
    default:
        start_span_opts.kind = opentelemetry::trace::SpanKind::kInternal;
        break;
    }

    return std::make_shared<Tango::telemetry::Span>(otel_tracer->StartSpan(name, attributes, start_span_opts));
}

//-------------------------------------------------------------------------------------------------
// Interface::Interface
//-------------------------------------------------------------------------------------------------
Interface::Interface()
{
    // default/dummy configuration
    initialize(cfg);

    // mark as not configured
    configured = false;
}

//-------------------------------------------------------------------------------------------------
// Interface::initialize
//-------------------------------------------------------------------------------------------------
void Interface::initialize(const Interface::Configuration &srv_cfg) noexcept
{
    // copy configuration locally
    cfg = srv_cfg;

    // init the trace provider
    init_trace_provider();

    // init the propagator
    init_propagator();

    // mark as configured
    configured = true;
}

//-------------------------------------------------------------------------------------------------
// Interface::terminate
//-------------------------------------------------------------------------------------------------
void Interface::terminate() noexcept
{
    cleanup_trace_provider();
    cleanup_propagator();
}

//-------------------------------------------------------------------------------------------------
// Interface::init_default_trace_provider
//-------------------------------------------------------------------------------------------------
void Interface::init_default_trace_provider() noexcept
{
    std::ostream &output_stream = std::cout;

    auto exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create(output_stream);

    auto processor = opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(std::move(exporter));

    opentelemetry::sdk::resource::ResourceAttributes resource_attributes{{"service.name", "tango.unknown"}};

    auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);

    provider = opentelemetry::sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);

    auto otel_tracer = provider->GetTracer("default_tracer", OPENTELEMETRY_SDK_VERSION);

    tracer = std::make_shared<Tango::telemetry::Tracer>(otel_tracer);
}

//-------------------------------------------------------------------------------------------------
// Interface::init_trace_provider
//-------------------------------------------------------------------------------------------------
void Interface::init_trace_provider() noexcept
{
    // init the trace provider
    // --------------------------------------------------------
    opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
    opts.endpoint = cfg.collector_endpoint;
    opts.use_ssl_credentials = false;
    auto exporter = opentelemetry::exporter::otlp::OtlpGrpcExporterFactory::Create(opts);

    auto processor = opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(std::move(exporter));

    auto util = Tango::Util::instance();
    auto tg_api_util = Tango::ApiUtil::instance();

    std::string tango_host;
    tg_api_util->get_env_var("TANGO_HOST", tango_host);

    std::string default_tracer;

    opentelemetry::sdk::resource::ResourceAttributes resource_attributes;

    std::string server_name(util->get_ds_name());

    // check interface configuration kind
    if(cfg.is_a(Interface::Kind::Server))
    {
        // interface is instantiated for a server
        const Server &srv_info = std::get<0>(cfg.details);
        default_tracer = srv_info.device_name;
        resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{
            {"server.namespace", cfg.name_space.empty() ? "tango" : cfg.name_space},
            {"service.name", srv_info.class_name},
            {"service.instance.id", srv_info.device_name},
            {"tango.server.name", util->get_ds_exec_name()},
            {"tango.server.instance.id", util->get_ds_inst_name()},
            {"tango.server.idl.version", util->get_version_str()},
            {"tango.server.lib.version", util->get_tango_lib_release()},
            {"process.id", util->get_pid()},
            {"tango.process.kind", tg_api_util->in_server() ? "server" : "client"}};
    }
    else
    {
        // interface is instantiated for a client
        const Client &clt_info = std::get<1>(cfg.details);
        default_tracer = clt_info.name;
        resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{
            {"client.namespace", cfg.name_space.empty() ? "tango" : cfg.name_space},
            {"service.name", clt_info.name},
            {"service.instance.id", clt_info.name},
            {"process.id", tg_api_util->get_client_pid()},
            {"tango.process.kind", tg_api_util->in_server() ? "server" : "client"}};
    }

    auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);

    provider = opentelemetry::sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);

    auto otel_tracer = provider->GetTracer(default_tracer, OPENTELEMETRY_SDK_VERSION);

    tracer = std::make_shared<Tango::telemetry::Tracer>(otel_tracer);
}

//-------------------------------------------------------------------------------------------------
// Interface::cleanup_trace_provider
//-------------------------------------------------------------------------------------------------
void Interface::cleanup_trace_provider() noexcept
{
    if(provider)
    {
        static_cast<opentelemetry::sdk::trace::TracerProvider *>(provider.get())->ForceFlush();
    }
}

//-------------------------------------------------------------------------------------------------
// Interface::init_propagator
//-------------------------------------------------------------------------------------------------
void Interface::init_propagator() noexcept
{
    // make the link between Tango (CORBA) and OpenTelemetry
    auto crtc = opentelemetry::context::RuntimeContext::GetCurrent();
    auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
    prop->Inject(Interface::carrier, crtc);
}

//-------------------------------------------------------------------------------------------------
// Interface::cleanup_propagator
//-------------------------------------------------------------------------------------------------
void Interface::cleanup_propagator() noexcept
{
    //- noop so far
}

//-------------------------------------------------------------------------------------------------
// rpc_exception_hook: interceptor hook - allows to catch and trace RPCs throwing an exception
//-------------------------------------------------------------------------------------------------
/*
CORBA::Boolean Interface::rpc_exception_hook(omni::omniInterceptors::serverSendException_T::info_T &)
{
    // catch trace context (see Tango::utils::tss)
    auto telemetry = Tango::utils::tss::current_telemetry_interface;

    std::cout << "in Interface::rpc_exception_hook: current_telemetry_interface is: " << telemetry->get_id()
              << std::endl;

    // the callee (i.e., the device) attached its telemetry interface to the thread executing this code.
    // we consequently have a change to be sure that any error will be reported to the telemetry service.
    // the last span created is certainly "ended" so we need to create one for our specific need and
    // force the error status on it.
    if(telemetry->is_configured())
    {
        auto s = Tango::telemetry::Scope(
            telemetry->get_tracer()->start_span("Tango::telemetry::Interface::rpc_exception_hook"));

        s->set_status(Tango::telemetry::Span::Status::Error,
                      "client request is resulting in an exception being thrown [see upstream part of the call
stack]");
    }

    return true;
}
*/

//-------------------------------------------------------------------------------------------------
// InterfaceFactory
//-------------------------------------------------------------------------------------------------
std::shared_ptr<Tango::telemetry::Interface>
    InterfaceFactory::create(const Tango::telemetry::Interface::Configuration &cfg)
{
    auto srv = std::make_shared<Tango::telemetry::Interface>();
    srv->initialize(cfg);
    return srv;
}

} // namespace Tango::telemetry

#endif // TELEMETRY_ENABLED
