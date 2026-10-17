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
Renderer, mouse and window coordinates remain pixels. The display DPI scales
both ImGui text and padding; the existing UI Scale option is a further user
factor (0.8 to 3). The font is rasterized at 26 pixels and displayed relative to
the original 13-pixel size. Moving between monitors updates the scale without
restarting or compounding style scaling. Systems that cannot report DPI fall
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

Still to observe in the integrated executable: both modes in window/fullscreen,
resize and DPI changes; dimensions/fullscreen/filters after restart; import and
mode dialogs in the small Windows window. The user's comfort and readability
assessment on their 4K screen is explicitly still pending. Mathematical 4K
layout checks are not a claim of human validation or a real 4K screenshot.
