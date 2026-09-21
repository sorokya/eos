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

    # All maximal wildcard-free runs become anchors. Using only the longest one
    # is fragile: linker padding at the region's start (or any single byte the
    # object stores differently) makes that one anchor absent from the object
    # even though the code is there, producing a false "no match".
    runs = []
    i = 0
    while i < len(mask):
        if mask[i]:
            j = i
            while j < len(mask) and mask[j]:
                j += 1
            # Also offer sub-runs starting a little further in: the linked
            # region may begin with linker padding (90/cc) or any single byte
            # the object stores differently, and an anchor that includes it
            # would never be found.
            for skip in range(0, 4):
                if j - (i + skip) >= args.min_anchor:
                    runs.append((j - (i + skip), i + skip))
            i = j
        else:
            i += 1
    if not runs:
        raise SystemExit('no wildcard-free anchor long enough to search')
    runs.sort(reverse=True)
    print(f'{len(runs)} anchor candidates, longest {runs[0][0]} bytes at +{runs[0][1]}')
    for alen, aoff in runs[:6]:
        print(f'   +{aoff:#04x} len {alen}: {pat[aoff:aoff+alen].hex()}')

    files = []
    for root in args.roots:
        for ext in ('obj', 'OBJ', 'lib', 'LIB'):
            files += glob.glob(f'{root}/**/*.{ext}', recursive=True)
    print(f'searching {len(files)} files ...')

    # Score every anchor hit by how many unmasked bytes match, and report the
    # best score per file. An exact full-region match is not required: the
    # linked region can include linker padding at its start, so requiring the
    # whole range would reject genuine hits.
    total = sum(mask)
    best = {}
    for alen, aoff in runs:
        anchor = pat[aoff:aoff + alen]
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
                # The linked region may be offset from the object's copy by
                # linker padding, so slide the region start around the nominal
                # position and keep the best-scoring alignment.
                for d in range(-40, 41):
                    p = j - aoff + d
                    if p < 0 or p + len(pat) > len(b):
                        continue
                    ok = sum(1 for k in range(len(pat)) if mask[k] and b[p + k] == pat[k])
                    frac = ok / total if total else 0
                    if frac > best.get(f, (0, 0, 0, 0))[3]:
                        best[f] = (ok, p, d, frac)
    if not best:
        print('  no anchor hit anywhere')
        return
    ranked = sorted(best.items(), key=lambda kv: -kv[1][3])
    for f, (ok, p, d, frac) in ranked[:8]:
        print(f'  {ok:4d}/{total} unmasked bytes match ({frac:5.1%})   {f}  (region at +{p}, slide {d:+d})')


if __name__ == '__main__':
    main()
