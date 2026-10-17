"""Read-only XP component evidence and authenticated DAT parameter comparison.

No game execution. Values and branch interpretations are static evidence, not
proof that the two collision solvers produce the same trajectories.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from inspect_windows_physics import load_xp_reader

ROOT = Path(__file__).resolve().parent.parent
NAMES = ('a_flip1', 'a_flip2', 'ramp_hole', 'a_kout1', 'a_kout2', 'a_kout3', 'table')
ATTRIBUTES = {305, 306, 407, 408, 701, 703, 800, 801, 802, 803, 804, 805, 1304}


def parameters(path, reference, filename):
    manifest = json.loads((ROOT / 'Doc/reference-manifests' / (reference + '.json')).read_text())
    expected = next(f['sha256'] for f in manifest['files'] if f['path'].lower() == filename)
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected:
        raise ValueError('DAT does not match authenticated reference: ' + reference)
    position = 0

    def read(n):
        nonlocal position
        if n < 0 or position + n > len(data):
            raise ValueError('Truncated DAT')
        value = data[position:position + n]
        position += n
        return value

    header = read(183)
    if header[:21].rstrip(b'\0') != b'PARTOUT(4.0)RESOURCE':
        raise ValueError('Unknown DAT signature')
    read(struct.unpack_from('<H', header, 181)[0])
    result = {}
    for index in range(struct.unpack_from('<H', header, 175)[0]):
        name, attrs = '', {}
        for _ in range(read(1)[0]):
            kind = read(1)[0]
            if kind > 12:
                raise ValueError('Unknown DAT field')
            length = 2 if kind in (0, 2) else struct.unpack('<I', read(4))[0]
            value = read(length)
            if kind == 3:
                name = value.rstrip(b'\0').decode('ascii')
            if kind == 11:
                if len(value) < 4 or len(value) % 4:
                    raise ValueError('Invalid float array')
                values = struct.unpack('<' + 'f' * (len(value) // 4), value)
                key = math.floor(values[0])
                if key in ATTRIBUTES:
                    attrs[str(key)] = list(values[1:])
        if name in NAMES:
            result[name] = {'group_index': index, 'attributes': attrs}
    if set(result) != set(NAMES):
        raise ValueError('Missing expected component')
    return {'reference': reference, 'sha256': digest, 'components': result}


def angle(attrs):
    origin, source, destination = (attrs[str(k)] for k in (800, 801, 802))
    a = [source[i] - origin[i] for i in range(2)]
    b = [destination[i] - origin[i] for i in range(2)]
    value = math.acos(sum(a[i] * b[i] for i in range(2)) / (math.hypot(*a) * math.hypot(*b)))
    return math.copysign(value, a[0] * b[1] - a[1] * b[0])


def inspect(exe, xp_dat, ft_dat):
    read, digest = load_xp_reader(exe)

    def scalar(fmt, va):
        return struct.unpack(fmt, read(va, struct.calcsize(fmt)))[0]

    # Vtable routes and the load instructions connect constants to the methods.
    checks = [
        (0x1002690 + 20, struct.pack('<I', 0x1016745)),
        (0x10026ac + 20, struct.pack('<I', 0x1018476)),
        (0x10026ac + 24, struct.pack('<I', 0x10169c0)),
        (0x101847e, bytes.fromhex('837e4e00')),
        (0x10184af, bytes.fromhex('d905d0240001')),
        (0x101678e, bytes.fromhex('d9057c1a0001')),
        (0x10167ec, bytes.fromhex('d9465e')),
        (0x1019a83, bytes.fromhex('c743620000003f')),
        (0x101c308, bytes.fromhex('8983d5000000')),
        (0x101c311, bytes.fromhex('8983d9000000')),
        (0x101c31a, bytes.fromhex('898395000000')),
        (0x1002854, struct.pack('<I', 0x101be06)),
        (0x101be94, bytes.fromhex('d94645d9e1d8b6d1000000')),
        (0x101bebf, bytes.fromhex('d88e95000000')),
    ]
    for address, expected in checks:
        if read(address, len(expected)) != expected:
            raise ValueError('Instruction/vtable mismatch at ' + hex(address))
    xp = parameters(xp_dat, 'windows-xp-x86-en', 'pinball.dat')
    ft = parameters(ft_dat, 'full-tilt-win95-en', 'cadet.dat')
    flippers = []
    for name in ('a_flip1', 'a_flip2'):
        a, b = xp['components'][name]['attributes'], ft['components'][name]['attributes']
        sweep = angle(a)
        flippers.append({
            'name': name, 'geometry_equal': all(a[str(k)] == b[str(k)] for k in (800, 801, 802)),
            'angle_radians_derived_double': sweep,
            'angle_degrees_derived_double': math.degrees(sweep),
            'collision_multiplier_xp': a['803'][0], 'collision_multiplier_ft': b['803'][0],
            'xp_extend_seconds': a['804'][0], 'xp_retract_seconds': a['805'][0],
            'xp_extend_radians_per_second_derived_double': abs(sweep) / a['804'][0],
            'xp_retract_radians_per_second_derived_double': abs(sweep) / a['805'][0],
            'ft_extend_radians_per_second': b['804'][0], 'ft_retract_radians_per_second': b['805'][0],
        })
    return {
        'schema_version': 1, 'xp_exe_sha256': digest,
        'method': 'hash-gated static PE/DAT inspection; no game execution',
        'xp_constants': {
            'hole_capture_delay_seconds': scalar('<f', 0x10024d0),
            'hole_missing_attribute_701_default': scalar('<f', 0x1019a86),
            'kickout_default_delay_seconds': scalar('<f', 0x10198c3),
            'kickout_rearm_delay_seconds': scalar('<f', 0x10198ca),
            'kickout_tilt_delay_seconds': scalar('<f', 0x1001a7c),
            'flipper_base_radius_guard_multiplier': scalar('<d', 0x1002830),
        },
        'xp_method_addresses': {
            'hole_collision': '0x1018476', 'hole_field': '0x10169c0',
            'kickout_collision': '0x1016745', 'kickout_release': '0x10167dd',
            'throw_ball': '0x1015935', 'flipper_edge_constructor': '0x101c2ba',
            'flipper_edge_collision': '0x101be06',
        },
        'data': [xp, ft], 'flippers': flippers,
        'limits': ['Derived angles use Python double precision, not an original runtime sample',
                   'Matching parameters do not prove matching collision timing or ball trajectories',
                   'Original missing-attribute default differs but is unused by the authenticated XP DAT'],
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xp-exe', type=Path, required=True)
    parser.add_argument('--xp-dat', type=Path, required=True)
    parser.add_argument('--ft-dat', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(json.dumps(inspect(args.xp_exe, args.xp_dat, args.ft_dat), indent=2) + '\n', encoding='utf-8')
