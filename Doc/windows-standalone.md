# Windows x64 standalone release

The distributed artifact is **SpaceCadetPinball.exe only**. It contains the game,
SDL 2.30.11 and SDL_mixer 2.8.0 linked statically, plus license notices. It needs
Windows 10/11 x64 system libraries; it does not require an SDL DLL, a compiler,
a Visual C++ redistributable, a soundfont or a separately installed codec.
Historical WAV audio and native Windows MIDI remain enabled. Full Tilt MDS is
converted to MIDI by the existing game code. Nonhistorical optional formats
(FLAC, MP3, Ogg, modules, Opus, WavPack) are not part of this release configuration.

Resources are supplied locally through the import screen. The release pipeline
never reads a user's resource folders and uploads only the explicitly named EXE,
not the build tree. No DAT, WAV, MID, MDS or original game EXE is packaged.

## Build on Windows

Install Python 3.12 and the pinned build tools (only needed by the builder):

```powershell
python -m pip install ziglang==0.13.0 cmake==3.31.6 ninja==1.11.1.4
$zig = python -c "import pathlib,ziglang;print(pathlib.Path(ziglang.__file__).parent/'zig.exe')"
$cmake = python -c "import cmake;print(cmake.CMAKE_BIN_DIR+'/cmake.exe')"
./tools/build_windows_release.ps1 -Zig $zig -CMake $cmake `
  -Ninja (Get-Command ninja).Source -BuildDirectory ./build/standalone `
  -OutputDirectory ./out/windows-release
python tools/check_windows_release.py ./out/windows-release/SpaceCadetPinball.exe
```

The output directory must be empty. Use a new directory for a later artifact;
the script never cleans an arbitrary directory. CMake caches verified dependency
sources, so an incremental build can reuse the build directory. The CMake
configuration downloads SDL archives from immutable commits and checks SHA256.
The Zig target is generic x86_64 Windows, not the builder's CPU instruction set.
Reproducible here means a pinned, repeatable build procedure; bit-for-bit equality
across machines or paths is not claimed (PE timestamps/debug paths may differ).

`.github/workflows/windows-standalone.yml` runs this procedure on Windows 2022
and uploads only the EXE. The import checker rejects non-system and delayed PE
imports and requires embedded notices and the GUI subsystem. It does not prove
that every dynamic system-library loading path will succeed on another machine.

## Licenses

From a writable directory without an existing file of that name, run:

```powershell
Start-Process ./SpaceCadetPinball.exe -ArgumentList '--licenses' -Wait
```

This exports `SpaceCadetPinball-LICENSES.txt` without initializing SDL or opening
the game. Existing files are preserved. The exact UTF-8 notices are also in
`Platform/Windows/THIRD-PARTY-NOTICES.txt`: project/ImGui/imgui_sdl/stb (MIT),
SDL/mixer (zlib), HIDAPI (BSD alternative), yuv2rgb, Zig, LLVM runtimes and MinGW.
No runtime source has been modified by this packaging change.

## Validation and remaining acceptance

The local build and PE inspection are recorded in
`comparison-runs/2026-10-17-issue31-pe.json`. License extraction was compared
byte-for-byte to the source document. These checks do **not** establish the
clean-machine interactive criterion.

On a Windows 10/11 x64 environment without development tools, copy only the EXE,
then check initial import, a played game in each available mode, preference
changes, a score entry, clean exit and relaunch with those values preserved.
Keep the game resources local and out of any attached report. A restricted PATH
on a developer machine is useful additional evidence, not a clean-machine test.

The normal CMake default is unchanged: Linux/macOS use their existing system
SDL dependencies and packaging. Their compilation has not been performed in
this Windows workspace. The standalone option is rejected outside Windows x64.
