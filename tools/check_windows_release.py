"""Audit a Windows x64 release without third-party Python packages."""
import argparse
import hashlib
import json
import pathlib
import struct


def inspect(path, allow_audit=False):
    data = path.read_bytes()
    def u16(offset): return struct.unpack_from('<H', data, offset)[0]
    def u32(offset): return struct.unpack_from('<I', data, offset)[0]
    pe = u32(0x3c)
    assert data[:2] == b'MZ' and data[pe:pe+4] == b'PE\0\0', 'Not a PE executable'
    assert u16(pe+4) == 0x8664, 'Expected Windows x64'
    optional = pe+24
    assert u16(optional) == 0x20b, 'Expected PE32+'
    sections = optional+u16(pe+20)
    def offset(rva):
        for index in range(u16(pe+6)):
            section = sections+40*index
            virtual_size, address, raw_size, raw = struct.unpack_from('<IIII', data, section+8)
            if address <= rva < address+max(virtual_size, raw_size):
                return raw+rva-address
        raise ValueError('Invalid RVA')
    imports = []
    import_rva = u32(optional+112+8)
    if import_rva:
        entry = offset(import_rva)
        while any(data[entry:entry+20]):
            start = offset(u32(entry+12))
            imports.append(data[start:data.index(b'\0', start)].decode('ascii').lower())
            entry += 20
    assert not u32(optional+112+13*8), 'Unexpected delay imports: inspect before release'
    allowed = {'advapi32.dll', 'bcrypt.dll', 'comctl32.dll', 'comdlg32.dll',
               'dinput8.dll', 'gdi32.dll', 'imm32.dll', 'kernel32.dll', 'msvcrt.dll', 'ntdll.dll',
               'ole32.dll', 'oleaut32.dll', 'setupapi.dll', 'shell32.dll',
               'shlwapi.dll', 'user32.dll', 'userenv.dll', 'version.dll',
               'winmm.dll', 'ws2_32.dll', 'ucrtbase.dll'}
    # API-set contracts are shipped with the minimum supported OS, Windows 10.
    allowed.update('api-ms-win-crt-'+name+'-l1-1-0.dll' for name in
                   ['convert', 'environment', 'filesystem', 'heap', 'locale', 'math',
                    'multibyte', 'private', 'runtime', 'stdio', 'string', 'time', 'utility'])
    unknown = set(imports)-allowed
    assert not unknown, f'Non-system imports: {sorted(unknown)}'
    for notice in [b'SDL 2.30.11', b'SDL_mixer 2.8.0', b'Dear ImGui', b'MIT License']:
        assert notice in data, f'Missing embedded notice: {notice!r}'
    instrumented = b'PINBALL_AUDIT_INI' in data or b'PINBALL_CADENCE_CSV' in data
    assert allow_audit or not instrumented, 'Audit instrumentation in release'
    assert u16(optional+68) == 2, 'Expected Windows GUI subsystem'
    return {'file': path.name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
            'architecture': 'x64', 'imports': sorted(imports), 'embedded_notices': True, 'audit_instrumentation': instrumented,
            'scope': 'Static PE inspection; not a clean-machine interactive test.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('executable', type=pathlib.Path)
    parser.add_argument('--json', type=pathlib.Path)
    parser.add_argument('--allow-audit', action='store_true', help='Inspect a test build; never use for distributed artifacts')
    args = parser.parse_args()
    result = inspect(args.executable, args.allow_audit)
    output = json.dumps(result, indent=2)+'\n'
    if args.json:
        args.json.write_text(output, encoding='utf-8')
    print(output)
