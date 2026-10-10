import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

from reference_inventory import inventory, sound_names


def dat_bytes(name=b'sound49D.wav\0'):
    header = bytearray(183)
    signature = b'PARTOUT(4.0)RESOURCE'
    header[:len(signature)] = signature
    struct.pack_into('<H', header, 175, 1)
    return bytes(header) + b'\x02\x00\xca\x00\x09' + struct.pack('<I', len(name)) + name


class ReferenceInventoryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def test_hash_recognition_does_not_authenticate_edition_or_depend_on_name(self):
        content = b'synthetic reference'
        catalog = {'fingerprints': [{'sha1': hashlib.sha1(content).hexdigest(), 'description': 'shared data'}]}
        path = self.root / 'renamed.bin'
        path.write_bytes(content)
        report = inventory(self.root, catalog, 'windows')
        self.assertEqual(report['files'][0]['recognition'], 'documented-fingerprint')
        self.assertFalse(report['edition_authenticated'])
        path.write_bytes(content + b'changed')
        self.assertEqual(inventory(self.root, catalog, 'windows')['files'][0]['recognition'], 'unknown')

    def test_dat_sound_dependencies_and_music_case_insensitive(self):
        (self.root / 'CADET.DAT').write_bytes(dat_bytes())
        report = inventory(self.root, {'fingerprints': []}, 'full-tilt')
        self.assertEqual(report['missing_sounds'], ['sound/sound49D.wav'])
        (self.root / 'SOUND').mkdir()
        (self.root / 'SOUND/SOUND49D.WAV').write_bytes(b'synthetic audio')
        for number in range(1, 4):
            (self.root / f'SOUND/TABA{number}.MDS').write_bytes(b'synthetic music')
        report = inventory(self.root, {'fingerprints': []}, 'full-tilt')
        self.assertEqual(report['missing_sounds'], [])
        self.assertEqual(report['missing_music'], [])
        self.assertEqual(report['diagnostics'], [])

    def test_truncated_and_unsafe_dat_are_diagnosed(self):
        path = self.root / 'PINBALL.DAT'
        for content in (b'bad', dat_bytes()[:-1], dat_bytes(b'../escape.wav\0')):
            path.write_bytes(content)
            with self.assertRaises(ValueError):
                sound_names(path)
            self.assertTrue(inventory(self.root, {'fingerprints': []}, 'windows')['diagnostics'])

    def test_windows_uses_root_sound_folder_and_empty_input_is_incomplete(self):
        self.assertTrue(inventory(self.root, {'fingerprints': []}, 'windows')['diagnostics'])
        (self.root / 'PINBALL.DAT').write_bytes(dat_bytes())
        (self.root / 'sound49D.wav').write_bytes(b'wave')
        (self.root / 'PINBALL.MID').write_bytes(b'music')
        report = inventory(self.root, {'fingerprints': []}, 'windows')
        self.assertEqual(report['missing_sounds'], [])
        self.assertEqual(report['missing_music'], [])
        self.assertFalse(report['edition_authenticated'])


if __name__ == '__main__':
    unittest.main()
