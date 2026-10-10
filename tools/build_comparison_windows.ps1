# Build the comparison executable using locally installed, pinned dependencies.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Zig,
    [Parameter(Mandatory)][string]$Sdl,
    [Parameter(Mandatory)][string]$Mixer,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$CadenceAudit
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$compiler = (Resolve-Path -LiteralPath $Zig).Path
$sdlRoot = (Resolve-Path -LiteralPath $Sdl).Path
$mixerRoot = (Resolve-Path -LiteralPath $Mixer).Path
foreach ($file in @("$sdlRoot/include/SDL.h", "$sdlRoot/lib/x64/SDL2.lib",
        "$sdlRoot/lib/x64/SDL2.dll", "$mixerRoot/include/SDL_mixer.h",
        "$mixerRoot/lib/x64/SDL2_mixer.lib", "$mixerRoot/lib/x64/SDL2_mixer.dll")) {
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing dependency: $file" }
}
$version = & $compiler version
if ($LASTEXITCODE -ne 0 -or $version -ne '0.13.0') { throw 'This procedure requires Zig 0.13.0.' }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$exe = Join-Path $output 'SpaceCadetPinball.exe'
# This lightweight audit build omits the Windows icon/version resource (.rc).
# Gameplay sources are the same translation units used by CMake.
$sources = Get-ChildItem -LiteralPath "$repo/SpaceCadetPinball" -Filter '*.cpp' |
    Sort-Object Name | ForEach-Object FullName
$auditFlags = @()
if ($CadenceAudit) { $auditFlags += '-DPINBALL_CADENCE_AUDIT' }
& $compiler c++ -std=c++11 -O2 -DNDEBUG -DUNICODE -D_UNICODE @auditFlags `
    -I "$sdlRoot/include" -I "$mixerRoot/include" @sources `
    "$sdlRoot/lib/x64/SDL2.lib" "$mixerRoot/lib/x64/SDL2_mixer.lib" `
    -lwinmm -o $exe
if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $LASTEXITCODE" }
Copy-Item -LiteralPath "$sdlRoot/lib/x64/SDL2.dll" -Destination $output -Force
Copy-Item -LiteralPath "$mixerRoot/lib/x64/SDL2_mixer.dll" -Destination $output -Force
Get-FileHash -LiteralPath $exe -Algorithm SHA256
