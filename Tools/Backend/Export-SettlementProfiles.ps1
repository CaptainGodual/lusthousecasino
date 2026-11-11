$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$oldTitle = $env:PLAYFAB_TITLE_ID
$oldKey = $env:PLAYFAB_SECRET_KEY
Push-Location -LiteralPath $projectRoot
try {
    if (Test-Path -LiteralPath 'Saved/SettlementMigration/profiles.json') {
        throw 'An export already exists. It was left unchanged; do not export over your frozen backup.'
    }
    if (-not (Test-Path -LiteralPath 'Saved/SettlementMigration/players.json')) {
        throw 'The player list is missing. No server changes were made.'
    }
    $env:PLAYFAB_TITLE_ID = '16CECB'
    Write-Host 'EXPORT ONLY: this reads PlayFab and saves a local backup. It does not migrate balances.'
    Write-Host 'Keep gameplay stopped. Enter the PlayFab TITLE SECRET KEY, not the Nakama HTTP key.'
    $enteredKey = Read-Host 'Paste PlayFab title secret key and press Enter (input is hidden)' -AsSecureString
    $env:PLAYFAB_SECRET_KEY = [System.Net.NetworkCredential]::new('', $enteredKey).Password
    if ([string]::IsNullOrWhiteSpace($env:PLAYFAB_SECRET_KEY)) {
        throw 'No key was entered. Paste the key at the hidden prompt and press Enter.'
    }
    & node './Tools/Backend/stage-economy-migration.js' --players './Saved/SettlementMigration/players.json' --export './Saved/SettlementMigration/profiles.json'
    if ($LASTEXITCODE -ne 0) { throw 'Export did not complete. The error immediately above explains why.' }
} finally {
    $env:PLAYFAB_TITLE_ID = $oldTitle
    $env:PLAYFAB_SECRET_KEY = $oldKey
    $enteredKey = $null
    Pop-Location
}
