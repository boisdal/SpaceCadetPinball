"""Run the audit executable with isolated settings and controlled inputs (Windows)."""
import argparse
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', action='append', required=True,
                        help='name:fps:width:height:stall_ms[:noaudio|resize|hidpi]')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for case in args.case:
        name, fps, width, height, stall, *extra = case.split(':')
        base = (args.output / name).resolve()
        if base.with_suffix('.csv').exists():
            raise FileExistsError(base)
        env = dict(os.environ, PINBALL_AUDIT_REPLAY='1', PINBALL_AUDIT_FPS=fps,
                   PINBALL_AUDIT_WIDTH=width, PINBALL_AUDIT_HEIGHT=height,
                   PINBALL_AUDIT_STALL_MS=stall, PINBALL_AUDIT_INI=str(base)+'.ini',
                   PINBALL_AUDIT_RESIZE='1' if 'resize' in extra else '0',
                   PINBALL_CADENCE_CSV=str(base)+'.csv')
        env['SDL_WINDOWS_DPI_SCALING'] = '1' if 'hidpi' in extra else '0'
        command = [str(args.exe.resolve())] + (['-noaudio'] if extra == ['noaudio'] else [])
        with open(str(base)+'.stdout', 'w') as out, open(str(base)+'.stderr', 'w') as err:
            subprocess.run(command, cwd=args.data.resolve(), env=env, stdout=out, stderr=err,
                           timeout=65, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
        if not Path(str(base)+'.csv.trajectory.csv').exists():
            raise RuntimeError(f'{name}: replay did not finish')
        print(json.dumps({'case': case, 'output': str(base)}), flush=True)


if __name__ == '__main__':
    main()
