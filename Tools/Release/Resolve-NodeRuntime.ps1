function Resolve-ProjectNodeRuntime {
    [CmdletBinding()]
    param([string]$NodePath)
    if ($NodePath -and -not (Test-Path -LiteralPath $NodePath -PathType Leaf)) {
        throw "The specified Node executable does not exist: $NodePath"
    }
    $runtimeCandidates = @($NodePath, $env:CODEX_PRIMARY_RUNTIME_NODE)
    $runtimeCommand = Get-Command node -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($runtimeCommand) { $runtimeCandidates += $runtimeCommand.Source }
    foreach ($runtimeLocation in @(
        @($env:ProgramFiles, 'nodejs\node.exe'),
        @($env:LOCALAPPDATA, 'Programs\nodejs\node.exe'),
        @($env:USERPROFILE, '.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe'),
        @($env:NVM_SYMLINK, 'node.exe')
    )) {
        if ($runtimeLocation[0]) { $runtimeCandidates += Join-Path $runtimeLocation[0] $runtimeLocation[1] }
    }
    $runtimeExecutable = $runtimeCandidates | Where-Object {
        $_ -and (Test-Path -LiteralPath $_ -PathType Leaf)
    } | Select-Object -First 1
    if (-not $runtimeExecutable) {
        throw 'Node.js was not found on PATH or in the Codex runtime. Supply -NodePath with the full path to node.exe.'
    }
    $runtimeVersion = & $runtimeExecutable --version
    if ($LASTEXITCODE -ne 0) { throw "Node could not start: $runtimeExecutable" }
    return [pscustomobject]@{ Path = [IO.Path]::GetFullPath($runtimeExecutable); Version = $runtimeVersion }
}
