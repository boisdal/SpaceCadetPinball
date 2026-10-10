"""Validate an active cadence replay and compare every sampled game state."""
import argparse
import csv
import gzip
import hashlib
import json
from pathlib import Path
from summarize_cadence import distribution


def read(path):
    opener = gzip.open if path.suffix == '.gz' else open
    with opener(path, 'rt', encoding='utf-8') as stream:
        return list(csv.DictReader(stream))


def summarize(path, trajectory):
    rows = read(path)
    state = read(trajectory)
    if not rows or rows[-1]['event'] != 'E' or float(rows[-1]['value']) != 1:
        raise ValueError('Incomplete capture')
    states = [float(r['value']) for r in rows if r['event'] == 'S']
    if not states or any(s != 1 for s in states):
        raise ValueError('Focus lost or game paused')
    if max(float(r['speed']) for r in state) < 20 or max(int(r['score']) for r in state) <= 0:
        raise ValueError('No evidence of a launched ball scoring: reject replay')
    if max(abs(float(r['left'])) for r in state) < .5 or max(abs(float(r['right'])) for r in state) < .5:
        raise ValueError('Flippers did not move')
    duration = float(rows[-1]['seconds'])
    result = dict(duration_s=duration, state_count=len(state), final_score=int(state[-1]['score']))
    for kind, label in [('U', 'updates'), ('R', 'presents')]:
        times = [float(r['seconds']) for r in rows if r['event'] == kind]
        result[label] = dict(count=len(times), per_second=len(times)/duration,
                             interval_ms=distribution([(b-a)*1000 for a, b in zip(times, times[1:])]))
    dt = [float(r['value']) for r in rows if r['event'] == 'U']
    result['simulation_ms'] = sum(dt)
    result['dt_ms'] = distribution(dt)
    result['lost_ms'] = sum(float(r['value']) for r in rows if r['event'] == 'L')
    result['inputs'] = [int(float(r['value'])) for r in rows if r['event'] == 'K']
    if result['inputs'] != list(range(29)):
        raise ValueError('Missing replay input')
    result['phases_ms'] = {kind: distribution([float(r['value']) for r in rows if r['event'] == kind])
                           for kind in 'IAPDVWJ' if any(r['event'] == kind for r in rows)}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('trajectory', type=Path)
    parser.add_argument('--compare', type=Path)
    args = parser.parse_args()
    result = summarize(args.capture, args.trajectory)
    if args.compare:
        result['identical_trajectory'] = read(args.trajectory) == read(args.compare)
        if not result['identical_trajectory']:
            raise ValueError('Trajectories differ')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
