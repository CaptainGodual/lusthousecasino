$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$base = [System.Drawing.Bitmap]::FromFile((Join-Path $root 'Content/Art/LustVend/machine_0.png'))
try {
    for ($i=1; $i -lt 8; $i++) {
        $frame = [System.Drawing.Bitmap]::FromFile((Join-Path $root "Content/Art/LustVend/machine_$i.png"))
        try {
            if ($frame.Width -ne 215 -or $frame.Height -ne 360) { throw "Frame $i dimensions changed" }
            for ($y=0; $y -lt 360; $y++) {
                for ($x=0; $x -lt 215; $x++) {
                    if ($x -ge 30 -and $x -lt 128 -and $y -ge 277 -and $y -lt 324) { continue }
                    if ($base.GetPixel($x,$y).ToArgb() -ne $frame.GetPixel($x,$y).ToArgb()) { throw "Cabinet moved in frame $i at $x,$y" }
                }
            }
        } finally { $frame.Dispose() }
    }
    Write-Output 'All 8 frames share identical cabinet pixels outside the chute.'
} finally { $base.Dispose() }
