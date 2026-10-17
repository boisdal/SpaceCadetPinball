"""Exercise the import catalog against private reference folders without changing them.
Usage: python tools/test_resource_import.py TEST_EXE WINDOWS_FOLDER FULL_TILT_FOLDER
No proprietary data is written to the repository.
"""
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import struct
import tempfile


def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def run(exe, *args, valid=True):
    result = subprocess.run([str(exe), *map(str, args)], capture_output=True, text=True)
    assert (result.returncode == 0) == valid, result.stdout + result.stderr
    return result.stdout


def main():
    exe, windows, full_tilt = map(Path, sys.argv[1:])
    before = [snapshot(windows), snapshot(full_tilt)]
    with tempfile.TemporaryDirectory(prefix='pinball-import-') as temporary:
        root = Path(temporary)
        print(run(exe, windows, full_tilt, root / 'resources.txt').strip())
        unknown = root / 'unknown'
        unknown.mkdir()
        (unknown / 'PINBALL.DAT').write_bytes(b'not a table')
        assert 'unrecognized table fingerprint' in run(exe, 'inspect', unknown, valid=False)
        empty = root / 'empty'
        empty.mkdir()
        assert '.DAT' in run(exe, 'inspect', empty, valid=False)
        incomplete = root / 'incomplete'
        shutil.copytree(windows, incomplete)
        target = next(p for p in incomplete.iterdir() if p.name.lower() == 'sound1.wav')
        target.unlink()
        warning = run(exe, 'inspect', incomplete)
        assert 'Sound unavailable:' in warning and 'SOUND1.WAV' in warning, warning
        shutil.copy2(next(p for p in windows.iterdir() if p.name.lower() == 'sound1.wav'), target)
        assert run(exe, 'inspect', incomplete).startswith('1 0 0'), 'retry after repair failed'
        target.write_bytes(b'changed wave')
        assert 'Audio differs from documented reference' in run(exe, 'inspect', incomplete)
        renamed = root / 'renamed'
        renamed.mkdir()
        shutil.copy2(next(p for p in windows.iterdir() if p.name.lower() == 'pinball.dat'), renamed / 'CADET.DAT')
        assert run(exe, 'inspect', renamed).startswith('1 0 '), 'filename dictated mode'
        shutil.copy2(next(p for p in full_tilt.iterdir() if p.name.lower() == 'cadet.dat'), renamed / 'PINBALL.DAT')
        assert 'Both tables' in run(exe, 'inspect', renamed, valid=False)
        for mode, source, dat_name in [(0, windows, 'pinball.dat'), (1, full_tilt, 'cadet.dat')]:
            variant = root / ('variant-' + str(mode))
            shutil.copytree(source, variant)
            dat = next(p for p in variant.iterdir() if p.name.lower() == dat_name)
            dat.chmod(0o600)  # Copied CD files may retain their read-only attribute.
            original = dat.read_bytes()
            changed = bytearray(original)
            changed[71] = ord('V')  # Description metadata: same compatible table, unknown fingerprint.
            dat.write_bytes(changed)
            assert 'Manual mode confirmation required' in run(exe, 'inspect', variant, valid=False)
            assert run(exe, 'inspect', variant, mode).startswith('1 ' + str(mode))
            run(exe, 'inspect', variant, 1-mode, valid=False)
            assert 'Manual mode and unknown provenance persisted' in run(exe, 'manual-persist', variant, root / ('manual-' + str(mode) + '.txt'), mode)
            # Unknown tables must not inherit missing-audio exceptions from a known fingerprint.
            assert 'SOUND2.WAV' in run(exe, 'inspect', variant, mode)
            mutations = []
            damaged = bytearray(changed); damaged[175:177] = b'\xff\xff'; mutations.append(damaged)
            damaged = bytearray(changed); damaged[21:71] = b'A' * 50; mutations.append(damaged)
            mutations.append(changed[:190])
            mutations.append(changed + b'trailing')
            damaged = bytearray(changed)
            missing_tag = damaged.index(b'lite199\0')
            damaged[missing_tag:missing_tag+7] = b'liteXYZ'
            mutations.append(damaged)

            cursor = 183 + struct.unpack_from('<H', changed, 181)[0]
            bitmap = None
            for _ in range(struct.unpack_from('<H', changed, 175)[0]):
                count = changed[cursor]; cursor += 1
                for _ in range(count):
                    kind = changed[cursor]; cursor += 1
                    if kind in (0, 2): size = 2
                    else: size = struct.unpack_from('<I', changed, cursor)[0]; cursor += 4
                    if kind == 1 and bitmap is None: bitmap = cursor
                    cursor += size
            assert bitmap is not None
            damaged = bytearray(changed); damaged[bitmap] = 3; mutations.append(damaged)
            damaged = bytearray(changed); damaged[bitmap+1:bitmap+3] = b'\xff\xff'; mutations.append(damaged)
            damaged = bytearray(changed); damaged[bitmap+9:bitmap+13] = b'\xff' * 4; mutations.append(damaged)
            for damaged in mutations:
                dat.write_bytes(damaged)
                run(exe, 'inspect', variant, mode, valid=False)
            dat.write_bytes(changed)
        print('Manual mode/provenance, wrong-mode rejection and 16 malformed DAT mutations: passed')
        print('Unknown, absent, incomplete, repaired, altered, renamed and ambiguous folders: passed')
    assert before == [snapshot(windows), snapshot(full_tilt)], 'reference folder changed'
    print('Source SHA-256 trees unchanged; no source or high-score file written')


if __name__ == '__main__':
    main()
