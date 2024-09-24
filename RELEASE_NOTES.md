<!-- Vim regex for creating markdown links from plain issue links: '<,'>s;, \(https://.\{-}\([0-9]\+\)\); [#\2](\1);g -->

# Tango C++ library 10.0.0 Release Notes

September 20th, 2024

Table of Contents
=================
  - [What's New (Since Version 9.5.0)?](#whats-new-since-version-950)
  - [10.0.0 - Bug Fixes](#1000-bug-fixes)
  - [10.0.0 - Changes](#1000-changes)
  - [10.0.0 - Source Code Comparison With Version 9.5.0](#1000-source-code-comparison-with-version-950)
  - [10.0.0 - Milestone](#1000-milestone)
  - [10.0.0 - Feedback](#1000-feedback)
  - [10.0.0 - Contributions](#1000-contributions)
  - [10.0.0 - Acknowledgement](#1000-acknowledgement)

## What's New Since Version 9.5.0?

This cppTango release is a major release. It contains new features, uses a new
IDL version and has improvements in several areas that are relevant to the
users of cppTango. Please refer to [features](#1000-features), [improvements
and changes](#1000-changes) and [the selection of fixes](#1000-bug-fixes)
where we highlight what was improved or modified.

## 10.0.0 - Features

### Experimental support for client/server tracing using OpenTelemetry [#1185](https://gitlab.com/tango-controls/cppTango/-/issues/1185), [#1323](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1323)

cppTango 10.0.0 provides telemetry support for all device servers using IDLv4 or
later and works on Linux, MacOSX and Windows.  For a description of the various
concepts introduce by telemetry see the [OpenTelemerty
website](https://opentelemetry.io/).  For more information about using telemetry
with cppTango see [Telemetry Support](@ref telemetry) in the documentation.

### New Alarm Event [#1030](https://gitlab.com/tango-controls/cppTango/-/issues/1030)

cppTango 10.0.0 introduces support for the new alarm events.  Alarm events can
be thought of as a subset of change events, only events which involve a
transition to or from the `Tango::ATTR_ALARM` or `Tango::ATTR_WARNING` quality
factors.

The polling thread will send alarm events for all polled attributes to any
clients that subscribe to alarm events with a call to
`DeviceProxy::subscribe_event()`, passing `Tango::ALARM_EVENT`.

Devices which manually push change events will automatically also push alarm
events to clients which have subscribed to alarm events, unless the manual
pushing of alarm events has been enabled for the attribute with
`Tango::Attr::set_alarm_event()` or the `CtrlSystem` property
`AutoAlarmOnChangeEvent` is defined and falsey.

### Extended version information [#1155](https://gitlab.com/tango-controls/cppTango/-/issues/1155)

cppTango 10.0.0 introduces support for more detailed version information which
is also extendable by users.  Servers can add key/value pairs to the version
information by calling `DeviceImpl::add_version_info()`.  cppTango 10.0.0
adds version information about itself and the various dependencies it was
compiled against.

Clients can request this information with a call to `DeviceProxy::info()`
where the assigned key/value pairs will be available in the
`Tango::_DeviceInfo::version_info` member.

### Other features

- Introduce `Tango::Device_6Impl` class with support for new Tango IDLv6 features
- `TANGO_BASE_CLASS` now points to `Tango::Device_6Impl`
- Require at least C++17 for compilation [#1176](https://gitlab.com/tango-controls/cppTango/-/issues/1176)
- Provide a cmake package configuration file on Linux/MacOSX/Windows [#857](https://gitlab.com/tango-controls/cppTango/-/issues/857), [#1277](https://gitlab.com/tango-controls/cppTango/-/issues/1277)
- Add new test framework catch2, version 3.3.0 or newer, for faster and
  dockerless testing supports FileDatabase, environment variables and runs on
  Linux/MacOSX/Windows [#870](https://gitlab.com/tango-controls/cppTango/-/issues/870), [#1204](https://gitlab.com/tango-controls/cppTango/-/issues/1204).

## 10.0.0 - Breaking Changes

- Remove the QUALITY_EVENT enum member from EVENT_TYPE [#1260](https://gitlab.com/tango-controls/cppTango/-/issues/1260)

## 10.0.0 - Other Changes

- Run existing (pre-catch2) tests with older IDLv5 in addition to the current IDL version [#1036](https://gitlab.com/tango-controls/cppTango/-/issues/1036)
- Implement DbPutProperty, DbDeleteProperty functions for free properties in the FileDatabase
- Add support for device servers in dynamic libraries on MacOSX [#1244](https://gitlab.com/tango-controls/cppTango/-/issues/1244)
- Add cmake presets [#1226](https://gitlab.com/tango-controls/cppTango/-/issues/1226)
- Make the tests also work with the static cppTango library [#1213](https://gitlab.com/tango-controls/cppTango/-/issues/1213)
- Introduce developer documentation for exception vs assert [#1041](https://gitlab.com/tango-controls/cppTango/-/issues/1041)
- Add CI with Ubuntu 22.04/24.04 LTS versions [#1279](https://gitlab.com/tango-controls/cppTango/-/issues/1279), [#1280](https://gitlab.com/tango-controls/cppTango/-/issues/1280)
- Update GCC version to 14.1 for testing [#1281](https://gitlab.com/tango-controls/cppTango/-/issues/1281)
- Switch to MSVC 2022 as compiler on Windows [#1282](https://gitlab.com/tango-controls/cppTango/-/issues/1282)
- Make passing UBSAN in CI required [#1175](https://gitlab.com/tango-controls/cppTango/-/issues/1175)
- Add compilation testing CI with C++20/23 [#1283](https://gitlab.com/tango-controls/cppTango/-/issues/1283)
- Prefer std::chrono over time(NULL) for accuracy reasons [#1080](https://gitlab.com/tango-controls/cppTango/-/issues/1080)
- Include `-otel` as part of the windows package name if included [#1284](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1284)
- Improve error messages when encountering a TangoMonitor timeout [#1215](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1215)
- Include the minor version in the SOVERSION [#1285](https://gitlab.com/tango-controls/cppTango/-/issues/1285)
- Improve core-dump reporting for Catch2Tests [#1296](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1296)
- Update CI to use LLVM 18 [#1241](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1241)
- Require tangoidl 6.0.2 and check for this at configure time [#1228](https://gitlab.com/tango-controls/cppTango/-/issues/1228)
- Run Catch2Tests on Windows [#1223](https://gitlab.com/tango-controls/cppTango/-/issues/1223)
- Install minimum required binaries when building Windows packages [#1290](https://gitlab.com/tango-controls/cppTango/-/issues/1290)
- Publish multi-version documentation, available [here](https://tango-controls.gitlab.io/cppTango/) [#890](https://gitlab.com/tango-controls/cppTango/-/issues/890)
- Improve install instructions [#1298](https://gitlab.com/tango-controls/cppTango/-/issues/1298)
- Allow superprojects to override searching for tangoidl [#1318](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1318)
- Improve set_value error message if data type is wrong [#1306](https://gitlab.com/tango-controls/cppTango/-/issues/1306)
- Reduced code duplication for Attribute::fire_<>_event code [#1317](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1317)
- DServer::create_cpp_class now looks for filenames with user provided prefixes [#1245](https://gitlab.com/tango-controls/cppTango/-/issues/1245)

## 10.0.0 - Bug fixes

- Fix some memory leaks in the FileDatabase [#1261](https://gitlab.com/tango-controls/cppTango/-/issues/1261)
- Avoid crash when listing the number of device classes where the admin device was deleted before [#1243](https://gitlab.com/tango-controls/cppTango/-/issues/1243)
- Fix segfault during shutdown [#1218](https://gitlab.com/tango-controls/cppTango/-/issues/1218)
- Fix a potential crash when running without Tango Database [#1080](https://gitlab.com/tango-controls/cppTango/-/issues/1080)
- Fix crash with forwarded attributes for Spectrum and Image types [#1018](https://gitlab.com/tango-controls/cppTango/-/issues/1018)
- Potential crash at server init when a device server uses groups in a thread [#867](https://gitlab.com/tango-controls/cppTango/-/issues/867)
- Fix attribute polling in nodb mode [#1220](https://gitlab.com/tango-controls/cppTango/-/issues/1220)
- Fix edge case in event handling in nodb mode
- Fix Attribute::set_write_value when called with a std::string [#1256](https://gitlab.com/tango-controls/cppTango/-/issues/1256)
- Fix segfault when calling check_alarm [#615](https://gitlab.com/tango-controls/pytango/-/issues/615)
- Fix order of shutting down threads [#1278](https://gitlab.com/tango-controls/cppTango/-/issues/1278)
- Fix change/archive event detection when transitioning to/from NaN [#1207](https://gitlab.com/tango-controls/cppTango/-/issues/1207)
- Fix Attribute::check_alarm() to always return true if attribute in ALARM/WARNIGN [#1182](https://gitlab.com/tango-controls/cppTango/-/issues/1182)
- Fix memory leak when pushing event from READ_WRITE attribute [#1292](https://gitlab.com/tango-controls/cppTango/-/issues/1292)
- Fix memory leak when using set_value with release=true and ATTR_INVALID [#1265](https://gitlab.com/tango-controls/cppTango/-/issues/1265)
- Fix linker warnings on Windows CI [#1302](https://gitlab.com/tango-controls/cppTango/-/issues/1302)
- Fix State evaluation can raise segfault when exception raised during attribute read [#1311](https://gitlab.com/tango-controls/cppTango/-/issues/1311)

## 10.0.0 - Source Code Comparison With Version 9.5.0

You can view the source code comparison with Tango 9.5.0, as well as the
commits and contributors on this page:
https://gitlab.com/tango-controls/cppTango/-/compare/9.5.0...10.0.0

## 10.0.0 - Milestone

All the issues and Merge Requests associated to the 10.0.0 milestone can be
found here: https://gitlab.com/tango-controls/cppTango/-/milestones/10.0.0

## 10.0.0 - Feedback

You can report issues on https://gitlab.com/tango-controls/cppTango/-/issues.
If you find a problem which you think could also affect Tango Controls packages
other than cppTango, e.g. cppTango and pyTango, or when you don't know where
you should create the issue at all - Well, you know now! - then head over to
https://gitlab.com/tango-controls/TangoTickets/-/issues.

## 10.0.0 - Contributions

Contributions are always welcome! Please do not hesitate to create new Merge
Requests in [cppTango Gitlab
repository](https://gitlab.com/tango-controls/cppTango). Please refer to
[CONTRIBUTING.md](https://gitlab.com/tango-controls/cppTango/-/blob/main/CONTRIBUTING.md)
to familiarise yourself with the easiest way to contribute to the cppTango
project.

## 10.0.0 - Acknowledgement

Many thanks to all the persons who contributed to this release, to the Tango
kernel team and to the Tango community for its ideas, feedback, bug reports and
tests.

# Tango C++ library 9.5.0 Release Notes

October 10th, 2023

Table of Contents
=================
  - [What's New (Since Version 9.4.2)?](#whats-new-since-version-942)
  - [9.5.0 - Bug Fixes](#950-bug-fixes)
  - [9.5.0 - Changes](#950-changes)
  - [9.5.0 - Source Code Comparison With Version 9.4.2](#950-source-code-comparison-with-version-942)
  - [9.5.0 - Milestone](#950-milestone)
  - [9.5.0 - Feedback](#950-feedback)
  - [9.5.0 - Contributions](#950-contributions)
  - [9.5.0 - Acknowledgement](#950-acknowledgement)

## What's New Since Version 9.4.2?

This cppTango release is a bug fix release. It contains improvements in several areas that are relevant to the users of cppTango. Please refer to [the selection of fixes](#950-bug-fixes) and [improvements and changes](#950-changes) where we highlight what was improved or modified.

## 9.5.0 - Bug fixes

- Fixes for double quoted strings and to support multiline strings with file DB. ([1162](https://gitlab.com/tango-controls/cppTango/-/issues/1162),[1163](https://gitlab.com/tango-controls/cppTango/-/issues/1163)).
-Fix regression introduced in #1129 in the template type check. ([#1137](https://gitlab.com/tango-controls/cppTango/-/issues/1137)).
- Fix a regression introduced in #1010 that affects the mapping of Tango 7 to Tango 9.4 events. ([#1135](https://gitlab.com/tango-controls/cppTango/-/issues/1135)).
- Fix to support the maximum hostname length depending on the OS. ([#1105](https://gitlab.com/tango-controls/cppTango/-/issues/1105)).
- Fix for conditional compilation on macOS/BSD. ([#1103](https://gitlab.com/tango-controls/cppTango/-/issues/1103)).
- Forward port of fix for handling of invalid attribute. ([#1088](https://gitlab.com/tango-controls/cppTango/-/issues/1088)).
- Forward port: Case sensitivity for class attributes property. ([#1087](https://gitlab.com/tango-controls/cppTango/-/issues/1087)).
-Fix in mapping event type to string to support all event types. ([#1065](https://gitlab.com/tango-controls/cppTango/-/issues/1065)).
- Forward port fix for event reconnection after renaming device server instance. ([#1063](https://gitlab.com/tango-controls/cppTango/-/issues/1063)).
- Fix broken DevDouble and on macOS aarch64 by requiring omniORB >= 4.3.1. ([#1056](https://gitlab.com/tango-controls/cppTango/-/issues/1056)).
- Substitute TANGO_XXX_BASE flags with XXX_ROOT flags autogenerated by find_package in Cmake. ([#1043](https://gitlab.com/tango-controls/cppTango/-/issues/1043)).
- Update minimum cmake version to 3.18. ([#975](https://gitlab.com/tango-controls/cppTango/-/issues/975)).
- Fix for wrong hostname information in blackbox in some situations. ([#865](https://gitlab.com/tango-controls/cppTango/-/issues/865)).
- Fix handling of ORB pointers by using \_var objects. ([#842](https://gitlab.com/tango-controls/cppTango/-/issues/842)).
- Fix for ownership handling of event data. ([#684](https://gitlab.com/tango-controls/cppTango/-/issues/684)).
- Fix to not use case sensitive comparisons in MultiClassAttribute. ([#](https://gitlab.com/tango-controls/cppTango/-/issues/)).

## 9.5.0 - Changes

- Forward port #927: Revise treatment of SIGTERM in Linux. ([#1123](https://gitlab.com/tango-controls/cppTango/-/issues/1123)).
- Document the release procedure. ([#1117](https://gitlab.com/tango-controls/cppTango/-/issues/1117)).
- Remove the unused and not implemented type DevInt. ([#1108](https://gitlab.com/tango-controls/cppTango/-/issues/1108)).
- Use standard file suffixes for source code files to placate editors (hh -> h, .tpp -> _tmpl.h)([#942](https://gitlab.com/tango-controls/cppTango/-/issues/942)).
- Add reconnect to command_inout_asynch. ([#906](https://gitlab.com/tango-controls/cppTango/-/issues/906)).
- Move all files under /cppapi to /src to match conventional repository structures. ([#964](https://gitlab.com/tango-controls/cppTango/-/issues/964)).
- Support also not exported devices in group. ([#303](https://gitlab.com/tango-controls/cppTango/-/issues/303)).
- Update to CONTRIBUTING.md to reflect new merging procedure. We require now only one approval by a code owner instead of two. ([#](https://gitlab.com/tango-controls/cppTango/-/issues/1104)).

## 9.5.0 - Source Code Comparison With Version 9.4.2

You can view the source code comparison with Tango 9.4.2, as well as the commits and contributors on this page:
https://gitlab.com/tango-controls/cppTango/-/compare/9.4.2...9.5.0

## 9.5.0 - Milestone

All the issues and Merge Requests associated to the 9.5.0 milestone can be found here: https://gitlab.com/tango-controls/cppTango/-/milestones/10

## 9.5.0 - Feedback

You can report issues on https://gitlab.com/tango-controls/cppTango/-/issues.
If you find a problem which you think could also affect Tango Controls packages other than cppTango, e.g. cppTango and pyTango, or when you don't know where you should create the issue at all - Well, you know now! - then head over to https://gitlab.com/tango-controls/TangoTickets/-/issues.

## 9.5.0 - Contributions

Contributions are always welcome!
Please do not hesitate to create new Merge Requests in [cppTango Gitlab repository](https://gitlab.com/tango-controls/cppTango). Please refer to [CONTRIBUTING.md](https://gitlab.com/tango-controls/cppTango/-/blob/main/CONTRIBUTING.md) to familiarise yourself with the easiest way to contribute to the cppTango project.

## 9.5.0 - Acknowledgement

Many thanks to all the persons who contributed to this release, to the Tango kernel team and to the Tango community for its ideas, feedback, bug reports and tests.


# Tango C++ library 9.4.2 Release Notes

June 19th, 2023

Table of Contents
=================
  - [What's New (Since Version 9.4.1)?](#whats-new-since-version-941)
  - [9.4.2 - Bug Fixes](#942-bug-fixes)
  - [9.4.2 - Changes](#942-changes)
  - [9.4.2 - Source Code Comparison With Version 9.4.1](#942-source-code-comparison-with-version-941)
  - [9.4.2 - Milestone](#942-milestone)
  - [9.4.2 - Feedback](#942-feedback)
  - [9.4.2 - Contributions](#942-contributions)
  - [9.4.2 - Acknowledgement](#942-acknowledgement)

## What's New Since Version 9.4.1?

This cppTango release is a bug fix release outside of the normal six month release cycle.

## 9.4.2 - Bug fixes

- A regression introduced in cppTango 9.4.1 was preventing to specify an omniORB endpoint with an empty hostname. This bug is fixed in cppTango 9.4.2 ([#1055](https://gitlab.com/tango-controls/cppTango/-/issues/1055)).
- MacOS support has been improved ([!1072](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1072), [!1094](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1094), [!1079](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1079), [!1056](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1056))
- When using `Group::write_attribute`, a bug could prevent client/server connection if the server was started after the client ([#906](https://gitlab.com/tango-controls/cppTango/-/issues/906))
- After our change of merging strategy, we add to update the ABI-API Compliance Check CI job which was failing from time to time([#1104](https://gitlab.com/tango-controls/cppTango/-/issues/1104))
- A bug was fixed when the specified endpoint is an ip address ([!1066](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1066),[#1081](https://gitlab.com/tango-controls/cppTango/-/issues/1081))

## 9.4.2 - Changes

- The device server programmer can use the `DeviceImpl::get_client_ident` method to get some information on the current client calling him ([!1028](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1028),[#801](https://gitlab.com/tango-controls/cppTango/-/issues/801)).
- cppTango 9.4.2 supports supplying nearly all dependent libraries via PKG_CONFIG_PATH ([!1042](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1042), [#1042](https://gitlab.com/tango-controls/cppTango/-/issues/1042), [#1044](https://gitlab.com/tango-controls/cppTango/-/issues/1044))
- Testing with debian 12 (bookworm) was added ([!1110](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1110),[#1130](https://gitlab.com/tango-controls/cppTango/-/issues/1130)).
- CI is no longer using gitlab.com Gitlab runners but using runners provided by several institutes from the Tango-Controls collaboration (many thanks to them!) ([#1110](https://gitlab.com/tango-controls/cppTango/-/issues/1110)).
- Some changes were required at the CMake level for a better integration with the Tango Source Distribution ([!1047](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1047)).
- Thomas Ives and Damien Lacoste are now belonging to the Code Owners team in the hope to speed up the development process. Welcome! ([#1098](https://gitlab.com/tango-controls/cppTango/-/issues/1098))
- The tests are now using a tag when cloning the tango-idl project ([#1082](https://gitlab.com/tango-controls/cppTango/-/issues/1082))
- The LLVM CI Job is now using LLVM 16 ([!1064](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1064)).
- The CMake omniidl check has been improved and is no longer called when cross-compiling without an emulator ([!1058](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1058)).
- We only support a few versions of omniORB.  We now ensure that a supported version has been found at cmake configuration time. If an invalid version is found, we raise an error ([!1039](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1039), [#1021](https://gitlab.com/tango-controls/cppTango/-/issues/1021)).
- The commit 0bcb356f introduced a std:: namespace in many places in documentation and comments where this was not intended.
These unexpected side effects are no longer present in cppTango 9.4.2 ([!1037](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1037), [#1026](https://gitlab.com/tango-controls/cppTango/-/issues/1026))
- The documentation about cross-compiling has been improved ([!1049](https://gitlab.com/tango-controls/cppTango/-/merge_requests/1049)).

## 9.4.2 - Source Code Comparison With Version 9.4.1

You can view the source code comparison with Tango 9.4.1, as well as the commits and contributors on this page:
https://gitlab.com/tango-controls/cppTango/-/compare/9.4.1...9.4.2

## 9.4.2 - Milestone

All the issues and Merge Requests associated to the 9.4.2 milestone can be found here: https://gitlab.com/tango-controls/cppTango/-/milestones/12

## 9.4.2 - Feedback

You can report issues on https://gitlab.com/tango-controls/cppTango/-/issues.
If you find a problem which you think could also affect Tango Controls packages other than cppTango, e.g. cppTango and pyTango, or when you don't know where you should create the issue at all - Well, you know now! - then head over to https://gitlab.com/tango-controls/TangoTickets/-/issues.

## 9.4.2 - Contributions

Contributions are always welcome!
Please do not hesitate to create new Merge Requests in [cppTango Gitlab repository](https://gitlab.com/tango-controls/cppTango). Please refer to [CONTRIBUTING.md](https://gitlab.com/tango-controls/cppTango/-/blob/main/CONTRIBUTING.md) to familiarise yourself with the easiest way to contribute to the cppTango project.

## 9.4.2 - Acknowledgement

Many thanks to all the persons who contributed to this release, to the Tango kernel team and to the Tango community for its ideas, feedback, bug reports and tests.

# Tango C++ library 9.4.1 Release Notes

February 2nd, 2023

Table of Contents
=================
  - [What's New (Since Version 9.3.5)?](#whats-new-since-version-935)
  - [9.4.0+ - Changes Which Might Have An Impact On Users](#940-changes-which-might-have-an-impact-on-users)
  - [9.4.0 - Bug Fixes](#940-bug-fixes)
  - [9.4.1 - Bug Fixes](#941-bug-fixes)
  - [9.4.1 - Changes](#941-changes)
  - [9.4.0+ - Source Code Comparison With Version 9.3.5](#940-source-code-comparison-with-version-935)
  - [9.4.1 - Source Code Comparison With Version 9.4.0](#941-source-code-comparison-with-version-940)
  - [9.4.1 - Feedback](#941-feedback)
  - [9.4.1 - Contributions](#941-contributions)
  - [9.4.1 - Acknowledgement](#941-acknowledgement)

## What's New Since Version 9.4.0?

This cppTango release is a bug fix release outside of the normal six month release cycle.

Finally it is possible to deploy DeviceServers in containerised and isolated environments and allow event subscriptions across these deployment islands. We laid the foundation for this already in 9.4.0 but a small bug prevented this feature from working (see below Fix ZMQ event port advertisement).

## What's New Since Version 9.3.5)?

### A fresh cppTango every six months!

We have introduced a [fixed release cycle for cppTango](https://gitlab.com/tango-controls/cppTango/-/issues/897). This means that we will release a new cppTango version every six months on April 2 and October 2 from now on. If severe bug fixes warrant an earlier release, we will issue bug fix releases in between.

We have also established a Milestone-based, i.e. release-oriented development process which you can follow on https://gitlab.com/tango-controls/cppTango/-/boards.

And how will this impact you? cppTango 9.4.0 came with an unavoidable amount of work if you want to upgrade. With a six month release cycle we believe that after the 9.4.0 release the differences between future cppTango releases will then only have a minimal impact on your system if you continue to upgrade. This will make it much easier for you to stay on top of the cppTango releases. The proof that new releases will usually only have a minimal impact is this 9.4.1 release.

### Binary incompatibility with cppTango <= 9.3

cppTango 9.4 is binary incompatible with any older version of cppTango, i.e. cppTango version <= 9.3. The source code underwent major refactoring to use modern [C++14 features](#c14-requirement). We also made a serious effort to make it easier to maintain in some areas. With the refactoring have also come a couple of source code incompatibilities. This will mostly affect users who want to use cppTango 9.4.x as a drop-in replacement for cppTango <= 9.3. A simple recompile of their C++ device servers will likely fail. Please read the section [9.4 - Changes Which Might Have An Impact On Users](#940---changes-which-might-have-an-impact-on-users) where we explain how to mitigate the immediate issues when upgrading from cppTango <= 9.3.

### No Changelog this time (again)

Further we will not provide a dedicated changelog file for this 9.4.1 release. There simply have been too many Merge Requests and Bug Fixes which since cppTango 9.3, this means 9.4.0 plus the ones for 9.4.1. We decided to save our time and not spend our limited manpower on individually listing all of them in a human readable form. Instead please refer to the Issues and Merge Requests on the cppTango repository. The [Milestone 9.4.1](https://gitlab.com/tango-controls/cppTango/-/milestones/8) will be a good starting point for your search.
We will assess the situation again for the next releases.

### server_init_hook

A new server_init_hook() has been added (in the same spirit as always_executed_hook() method) at device level. This new method is called once the device server admin device is exported.
This allows for instance for the different devices to subscribe to events at server startup on attributes from other devices of the same device server with stateless parameter set to false. The subscription should now work in this case since the admin device is exported when the server_init_hook() method is called.
([#498](https://gitlab.com/tango-controls/cppTango/-/issues/498), [!551](https://gitlab.com/tango-controls/cppTango/-/merge_requests/551), [TangoTickets#7](https://gitlab.com/tango-controls/TangoTickets/-/issues/7)).

### Tango Controls On MacOS

Since cppTango 9.4.0 it is possible to compile cppTango on MacOS! ([#956](https://gitlab.com/tango-controls/cppTango/-/issues/956), [!978](https://gitlab.com/tango-controls/cppTango/-/merge_requests/978),[#714](https://gitlab.com/tango-controls/cppTango/-/issues/714), [!725](https://gitlab.com/tango-controls/cppTango/-/merge_requests/725)). See https://gitlab.com/tjuerges/build_tango for a set of build scripts for macOS (x86_64 and M1/M2 archs) for tango-idl, cppTango, TangoDatabase, TangoTest, UniversalTest, tango_admin and PyTango.

### CMake Improvements

### cppTango Dev Conda Packages

Development cppTango Conda Packages are now automatically created and uploaded by CI when there are new commits to the cppTango main branch ([!875](https://gitlab.com/tango-controls/cppTango/-/merge_requests/875), [!956](https://gitlab.com/tango-controls/cppTango/-/merge_requests/956), [!938](https://gitlab.com/tango-controls/cppTango/-/merge_requests/938), [!921](https://gitlab.com/tango-controls/cppTango/-/merge_requests/921)]).
These packages are uploaded to the tango-controls channel with the dev label.
They can then be installed using:
`conda install -c conda-forge -c tango-controls/label/dev cpptango=9.4.0dev0`

### Warnings Fixes

Many compilation warnings have been fixed (([#962](https://gitlab.com/tango-controls/cppTango/-/issues/962), [!984](https://gitlab.com/tango-controls/cppTango/-/merge_requests/984), ...).

### Code Refactoring And Cleanup

A big part of the code has been rewritten in order to remove some code duplication. For instance, MR [!735](https://gitlab.com/tango-controls/cppTango/-/merge_requests/735) reduced the code size by about 5000 lines of code.

## 9.4.0+ - Changes Which Might Have An Impact On Users

### Binary Incompatibility With cppTango 9.3.x Versions

To fix some of the bugs, the cppTango developers had no choice but to break the binary compatibility. So this new version is no longer binary compatible with previous cppTango 9.3.x releases. As a consequence, the user code using cppTango will have to be recompiled to use this new version. The good news is some annoying bugs have been fixed. This was the price to pay to get them fixed.

### C++14 Requirement

To ease the maintenance of cppTango, it has been decided to allow the usage of C++11 and C++14 features in cppTango code.
As a consequence, you need to use a C++14 compatible C++ compiler to compile cppTango and your device servers using cppTango 9.4.0.
You have to use `-std=c++14` or `-std=gnu++14` (or above) option when compiling your device server.
If you are using CMake, you can add the following lines to your CMakeLists.txt:

```
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)  # or above
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### "TANGO_" Prefix Added To CMake Options

All the cppTango specific CMake options are now prefixed with "TANGO_" ([!895](https://gitlab.com/tango-controls/cppTango/-/merge_requests/895)).
So if you got used to compile cppTango 9.3 using CMake options like `-DOMNI_BASE=/path/to/my/omniorb`, you will have to use `-DTANGO_OMNI_BASE=/path/to/my/omniorb` instead.

Please refer to [INSTALL.md file](INSTALL.md) to get the detailed list of available CMake variables with their default value and description.

### Removed `using namespace std;` Directive From tango.h

Since it is considered as a bad practice to put a `using namespace std;` directive in a header file, it has been decided to remove it from `tango.h` file. As a consequence, cppTango code had to be massively updated to add missing `std::` prefixes in all locations where it was required to compile successfully ([#206](https://gitlab.com/tango-controls/cppTango/-/issues/206), [!528](https://gitlab.com/tango-controls/cppTango/-/merge_requests/528), [!981](https://gitlab.com/tango-controls/cppTango/-/merge_requests/981)).
Users who are used to the old way can just do
```
 #include <tango.h>
 using namespace std;
```
wherever `tango.h` is included.

Please note that the code generated by Pogo for the C++ device servers will have to be updated or regenerated too if it was generated with an old Pogo version ( < Pogo 9.8.0) because these previous Pogo versions were relying on the fact that `using namespace std;` was present in `tango.h` file.

### coutXXX Macros Renamed To TANGO_LOG_XX

There was a `cout` macro defined in cppTango code which was under some configurations redefining `cout` and redirecting it to the Tango logs. This was causing some compilation errors when using `std::cout` instead of `cout` in the cppTango code. To avoid this problem, the `cout` macro has been renamed `TANGO_LOG`.
`cout1` and `cout2` macros have been replaced with` TANGO_LOG_INFO` macro.
`cout3`, `cout4` and `cout5` macros have been replaced with `TANGO_LOG_DEBUG` macro.
As a consequence, the device servers POGO generated codes, which were using some of these macros will have to be regenerated with a recent POGO version >= 9.8.0 in order to use the new macros names.
More details in [#891](https://gitlab.com/tango-controls/cppTango/-/issues/891), [!934](https://gitlab.com/tango-controls/cppTango/-/merge_requests/934) and [pogo#137](https://gitlab.com/tango-controls/pogo/-/issues/137). Pogo >= 9.8.0 will also generate some code to define these `TANGO_LOG` macros if an older version of cppTango is used, making the generated code still compatible with older cppTango versions.

### New Header Files Installation Paths And Default Include Path

To prevent including a wrong file by accident ([#720](https://gitlab.com/tango-controls/cppTango/-/issues/720)), it has been decided to change the default include path in tango.pc file which is used by pkgconfig ([!952](https://gitlab.com/tango-controls/cppTango/-/merge_requests/952)).
The previous include path was `${prefix}/include/tango`. It is now set to `${prefix}/include` instead.
`tango.h` file is still installed under `${prefix}/include/tango`.
The consequence of this default include path change is that all files currently doing `#include <tango.h>` will have to be modified in order to use `#include <tango/tango.h>` instead.
Recent POGO versions (>= 9.8.0) are taking this change into account and are already generating code with `#include <tango/tango.h>`.
The header files have been reorganized in cppTango source code and are now located under cppapi/include/tango directory and its subdirectories. They are installed under `${prefix}/include/tango` subdirectories now.

### New Optional Dependency To libjpeg Or libjpeg-turbo

In order to ease the maintenance of the encode and decode jpeg methods provided in EncodedAttribute class, it has been decided to get rid of the custom code and to replace it with an optional dependency to libjpeg or libjpeg-turbo library, which are well known and well maintained jpeg libraries specialized in this domain.
If you know that you will never use the EncodedAttribute encode_jpeg_xx/decode_jpeg_xx methods, you can compile cppTango using `-DTANGO_USE_JPEG=OFF` during the cppTango CMake configure compilation step.
By default `TANGO_USE_JPEG` is ON and if your libjpeg or libjpeg-turbo dependency is not at a location easily found by pkg-config, you can use `-DTANGO_JPEG_BASE=/your/jpeg/install/path` CMake option to specify the directory where your libjpeg or libjpeg-turbo library is installed.

### TANGO_LONG_32 and TANGO_LONG_64 Defines Removed

While fixing an issue on Windows amd64 ([#768](https://gitlab.com/tango-controls/cppTango/-/issues/768), [!792](https://gitlab.com/tango-controls/cppTango/-/merge_requests/792)), it has been decided, after consultation of the Tango-Controls community, to refactor the cppTango code and to remove no longer needed TANGO_LONG_32 and TANGO_LONG_64 definitions from tango_const.h file.
If your code is relying on the presence of this definitions, you will have to adapt it. The only device server known to be using these definition was the Tango Database device server. The code of this device server has been updated (See [TangoDatabase!34](https://gitlab.com/tango-controls/TangoDatabase/-/merge_requests/34)).

### Input References Arguments Changed To Const References

The methods input reference arguments have been changed to equivalent const reference arguments when possible ([#622](https://gitlab.com/tango-controls/cppTango/-/issues/622), [!886](https://gitlab.com/tango-controls/cppTango/-/merge_requests/886)).
This is a change of the API which will hopefully ease the Tango developer work.
Thanks to this change, it is now possible to do:

```
ApiUtil.get_db_ind("hostname", 12345);
```

instead of

```
std::string tmp = "hostname";
ApiUtil.get_db_ind(tmp, 12345);
```

There might be some side effects compilation errors on your device servers.

### Default Logging Layout Changed

The default logging layout has been changed to contain human readable timestamps instead of timestamps since the Unix epoch ([!845](https://gitlab.com/tango-controls/cppTango/-/merge_requests/845)).

Before 9.4.0, the logs on the console looked like this by default:

```
1619681907 [140266166015744] DEBUG my/super/device The log message text #1
1619681907 [140266166015744] DEBUG my/super/device The log message text #2
1619681907 [140266157623040] my/super/device The log message text #3
```
Since 9.4.0 they look like this:
```
2021-05-04T23:04:41,544490+0200 DEBUG (MySuperDevice.cpp:412) my/super/device The log message text #1
2021-05-04T23:04:41,544872+0200 DEBUG (MySuperDevice.cpp:1627) my/super/device The log message text #2
2021-05-04T23:04:41,544987+0200 DEBUG (MySuperDevice.cpp:1632) my/super/device The log message text #3
```

The timestamps are now in a human readable format, the thread id ([140266166015744] and [140266157623040] in the first example) has been removed. The log message origin (file and line number) is now automatically added to the log ([!742](https://gitlab.com/tango-controls/cppTango/-/merge_requests/742)).

### Function Name And Line Number In Logs And Exceptions

As explained just above, [!742](https://gitlab.com/tango-controls/cppTango/-/merge_requests/742) added file name and line number information in the logs displayed on the console.
This Merge Request also provided new macros to throw exceptions without having the need to specify the origin field of the exception.
To have exception origin field filled automatically, please use `TANGO_THROW_EXCEPTION(reason, desc)` macro (there is also `TANGO_RETHROW_EXCEPTION` and corresponding macros for `_API_EXCEPTION`).
When using the new `TANGO_THROW_EXCEPTION(reason, desc)` macro (there is also `TANGO_RETHROW_EXCEPTION` and corresponding macros for `_API_EXCEPTION`), the origin field is no longer required and is automatically deduced and filled with the name of the current method. The file name and line number is also automatically added in the exception origin field.
So don't hesitate to use these new macros to avoid the classical copy/paste errors where you end up with an misleading origin field in an exception pointing to a wrong method in the code.
Please be aware that your code will no longer be backwards compatible with cppTango 9.3.x versions if you use these new macros, though.

### End Of Support For Notifd Events And Multicast Events

According to surveys with the tango-controls community, nobody seemed to use notifyd and ZMQ Multicast events. Therefore we have removed support for notifd events and ZMQ Multicast events in cppTango 9.4.0.  Only ZMQ Events are supported from cppTango 9.4.0 on.

### Removed `Tango_isnan` helper function

As it was simply pointing towards `std::isnan` we now recommend to use the later directly.

## 9.4.0 - Bug Fixes

Many bugs have been fixed in this new version. please refer to the Issues and Merge Requests on the cppTango repository. [Milestone 9.4.0](https://gitlab.com/tango-controls/cppTango/-/milestones/7) is listing some of the many issues which have been fixed in this version.

## 9.4.1 - Bug fixes

- Fix ZMQ event port advertisement ([https://gitlab.com/tango-controls/cppTango/-/issues/](https://gitlab.com/tango-controls/cppTango/-/issues/915)[915](https://gitlab.com/tango-controls/cppTango/-/issues/915), https://gitlab.com/tango-controls/cppTango/-/merge_requests/1035)

- Fix problems with adding/removing dynamic attributes in version 9.4 (https://gitlab.com/tango-controls/cppTango/-/issues/1022, https://gitlab.com/tango-controls/cppTango/-/merge_requests/1031)

- Improve DeviceAttribute::get_type() behaviour (https://gitlab.com/tango-controls/cppTango/-/issues/1010, https://gitlab.com/tango-controls/cppTango/-/issues/1002, https://gitlab.com/tango-controls/cppTango/-/merge_requests/1030). When reading an empty spectrum, clients are now able to get the expected type with DeviceAttribute::get_type(). DATA_TYPE_UNKNOWN is returned when an attribute reading is throwing an exception.

- Fix tango.pc.cmake to take into account jpeg dependency (https://gitlab.com/tango-controls/cppTango/-/issues/1016, https://gitlab.com/tango-controls/cppTango/-/merge_requests/1024)

- tests: Fix using invalid cmake variable COMMON_TEST_CXX_DEFS (https://gitlab.com/tango-controls/cppTango/-/merge_requests/1038)

- Fix api-abi-compliance CI job (https://gitlab.com/tango-controls/cppTango/-/issues/1045, https://gitlab.com/tango-controls/cppTango/-/issues/1040, https://gitlab.com/tango-controls/cppTango/-/merge_requests/1041)

## 9.4.1 - Changes

- ci: Raise gcc/llvm latest compiler versions to gcc 12.2.0 and LLVM 15 (https://gitlab.com/tango-controls/cppTango/-/merge_requests/1033)

- tango.pc: Remove spaces in variable definitions (https://gitlab.com/tango-controls/cppTango/-/issues/1024, https://gitlab.com/tango-controls/cppTango/-/merge_requests/1032)

## 9.4.0+ - Source Code Comparison With Version 9.3.5

You can view the source code comparison with Tango 9.3.5, as well as the commits and contributors on this page:
https://gitlab.com/tango-controls/cppTango/-/compare/9.3.5...9.4.1

Sadly, at the time of writing these release notes, Gitlab imposes a limitation which prevents from displaying more than 100 commits at a time. We hope that this limit will be removed in the future.

## 9.4.1 - Source Code Comparison With Version 9.4.0

You can view the source code comparison with Tango 9.4.0, as well as the commits and contributors on this page:
https://gitlab.com/tango-controls/cppTango/-/compare/9.4.0...9.4.1

## 9.4.1 - Feedback

You can report issues on https://gitlab.com/tango-controls/cppTango/-/issues. If you find a problem which you think could also affect Tango Controls packages other than cppTango, e.g. cppTango and pyTango, or when you don't know where you should create the issue at all - Well, you know now! - then head over to https://gitlab.com/tango-controls/TangoTickets/-/issues.

## 9.4.1 - Contributions

Contributions are always welcome!
Please do not hesitate to create new Merge Requests in [cppTango Gitlab repository](https://gitlab.com/tango-controls/cppTango). Please refer to [CONTRIBUTING.md](https://gitlab.com/tango-controls/cppTango/-/blob/main/CONTRIBUTING.md) to familiarise yourself with the easiest way to contribute to the cppTango project.

## 9.4.1 - Acknowledgement

Many thanks to all the persons who contributed to this release, to the Tango kernel team and to the Tango community for its ideas, feedback, bug reports and tests.
