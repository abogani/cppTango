#ifndef TANGO_COMMON_OBSERVABILITY_H
#define TANGO_COMMON_OBSERVABILITY_H

#if defined(OBSERVABILITY_ENABLED)

  #include <map>
  #include <type_traits>
  #include <string>
  #include <opentelemetry/context/propagation/text_map_propagator.h>
  #include <opentelemetry/sdk/version/version.h>
  #include "opentelemetry/context/context.h"
  #include "opentelemetry/trace/provider.h"

namespace Tango::observability
{

//-------------------------------------------------------------------------------------------------
// Span
//-------------------------------------------------------------------------------------------------
// A Span represents a single operation within a Trace.
//-------------------------------------------------------------------------------------------------
class Span final
{
  public:
    //-------------------------------------------------------------------------------------------------
    //  Kind: the different flavors of Span (see Tracer::start_span for details).
    //-------------------------------------------------------------------------------------------------
    enum class Kind
    {
        Internal,
        Server,
        Client,
        Producer,
        Consumer
    };

    //-------------------------------------------------------------------------------------------------
    // SpanAttributes
    //-------------------------------------------------------------------------------------------------
    // Optional Span attributes (see Tracer::start_span for details).
    //-------------------------------------------------------------------------------------------------
    using Attributes = std::map<const std::string, const std::string>;

    //---------------------------------------------------------------------------------------------
    // Note that Spans should be created using the Tracer class. Please refer to the Tracer class.
    //---------------------------------------------------------------------------------------------
    Span() = default;

    //---------------------------------------------------------------------------------------------
    // The Span destructor end()s the Span, if it hasn't been ended already.
    //---------------------------------------------------------------------------------------------
    virtual ~Span() = default;

    //---------------------------------------------------------------------------------------------
    // A Span is not copiable nor movable.
    //---------------------------------------------------------------------------------------------
    Span(const Span &) = delete;
    Span(Span &&) = delete;
    Span &operator=(const Span &) = delete;
    Span &operator=(Span &&) = delete;

    //---------------------------------------------------------------------------------------------
    //! Add an event to the span.
    //!
    //! Parameters:
    //!   msg – the event message.
    //---------------------------------------------------------------------------------------------
    inline void add_event(const std::string &msg) noexcept
    {
        otel_span->AddEvent(msg);
    }

    //---------------------------------------------------------------------------------------------
    //! Sets a span attribute by name.
    //!
    //! Parameters:
    //!   key – the key name.
    //!   value – the value. See opentelemetry::common::AttributeValue for supported data types.
    //---------------------------------------------------------------------------------------------
    template <typename T>
    void set_attribute(const std::string &key, const T &value) noexcept
    {
        otel_span->SetAttribute(key, value);
    }

    //---------------------------------------------------------------------------------------------
    //! Mark the end of the Span.
    //!
    //! Only the timing (timestamp) of the first end call for a given Span will be recorded.
    //---------------------------------------------------------------------------------------------
    inline void end() noexcept
    {
        otel_span->End();
    }

  private:
    friend class Tracer;
    friend class Scope;

    // the underlying opentelemetry Span
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> otel_span;

    // private ctor
    Span(const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &s) :
        otel_span(s)
    {
    }
};

//-------------------------------------------------------------------------------------------------
// Scope
//-------------------------------------------------------------------------------------------------
// Controls how long a span is active.
// On creation of the Scope object, the given span is set to the currently active span.
// On destruction, the given span is ended and the previously active span will be the currently
// active span again.
//-------------------------------------------------------------------------------------------------
class Scope final
{
  public:
    //---------------------------------------------------------------------------------------------
    //! Initialize a new scope.
    //!
    //! Parameters:
    //!   span – the given span will be set as the currently active span.
    //---------------------------------------------------------------------------------------------
    Scope(const std::shared_ptr<Tango::observability::Span> &s) noexcept :
        token(opentelemetry::context::RuntimeContext::Attach(
            opentelemetry::context::RuntimeContext::GetCurrent().SetValue(opentelemetry::trace::kSpanKey,
                                                                          s->otel_span))),
        scoped_span(s)
    {
    }

    //---------------------------------------------------------------------------------------------
    //! Provide access to the underlying Span.
    //
    //! Returns:
    //!   the underlying Span.
    //---------------------------------------------------------------------------------------------
    inline Tango::observability::Span &span() noexcept
    {
        return *scoped_span;
    }

    //---------------------------------------------------------------------------------------------
    //! Provide access to the underlying Span.
    //
    //! Returns:
    //!   the underlying Span (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::observability::Span> &operator->() noexcept
    {
        return scoped_span;
    }

  private:
    opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> token;
    std::shared_ptr<Tango::observability::Span> scoped_span;
};

//-------------------------------------------------------------------------------------------------
// Tracer
//-------------------------------------------------------------------------------------------------
// Handles span creation and in-process context propagation. This class provides methods for
// manipulating the context, creating spans, and controlling spans’ lifecycles.
//-------------------------------------------------------------------------------------------------
class Tracer final
{
  public:
    //---------------------------------------------------------------------------------------------
    //! Start a span.
    //!
    //! Optionally specifies the kind of span to be created.
    //!
    //! Tango::DeviceProxy automatically set the span kind to Span::Kind::Client before calling
    //! Symmetrically, a Tango device server automatically set the span kind to Span::Kind::Server
    //! when receiving a request from a remote client. Collocated calls use Span::Kind::Internal.
    //!
    //! Parameters:
    //!   name – the span name (will follow a naming convention - to be defined).
    //!   kind – the span kind
    //!
    //! Returns:
    //!   the newly created Span (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::observability::Span>
        start_span(const std::string &name, const Span::Kind &kind = Span::Kind::Internal) noexcept
    {
        return start_span(name, {}, kind);
    }

    //---------------------------------------------------------------------------------------------
    //! Start a span.
    //!
    //! Optionally attaches some Span attributes and  the kind of span to be created.
    //!
    //! Tango::DeviceProxy automatically set the span kind to Span::Kind::Client before calling
    //! Symmetrically, a Tango device server automatically set the span kind to Span::Kind::Server
    //! when receiving a request from a remote client. Collocated calls use Span::Kind::Internal.
    //!
    //! Parameters:
    //!   name – the span name (will follow a naming convention - to be defined).
    //!   attributes - the optional Span attributes.
    //!   kind – the span kind
    //!
    //! Returns:
    //!   the newly created Span (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    std::shared_ptr<Tango::observability::Span> start_span(const std::string &name,
                                                           const Span::Attributes &attributes = {},
                                                           const Span::Kind &kind = Span::Kind::Internal) noexcept;

    //---------------------------------------------------------------------------------------------
    //! Get the currently active span.
    //!
    //! Returns:
    //!   the currently active span (as a std::shared_ptr), or an invalid default span if no span
    //!   is active.
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::observability::Span> get_current_span() noexcept
    {
        return std::shared_ptr<Tango::observability::Span>(
            new Tango::observability::Span(otel_tracer->GetCurrentSpan()));
    }

    //---------------------------------------------------------------------------------------------
    //! Set the active span.
    //!
    //! The span will remain active until the returned Scope object is destroyed.
    //!
    //! Parameters:
    //!   span – the span that should be set as the new active span.
    //!
    //! Returns:
    //!   a Scope that controls how long the span will be active.
    //---------------------------------------------------------------------------------------------
    static inline Tango::observability::Scope
        with_active_span(std::shared_ptr<Tango::observability::Span> &span) noexcept
    {
        return Tango::observability::Scope{span};
    }

  private:
    friend class Service;

    // the underlying opentelemetry tracer
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> otel_tracer;

    // private ctor
    explicit Tracer(const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> &t);
};

//-------------------------------------------------------------------------------------------------
// TangoCarrier
//-------------------------------------------------------------------------------------------------
// OpenTelemetry does not specify how the context is propagated. It simply provides a mechanism for
// injecting and extracting the context. This mechanism relies on a Propagator that itself delegates
// the actual I/O actions to a Carrier implementing a 'Set' (injection) and a 'Get' (extraction)
// method.
// We consequently have to provide a 'TangoCarrier' so that we will be able to inject/extract the
// the trace context from the data struct that carries it. So far, the context information is
// encapsulated into the ClntIdent data struct (of the CORBA IDL) passed by a client (the caller)
// to a server (the callee).
//-------------------------------------------------------------------------------------------------
class TangoCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
  public:
    TangoCarrier() = default;

    //---------------------------------------------------------------------------------------------
    // TangoCarrier::Set - trace context propagation. Called when we  are receiving a CORBA call
    //---------------------------------------------------------------------------------------------
    virtual void Set(opentelemetry::nostd::string_view key, opentelemetry::nostd::string_view value) noexcept override;

    //---------------------------------------------------------------------------------------------
    // TangoCarrier::Get - trace context propagation. Called when we are about to make a CORBA call
    //---------------------------------------------------------------------------------------------
    virtual opentelemetry::nostd::string_view Get(opentelemetry::nostd::string_view key) const noexcept override;
};

//-------------------------------------------------------------------------------------------------
// Service: the Tango observability service
//-------------------------------------------------------------------------------------------------
class Service
{
  public:
    //---------------------------------------------------------------------------------------------
    // Singleton like implementation
    //---------------------------------------------------------------------------------------------
    Service() = delete;
    Service(const Service &) = delete;
    Service(Service &&) = delete;
    Service &operator=(const Service &) = delete;
    Service &operator=(Service &&) = delete;

    //---------------------------------------------------------------------------------------------
    //! Service initialization.
    //---------------------------------------------------------------------------------------------
    static void initialize(const std::string &dserver_name,
                           const std::string &default_collector_endpoint = Service::DEFAULT_COLLECTOR_ENDPOINT);

    //---------------------------------------------------------------------------------------------
    //! Service termination/cleanup.
    //---------------------------------------------------------------------------------------------
    static void terminate();

    //---------------------------------------------------------------------------------------------
    //! Get the default endpoint of OpenTelemetry collector to which traces are exported.
    //!
    //! Returns:
    //!   the default OpenTelemetry collector (endpoint url).
    //---------------------------------------------------------------------------------------------
    static inline const std::string &get_default_collector_endpoint() noexcept
    {
        return default_collector_endpoint;
    }

    //---------------------------------------------------------------------------------------------
    //! Gets or creates a named Tracer instance.
    //!
    //! Parameters:
    //!   name – name Tracer instrumentation scope.
    //!
    //! Returns:
    //!   the requested Tracer (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    static inline std::shared_ptr<Tango::observability::Tracer> get_tracer(const std::string &name) noexcept
    {
        auto provider = opentelemetry::trace::Provider::GetTracerProvider();
        auto tracer = provider->GetTracer(name, OPENTELEMETRY_SDK_VERSION);
        return std::shared_ptr<Tango::observability::Tracer>(new Tango::observability::Tracer(tracer));
    }

  private:
    // the default endpoint to which traces are exported
    //---------------------------------------------------------------------------------------------
    static const std::string DEFAULT_COLLECTOR_ENDPOINT;

    // tracer initialization and cleanup
    static void init_trace_provider(const std::string &dserver_name, const std::string &endpoint);
    static void cleanup_trace_provider();

    // propagator initialization and cleanup
    static void init_propagator();
    static void cleanup_propagator();

    // default endpoint to which traces are exported
    static std::string default_collector_endpoint;

    // the bridge between Tango (CORBA) and OpenTelemetry
    static TangoCarrier carrier;
};

} // namespace Tango::observability

#endif // OBSERVABILITY_ENABLED

#endif