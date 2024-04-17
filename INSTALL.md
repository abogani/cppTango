# Prerequisites

The following software packages are required to build cppTango:

- A C++17 compliant compiler like GCC, clang or Visual Studio (2019 or newer)
- [cmake](https://cmake.org), 3.18 or newer
- [tango-idl](https://gitlab.com/tango-controls/tango-idl)
- [omniORB](http://omniorb.sourceforge.net), 4.3.0 or newer
- [libzmq](https://github.com/zeromq/libzmq), 4.0.5 or newer
- [cppzmq](https://github.com/zeromq/cppzmq), 4.7.1 or newer

Additionally cppTango can be built with jpeg support.
In this case a jpeg implementation must be present:

- [libjpeg-turbo](https://www.libjpeg-turbo.org/), 1.5.2 or newer

Building the tests requires:

- [Catch2](https://github.com/catchorg/Catch2/), 3.1.1 or newer

In the following we assume a linux-based system, see [here](#building-on-windows) for building on Windows.

On current debian systems only some dependencies are available. Most notably tango-idl and omniORB are missing.

```bash
sudo apt install cmake cppzmq-dev build-essential git libjpeg-dev python3
```

If your linux does not have precompiled packages for these dependencies jump to the
[next](#compiling-the-dependencies) section for compilation instructions.

## Keeping Up-to-date

This repository uses git submodules.

- Ensure that you use `--recurse-submodules` when cloning:

    `git clone --recurse-submodules ...`

- If you didn't clone with `--recurse-submodules`, run

    `git submodule update --init`

  to initialise and fetch submodules data.

- Ensure that updates to git submodules are pulled:

    `git pull --recurse-submodules`

## Compiling tango

## tango-idl

```bash
git clone --depth 1 -b 6.0-alpha2 https://gitlab.com/tango-controls/tango-idl
cd tango-idl
mkdir build
cd build
cmake ..
// no make required
sudo make install
```

## cppTango

```bash
git clone --recurse-submodules --depth 1 https://gitlab.com/tango-controls/cppTango
cd cppTango
mkdir build
cd build
cmake ..
make [-j NUMBER_OF_CPUS]
sudo make install
```

## CMake Variables

Dependencies are located through cmake's find modules.
You can provide custom locations for the dependencies using [`<PackageName>_ROOT`](https://cmake.org/cmake/help/latest/variable/PackageName_ROOT.html#variable:%3CPackageName%3E_ROOT)
which can be both used as environment variables or directly as a cmake variable.
e.g.:
```bash
cmake -Dtangoidl_ROOT=/us/local ..
```

On linux-based systems we leverage the use of `pkg-config`.
Therefore you can also pass additional search locations via the
`PKG_CONFIG_PATH` environment variable as in

```
PKG_CONFIG_PATH="/usr/local/libzmq:/usr/local/omniORB" cmake ..
```

the dependencies we need are:
 * cppzmq
 * JPEG (optional)
 * omniORB4
 * pthread (optional, windows only)
 * tangoidl
 * ZeroMQ
 * opentelemetry-cpp, protobuf, grpc, abseil, libcurl, c-ares, libre2 (optional)

The following variable can be passed to cmake to tweak compilation. The general syntax is `-D$var=$value`.

<!-- Keep the variable list sorted -->

| Variable name                      |  Default value                                  | Description
|------------------------------------|-------------------------------------------------|--------------------------------------------------------------------------------------------------
| `BUILD_SHARED_LIBS`                | `ON`                                            | Build tango as shared library, `OFF` creates a static library
| `BUILD_TESTING`                    | `ON`                                            | Build the test suite
| `CMAKE_BUILD_TYPE`                 | `Release`                                       | Compilation type, can be `Release`, `Debug` or `RelWithDebInfo/MinSizeRel` (Linux only)
| `CMAKE_DISABLE_PRECOMPILE_HEADERS` | `OFF`                                           | Precompiled headers (makes compilation much faster)
| `CMAKE_INSTALL_PREFIX`             | `/usr/local` or `C:/Program Files`              | Desired install path
| `CMAKE_VERBOSE_MAKEFILE`           | `OFF`                                           | Allows to increase the verbosity level with `ON`
| `TANGO_ENABLE_COVERAGE`            | `OFF`                                           | Instrument code for coverage analysis
| `TANGO_ENABLE_SANITIZER`           | *empty*                                         | Compile with sanitizers, one of: `ASAN`, `TSAN`, `UBSAN` or `MSAN` (Requires Clang/GCC)
| `TANGO_GIT_SUBMODULE_INIT`         | `ON`                                   | If cppTango is a git repository, automatically checkout TangoCMakeModules at CMake configure time.
| `TANGO_INSTALL_DEPENDENCIES`       | `OFF`                                           | Install dependencies of tango as well (Windows only)
| `TANGO_OMNIIDL_PATH`               |                                                 | omniORB4 search path for omniidl
| `TANGO_SKIP_OMNIORB_VERSION_CHECK` | `OFF`                                           | Do not check the version of omniORB.  Enable this at your own risk.
| `TANGO_USE_JPEG`                   | `ON`                                            | Build with jpeg support, in this case a jpeg library implementation is needed.
| `TANGO_USE_LIBCPP`                 | `OFF`                                           | Compile against libc++ instead of stdlibc++ (Requires Clang)
| `TANGO_USE_PTHREAD`                | `OFF`                                           | On windows platforms, build with pthread library.
| `TANGO_USE_TELEMETRY`              | `OFF`                                           | Enable tracing for servers and clients
| `TANGO_TELEMETRY_USE_GRPC`         | `OFF`, enabled if `TANGO_USE_TELEMETRY` == `ON` | Enable GRPC exporter for tracing
| `TANGO_TELEMETRY_USE_HTTP`         | `OFF`, enabled if `TANGO_USE_TELEMETRY` == `ON` | Enable HTTP exporter for tracing
| `TANGO_WARNINGS_AS_ERRORS`         | `OFF`                                           | Treat compiler warnings as errors

cppTango supports unity builds to speed up the compilation. Please see the
[related CMake documentation](https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html)
for more details on how to enable and configure this feature.

## Cross-compiling tango

For compiling tango for a different architectures than the host architecture, it is
advisable to cross compile tango. We demonstrate the approach by compiling for
32-bit on a 64-bit linux. See [here](https://wiki.debian.org/Multiarch/HOWTO)
for the generic debian howto.

```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt remove libcos4-dev libomniorb4-dev libomnithread4-dev libzmq3-dev
sudo apt install gcc-i686-linux-gnu
sudo apt install gcc-multilib g++-multilib
sudo apt install libcos4-dev:i386 libomniorb4-dev:i386 libomnithread4-dev:i386 libzmq3-dev:i386 cppzmq-dev
mkdir build-cross-32bit
cd build-cross-32bit
cmake -DCMAKE_TOOLCHAIN_FILE=../configure/toolchain-i686.cmake ..
make [-j NUMBER_OF_CPUS]
```

cmake should output `Target platform: Linux 32-bit`. You can also inspect the
created library using `file` to check that you built it correctly.

##  Installation with custom prefix

It is possible to have multiple tango installations on one machine. It uses
`CMAKE_PREFIX_PATH` for that.

```bash
cmake -B build-XXX -S . -DCMAKE_INSTALL_PREFIX=/usr/local/tango-XXX
cmake --build build-XXX
sudo cmake --install build-XXX
```

Compiling a tango DS against this installation can then be done with

```bash
PKG_CONFIG_PATH=/usr/local/tango-XXX/lib/pkgconfig cmake -B build-YYY -S .
cmake --build build-YYY
```

# Compiling the dependencies

We assume that you have a compiler already and are on a linux based system.
Additionally the tools git, wget, tar and bzip2 are required.

## CMake

```bash
git clone --depth 1 -b v3.18.4 https://github.com/Kitware/CMake cmake
cd cmake
mkdir build
cd build
../bootstrap
make [-j NUMBER_OF_CPUS]
sudo make install
```

## libzmq

```bash
git clone --depth 1 -b v4.2.0 https://github.com/zeromq/libzmq
cd libzmq
mkdir build
cd build
cmake -DENABLE_DRAFTS=OFF -DWITH_DOC=OFF -DZMQ_BUILD_TESTS=OFF ..
make [-j NUMBER_OF_CPUS]
sudo make install
```

## cppzmq

```bash
git clone --depth 1 -b v4.7.1 https://github.com/zeromq/cppzmq
cd cppzmq
mkdir build
cd build
cmake -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
make [-j NUMBER_OF_CPUS]
sudo make install
```

## omniORB4

```bash
sudo apt install libzstd-dev zlib1g-dev python3-dev libssl-dev
wget -L https://sourceforge.net/projects/omniorb/files/omniORB/omniORB-4.3.0/omniORB-4.3.0.tar.bz2/download -O omniORB-4.3.0.tar.bz2
tar xjf omniORB-4.3.0.tar.bz2
cd omniORB-4.3.0
./configure --with-openssl
make [-j NUMBER_OF_CPUS]
sudo make install
```

## opentelemetry with dependencies

The steps are too involved to post here. Please see the files
`install_opentelemetry.sh` and `install_opentelemetry_deps.sh`
[in this repository](https://gitlab.com/tango-controls/docker/ci/cpptango/scripts).

Now with all these dependencies installed the cmake invocation to compile cppTango looks like

```bash
cd cppTango/build
cmake ..
```

if cmake does not find some of the dependencies, you can either add a custom `PKG_CONFIG_PATH` environment variable with

```bash
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig cmake ..
```

or use the the CMAKE variables `ZeroMQ_ROOT`, `cppzmq_ROOT`, `tangoidl_ROOT`, `omniORB4_ROOT` from [here](#cmake-variables).

# Using pkg-config in packages requiring tango

Once installed cppTango provides
[pkg-config](https://en.wikipedia.org/wiki/Pkg-config) file `tango.pc`. One can
use it to resolve libtango dependencies:

```cmake
include(FindPkgConfig)
pkg_search_module(TANGO_PKG REQUIRED tango)

if(NOT TANGO_PKG_FOUND)
  message(FATAL_ERROR "Could not find tango")
endif()

link_directories(${TANGO_PKG_LIBRARY_DIRS})

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${TANGO_PKG_INCLUDE_DIRS})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC ${TANGO_PKG_CFLAGS_OTHER})
target_link_libraries(${PROJECT_NAME} PUBLIC ${TANGO_PKG_LIBRARIES})
```

`tango.pc` provides default installation directory for all Tango devices linked against this libtango:

```bash
pkg-config --variable=tangodsdir tango
/usr/bin
```
# Header file location in cppTango 9.4

From cppTango version 9.4.0 on the header files will still be installed in `${prefix}/include/tango`, but they will not be installed in a single directory any more. The new directory structure for the header files follows discussion in issue [!735](https://gitlab.com/tango-controls/cppTango/-/issues/735):

```bash
CPPTANGO REPO/src/
  include/
  include/tango/
  include/tango/server/
  include/tango/client/
  include/tango/common/
  include/tango/common/log4tango
  include/tango/common/utils
```

As can be seen are header files now split into functional groups. The files are also installed following this mapping. New code which is based in cppTango 9.4.0 should follow the guideline to include only relevant header files and specify them with the proper path like `#include <tango/client/DeviceProxy.h` instead of just including `tango/tango.h`.

Old device servers which include just `tango.h` can easily be recompiled after `tango/tango.h` is symlinked to `tango.h`.

# Building on windows

For the majority of users using the prebuilt binaries from the release page is
easier. The following documentation is targeted for developers.

We assume Windows 10 and a Visual Studio 2017 development environment. In
addition python 3.7 must be installed (this is required by omniidl), get it
from [here](https://www.python.org/downloads/release/python-379). You need the
same bitness as you want to compile tango for. And add python to the path
during installation as well.

- Fetch the required dependencies into `c:\projects`

```bat
SET ARCH=x64-msvc15
SET PYVER=py37
https://github.com/tango-controls/omniorb-windows-ci/releases/download/4.3.0/omniorb-4.3.0_%ARCH%_%PYVER%.zip
https://github.com/tango-controls/Pthread_WIN32/releases/download/2.9.1/pthreads-win32-2.9.1_%ARCH%.zip
https://github.com/tango-controls/zmq-windows-ci/releases/download/4.0.5-2/zmq-4.0.5-2_%ARCH%.zip
git clone --depth 1 -b 5.1.2 https://gitlab.com/tango-controls/tango-idl tango-idl-source
```

- Open a VS 2017 command prompt

- Install tango-idl

```bat
cd tango-idl-source
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX="c:/projects/tango-idl"
cmake --build . --target install
```

- Switch to cppTango directory
- `mkdir build`
- `cd build`
- Configure with

```
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Release                             \
      -Dtangoidl_ROOT="c:/projects/tango-idl"                                                                               \
      -DomniORB4_ROOT="c:/projects/omniorb" -DZeroMQ_ROOT="c:/projects/zeromq" -Dcppzmq_ROOT="c:/projects/zeromq" \
      -Dpthread_ROOT="c:/projects/pthreads-win32" -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON -DBUILD_TESTING=OFF ..
```

- Compile with `cmake --build .`
- Install with `cmake --build . --target install`
- You now have a full tango installation in `build/install`

## Compiling device servers

This is currently not very well integrated into cmake, therefore the command line looks a bit clumsy:

shared/debug/x64:
```bat
    set BASE_FOLDER="C:\\Users\\Test\\projekte\\tango-projects\\libtango_9.3.5_v141_x64_shared_debug"
    cmake -B build-shared -S . -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-DTANGO_HAS_DLL" -DTANGO_PKG_LIBRARY_DIRS="%BASE_FOLDER%\\lib" -DTANGO_PKG_LIBRARIES="tangod;omniORB4_rtd;omnithread_rtd;COS4_rtd;omniDynamic4_rtd;libzmq-v141-mt-gd-4_0_5;comctl32;wsock32;Ws2_32" -DTANGO_PKG_INCLUDE_DIRS="%BASE_FOLDER%\\include;%BASE_FOLDER%\\include\\tango"
    cmake --build build-shared --config Debug
```

static/debug/x64:
```bat
    set BASE_FOLDER="C:\\Users\\Test\\projekte\\tango-projects\\libtango_9.3.5_v141_x64_static_debug"
    cmake -B build-static -S . -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-D_WINSTATIC" -DTANGO_PKG_LIBRARY_DIRS="%BASE_FOLDER%\\lib" -DTANGO_PKG_LIBRARIES="libtangod;omniORB4d;omnithreadd;COS4d;omniDynamic4d;libzmq-v141-mt-sgd-4_0_5;comctl32;wsock32;Ws2_32" -DTANGO_PKG_INCLUDE_DIRS="%BASE_FOLDER%\\include;%BASE_FOLDER%\\include\\tango"
    cmake --build build-static --config Debug
```

shared/release/x64:
```bat
    set BASE_FOLDER="C:\\Users\\Test\\projekte\\tango-projects\\libtango_9.3.5_v141_x64_shared_release"
    cmake -B build-shared -S . -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DTANGO_HAS_DLL" -DTANGO_PKG_LIBRARY_DIRS="%BASE_FOLDER%\\lib" -DTANGO_PKG_LIBRARIES="tango;omniORB4_rt;omnithread_rt;COS4_rt;omniDynamic4_rt;libzmq-v141-mt-4_0_5;comctl32;wsock32;Ws2_32" -DTANGO_PKG_INCLUDE_DIRS="%BASE_FOLDER%\\include;%BASE_FOLDER%\\include\\tango"
    cmake --build build-shared --config Release
```

static/release/x64:
```bat
    set BASE_FOLDER="C:\\Users\\Test\\projekte\\tango-projects\\libtango_9.3.5_v141_x64_static_release"
    cmake -B build-static -S . -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-D_WINSTATIC" -DTANGO_PKG_LIBRARY_DIRS="%BASE_FOLDER%\\lib" -DTANGO_PKG_LIBRARIES="libtango;omniORB4;omnithread;COS4;omniDynamic4;libzmq-v141-mt-s-4_0_5;comctl32;wsock32;Ws2_32" -DTANGO_PKG_INCLUDE_DIRS="%BASE_FOLDER%\\include;%BASE_FOLDER%\\include\\tango"
    cmake --build build-static --config Release
```

For 32-bit replace `-A x64` with `-A Win32`.
