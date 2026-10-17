# Display sizing and high DPI (#32)

The Graphics menu now offers window widths of 800, 1280, 1920 and 2560 pixels.
These sizes retain the selected table's proportions and reserve room for the
menu. They are capped to the monitor's usable area. Window dimensions and
maximization are saved; startup also caps saved dimensions when using a smaller
monitor. Fullscreen uses the desktop resolution and remains selected when the
application loses focus. Windowed dimensions are not overwritten by fullscreen
or maximized size events.

Uniform scaling remains enabled by default. Letterboxing preserves the table
ratio; integer scaling and linear filtering remain optional and persistent.
The renderer uses at least a one-pixel extent while minimized. Clamping the
unused margins also avoids a one-pixel floating-point overflow at some sizes.

The default table resolution remains the highest supported by the selected data
(Windows 600 x 416 table, Full Tilt 960 x 666 table in the 1024 x 768 resource).
A manually selected Full Tilt resolution is retained while temporarily playing
Windows; only the effective resolution is clamped, not the preference. These
are scaled historical images, not newly drawn HD assets.

On Windows SDL requests per-monitor DPI awareness before initialization.
Renderer, mouse and window coordinates remain pixels. The effective window DPI (GetDpiForWindow on Windows 10+) scales
both ImGui text and padding; the existing UI Scale option is a further user
factor (0.8 to 3). Fonts are rasterized at their actual rounded display size (13 pixels times
DPI and user scaling), with FontGlobalScale fixed to 1: e.g. 20 pixels at 144 DPI.
The atlas is rebuilt between frames only when this pixel size changes. This avoids
the observed loss of strokes when reducing a 26-pixel default font to 13 pixels.
Moving between monitors updates the scale without restarting or compounding
style scaling. The initial window dimensions also scale with the DPI when no
window size has been saved. A Display stdout diagnostic reports effective DPI, font size, window dimensions
and renderer dimensions whenever the font changes. Systems that cannot report DPI fall
back to 96 DPI. No window-DPI logical-coordinate hint is enabled.

Import, startup mode selection, mode confirmation and controls dialogs are
centered and constrained to the viewport. Their content can scroll in a small
window. Import diagnostics have their own scroll area; mode confirmation text
wraps and its two actions occupy separate rows.

Validation performed locally:

- `tests/display_layout_test.cpp`, compiled as C++11 with assertions enabled:
  120 combinations of three table resolutions, five window sizes including
  3840 x 2160 and portrait 2160 x 3840, four menu heights and integer scaling
  on/off. Bounds and uniform ratios pass, plus minimized/zero-size handling.
- Windows x64 audit build with Zig 0.13, SDL 2.30.11 and SDL_mixer 2.8.0.
- This change does not modify simulation scheduling, collision geometry or game
  rules. The #25 deterministic cadence results remain separate evidence.

Interactive checks on 2026-10-17 used isolated audit preferences and the two
registered reference directories. No normal player settings or OS DPI settings
were changed.

- The final font build reported effective DPI 144 and a 20px font at UI Scale 1.
  Full Tilt was observed in desktop fullscreen and returned to its 1280 x 916
  renderer/window size through the Full Screen menu command.
- Changing UI Scale from 1 to 1.5 in the open Graphics menu rebuilt the font to
  29px between frames. The labels enlarged immediately and remained readable.
  The import dialog stayed within the window with wrapped paths and accessible
  Inspect/Cancel buttons. The mode-change dialog also fitted with both actions
  visible; switching to Windows retained the window size and UI scale.
- Windows was observed in both windowed and desktop-fullscreen modes. Closing
  through Game > Exit saved FullScreen=1, UI Scale=1.500000, Window Width=1280,
  Window Height=916 and Selected historical mode=windows to the isolated INI.
- Restarting the statically linked, instrumented single executable from #31
  (`audit/issue31/final-audit/SpaceCadetPinball.exe`, production base 04c998d)
  restored Windows in fullscreen and the enlarged UI. Returning to windowed
  mode restored the same size. Its Game Data menu showed both registered modes
  and Windows selected. It then closed normally; no test game process remained.
  This is an interactive smoke test on the development machine, not a clean
  Windows installation test. The instrumented build enables fixture isolation.
- Both builds used the SDL Direct3D renderer. The diagnostic confirmed equal
  window and renderer pixel dimensions. Sky screenshots report logical sizes
  (e.g. 855 x 642 including the frame for a 1280 x 916 client at 144 DPI), so
  their dimensions must not be mistaken for physical renderer resolution.

The test did not change OS DPI or move between monitors of different DPI. That
transition remains to observe. Automated geometry checks cover 4K dimensions;
interactive captures establish display operation, not the user's subjective
comfort. User comfort and readability on their 4K screen remain pending. No
new deterministic physics/cadence claim is made from these UI checks.
