#!/usr/bin/env python3
"""Compare a rebuilt PE against the reference at three levels.

Usage:
    compare_pe.py REFERENCE CANDIDATE [--ignore-timestamp]

Reports, in order: whole-file size/hash, header-field differences, per-section
geometry/hash differences, and the first differing byte offset. Exits non-zero
if the images differ (after applying any requested ignore rules).
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402


def first_difference(a: bytes, b: bytes) -> int | None:
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            return i
    return None if len(a) == len(b) else n


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("reference")
    ap.add_argument("candidate")
    ap.add_argument("--ignore-timestamp", action="store_true",
                    help="do not treat the PE TimeDateStamp as a difference")
    args = ap.parse_args()

    ref = PE.from_file(args.reference)
    cand = PE.from_file(args.candidate)
    different = False

    print(f"reference : {args.reference}  {len(ref.data)} bytes  md5={ref.whole_file_md5()}")
    print(f"candidate : {args.candidate}  {len(cand.data)} bytes  md5={cand.whole_file_md5()}")
    same_md5 = ref.whole_file_md5() == cand.whole_file_md5()
    print(f"whole-file: {'IDENTICAL' if same_md5 else 'DIFFERS'}")
    if not same_md5:
        different = True

    # Header fields --------------------------------------------------------
    ignore = {"time_date_stamp", "time_date_stamp_offset"} if args.ignore_timestamp else set()
    rh, ch = ref.header(), cand.header()
    print("\nheader:")
    for key in rh:
        if key in ignore:
            continue
        if rh[key] != ch[key]:
            different = True
            print(f"  {key:28} ref={rh[key]!r:>12} cand={ch[key]!r:>12}")

    # Sections -------------------------------------------------------------
    rs = {s.name: s for s in ref.sections}
    cs = {s.name: s for s in cand.sections}
    print("\nsections:")
    for name in [*rs, *(n for n in cs if n not in rs)]:
        r, c = rs.get(name), cs.get(name)
        if r is None:
            different = True
            print(f"  {name:8} only in candidate")
            continue
        if c is None:
            different = True
            print(f"  {name:8} only in reference")
            continue
        rmd5, cmd5 = ref.section_md5(r), cand.section_md5(c)
        ok = (r.virtual_address == c.virtual_address and r.raw_size == c.raw_size
              and r.characteristics == c.characteristics and rmd5 == cmd5)
        flag = "ok" if ok else "DIFF"
        print(f"  {name:8} {flag:4} vaddr {r.virtual_address:#010x}/{c.virtual_address:#010x} "
              f"raw {r.raw_size:#08x}/{c.raw_size:#08x} md5 {rmd5[:12]}/{cmd5[:12]}")
        if not ok:
            different = True

    # First differing byte -------------------------------------------------
    if len(ref.data) == len(cand.data):
        off = first_difference(ref.data, cand.data)
        if off is not None:
            print(f"\nfirst differing byte at file offset {off:#x}: "
                  f"ref={ref.data[off]:#04x} cand={cand.data[off]:#04x}")
    else:
        print("\nfile sizes differ")

    print(f"\nresult: {'DIFFERENT' if different else 'IDENTICAL'}")
    return 1 if different else 0


if __name__ == "__main__":
    raise SystemExit(main())
