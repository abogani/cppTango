## Event system monitoring with QueryEventSystem()

cppTango DServer devices provide two additional commands `QueryEventSystem()` and
`EnableEventSystemPerfMon()` to allow users to inspect the ZMQ event system and
optionally monitor its performance.  This is intended to aid in debugging Tango
devices making heavy use of the Tango event system.

The `QueryEventSystem()` command returns a `DevString` holding a JSON encoded
object.  This document describes the keys and values found in this JSON
object. The `QueryEventSystem()` command exposes internals of the cppTango event
system and as such the data provided by this command may change in the future.
In particular, all the keys listed for JSON objects described here should not be
considered exhaustive.

The `QueryEventSystem()` command provides information about both the ZMQ events
sent by the device server and the ZMQ events received by the server.  The query
response includes data being kept by the Tango kernel into order to service
events and, optionally, includes performance data about the most recent
events that have been sent and received by the device server (up to 256 of
each).

The `EnableEventSystemPerfMon()` command accepts a single `DevBoolean` argument,
which, if true, will turn on the performance monitoring on for both the event
consumer and event supplier.  This performance monitoring has a small
performance penalty for the event system.

### Event system inspection with QueryEventSystem()

The `QueryEventSystem()` command returns a JSON encoded object which always has
three keys, `"server"` and `"client"`, which hold the `server` object and `client` object
respectively; and `"version"` which is currently always `1`.  Future releases of
cppTango may change the layout of this JSON object, in which case the
`"version"` key will be incremented.

- The `server` object may be `null` if the device server is not setup to send events
  via the ZMQ event system. Otherwise, it is a JSON object with data describing
  the state of the `ZmqEventSupplier` object, which is responsible for sending ZMQ
  events from the device server.
- The `client` object is `null` if the device server process has never subscribed to
  an event. Otherwise, it is a JSON object with data describing the state of the
  `ZmqEventConsumer`, which is responsible for managing Tango event subscriptions.

*Example Response*

Below is the possible output of `QueryEventSystem()` for a device server with
performance monitoring enabled.  This device server has a single device
`"foo/bar/mid"`, which is subscribing to two attributes and is publishing events
for two attributes.  The events being published are:

- `CHANGE_EVENT` for `fluxCapacity`
- `ARCHIVE_EVENT` for `rainFall`

The events being received are:

- `CHANGE_EVENT` for `achievedPointing`
- `ARCHIVE_EVENT` for `coolFactor`

```json
{
  "version": 1,
  "server": {
    "event_counters": {
      "tango://build-pytango-wheel-tango-dbds-1.tango-net:10000/foo/bar/mid/fluxcapacity.change": 2744,
      "tango://build-pytango-wheel-tango-dbds-1.tango-net:10000/foo/bar/mid/rainfall.archive": 550
    },
    "perf": [
      {
        "micros_since_last_event": 199214,
        "push_event_micros": 46
      },
      {
        "micros_since_last_event": 46,
        "push_event_micros": 4
      },
      {
        "micros_since_last_event": 100275,
        "push_event_micros": 54
      },
      {
        "micros_since_last_event": 54,
        "push_event_micros": 4
      },
      {
        "micros_since_last_event": 100297,
        "push_event_micros": 65
      },
      {
        "micros_since_last_event": 65,
        "push_event_micros": 4
      }
    ]
  },
  "client": {
    "event_callbacks": {
      "tango://build-pytango-wheel-tango-dbds-1.tango-net:10000/foo/bar/pub/achievedpointing.idl5_change": {
        "channel_name": "tango://build-pytango-wheel-tango-dbds-1.tango-net:10000/dserver/myserver/1",
        "callback_count": 1,
        "server_counter": 2713,
        "event_count": 2713,
        "missed_event_count": 0,
        "discarded_event_count": 1703,
        "last_resubscribed": "2025-04-08T12:36:33"
      },
      "tango://build-pytango-wheel-tango-dbds-1.tango-net:10000/foo/bar/pub/coolfactor.idl5_archive": {
        "channel_name": "tango://build-pytango-wheel-tango-dbds-1.tango-net:10000/dserver/myserver/1",
        "callback_count": 1,
        "server_counter": 1357,
        "event_count": 1357,
        "missed_event_count": 0,
        "discarded_event_count": 850,
        "last_resubscribed": "2025-04-08T12:36:33"
      }
    },
    "not_connected": [],
    "event_channels": {
      "tango://build-pytango-wheel-tango-dbds-1.tango-net:10000/dserver/myserver/1": {
        "endpoint": "tcp://172.18.0.4:36879"
      }
    },
    "perf": [
      {
        "attr_name": "",
        "micros_since_last_event": 200159,
        "sleep_micros": 200032,
        "process_micros": 30,
        "first_callback_latency_micros": null,
        "callback_count": 0,
        "wake_count": 1,
        "discarded": true
      },
      {
        "attr_name": "achievedpointing",
        "micros_since_last_event": 35,
        "sleep_micros": 3,
        "process_micros": 123,
        "first_callback_latency_micros": 215,
        "callback_count": 1,
        "wake_count": 1,
        "discarded": false
      },
      {
        "attr_name": "",
        "micros_since_last_event": 100206,
        "sleep_micros": 100082,
        "process_micros": 23,
        "first_callback_latency_micros": null,
        "callback_count": 0,
        "wake_count": 1,
        "discarded": true
      },
      {
        "attr_name": "coolfactor",
        "micros_since_last_event": 27,
        "sleep_micros": 2,
        "process_micros": 125,
        "first_callback_latency_micros": 222,
        "callback_count": 1,
        "wake_count": 1,
        "discarded": false
      },
      {
        "attr_name": "",
        "micros_since_last_event": 100225,
        "sleep_micros": 100100,
        "process_micros": 29,
        "first_callback_latency_micros": null,
        "callback_count": 0,
        "wake_count": 1,
        "discarded": true
      },
      {
        "attr_name": "achievedpointing",
        "micros_since_last_event": 33,
        "sleep_micros": 2,
        "process_micros": 112,
        "first_callback_latency_micros": 234,
        "callback_count": 1,
        "wake_count": 1,
        "discarded": false
      }
    ]
  }
}
```
#### The server object -- ZmqEventSupplier data

The `server` object has two keys:
- `"event_counters"`, holding the `event_counter_map`.
- `"perf"`

The keys of the `event_counter_map` are fully qualified event stream names and
the values are the number of events that have been pushed on that stream.  For
attribute value events, a event stream is an `(Attribute, EventType)` pair that
some client has subscribed to.  If all the clients subscribing to this event
stream stop periodically resubscribing to the event stream, the event stream
will be removed from the `event_counter_map`.  If a client later subscribes to
the same event stream after it has been removed from the map, the counter will
be reset.

If event system performance monitoring has not been enabled with the
`EnableEventSystemPerfMon()` command, the `"perf"` key will hold `null.`  Performance
monitoring is discussed in the next section.

#### The client object -- ZmqEventConsumer data

The client object has three keys:
- `"event_callbacks"`, holding the `event_callback_map`
- `"not_connected"`, holding an array of `not_conncted_objects`
- `"event_channels"`, holding the `event_channel_map`
- `"perf"`

The keys of the `event_callback_map` are event topics and the values are
`event_callback` objects.  For attribute value events, an "event topic" is an
`(Attribute, EventType, CompabilityIDLVersion)` triple.  This is the equivalent
to the "event stream name" for the server object's `event_counter_map`, however,
for the `event_callback_map` we also include the compatibility IDL version.
This difference is an implementation detail of the current ZMQ event system that
is exposed by the `QueryEventSystem()` command.

The `event_callback` objects have the following keys:
- `"channel_name"` - Name of the device server supplying the event topic
- `"callback_count"` - Number of user callbacks interested in the event topic
- `"server_counter"` - The value of the last counter received from the device
  server, this corresponds to the values in `event_counter_map` in the server
  object.
- `"event_count"` - The number of events the device server has received and not
  discarded for this event topic
- `"missed_event_count"` - The number of the times the `ZmqEventConsumer` has
  detected that there are missed for this event topic.
- `"discarded_event_count"` - The number of events discarded for this topic.
- `"last_resubscribed"` - A string holing the last time the device server
  resubscribed to this topic, or `null` if the `ZmqEventConsumer` is still using the
  initial subscription.

Each `not_connected_object` represents a event stream that the
`ZmqEventConsumer` is attempt to re-connect to.  Each object has the following
keys:
- `"device"` - The name of the Tango device attempting to be connected to
- `"attribute"` - The name of the attribute associated with an event
- `"event_type"` - The name of the type of the event
- `"last_heartbeat"` - The time of the last attempt to connect to the device
- `"tango_host"` - The `TANGO_HOST` used to locate the device or `null` if `dbase=no`

The keys of the `event_channel_map` are device servers supplying events to the
`ZmqEventConsumer` and the values are `event_channel` objects.  The keys here
correspond to the `"channel_name"` key from the event_callback object.  For each
`"channel_name"` found in an `event_callback` object, there is a corresponding entry
in the `event_channel_map`.

Each `event_channel` object holds a single `"endpoint"` key, which holds a string
containing the ZMQ endpoint the `ZmqEventConsumer` connects to in order to receive
events from this device server.

If event system performance monitoring has not been enabled with the
`EnableEventSystemPerfMon()` command, the `"perf"` key will hold `null`. Performance
monitoring is discussed in the next section.

### Event system performance monitoring with QueryEventSystem()

When event system performance monitoring has been enabled, by invoking
the `EnableEventSystemPerfMon()` command with a true argument, the `"perf"` keys of
the server and client objects will each hold a JSON array.  The JSON array
contains `server_perf_sample` objects and `client_perf_sample` objects
respectively.  Each array will hold at most 256 samples.

Each time `QueryEventSystem()` is called, the buffers holding
`server_perf_sample`s and `client_perf_sample`s are cleared so that subsequent
calls to the `QueryEventSystem()`, for any client, will not see these events again.

#### server_perf_sample objects

Each `server_perf_sample` object has the following keys:
- `"micros_since_last_event"` - Microseconds since the last event was sampled,
  or `null` if this is the first event sampled since performance monitoring was
  enabled.
- `"push_event_micros"` - Microseconds it took for the event to be handed off to
  ZMQ.

Note, `server_per_sample` objects are only generated for events which make it to
the `ZmqEventSupplier`.  This means if the event is discarded before this point,
because, for example, no clients have subscribed to the event or the value does
not surpass the change threshold for the attribute, no sample will be generated
for this event.

*Example server_perf_sample object*

```json
{
  "micros_since_last_event": null,
  "push_event_micros": 61
}
```

#### client_perf_sample objects

Each `client_perf_sample` object has the following keys:
- `"attr_name"` - First 31 characters of the name of the attribute, or an empty
  sting if the event is not associated with an attribute or the event was
  discarded.
- `"micros_since_last_event"` - Microseconds since the last event was sampled,
  or `null` if this is the first event sampled since performance monitoring was
  enabled.
- `"sleep_micros"` - Microseconds the `ZmqEventConsumer` thread slept before
  receiving this event.
- `"process_micros"` - Microseconds taken to process the event.  This includes
  calling all the user callbacks for this event topic.
- `"first_callback_latency_micros"` - Microseconds between the event being sent
  by the server and being processed by the first callback for this user
  callback.  This is `null` if there is no attribute value associated with the
  event or the event was discarded.
- `"callback_count"` - Number of user callbacks called to process this event.
- `"wake_count"` - Number of times the `ZmqEventConsumer` thread woke while wait
  for this event.
- `"discarded"` - True if the event was discarded before processing.

Note, the `first_callback_latency_micros` uses the time stamp provided by the
server to compute the latency.  There are two potential issues with this:
1. This time stamp is not necessarily synchronised with time stamps generated by
   the client process, so this value may have a systematic bias.
2. The time stamp can be provided by the user by, for example, calling
   `set_value_date_quality()`.  As such, this time stamp might not correspond to
   the event being pushed.

*Example client_perf_sample object*

```json
{
  "attr_name": "attr#dbase=no",
  "micros_since_last_event": 11,
  "sleep_micros": 4,
  "process_micros": 79,
  "first_callback_latency_micros": 318,
  "callback_count": 1,
  "wake_count": 1,
  "discarded": false
}
```
