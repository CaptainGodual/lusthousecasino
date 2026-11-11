$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$uat = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
Push-Location -LiteralPath $projectRoot
try {
    & $uat BuildCookRun "-project=$projectRoot/SugarLustUE.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -cook -build -stage -package -pak -iostore -archive "-archivedirectory=$projectRoot/DemoBuilds/v0.0.2/Windows" -prereqs -unattended -utf8output -nocompileeditor -SkipBuildEditor -nodebuginfo
    if ($LASTEXITCODE -ne 0) { throw 'Windows packaging failed. Check the error above.' }
} finally { Pop-Location }
