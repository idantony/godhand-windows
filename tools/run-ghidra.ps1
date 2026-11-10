param(
    [string]$InputElf = 'E:\godhand_windows\reference\SLUS_215.03',
    [string]$ProjectName = 'godhand_probe',
    [object[]]$Addresses = @('0031cf30','0031cf78','00100008','00320260','0031cfe0','0031d020'),
    [string]$OutputJson = 'E:\godhand_windows\analysis\ghidra-startup.json',
    [ValidateRange(1,65536)][int]$MaxInstructions = 2048
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$scratch = 'G:\middle_thing\godhand_windows\ghidra'
$ghidra = Join-Path $projectRoot '.tools\ghidra_12.1.4_PUBLIC'
$python = Join-Path $projectRoot '.tools\python\Scripts\python.exe'
$validator = Join-Path $PSScriptRoot 'validate-ghidra-report.py'
if (!(Test-Path -LiteralPath $InputElf -PathType Leaf)) { throw "ELF missing: $InputElf" }
if ($ProjectName -notmatch '^[A-Za-z0-9_-][A-Za-z0-9_.-]*$') { throw 'ProjectName must be a single safe scratch project name' }
$outputPath = [IO.Path]::GetFullPath($OutputJson)
if (Test-Path -LiteralPath $outputPath) { throw "Output already exists; choose a fresh report path: $outputPath" }
$run = Join-Path "$scratch\runs" ([Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $run,"$scratch\projects","$scratch\tmp","$scratch\settings","$scratch\cache" -Force | Out-Null
$requested = Join-Path $run 'requested.json'
$manifest = Join-Path $run 'manifest.json'
$candidate = Join-Path $run 'report.json'
$combinedLog = Join-Path $run 'combined.log'
$headlessLog = Join-Path $run 'headless.log'
$scriptLog = Join-Path $run 'script.log'
# Preserve argument types: an unquoted PowerShell scientific/decimal number
# must not be silently reinterpreted as an instruction address.
ConvertTo-Json -InputObject @($Addresses) | Set-Content -LiteralPath $requested -Encoding utf8
& $python $validator prepare --elf $InputElf --addresses $requested --report $candidate --manifest $manifest --log $combinedLog --log $headlessLog --log $scriptLog --limit $MaxInstructions
if ($LASTEXITCODE -ne 0) { throw "Ghidra request validation failed; evidence: $run" }
$prepared = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
[string[]]$canonicalAddresses = @($prepared.addresses)
$env:JAVA_HOME = Join-Path $projectRoot '.tools\jdk-21.0.13+9'
$env:GHIDRA_JAVA_OPTIONS = "-Dapplication.settingsdir=$scratch/settings -Dapplication.cachedir=$scratch/cache -Dapplication.tempdir=$scratch/tmp -Djava.io.tmpdir=$scratch/tmp"
$env:TEMP = "$scratch\tmp"
$env:TMP = "$scratch\tmp"
# Only scratch project data may be overwritten. The requested evidence path
# stays absent until the current report, log, hash and raw bytes all validate.
& "$ghidra\support\analyzeHeadless.bat" "$scratch\projects" $ProjectName -import $InputElf -overwrite -processor 'r5900:LE:32:default' -cspec default -noanalysis -scriptPath "$PSScriptRoot\ghidra_scripts" -postScript GodHandProbe.java $candidate max-insns $MaxInstructions @canonicalAddresses -log $headlessLog -scriptlog $scriptLog -max-cpu 4 2>&1 | Tee-Object -FilePath $combinedLog | Out-Null
$headlessExit = $LASTEXITCODE
& $python $validator validate --manifest $manifest --exit-code $headlessExit --publish $outputPath
if ($LASTEXITCODE -ne 0) { throw "Ghidra report rejected; original reports preserved; evidence: $run" }
Write-Output "GODHAND_VERIFIED_RUN=$run"
