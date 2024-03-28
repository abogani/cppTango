#ifndef TANGO_COMMON_TELEMETRY_KERNEL_MACROS_H
#define TANGO_COMMON_TELEMETRY_KERNEL_MACROS_H

#if defined(TANGO_USE_TELEMETRY)

//---------------------------------------------------------------------------------------------------------------------
// Tango::telemetry::SilentKernelScope
//
// Disable the kernel traces for the current interface and for the current C++ scope. This allows to hide unwanted
// kernel traces and prevents flooding the backend with useless traces. One could ask why we need this and argue that
// we simply have to remove the trace assertions where we don't want them. That's not so simple because it depends on
// the "context" in which these traces are created. For instance the constructor of the Tango::DeviceProxy would
// systematically generates 4 traces. There are some situations (e.g., a problem analysis) for which we want to see
// these traces but, in the majority of the cases, we don't nedd them. That's why we provide an on demand activation
// of the kernel traces.
//---------------------------------------------------------------------------------------------------------------------
class SilentKernelScope
{
  public:
    SilentKernelScope()
    {
        interface = Tango::telemetry::Interface::get_current();
        interface_temporary_disabled = interface->is_enabled() && interface->are_kernel_traces_disabled();
        if(interface_temporary_disabled)
        {
            interface->disable();
        }
    }

    ~SilentKernelScope()
    {
        if(interface_temporary_disabled)
        {
            interface->enable();
        }
    }

  private:
    bool interface_temporary_disabled{false};
    Tango::telemetry::InterfacePtr interface{nullptr};
};

    //-----------------------------------------------------------------------------------------------------------------
    // TANGO_TELEMETRY_SILENT_KERNEL_SCOPE- see SilentKernelScope for details.
    //
    // Usage:
    //
    //    auto silent_kernel_scope = TANGO_TELEMETRY_SILENT_KERNEL_SCOPE();
    //-----------------------------------------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_SILENT_KERNEL_SCOPE SilentKernelScope()

    //-----------------------------------------------------------------------------------------------------------------
    // TANGO_TELEMETRY_KERNEL_CLIENT_SPAN & TANGO_TELEMETRY_KERNEL_CLIENT_SPAN  (FOR CPP KERNEL ONLY)
    //
    // Start a new "client" span.
    //
    // For Tango kernel internal usage only. This is used by the Tango::DeviceProxy to initiate a client RPC.
    //
    // Usage:
    //
    //    auto span = TANGO_TELEMETRY_KERNEL_CLIENT_SPAN();
    //    TANGO_TELEMETRY_KERNEL_CLIENT_SPAN(some_arg_name);
    //-----------------------------------------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_KERNEL_CLIENT_SPAN(ATTRS)           \
      Tango::telemetry::Interface::get_current()->start_span( \
          TANGO_CURRENT_FUNCTION, ATTRS, Tango::telemetry::Span::Kind::kClient)

    //-----------------------------------------------------------------------------------------------------------------
    // TANGO_TELEMETRY_KERNEL_SERVER_SPAN (FOR CPP KERNEL ONLY)
    //
    // Start a new "server" span.
    //
    // For Tango kernel internal usage only. This is used by the several flavors of Tango::DeviceImpl to initiate a
    // reply to a client RPC.
    //
    // Usage:
    //    TANGO_TELEMETRY_KERNEL_SERVER_SPAN;
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_KERNEL_SERVER_SPAN(...) telemetry()->set_trace_context(__VA_ARGS__)

    //-------------------------------------------------------------------------------
    // TANGO_TELEMETRY_TRY (FOR CPP KERNEL ONLY)
    //-------------------------------------------------------------------------------
  #define TANGO_TELEMETRY_TRY \
      try                     \
      {
  //---------------------------------------------------------------------------------
  // TANGO_TELEMETRY_TRY (FOR CPP KERNEL ONLY)
  //---------------------------------------------------------------------------------
  // see otel. specs for details
  // https://github.com/open-telemetry/semantic-conventions/blob/main/docs/exceptions/exceptions-spans.md#semantic-conventions-for-exceptions-on-spans
  #define TANGO_TELEMETRY_CATCH                                                                                \
      }                                                                                                        \
      catch(...)                                                                                               \
      {                                                                                                        \
          std::string type;                                                                                    \
          std::string msg;                                                                                     \
          Tango::telemetry::Interface::extract_exception_info(type, msg);                                      \
          auto current_span = Tango::telemetry::Interface::get_current() -> get_current_span();                \
          current_span->add_event("exception caught", {{"exception.type", type}, {"exception.message", msg}}); \
          TANGO_TELEMETRY_SET_ERROR_STATUS("exception caught (see associated event)");                         \
          throw;                                                                                               \
      }

#endif // TANGO_USE_TELEMETRY

#endif // TANGO_COMMON_TELEMETRY_KERNEL_MACROS_H
