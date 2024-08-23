#ifndef TANGO_COMMON_TELEMETRY_H
#define TANGO_COMMON_TELEMETRY_H

#include <tango/common/tango_version.h>

#if defined(TANGO_USE_TELEMETRY)

  #include <tango/common/telemetry/configuration.h>

  #include <map>
  #include <regex>
  #include <memory>
  #include <string>
  #include <cstring>
  #include <variant>

namespace Tango::telemetry
{
//---------------------------------------------------------------------------------------------------------------------
// Implementation note
//
// We decided to totally hide the underlying telemetry dependency (opentelemetry) using the "PIMPL" the idiom. The
// latter makes use of opaque implementation classes to which the actual 'actions' are delegated - the aim is to
// provide a stable ABI by allowing to change the underlying implementation without impacting users' code.
//
// See the iso-cpp guidelines on PIMPL:
// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#i27-for-stable-library-abi-consider-the-pimpl-idiom
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// The opaque implementation classes.
//
// Such classes are usually inner classes of their associated public class but, due to a technical constraint we have
// here on one of them, we decide, for all of them, to put their respective forward declaration here.
//---------------------------------------------------------------------------------------------------------------------
class SpanImplementation;
class ScopeImplementation;
class InterfaceImplementation;

constexpr const char *kTelemetryLogAppenderName = "telemetry_logs_appender";

//---------------------------------------------------------------------------------------------------------------------
//! Traces endpoint env. variable. This is the name of the optional env. variable containing the url to which traces
//! are sent (collector).
//---------------------------------------------------------------------------------------------------------------------
constexpr const char *kEnvVarTelemetryTracesEndPoint = "TANGO_TELEMETRY_TRACES_ENDPOINT";

//---------------------------------------------------------------------------------------------------------------------
//! Traces endpoint env. variable. This is the name of the optional env. variable containing the url to which logs
//! are sent (collector).
//---------------------------------------------------------------------------------------------------------------------
constexpr const char *kEnvVarTelemetryLogsEndPoint = "TANGO_TELEMETRY_LOGS_ENDPOINT";

constexpr const char *kEnvVarTelemetryEnable = "TANGO_TELEMETRY_ENABLE";

constexpr const char *kEnvVarTelemetryKernelEnable = "TANGO_TELEMETRY_KERNEL_ENABLE";

constexpr const char *kEnvVarTelemetryTracesExporter = "TANGO_TELEMETRY_TRACES_EXPORTER";

constexpr const char *kEnvVarTelemetryLogsExporter = "TANGO_TELEMETRY_LOGS_EXPORTER";

//---------------------------------------------------------------------------------------------------------------------
//! AttributeValue
//!
//! Telemetry signals can be enriched by adding metadata in the form of some key:value pairs known as "attributes". The
//! following std::variant holds the supported data types for the value. The types its supports is a subset of the ones
//! supported by the underlying implementation. This list could evolve with the adoption of the c++ >= 20.
//!
//! \see Tango::telemetry::Span::set_attribute.
//---------------------------------------------------------------------------------------------------------------------
using AttributeValue = std::variant<bool, std::int32_t, std::int64_t, std::uint32_t, double, const char *, std::string>;

//---------------------------------------------------------------------------------------------------------------------
//! Attributes
//!
//! A list of signal attributes - with (potentially) polymorphic values.
//!
//! \see Tango::telemetry::Interface::start_span.
//---------------------------------------------------------------------------------------------------------------------
using Attributes = std::map<std::string, AttributeValue>;

//---------------------------------------------------------------------------------------------------------------------
//! Span
//---------------------------------------------------------------------------------------------------------------------
//! A "Span" is a fundamental concept that represents a single 'operation' within a trace. Spans are the building
//! blocks of a trace and can be thought of as individual units of work done in a distributed system.
//!
//! What a Span represents:
//! A span represents an operation or a set of operations, such as a CORBA request, a database query, or any discrete
//! unit of work in the application or system. Spans can be nested and ordered to model the relationships between
//! operations, creating a structured trace of the transaction.
//!
//! Hierarchy and relationships:
//! Spans have a parent-child relationship. A span without a parent is called a "root span", and it usually represents
//! the start of a trace. Child spans can be created to represent operations that are part of a larger parent operation.
//! This hierarchical structure helps to trace the flow of a request through various services and operations.
//!
//! Attributes of a Span:
//! Each span can contain multiple attributes that provide context about the operation, such as its name, start time,
//! end time, status, and additional metadata.
//!
//! Events and links:
//! Spans can also contain events, which are timestamped annotations that provide additional information about the
//! operation.
//!
//! Trace context propagation:
//! In distributed tracing, spans carry a trace context that uniquely identifies the trace and also maintains the
//! position of the span within the trace. This context is propagated across process boundaries, allowing distributed
//! systems to be traced as a unified operation.
//!
//! Use in observability:
//! Spans are used to measure the latency of operations, track their outcomes, and diagnose issues. When collected
//! and analyzed, they provide insights into the performance and behavior of applications and services.
//---------------------------------------------------------------------------------------------------------------------
class Span final
{
  public:
    //-----------------------------------------------------------------------------------------------------------------
    //! In telemetry, the "kind" of a span specifies the role of a span within a trace, giving context to how the
    //! span relates to other spans. This is particularly important in distributed tracing to understand the
    //! interactions between different components in a system (a client and a server, a producer and a consumer).
    //!
    //! The kind of a span helps in understanding whether a span is part of a remote request/response cycle, a messaging
    //! system, or an internal process. This information is crucial for correctly analyzing the trace data, especially
    //! in complex control systems where multiple devices (and services) interact with each other.
    //-----------------------------------------------------------------------------------------------------------------
    enum class Kind
    {
        kInternal, //! The kind of span created to represent an internal operation (default value).
        kServer,   //! The kind of span created by a server at the entry point of a RPC.
        kClient,   //! The kind of span created by a client initiating a RPC.
        kProducer, //! The kind of span created by an (async.) event producer.
        kConsumer  //! The kind of span created by an (async.) event consumer.
    };

    //-----------------------------------------------------------------------------------------------------------------
    //! StatusCode: represents the canonical set of status codes of a finished Span.
    //-----------------------------------------------------------------------------------------------------------------
    enum class Status
    {
        kUnset, //! The unknown/unset status (default value).
        kOk,    //! The operation has completed successfully.
        kError  //! The operation contains an error. This is the only status code that can be actually set.
    };

    //! A Span has a private default constructor and is neither copyable nor movable.
    Span(const Span &) = delete;

    //! A Span has a private default constructor and is neither copyable nor movable.
    Span(Span &&) = delete;

    //! A Span has a private default constructor and is neither copyable nor movable.
    Span &operator=(const Span &) = delete;

    //! A Span has a private default constructor and is neither copyable nor movable.
    Span &operator=(Span &&) = delete;

    //! The Span destructor end()s the Span, if it hasn't been ended already.
    ~Span();

    //!----------------------------------------------------------------------------------------------------------------
    //! Set an attribute on the Span.
    //!
    //! Attaches the specified attribute to the span.
    //! If the Span previously contained a mapping for the key, the old value is replaced.
    //!
    //! @param key The attribute key (i.e., label)
    //! @param value The attribute value.
    //!
    //! \see Tango::telemetry::AttributeValue.
    //!----------------------------------------------------------------------------------------------------------------
    void set_attribute(const std::string &key, const AttributeValue &value) noexcept;

    //!----------------------------------------------------------------------------------------------------------------
    //! Add an event on the Span.
    //!
    //! Attaches the specified timestamped annotation to the span.
    //!
    //! @param name the event name (annotation).
    //! @param attributes some attributes optional attributes to be attached to the Span.
    //!----------------------------------------------------------------------------------------------------------------
    void add_event(const std::string &name, const Attributes &attributes = {}) noexcept;

    //!----------------------------------------------------------------------------------------------------------------
    //! Set the status of the span.
    //!
    //! Changes the status of the span to the specified value. Only the value of the last call will be recorded.
    //!
    //! @param code The status code.
    //! @param description A description of the new status.
    //!
    //! \see Tango::telemetry::Span::Status.
    //!----------------------------------------------------------------------------------------------------------------
    void set_status(const Span::Status &code, const std::string &description = "") noexcept;

    //!----------------------------------------------------------------------------------------------------------------
    //! Get the status of the span.
    //!
    //! \return the current status of the Span.
    //!
    //! \see Tango::telemetry::Span::Status.
    //!----------------------------------------------------------------------------------------------------------------
    Span::Status get_status() const noexcept;

    //!----------------------------------------------------------------------------------------------------------------
    //! Get the error status of the span.
    //!
    //! \return returns true is the current status of the Span is set to Span::Status::kError, returns false otherwise.
    //!
    //! \see Tango::telemetry::Span::Status.
    //!----------------------------------------------------------------------------------------------------------------
    bool has_error() const noexcept
    {
        return get_status() != Span::Status::kError;
    }

    //!----------------------------------------------------------------------------------------------------------------
    //! Mark the end of the Span.
    //!
    //! Marks the end of the Span (freezes it).
    //! Only the timing of the first "end" call for a given Span will be recorded.
    //!----------------------------------------------------------------------------------------------------------------
    void end() noexcept;

    //!----------------------------------------------------------------------------------------------------------------
    //! Returns true is the Span is recording, returns false otherwise.
    //! TODO: document the semantic of "recording/sampling" in (open)telemetry.
    //!----------------------------------------------------------------------------------------------------------------
    bool is_recording() const noexcept;

  private:
    // some friend classes that need to access the opaque part of a Span behind the scenes
    //------------------------------------------------------------------------------------
    // the Scope class needs to access the concrete Span impl to activate it
    friend class ScopeImplementation;
    // the Interface class must be able to instantiate a Span
    friend class InterfaceImplementation;
    // private ctor - a Span can't be directly instantiated
    Span() = default;
    // opaque implementation
    std::unique_ptr<SpanImplementation> impl;
};

//---------------------------------------------------------------------------------------------------------------------
// SpanPtr
//---------------------------------------------------------------------------------------------------------------------
using SpanPtr = std::unique_ptr<Span>;

//---------------------------------------------------------------------------------------------------------------------
// Scope
//---------------------------------------------------------------------------------------------------------------------
// In telemetry, the concept of "Scope" is crucial for context management, particularly in relation to Spans.
// A Scope essentially defines the current active Span in a specific execution thread or context. Scope is a mechanism
// for managing the active Span in the context of an execution thread, ensuring that Spans are correctly nested and the
// trace is accurately constructed as the program executes. This concept is integral to distributed tracing, as it aids
// in correctly associating the work being performed with the appropriate Span. Any Span instantiated in the context of
// Scope becomes a child on the Span activated by the Scope. A Span created out of a Scope is considered as a root Span.
//---------------------------------------------------------------------------------------------------------------------
class Scope final
{
  public:
    //!----------------------------------------------------------------------------------------------------------------
    //! Initialize a new scope.
    //! The given span will be set as the currently active span.
    //!
    //! @param span The span to activate for the lifetime of the Scope.
    //!----------------------------------------------------------------------------------------------------------------
    Scope(const SpanPtr &span) noexcept;

    //!----------------------------------------------------------------------------------------------------------------
    //! Release the scope.
    //! The associated Span is deactivated (as a consequence, the previously active one is restored).
    //!----------------------------------------------------------------------------------------------------------------
    ~Scope();

  private:
    // opaque implementation
    std::unique_ptr<ScopeImplementation> impl;
};

//---------------------------------------------------------------------------------------------------------------------
// ScopePtr
//---------------------------------------------------------------------------------------------------------------------
using ScopePtr = std::unique_ptr<Scope>;

//---------------------------------------------------------------------------------------------------------------------
// InterfacePtr
//---------------------------------------------------------------------------------------------------------------------
class Interface;
using InterfacePtr = std::shared_ptr<Interface>;

//---------------------------------------------------------------------------------------------------------------------
// The telemetry interface currently attached to the current thread (thread_local storage) - see Interface for details
//---------------------------------------------------------------------------------------------------------------------
// This is notably set by the Tango device upon reception of an external request using the RAII idiom implemented by
// the InterfaceScope class (which makes a call to Interface::set_current). We do attach the telemetry interface of the
// device to the thread handling the client request at the most upstream point in the call stack - i.e. the location
// considered as the entry point of the request. This allows any downstream code to generate traces on behalf of a
// given device by retrieving the "current" telemetry interface. This contributes to the required "per device telemetry"
// design. A pure client will tipically instantiate a client flavor of the Interface class at startup. This interface
// can be shared and activated by any thread wanting to generate telemetry signals on behalf og the client.
//---------------------------------------------------------------------------------------------------------------------
extern thread_local InterfacePtr current_telemetry_interface;

//---------------------------------------------------------------------------------------------------------------------
// Interface: an interface for Tango telemetry
//---------------------------------------------------------------------------------------------------------------------
// Any "entity" wanting to generate telemetry signals on its own (i.e. not on behalf of another entity) should hold its
// own instance of the Tango::telemetry::Interface class. That's the case for any Tango device (per device telemetry
// design). The same rule applies to a pure client. The interface configuration allows to distinguish the two cases.
// See the thread_local Tango::telemetry::current_telemetry_interface variable for details.
//---------------------------------------------------------------------------------------------------------------------
class Interface
{
    //-----------------------------------------------------------------------------------------------------------------
    // internal helper function: thread_id_to_string (for tracing location)
    //-----------------------------------------------------------------------------------------------------------------
    static std::string thread_id_to_string() noexcept
    {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    }

    //-----------------------------------------------------------------------------------------------------------------
    // internal helper function: forward declaration of the method returning the default interface.
    // see Interface::get_current for details regarding the semantic and usage of the default interface.
    //-----------------------------------------------------------------------------------------------------------------
    static Tango::telemetry::InterfacePtr get_default_interface();

  public:
    //-----------------------------------------------------------------------------------------------------------------
    //  CTORs & ASSIGNMENT OPERATORs
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! Construct a new Interface object.
    //!
    //! This constructor might throw a Tango::DevFailed in case the instantiation of the underlying interface failed -
    //! it will notably be the case if one of telemetry endpoints is ill-formed.
    //!
    //! @param cfg the v configuration.
    //!
    //! \throws Tango::DevFailed (see description)
    //!
    //! \see Configuration
    //-----------------------------------------------------------------------------------------------------------------
    Interface(const Configuration &cfg);

    //- some disabled features
    Interface(const Interface &) = delete;
    Interface(Interface &&) = delete;
    Interface &operator=(const Interface &) = delete;
    Interface &operator=(Interface &&) = delete;

    ~Interface();

    //-----------------------------------------------------------------------------------------------------------------
    //  MISC. ACCESSORs & MUTATORs
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! Check if the interface is the default one.
    //!
    //! \returns: true if the telemetry interface the default one, returns false otherwise.
    //-----------------------------------------------------------------------------------------------------------------
    bool is_default() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //  MISC. ACCESSORs & MUTATORs
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! Get the interface state.
    //!
    //! A disabled Interface does nothing (telemetry signals are dropped).
    //!
    //! \returns: true if the telemetry interface is enabled, returns false otherwise.
    //-----------------------------------------------------------------------------------------------------------------
    bool is_enabled() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Enable the interface.
    //!
    //! A disabled Interface does nothing (i.e., telemetry signals are silently dropped).
    //-----------------------------------------------------------------------------------------------------------------
    void enable() noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Disable the interface.
    //!
    //! A disabled Interface does nothing (i.e., telemetry signals are silently dropped).
    //-----------------------------------------------------------------------------------------------------------------
    void disable() noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! A helper function that returns true if the kernel traces are enabled, it returns false otherwise.
    //! Please note, that this concerns only a subset of the kernel traces that are disabled by default (e.g., the ones
    //! related to the calls to the Tango database when a Tango::DeviceProxy is instantiated).
    //-----------------------------------------------------------------------------------------------------------------
    bool are_kernel_traces_enabled() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! A helper function that returns true if the kernel traces are disabled, its returns false otherwise.
    //! Please note, that this concerns only a subset of the kernel traces that are disabled by default (e.g., the ones
    //! related to the calls to the Tango database when a Tango::DeviceProxy is instantiated).
    //-----------------------------------------------------------------------------------------------------------------
    bool are_kernel_traces_disabled() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Enable the kernel traces.
    //! Please note, that this concerns only a subset of the kernel traces that are disabled by default (e.g., the ones
    //! related to the calls to the Tango database when a Tango::DeviceProxy is instantiated).
    //-----------------------------------------------------------------------------------------------------------------
    void enable_kernel_traces() noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Disable the kernel traces.
    //! Please note, that this concerns only a subset of the kernel traces that are disabled by default (e.g., the ones
    //! related to the calls to the Tango database when a Tango::DeviceProxy is instantiated).
    //-----------------------------------------------------------------------------------------------------------------
    void disable_kernel_traces() noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Get the interface identifier.
    //!
    //! The interface identifier is a user defined label that do not play any particular role in the behavior of the
    //! telemetry interface. It is specified at instantiation and can't be changed.
    //!
    //! \returns The interface identifier.
    //!
    //! \see Configuration
    //-----------------------------------------------------------------------------------------------------------------
    const std::string &get_id() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Get the interface configuration.
    //!
    //! \returns The interface configuration.
    //!
    //! \see Configuration
    //-----------------------------------------------------------------------------------------------------------------
    const Configuration &get_configuration() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    // API RELATED TO SPANs
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! Start a new Span.
    //!
    //! It is recommended to use the TANGO_TELEMETRY_SPAN macro rather than this function. Please use the macro unless
    //! you know what you're doing. A misusage of the \p kind parameter can notably created some side effects on backend
    //! side.
    //
    //! For consistency reasons, it is recommended to use the TANGO_CURRENT_FUNCTION macro to name the new span.
    //! Some attributes can be attached to the new span. Use \ref Span::set_attribute to add an individual attribute
    //! to the span.
    //!
    //! In case of memory error, an invalid default span is returned. The latter will simply be transparently
    //! dropped by the underlying implementation. This is part of our "transparent telemetry strategy" aiming to
    //! guarantee that the telemetry service never influences the behavior of the client or server making use of it.
    //!
    //! Example:
    //! @code
    //! auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION, {{"key", "value"}});
    //! auto scope = Tango::telemetry::Interface::Scope(span);
    //! @endcode
    //!
    //! @param name the name of the Span - use TANGO_CURRENT_FUNCTION unless you want to specify a particular name.
    //! @param attributes a std::map of key/value pairs where the key is a std::string and the value an
    //! AttributeValue.
    //! @param kind the Span::Kind - defaults to Span::Kind::kInternal.
    //!
    //! \returns std::shared_ptr holding the new Tango::telemetry::Span or an invalid default span.
    //!
    //! \see TANGO_TELEMETRY_SPAN
    //! \see Tango::telemetry::Span::set_attribute
    //! \see Tango::telemetry::AttributeValue
    //-----------------------------------------------------------------------------------------------------------------
    SpanPtr start_span(const std::string &name,
                       const Attributes &attributes = {},
                       const Span::Kind &kind = Span::Kind::kInternal) noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Return the current Span.
    //!
    //! Allows to retrieve the current span for a downstream context. The TANGO_TELEMETRY_ADD_ATTRIBUTE macro offers a
    //! syntactic shortcut for this function.
    //!
    //! @code
    //! // typical usage
    //! auto current_span = Tango::telemetry::Interface::get_current().get_current_span();
    //! current_span.add_event("some annotation");
    //! @endcode
    //!
    //! \returns A SpanPtr (i.e., a std::shared_ptr) holding the current Tango::telemetry::Span.
    //-----------------------------------------------------------------------------------------------------------------
    SpanPtr get_current_span() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    // API RELATED TO SCOPEs
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! A helper function for macros passing __FILE__ and __LINE__. It's not supposed to be used directly.
    //! The aim of this function is to make the specified Span the active one for the lifetime of the returned Scope.
    //!
    //! Example:
    //! @code
    //! auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION);
    //! auto scope = TANGO_TELEMETRY_SCOPE(span);
    //! @endcode
    //!
    //! @param span the Span to activate.
    //!
    //! \returns a Tango::telemetry::Scope.
    //!
    //! \see TANGO_TELEMETRY_SCOPE
    //! \see Tango::telemetry::Scope
    //-----------------------------------------------------------------------------------------------------------------
    Tango::telemetry::Scope
        scope(Tango::telemetry::SpanPtr &span, const char *file = "unknown-source-file", int line = -1)
    {
        span->set_attribute("code.filepath", Tango::logging_detail::basename(file));
        span->set_attribute("code.lineno", std::to_string(line));
        span->set_attribute("thread.id", thread_id_to_string());
        return Tango::telemetry::Scope(span);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // API RELATED TO THE ACTIVE INTERFACE
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! Get the active Tango::telemetry::Interface for the current thread.
    //!
    //! The current interface is either the one activated by a pure client at startup or the one activated by the cpp
    //! kernel at the entry point of a remote request. In case there is no activate interface for the current thread,
    //! the default one is returned. The aim of the default interface is to guarantee that we always have a valid
    //! interface to return to the caller. However, the default interface will silently drop any telemetry signal.
    //! Having a default "noop" interface simplifies the implementation by avoid systematic validity checking of the
    //! Tango::telemetry::current_telemetry_interface.
    //!
    //! \returns: the active Tango::telemetry::Interface (as a std::shared_ptr) or the default one if none.
    //!
    //! \see Tango::telemetry::Interface::set_current
    //-----------------------------------------------------------------------------------------------------------------
    static InterfacePtr get_current()
    {
        return Tango::telemetry::current_telemetry_interface ? Tango::telemetry::current_telemetry_interface
                                                             : Interface::get_default_interface();
    }

    //-----------------------------------------------------------------------------------------------------------------
    //! Set the active Tango::telemetry::Interface for the current thread.
    //!
    //! @param telemetry_interface the Tango::telemetry::Interface to activate.
    //!
    //! \see Tango::telemetry::Interface::get_current
    //-----------------------------------------------------------------------------------------------------------------
    static void set_current(Tango::telemetry::InterfacePtr &telemetry_interface) noexcept
    {
        Tango::telemetry::current_telemetry_interface = telemetry_interface;
    }

    //-----------------------------------------------------------------------------------------------------------------
    // API RELATED TO CONTEXT PROPAGATION ALONG THE DISTRIBUTED CALL STACK
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! Set the trace context (part of the trace context propagation)
    //!
    //! This method is used by the Tango::Device_[X]Impl to setup the trace context upon reception of a remote call.
    //! This flavor of "set_trace_context" is used where the caller is using an IDL version >= 4 and propagates context
    //! information. A new span is created and becomes the active one thanks to the returned Scope. The name of the new
    //! span is specified by the "new_span_name" argument.
    //!
    //! @param new_span_name the name of the Span created behind the scenes.
    //! @param span_attrs some optional Span attributes - just pass {} if none.
    //! @param client_identification the data structure containing the telemetry context coming from the client.
    //!
    //! \see Tango::telemetry::Interface::get_trace_context
    //-----------------------------------------------------------------------------------------------------------------
    static ScopePtr set_trace_context(const std::string &new_span_name,
                                      const Tango::telemetry::Attributes &span_attrs,
                                      const Tango::ClntIdent &client_identification) noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Set the trace context (part of the trace context propagation)
    //!
    //! This method is used by the Tango::Device_[X]Impl to setup the trace context upon reception of a remote call.
    //! This flavor of "set_trace_context" is used where the caller is using an IDL version < 4 and doesn't propagate
    //! any context information (local tracing only). A new span is created and becomes the active one thanks to the
    //! returned Scope. The name of the new span is specified by the "new_span_name" argument.
    //!
    //! @param new_span_name the name of the Span created behind the scenes.
    //! @param span_attrs some optional Span attributes.
    //!
    //! \see Tango::telemetry::Interface::get_trace_context
    //-----------------------------------------------------------------------------------------------------------------
    static ScopePtr set_trace_context(const std::string &new_span_name,
                                      const Tango::telemetry::Attributes &span_attrs = {}) noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Set the trace context (part of the trace context propagation)
    //!
    //! This is a helper function for pyTango or any other binding. It's not supposed to be used directly.
    //!
    //! All we have to do here, is to provide the binding with a way to get and set the current telemetry context when
    //! it calls the kernel (e.g., making use of a DeviceProxy). This method allows to set the current telemetry context
    //! (i.e., the one currently active on binding side) using its W3C format (i.e., trough the trace_parent &
    //! trace_state headers passed as arguments.
    //!
    //! The new context creates a span that becomes the active one thanks to the returned Scope. The name of that new
    //! span is specified by the "new_span_name" argument.
    //!
    //! @param new_span_name the name of the Span created behind the scenes.
    //! @param trace_parent the W3C 'traceparent' header.
    //! @param trace_state the W3C 'tracestate' header.
    //! @param kind the 'kind' of Span to create.
    //!
    //! \see Tango::telemetry::Interface::get_trace_context
    //! \see Tango::telemetry::Span::Kind
    //-----------------------------------------------------------------------------------------------------------------
    static ScopePtr set_trace_context(const std::string &new_span_name,
                                      const std::string &trace_parent,
                                      const std::string &trace_state,
                                      const Span::Kind &kind = Span::Kind::kClient) noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //! Get the current trace context.
    //!
    //! This method is used by the Tango::Connection (and its child classes - e.g., DeviceProxy) to propagate the trace
    //! context to the callee. It could also be used by pyTango. The trace context is obtained in its W3C format trough
    //! the two strings passed as arguments(trace_parent & trace_state). It is also use by any binding (e.g., pyTango)
    //! that wants to propagate the c++ tracing context in its own environment.
    //!
    //! @param trace_parent the W3C 'traceparent' header.
    //! @param trace_state the W3C 'tracestate' header.
    //!
    //! \see Tango::telemetry::Interface::set_trace_context
    //-----------------------------------------------------------------------------------------------------------------
    static void get_trace_context(std::string &trace_parent, std::string &trace_state) noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    // API RELATED TO LOGs
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    // Instantiate a log4tango::Appender routing the device logs to the telemetry collector.
    //
    // The caller obtains the ownership of the returned pointer and must delete once done with it.
    //-----------------------------------------------------------------------------------------------------------------
    log4tango::Appender *get_appender() const noexcept;

    //-----------------------------------------------------------------------------------------------------------------
    //  MISC HELPERs
    //-----------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------
    //! A helper function that tries to extract an error message from the current exception.
    //!
    //! Example:
    //! @code
    //! try
    //! {
    //!   ...
    //! }
    //! catch (...)
    //! {
    //!   auto error_msg = Tango::telemetry::extract_exception_info();
    //!   std::cout << "oops, an error occurred: " << error_msg << std::endl;
    //! }
    //! @endcode
    //!
    //! @param current_exception the exception from which we want to extract the message. Defaults to the current one.
    //!
    //! \returns the error message
    //-----------------------------------------------------------------------------------------------------------------
    static std::string extract_exception_info(std::exception_ptr c = std::current_exception());

    //-----------------------------------------------------------------------------------------------------------------
    //! A helper function that tries to extract both the error type and a message from the current exception.
    //!
    //! Example:
    //! @code
    //! try
    //! {
    //!   ...
    //! }
    //! catch (...)
    //! {
    //!   std::string error_type, error_msg;
    //!   Tango::telemetry::extract_exception_info(error_type, error_msg);
    //!   std::cout << "oops, an error of type " << error_type << " has been caught: " << error_msg << std::endl;
    //! }
    //! @endcode
    //!
    //! @param error_type the exception type as a std::string.
    //! @param error_message the associated error message as a std::string.
    //-----------------------------------------------------------------------------------------------------------------
    static void extract_exception_info(std::string &error_type, std::string &error_message);

  private:
    // opaque implementation
    std::shared_ptr<InterfaceImplementation> impl;
};

//-----------------------------------------------------------------------------------------------------------------
//! A telemetry Interface factory.
//-----------------------------------------------------------------------------------------------------------------
class InterfaceFactory
{
  public:
    //-----------------------------------------------------------------------------------------------------------------
    //! Create a new Interface object.
    //!
    //! This factory function might throw a Tango::DevFailed in case the instantiation of the underlying interface
    //! failed - it will notably be the case if one of telemetry endpoints is ill-formed.
    //!
    //! @param cfg the v configuration.
    //!
    //! \throws Tango::DevFailed (see description)
    //!
    //! \see Configuration
    //-----------------------------------------------------------------------------------------------------------------
    static InterfacePtr create(const Tango::telemetry::Configuration &cfg);

  private:
    InterfaceFactory() = delete;
};

//---------------------------------------------------------------------------------------------------------------------
//! A instance of the class InterfaceScope snapshots which interface is currently attached to the current thread then
//! attaches the specified one to the thread. Uses the magic provide by the C++11 "thread_local" keyword. The interface
//! that was initially active is restored when the InterfaceScope instance goes out of scope (RAII paradigm). This is
//! mostly used by the Tango kernel. However, the users could use it in pure clients (to trace on behalf on that
//! client).
//!
//! Usage example:
//!
//!    void myClientEntryPoint()
//!    {
//!        Tango::telemetry::Configuration cfg{
//!                                 true,
//!                                 false,
//!                                 "myClientTelemetryInterface",
//!                                 "diagnostics",
//!                                 Tango::telemetry::Configuration::Client{"myClientName"},
//!                                 Tango::telemetry::DEFAULT_GRPC_TRACES_ENDPOINT};
//!
//!        Tango::telemetry::InterfacePtr tti = Tango::telemetry::InterfaceFactory::create(cfg);
//!
//!        {
//!           auto interface_scope = Tango::telemetry::InterfaceScope(tti);
//!           // tti is now the active interface
//!
//!           while (go_on)
//!           {
//!               // generate traces on behalf of the client
//!             ...
//!           }
//!        }
//!
//!        // tti is no longer the active interface
//!        // the previous one is restored (if any, otherwise use the default one)
//!    }
//!
//---------------------------------------------------------------------------------------------------------------------
class InterfaceScope
{
  public:
    InterfaceScope(InterfacePtr active_interface)
    {
        previous_interface = Tango::telemetry::Interface::get_current();

        Tango::telemetry::Interface::set_current(active_interface);
    }

    ~InterfaceScope()
    {
        Tango::telemetry::Interface::set_current(previous_interface);
    }

  private:
    InterfacePtr previous_interface{nullptr};
};

    //-----------------------------------------------------------------------------------------
    // MISC. MACROS FOR DEVICE DEVELOPER
    //-----------------------------------------------------------------------------------------
    //
    //            THE FOLLOWING MACROS NOT ONLY EASE THE USAGE OF THE TELEMETRY API
    //
    //                    **** IT IS HIGHLY RECOMMENDED TO USE THEM ****
    //
    //------------------------------------------------------------------------------------------
    //
    // Usage example:
    //
    // void MyDeviceClass::read_some_attribute(Tango::Attribute &attr)
    //  {
    //     //-----------------------------------------------------------------------------------
    //     // The telemetry interface of the device has been activated by the kernel upon
    //     // reception of a "remote" call. We can consequently generate traces on behalf of
    //     // our device in this context and contribute to the "end-to-end" observability.
    //     // ----------------------------------------------------------------------------------
    //     // Let's create a span then make it the active one using a scope.
    //     // The scope makes use of teh RAII paradigm to control how long the span remains
    //     // active. The active span will be the parent of any span created downstream.
    //     //-----------------------------------------------------------------------------------
    //     auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION);
    //     auto scope = TANGO_TELEMETRY_SCOPE(span);
    //     // -----------------------------------------------------------------------------------
    //     // Now, let's add an attribute (as a key/value pair) to the span. An attribute is
    //     // purely user defined and there is no semantic convention to respect. This a contextual
    //     // information that will be propagated together with the span and that will available
    //     // as a meta-data in the backend (e.g., as a search criteria). We can do so using the
    //     // span object or the TANGO_TELEMETRY_ADD_ATTRIBUTE macro. The key must be a std::string while
    //     // the value must be one of the types supported by Tango::telemetry::AttributeValue.
    //     //-----------------------------------------------------------------------------------
    //     TANGO_TELEMETRY_ADD_ATTRIBUTE("data_update_counter", get_data_update_counter());
    //     try
    //     {
    //          ...
    //          // add an annotations to the span (an event in the OpenTelemetry jargon)
    //          TANGO_TELEMETRY_ADD_EVENT("so far, so good...");
    //          ...
    //          // end the span for precise profiling
    //          // will be ended automatically otherwise when it goes out of scope
    //          span->End();
    //      }
    //      catch(...)
    //      {
    //          // tell the world that an error occurred in the span
    //          auto err_msg = Tango::telemetry::extract_exception_info();
    //          TANGO_TELEMETRY_SET_ERROR_STATUS(err_msg);
    //          throw(...);
    //      }
    //
    //      //------------------------------------------------------------------------
    //      // the span and the scope goes out of (function) scope
    //      //------------------------------------------------------------------------
    //      // when released, the scope restores the previously active span
    //      // when released, the span is automatically ended (if not already ended)
    //      //------------------------------------------------------------------------
    //  }
    //-------------------------------------------------------------------------------

    //-------------------------------------------------------------------------------
    // TANGO_TELEMETRY_ACTIVE_INTERFACE
    //
    // Snapshots which interface is currently attached to the current thread then attaches
    // the specified one. Uses the magic provide by the C++11 "thread_local" keyword.
    // The interface that was initially active is restored when the underlying InterfaceScope
    // goes out of scope (RAII paradigm).
    //
    // Usage example:
    //
    //    void myClient()
    //    {
    //        Tango::telemetry::Configuration cfg{
    //                                 true,
    //                                 false,
    //                                 "Some Diagnostics Application",
    //                                 "diagnostics",
    //                                 Tango::telemetry::Configuration::Client{"myDiagnosticsClientApp"},
    //                                 Tango::telemetry::Configuration::DEFAULT_COLLECTOR_ENDPOINT};
    //
    //        Tango::telemetry::InterfacePtr tti = Tango::telemetry::InterfaceFactory(cfg);
    //
    //        // activate the interface (make it the "current" one for the thread executing this code)
    //        auto interface_scope = TANGO_TELEMETRY_ACTIVE_INTERFACE(tti);
    //
    //        // create the client root span (i.e., the parent of any span created in this thread)
    //        auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION);
    //        auto scope = TANGO_TELEMETRY_SCOPE(span);
    //
    //        while (go_on)
    //        {
    //            // generate traces on behalf of the client
    //            ...
    //        }
    //    }
    //
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_ACTIVE_INTERFACE(TI) Tango::telemetry::InterfaceScope(TI)

    //-------------------------------------------------------------------------------
    // MACRO: TANGO_TELEMETRY_SPAN
    //
    // Start a new span.
    //
    // The "location" (i.e., the file name and the line number) where the scope(i.e., the span)
    // has been created is automatically added to the span attributes.
    //
    // Usage:
    //
    //    auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION);
    //        `-> span name = TANGO_CURRENT_FUNCTION (the recommended span name for consistency reasons)
    //        `-> no Tango::telemetry::Attributes (no 2nd  arg)
    //
    //    auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION, {{"myKey", "myValue"}});
    //        `-> span name = TANGO_CURRENT_FUNCTION (the recommended span name for consistency reasons)
    //        `-> attributes = user defined key/value pairs (as string/AttributeValue pairs)
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_SPAN(...) Tango::telemetry::Interface::get_current()->start_span(__VA_ARGS__)

    //-------------------------------------------------------------------------------
    // MACRO: TANGO_TELEMETRY_SCOPE
    //
    // Start a new scope (i.e., makes the specified span the active one).
    //
    // The "location" (i.e., the file name and the line number) where the scope has been created is automatically
    // added to the span attributes.
    //
    // Usage:
    //    auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION);
    //    auto scope = TANGO_TELEMETRY_SCOPE(span);
    //    ...
    //    span->End();
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_SCOPE(SPAN) Tango::telemetry::Interface::get_current()->scope(SPAN, __FILE__, __LINE__)

    //-------------------------------------------------------------------------------
    // TANGO_TELEMETRY_ADD_EVENT
    //
    // Add an event to the current span.
    //
    // Usage:
    //
    //    TANGO_TELEMETRY_ADD_EVENT("this annotation will be attached to the current span");
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_ADD_EVENT(MSG)                                                        \
      {                                                                                         \
          auto current_span = Tango::telemetry::Interface::get_current() -> get_current_span(); \
          if(current_span)                                                                      \
          {                                                                                     \
              current_span->add_event(MSG);                                                     \
          }                                                                                     \
      }

    //-------------------------------------------------------------------------------
    // TANGO_TELEMETRY_ADD_ATTRIBUTE
    //
    // Add an attribute to the current span.
    //
    // The key must be a std::string while the value must be one of the types supported by
    // Tango::telemetry::AttributeValue.
    //
    // Usage:
    //
    //    TANGO_TELEMETRY_ADD_ATTRIBUTE("somekey", someValue);
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_ADD_ATTRIBUTE(KEY, VALUE)                                             \
      {                                                                                         \
          auto current_span = Tango::telemetry::Interface::get_current() -> get_current_span(); \
          if(current_span)                                                                      \
          {                                                                                     \
              current_span->set_attribute(KEY, VALUE);                                          \
          }                                                                                     \
      }

    //-------------------------------------------------------------------------------
    // TANGO_TELEMETRY_SET_ERROR_STATUS
    //
    // Set the error status of the current span.
    //
    // This is typically used in error/exception context.
    //
    // Usage:
    //
    //    try
    //    {
    //       do_some_job();
    //    }
    //    catch (...)
    //    {
    //      TANGO_TELEMETRY_SET_ERROR_STATUS("oops, an error occurred!");
    //    }
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_SET_ERROR_STATUS(DESC)                                                \
      {                                                                                         \
          auto current_span = Tango::telemetry::Interface::get_current() -> get_current_span(); \
          if(current_span->get_status() != Tango::telemetry::Span::Status::kError)              \
          {                                                                                     \
              current_span->set_status(Tango::telemetry::Span::Status::kError, DESC);           \
          }                                                                                     \
      }

} // namespace Tango::telemetry

#endif // TANGO_USE_TELEMETRY

#endif // TANGO_COMMON_TELEMETRY_H
