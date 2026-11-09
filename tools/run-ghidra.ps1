param(
    [string]$InputElf = 'E:\godhand_windows\reference\SLUS_215.03',
    [string]$ProjectName = 'godhand_probe',
    [string[]]$Addresses = @('0031cf30','0031cf78','00100008','00320260','0031cfe0','0031d020'),
    [string]$OutputJson = 'E:\godhand_windows\analysis\ghidra-startup.json'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$scratch = 'G:\middle_thing\godhand_windows\ghidra'
$ghidra = Join-Path $projectRoot '.tools\ghidra_12.1.4_PUBLIC'
$env:JAVA_HOME = Join-Path $projectRoot '.tools\jdk-21.0.13+9'
$env:GHIDRA_JAVA_OPTIONS = "-Dapplication.settingsdir=$scratch/settings -Dapplication.cachedir=$scratch/cache -Dapplication.tempdir=$scratch/tmp -Djava.io.tmpdir=$scratch/tmp"
$env:TEMP = "$scratch\tmp"
$env:TMP = "$scratch\tmp"
New-Item -ItemType Directory -Path "$scratch\projects","$scratch\tmp","$scratch\settings","$scratch\cache" -Force | Out-Null
if (!(Test-Path -LiteralPath $InputElf -PathType Leaf)) { throw "ELF missing: $InputElf" }
# Re-import only the named binary in this dedicated scratch project on repeat runs.
& "$ghidra\support\analyzeHeadless.bat" "$scratch\projects" $ProjectName -import $InputElf -overwrite -processor 'r5900:LE:32:default' -cspec default -noanalysis -scriptPath "$PSScriptRoot\ghidra_scripts" -postScript GodHandProbe.java $OutputJson @Addresses -log "$scratch\ghidra-headless.log" -scriptlog "$scratch\ghidra-script.log" -max-cpu 4
exit $LASTEXITCODE
