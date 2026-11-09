$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$godhandProjectRoot = Split-Path -Parent $PSScriptRoot
$godhandScratchRoot = 'G:\middle_thing\godhand_windows'
$godhandToolRoot = Join-Path $godhandProjectRoot '.tools'
$godhandArchive = Join-Path $godhandScratchRoot 'cache\zig-x86_64-windows-0.17.1.zip'
$godhandReleaseUrl = 'https://ziglang.org/download/index.json'
$godhandDownloadUrl = 'https://ziglang.org/download/0.17.1/zig-x86_64-windows-0.17.1.zip'
$godhandExpectedHash = 'b03f477a1943673e02aca0f570e8030dffd536ce1f45cbc4d3326ec31159562e'
$godhandInstallRoot = Join-Path $godhandToolRoot 'zig-x86_64-windows-0.17.1'
New-Item -ItemType Directory -Force -Path $godhandToolRoot,(Split-Path -Parent $godhandArchive) | Out-Null
if (-not (Test-Path -LiteralPath $godhandArchive -PathType Leaf)) {
    Invoke-WebRequest -Uri $godhandDownloadUrl -OutFile ($godhandArchive + '.partial')
    Move-Item -LiteralPath ($godhandArchive + '.partial') -Destination $godhandArchive
}
$godhandArchiveHash = (Get-FileHash -LiteralPath $godhandArchive -Algorithm SHA256).Hash.ToLowerInvariant()
if ($godhandArchiveHash -ne $godhandExpectedHash) { throw 'Official Zig archive SHA-256 mismatch.' }
if (-not (Test-Path -LiteralPath (Join-Path $godhandInstallRoot 'zig.exe') -PathType Leaf)) {
    Expand-Archive -LiteralPath $godhandArchive -DestinationPath $godhandToolRoot
}
$godhandCompiler = Join-Path $godhandInstallRoot 'zig.exe'
if (-not (Test-Path -LiteralPath $godhandCompiler -PathType Leaf)) { throw 'Expected compiler missing after extraction.' }
$godhandCompilerVersion = (& $godhandCompiler version | Out-String).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Compiler version check failed.' }
$godhandManifest = [ordered]@{
    schema_version = 1
    installed_utc = [DateTime]::UtcNow.ToString('o')
    release = $godhandReleaseUrl
    download = $godhandDownloadUrl
    archive = $godhandArchive
    archive_sha256 = $godhandArchiveHash
    expected_archive_sha256 = $godhandExpectedHash
    checksum_provenance = 'Verified against the SHA-256 published at https://ziglang.org/download/index.json.'
    install = $godhandInstallRoot
    compiler = $godhandCompiler
    compiler_version = $godhandCompilerVersion
    changes_global_environment = $false
    purpose = 'Portable Zig cc (Clang) compiles recovered C into native Windows x86-64 tools and game modules.'
}
$godhandManifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $godhandProjectRoot 'analysis\toolchain-manifest.json') -Encoding utf8
$godhandManifest | ConvertTo-Json -Depth 5
