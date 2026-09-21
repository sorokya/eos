#!/usr/bin/env python3
"""Find which object/library member a linked-image address range came from.

The linked image has its relocations applied, so a range copied from it cannot
be compared byte-for-byte against an object file. This masks the relocation
sensitive dwords (any little-endian value that looks like an address in the
image), picks a wildcard-free anchor from what remains, locates candidates in
every `.obj`/`.lib` in the toolchain tree and the build tree, and reports the
members whose masked bytes match.

Usage:
    scripts/find_region.py --start 0x401150 --end 0x4012c8 \
        [--exe GameServer.exe] [--roots ref/Borland5 build]

Diagnostic tool; a match names the provider, it does not prove byte identity.
"""
import argparse
import glob
import os
import struct


def text_section(path):
    d = open(path, 'rb').read()
    e = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, e + 6)[0]
    opt = struct.unpack_from('<H', d, e + 20)[0]
    ib = struct.unpack_from('<I', d, e + 24 + 28)[0]
    off = e + 24 + opt
    for i in range(nsec):
        nm = d[off + i * 40:off + i * 40 + 8].rstrip(b'\0').decode('latin1')
        vs, va, rs, ro = struct.unpack_from('<IIII', d, off + i * 40 + 8)
        if nm == '.text':
            return d, ib + va, ro
    raise SystemExit('no .text')


def mask_for(buf):
    """1 = compare this byte, 0 = wildcard (an applied relocation).

    In a linked image both kinds of relocation have been applied and must be
    wildcarded before comparing against an object:

      * absolute dwords (any little-endian value that looks like an image
        address), and
      * the rel32 operand of a direct ``call``/``jmp``/``jcc`` (``e8``/``e9``/
        ``0f 8x``), whose displacement is small and looks nothing like an
        address.

    Missing the second class made an earlier run report "no match" for a
    region full of calls; it is the more important of the two.
    """
    m = bytearray(b'\x01' * len(buf))
    i = 0
    while i < len(buf):
        op = buf[i]
        if op in (0xE8, 0xE9) and i + 5 <= len(buf):
            for k in range(1, 5):
                m[i + k] = 0
            i += 5
            continue
        if op == 0x0F and i + 6 <= len(buf) and 0x80 <= buf[i + 1] <= 0x8F:
            for k in range(2, 6):
                m[i + k] = 0
            i += 6
            continue
        i += 1
    # absolute image addresses (any alignment)
    i = 0
    while i + 4 <= len(buf):
        v = struct.unpack_from('<I', buf, i)[0]
        if 0x400000 <= v < 0x600000:
            for k in range(4):
                m[i + k] = 0
            i += 4
        else:
            i += 1
    return m


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default='GameServer.exe')
    ap.add_argument('--start', required=True)
    ap.add_argument('--end', required=True)
    ap.add_argument('--roots', nargs='*', default=['ref/Borland5', 'build'])
    ap.add_argument('--min-anchor', type=int, default=6)
    args = ap.parse_args()

    d, tva, tro = text_section(args.exe)
    lo, hi = int(args.start, 16), int(args.end, 16)
    o = tro + (lo - tva)
    pat = d[o:o + (hi - lo)]
    mask = mask_for(pat)
    print(f'region {args.start}-{args.end}: {len(pat)} bytes; '
          f'{mask.count(0)} bytes wildcarded as addresses')

    # longest wildcard-free run -> anchor
    best = (0, 0)
    i = 0
    while i < len(mask):
        if mask[i]:
            j = i
            while j < len(mask) and mask[j]:
                j += 1
            if j - i > best[0]:
                best = (j - i, i)
            i = j
        else:
            i += 1
    alen, aoff = best
    print(f'longest wildcard-free run: {alen} bytes at +{aoff}')
    if alen < args.min_anchor:
        raise SystemExit('anchor too short to search reliably')
    anchor = pat[aoff:aoff + alen]

    files = []
    for root in args.roots:
        for ext in ('obj', 'OBJ', 'lib', 'LIB'):
            files += glob.glob(f'{root}/**/*.{ext}', recursive=True)
    print(f'searching {len(files)} files ...')
    found = []
    for f in files:
        try:
            b = open(f, 'rb').read()
        except Exception:
            continue
        st = 0
        while True:
            j = b.find(anchor, st)
            if j < 0:
                break
            st = j + 1
            p = j - aoff
            if p < 0 or p + len(pat) > len(b):
                continue
            ok = True
            for k in range(len(pat)):
                if mask[k] and b[p + k] != pat[k]:
                    ok = False
                    break
            if ok:
                found.append((f, p))
    for f, p in found:
        print(f'  MATCH  {f}  at +{p}')
    if not found:
        print('  no match')


if __name__ == '__main__':
    main()
