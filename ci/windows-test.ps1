. $PSScriptRoot/windows-lib.ps1

$cwd = $(pwd | Convert-Path).Replace("\", "/")

# These tests are not run from anything that is installed,
# so let's only run them once
if ($TANGO_INSTALL_DEPENDENCIES -eq "ON") {
    Write-Host "== Run Catch2 tests"
    try {
        Invoke-NativeCommand ctest `
            --output-on-failure `
            --test-dir build `
            -C ${CMAKE_BUILD_TYPE} `
            -R "^catch2.*$"
    } catch {
        Write-Host "== Rerunning failed tests"
        Invoke-NativeCommand ctest `
            --output-on-failure `
            --test-dir build `
            -C ${CMAKE_BUILD_TYPE} `
            --rerun-failed
    }
}

Write-Host "== Test installation"
$prefix = "$cwd/prefix"
Invoke-NativeCommand cmake `
    --install build `
    --config "${CMAKE_BUILD_TYPE}" `
    --prefix "$prefix"

$builddir = "test-build"
if ($BUILD_SHARED_LIBS -eq "ON") {
    $RuntimeLibrary = "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
} else {
    $RuntimeLibrary = "MultiThreaded$<$<CONFIG:Debug>:Debug>"
}

$additional_args = @()
if ($TANGO_INSTALL_DEPENDENCIES -eq "OFF") {
  $additional_args += "-DomniORB4_ROOT=${cwd}/${TANGO_OMNI_ROOT}"
  $additional_args += "-DZeroMQ_ROOT=${cwd}/${TANGO_ZMQ_ROOT}"
  $additional_args += "-Dcppzmq_ROOT=${cwd}/${TANGO_CPPZMQ_ROOT}"
  $additional_args += "-DJPEG_ROOT=${cwd}/${TANGO_JPEG_ROOT}"
  $additional_args += "-DZLIB_ROOT=${cwd}/${OTEL_ROOT}/zlib-ng"
  $additional_args += "-DCMAKE_PREFIX_PATH=${cwd}/${OTEL_ROOT}/cmake;${cwd}/${OTEL_ROOT}/lib/cmake;${cwd}/${OTEL_ROOT}/share/cmake"
}

Invoke-NativeCommand cmake `
    -B $builddir `
    -S tests/cmake_config `
    -G "${CMAKE_GENERATOR}" `
    -A "${ARCHITECTURE}" `
    -DCMAKE_MSVC_RUNTIME_LIBRARY="$RuntimeLibrary" `
    -DTango_ROOT="$prefix" `
    @additional_args

Invoke-NativeCommand cmake `
    --build $builddir `
    --config ${CMAKE_BUILD_TYPE}

$env:PATH += ";$prefix/bin"
if ($TANGO_INSTALL_DEPENDENCIES -eq "OFF") {
    $env:PATH += ";${cwd}/${TANGO_OMNI_ROOT}/bin/x86_win32"
    $env:PATH += ";${cwd}/${TANGO_ZMQ_ROOT}/bin/${CMAKE_BUILD_TYPE}"
    $env:PATH += ";${cwd}/${TANGO_JPEG_ROOT}/bin"

    # Even though we only have OTEL with LIBRARY_TYPE=Static, we still end up linking to
    # zlib.dll which is found here:
    $env:PATH += ";${cwd}/${OTEL_ROOT}/zlib-ng/bin"
}
$output = (& $builddir/${CMAKE_BUILD_TYPE}/dummy 2>&1)
$result = $LASTEXITCODE

if ($result -ne -1) {
    throw "Unexpected return code from dummy: $result"
}

if (!$output.ToString().StartsWith("usage")) {
    throw "Unexpected output from dummy: $output"
}
