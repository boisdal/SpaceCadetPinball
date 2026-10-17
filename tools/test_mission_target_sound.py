"""Compile the real controller/audio method bodies against observable test doubles.

No game assets, SDL device, or window are used. This checks event selection and
missing-file handling, not audio decoding, audible fidelity, or importer UX.
Usage: python tools/test_mission_target_sound.py --cxx path/to/zig --zig
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def body(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--zig", action="store_true")
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1] / "SpaceCadetPinball")
    args = parser.parse_args()
    methods = []
    for filename, signatures in [
        ("control.cpp", ["void control::MissionSpotTargetControl("]),
        ("Sound.cpp", ["void Sound::PlaySound(", "Mix_Chunk* Sound::LoadWaveFile("]),
        ("loader.cpp", ["float loader::play_sound("]),
    ]:
        source = (args.source / filename).read_text(encoding="utf-8")
        methods.extend(body(source, signature) for signature in signatures)
    fixture = Path(__file__).with_suffix(".cpp").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="pinball-sounds-") as temp:
        source = Path(temp) / "test.cpp"
        executable = Path(temp) / "test.exe"
        source.write_text(fixture.replace("// ACTUAL_METHODS", "\n\n".join(methods)), encoding="utf-8")
        command = [args.cxx] + (["c++"] if args.zig else [])
        subprocess.run(command + ["-std=c++11", "-O0", str(source), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], cwd=temp, check=True)


if __name__ == "__main__":
    main()
