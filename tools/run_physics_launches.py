"""Measure manual and automatic plunger pulses in an audit build, with isolated prefs.

The results describe the fork only. No claim of original trajectory equivalence.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--mode', choices=['windows', 'full-tilt'], required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    for scenario in ('weak', 'maximum', 'automatic'):
        base = (args.output / scenario).resolve()
        if any(args.output.glob(scenario + '.*')):
            raise FileExistsError(base)
        env = {k: v for k, v in os.environ.items() if not k.startswith(('PINBALL_AUDIT_', 'PINBALL_PHYSICS_', 'PINBALL_CADENCE_'))}
        env.update(PINBALL_AUDIT_REPLAY='1', PINBALL_AUDIT_FPS='60',
                   PINBALL_AUDIT_INI=str(base) + '.ini', PINBALL_CADENCE_CSV=str(base) + '.csv',
                   PINBALL_PHYSICS_SCENARIO=scenario)
        with open(str(base) + '.stdout', 'w') as out, open(str(base) + '.stderr', 'w') as err:
            subprocess.run([str(args.exe.resolve()), '-noaudio'], cwd=args.data.resolve(), env=env,
                           stdout=out, stderr=err, timeout=65, check=True,
                           creationflags=subprocess.CREATE_NO_WINDOW)
        with open(str(base) + '.csv.physics.csv') as source:
            samples = [{k: float(v) for k, v in row.items()} for row in csv.DictReader(source)]
        pulse = [s for s in samples if s['plunger_threshold'] == 0 and s['plunger_boost'] > 0]
        if len(samples) != 2160 or not pulse:
            raise RuntimeError('Incomplete replay or no positive launch pulse')
        result = dict(scenario=scenario, samples=len(samples), pulse_samples=len(pulse),
                      pulse_first_sample_ms=pulse[0]['time_ms'], pulse_last_sample_ms=pulse[-1]['time_ms'],
                      pulse_boost=pulse[0]['plunger_boost'],
                      pulse_sample_coverage_ms=len(pulse) * 1000 / 120)
        results.append(result)
        print(json.dumps(result), flush=True)
    (args.output / 'summary.json').write_text(json.dumps(dict(
        mode=args.mode, executable_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
        simulation_hz=120, results=results,
        limitations='Fork measurements only; duration quantized by fixed steps; no original trajectory comparison.'
    ), indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
