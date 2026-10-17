# Full Tilt mission replays (#23)

The authenticated Full Tilt WIN95 `CADET.EXE` (SHA-1
`3f7b5699074b83fd713657cd94671f2156dbedc4`) contains sixteen calls to replay
routine `0x422ce0` in its 33-state mission dispatch table. Its executable code
and data sections match the earlier unpacked reference used for the audit.
The calls follow the transition back to mission selection and precede the
completion reward. The argument `0xbf800000` is `-1.0f`: it controls the replay
message display, not the duration of the award.

The fork now calls its existing `table_set_replay(-1.0f)` in these terminal
branches, guarded by `pb::FullTiltMode`:

| State | Terminal controller | Reference call |
| --- | --- | --- |
| 2 | PracticeMission | 0x426f2e |
| 3 | LaunchTraining | 0x4270d7 |
| 4 | ReentryTraining | 0x427278 |
| 5 | ScienceMission | 0x4274b4 |
| 6 | StrayComet | 0x427754 |
| 7 | BlackHoleThreat | 0x427baf |
| 8 | SpaceRadiation | 0x427963 |
| 9 | BugHunt | 0x427f41 |
| 11 | RescueMission | 0x428298 |
| 12 | Satellite | 0x428827 |
| 13 | Reconnaissance | 0x428a52 |
| 14 | DoomsdayMachine | 0x428c3e |
| 20 | AlienMenacePartTwo | 0x42864f |
| 21 | CosmicPlaguePartTwo | 0x428f13 |
| 23 | SecretMissionGreen | 0x4291ee |
| 31 | MaelstromPartEight | 0x429ed2 |

Time Warp deliberately awards no replay, including its terminal second stage.
Mission initialization and intermediate phases also award none. Windows mode
does not enter the new calls; existing hyperspace replay behavior is unchanged.

The existing replay helper turns on `lite199` and displays `STRING101`. The
existing drain controller consumes this light, relaunches without decrementing
the player's ball count, and keeps the current player. Ball-save protection
(`lite200`) takes precedence and preserves an earned replay. Tilt clears both
lights before drain selection. After replay consumption and the subsequent
ball-save period, ordinary loss advances the ball/player normally. Awards are
boolean, not a stack of extra balls. No drain or lamp behavior was changed.

## Verification

Run `python tools/test_mission_replays.py --cxx /path/to/c++` (or the path to
`zig.exe`). This builds the actual controller functions extracted from the
current `control.cpp`, with observable component doubles. It checks both modes:

- all sixteen terminal events, including replay message argument and lamp;
- thirteen intermediate/exception transitions and their destination state;
- initialization and progress messages, unrelated and nonfinal hits;
- replay consumption, ball-save precedence, tilt cancellation, ball counts,
  and whether the drain requests player rotation.

These tests exercise production event routing but substitute components and
mission dispatch. They do not prove rendering, sound playback, real timer
execution, or per-player lamp persistence. Source inspection of
`TLight::Message(PlayerChanged)` confirms that lamp state is stored/restored per
player; the normal drain consumes or clears the replay before that transition.
A normal Windows x64 build is also required. Interactive comparison of the
visible replay message/lamp and relaunch against the original remains a separate
validation, not a claim made by this test harness.

On 2026-10-17 the event harness passed with Zig 0.13.0, and the normal x64
build succeeded with SDL 2.30.11 / SDL_mixer 2.8.0. Local executable SHA-256:
`e624c5824710da4eba2dae8b16569d94915afe05bde5df2dd95717dddf0f5c42`.
