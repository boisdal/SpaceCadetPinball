# Resource folder import (#30)

The first launch without existing data opens an ImGui setup window. Paste an
absolute resource folder path, inspect it, then remember it. Existing installs
beside the executable retain their legacy search behavior. Options > Game Data >
Import resource folder opens the same form while a game is running (paused).
Adding a folder does not restart the current game; selection is covered by #28.

Import currently means registering the source location, not copying it. The form
states that the folder must remain available. The application never writes to
that folder through this code. Windows and Full Tilt locations live in separate
slots in `resources-v1.txt` in SDL's user preference folder. Registration writes a
temporary file and atomically replaces the registry only after successful write
and validation. Cancellation, validation failure and write failure preserve the
previous entries. An unreadable/malformed registry is not silently overwritten.
High-score persistence is entirely separate.

## Recognition and diagnostics

`ResourceCatalog::Inspect` compares the actual DAT SHA-256 with the Windows XP
and Full Tilt reference manifests from #20. Renaming a DAT does not change its
recognized mode. The shared Full Tilt DAT does not distinguish Windows 95 from
Windows 3.1; the interface explicitly leaves the exact edition unverified. The
original executable is not required. Both recognized tables in one folder are
ambiguous and require separate folders.

The bounded DAT reader inventories declared sounds. The placeholder `...` is
excluded. Six declared files absent on each original reference are reported as
reference observations, not newly lost files. Other missing/unreadable sounds,
missing music, and audio differing from the reference SHA-256 produce named
warnings and permit registration. No sound is fabricated or borrowed from the
other mode. Users can repair the source and inspect again.

**Remaining compatibility work:** `Doc/reference-inventory.md` proposed manual
mode confirmation for an unknown DAT. This implementation instead diagnoses and
rejects an unknown fingerprint. This is an implementation limitation, not a new
product decision: enabling unknown tables still needs adequate parser validation
and explicit mode/provenance confirmation. Do not mark that path implemented.
The current reference XP and FT folders are supported.

## Integration API

- `ResourceCatalog::Inspect(directory)` returns `ResourceInspection` with
  `valid`, `fullTilt`, `datName`, `fingerprint`, `errors`, `warnings`, `observations`.
- `Register(inspection, error)` revalidates and commits only the matching slot.
- `Get(bool fullTilt)` returns that slot's absolute source path; `Load(error)`
  restores both slots transactionally.
- `ResourceImport::Catalog()` exposes the active registry to #28.
- `ResourceImport::SelectPreferred(bool preferWindows)` selects a registered
  table and assigns `pb::BasePath`, `DatFileName`, `FullTiltMode`. It falls back to
  the other mode only if the preferred slot is empty, never if it is invalid.
- `ResourceImport::RunSetup/Open/Draw` provide setup and in-game import UI.

## Verification

On Windows, 17 October 2026:

- Standalone C++11 tests: SHA-256 empty/abc/million-a vectors; XP/FT recognition;
  registration, both slots after restart, reimport, invalid import rollback,
  failed save rollback, UTF-8 registry filename, corrupt registry preservation.
- `tools/test_resource_import.py`: unknown, absent, incomplete, repaired, modified
  audio, renamed DAT and ambiguous folders; SHA-256 of both source trees unchanged.
- Full game built with Zig 0.13, SDL 2.30.11, SDL_mixer 2.8.0, normal and audit builds.
- Interactive check on the audit build: first-launch cancellation quits without
  creating a registry; relaunch, inspect the XP folder, recognize its table and
  known absent sounds, remember the folder, and display the Windows table.
  The isolated registry retains the source after normal shutdown. Adding the
  second mode and unknown variants remain separate checks.

Compile `tests/resource_catalog_test.cpp` with C++11, then run:

```text
python tools/test_resource_import.py TEST_EXE WINDOWS_REFERENCE FULL_TILT_REFERENCE
```

For isolated UI checks, build with `-CadenceAudit` and set both
`PINBALL_AUDIT_INI` and `PINBALL_AUDIT_RESOURCE_REGISTRY` to fresh files outside the
source folder. Start with an empty working directory and executable folder with
no DAT. Cancel must quit with neither a registry nor any source changes. Relaunch,
inspect an empty folder, then XP, register and observe the table. Add FT through
Options > Game Data; restart and verify both locations remain. Inspect a copy
with a removed WAV to verify the named warning and successful retry after repair.
The audit registry override is excluded from normal builds.
