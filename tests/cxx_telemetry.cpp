// NOLINTBEGIN(*)

#ifndef TelemetryTestSuite_h
  #define TelemetryTestSuite_h

  #include "cxx_common.h"

  #undef SUITE_NAME
  #define SUITE_NAME TelemetryTestSuite

  #if defined(TANGO_USE_TELEMETRY)
    #include <opentelemetry/context/context.h>
    #include <opentelemetry/sdk/version/version.h>
    #include <opentelemetry/trace/provider.h>
    #include <opentelemetry/trace/noop.h>
    #include <opentelemetry/sdk/version/version.h>
    #include <opentelemetry/context/propagation/global_propagator.h>
    #include <opentelemetry/trace/propagation/http_trace_context.h>
    #include <opentelemetry/trace/context.h>

    #if defined(TANGO_TELEMETRY_USE_GRPC)
      #include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>
    #endif
    #if defined(TANGO_TELEMETRY_USE_HTTP)
      #include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
    #endif

    #include <opentelemetry/sdk/resource/resource.h>
    #include <opentelemetry/sdk/trace/processor.h>
    #include <opentelemetry/sdk/trace/simple_processor_factory.h>
    #include <opentelemetry/sdk/trace/tracer_provider.h>
    #include <opentelemetry/sdk/trace/tracer_provider_factory.h>
    #include <opentelemetry/trace/span_context.h>
    #include <opentelemetry/trace/provider.h>
  #endif

class SUITE_NAME : public CxxTest::TestSuite
{
  protected:
  public:
    SUITE_NAME()
    {
        //
        // Arguments check -------------------------------------------------
        //

        CxxTest::TangoPrinter::validate_args();

        //
        // Initialization --------------------------------------------------
        //
    }

    virtual ~SUITE_NAME() = default;

    static SUITE_NAME *createSuite()
    {
        return new SUITE_NAME();
    }

    static void destroySuite(SUITE_NAME *suite)
    {
        delete suite;
    }

    //
    // Tests -------------------------------------------------------
    //

    void test_telemetry_noop_tracer()
    {
  #if defined(TANGO_USE_TELEMETRY)
        // noop tracer
        auto noop_tracer =
            opentelemetry::nostd::shared_ptr<opentelemetry::trace::NoopTracer>(new opentelemetry::trace::NoopTracer);

        {
            // a span
            auto span = noop_tracer->StartSpan("test_telemetry_noop_tracer_span",
                                               opentelemetry::common::NoopKeyValueIterable(),
                                               opentelemetry::trace::NullSpanContext(),
                                               {});

            // a scope that makes the span the active one (RAII)
            auto scope = opentelemetry::trace::Scope(span);
        }
  #else
        std::cout << "TANGO_USE_TELEMETRY is set to FALSE (noop test)" << std::endl;
  #endif
    }
};
#endif // TelemetryTestSuite_h

// NOLINTEND(*)
