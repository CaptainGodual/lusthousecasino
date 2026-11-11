param([string]$Atlas, [string]$ProjectRoot = (Resolve-Path "$PSScriptRoot/../..").Path)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $Atlas))
try {
    if ($source.Width -ne 1536 -or $source.Height -ne 1024) { throw 'Expected the supplied 1536x1024 atlas; update rectangles for another resolution.' }
    $out = Join-Path $ProjectRoot 'Content/Art/LustVend'
    $web = Join-Path $ProjectRoot 'Backend/LustVend/web/assets'
    New-Item -ItemType Directory -Force $out,$web | Out-Null
    $rects = @()
    # The illustrated cabinets are not a regular sprite grid. Keep one complete
    # cabinet locked in place and replace only the interior of its chute.
    $rects += @{name='cabinet_base'; x=8; y=0; w=215; h=360}
    $bounds = @(8,148,288,427,565,698,840,986,1124,1258,1395,1536)
    for ($i=0; $i -lt 11; $i++) {
        $rects += @{name="product_$i"; x=$bounds[$i]; y=419; w=$bounds[$i+1]-$bounds[$i]; h=178}
    }
    for ($i=0; $i -lt 10; $i++) {
        $rects += @{name="chute_$i"; x=[int]($i*153.6); y=749; w=153; h=84}
    }
    foreach ($r in $rects) {
        $crop = $source.Clone([System.Drawing.Rectangle]::new($r.x,$r.y,$r.w,$r.h), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try { $crop.Save((Join-Path $out ($r.name+'.png')), [System.Drawing.Imaging.ImageFormat]::Png) } finally { $crop.Dispose() }
        Copy-Item -LiteralPath (Join-Path $out ($r.name+'.png')) -Destination $web
    }
    $base = $source.Clone([System.Drawing.Rectangle]::new(8,0,215,360), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        # Interior-only rectangles preserve the base cabinet's neon frame.
        $chutes = @(
            @{x=838; y=277; w=98; h=47},
            @{x=1028; y=277; w=98; h=47},
            @{x=1223; y=277; w=98; h=47}
        )
        for ($i=0; $i -lt 8; $i++) {
            $frame = $base.Clone([System.Drawing.Rectangle]::new(0,0,215,360), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            try {
                if ($i -ge 4 -and $i -le 6) {
                    $r = $chutes[$i-4]
                    $graphics = [System.Drawing.Graphics]::FromImage($frame)
                    try {
                        $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                        $graphics.DrawImage($source, [System.Drawing.Rectangle]::new(30,277,98,47), $r.x,$r.y,$r.w,$r.h,[System.Drawing.GraphicsUnit]::Pixel)
                    } finally { $graphics.Dispose() }
                }
                $path = Join-Path $out ("machine_$i.png")
                $frame.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
                Copy-Item -LiteralPath $path -Destination $web
            } finally { $frame.Dispose() }
        }
    } finally { $base.Dispose() }
    $rects | ConvertTo-Json | Set-Content (Join-Path $out 'atlas_rects.json')
    $sourcePath = Join-Path $out 'source_atlas.png'
    if ((Resolve-Path -LiteralPath $Atlas).Path -ne $sourcePath) { Copy-Item -LiteralPath $Atlas -Destination $sourcePath }
} finally { $source.Dispose() }
