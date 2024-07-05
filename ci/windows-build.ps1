. $PSScriptRoot/windows-lib.ps1

$CMAKE_BUILD_PARALLEL_LEVEL=$env:NUMBER_OF_PROCESSORS
# avoid cmake warning about unknown escape sequences
$cwd = $(pwd | Convert-Path).Replace("\", "/")

Write-Host "== Get ZeroMQ" -ForegroundColor Blue
$FILENAME="zmq-${ZMQ_VERSION}_${VC_ARCH_VER}.zip"
Invoke-NativeCommand curl.exe -JOL https://github.com/tango-controls/zmq-windows-ci/releases/download/${ZMQ_VERSION}/${FILENAME}
md -Force ${TANGO_ZMQ_ROOT}
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_ZMQ_ROOT}

Write-Host "== Get omniORB" -ForegroundColor Blue
$FILENAME="omniorb-${OMNI_VERSION}_${VC_ARCH_VER}_${PYVER}.zip"
Invoke-NativeCommand curl.exe -JOL https://github.com/tango-controls/omniorb-windows-ci/releases/download/${OMNI_VERSION}/${FILENAME}
md -Force ${TANGO_OMNI_ROOT}
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_OMNI_ROOT}

Write-Host "== Get nasm" -ForegroundColor Blue
Invoke-NativeCommand curl.exe -L ${NASM_DOWNLOAD_LINK} -o nasm.exe
Invoke-NativeCommand ./nasm.exe /S /v/qn

Write-Host "== Build jpeg" -ForegroundColor Blue
$FILENAME="libjpeg-turbo-${JPEG_VERSION}.zip"
Invoke-NativeCommand curl.exe -L https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/${JPEG_VERSION}.zip -o ${FILENAME}
md -Force ${TANGO_JPEG_SOURCE}/..
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_JPEG_SOURCE}/..
Invoke-NativeCommand cmake `
  -S "${TANGO_JPEG_SOURCE}" `
  -B "${TANGO_JPEG_SOURCE}/build" `
  -G "${CMAKE_GENERATOR}" `
  -A "${ARCHITECTURE}" `
  -DWITH_TURBOJPEG=ON `
  -DCMAKE_INSTALL_PREFIX="${TANGO_JPEG_ROOT}" `
  -DCMAKE_DEBUG_POSTFIX=d
Invoke-NativeCommand cmake `
  --build "${TANGO_JPEG_SOURCE}/build" `
  --target install `
  --config "${CMAKE_BUILD_TYPE}"

Write-Host "== Get OTEL" -ForegroundColor Blue
$FILENAME="opentelemetry-with-deps-static-${ARCHITECTURE}.zip"
Invoke-NativeCommand curl.exe -JOL "https://gitlab.com/api/v4/projects/54003303/packages/generic/opentelemetry/${OTEL_VERSION}/${FILENAME}"
md -Force ${OTEL_ROOT}
Expand-Archive -Path ${FILENAME} -DestinationPath ${OTEL_ROOT}
mv ${OTEL_ROOT}/opentelemetry-cpp*/* ${OTEL_ROOT}
rm -Recurse ${OTEL_ROOT}/opentelemetry-cpp*
$ZLIB_NG_ROOT="${OTEL_ROOT}/zlib-ng"

Write-Host "== Build IDL" -ForegroundColor Blue
md -Force "${TANGO_IDL_SOURCE}"
Invoke-NativeCommand git clone -b ${TANGO_IDL_TAG} --depth 1 --quiet https://gitlab.com/tango-controls/tango-idl ${TANGO_IDL_SOURCE}
Invoke-NativeCommand cmake `
  -S "${TANGO_IDL_SOURCE}" `
  -B "${TANGO_IDL_SOURCE}/build" `
  -G "${CMAKE_GENERATOR}" `
  -A "${ARCHITECTURE}" `
  -DCMAKE_INSTALL_PREFIX="${TANGO_IDL_ROOT}"
Invoke-NativeCommand cmake `
  --build "${TANGO_IDL_SOURCE}/build" `
  --target install `
  --config "${CMAKE_BUILD_TYPE}"

Write-Host "== Build Catch" -ForegroundColor Blue
$FILENAME="catch-${CATCH_VERSION}.zip"
Invoke-NativeCommand curl.exe -L https://github.com/catchorg/Catch2/archive/refs/tags/v${CATCH_VERSION}.zip -o ${FILENAME}
md -Force ${TANGO_CATCH_SOURCE}/..
Expand-Archive -Path ${FILENAME} -DestinationPath ${TANGO_CATCH_SOURCE}/..
# The catch CMakeList.txt does not support CMAKE_MSVC_RUNTIME_LIBRARY so we have to do it manually ourselves.
$ADDITIONAL_ARGS=@()
if ($BUILD_SHARED_LIBS -eq "OFF") {
    $ADDITIONAL_ARGS+="-DCMAKE_CXX_FLAGS_DEBUG=/MTd /Zi /Ob0 /Od /RTC1 "
    $ADDITIONAL_ARGS+="-DCMAKE_CXX_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG "
}
Invoke-NativeCommand cmake `
  -S "${TANGO_CATCH_SOURCE}" `
  -B "${TANGO_CATCH_SOURCE}/build" `
  -G "${CMAKE_GENERATOR}" `
  -A "${ARCHITECTURE}" `
  -DCMAKE_INSTALL_PREFIX="${TANGO_CATCH_ROOT}" `
  -DCATCH_INSTALL_DOCS=OFF `
  -DCATCH_BUILD_TESTING=OFF `
  -DCATCH_ENABLE_WERROR=OFF `
  @ADDITIONAL_ARGS
Invoke-NativeCommand cmake `
  --build "${TANGO_CATCH_SOURCE}/build" `
  --target install `
  --config "${CMAKE_BUILD_TYPE}"

Write-Host "== Get WIX" -ForegroundColor Blue
$FILENAME="wix311-binaries.zip"
Invoke-NativeCommand curl.exe -JOL https://github.com/wixtoolset/wix3/releases/download/wix3112rtm/${FILENAME}
md -Force ${WIX_TOOLSET_LOCATION}
Expand-Archive -Path ${FILENAME} -DestinationPath ${WIX_TOOLSET_LOCATION}

Write-Host "== Get Python" -ForegroundColor Blue
$FILENAME="python-${PYTHON_VERSION}-embed-${PY_ARCH}.zip"
Invoke-NativeCommand curl.exe -JOL https://www.python.org/ftp/python/${PYTHON_VERSION}/${FILENAME}
md -FORCE ${PYTHON_LOCATION}
Expand-Archive -Path ${FILENAME} -DestinationPath ${PYTHON_LOCATION}
$env:Path += ";${cwd}/${PYTHON_LOCATION}"

Write-Host "== Build tango" -ForegroundColor Blue
Invoke-NativeCommand cmake `
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
  -DTANGO_INSTALL_DEPENDENCIES="${TANGO_INSTALL_DEPENDENCIES}" `
  -DomniORB4_ROOT="${cwd}/${TANGO_OMNI_ROOT}" `
  -DZeroMQ_ROOT="${cwd}/${TANGO_ZMQ_ROOT}" `
  -Dcppzmq_ROOT="${cwd}/${TANGO_CPPZMQ_ROOT}" `
  -DJPEG_ROOT="${cwd}/${TANGO_JPEG_ROOT}" `
  -DCatch2_ROOT="${cwd}/${TANGO_CATCH_ROOT}" `
  -DZLIB_ROOT="${cwd}/${ZLIB_NG_ROOT}" `
  -DTANGO_USE_JPEG=ON `
  -DJPEG_DEBUG_POSTFIX=d `
  -DBUILD_TESTING="${BUILD_TESTING}" `
  -DTANGO_USE_TELEMETRY="${TANGO_USE_TELEMETRY}" `
  -DCMAKE_PREFIX_PATH="${OTEL_ROOT}/cmake;${OTEL_ROOT}/lib/cmake;${OTEL_ROOT}/share/cmake"
Invoke-NativeCommand cmake `
  --build build `
  --config "${CMAKE_BUILD_TYPE}"

# We only want to make packages with bundled dependencies
if ($TANGO_INSTALL_DEPENDENCIES -eq "ON") {
  Write-Host "== Create archive" -ForegroundColor Blue
  # -B not working here, so we have to change directories
  Push-Location build
  # space after `-D` is required
  Invoke-NativeCommand cpack -D CPACK_WIX_ROOT="${cwd}/${WIX_TOOLSET_LOCATION}" -C "${CMAKE_BUILD_TYPE}" -G "WIX;ZIP"
  Pop-Location
}
