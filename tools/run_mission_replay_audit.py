"""Run real-component replay checks using local resources and SDL dummy drivers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--data', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=False)
output = args.output.resolve()
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
           PINBALL_AUDIT_REPLAY='1', PINBALL_AUDIT_MISSION_REPLAY='1',
           PINBALL_AUDIT_INI=str(output / 'settings.ini'),
           PINBALL_CADENCE_CSV=str(output / 'cadence.csv'))
result = subprocess.run([str(args.exe.resolve()), '-sw', '-noaudio'],
                        cwd=args.data.resolve(), env=env, capture_output=True,
                        timeout=65, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
(output / 'stdout.txt').write_bytes(result.stdout)
(output / 'stderr.txt').write_bytes(result.stderr)
checks = [line for line in result.stdout.decode(errors='replace').splitlines()
          if line.startswith('mission_replay:')]
evidence = {'executable_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'driver': 'SDL dummy video/audio; noaudio', 'exit_code': result.returncode,
            'checks': checks, 'resources_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
              for p in args.data.iterdir() if p.suffix.lower() == '.dat'}}
(output / 'result.json').write_text(json.dumps(evidence, indent=2) + '\n')
print(json.dumps(evidence, indent=2))
if result.returncode or 'mission_replay:COMPLETE' not in checks:
    raise SystemExit('Real-component mission replay audit did not complete')
