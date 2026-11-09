param(
    [string]$Compiler,
    [string]$ScratchRoot = 'G:\middle_thing\godhand_windows',
    [string]$InputRgba,
    [ValidateRange(1,4096)][int]$Width = 64,
    [ValidateRange(1,4096)][int]$Height = 64,
    [string]$OutputPng
)
$ErrorActionPreference = 'Stop'
$godhandProject = Split-Path -Parent $PSScriptRoot
if (-not $Compiler) { $Compiler = Join-Path $godhandProject '.tools\zig-x86_64-windows-0.17.1\zig.exe' }
$godhandBuild = Join-Path $ScratchRoot 'build\asset-qa'
$godhandCache = Join-Path $ScratchRoot 'cache'
$godhandSource = Join-Path $PSScriptRoot 'render-texture.cpp'
$godhandExe = Join-Path $godhandBuild 'render-texture.exe'
$godhandSdkRoot = 'C:\Program Files (x86)\Windows Kits\10\Lib'
$godhandSdk = Get-ChildItem -LiteralPath $godhandSdkRoot -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um\x64\d3d11.lib') } |
    Select-Object -First 1
if (-not $godhandSdk) { throw 'Optional GPU QA build needs the Windows SDK x64 import libraries.' }
$godhandSdkLib = Join-Path $godhandSdk.FullName 'um\x64'
New-Item -ItemType Directory -Force -Path $godhandBuild,$godhandCache,(Join-Path $godhandCache 'tmp') | Out-Null
$godhandBefore = @{}
foreach ($godhandKey in @('TEMP','TMP','ZIG_GLOBAL_CACHE_DIR','ZIG_LOCAL_CACHE_DIR')) {
    $godhandBefore[$godhandKey] = [Environment]::GetEnvironmentVariable($godhandKey,'Process')
}
try {
    $env:TEMP = Join-Path $godhandCache 'tmp'
    $env:TMP = $env:TEMP
    $env:ZIG_GLOBAL_CACHE_DIR = Join-Path $godhandCache 'zig-global'
    $env:ZIG_LOCAL_CACHE_DIR = Join-Path $godhandCache 'zig-local'
    & $Compiler cc -x c++ -std=c++17 -fno-exceptions -fno-rtti -O2 -Wall -Wextra -Werror -municode $godhandSource "-L$godhandSdkLib" -ld3d11 -ldxgi -ld3dcompiler -lwindowscodecs -lole32 -luuid -o $godhandExe
    if ($LASTEXITCODE -ne 0) { throw 'GPU QA tool compilation failed.' }
    $godhandReport = [ordered]@{
        status = 'built'; scope = 'Optional offscreen asset QA; not game renderer or gameplay'
        source_sha256 = (Get-FileHash -LiteralPath $godhandSource -Algorithm SHA256).Hash.ToLowerInvariant()
        executable = $godhandExe; sdk_import_libraries = $godhandSdkLib
    }
    if ($InputRgba) {
        if (-not $OutputPng) { $OutputPng = Join-Path $godhandBuild ('texture-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff') + '.png') }
        $godhandPixels = & $godhandExe --rgba $InputRgba --width $Width --height $Height --output $OutputPng
        if ($LASTEXITCODE -ne 0) { throw 'GPU QA failed; outputs are never overwritten.' }
        $godhandReport.status = 'passed'
        $godhandReport.pixel_check = ($godhandPixels | ConvertFrom-Json)
        $godhandReport.input_sha256 = (Get-FileHash -LiteralPath $InputRgba -Algorithm SHA256).Hash.ToLowerInvariant()
        $godhandReport.output = $OutputPng
        $godhandReport.output_sha256 = (Get-FileHash -LiteralPath $OutputPng -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    $godhandReport | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $godhandProject 'analysis\gpu-qa-last-run.json') -Encoding utf8
    $godhandReport | ConvertTo-Json -Depth 6
}
finally {
    foreach ($godhandKey in $godhandBefore.Keys) { [Environment]::SetEnvironmentVariable($godhandKey,$godhandBefore[$godhandKey],'Process') }
}
