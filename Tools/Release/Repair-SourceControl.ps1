[CmdletBinding()]
param(
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [string]$PreparedDirectory
)
$ErrorActionPreference='Stop'
if(-not $PreparedDirectory) { $PreparedDirectory=Join-Path $ProjectRoot 'Saved/ReleaseAudit/REL01-2026-11-11/prepared-baseline' }
& node (Join-Path $PSScriptRoot 'source-control-baseline.js') apply --project-root $ProjectRoot --prepared $PreparedDirectory
if($LASTEXITCODE -ne 0) { throw 'Source-control repair did not complete. Preserve the reported state and inspect the error; no working-tree reset is performed.' }
Write-Output 'Git metadata verified. Working files and player saves were preserved.'
