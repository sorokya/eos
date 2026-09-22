#!/usr/bin/env python3
"""Our linked image's function inventory, read out of the Borland TDS.

The reference is stripped, so every differential so far ran *reference ->
rebuild*: take a Ghidra range, mask it, look for it in our image.  That
direction cannot see code we emit and the reference does not have, and it
never looks at the bytes between Ghidra's function ranges (which is where the
compiler's constant pools live).  `ilink32 -v` writes a full TD32 debug side
file for our link, so the rebuild *does* have a complete inventory -- name,
virtual address and exact COMDAT length for every linked function, including
the template/RTL COMDATs Ghidra never named.  That is what this reads, and it
is what makes the opposite direction possible (`comdatdiff.py`).

TD32 layout used here (see also `tdsinfo.py`, which reads the module table):

    char[4]  "FB0A"
    int32    dir_offset
    -- at dir_offset --
    int16 dir_header_size, int16 dir_entry_size, int32 count, int32 x2
    { int16 type; int16 module_index; int32 offset; int32 size; }[count]

  subsection 0x130 Names       : int32 count, then count * (uint8 len, chars, NUL)
  subsection 0x120 Module      : int16 overlay/lib/nsegs, char[2] style,
                                 int32 name_id (1-based into Names), ...
  subsection 0x125 AlignSym    : int32 signature, then CodeView records
                                 (int16 length, int16 type, body)

  S_GPROC32 (0x0205) / S_LPROC32 (0x0204) body:
    +0x00 parent  +0x04 end  +0x08 next
    +0x0c length  +0x10 debug_start  +0x14 debug_end
    +0x18 offset (segment-relative)  +0x1c segment  +0x20 type
    +0x2c name_len  +0x2d name
  Delphi (.pas) modules emit the same records with an empty name; keep them --
  the address and length are what the byte differential needs.

Usage:
    scripts/tdsfuncs.py [build/GameServer.tds] [--seg-base 0x401000]
"""
import argparse
import struct
import sys

NAMES, MODULE, ALIGNSYM = 0x130, 0x120, 0x125
S_LPROC32, S_GPROC32 = 0x0204, 0x0205


def parse(path='build/GameServer.tds', seg_base=0x401000):
    """[(va, length, name, module, 'G'|'L')], {module_index: module_name}."""
    data = open(path, 'rb').read()
    if data[0:4].decode('latin1') not in ('FB0A', 'FB09'):
        raise SystemExit(f'{path}: not a TDS file')
    dir_off = struct.unpack_from('<I', data, 4)[0]
    hdr, ent_size, count = struct.unpack_from('<hhI', data, dir_off)[:3]
    entries, off = [], dir_off + hdr
    for _ in range(count):
        entries.append(struct.unpack_from('<hhII', data, off))
        off += ent_size

    names = []
    for kind, _mi, o, _s in entries:
        if kind != NAMES:
            continue
        n = struct.unpack_from('<I', data, o)[0]
        p = o + 4
        for _ in range(n):
            ln = data[p]
            names.append(data[p + 1:p + 1 + ln].decode('latin1'))
            p += ln + 2                      # length byte, text, NUL

    modules = {}
    for kind, mi, o, _s in entries:
        if kind == MODULE:
            nid = struct.unpack_from('<I', data, o + 8)[0]
            modules[mi] = names[nid - 1] if 0 < nid <= len(names) else f'?{nid}'

    funcs = []
    for kind, mi, o, size in entries:
        if kind != ALIGNSYM:
            continue
        p, end = o + 4, o + size
        while p < end - 3:
            rec_len, rec_type = struct.unpack_from('<HH', data, p)
            if rec_len == 0:
                break
            body = data[p + 4:p + 2 + rec_len]
            if rec_type in (S_LPROC32, S_GPROC32) and len(body) >= 32:
                length = struct.unpack_from('<I', body, 12)[0]
                offset = struct.unpack_from('<I', body, 24)[0]
                nl = body[44] if len(body) > 44 else 0
                funcs.append((seg_base + offset, length,
                              body[45:45 + nl].decode('latin1'),
                              modules.get(mi, f'module{mi}'),
                              'G' if rec_type == S_GPROC32 else 'L'))
            p += 2 + rec_len
    return funcs, modules


def unique(funcs):
    """Sorted, de-duplicated by (va, length) -- a COMDAT can be listed twice."""
    seen, out = set(), []
    for va, ln, nm, mod, kind in sorted(funcs):
        if (va, ln) in seen:
            continue
        seen.add((va, ln))
        out.append((va, ln, nm, mod, kind))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('tds', nargs='?', default='build/GameServer.tds')
    ap.add_argument('--seg-base', type=lambda x: int(x, 0), default=0x401000,
                    help='virtual address of code segment 1 (default .text base)')
    args = ap.parse_args()
    funcs, modules = parse(args.tds, args.seg_base)
    rows = unique(funcs)
    print(f'# {len(rows)} functions, {len(modules)} modules from {args.tds}')
    print('va\tlength\tkind\tmodule\tname')
    for va, ln, nm, mod, kind in rows:
        print(f'{va:#010x}\t{ln}\t{kind}\t{mod}\t{nm}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
