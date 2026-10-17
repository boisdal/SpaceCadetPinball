"""Read-only, fingerprint-bound static evidence; never executes either EXE.

Requires pefile and capstone, optionally supplied through --dependencies.
Output contains hashes, addresses and interpreted call metadata, not game bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("--previous", type=Path)
    parser.add_argument("--dependencies", type=Path)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1] / "SpaceCadetPinball/control.cpp")
    args = parser.parse_args()
    if args.dependencies:
        sys.path.insert(0, str(args.dependencies))
    import pefile
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32

    def load(path, expected):
        data = path.read_bytes()
        if hashlib.sha1(data).hexdigest() != expected:
            parser.error("Reference fingerprint mismatch; address map must not be reused")
        return pefile.PE(data=data), {"sha1": expected, "sha256": hashlib.sha256(data).hexdigest()}

    pe, identity = load(args.reference, "3f7b5699074b83fd713657cd94671f2156dbedc4")
    previous, previous_identity = (None, None)
    if args.previous:
        previous, previous_identity = load(args.previous, "cff7a1f01a039f24260cd9e9384924ab10b19878")
    base = pe.OPTIONAL_HEADER.ImageBase
    md = Cs(CS_ARCH_X86, CS_MODE_32)

    def code(addr, size):
        return list(md.disasm(pe.get_data(addr - base, size), addr))

    source = args.source.read_text(encoding="utf-8")
    dispatch = source[source.index("void control::MissionControl"):source.index("void control::HyperspaceKickOutControl")]
    names = {int(i): name for i, name in re.findall(r"case (\d+):\s*([A-Za-z]+Controller)\(", dispatch)}
    functions = []
    for index in range(33):
        stub = struct.unpack("<I", pe.get_data(0x42a5d4 - base + index * 4, 4))[0]
        call = next(i for i in code(stub, 32) if i.mnemonic == "call")
        functions.append((index, names.get(index, "QuoteController"), int(call.op_str, 16)))
    starts = sorted({addr for _, _, addr in functions})
    missions = []
    for state, name, start in functions:
        end = next((addr for addr in starts if addr > start), 0x42a240)
        instructions = code(start, end - start)
        calls = [{"address": hex(ins.address), "previous_instruction": instructions[i - 1].mnemonic + " " + instructions[i - 1].op_str}
                 for i, ins in enumerate(instructions) if ins.mnemonic == "call" and ins.op_str == "0x422ce0"]
        missions.append(dict(state=state, function=name, start=hex(start), end=hex(end), replay_calls=calls))
    sections = []
    for sec in pe.sections:
        name = sec.Name.rstrip(b"\0").decode("ascii")
        old = next((s for s in previous.sections if s.Name == sec.Name), None) if previous else None
        sections.append({"name": name, "rva": hex(sec.VirtualAddress), "raw_size": sec.SizeOfRawData,
                         "sha256": hashlib.sha256(sec.get_data()).hexdigest(),
                         "same_as_previous": old.VirtualAddress == sec.VirtualAddress and old.get_data() == sec.get_data() if old else None})
    print(json.dumps({"schema_version": 1, "method": "static_pe_disassembly", "reference": identity,
                      "previous": previous_identity, "source_sha256": hashlib.sha256(args.source.read_bytes()).hexdigest(),
                      "image_base": hex(base), "sections": sections, "missions": missions,
                      "limitations": ["Names are mapped from source dispatch, not historical symbols.",
                                      "Calls do not establish runtime reachability or transition behavior.",
                                      "No dynamic observation or audio listening is performed."]}, indent=2))


if __name__ == "__main__":
    main()
