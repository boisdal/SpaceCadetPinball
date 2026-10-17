[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Zig,
    [Parameter(Mandatory)][string]$CMake,
    [Parameter(Mandatory)][string]$Ninja,
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$compiler = (Resolve-Path -LiteralPath $Zig).Path.Replace('\', '/')
$cmakeExe = (Resolve-Path -LiteralPath $CMake).Path
$ninjaExe = (Resolve-Path -LiteralPath $Ninja).Path.Replace('\', '/')
if ((& $compiler version) -ne '0.13.0') { throw 'Use Zig 0.13.0 for this release procedure.' }
if ((& $cmakeExe --version)[0] -notmatch '3\.31\.6$') { throw 'Use CMake 3.31.6 for this release procedure.' }
if ((& $ninjaExe --version) -notmatch '^1\.11\.1(\.git\.kitware\.jobserver-1)?$') { throw 'Use Ninja 1.11.1 for this release procedure.' }
New-Item -ItemType Directory -Force -Path $BuildDirectory, $OutputDirectory | Out-Null
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path.Replace('\', '/')
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path.Replace('\', '/')
if ($build -eq $output -or $build.StartsWith($output.TrimEnd('/') + '/', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The release directory must not contain the build directory.'
}
# Never package a folder with existing files: the release allowlist is one EXE.
if (Get-ChildItem -LiteralPath $output -Force) { throw 'OutputDirectory must be empty.' }
foreach ($language in @('cc', 'cxx')) {
    $subcommand = if ($language -eq 'cc') { 'cc' } else { 'c++' }
    # CMake's Clang probe passes an unsupported verbose linker flag to Zig 0.13.
    @('@echo off', 'set "args=%*"', 'set "args=%args:-Wl,-v=%"',
      ('"' + $compiler + '" ' + $subcommand + ' -target x86_64-windows-gnu %args%')) |
        Set-Content -LiteralPath "$build/zig-$language.cmd" -Encoding ascii
}
@('@echo off', ('"' + $compiler + '" ar %*')) |
    Set-Content -LiteralPath "$build/zig-ar.cmd" -Encoding ascii
@"
set(CMAKE_C_COMPILER "$build/zig-cc.cmd")
set(CMAKE_CXX_COMPILER "$build/zig-cxx.cmd")
set(CMAKE_RC_COMPILER "$compiler")
set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> rc /fo <OBJECT> <INCLUDES> <SOURCE>")
set(CMAKE_AR "$build/zig-ar.cmd")
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "")
set(CMAKE_CXX_ARCHIVE_FINISH "")
"@ | Set-Content -LiteralPath "$build/zig.cmake" -Encoding ascii
& $cmakeExe -S $repo -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninjaExe" `
    "-DCMAKE_TOOLCHAIN_FILE=$build/zig.cmake" -DPINBALL_WINDOWS_STANDALONE=ON `
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON `
    -DCMAKE_CXX_FLAGS= -DCMAKE_C_FLAGS=
if ($LASTEXITCODE) { throw 'CMake configure failed.' }
& $cmakeExe --build $build --config Release --parallel $Jobs
if ($LASTEXITCODE) { throw 'Compilation failed.' }
Copy-Item -LiteralPath "$build/bin/SpaceCadetPinball.exe" -Destination $output
Get-FileHash -LiteralPath "$output/SpaceCadetPinball.exe" -Algorithm SHA256
