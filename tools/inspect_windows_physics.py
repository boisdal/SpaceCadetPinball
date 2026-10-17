"""Extract narrowly identified plunger evidence from the authenticated XP binary.

No disassembler dependency, execution, memory patching, or proprietary output.
Addresses were identified by constructor constants/attribute 601, its vtable,
and the Message switch; this is not a general binary/function recognizer.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

EXPECTED_SHA256 = '2bbc8234685fe2f6324040af6ea20123cf00c4a56882ce0d9074f0beefac67bc'


def load_xp_reader(path):
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        raise ValueError('Unrecognized XP executable; refusing address-based interpretation')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count, opt_size = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    sections = pe + 24 + opt_size
    image_base = struct.unpack_from('<I', data, pe + 24 + 28)[0]

    def read(va, size):
        rva = va - image_base
        for i in range(count):
            _, start, raw_size, raw = struct.unpack_from('<IIII', data, sections + i * 40 + 8)
            if start <= rva and rva + size <= start + raw_size:
                return data[raw + rva - start:raw + rva - start + size]
        raise ValueError('Unmapped address')

    return read, digest


def inspect(path):
    read, digest = load_xp_reader(path)

    def unpack(fmt, va):
        return struct.unpack(fmt, read(va, struct.calcsize(fmt)))[0]

    # Switch uses message - 1004, a byte index table, then a pointer table.
    index = unpack('<B', 0x1018745 + 1017 - 1004)
    target = unpack('<I', 0x1018729 + index * 4)
    assert unpack('<I', 0x1002750) == 0x10185ad and target == 0x1018691
    assert read(target, 11) == bytes.fromhex('d9eed95e42db4656d95e3e')
    assert read(0x101869c, 6) == bytes.fromhex('d905a4230001')
    assert read(0x1018712, 5) == bytes.fromhex('68c8730101')
    assert read(0x10173d9, 5) == bytes.fromhex('d9eed9583e')
    return {
        'schema_version': 1, 'reference': 'windows-xp-x86-en', 'exe_sha256': digest,
        'method': 'hash-gated PE reads; static evidence only',
        'constructor_va': '0x101a4f3', 'vtable_va': '0x1002750',
        'message_va': '0x10185ad', 'collision_va': '0x101875f',
        'automatic_launch_message': 1017, 'automatic_launch_branch_va': hex(target),
        'automatic_launch_reset_callback_va': '0x10173c8',
        'automatic_launch_delay_seconds': unpack('<f', 0x10023a4),
        'maximum_pullback': unpack('<I', 0x101a54d),
        'pullback_dividend': unpack('<d', 0x1002740),
        'pullback_bitmap_multiplier': unpack('<d', 0x1002748),
        'manual_release_delay_seconds': unpack('<f', 0x101a584),
        'limits': ['No dynamic original trajectory measured',
                   'No conclusion about general flipper or rebound fidelity'],
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(json.dumps(inspect(args.exe), indent=2) + '\n', encoding='utf-8')
