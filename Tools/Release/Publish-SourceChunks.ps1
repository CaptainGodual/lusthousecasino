[CmdletBinding()]
param(
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [string]$PreparedDirectory
)
$ErrorActionPreference = 'Stop'
if (-not $PreparedDirectory) {
    $PreparedDirectory = Join-Path $ProjectRoot 'Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload'
}
$uploadTool = Join-Path $PSScriptRoot 'chunked-source-upload.js'
& node $uploadTool publish --project-root $ProjectRoot --prepared $PreparedDirectory
if ($LASTEXITCODE -ne 0) {
    throw 'Upload stopped. The original source and commit are preserved. Resolve the reported error, then run this same command to resume.'
}
Write-Output 'Each source commit was verified on origin. The active branch now uses the split LFS history; the original commit is retained on a local backup branch.'
