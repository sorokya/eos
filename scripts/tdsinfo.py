#!/usr/bin/env python3
"""Read the module map out of a Borland TDS (TD32) debug file.

ilink32's detailed segment map is the only other source of the per-module
layout, and under Wine it is truncated by a fault before its final modules are
written. The .tds side file is written in full, so this reconstructs the same
information from it: every linked module (application unit or library member)
with its code segments' base offsets and lengths.

Format (Borland TD32, CodeView-4 derivative):
    char[4]  signature            "FB0A" (C++Builder) or "FB09" (Delphi)
    int32    dir_offset           file offset of the subsection directory
    -- at dir_offset --
    int16    dir_header_size (16)
    int16    dir_entry_size (12)
    int32    subsection_count
    int32    reserved (0)
    int32    reserved (0)
    { int16 type; int16 module_index; int32 offset; int32 size; }[count]
Subsection types: Module 0x120, SrcModule 0x127, GlobalSym 0x129,
GlobalTypes 0x12b, Names 0x130.

Module subsection (0x120):
    int16 overlay, int16 library_index, int16 num_code_segs, char[2] style,
    int32 name_id, int32 unk1..unk4,
    { int16 seg_index; int16 flags; int32 offset; int32 length; }[num_code_segs]

Usage:
    scripts/tdsinfo.py [build/GameServer.tds] [--exe GameServer.exe]
"""
import argparse
import struct
import sys

NAME, MODULE = 0x130, 0x120


def parse(path):
    d = open(path, 'rb').read()
    sig = d[0:4].decode('latin1')
    if sig not in ('FB0A', 'FB09'):
        sys.exit(f'not a TDS file (signature {sig!r})')
    dir_off = struct.unpack_from('<I', d, 4)[0]
    dh, de, n, _r1, _r2 = struct.unpack_from('<hhIII', d, dir_off)
    entries = []
    off = dir_off + dh
    for _ in range(n):
        t, mi, o, s = struct.unpack_from('<hhII', d, off)
        entries.append((t, mi, o, s))
        off += de

    names = None
    for t, mi, o, s in entries:
        if t == NAME:
            cnt = struct.unpack_from('<I', d, o)[0]
            p = o + 4
            names = []
            for _ in range(cnt):
                ln = d[p]
                p += 1
                names.append(d[p:p + ln].decode('latin1'))
                p += ln + 1  # NUL terminator
    if names is None:
        names = []

    def name_of(nid):
        return names[nid - 1] if nid else None

    modules = {}
    for t, mi, o, s in entries:
        if t != MODULE or mi in (0, -1):
            continue
        p = o
        overlay, libidx, nseg = struct.unpack_from('<hhh', d, p)
        p += 6
        style = d[p:p + 2].decode('latin1')
        p += 2
        name_id, u1, u2, u3, u4 = struct.unpack_from('<iiiii', d, p)
        p += 20
        segs = []
        for _ in range(nseg):
            si, flags, soff, slen = struct.unpack_from('<hhII', d, p)
            p += 12
            segs.append((si, flags, soff, slen))
        modules.setdefault(mi, {'name': name_of(name_id), 'style': style,
                                'segs': []})['segs'].extend(segs)

    out = []
    for mi in sorted(modules):
        m = modules[mi]
        for si, flags, soff, slen in m['segs']:
            out.append((soff, slen, si, flags, m['name'], m['style'], mi))
    out.sort()
    return sig, out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('tds', nargs='?', default='build/GameServer.tds')
    ap.add_argument('--seg', type=int, default=None, help='only this segment index')
    ap.add_argument('--image-base', type=lambda x: int(x, 0), default=0x400000)
    args = ap.parse_args()

    sig, out = parse(args.tds)
    rows = out
    if args.seg is not None:
        rows = [r for r in rows if r[2] == args.seg]
    print(f"{args.tds}: {sig}, {len(rows)} segments "
          f"({'seg ' + str(args.seg) if args.seg is not None else 'all'})\n")
    base = args.image_base + 0x1000
    print(f"  {'base':>10} {'length':>8} {'end':>10} {'VA':>10}  module")
    total = 0
    for soff, slen, si, flags, name, style, mi in rows:
        print(f"  0x{soff:08x} {slen:8d} 0x{soff + slen:08x} "
              f"0x{base + soff:08x}  {name}  seg={si} mi={mi}")
        total += slen
    print(f"\nsum of lengths: {total} (0x{total:x})")
    return 0


if __name__ == '__main__':
    sys.exit(main())
