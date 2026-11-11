param([switch]$GeneratePassword)
$ErrorActionPreference = 'Stop'
$keyTool = 'C:\Program Files\Microsoft\jdk-21.0.13.21-hotspot\bin\keytool.exe'
if (-not (Test-Path -LiteralPath $keyTool)) { throw 'Java 21 keytool was not found. No signing files were changed.' }
$signingFolder = Join-Path ([Environment]::GetFolderPath('UserProfile')) '.lusthouse-signing'
$keyPath = Join-Path $signingFolder 'lusthouse-release.keystore'
$credentialPath = Join-Path $signingFolder 'release-password.clixml'
$oldPassword = $env:LUSTHOUSE_SIGNING_PASSWORD
try {
    Write-Host 'This creates your permanent Android release identity outside the game project.'
    Write-Host 'It does not build, upload or install the game. Existing keys are never replaced.'
    if (Test-Path -LiteralPath $keyPath) {
        Write-Host ('Existing release key retained: ' + $keyPath)
        Write-Host 'No new key was created. Keep using this key for all public Android updates.'
        return
    }
    if ($GeneratePassword) {
        $bytes = New-Object byte[] 24
        $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
        try { $rng.GetBytes($bytes) } finally { $rng.Dispose() }
        $plain = [Convert]::ToBase64String($bytes)
        $securePassword = ConvertTo-SecureString $plain -AsPlainText -Force
        $repeat = $plain
    } else {
        $securePassword = Read-Host 'Choose a release-key password (at least 12 characters; hidden)' -AsSecureString
        $confirm = Read-Host 'Enter the same password again (hidden)' -AsSecureString
        $plain = [System.Net.NetworkCredential]::new('', $securePassword).Password
        $repeat = [System.Net.NetworkCredential]::new('', $confirm).Password
    }
    if ($plain.Length -lt 12 -or $plain -ne $repeat) { throw 'Passwords must match and contain at least 12 characters. Nothing created.' }
    New-Item -ItemType Directory -Path $signingFolder -Force | Out-Null
    $env:LUSTHOUSE_SIGNING_PASSWORD = $plain
    & $keyTool -genkeypair -keystore $keyPath -storetype JKS -alias lusthouse-release -keyalg RSA -keysize 3072 -validity 10000 -dname 'CN=Lust House Casino' -storepass:env LUSTHOUSE_SIGNING_PASSWORD -keypass:env LUSTHOUSE_SIGNING_PASSWORD -noprompt
    if ($LASTEXITCODE -ne 0) { throw 'Key creation failed. No replacement was attempted; preserve any existing key file.' }
    [System.Management.Automation.PSCredential]::new('lusthouse-release', $securePassword) | Export-Clixml -LiteralPath $credentialPath
    Write-Host ('RELEASE KEY READY: ' + $keyPath)
    Write-Host 'Alias: lusthouse-release'
    Write-Host 'The password copy is encrypted for your Windows account, outside the project.'
    Write-Host 'Back up this keystore separately and retain its password before publishing.'
    Write-Host 'A release-signed APK cannot install over your earlier debug-signed APK. Do not uninstall until guest-account access is recoverable.'
} finally {
    $env:LUSTHOUSE_SIGNING_PASSWORD = $oldPassword
    $plain = $null; $repeat = $null; $securePassword = $null; $confirm = $null
}
