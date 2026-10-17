# Historical mode selection (#28)

`Options > Game Data` shows the active mode and the registered Windows / Full Tilt
choices. An unavailable mode is disabled with instructions to import or restore its
folder. Resource validity is refreshed when the menu opens and checked again before
switching. The window title includes the mode, including when the frame-rate display
is enabled.

Selecting another mode pauses play and asks whether to end the current game and
start a new one. Cancellation keeps the game and its previous pause state. Confirmation
persists the selected mode, then uses the existing complete restart path: table,
audio, resolution and mode-specific records are reloaded. The current table's mode
is restored before teardown so its scores cannot be written to the destination mode.
An unfinished game's score is not submitted as a high score.

The setting `Selected historical mode` uses stable values `windows` and `full-tilt`.
It is independent of the former preference toggle. If the saved source disappears,
the startup chooser explains the problem and requires an explicit replacement
choice or import; another registered source never substitutes automatically.
The first run with registered imports also presents this chooser. Import cancellation
returns to the chooser. Saved resource folders and scores are not removed.

Existing portable installations without an import registry still discover their
local data using the established search paths. Their mode is remembered on first
launch. Subsequent discovery must match the saved mode; a different table requires
an explicit selection/import. This preserves portable installations and comparison
builds while preventing a silent historical-mode change.

## Validation

`tests/run_mode_selection.ps1` builds an assertion-enabled integration executable
against the real game sources, ImGui settings and resource catalog. Supply the same
`-Zig`, `-Sdl`, `-Mixer` arguments as `tools/build_comparison_windows.ps1`, plus an
isolated `-OutputDirectory`, `-WindowsResources` and `-FullTiltResources`.

It copies only the two DAT files into fresh test folders, never changes original
resources or player preferences, and exercises Windows-only, Full-Tilt-only, both
sources, persistence across separate processes, and a missing Full Tilt DAT while
Windows remains available. Failed selection must preserve the saved choice and must
not load the other table. Audio warnings are expected for these DAT-only fixtures.

The normal Windows build also compiles. Interactive confirmation/cancellation and
the startup chooser require a desktop pass; the integration executable does not
claim to simulate user clicks or validate visual layout.
