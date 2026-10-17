[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Zig,
    [Parameter(Mandatory)][string]$Sdl,
    [Parameter(Mandatory)][string]$Mixer,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$compiler = (Resolve-Path -LiteralPath $Zig).Path
$sdlRoot = (Resolve-Path -LiteralPath $Sdl).Path
$mixerRoot = (Resolve-Path -LiteralPath $Mixer).Path
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$sources = Get-ChildItem -LiteralPath "$repo/SpaceCadetPinball" -Filter '*.cpp' |
    Where-Object Name -ne 'SpaceCadetPinball.cpp' | Sort-Object Name | ForEach-Object FullName
$exe = Join-Path $output 'high_score_integration_test.exe'
# Assertions stay enabled. The test links the actual game code, without opening a window.
& $compiler c++ -std=c++11 -O2 -DUNICODE -D_UNICODE `
    -I "$sdlRoot/include" -I "$mixerRoot/include" `
    "$PSScriptRoot/high_score_integration_test.cpp" @sources `
    "$sdlRoot/lib/x64/SDL2.lib" "$mixerRoot/lib/x64/SDL2_mixer.lib" -lwinmm -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Integration test compilation failed' }
Copy-Item -LiteralPath "$sdlRoot/lib/x64/SDL2.dll" -Destination $output -Force
Copy-Item -LiteralPath "$mixerRoot/lib/x64/SDL2_mixer.dll" -Destination $output -Force
# A unique file never touches the player's real preferences or a previous test.
$ini = Join-Path $output ("high-scores-test-" + [guid]::NewGuid() + '.ini')
@'
[Pinball][Settings]
0.Name=Legacy player
0.Score=3000000
1.Name=
1.Score=-999
2.Name=
2.Score=-999
3.Name=
3.Score=-999
4.Name=
4.Score=-999
Verification=2997286
'@ | Set-Content -LiteralPath $ini -Encoding utf8
foreach ($stage in @('seed', 'clear-windows', 'verify')) {
    & $exe $ini $stage
    if ($LASTEXITCODE -ne 0) { throw "Integration stage failed: $stage" }
}
Write-Output "Verified preferences retained at $ini"
