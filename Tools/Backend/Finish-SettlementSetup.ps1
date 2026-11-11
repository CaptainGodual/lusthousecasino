$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$oldTitle = $env:PLAYFAB_TITLE_ID
$oldKey = $env:PLAYFAB_SECRET_KEY
Push-Location -LiteralPath $projectRoot
try {
    if (-not (Get-Command node -ErrorAction SilentlyContinue)) { throw 'Node.js is not available in this PowerShell session.' }
    if (-not (Test-Path -LiteralPath 'Saved/SettlementMigration/profiles.json')) { throw 'The frozen backup is missing. No server changes made.' }
    Write-Host 'Keep gameplay stopped while this finishes setup.'
    Write-Host 'This reads your existing server settings, verifies your staged balance, and enables migration.'
    Write-Host 'It does not reset accounts, overwrite the backup, or perform spins.'
    $enteredKey = Read-Host 'Paste your PLAYFAB TITLE SECRET KEY and press Enter (hidden)' -AsSecureString
    $env:PLAYFAB_SECRET_KEY = [System.Net.NetworkCredential]::new('', $enteredKey).Password
    $env:PLAYFAB_TITLE_ID = '16CECB'
    if ([string]::IsNullOrWhiteSpace($env:PLAYFAB_SECRET_KEY)) { throw 'No key entered. Run again and paste your PlayFab title secret key at the hidden prompt.' }
    & node './Tools/Backend/finish-settlement-setup.js'
    if ($LASTEXITCODE -ne 0) { throw 'Setup stopped safely. The explanation is directly above; preserve your backup and do not reset the profile.' }
} finally {
    $env:PLAYFAB_TITLE_ID = $oldTitle
    $env:PLAYFAB_SECRET_KEY = $oldKey
    $enteredKey = $null
    Pop-Location
}
