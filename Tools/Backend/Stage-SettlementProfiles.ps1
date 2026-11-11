$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$oldUrl = $env:NAKAMA_URL
$oldKey = $env:NAKAMA_HTTP_KEY
Push-Location -LiteralPath $projectRoot
try {
    if (-not (Test-Path -LiteralPath 'Saved/SettlementMigration/profiles.json')) {
        throw 'The frozen backup is missing. No staging attempted.'
    }
    Write-Host 'This stages the saved backup in Nakama. It does not enable migration or overwrite an existing account.'
    Write-Host 'The new Nakama index.js must already be uploaded and Nakama restarted. Keep gameplay stopped.'
    $env:NAKAMA_URL = (Read-Host 'Paste SugarLustNakamaGatewayUrl from PlayFab Title Internal Data').Trim()
    $enteredKey = Read-Host 'Paste SugarLustNakamaHttpKey from Title Internal Data (hidden)' -AsSecureString
    $env:NAKAMA_HTTP_KEY = [System.Net.NetworkCredential]::new('', $enteredKey).Password
    if ([string]::IsNullOrWhiteSpace($env:NAKAMA_URL) -or [string]::IsNullOrWhiteSpace($env:NAKAMA_HTTP_KEY)) {
        throw 'A gateway URL and Nakama HTTP key are required. No staging attempted.'
    }
    & node './Tools/Backend/stage-economy-migration.js' --snapshot './Saved/SettlementMigration/profiles.json' --apply
    if ($LASTEXITCODE -ne 0) { throw 'Staging did not complete. Preserve the backup and check the error above.' }
} finally {
    $env:NAKAMA_URL = $oldUrl
    $env:NAKAMA_HTTP_KEY = $oldKey
    $enteredKey = $null
    Pop-Location
}
