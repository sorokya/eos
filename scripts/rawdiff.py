#!/usr/bin/env python3
"""Raw byte differential of one PE section, attributed to reference modules.

Once every module sits at its reference RVA (`make layout`), a plain byte diff
is meaningful: it sees what the masked tools canonicalise away -- a call to a
different callee of the same shape, a branch to the wrong target, a literal
reference into the wrong NUL of the pool. This prints the differing-byte count,
the per-module cluster totals and (with -v) the clusters themselves.

Usage:
    scripts/rawdiff.py [--section .text] [--gap 16] [-v] [REF] [OURS]
"""
import argparse
import bisect
import struct


def sections(b):
    e = struct.unpack_from('<I', b, 0x3c)[0]
    n = struct.unpack_from('<H', b, e + 6)[0]
    opt = struct.unpack_from('<H', b, e + 20)[0]
    off, out = e + 24 + opt, {}
    for _ in range(n):
        name = b[off:off + 8].rstrip(b'\0').decode()
        _vs, va, rs, ro = struct.unpack_from('<IIII', b, off + 8)
        out[name] = (va, rs, ro)
        off += 40
    return out


def modules(path):
    mods = []
    for line in open(path).read().split('\n')[1:]:
        f = line.split('\t')
        if len(f) > 5 and f[0]:
            mods.append((int(f[0], 16), int(f[1], 16), f[4] or f[5]))
    return sorted(mods)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ref', nargs='?', default='GameServer.exe')
    ap.add_argument('ours', nargs='?', default='build/GameServer.exe')
    ap.add_argument('--section', default='.text')
    ap.add_argument('--gap', type=int, default=16, help='merge differences closer than this')
    ap.add_argument('--modules', default='analysis/target/modules.tsv')
    ap.add_argument('-v', action='store_true', help='list every cluster')
    args = ap.parse_args()
    a, c = open(args.ref, 'rb').read(), open(args.ours, 'rb').read()
    va, rs, ro = sections(a)[args.section]
    mods = modules(args.modules)
    starts = [m[0] for m in mods]

    def mod(v):
        i = bisect.bisect_right(starts, v) - 1
        return mods[i][2] if i >= 0 and v < mods[i][1] else '?'

    diffs = [i for i in range(rs) if a[ro + i] != c[ro + i]]
    clusters = []
    for d in diffs:
        if clusters and d - clusters[-1][1] <= args.gap:
            clusters[-1][1] = d
        else:
            clusters.append([d, d])
    print(f'{args.section}: {len(diffs)} differing bytes in {len(clusters)} clusters')
    per = {}
    for s, e in clusters:
        m = mod(0x400000 + va + s)
        per[m] = per.get(m, 0) + e - s + 1
    for m, n in sorted(per.items(), key=lambda x: -x[1]):
        print(f'{n:8d}  {m}')
    if args.v:
        for s, e in clusters:
            print(f'{0x400000 + va + s:#x}-{0x400000 + va + e:#x} {e - s + 1:5d}  {mod(0x400000 + va + s)}')
    return 1 if diffs else 0


if __name__ == '__main__':
    raise SystemExit(main())
