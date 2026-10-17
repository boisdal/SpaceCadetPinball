"""Fingerprint-bound checks of FT WIN95 lock/multiball control instructions.

Requires pefile/capstone. Does not execute or redistribute reference code.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('reference', type=Path)
parser.add_argument('--dependencies', type=Path)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
if args.dependencies:
    sys.path.insert(0, str(args.dependencies))
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

data = args.reference.read_bytes()
identity = hashlib.sha1(data).hexdigest()
if identity != '3f7b5699074b83fd713657cd94671f2156dbedc4':
    parser.error('Reference fingerprint mismatch; fixed addresses are not valid')
pe = pefile.PE(data=data)
md = Cs(CS_ARCH_X86, CS_MODE_32)
checks = [
    (0x422d32, 'cmp', 'eax, 1', 'Multiball launch gate: count <= 1'),
    (0x422d3b, 'add', 'eax, 3', 'Schedule three additional balls'),
    (0x422f42, 'cmp', 'eax, 1', 'Lock gate: count <= 1'),
    (0x422f4b, 'dec', 'eax', 'Captured ball removed from active count'),
    (0x422f5e, 'cmp', 'ecx, 2', 'Third lock checks two previous locks'),
    (0x422f63, 'mov', 'dword ptr [eax], 0', 'Clear lock count for multiball'),
    (0x422f7b, 'push', '0x40000000', 'Multiball delay: float 2.0 seconds'),
    (0x422f80, 'call', '0x422d20', 'Invoke multiball launch routine'),
    (0x422f86, 'inc', 'ecx', 'First/second lock increment'),
    (0x422f9b, 'push', '0x40000000', 'Replacement delay: float 2.0 seconds'),
    (0x422fa6, 'push', '0x3fb', 'Replacement plunger message 1019'),
]
results = []
for address, mnemonic, operand, meaning in checks:
    ins = next(md.disasm(pe.get_data(address-pe.OPTIONAL_HEADER.ImageBase, 16), address))
    if (ins.mnemonic, ins.op_str) != (mnemonic, operand):
        raise RuntimeError(f'Unexpected instruction at {address:#x}')
    results.append({'address': hex(address), 'interpretation': meaning, 'verified': True})

report = {'schema_version': 1, 'reference_sha1': identity,
          'reference_sha256': hashlib.sha256(data).hexdigest(),
          'method': 'static instruction checks, interpreted with control.cpp',
          'checks': results,
          'limitations': ['No execution of the historical game.',
                         'Does not establish physical ejection timing, trajectories or complete drain behavior.',
                         'Names are inferred from source/control structure, not original debug symbols.']}
text = json.dumps(report, indent=2) + '\n'
if args.output:
    args.output.write_text(text, encoding='utf-8')
else:
    print(text)
