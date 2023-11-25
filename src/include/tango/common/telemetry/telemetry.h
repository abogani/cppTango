#ifndef TANGO_COMMON_TELEMETRY_H
#define TANGO_COMMON_TELEMETRY_H

#if defined(TELEMETRY_ENABLED)

  #include <grpcpp/grpcpp.h>
  #include <map>
  #include <string>
  #include <variant>
  #include <omniORB4/omniInterceptors.h>
  #include <opentelemetry/context/context.h>
  #include <opentelemetry/context/propagation/text_map_propagator.h>
  #include <opentelemetry/sdk/version/version.h>
  #include <opentelemetry/trace/provider.h>
  #include <tango/server/tango_current_function.h>

namespace Tango::telemetry
{
//-------------------------------------------------------------------------------------------------
// Attributes: as key:value dictionary (string only)
//-------------------------------------------------------------------------------------------------
// Optional Interface or Span attributes (see Interface::Configuration or
// Tracer::start_span).
//-------------------------------------------------------------------------------------------------
using Attributes = std::map<const std::string, const std::string>;

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
    // Status: the possible values of the Span status.
    //-------------------------------------------------------------------------------------------------
    enum class Status
    {
        Unset, // Default status
        Ok,    // Operation has completed successfully.
        Error  // The operation contains an error.
    };

    //---------------------------------------------------------------------------------------------
    // Note that Spans should be created using the Tracer class.
    // Please refer to the Tracer class.
    //---------------------------------------------------------------------------------------------
    Span() = default;

    //---------------------------------------------------------------------------------------------
    // Note that Spans should be created using the Tracer class.
    // Please refer to the Tracer class.
    //---------------------------------------------------------------------------------------------
    Span(const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &s) :
        otel_span(s)
    {
    }

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
    //!   value – the value. See opentelemetry::common::AttributeValue for
    //!   supported data types.
    //---------------------------------------------------------------------------------------------
    template <typename T>
    void set_attribute(const std::string &key, const T &value) noexcept
    {
        otel_span->SetAttribute(key, value);
    }

    //---------------------------------------------------------------------------------------------
    //! Sets some span attributes of the form <string_key:string_value>
    //!
    //! Parameters:
    //!   attributes – the map of attributes to be attached to the Span.
    //---------------------------------------------------------------------------------------------
    inline void set_attributes(const Tango::telemetry::Attributes &attributes) noexcept
    {
        for(const auto &pair : attributes)
        {
            otel_span->SetAttribute(pair.first, pair.second);
        }
    }

    //---------------------------------------------------------------------------------------------
    //! Sets the Span status.
    //!
    //! Parameters:
    //!   status – the Span status.
    //!   description – the description of the status.
    //---------------------------------------------------------------------------------------------
    inline void set_status(const Status &status, const std::string &description = "no error") noexcept
    {
        opentelemetry::trace::StatusCode otel_status = opentelemetry::trace::StatusCode::kUnset;

        switch(status)
        {
        case Span::Status::Ok:
            otel_status = opentelemetry::trace::StatusCode::kOk;
            break;
        case Span::Status::Error:
            otel_status = opentelemetry::trace::StatusCode::kError;
            break;
        default:
            break;
        }

        otel_span->SetStatus(otel_status, description);
    }

    //---------------------------------------------------------------------------------------------
    //! Mark the end of the Span.
    //!
    //! Only the timing (timestamp) of the first end call for a given Span will be
    //! recorded.
    //---------------------------------------------------------------------------------------------
    inline void end() noexcept
    {
        otel_span->End();
    }

  private:
    friend class Scope;
    // the underlying opentelemetry Span
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> otel_span;
};

//-------------------------------------------------------------------------------------------------
// Scope
//-------------------------------------------------------------------------------------------------
// Controls how long a span is active.
// On creation of the Scope object, the given span is set to the currently
// active span. On destruction, the given span is ended and the previously
// active span will be the currently active span again.
//-------------------------------------------------------------------------------------------------
class Scope final
{
  public:
    //---------------------------------------------------------------------------------------------
    //! Create a scoped span to control its life time using the RAII paradigm.
    //!
    //! Parameters:
    //!   span – the given span will be set as the currently active span.
    //---------------------------------------------------------------------------------------------
    Scope(const std::shared_ptr<Tango::telemetry::Span> &s) noexcept :
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
    inline Tango::telemetry::Span &span() noexcept
    {
        return *scoped_span;
    }

    //---------------------------------------------------------------------------------------------
    //! Provide access to the underlying Span.
    //
    //! Returns:
    //!   the underlying Span (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::telemetry::Span> &operator->() noexcept
    {
        return scoped_span;
    }

  private:
    opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> token;
    std::shared_ptr<Tango::telemetry::Span> scoped_span;
};

//-------------------------------------------------------------------------------------------------
// Tracer
//-------------------------------------------------------------------------------------------------
// Handles span creation and in-process context propagation. This class provides
// methods for manipulating the context, creating spans, and controlling spans’
// lifecycles.
//-------------------------------------------------------------------------------------------------
class Tracer final
{
  public:
    //---------------------------------------------------------------------------------------------
    // ctor
    //---------------------------------------------------------------------------------------------
    Tracer(const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> &t);

    //---------------------------------------------------------------------------------------------
    //! Start an "Kind::Internal" span.
    //!
    //! Parameters:
    //!   name – the span name (will follow a naming convention - to be defined).
    //!
    //! Returns:
    //!   the newly created Span (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::telemetry::Span> start_span(const std::string &name) noexcept
    {
        return start_span(name, {}, Span::Kind::Internal);
    }

    //---------------------------------------------------------------------------------------------
    //! Start a span.
    //!
    //! Optionally specifies the kind of span to be created.
    //!
    //! Tango::DeviceProxy automatically set the span kind to Span::Kind::Client
    //! before calling Symmetrically, a Tango device server automatically set the
    //! span kind to Span::Kind::Server when receiving a request from a remote
    //! client. Collocated calls use Span::Kind::Internal.
    //!
    //! Parameters:
    //!   name – the span name (will follow a naming convention - to be defined).
    //!   kind – the span kind
    //!
    //! Returns:
    //!   the newly created Span (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::telemetry::Span> start_span(const std::string &name, const Span::Kind &kind) noexcept
    {
        return start_span(name, {}, kind);
    }

    //---------------------------------------------------------------------------------------------
    //! Start a span.
    //!
    //! Optionally attaches some Span attributes and  the kind of span to be
    //! created.
    //!
    //! Tango::DeviceProxy automatically set the span kind to Span::Kind::Client
    //! before calling Symmetrically, a Tango device server automatically set the
    //! span kind to Span::Kind::Server when receiving a request from a remote
    //! client. Collocated calls use Span::Kind::Internal.
    //!
    //! Parameters:
    //!   name – the span name (will follow a naming convention - to be defined).
    //!   attributes - the optional Span attributes.
    //!   kind – the span kind
    //!
    //! Returns:
    //!   the newly created Span (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    std::shared_ptr<Tango::telemetry::Span> start_span(const std::string &name,
                                                       const Tango::telemetry::Attributes &attributes,
                                                       const Tango::telemetry::Span::Kind &kind) noexcept;

    //---------------------------------------------------------------------------------------------
    //! Get the currently active span.
    //!
    //! Returns:
    //!   the currently active span (as a std::shared_ptr), or an invalid default
    //!   span if no span is active.
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::telemetry::Span> get_current_span() noexcept
    {
        return std::make_shared<Tango::telemetry::Span>(otel_tracer->GetCurrentSpan());
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
    static inline Tango::telemetry::Scope with_active_span(std::shared_ptr<Tango::telemetry::Span> &span) noexcept
    {
        return Tango::telemetry::Scope{span};
    }

  private:
    // the underlying opentelemetry trace
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> otel_tracer;
};

//-------------------------------------------------------------------------------------------------
// TangoCarrier
//-------------------------------------------------------------------------------------------------
// OpenTelemetry does not specify how the context is propagated. It simply
// provides a mechanism for injecting and extracting the context. This mechanism
// relies on a Propagator that itself delegates the actual I/O actions to a
// Carrier implementing a 'Set' (injection) and a 'Get' (extraction) method. We
// consequently have to provide a 'TangoCarrier' so that we will be able to
// inject/extract the the trace context from the data struct that carries it. So
// far, the context information is encapsulated into the ClntIdent data struct
// (of the CORBA IDL) passed by a client (the caller) to a server (the callee).
//-------------------------------------------------------------------------------------------------
class TangoCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
  public:
    TangoCarrier() = default;

    //---------------------------------------------------------------------------------------------
    // TangoCarrier::Set - trace context propagation. Called when we  are
    // receiving a CORBA call
    //---------------------------------------------------------------------------------------------
    virtual void Set(opentelemetry::nostd::string_view key, opentelemetry::nostd::string_view value) noexcept override;

    //---------------------------------------------------------------------------------------------
    // TangoCarrier::Get - trace context propagation. Called when we are about to
    // make a CORBA call
    //---------------------------------------------------------------------------------------------
    virtual opentelemetry::nostd::string_view Get(opentelemetry::nostd::string_view key) const noexcept override;
};

//-------------------------------------------------------------------------------------------------
// Interface: an interface of the Tango telemetry
//-------------------------------------------------------------------------------------------------
class Interface
{
    //-------------------------------------------------------------------------------------------------
    // internal helper function: file_basename (TO BE MOVED ELSEWHERE)
    //-------------------------------------------------------------------------------------------------
    static inline const char *file_basename(const char *path) noexcept
    {
  #ifdef __WINDOWS__
        static constexpr auto cm_path_separator = '\\';
  #else
        static constexpr auto cm_path_separator = '/';
  #endif
        auto last_dir_sep = std::strrchr(path, cm_path_separator);
        if(last_dir_sep == nullptr)
        {
            return path;
        }
        return last_dir_sep + 1;
    }

    //-------------------------------------------------------------------------------------------------
    // internal helper function: thread_id_to_string (TO BE MOVED ELSEWHERE)
    //-------------------------------------------------------------------------------------------------
    static inline const std::string thread_id_to_string() noexcept
    {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    }

  public:
    // the default endpoint to which traces are exported
    //---------------------------------------------------------------------------------------------
    static const std::string DEFAULT_COLLECTOR_ENDPOINT;

    //---------------------------------------------------------------------------------------------
    enum class Kind
    {
        //- the telemetry interface for pure Tango client
        Client,
        //- the telemetry interface for Tango server (i.e., device)
        Server
    };

    //---------------------------------------------------------------------------------------------
    struct Client
    {
        Client() = default;
        Client(const Client &) = default;
        Client &operator=(const Client &) = default;
        Client &operator=(Client &&) = default;

        //- the pure Tango client name
        std::string name{"undefined Tango client name"};
    };

    //---------------------------------------------------------------------------------------------
    struct Server
    {
        Server() = default;
        Server(const Server &) = default;
        Server &operator=(const Server &) = default;
        Server &operator=(Server &&) = default;

        //- the Tango class name
        std::string class_name{"undefined Tango class name"};
        //- the Tango device name
        std::string device_name{"undefined Tango device name"};
    };

    // the interface Configuration
    //---------------------------------------------------------------------------------------------
    struct Configuration
    {
        Configuration() = default;
        Configuration(const Configuration &) = default;
        Configuration &operator=(const Configuration &) = default;
        Configuration &operator=(Configuration &&) = default;

        //- an optional telemetry interface name (user defined)
        std::string id{"unspecified interface name"};
        //- the optional name space to which the Interface owner belongs (user defined)
        std::string name_space{"tango"};
        //- the client or server info
        std::variant<Server, Client> details{Server()};
        //- the telemetry data collector endpoint - a string of the form: "addr:port"
        std::string collector_endpoint{Interface::DEFAULT_COLLECTOR_ENDPOINT};

        // get configuration kind
        inline Interface::Kind get_kind() const noexcept
        {
            if(std::get_if<Interface::Server>(&details))
            {
                return Interface::Kind::Server;
            }
            return Interface::Kind::Client;
        }

        // check configuration kind
        inline bool is_a(const Interface::Kind &kind) const noexcept
        {
            return get_kind() == kind;
        }
    };

    //- constructor
    Interface();

    //- disabled features
    Interface(const Interface &) = delete;
    Interface(Interface &&) = delete;
    Interface &operator=(const Interface &) = delete;
    Interface &operator=(Interface &&) = delete;

    //---------------------------------------------------------------------------------------------
    //! Interface initialization.
    //---------------------------------------------------------------------------------------------
    void initialize(const Interface::Configuration &cfg) noexcept;

    //---------------------------------------------------------------------------------------------
    //! Interface termination/cleanup.
    //---------------------------------------------------------------------------------------------
    void terminate() noexcept;

    //---------------------------------------------------------------------------------------------
    //! Check configured (or still in post instanciation state)
    //---------------------------------------------------------------------------------------------
    inline bool is_configured() const noexcept
    {
        return configured;
    }

    //---------------------------------------------------------------------------------------------
    //! Get the interface identifier.
    //!
    //! Returns:
    //!   the interface identifier.
    //---------------------------------------------------------------------------------------------
    inline const std::string &get_id() const noexcept
    {
        return cfg.id;
    }

    //---------------------------------------------------------------------------------------------
    //! Get the default Tracer instance.
    //!
    //! Returns:
    //!   the default Tracer (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::telemetry::Tracer> &get_tracer() noexcept
    {
        return tracer;
    }

    //---------------------------------------------------------------------------------------------
    //! Get or create a named Tracer instance.
    //!
    //! Parameters:
    //!   name – name Tracer instrumentation scope.
    //!
    //! Returns:
    //!   the named Tracer (as a std::shared_ptr).
    //---------------------------------------------------------------------------------------------
    inline std::shared_ptr<Tango::telemetry::Tracer> get_tracer(const std::string &name) noexcept
    {
        auto otel_tracer = provider->GetTracer(name, OPENTELEMETRY_SDK_VERSION);
        return std::make_shared<Tango::telemetry::Tracer>(otel_tracer);
    }

    //-------------------------------------------------------------------------------------------------
    // scope:  a helper function for macro passing things like __FUNCTION__, __FILE__, __LINE__
    //-------------------------------------------------------------------------------------------------
    inline Tango::telemetry::Scope scope(const char *file = "unknown-source-file",
                                         int line = -1,
                                         const char *name = "anonymous-span",
                                         const Tango::telemetry::Attributes &user_defined_attributes = {},
                                         const Span::Kind &kind = Span::Kind::Internal)
    {
        Tango::telemetry::Attributes span_attributes{{"code.filepath", file_basename(file)},
                                                     {"code.lineno", std::to_string(line)},
                                                     {"thread.id", thread_id_to_string()}};

        span_attributes.insert(user_defined_attributes.begin(), user_defined_attributes.end());

        return Tango::telemetry::Scope(get_tracer()->start_span(name, span_attributes, kind));
    }

    //-------------------------------------------------------------------------------------------------
    // rpc_exception_hook: interceptor hook - allows to catch and trace exception
    //-------------------------------------------------------------------------------------------------
    static CORBA::Boolean rpc_exception_hook(omni::omniInterceptors::serverSendException_T::info_T &);

  private:
    //- default (dummy) tracer initialization
    void init_default_trace_provider() noexcept;

    //- tracer initialization and cleanup
    void init_trace_provider() noexcept;
    void cleanup_trace_provider() noexcept;

    //- propagator initialization and cleanup
    void init_propagator() noexcept;
    void cleanup_propagator() noexcept;

    // not  yet configured
    bool configured{false};

    //- the interface configuration
    Interface::Configuration cfg;

    // the opentelemetry provider
    std::shared_ptr<opentelemetry::trace::TracerProvider> provider;

    // the opentelemetry tracer
    std::shared_ptr<Tango::telemetry::Tracer> tracer;

    //- the bridge between Tango (CORBA) and OpenTelemetry
    TangoCarrier carrier;
};

//-------------------------------------------------------------------------------------------------
// InterfaceFactory: an Interface factory.
//-------------------------------------------------------------------------------------------------
class InterfaceFactory
{
  public:
    //---------------------------------------------------------------------------------------------
    //! Interface instantiation.
    //!
    //! Parameters:
    //!   cfg – the interface configuration.
    //!
    //! Throws:
    //!   a Tango::DevFailed in case a interface already exists with the same name.
    //---------------------------------------------------------------------------------------------
    static std::shared_ptr<Tango::telemetry::Interface> create(const Tango::telemetry::Interface::Configuration &cfg);

  private:
    InterfaceFactory() = delete;
};

} // namespace Tango::telemetry

  #define INTERNAL_SPAN Tango::telemetry::Span::Kind::Internal
  #define CLIENT_SPAN Tango::telemetry::Span::Kind::Client
  #define SERVER_SPAN Tango::telemetry::Span::Kind::Server
  #define PRODUCER_SPAN Tango::telemetry::Span::Kind::Producer
  #define CONSUMER_SPAN Tango::telemetry::Span::Kind::Consumer

  //-------------------------------------------------------------------------------
  // TELEMETRY_SCOPE
  //
  // Start a new "scoped" span.
  //
  // The "location" (i.e., the file name and the line number) where the scope (i.e.,
  // the span) has been created is automatically added to the span attributes.
  //
  // Usage:
  //
  //    TELEMETRY_SCOPE(TANGO_CURRENT_FUNCTION);
  //        `-> span name = current function name (see TANGO_CURRENT_FUNCTION)
  //        `-> no span attributes
  //        `-> span kind = Tango::telemetry::Span::Kind::Internal
  //
  //    TELEMETRY_SCOPE("MyCustomSpanName");
  //        `-> span name = user defined span name
  //        `-> no span attributes
  //        `-> span kind = Tango::telemetry::Span::Kind::Internal
  //
  //    TELEMETRY_SCOPE("MyCustomSpanName", myAttrs);
  //        `-> user defined span name
  //        `-> user defined span attributes
  //        `-> span kind = Tango::telemetry::Span::Kind::Internal
  //
  //    TELEMETRY_SCOPE("MyCustomSpanName", myAttrs, Tango::telemetry::Span::Kind::Producer);
  //        `-> user defined span name
  //        `-> user defined span attributes
  //        `-> user defined span kind
  //-------------------------------------------------------------------------------
  #define TELEMETRY_SCOPE(...) \
      auto __telemetry_scope__ = Tango::utils::tss::current_telemetry_interface->scope(__FILE__, __LINE__, __VA_ARGS__)

  //-------------------------------------------------------------------------------
  // TELEMETRY_CLIENT_SCOPE
  //
  // Start a new "client" span.
  //
  // For Tango kernel internal usage only. This is used by the Tango::DeviceProxy
  // to initiate a client RPC. This follows the Opentelemetry semantic convention.
  //
  // Usage:
  //
  //    TELEMETRY_CLIENT_SCOPE;
  //-------------------------------------------------------------------------------
  #define TELEMETRY_CLIENT_SCOPE TELEMETRY_SCOPE(TANGO_CURRENT_FUNCTION, {}, CLIENT_SPAN)

  //-------------------------------------------------------------------------------
  // TELEMETRY_SERVER_SCOPE
  //
  // Start a new "server" span.
  //
  // For Tango kernel internal usage only. This is used by the,several flavors of
  // Tango::DeviceImpl to initiate a reply to a client RPC. This follows the
  // Opentelemetry semantic convention.
  //
  // Usage:
  //
  //    TELEMETRY_CLIENT_SCOPE;
  //-------------------------------------------------------------------------------
  #define TELEMETRY_SERVER_SCOPE TELEMETRY_SCOPE(TANGO_CURRENT_FUNCTION, {}, SERVER_SPAN)

  //-------------------------------------------------------------------------------
  // TELEMETRY_ADD_EVENT
  //
  // Add an event to the current span.
  //
  // To be used in a context where TELEMETRY_SCOPE, TELEMETRY_CLIENT_SCOPE or
  // TELEMETRY_SERVER_SCOPE has been previously called.
  //
  // Usage:
  //
  //    TELEMETRY_SCOPE;
  //    ...
  //    TELEMETRY_ADD_EVENT("this event msg will be attached to the current span");
  //-------------------------------------------------------------------------------
  #define TELEMETRY_ADD_EVENT(__MSG__) __telemetry_scope__->add_event(__MSG__)

  //-------------------------------------------------------------------------------
  // TELEMETRY_SET_ERROR_STATUS
  //
  // Set the error status of the current span.
  //
  // To be used in a context where TELEMETRY_SCOPE, TELEMETRY_CLIENT_SCOPE or
  // TELEMETRY_SERVER_SCOPE has been previously called. This is typically
  // called in error/exception context.
  //
  // Usage:
  //
  //    TELEMETRY_SCOPE;
  //    try
  //    {
  //       do_some_job();
  //    }
  //    catch (...)
  //    {
  //      TELEMETRY_SET_ERROR_STATUS("oops, an error occurred in the current span");
  //    }
  //-------------------------------------------------------------------------------
  #define TELEMETRY_SET_ERROR_STATUS(__MSG__) \
      __telemetry_scope__->set_status(Tango::telemetry::Span::Status::Error, __MSG__)

  //-------------------------------------------------------------------------------
  // TSS_ATTACH_TELEMETRY_INTERFACE
  //
  // Attach the specified interface to the current thread (Thread Specific Storage).
  // Uses the magic provide by the C++11 "thread_local" keyword. See Tango::utils::tss.
  //
  //
  // To be used in a context where TELEMETRY_SCOPE, TELEMETRY_CLIENT_SCOPE or
  // TELEMETRY_SERVER_SCOPE has been previously called. This is typically
  // called in error/exception context. The argument of this macro must be a
  // std::shared_ptr<Tango::telemetry::Interface>. An isolated thread would be
  // a typical context in which one would use this macro.
  //
  // Usage:
  //
  //    std::shared_ptr<Tango::telemetry::Interface> ti = get_my_telemetry_interface();
  //
  //    void thread_entry_point()
  //    {
  //        TSS_ATTACH_TELEMETRY_INTERFACE(ti);
  //        while (go_on)
  //        {
  //            TELEMETRY_SCOPE("myThreadLoop");
  //            do_some_job();
  //        }
  //    }
  //-------------------------------------------------------------------------------
  #define TSS_ATTACH_TELEMETRY_INTERFACE(__TI__) Tango::utils::tss::current_telemetry_interface = __TI__

#else // TELEMETRY_ENABLED

  #define INTERNAL_SPAN 0
  #define CLIENT_SPAN 1
  #define SERVER_SPAN 2
  #define PRODUCER_SPAN 3
  #define CONSUMER_SPAN 4

  #define TELEMETRY_SCOPE
  #define TELEMETRY_CLIENT_SCOPE
  #define TELEMETRY_SERVER_SCOPE
  #define TELEMETRY_ADD_EVENT
  #define TELEMETRY_SET_ERROR_STATUS
  #define TSS_ATTACH_TELEMETRY_INTERFACE

#endif // TELEMETRY_ENABLED

#endif // TANGO_COMMON_TELEMETRY_H
