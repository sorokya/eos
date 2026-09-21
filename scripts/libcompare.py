#!/usr/bin/env python3
"""Compare the linked build's modules against the reference image, one by one.

For every CODE module in our ilink32 map, take its bytes from our .text and
locate them in the reference .text, wildcarding the operands that legitimately
differ between two linked images:

  * the 4-byte slots carrying a base relocation (taken from our own relocation
    table, so data members keep their string/constant bytes comparable), and
  * the rel32 operand of a direct call/jmp/jcc.

A module with no home in the reference is one we pull and the reference does
not -- i.e. it accounts for the .text surplus; a module found at an unexpected
RVA points at a composition/order difference.

Usage:
    MAP=1 scripts/build.sh
    scripts/libcompare.py --ref GameServer.exe --linked build/GameServer.exe \
        --map build/GameServer_map.map [--libs-only] [--threshold 0.98]
"""
import argparse
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402
from find_region import mask_for  # noqa: E402


def image_of(path):
    pe = PE.from_file(path)
    for s in pe.sections:
        if s.name == '.text':
            base = pe.header().get('image_base', 0x400000)
            relocs = {va - base for va, _ in pe.relocations()}
            return pe.section_data(s), s.virtual_address, relocs
    raise SystemExit(f'no .text in {path}')


def load_map(path):
    rx = re.compile(r'0001:([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})\s+C=CODE\s+.*?M=(\S+)')
    mods = []
    with open(path, errors='replace') as fh:
        for line in fh:
            m = rx.search(line)
            if not m:
                continue
            base = m.group(3).split('\\')[-1]
            lib, member = (base.split('|', 1) if '|' in base else (None, base))
            mods.append({'off': int(m.group(1), 16), 'size': int(m.group(2), 16),
                         'lib': lib.upper() if lib else None,
                         'name': member.upper()})
    return mods


def anchors(mask, min_len):
    runs = []
    i = 0
    while i < len(mask):
        if mask[i]:
            j = i
            while j < len(mask) and mask[j]:
                j += 1
            for skip in range(0, 4):
                if j - (i + skip) >= min_len:
                    runs.append((j - (i + skip), i + skip))
            i = j
        else:
            i += 1
    runs.sort(reverse=True)
    return runs


def locate(pat, mask, hay, min_len, max_anchors=12, threshold=0.98):
    """Locate pat in hay; return (best_ratio, start, total, longest_hit_anchor).

    `longest_hit_anchor` is the length of the longest wildcard-free run of pat
    that actually occurs in hay, which distinguishes "we pull a module the
    reference does not have" (no run occurs) from "the module is there but later
    diverges" (long run occurs, ratio may be low).
    """
    total = sum(mask)
    if total == 0:
        return None
    runs = anchors(mask, min_len)
    if not runs:
        return None
    best = None
    longest_hit = 0
    seen = set()
    for alen, aoff in runs[:max_anchors]:
        anchor = pat[aoff:aoff + alen]
        st = 0
        hit = False
        while True:
            j = hay.find(anchor, st)
            if j < 0:
                break
            st = j + 1
            hit = True
            start = j - aoff
            if start < 0 or start + len(pat) > len(hay) or start in seen:
                continue
            seen.add(start)
            good = sum(1 for k in range(len(pat)) if mask[k] and pat[k] == hay[start + k])
            ratio = good / total
            if best is None or ratio > best[0]:
                best = (ratio, start, total, alen)
                if ratio == 1.0:
                    return best
        if hit and alen > longest_hit:
            longest_hit = alen
    if best is None:
        return (0.0, -1, total, longest_hit) if longest_hit else None
    return best


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--ref', default='GameServer.exe')
    ap.add_argument('--linked', default='build/GameServer.exe')
    ap.add_argument('--map', dest='map_path', default='build/GameServer_map.map')
    ap.add_argument('--libs-only', action='store_true')
    ap.add_argument('--objs-only', action='store_true')
    ap.add_argument('--only-unmatched', action='store_true')
    ap.add_argument('--sizes', action='store_true',
                    help='report per-module size vs the reference member between '
                         'its matched start and the next matched start')
    ap.add_argument('--threshold', type=float, default=0.98)
    args = ap.parse_args()

    ours, our_rva, our_rel = image_of(args.linked)
    ref, ref_rva, ref_rel = image_of(args.ref)
    print(f"our .text {len(ours)} B, ref .text {len(ref)} B, "
          f"our relocs in .text {sum(1 for r in our_rel if our_rva <= r < our_rva+len(ours))}")

    mods = load_map(args.map_path)
    if args.libs_only:
        mods = [m for m in mods if m['lib']]
    if args.objs_only:
        mods = [m for m in mods if not m['lib']]
    print(f"comparing {len(mods)} modules\n")

    n_found = n_partial = n_unver = n_absent = 0
    unmatched = []
    absent = []
    located = []
    for m in mods:
        off, size = m['off'], m['size']
        if off < 0 or off + size > len(ours):
            continue
        pat = ours[off:off + size]
        mask = mask_for(pat)
        total = sum(mask)
        tag = f"{m['lib']}|{m['name']}" if m['lib'] else m['name']
        min_len = 8 if size < 256 else 16
        res = locate(pat, mask, ref, min_len, threshold=args.threshold)
        if res is None:
            n_unver += 1
            if not args.only_unmatched and not args.sizes:
                print(f"  UNVER   {tag:44s} sz={size:6d} (all bytes wildcarded)")
            continue
        ratio, start, _, longest = res
        if start >= 0 and longest >= min_len:
            located.append((start, off, size, tag, ratio, longest))
        present = longest > 0
        if not present:
            n_absent += 1
            absent.append((size, tag, longest))
            if not args.sizes:
                print(f"  ABSENT  {tag:44s} sz={size:6d} longest-run={longest}")
        elif ratio >= args.threshold:
            n_found += 1
            if not args.only_unmatched and not args.sizes:
                print(f"  ok      {tag:44s} sz={size:6d} "
                      f"ref=0x{ref_rva + start + 0x400000:06x} ({ratio*100:.1f}%)")
        else:
            n_partial += 1
            unmatched.append((size, tag, ratio))
            if not args.sizes:
                print(f"  PARTIAL {tag:44s} sz={size:6d} "
                      f"ref=0x{ref_rva + start + 0x400000:06x} ({ratio*100:.1f}%, "
                      f"run={longest})")

    if args.sizes:
        located = [e for e in located if e[4] >= 0.99 and e[5] >= 16]
        located.sort()
        print("size deltas (our size - reference gap to the next matched module), "
              "high-confidence matches only:\n")
        size_rows = []
        for i, (start, off, size, tag, ratio, run) in enumerate(located[:-1]):
            nxt = located[i + 1][0]
            ref_size = nxt - start
            if ref_size <= 0 or ref_size > 4 * size + 4096:
                continue
            if abs(size - ref_size) > 2:
                size_rows.append((size - ref_size, size, ref_size, tag, ratio))
        size_rows.sort()
        for delta, size, ref_size, tag, ratio in size_rows:
            print(f"  {delta:+6d}  ours={size:6d} ref={ref_size:6d}  "
                  f"({ratio*100:3.0f}%)  {tag}")
        print(f"\nmodules with a size delta: {len(size_rows)}; "
              f"sum(ours-ref) = {sum(r[0] for r in size_rows)} B "
              f"(the reference has ~1 byte of padding per member)")
        return 0

    print(f"\nfound={n_found} partial={n_partial} absent={n_absent} "
          f"unverifiable={n_unver}")
    if absent:
        absent.sort(reverse=True)
        print(f"\nabsent modules (no long run in the reference): "
              f"{len(absent)}, {sum(a[0] for a in absent)} B")
        for size, tag, run in absent[:40]:
            print(f"  {size:7d}  {tag}  (run={run})")
    if unmatched:
        unmatched.sort(reverse=True)
        print(f"\npartial bytes = {sum(u[0] for u in unmatched)}; "
              f"largest partial matches:")
        for size, tag, ratio in unmatched[:20]:
            print(f"  {size:7d}  {tag}  ({ratio*100:.1f}%)")
    return 0


if __name__ == '__main__':
    sys.exit(main())
