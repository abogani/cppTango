# From https://stackoverflow.com/questions/47032005
function Invoke-NativeCommand() {
    if ($args.Count -eq 0) {
        throw "No arguments."
    }

    $command = $args[0]
    $commandArgs = @()

    if ($args.Count -gt 1) {
        $commandArgs = $args[1..($args.Count - 1)]
    }

    & $command $commandArgs
    $result = $LASTEXITCODE

    if ($result -ne 0) {
        throw "`"$command $commandArgs`" exited with exit code $result."
    }
}

if ($LIBRARY_TYPE -eq "Shared") {
    $BUILD_SHARED_LIBS = "ON"
} else {
    $BUILD_SHARED_LIBS = "OFF"
}

if ($TELEMETRY_USAGE -eq "With Otel") {
    $TANGO_USE_TELEMETRY = "ON"
} else {
    $TANGO_USE_TELEMETRY = "OFF"
}
