## Getting started with OpenTelemetry and cppTango

cppTango can be built with or without telemetry support, this is
controlled by the `TANGO_USE_TELEMETRY` CMake option. By default, cppTango
is compiled with telemetry support. See INSTALL.md for more detailed
instructions about how to build cppTango with telemetry enabled.

When cppTango has been compiled with telemetry support, it can be enabled at
runtime using the environment variable `TANGO_TELEMETRY_ENABLE`, this must be
set to `on` to enable telemetry. By default, telemetry is not enabled at
runtime. For example,

```shell
$ TANGO_TELEMETRY_ENABLE=on TangoTest test
```

By default, this will output telemetry data to stdout. This telemetry data
consists of a series of "exports", each describing something which has happened
inside the process. During device server startup of TangoTest you will see a
span export for the call to the `DbGetDataForServerCache` command:

```shell
{
  name          : virtual CORBA::Any_var Tango::Connection::command_inout(const std::string&, const CORBA::Any&)
  trace_id      : 45ebbe9071f4b3818b7a77fa84661076
  span_id       : 104d2d45d32dd934
  tracestate    :
  parent_span_id: 0000000000000000
  start         : 1727180999748689364
  duration      : 20102985
  description   :
  span kind     : Client
  status        : Unset
  attributes    :
        thread.id: 140508933565632
        code.lineno: 1492
        code.filepath: devapi_base.cpp
        tango.operation.target: sys/database/2
        tango.operation.argument: DbGetDataForServerCache
  events        :
  links         :
  resources     :
        telemetry.sdk.version: 1.15.0
        telemetry.sdk.name: opentelemetry
        telemetry.sdk.language: cpp
        tango.host: localhost:10000
        tango.process.kind: server
        tango.process.id: 103275
        service.name: tango.telemetry.default
        service.namespace: tango
  instr-lib     : tango.cpp-10.0.0
}
```

If telemetry has also been enabled at runtime for DatabaseDS we will see a
corresponding export for the call, for example:

```shell
{
  name          : virtual CORBA::Any* Tango::Device_4Impl::command_inout_4(const char*, const CORBA::Any&, Tango::DevSource, const Tango::ClntIdent&)
  trace_id      : 45ebbe9071f4b3818b7a77fa84661076
  span_id       : b09e260763fefa8d
  tracestate    :
  parent_span_id: 104d2d45d32dd934
  start         : 1727180999749113378
  duration      : 19416511
  description   :
  span kind     : Server
  status        : Unset
  attributes    :
        tango.operation.argument: DbGetDataForServerCache
  events        :
  links         :
  resources     :
        telemetry.sdk.version: 1.15.0
        telemetry.sdk.language: cpp
        tango.host: localhost:10000
        tango.process.kind: server
        telemetry.sdk.name: opentelemetry
        service.instance.id: sys/database/2
        tango.process.id: 103225
        service.name: DataBase
        tango.server.name: Databaseds/2
        service.namespace: tango
  instr-lib     : tango.cpp-10.0.0
}
```

Notice here that the `trace_id` (45ebbe9071f4b3818b7a77fa84661076) matches for
each export, meaning they are part of the same trace. Also, the `parent_span_id`
(104d2d45d32dd934) of the DatabaseDS event matches the `span_id` of the
TangoTest event, meaning that the former is a subspan of the latter.

## Using a OpenTelemetry collector

Typically, making sense of the telemetry events that are emitted to stdout is
not possible and instead you will need a collector to organise the various
events and present them in a human digestible manner on a web page. These
collectors are separate services which must be deployed in order to collect the
various telemetry events.

cppTango can be compiled with or without a gRPC or HTTP exporter, this is
controlled by the `TANGO_TELEMETRY_USE_GRPC` and `TANGO_TELEMETRY_USE_HTTP` CMake
options. Both of these are enabled by default if cppTango is compiled with
telemetry support (`-DTANGO_USE_TELEMETRY=ON`).

At runtime, exporters can be selected for traces and logs using the
following environment variables:

- `TANGO_TELEMETRY_TRACES_EXPORTER`
- `TANGO_TELEMETRY_TRACES_ENDPOINT`
- `TANGO_TELEMETRY_LOGS_EXPORTER`
- `TANGO_TELEMETRY_LOGS_ENDPOINT`

Each of the `TANGO_TELEMETRY_<X>_EXPORTER` environment variables can be one of:

- `console`
- `grpc`
- `http`
- `none`

The default is `console`. `none` can be used to e.g. disable emitted events for
only logs, which might be useful if you have some other system for handling
logs.

If a `TANGO_TELEMETRY_<X>_EXPORTER` is either `grpc` or `http` then a
corresponding `TANGO_TELEMETRY_<X>_ENDPOINT` is required. For `grpc` the
endpoint must begin with `grpc://` and for `http` it must begin with either
`http://` or `https://`. For example, to use a local gRPC collector you might
set the following environment variables:

```shell
$ export TANGO_TELEMETRY_ENABLE=on
$ export TANGO_TELEMETRY_TRACES_EXPORTER=grpc
$ export TANGO_TELEMETRY_TRACES_ENDPOINT=grpc://localhost:4317
$ export TANGO_TELEMETRY_LOGS_EXPORTER=grpc
$ export TANGO_TELEMETRY_LOGS_ENDPOINT=grpc://localhost:4317
```

\warning

Emitting telemetry events from a large number of device servers and clients
an generate a high load on the backend receiving this data. There is also a
small impact on the Tango servers and clients. Be careful when enabling this
feature, and monitor the performance impact. See these
[benchmarks](https://gitlab.com/tango-controls/TangoTickets/-/issues/109).

### Adding custom telemetry spans

Custom telemetry spans can be added to a device class using the
\ref TANGO_TELEMETRY_SPAN and \ref TANGO_TELEMETRY_SCOPE macros.  A span can be
started using \ref TANGO_TELEMETRY_SPAN and then activated with \ref
TANGO_TELEMETRY_SCOPE.

When a span is active any logs (emitted with e.g. \ref TANGO_LOG_DEBUG) or any
telemetry events (emitted with TANGO_TELEMETRY_ADD_EVENT) are associated with
the active span.  A time stamp is recorded any time a span is started or ended
so that the duration of the span can be calculated.

When the \ref Tango::telemetry::Scope object returned by \ref
TANGO_TELEMETRY_SCOPE goes out of (function) scope, the span will no longer be
active and the previously active span will be restored.  When the \ref
Tango::telemetry::Span object returned by \ref TANGO_TELEMETRY_SPAN goes out of
(function) scope, the span is ended.

For example, to add an additional span to your read attribute callback and
activate it:

```cpp
    void read_attribute(Tango::Attribute &att)
    {
        // The span is started here.
        auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION, {{"myKey", "myValue"}});

        // The span is activated here, the previously active span is saved.
        auto scope = TANGO_TELEMETRY_SCOPE(span);

        attr_value = get_my_attribute_value();
        att.set_value_date_quality(&attr_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);

        // The span is deactivated in scope's dtor, the previously active span
        // is made active again.

        // The span is ended in span's dtor.
    }
```
