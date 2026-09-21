#!/usr/bin/env python3
"""Classify one object's functions as present in the reference image or not.

Parses a `tdump` listing of a .obj (function segment names and their bytes),
masks relocation placeholders and rel32/absolute operands, and locates each
function in the reference .text. A function with no home in the reference is one
the unit emits and the reference does not -- the per-function form of the .text
surplus.

Usage:
    scripts/objfuncs.py --tdump build/linktest/Npcvalue.tdump --ref GameServer.exe
"""
import argparse
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from find_region import mask_for  # noqa: E402
from libcompare import locate  # noqa: E402


def text_section(path):
    from pe import PE
    pe = PE.from_file(path)
    for s in pe.sections:
        if s.name == '.text':
            return pe.section_data(s), s.virtual_address
    raise SystemExit('no .text')


NAME_RE = re.compile(r"Name:\s*\d+:\s*'(.*?)'\s+virtual\(_TEXT\)\s+Length:\s*([0-9a-f]+)")
LEDATA_RE = re.compile(r"LEDATA\s+Segment:\s*'(.*?)'\s*Offset:\s*([0-9a-f]+)\s+Length:\s*([0-9a-f]+)")
HEXL_RE = re.compile(r'^\s+[0-9A-F]{4}:\s+(.*)$')


def _hexbytes(line):
    """Parse one tdump hex line. The 16-byte group is fixed width (48 chars)
    before the ASCII column, so slicing avoids reading ASCII as hex."""
    m = HEXL_RE.match(line)
    if not m:
        return None
    out = bytearray()
    for tok in m.group(1)[:48].split():
        if len(tok) == 2:
            try:
                out.append(int(tok, 16))
            except ValueError:
                break
    return bytes(out)


def parse(tdump_path):
    """Return [(name, bytes)] for every _TEXT segment with an LEDATA record."""
    text_names = set()
    for line in open(tdump_path, errors='replace'):
        m = NAME_RE.search(line)
        if m:
            text_names.add(m.group(1))
    funcs = []
    cur = None
    for line in open(tdump_path, errors='replace'):
        m = LEDATA_RE.search(line)
        if m:
            name = m.group(1)
            cur = [name, bytearray()] if name in text_names else None
            if cur is not None:
                funcs.append(cur)
            continue
        if cur is not None:
            hb = _hexbytes(line)
            if hb is None:
                cur = None
            else:
                cur[1] += hb
    return [(n, bytes(b)) for n, b in funcs]


def mask_bytes(buf):
    """mask_for, plus the zeroed fixup placeholders an .obj carries."""
    m = mask_for(buf)
    i = 0
    while i + 4 <= len(buf):
        if buf[i:i + 4] == b'\x00\x00\x00\x00':
            m[i:i + 4] = b'\x00\x00\x00\x00'
        i += 1
    return m


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--tdump', required=True)
    ap.add_argument('--ref', default='GameServer.exe')
    args = ap.parse_args()

    ref, ref_rva = text_section(args.ref)
    funcs = parse(args.tdump)
    print(f"{len(funcs)} _TEXT functions in {args.tdump}\n")

    absent = []
    found = 0
    for name, buf in funcs:
        if not buf:
            continue
        mask = mask_bytes(buf)
        # Small functions are call-dense, so their unmasked runs are short;
        # scale the anchor minimum to the function size.
        min_len = 4 if len(buf) < 128 else (6 if len(buf) < 512 else 10)
        res = locate(buf, mask, ref, min_len)
        if res is None or res[3] == 0:
            absent.append((len(buf), name))
            print(f"  ABSENT  sz={len(buf):5d}  {name}")
        else:
            found += 1
            ratio, start, _, run = res
            if ratio < 0.98:
                print(f"  partial sz={len(buf):5d}  ref=0x{ref_rva+start+0x400000:06x} "
                      f"({ratio*100:.0f}%, run={run})  {name}")

    print(f"\nfound={found} absent={len(absent)}")
    if absent:
        absent.sort(reverse=True)
        print(f"absent bytes = {sum(a[0] for a in absent)}")
        for size, name in absent:
            print(f"  {size:6d}  {name}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
