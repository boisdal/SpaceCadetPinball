"""Summarize a 45 s fork capture or the final 45 s of a 55 s native-title log."""
import argparse
import csv
import gzip
import json
import math
from pathlib import Path


def distribution(values):
    ordered = sorted(values)
    if not ordered:
        raise ValueError('No measurements')
    return dict(min=min(values), mean=sum(values)/len(values),
                p50=ordered[math.ceil(len(values)*.5)-1],
                p95=ordered[math.ceil(len(values)*.95)-1],
                p99=ordered[math.ceil(len(values)*.99)-1], max=max(values))


def summarize(path):
    opener = gzip.open if path.suffix == '.gz' else open
    if path.name.endswith(('.csv', '.csv.gz')):
        with opener(path, 'rt', encoding='utf-8') as stream:
            rows = list(csv.DictReader(stream))
        if rows[-1]['event'] != 'E' or float(rows[-1]['value']) != 1:
            raise ValueError('Incomplete or overflowing capture')
        result = dict(method='instrumented fork', duration_s=45,
                      capture_end_s=float(rows[-1]['seconds']),
                      states=[r for r in rows if r['event'] == 'S'])
        for kind, label in [('U', 'updates'), ('R', 'presents')]:
            events = [r for r in rows if r['event'] == kind]
            times = [float(r['seconds']) for r in events]
            result[label] = dict(count=len(times), per_second=len(times)/45,
                                 interval_ms=distribution([(b-a)*1000 for a, b in zip(times, times[1:])]))
        result['simulation_dt_ms'] = distribution([float(r['value']) for r in rows if r['event'] == 'U'])
        result['simulation_seconds'] = sum(float(r['value']) for r in rows if r['event'] == 'U')/1000
        if not result['states'] or any(float(r['value']) != 1 for r in result['states']):
            raise ValueError('Inactive/paused capture; inspect raw states')
        return result
    with opener(path, 'rt', encoding='utf-8') as stream:
        data = json.load(stream)
    if data['duration_s'] < 55:
        raise ValueError('Native capture requires 10 s warm-up and 45 s sampling')
    samples = [s for s in data['samples'] if 10 <= s['elapsed_s'] < 55]
    if not samples or any(not s['foreground'] or s['diagnostic_rate'] is None for s in samples):
        raise ValueError('Missing diagnostic or foreground lost')
    return dict(method='native title; repeated polls, not independent batches',
                duration_s=45, discarded_warmup_s=10, polls=len(samples),
                diagnostic_rate=distribution([s['diagnostic_rate'] for s in samples]),
                title_value_changes=sum(a['diagnostic_rate'] != b['diagnostic_rate']
                                        for a, b in zip(samples, samples[1:])))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path', type=Path)
    args = parser.parse_args()
    print(json.dumps(summarize(args.path), indent=2))
