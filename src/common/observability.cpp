#if defined(OBSERVABILITY_ENABLED)

  #include <memory>
  #include <iostream>
  #include <grpcpp/grpcpp.h>
  #include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>
  #include <opentelemetry/sdk/resource/resource.h>
  #include <opentelemetry/sdk/trace/processor.h>
  #include <opentelemetry/sdk/trace/simple_processor_factory.h>
  #include <opentelemetry/sdk/trace/tracer_provider_factory.h>
  #include <opentelemetry/sdk/trace/tracer_provider.h>
  #include <opentelemetry/trace/provider.h>
  #include <opentelemetry/context/propagation/global_propagator.h>
  #include <tango/common/observability/observability.h>
  #include "opentelemetry/sdk/version/version.h"

namespace Tango::observability
{

//-------------------------------------------------------------------------------------------------
// The default endpoint to which traces are exported (default value)
//-------------------------------------------------------------------------------------------------
// TODO: default endpoint port
const std::string Service::DEFAULT_COLLECTOR_ENDPOINT = "localhost:4317";

//-------------------------------------------------------------------------------------------------
// The default endpoint of the observability service (overwrites the DEFAULT_COLLECTOR_ENDPOINT)
//-------------------------------------------------------------------------------------------------
std::string Service::default_collector_endpoint;

//-------------------------------------------------------------------------------------------------
// The bridge between Tango (CORBA) and OpenTelemetry
//-------------------------------------------------------------------------------------------------
TangoCarrier Service::carrier;

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
    // Get: implementation to retrieve the value for the given key from the carrier
    // For example, we might look up the value in a map or another data structure.
    // Here, we return a dummy string for demonstration purposes.

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
std::shared_ptr<Tango::observability::Span>
    Tracer::start_span(const std::string &name, const Span::Attributes &attributes, const Span::Kind &kind) noexcept
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

    return std::shared_ptr<Tango::observability::Span>(
        new Tango::observability::Span(otel_tracer->StartSpan(name, attributes, start_span_opts)));
}

//-------------------------------------------------------------------------------------------------
// Service::initialize
//-------------------------------------------------------------------------------------------------
void Service::initialize(const std::string &dserver_name, const std::string &endpoint)
{
    // overwrite (or not) the default endpoint
    Service::default_collector_endpoint = endpoint;

    // init the trace provider
    // --------------------------------------------------------
    init_trace_provider(dserver_name, endpoint);

    // init the propagator
    // --------------------------------------------------------
    init_propagator();
}

//-------------------------------------------------------------------------------------------------
// Service::terminate
//-------------------------------------------------------------------------------------------------
void Service::terminate()
{
    cleanup_trace_provider();
    cleanup_propagator();
}

//-------------------------------------------------------------------------------------------------
// Service::init_trace_provider
//-------------------------------------------------------------------------------------------------
void Service::init_trace_provider(const std::string &dserver_name, const std::string &endpoint)
{
    // init the trace provider
    // --------------------------------------------------------

    // step 1 - create the exporter
    opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
    opts.endpoint = endpoint;
    opts.use_ssl_credentials = false;
    auto exporter = opentelemetry::exporter::otlp::OtlpGrpcExporterFactory::Create(opts);

    // step 2 - create the processor
    auto processor = opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(std::move(exporter));

    // step 3 - create the provider
    // TODO: add more "service" information - e.g., {"service.namespace", "aControlSystemDomain"}
    auto resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{{"service.name", dserver_name}};
    auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);

    std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
        opentelemetry::sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);

    // step 4 - set the global trace providermy
    opentelemetry::trace::Provider::SetTracerProvider(provider);
}

//-------------------------------------------------------------------------------------------------
// Service::cleanup_trace_provider
//-------------------------------------------------------------------------------------------------
void Service::cleanup_trace_provider()
{
    // we call ForceFlush to prevent to cancel running exportings, it's optional.
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider> provider =
        opentelemetry::trace::Provider::GetTracerProvider();

    if(provider)
    {
        static_cast<opentelemetry::sdk::trace::TracerProvider *>(provider.get())->ForceFlush();
    }
}

//-------------------------------------------------------------------------------------------------
// Service::init_propagator
//-------------------------------------------------------------------------------------------------
void Service::init_propagator()
{
    // make the link between Tango (CORBA) and OpenTelemetry
    auto crtc = opentelemetry::context::RuntimeContext::GetCurrent();
    auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
    prop->Inject(Service::carrier, crtc);
}

//-------------------------------------------------------------------------------------------------
// Service::cleanup_propagator
//-------------------------------------------------------------------------------------------------
void Service::cleanup_propagator()
{
    //- noop so far
}

} // namespace Tango::observability

#endif // OBSERVABILITY_ENABLED