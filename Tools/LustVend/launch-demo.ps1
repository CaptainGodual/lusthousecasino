$ErrorActionPreference = 'Stop'
$project = (Resolve-Path "$PSScriptRoot/../..").Path
$address = 'http://127.0.0.1:8787'
function Test-DemoReady {
    try {
        $state = Invoke-RestMethod -Uri "$address/api/state" -Headers @{Authorization='Bearer local-demo'} -TimeoutSec 2
        return $state.player -eq 'demo-player'
    } catch { return $false }
}
if (-not (Test-DemoReady)) {
    Start-Process -FilePath python -ArgumentList @('-u','Backend/LustVend/server.py','--demo') -WorkingDirectory $project -WindowStyle Hidden
    for ($attempt=0; $attempt -lt 20; $attempt++) {
        if (Test-DemoReady) { break }
        Start-Sleep -Milliseconds 250
    }
    if (-not (Test-DemoReady)) { throw 'The demo could not start on port 8787. Check whether another application is using that port.' }
}
Start-Process $address
