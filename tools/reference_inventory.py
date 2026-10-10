"""Read-only reference inventory. Python 3.10+, standard library only."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys


def sound_names(path):
    """Read sound filenames from PARTOUT groups, never extract asset content."""
    with path.open('rb') as stream:
        def read(size):
            value = stream.read(size)
            if len(value) != size:
                raise ValueError('Truncated DAT')
            return value

        header = read(183)
        if header[:21].rstrip(b'\0') != b'PARTOUT(4.0)RESOURCE':
            raise ValueError('Unknown DAT signature')
        groups = struct.unpack_from('<H', header, 175)[0]
        read(struct.unpack_from('<H', header, 181)[0])
        names = set()
        for _ in range(groups):
            fields = {}
            for _ in range(read(1)[0]):
                kind = read(1)[0]
                if kind > 12:
                    raise ValueError('Unknown DAT field type')
                size = 2 if kind in (0, 2) else struct.unpack('<I', read(4))[0]
                # Bound reads even for malformed/untrusted input.
                if size > path.stat().st_size - stream.tell():
                    raise ValueError('DAT field exceeds file size')
                if kind in (0, 9):
                    fields[kind] = read(size)
                else:
                    stream.seek(size, 1)
            if fields.get(0) == b'\xca\x00' and 9 in fields:
                name = fields[9].rstrip(b'\0').decode('ascii')
                if not name or '/' in name or '\\' in name or ':' in name or name in ('.', '..'):
                    raise ValueError('Unsafe sound filename')
                names.add(name)
        return sorted(names)


def inventory(root, catalog, layout):
    result = {'schema_version': 1, 'layout_hint': layout,
              'edition_authenticated': False, 'files': [], 'diagnostics': []}
    files = {}
    for path in sorted(root.rglob('*')):
        if path.is_symlink():
            continue
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        key = relative.casefold()
        if key in files:
            result['diagnostics'].append({'path': relative, 'error': 'Case-insensitive path collision'})
        files[key] = relative
        item = {'path': relative}
        try:
            hashes = [hashlib.sha1(), hashlib.sha256()]
            size = 0
            with path.open('rb') as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b''):
                    size += len(block)
                    for digest in hashes:
                        digest.update(block)
            item.update(size=size, sha1=hashes[0].hexdigest(), sha256=hashes[1].hexdigest())
            item['matches'] = [entry for entry in catalog['fingerprints']
                               if entry['sha1'] == item['sha1']]
            item['recognition'] = 'documented-fingerprint' if item['matches'] else 'unknown'
        except OSError as error:
            item['error'] = type(error).__name__
            result['diagnostics'].append({'path': relative, 'error': item['error']})
        result['files'].append(item)
    dat = 'cadet.dat' if layout == 'full-tilt' else 'pinball.dat'
    if dat not in files:
        result['diagnostics'].append({'path': dat, 'error': 'Missing table data'})
    else:
        try:
            prefix = 'sound/' if layout == 'full-tilt' else ''
            required = [prefix + name for name in sound_names(root / files[dat])]
            result['referenced_sounds'] = required
            result['missing_sounds'] = [name for name in required if name.casefold() not in files]
        except (OSError, ValueError) as error:
            result['diagnostics'].append({'path': dat, 'error': str(error)})
    tracks = ['sound/taba1', 'sound/taba2', 'sound/taba3'] if layout == 'full-tilt' else ['pinball']
    result['missing_music'] = [track for track in tracks
                               if not any(track + ext in files for ext in ('.mid', '.mds'))]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--layout', required=True, choices=['windows', 'full-tilt'],
                        help='Expected folder layout only; does not identify an edition')
    args = parser.parse_args()
    if not args.directory.is_dir():
        parser.error('directory must be an existing resource folder')
    catalog = json.loads((Path(__file__).resolve().parent.parent / 'Doc/reference-fingerprints.json').read_text())
    report = inventory(args.directory, catalog, args.layout)
    print(json.dumps(report, indent=2, ensure_ascii=True))
    return 1 if report['diagnostics'] or report.get('missing_sounds') or report['missing_music'] else 0


if __name__ == '__main__':
    sys.exit(main())
