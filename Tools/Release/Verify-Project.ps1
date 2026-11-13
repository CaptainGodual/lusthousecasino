[CmdletBinding()]
param(
    [ValidateSet('EditorBuild','ShippingBuild','EditorTests','WindowsCheckpoint')][string]$Mode = 'EditorBuild',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputDirectory,
    [string]$SourceFingerprint,
    [string]$NodePath,
    [switch]$CheckRuntime,
    [int]$AutomationTimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Resolve-NodeRuntime.ps1')
$nodeRuntime = Resolve-ProjectNodeRuntime -NodePath $NodePath
$nodeExecutable = $nodeRuntime.Path
if ($CheckRuntime) { Write-Output "Node runtime: $nodeExecutable ($($nodeRuntime.Version))"; return }
if ($AutomationTimeoutSeconds -lt 1) { throw 'Automation timeout must be positive.' }
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
$result = [ordered]@{ Mode=$Mode; Status='blocked'; SourceFingerprint=$SourceFingerprint; StartedAt=$started.ToString('o'); CheckedAt=$null; ExitCode=$null; Summary='Verification did not complete'; Evidence=@(); ArtifactFingerprint=$null; NodeVersion=$nodeRuntime.Version; EngineVersion=$null; SourceCommit=$null; Toolchain=$null; Warnings=@(); ExpectedTests=@() }
$exitCode = 2
$verificationSourceReady = $false
Push-Location -LiteralPath $projectRoot
try {
    Write-Output "Checking source identity for $Mode..."
    $actualFingerprint = & $nodeExecutable -e "process.stdout.write(require('./Tools/Release/release-policy').fingerprint(process.cwd()))"
    if ($LASTEXITCODE -ne 0 -or $actualFingerprint -notmatch '^[a-f0-9]{64}$') { throw 'Could not fingerprint the current source and assets.' }
    if ($SourceFingerprint -and $SourceFingerprint -ne $actualFingerprint) { throw 'The supplied source fingerprint does not match the current source.' }
    $SourceFingerprint=$actualFingerprint
    $result.SourceFingerprint=$SourceFingerprint
    $verificationSourceReady=$true
    $result.SourceCommit=(& git --no-optional-locks rev-parse --verify HEAD)
    if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve the native build source commit.' }
    $result.EngineVersion=Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
    $inspectionTool=Join-Path $PSScriptRoot 'inspect-native-evidence.js'
    $env:UBA_ROOT = Join-Path $projectRoot 'Saved/ReleaseAudit/UBA'
    $env:uebp_LogFolder = Join-Path $output 'UAT'
    $env:uebp_EngineSavedFolder = Join-Path $output 'CommandletLogs'
    New-Item -ItemType Directory -Path $env:UBA_ROOT,$env:uebp_LogFolder,$env:uebp_EngineSavedFolder -Force | Out-Null
    $buildConfigTemplate=Join-Path $projectRoot 'Config/UnrealBuildTool/BuildConfiguration.xml'
    $buildConfigDirectory=Join-Path $projectRoot 'Saved/UnrealBuildTool'
    New-Item -ItemType Directory -Path $buildConfigDirectory -Force | Out-Null
    $buildConfigPath=Join-Path $buildConfigDirectory 'BuildConfiguration.xml'
    if (Test-Path -LiteralPath $buildConfigPath) {
        Copy-Item -LiteralPath $buildConfigPath -Destination (Join-Path $output ('BuildConfiguration-before-'+$Mode+'.xml')) -Force
    }
    Copy-Item -LiteralPath $buildConfigTemplate -Destination $buildConfigPath -Force
    Copy-Item -LiteralPath $buildConfigTemplate -Destination (Join-Path $output ('BuildConfiguration-'+$Mode+'.xml')) -Force
    $project = Join-Path $projectRoot 'SugarLustUE.uproject'
    if ($Mode -in 'EditorBuild','ShippingBuild') {
        $buildLabel = if ($Mode -eq 'EditorBuild') { 'editor' } else { 'shipping' }
        $target = if ($Mode -eq 'EditorBuild') { 'SugarLustUEEditor' } else { 'SugarLustUE' }
        $configuration = if ($Mode -eq 'EditorBuild') { 'Development' } else { 'Shipping' }
        $log = Join-Path $output ($buildLabel+'-build.log')
        $console = Join-Path $output ($buildLabel+'-build-console.log')
        Write-Output "Building $target Win64 $configuration; log: $log"
        & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') $target Win64 $configuration "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild -NoSNDBS -NoArtifactReads -NoArtifactWrites "-UBARootDir=$env:UBA_ROOT" "-Log=$log" *> $console
        $result.ExitCode = $LASTEXITCODE
        $result.Evidence = @($log,$console)
        if ((Test-Path -LiteralPath $log) -and (Get-Item -LiteralPath $log).LastWriteTimeUtc -ge $started) {
            $assessmentText=& $nodeExecutable $inspectionTool --mode $Mode --root $projectRoot --engine $EngineRoot --log $log --exit-code $result.ExitCode
            $assessment=$assessmentText | ConvertFrom-Json
            $assessmentFile=Join-Path $output ($Mode+'-assessment.json')
            $assessmentText | Set-Content -LiteralPath $assessmentFile -Encoding utf8
            $result.Evidence+=@($assessmentFile,(Join-Path $output ('BuildConfiguration-'+$Mode+'.xml')))
            $result.Status=$assessment.status
            $result.Summary=('{0} Win64 {1}: {2}' -f $target,$configuration,$assessment.summary)
            $result.Toolchain=$assessment.toolchain
            $result.Warnings=@($assessment.warnings)
            $exitCode=if($assessment.status -eq 'passed'){0}elseif($assessment.status -eq 'failed'){1}else{2}
        } else { $result.Summary='Unreal build did not produce fresh successful build evidence; inspect the console log.' }
    }
    elseif ($Mode -eq 'EditorTests') {
        $log = Join-Path $output 'editor-automation.log'
        $report = Join-Path $output 'Automation'
        $editorArguments = @('"'+$project+'"','-unattended','-nullrhi','-nosound','-nosplash','-nop4','-stdout','-FullStdOutLogOutput',('"-AbsLog='+$log+'"'),'"-ExecCmds=Automation RunTests SugarLust"','"-TestExit=Automation Test Queue Empty"',('"-ReportExportPath='+$report+'"'))
        $stdout=Join-Path $output 'editor-test-stdout.log'
        $stderr=Join-Path $output 'editor-test-stderr.log'
        $result.ExpectedTests=@((& $nodeExecutable -e "process.stdout.write(JSON.stringify(require('./Tools/Release/inspect-native-evidence').expectedTests(process.cwd())))") | ConvertFrom-Json)
        if ($LASTEXITCODE -ne 0 -or $result.ExpectedTests.Count -eq 0) { throw 'Could not discover the required project automation tests.' }
        Write-Output ("Running {0} project automation tests; report: {1}" -f $result.ExpectedTests.Count,$report)
        $process = Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -WorkingDirectory $projectRoot -ArgumentList $editorArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        while (-not $process.WaitForExit(1000)) {
            if (([DateTime]::UtcNow-$started).TotalSeconds -gt $AutomationTimeoutSeconds) { $process.Kill(); $process.WaitForExit(); throw 'Editor automation timed out; no successful test run is assumed.' }
        }
        $process.WaitForExit()
        $result.ExitCode=$process.ExitCode
        $index = Join-Path $report 'index.json'
        $assessmentText=& $nodeExecutable $inspectionTool --mode EditorTests --root $projectRoot --report $index --started $started.ToString('o') --exit-code $result.ExitCode
        $assessment=$assessmentText | ConvertFrom-Json
        $assessmentFile=Join-Path $output 'EditorTests-assessment.json'
        $assessmentText | Set-Content -LiteralPath $assessmentFile -Encoding utf8
        $result.Evidence=@($log,$index,$stdout,$stderr,$assessmentFile)
        $result.Status=$assessment.status
        $result.Summary=$assessment.summary
        $exitCode=if($assessment.status -eq 'passed'){0}elseif($assessment.status -eq 'failed'){1}else{2}
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
    if ($verificationSourceReady) {
        $finishedFingerprint=& $nodeExecutable -e "process.stdout.write(require('./Tools/Release/release-policy').fingerprint(process.cwd()))"
        if ($LASTEXITCODE -ne 0 -or $finishedFingerprint -ne $SourceFingerprint) {
            $result.Status='blocked';$result.Summary='Source changed during native verification; rerun against one stable source snapshot.';$exitCode=2
        }
    }
    $result.Evidence=@($result.Evidence | Where-Object { (Test-Path -LiteralPath $_ -PathType Leaf) -and (Get-Item -LiteralPath $_).Length -gt 0 })
    $result.CheckedAt=[DateTime]::UtcNow.ToString('o')
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output ($Mode + '-evidence.json')) -Encoding utf8
    $env:UBA_ROOT=$oldUba; $env:uebp_LogFolder=$oldUatLogs; $env:uebp_EngineSavedFolder=$oldEngineSaved
    Pop-Location
}
$result | ConvertTo-Json -Depth 6
exit $exitCode
