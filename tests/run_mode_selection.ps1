[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Zig,
    [Parameter(Mandatory)][string]$Sdl,
    [Parameter(Mandatory)][string]$Mixer,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$WindowsResources,
    [Parameter(Mandatory)][string]$FullTiltResources
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$compiler = (Resolve-Path -LiteralPath $Zig).Path
$sdlRoot = (Resolve-Path -LiteralPath $Sdl).Path
$mixerRoot = (Resolve-Path -LiteralPath $Mixer).Path
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$sources = Get-ChildItem -LiteralPath "$repo/SpaceCadetPinball" -Filter '*.cpp' |
    Where-Object { $_.Name -notin @('SpaceCadetPinball.cpp', 'ModeSelection.cpp') } | Sort-Object Name | ForEach-Object FullName
$exe = Join-Path $output 'mode_selection_test.exe'
# Assertions stay enabled. The test links the actual game code, without opening a window.
& $compiler c++ -std=c++11 -O2 -DUNICODE -D_UNICODE `
    -I "$sdlRoot/include" -I "$mixerRoot/include" `
    "$PSScriptRoot/mode_selection_test.cpp" @sources `
    "$sdlRoot/lib/x64/SDL2.lib" "$mixerRoot/lib/x64/SDL2_mixer.lib" -lwinmm -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Integration test compilation failed' }
Copy-Item -LiteralPath "$sdlRoot/lib/x64/SDL2.dll" -Destination $output -Force
Copy-Item -LiteralPath "$mixerRoot/lib/x64/SDL2_mixer.dll" -Destination $output -Force
# Isolate both preference files and source copies; originals are read-only.
$case = Join-Path $output ("mode-test-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $case | Out-Null
foreach ($mode in @('windows', 'full-tilt')) { New-Item -ItemType Directory -Path (Join-Path $case $mode) | Out-Null }
Copy-Item -LiteralPath (Join-Path $WindowsResources 'PINBALL.DAT') -Destination (Join-Path $case 'windows/PINBALL.DAT')
Copy-Item -LiteralPath (Join-Path $FullTiltResources 'CADET.DAT') -Destination (Join-Path $case 'full-tilt/CADET.DAT')
foreach ($stage in @('windows-only', 'two-sources', 'restart', 'missing')) {
    & $exe $case $stage
    if ($LASTEXITCODE -ne 0) { throw "Integration stage failed: $stage" }
}
$ftCase = Join-Path $output ("mode-test-ft-only-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path (Join-Path $ftCase 'full-tilt') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $FullTiltResources 'CADET.DAT') -Destination (Join-Path $ftCase 'full-tilt/CADET.DAT')
& $exe $ftCase 'full-tilt-only'
if ($LASTEXITCODE -ne 0) { throw 'Full Tilt only test failed' }
Write-Output "Mode selection evidence retained at $case and $ftCase"
