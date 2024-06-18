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
