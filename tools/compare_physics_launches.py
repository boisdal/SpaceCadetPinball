"""Verify and archive the 12 before/after launch replays for issue #26."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path


def compare(source, destination):
    destination.mkdir(parents=True, exist_ok=True)
    report = {'schema_version': 1, 'runs': [], 'comparisons': [],
              'limit': 'Fork before/after comparison, not original trajectory equivalence.'}
    for mode in ('windows', 'full-tilt'):
        for version in ('baseline', 'fixed'):
            directory = source / (mode + '-' + version)
            summary = json.loads((directory / 'summary.json').read_text())
            assert summary['mode'] == mode and summary['simulation_hz'] == 120
            for case in summary['results']:
                expected = 23 if (mode, version, case['scenario']) == ('windows', 'fixed', 'automatic') else 2
                assert case['samples'] == 2160 and case['pulse_samples'] == expected
                for suffix in ('.csv.physics.csv', '.csv.trajectory.csv'):
                    name = mode + '-' + version + '-' + case['scenario'] + suffix
                    raw = (directory / (case['scenario'] + suffix)).read_bytes()
                    (destination / (name + '.gz')).write_bytes(gzip.compress(raw, mtime=0))
                case['executable_sha256'] = summary['executable_sha256']
                case['mode'], case['version'] = mode, version
                report['runs'].append(case)
        for scenario in ('weak', 'maximum', 'automatic'):
            comparison = dict(mode=mode, scenario=scenario)
            for suffix, key in (('.csv.physics.csv', 'physics'), ('.csv.trajectory.csv', 'trajectory')):
                before = (source / (mode + '-baseline') / (scenario + suffix)).read_bytes()
                after = (source / (mode + '-fixed') / (scenario + suffix)).read_bytes()
                comparison[key + '_equal'] = before == after
                comparison[key + '_baseline_sha256'] = hashlib.sha256(before).hexdigest()
                comparison[key + '_fixed_sha256'] = hashlib.sha256(after).hexdigest()
                if mode == 'full-tilt' or scenario != 'automatic':
                    assert before == after, 'Unexpected regression: ' + str(comparison)
            report['comparisons'].append(comparison)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--traces', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(json.dumps(compare(args.source, args.traces), indent=2) + '\n', encoding='utf-8')
