## Versioning Schema

This document describes the versioning schema used by cppTango.

cppTango uses version numbers of the form `$major.$minor.$patch`, however,
cppTango does not follow semantic versioning so these components have different
meaning to the version components described by semantic versioning.

cppTango uses the major component to describe the _network_ interface to other
Tango devices.  This includes, for example, Tango devices using JTango which is
an entirely different Tango implementation but uses the network interface.  For
example, a device server built with cppTango 9.x will use the same network
interface as a device server built with JTango 9.x. Things which contribute to
the network interface include the tango-idl version and the event system
protocol.

Historically, the network interface has been backwards compatible, however,
there are currently plans to deprecate and remove parts of the network interface
(pipes and notifd events) which would result in a future network interface
version having a minimum compatible version of the network interface.

cppTango uses the `$major.$minor` version number pair to describe the API/ABI
compatibility of the libtango.so library.  In other words, any changes to the
API/ABI can occur in either a major or minor release.

The cppTango core development team are more conservative when making API/ABI
breaks in a minor release cycle, however, they reserve the right to break
API/ABI if the benefits significantly outweigh the costs.  It is possible that a
minor release will not involve an API/ABI break, in which case the SO_NAME of
the libtango.so will reflect that.

A patch release will never break API/ABI and is only used to provide urgent
fixes for a previous release.

During the development of a release, the most recent tag on the main branch will
be `$major.$minor.$patch-dev` so that `git describe --tags --always main`
outputs something like `$major.$minor.$patch-dev-$count-g$sha`.  The
`Tango::git_revision()` function can be used to get the output of this command
as it was recorded at compilation time.  Development versions of libtango.so
have a dev suffix.
