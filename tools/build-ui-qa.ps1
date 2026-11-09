param(
    [string]$Compiler,
    [string]$ScratchRoot = 'G:\middle_thing\godhand_windows',
    [Parameter(Mandatory=$true)][string]$Iso,
    [string]$OutputRgba
)
$ErrorActionPreference = 'Stop'
$godhandProject = Split-Path -Parent $PSScriptRoot
if (-not $Compiler) { $Compiler = Join-Path $godhandProject '.tools\zig-x86_64-windows-0.17.1\zig.exe' }
$godhandBuild = Join-Path $ScratchRoot 'build\ui-renderer'
$godhandCache = Join-Path $ScratchRoot 'cache'
$godhandScratchPath = [IO.Path]::GetFullPath($ScratchRoot).TrimEnd('\')
$godhandProjectPath = [IO.Path]::GetFullPath($godhandProject).TrimEnd('\')
if ($godhandScratchPath.Equals($godhandProjectPath,[StringComparison]::OrdinalIgnoreCase) -or
    $godhandScratchPath.StartsWith($godhandProjectPath+'\',[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Scratch must stay outside the permanent project tree.'
}
if (-not (Test-Path -LiteralPath $Iso -PathType Leaf)) { throw 'Supply your existing game ISO.' }
$godhandSdkRoot = 'C:\Program Files (x86)\Windows Kits\10\Lib'
$godhandSdk = Get-ChildItem -LiteralPath $godhandSdkRoot -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um\x64\d3d11.lib') } |
    Select-Object -First 1
if (-not $godhandSdk) { throw 'This optional GPU check requires Windows SDK x64 import libraries.' }
$godhandSdkLib = Join-Path $godhandSdk.FullName 'um\x64'
$godhandSources = @('src/core/font_draw.c','src/core/font_state.c','src/assets/font_texture.c',
    'src/assets/core_bundle.c','src/platform/disc.c')
$godhandInputs = $godhandSources + @('src/core/font_draw.h','src/core/font_state.h',
    'src/assets/font_texture.h','src/assets/core_bundle.h','src/platform/disc.h',
    'src/platform/ui_renderer.cpp','src/platform/ui_renderer.h',
    'tests/platform/test_ui_renderer.cpp','tools/build-ui-qa.ps1')
$godhandHashes = [ordered]@{}
foreach ($godhandName in $godhandInputs) {
    $godhandHashes[$godhandName] = (Get-FileHash -LiteralPath (Join-Path $godhandProject $godhandName) -Algorithm SHA256).Hash.ToLowerInvariant()
}
New-Item -ItemType Directory -Force -Path $godhandBuild,(Join-Path $godhandCache 'tmp'),(Join-Path $godhandProject 'analysis') | Out-Null
$godhandBefore = @{}
foreach ($godhandKey in @('TEMP','TMP','ZIG_GLOBAL_CACHE_DIR','ZIG_LOCAL_CACHE_DIR')) {
    $godhandBefore[$godhandKey] = [Environment]::GetEnvironmentVariable($godhandKey,'Process')
}
try {
    $env:TEMP = Join-Path $godhandCache 'tmp'; $env:TMP = $env:TEMP
    $env:ZIG_GLOBAL_CACHE_DIR = Join-Path $godhandCache 'zig-global'
    $env:ZIG_LOCAL_CACHE_DIR = Join-Path $godhandCache 'zig-local'
    $godhandObjects = @()
    foreach ($godhandSource in $godhandSources) {
        $godhandObject = Join-Path $godhandBuild (([IO.Path]::GetFileNameWithoutExtension($godhandSource))+'.o')
        & $Compiler cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -D_CRT_SECURE_NO_WARNINGS -c (Join-Path $godhandProject $godhandSource) -o $godhandObject
        if ($LASTEXITCODE -ne 0) { throw "UI dependency compilation failed: $godhandSource" }
        $godhandObjects += $godhandObject
    }
    $godhandExe = Join-Path $godhandBuild 'test_ui_renderer.exe'
    & $Compiler c++ -target x86_64-windows-gnu -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -D_CRT_SECURE_NO_WARNINGS @godhandObjects (Join-Path $godhandProject 'src/platform/ui_renderer.cpp') (Join-Path $godhandProject 'tests/platform/test_ui_renderer.cpp') "-L$godhandSdkLib" -ld3d11 -ldxgi -ld3dcompiler -o $godhandExe
    if ($LASTEXITCODE -ne 0) { throw 'UI renderer compilation failed.' }
    $godhandArgs = @($Iso)
    if ($OutputRgba) { $godhandArgs += $OutputRgba }
    $godhandOutput = & $godhandExe @godhandArgs
    if ($LASTEXITCODE -ne 0) { throw 'UI GPU verification failed or selected hardware is unavailable.' }
    foreach ($godhandName in $godhandInputs) {
        if ((Get-FileHash -LiteralPath (Join-Path $godhandProject $godhandName) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $godhandHashes[$godhandName]) {
            throw "Source changed while checking UI renderer: $godhandName"
        }
    }
    $godhandReport = [ordered]@{
        status='passed'; scope='Restricted original-font quad/GS fragment GPU QA; not full game renderer or gameplay'
        source_sha256=$godhandHashes; compiler=$Compiler; compiler_version=(& $Compiler version)
        executable=$godhandExe; executable_sha256=(Get-FileHash -LiteralPath $godhandExe -Algorithm SHA256).Hash.ToLowerInvariant()
        output=$godhandOutput; gpu_requirement='NVIDIA hardware adapter, no software fallback'
        compared_pixels=12288; actual_game_scene_observed=$false
    }
    if ($OutputRgba) {
        $godhandReport.raw_output=$OutputRgba
        $godhandReport.raw_sha256=(Get-FileHash -LiteralPath $OutputRgba -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    $godhandReport | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $godhandProject 'analysis/ui-renderer-verification.json') -Encoding utf8
    $godhandOutput
}
finally {
    foreach ($godhandKey in $godhandBefore.Keys) { [Environment]::SetEnvironmentVariable($godhandKey,$godhandBefore[$godhandKey],'Process') }
}
