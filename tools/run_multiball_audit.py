"""Run the real loaded FT game with dummy SDL video and isolated preferences.

Requires a -CadenceAudit build. No UI window, audio playback or original EXE run.
"""
import argparse
import csv
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--data', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--runs', type=int, default=2)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
results = []
for number in range(args.runs):
    base = (args.output / f'ft-{number+1}').resolve()
    if list(args.output.glob(base.name + '.*')):
        raise FileExistsError(base)
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', PINBALL_AUDIT_MULTIBALL='1',
               PINBALL_AUDIT_REPLAY='1', PINBALL_AUDIT_FPS='60',
               PINBALL_AUDIT_INI=str(base)+'.ini', PINBALL_CADENCE_CSV=str(base)+'.csv')
    # Do not inherit unrelated replay injections from another audit.
    for name in ['PINBALL_AUDIT_STALL_MS','PINBALL_AUDIT_RESIZE','PINBALL_AUDIT_LAUNCH']:
        env.pop(name,None)
    with open(str(base)+'.stdout','w') as out, open(str(base)+'.stderr','w') as err:
        subprocess.run([str(args.exe.resolve()),'-noaudio'], cwd=args.data.resolve(),env=env,
                       stdout=out,stderr=err,timeout=90,check=True,
                       creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    log = Path(str(base)+'.csv.multiball.csv')
    data = log.read_bytes()
    rows = list(csv.DictReader(data.decode().splitlines()))
    events = [line for line in Path(str(base)+'.stdout').read_text().splitlines()
              if line.startswith(('Multiball event','PASS:'))]
    if len([x for x in events if x.startswith('Multiball event')]) != 13:
        raise RuntimeError('Not all integration stages completed')
    if len({r['time_ms'] for r in rows}) != 2160 or rows[-1]['event'] != '13':
        raise RuntimeError('Incomplete simulation trace')
    compressed = Path(str(log)+'.gz')
    compressed.write_bytes(gzip.compress(data,mtime=0))
    results.append({'run':number+1,'simulation_steps':2160,'ball_rows':len(rows),
                    'trace_sha256':hashlib.sha256(data).hexdigest(),
                    'events':events,'trace':compressed.name})
if len({r['trace_sha256'] for r in results}) != 1:
    raise RuntimeError('Repeated integration traces differ')
report={'schema_version':1,'mode':'full-tilt','simulation_hz':120,'duration_seconds':18,
        'seed':1,'sdl_video':'dummy','audio':False,
        'exe_sha256':hashlib.sha256(args.exe.read_bytes()).hexdigest(),
        'dat_sha256':hashlib.sha256((args.data/'CADET.DAT').read_bytes()).hexdigest(),
        'runs':results,
        'limitations':['Fork integration only; no execution of the historical reference.',
                       'Sink/drain/kickout/hole collisions are injected at component entry points.',
                       'Lock eligibility and kickout tilt path are controlled fixture state.',
                       'Does not prove natural collision reachability, historical trajectories, audio or visual behavior.']}
(args.output/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'runs':len(results),'identical_traces':True,'steps_per_run':2160}))
