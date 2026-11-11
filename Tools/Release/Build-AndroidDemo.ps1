$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
$signingFolder = Join-Path ([Environment]::GetFolderPath('UserProfile')) '.lusthouse-signing'
$keyPath = Join-Path $signingFolder 'lusthouse-release.keystore'
$credentialPath = Join-Path $signingFolder 'release-password.clixml'
$overlay = Join-Path $projectRoot 'Config/UserEngine.ini'
$hadOverlay = Test-Path -LiteralPath $overlay
$oldOverlay = if ($hadOverlay) { [System.IO.File]::ReadAllBytes($overlay) } else { $null }
$oldJava = $env:JAVA_HOME
$password = $null
Push-Location -LiteralPath $projectRoot
try {
    if (-not (Test-Path -LiteralPath $keyPath) -or -not (Test-Path -LiteralPath $credentialPath)) { throw 'Prepare the private Android release key first with Tools/Release/Prepare-AndroidReleaseKey.ps1.' }
    $credential = Import-Clixml -LiteralPath $credentialPath
    $password = $credential.GetNetworkCredential().Password
    if ([string]::IsNullOrEmpty($password) -or $password.Contains("'") -or $password.Contains("`r") -or $password.Contains("`n")) { throw 'The signing password cannot be passed safely to this Unreal Gradle generator.' }
    $env:JAVA_HOME = 'C:\Program Files\Microsoft\jdk-21.0.13.21-hotspot'
    $store = $keyPath.Replace('\','/')
    $existing = if ($hadOverlay) { [System.IO.File]::ReadAllText($overlay) } else { '' }
    $config = $existing + "`n[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]`nKeyStore=$store`nKeyAlias=lusthouse-release`nKeyStorePassword=$password`nKeyPassword=$password`n"
    [System.IO.File]::WriteAllText($overlay,$config,(New-Object System.Text.UTF8Encoding($false)))
    Write-Host 'Building Android ASTC Shipping for distribution. Passwords are not command-line arguments.'
    $destination = Join-Path $projectRoot 'DemoBuilds/v0.0.2/Android'
    & (Join-Path $engineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$projectRoot/SugarLustUE.uproject" -noP4 -platform=Android -cookflavor=ASTC -clientconfig=Shipping -distribution -cook -build -stage -package -pak -iostore -archive "-archivedirectory=$destination" -unattended -utf8output -nocompileeditor -SkipBuildEditor -nodebuginfo
    if ($LASTEXITCODE -ne 0) { throw 'Android packaging failed. The signing identity is preserved; see the build error above.' }
    Write-Host ('ANDROID PACKAGE COMPLETE: ' + $destination)
} finally {
    if ($hadOverlay) { [System.IO.File]::WriteAllBytes($overlay,$oldOverlay) }
    elseif (Test-Path -LiteralPath $overlay) { Remove-Item -LiteralPath $overlay }
    # Unreal generates temporary Gradle signing files. Remove the password from
    # those generated text files after packaging; the next build regenerates them.
    $generatedRoot = [System.IO.Path]::GetFullPath((Join-Path $projectRoot 'Intermediate/Android'))
    if ($password -and (Test-Path -LiteralPath $generatedRoot)) {
        Get-ChildItem -LiteralPath $generatedRoot -File -Recurse | Where-Object { $_.Extension -in '.gradle','.properties','.ini','.xml','.json','.txt' -and $_.Length -lt 16777216 } | ForEach-Object {
            $resolved = [System.IO.Path]::GetFullPath($_.FullName)
            if (-not $resolved.StartsWith($generatedRoot + [System.IO.Path]::DirectorySeparatorChar,[System.StringComparison]::OrdinalIgnoreCase)) { throw 'Generated signing cleanup escaped its intended directory.' }
            $text = [System.IO.File]::ReadAllText($resolved)
            if ($text.Contains($password)) { [System.IO.File]::WriteAllText($resolved,$text.Replace($password,'SIGNING_SECRET_REMOVED'),(New-Object System.Text.UTF8Encoding($false))) }
        }
    }
    $password = $null; $config = $null; $credential = $null
    $env:JAVA_HOME = $oldJava
    Pop-Location
}
