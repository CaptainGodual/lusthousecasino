[CmdletBinding()]
param(
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [string]$PreparedDirectory,
    [string]$NodePath,
    [switch]$CheckRuntime
)
$ErrorActionPreference = 'Stop'
if ($NodePath -and -not (Test-Path -LiteralPath $NodePath -PathType Leaf)) {
    throw "The specified Node executable does not exist: $NodePath"
}
$nodeCandidates = @($NodePath, $env:CODEX_PRIMARY_RUNTIME_NODE)
$nodeCommand = Get-Command node -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
if ($nodeCommand) { $nodeCandidates += $nodeCommand.Source }
foreach ($runtimeBase in @($env:ProgramFiles, $env:LOCALAPPDATA, $env:USERPROFILE, $env:NVM_SYMLINK)) {
    if (-not $runtimeBase) { continue }
    $runtimeSuffixes = if ($runtimeBase -eq $env:USERPROFILE) {
        @('.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe')
    } elseif ($runtimeBase -eq $env:LOCALAPPDATA) {
        @('Programs\nodejs\node.exe')
    } elseif ($runtimeBase -eq $env:NVM_SYMLINK) {
        @('node.exe')
    } else {
        @('nodejs\node.exe')
    }
    foreach ($runtimeSuffix in $runtimeSuffixes) {
        $nodeCandidates += Join-Path $runtimeBase $runtimeSuffix
    }
}
$nodeExecutable = $nodeCandidates | Where-Object {
    $_ -and (Test-Path -LiteralPath $_ -PathType Leaf)
} | Select-Object -First 1
if (-not $nodeExecutable) {
    throw 'Node.js was not found on PATH or in the Codex runtime. Run this command with -NodePath followed by the full path to node.exe.'
}
$nodeVersion = & $nodeExecutable --version
if ($LASTEXITCODE -ne 0) { throw "Node could not start: $nodeExecutable" }
if ($CheckRuntime) {
    Write-Output "Node runtime: $nodeExecutable ($nodeVersion)"
    return
}
if (-not $PreparedDirectory) {
    $PreparedDirectory = Join-Path $ProjectRoot 'Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload'
}
$uploadTool = Join-Path $PSScriptRoot 'chunked-source-upload.js'
& $nodeExecutable $uploadTool publish --project-root $ProjectRoot --prepared $PreparedDirectory
if ($LASTEXITCODE -ne 0) {
    throw 'Upload stopped. The original source and commit are preserved. Resolve the reported error, then run this same command to resume.'
}
Write-Output 'Each source commit was verified on origin. The active branch now uses the split LFS history; the original commit is retained on a local backup branch.'
