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

Unknown fingerprints are never assigned a mode from their filename. If the
container is compatible, the form requires an explicit Windows or Full Tilt
confirmation before registration. The selected mode and unknown provenance are
persisted alongside the exact fingerprint in registry format v2 (v1 remains
readable). Restart revalidates the confirmed mode and fingerprint; a changed DAT
requires inspection and confirmation again. Historical missing-audio exceptions
are not applied to unknown tables.

The format check bounds header/group/field reads, strings, numeric arrays,
bitmap/depth-map resolutions and allocations, spliced bitmap decoding and object
group references. Missing component names used by `control.cpp` (including
`lite199`) are rejected before any table is instantiated. This does not validate
every physics attribute, component state or cross-reference consumed by legacy
`loader` and `control` code. The chosen mode must fit depth-map headers, ball/font groups
and required camera/background records. Limits are 64 MiB encoded data, 4,096
groups, 4,096-pixel dimensions and 256 MiB estimated decoded allocation. These
are format/resource limits, not a claim to authenticate or fully sanitize every
possible gameplay field. An unknown variant still needs gameplay validation.

## Integration API

- `ResourceCatalog::Inspect(directory, manualMode = -1)` returns `ResourceInspection` with
  `valid`, `fullTilt`, `knownFingerprint`, `needsManualMode`, `datName`,
  `fingerprint`, `errors`, `warnings`, `observations`. Manual mode 0 is Windows;
  1 is Full Tilt. Omitted mode never silently approves an unknown table.
- `Register(inspection, error)` revalidates and commits only the matching slot.
- `Get(bool fullTilt)` returns that slot's absolute source path; `Load(error)`
  restores both slots transactionally.
- `InspectRegistered(mode)` revalidates the persisted mode and fingerprint.
- `ResourceImport::Catalog()` exposes the active registry to #28.
- `ResourceImport::SelectPreferred(bool preferWindows)` selects a registered
  table and assigns `pb::BasePath`, `DatFileName`, `FullTiltMode`. It never falls back
  to another mode (#28 provides the explicit selection flow).
- `ResourceImport::RunSetup/Open/Draw` provide setup and in-game import UI.

## Verification

On Windows, 17 October 2026:

- Standalone C++11 tests: SHA-256 empty/abc/million-a vectors; XP/FT recognition;
  registration, both slots after restart, reimport, invalid import rollback,
  failed save rollback, UTF-8 registry filename, corrupt registry preservation, v1-to-v2 migration.
- `tools/test_resource_import.py`: unknown, absent, incomplete, repaired, modified
  audio, renamed DAT and ambiguous folders; unknown variants require explicit
  mode, retain provenance on restart and reject the wrong mode; 16 corrupted DAT
  mutations rejected; SHA-256 of both source trees unchanged.
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
no DAT. Back/cancellation must leave setup with neither a registry nor any source changes. Relaunch,
inspect an empty folder, then XP, register and observe the table. Add FT through
Options > Game Data; restart and verify both locations remain. Inspect a copy
with a removed WAV to verify the named warning and successful retry after repair.
The audit registry override is excluded from normal builds.
