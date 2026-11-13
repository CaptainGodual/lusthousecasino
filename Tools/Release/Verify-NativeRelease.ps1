[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputDirectory,
    [string]$NodePath,
    [switch]$CheckRuntime
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Resolve-NodeRuntime.ps1')
$nativeRuntime = Resolve-ProjectNodeRuntime -NodePath $NodePath
if ($CheckRuntime) { Write-Output "Node runtime: $($nativeRuntime.Path) ($($nativeRuntime.Version))"; return }
$nativeProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $nativeProjectRoot 'Saved/ReleaseAudit/REL02-2026-11-11/native-current' }
$nativeVerifier = Join-Path $PSScriptRoot 'Verify-Project.ps1'
$nativeExit = 0
Push-Location -LiteralPath $nativeProjectRoot
try {
    $nativeFingerprint = & $nativeRuntime.Path -e "process.stdout.write(require('./Tools/Release/release-policy').fingerprint(process.cwd()))"
    if ($LASTEXITCODE -ne 0 -or $nativeFingerprint -notmatch '^[a-f0-9]{64}$') { throw 'Cannot fingerprint the native verification source.' }
    foreach ($nativeMode in @('EditorBuild','ShippingBuild','EditorTests')) {
        & $nativeVerifier -Mode $nativeMode -EngineRoot $EngineRoot -OutputDirectory $OutputDirectory -SourceFingerprint $nativeFingerprint -NodePath $nativeRuntime.Path
        if ($LASTEXITCODE -ne 0) { $nativeExit=$LASTEXITCODE; break }
    }
    & $nativeRuntime.Path (Join-Path $PSScriptRoot 'audit-release.js') --unreal-evidence $OutputDirectory
    if ($LASTEXITCODE -eq 1 -and $nativeExit -eq 0) { $nativeExit=1 }
    if ($nativeExit -eq 0) { Write-Output 'Editor build, Shipping compilation, and all required Unreal automation tests passed. Review the master board for remaining release acceptance.' }
    else { Write-Output 'Native verification is incomplete. Inspect the evidence and logs in the output directory; completed builds are retained.' }
}
finally { Pop-Location }
exit $nativeExit
