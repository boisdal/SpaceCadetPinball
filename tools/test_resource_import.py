"""Exercise the import catalog against private reference folders without changing them.
Usage: python tools/test_resource_import.py TEST_EXE WINDOWS_FOLDER FULL_TILT_FOLDER
No proprietary data is written to the repository.
"""
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
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
        print('Unknown, absent, incomplete, repaired, altered, renamed and ambiguous folders: passed')
    assert before == [snapshot(windows), snapshot(full_tilt)], 'reference folder changed'
    print('Source SHA-256 trees unchanged; no source or high-score file written')


if __name__ == '__main__':
    main()
