. $PSScriptRoot/windows-lib.ps1

$cwd = $(pwd | Convert-Path).Replace("\", "/")

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
Invoke-NativeCommand cmake `
    -B $builddir `
    -S tests/cmake_config `
    -G "${CMAKE_GENERATOR}" `
    -A "${ARCHITECTURE}" `
    -DCMAKE_MSVC_RUNTIME_LIBRARY="$RuntimeLibrary" `
    -DTango_ROOT="$prefix"
Invoke-NativeCommand cmake `
    --build $builddir `
    --config ${CMAKE_BUILD_TYPE}

$env:PATH += ";$prefix/bin"
$output = (& $builddir/${CMAKE_BUILD_TYPE}/dummy 2>&1)
$result = $LASTEXITCODE

if ($result -ne -1) {
    throw "Unexpected return code from dummy: $result"
}

if (!$output.ToString().StartsWith("usage")) {
    throw "Unexpected output from dummy: $output"
}
