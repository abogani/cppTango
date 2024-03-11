$CMAKE_BUILD_PARALLEL_LEVEL=$env:NUMBER_OF_PROCESSORS
# avoid cmake warning about unknown escape sequences
$cwd = $(pwd | Convert-Path).Replace("\\", "/")
# zmq/cppzmq
$FILENAME="zmq-${ZMQ_VERSION}_${VC_ARCH_VER}.zip"
curl.exe -JOL https://github.com/tango-controls/zmq-windows-ci/releases/download/${ZMQ_VERSION}/${FILENAME}
md -Force ${TANGO_ZMQ_ROOT}
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_ZMQ_ROOT}
# omniORB
$FILENAME="omniorb-${OMNI_VERSION}_${VC_ARCH_VER}_${PYVER}.zip"
curl.exe -JOL https://github.com/tango-controls/omniorb-windows-ci/releases/download/${OMNI_VERSION}/${FILENAME}
md -Force ${TANGO_OMNI_ROOT}
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_OMNI_ROOT}
# pthread
$FILENAME="pthreads-win32-${PTHREAD_VERSION}_${VC_ARCH_VER}.zip"
curl.exe -JOL https://github.com/tango-controls/Pthread_WIN32/releases/download/${PTHREAD_VERSION}/${FILENAME}
md -Force ${PTHREAD_ROOT}
Expand-Archive -Path ${FILENAME} -DestinationPath ${PTHREAD_ROOT}
# nasm (for libjpeg-turbo)
curl.exe -L ${NASM_DOWNLOAD_LINK} -o nasm.exe
./nasm.exe /S /v/qn
# libjpeg-turbo
$FILENAME="libjpeg-turbo-${JPEG_VERSION}.zip"
curl.exe -L https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/${JPEG_VERSION}.zip -o ${FILENAME}
md -Force ${TANGO_JPEG_SOURCE}/..
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_JPEG_SOURCE}/..
# Using multiline strings with `>` does not work with powershell and suprisingly plain multiline statements work
cmake `
  -S "${TANGO_JPEG_SOURCE}" `
  -B "${TANGO_JPEG_SOURCE}/build" `
  -G "${CMAKE_GENERATOR}" `
  -A "${ARCHITECTURE}" `
  -DWITH_TURBOJPEG=ON `
  -DCMAKE_INSTALL_PREFIX="${TANGO_JPEG_ROOT}" `
  -DCMAKE_DEBUG_POSTFIX=d
cmake `
  --build "${TANGO_JPEG_SOURCE}/build" `
  --target install `
  --config "${CMAKE_BUILD_TYPE}"
# opentelemetry with dependencies
$FILENAME="opentelemetry-with-deps-static-${ARCHITECTURE}.zip"
curl.exe -JOL "https://gitlab.com/api/v4/projects/54003303/packages/generic/opentelemetry/${OTEL_VERSION}/${FILENAME}"
md -Force ${OTEL_ROOT}
Expand-Archive -Path ${FILENAME} -DestinationPath ${OTEL_ROOT}
mv ${OTEL_ROOT}/opentelemetry-cpp*/* ${OTEL_ROOT}
rm -Recurse ${OTEL_ROOT}/opentelemetry-cpp*
$ZLIB_NG_ROOT="${OTEL_ROOT}/zlib-ng"
$LIBCURL_ROOT="${OTEL_ROOT}/libcurl"
$OPENSSL_ROOT="${LIBCURL_ROOT}"
# tango idl
md -Force "${TANGO_IDL_SOURCE}"
git clone -b ${TANGO_IDL_TAG} --depth 1 --quiet https://gitlab.com/tango-controls/tango-idl ${TANGO_IDL_SOURCE}
cmake `
  -S "${TANGO_IDL_SOURCE}" `
  -B "${TANGO_IDL_SOURCE}/build" `
  -G "${CMAKE_GENERATOR}" `
  -A "${ARCHITECTURE}" `
  -DCMAKE_INSTALL_PREFIX="${TANGO_IDL_ROOT}"
cmake `
  --build "${TANGO_IDL_SOURCE}/build" `
  --target install `
  --config "${CMAKE_BUILD_TYPE}"
# catch
$FILENAME="catch-${CATCH_VERSION}.zip"
curl.exe -L https://github.com/catchorg/Catch2/archive/refs/tags/v${CATCH_VERSION}.zip -o ${FILENAME}
md -Force ${TANGO_CATCH_SOURCE}/..
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_CATCH_SOURCE}/..
cmake `
  -S "${TANGO_CATCH_SOURCE}" `
  -B "${TANGO_CATCH_SOURCE}/build" `
  -G "${CMAKE_GENERATOR}" `
  -A "${ARCHITECTURE}" `
  -DCMAKE_INSTALL_PREFIX="${TANGO_CATCH_ROOT}" `
  -DCATCH_INSTALL_DOCS=OFF `
  -DCATCH_BUILD_TESTING=OFF `
  -DCATCH_ENABLE_WERROR=OFF
cmake `
  --build "${TANGO_CATCH_SOURCE}/build" `
  --target install `
  --config "${CMAKE_BUILD_TYPE}"
# wix toolset
$FILENAME="wix311-binaries.zip"
curl.exe -JOL https://github.com/wixtoolset/wix3/releases/download/wix3112rtm/${FILENAME}
md -Force ${WIX_TOOLSET_LOCATION}
Expand-Archive -Path ${FILENAME} -DestinationPath ${WIX_TOOLSET_LOCATION}
# python (required for building the tests)
$FILENAME="python-${PYTHON_VERSION}-embed-${PY_ARCH}.zip"
curl.exe -JOL https://www.python.org/ftp/python/${PYTHON_VERSION}/${FILENAME}
md -FORCE ${PYTHON_LOCATION}
Expand-Archive -Path ${FILENAME} -DestinationPath ${PYTHON_LOCATION}
$env:Path += ";${cwd}/${PYTHON_LOCATION}"
cmake `
  -S . `
  -B  build `
  -G "${CMAKE_GENERATOR}" `
  -A "${ARCHITECTURE}" `
  -DCMAKE_VERBOSE_MAKEFILE=ON `
  -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON `
  -DBUILD_SHARED_LIBS="${BUILD_SHARED_LIBS}" `
  -DTANGO_WARNINGS_AS_ERRORS="${TANGO_WARNINGS_AS_ERRORS}" `
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" `
  -Dtangoidl_ROOT="${cwd}/${TANGO_IDL_ROOT}" `
  -DTANGO_INSTALL_DEPENDENCIES=ON `
  -DomniORB4_ROOT="${cwd}/${TANGO_OMNI_ROOT}" `
  -DZeroMQ_ROOT="${cwd}/${TANGO_ZMQ_ROOT}" `
  -Dcppzmq_ROOT="${cwd}/${TANGO_CPPZMQ_ROOT}" `
  -DJPEG_ROOT="${cwd}/${TANGO_JPEG_ROOT}" `
  -Dpthread_ROOT="${PTHREAD_ROOT}" `
  -DCatch2_ROOT="${cwd}/${TANGO_CATCH_ROOT}" `
  -DTANGO_USE_PTHREAD=ON `
  -DTANGO_USE_JPEG=ON `
  -DJPEG_DEBUG_POSTFIX=d `
  -DBUILD_TESTING="${BUILD_TESTING}" `
  -DTANGO_USE_TELEMETRY="${TANGO_USE_TELEMETRY}" `
  -DCMAKE_PREFIX_PATH="${OTEL_ROOT}/cmake;${OTEL_ROOT}/lib/cmake;${OTEL_ROOT}/share/cmake" `
  -DCMAKE_MODULE_PATH="${LIBCURL_ROOT}" `
  -DZLIB_INCLUDE_DIR="${cwd}/${ZLIB_NG_ROOT}/include" `
  -DZLIB_LIBRARY="${cwd}/${ZLIB_NG_ROOT}/lib/zlib.lib" `
  -DCURL_INCLUDE_DIR="${cwd}/${LIBCURL_ROOT}/include" `
  -DCURL_LIBRARY="${cwd}/${LIBCURL_ROOT}/lib/libcurl.a" `
  -DOPENSSL_USE_STATIC_LIBS=TRUE `
  -DOPENSSL_MSVC_STATIC_RT="ON" `
  -DOPENSSL_ROOT_DIR="${cwd}/${OPENSSL_ROOT}" `
  -DTANGO_OTEL_ROOT="${cwd}/${OTEL_ROOT}"
cmake `
  --build build `
  --config "${CMAKE_BUILD_TYPE}"
# -B not working here, so we have to use cd
cd build
# space after `-D` is required
cpack -D CPACK_WIX_ROOT="${cwd}/${WIX_TOOLSET_LOCATION}" -C "${CMAKE_BUILD_TYPE}" -G "WIX;ZIP"
