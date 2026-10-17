# Integrated replay checks

Source: `06fcb74`, combining the imported mode selection/display changes with
both audit extensions and the XP plunger correction. Audit executable SHA-256:
`74d80320c03c0c9a593fad82b42536af803d90b40e342de28528eaf6ec110f6c`.

`run_mission_replay_audit.py` passed 19 checks each for Windows and Full Tilt.
`run_multiball_audit.py` passed both Full Tilt runs: 2160 steps and 3599 ball
states each, with identical traces. All used SDL dummy video/audio and isolated
settings. These are fork integration checks, not runs of historical executables
or measurements of audio, display comfort, or natural collision reachability.
