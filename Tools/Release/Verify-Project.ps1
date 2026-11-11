[CmdletBinding()]
param(
    [ValidateSet('EditorBuild','EditorTests','WindowsCheckpoint')][string]$Mode = 'EditorBuild',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputDirectory,
    [string]$SourceFingerprint
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot ('Saved/ReleaseAudit/run-' + (Get-Date -Format 'yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputDirectory)
$prefix = $projectRoot.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)) { throw 'Verification output must remain inside the project.' }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$oldUba = $env:UBA_ROOT
$oldUatLogs = $env:uebp_LogFolder
$oldEngineSaved = $env:uebp_EngineSavedFolder
$started = [DateTime]::UtcNow
$result = [ordered]@{ Mode=$Mode; Status='blocked'; SourceFingerprint=$SourceFingerprint; StartedAt=$started.ToString('o'); CheckedAt=$null; ExitCode=$null; Summary='Verification did not complete'; Evidence=@(); ArtifactFingerprint=$null }
$exitCode = 2
Push-Location -LiteralPath $projectRoot
try {
    if (-not $SourceFingerprint) {
        $SourceFingerprint = & node -e "process.stdout.write(require('./Tools/Release/release-policy').fingerprint(process.cwd()))"
        if ($LASTEXITCODE -ne 0 -or $SourceFingerprint -notmatch '^[a-f0-9]{64}$') { throw 'Could not fingerprint the current source and assets.' }
        $result.SourceFingerprint=$SourceFingerprint
    }
    $env:UBA_ROOT = Join-Path $projectRoot 'Saved/ReleaseAudit/UBA'
    $env:uebp_LogFolder = Join-Path $output 'UAT'
    $env:uebp_EngineSavedFolder = Join-Path $output 'CommandletLogs'
    New-Item -ItemType Directory -Path $env:UBA_ROOT,$env:uebp_LogFolder,$env:uebp_EngineSavedFolder -Force | Out-Null
    $project = Join-Path $projectRoot 'SugarLustUE.uproject'
    if ($Mode -eq 'EditorBuild') {
        $log = Join-Path $output 'editor-build.log'
        $console = Join-Path $output 'editor-build-console.log'
        & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') SugarLustUEEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild -NoSNDBS -NoArtifactReads -NoArtifactWrites "-UBARootDir=$env:UBA_ROOT" "-Log=$log" *> $console
        $result.ExitCode = $LASTEXITCODE
        $success = Test-Path -LiteralPath $log
        $success = $success -and [bool](Select-String -LiteralPath $log -Pattern 'Result: Succeeded|Target is up to date' -Quiet)
        $result.Evidence = @($log,$console)
        if ($LASTEXITCODE -eq 0 -and $success) { $result.Status='passed'; $result.Summary='Current Unreal Editor target compiled successfully'; $exitCode=0 }
        else { $result.Summary='Unreal build did not produce explicit successful build evidence' }
    }
    elseif ($Mode -eq 'EditorTests') {
        $log = Join-Path $output 'editor-automation.log'
        $report = Join-Path $output 'Automation'
        $args = @('"'+$project+'"','-unattended','-nullrhi','-nosound','-nosplash','-nop4','-stdout','-FullStdOutLogOutput',('"-AbsLog='+$log+'"'),'"-ExecCmds=Automation RunTests SugarLust"','"-TestExit=Automation Test Queue Empty"',('"-ReportExportPath='+$report+'"'))
        $process = Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $output 'editor-test-stdout.log') -RedirectStandardError (Join-Path $output 'editor-test-stderr.log')
        if (-not $process.WaitForExit(180000)) { $process.Kill(); throw 'Editor automation timed out; no successful test run is assumed.' }
        $result.ExitCode=$process.ExitCode
        $index = Join-Path $report 'index.json'
        $result.Evidence=@($log,$index)
        if ((Test-Path -LiteralPath $index) -and (Get-Item -LiteralPath $index).LastWriteTimeUtc -ge $started) {
            $data=Get-Content -Raw -LiteralPath $index | ConvertFrom-Json
            if ($process.ExitCode -eq 0 -and $data.tests.Count -gt 0 -and @($data.tests | Where-Object { $_.state -notin 'Success','SuccessWithWarnings' }).Count -eq 0 -and $data.failed -eq 0 -and $data.notRun -eq 0) { $result.Status='passed'; $result.Summary=('{0} Unreal tests executed successfully' -f $data.tests.Count); $exitCode=0 }
            else { $result.Status='failed'; $result.Summary='Unreal automation report contains failed or unexecuted tests'; $exitCode=1 }
        } else { $result.Summary='Editor returned without a fresh nonempty automation report; execution remains unverified' }
    }
    else {
        $archive=Join-Path $projectRoot ('DemoBuilds/Checkpoint-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '/Windows')
        $log=Join-Path $output 'windows-package-console.log'
        & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Shipping -cook -build -stage -package -pak -iostore -archive "-archivedirectory=$archive" -prereqs -unattended -utf8output -nocompileeditor -SkipBuildEditor -nodebuginfo '-ubtargs=-NoArtifactReads -NoArtifactWrites -NoUBA' *> $log
        $result.ExitCode=$LASTEXITCODE
        $game=Join-Path $archive 'SugarLustUE/Binaries/Win64/SugarLustUE-Win64-Shipping.exe'
        $result.Evidence=@($log,$game)
        $containers=@(Get-ChildItem -LiteralPath $archive -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in '.pak','.utoc','.ucas' })
        if ($LASTEXITCODE -eq 0 -and (Test-Path -LiteralPath $game) -and $containers.Count -gt 0) {
            $files=@(Get-Item -LiteralPath $game) + $containers
            $manifest=@($files | Sort-Object FullName | ForEach-Object { [pscustomobject]@{ Path=[IO.Path]::GetRelativePath($archive,$_.FullName); Bytes=$_.Length; Sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash } })
            $manifestPath=Join-Path $output 'windows-artifact-manifest.json'
            $manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifestPath -Encoding utf8
            $result.ArtifactFingerprint=(Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
            $result.Evidence+=@($manifestPath)
            $result.Status='passed'; $result.Summary='New Windows Shipping checkpoint built; interactive gameplay acceptance remains separate'; $exitCode=0
        } else {
            $denial=Select-String -LiteralPath $log -Pattern 'UnauthorizedAccessException:|Access to the path .* is denied' | Select-Object -First 1
            $result.Summary=if($denial) { 'Windows staging or cooking blocked: '+$denial.Line.Trim() } else { 'Windows packaging did not produce a complete new executable and cooked content' }
            $uatLog=Join-Path $env:uebp_LogFolder 'Log.txt'
            if(Test-Path -LiteralPath $uatLog) { $result.Evidence+=@($uatLog) }
        }
    }
}
catch { $result.Summary=$_.Exception.Message; $result.Status='blocked' }
finally {
    $result.CheckedAt=[DateTime]::UtcNow.ToString('o')
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output ($Mode + '-evidence.json')) -Encoding utf8
    $env:UBA_ROOT=$oldUba; $env:uebp_LogFolder=$oldUatLogs; $env:uebp_EngineSavedFolder=$oldEngineSaved
    Pop-Location
}
$result | ConvertTo-Json -Depth 6
exit $exitCode
