#!/usr/bin/env python3
"""Classify reference functions as library by matching the linked build.

The Borland libraries are built by the same toolchain with the same flags the
reference used, so their code is byte-identical modulo address fixups. Linking
the project against the same libraries therefore yields a library block that can
be searched directly: a reference function whose bytes appear inside a library
module of the linked image is a library member, not application code.

This is far more precise than sliding the pattern over the raw `.lib` files --
the linked block is ~200 KB of exactly the members the program pulls in, rather
than a 50 MB concatenation of every member -- and the linker map names each
module, so a hit also yields the member name.

Usage:
    MAP=1 scripts/build.sh
    scripts/libmatch.py [--tsv analysis/target/functions.tsv]
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import unitmap  # noqa: E402
from pe import PE  # noqa: E402

MIN_LITERAL = 16      # literal (unmasked) bytes required for a match to count
MAX_SIG = 64          # bytes of the reference function used as the needle


def masked_pattern(va, sig, relocs, minimum=MIN_LITERAL):
    """Regex for `sig` with address-dependent bytes wildcarded.

    Masked: base-relocation slots, and the 4-byte operand of `call`/`jmp rel32`
    (those carry no relocation yet still move with the layout). Returns None
    when too little literal content remains to be meaningful.
    """
    parts, i, literal = [], 0, 0
    while i < len(sig):
        if va + i in relocs:
            parts.append(b".{4}")
            i += 4
        elif sig[i] in (0xE8, 0xE9) and i + 4 < len(sig):
            parts.append(re.escape(sig[i:i + 1]) + b".{4}")
            i += 5
        else:
            parts.append(re.escape(sig[i:i + 1]))
            literal += 1
            i += 1
    if literal < minimum:
        return None
    return re.compile(b"".join(parts), re.DOTALL)


def library_block(pe, map_path):
    """(blob, spans) where spans is [(blob_off, blob_end, module)]."""
    blob = bytearray()
    spans = []
    for va, length, _sec, cls, module in unitmap.parse_map(map_path):
        if cls != "CODE" or not unitmap.module_kind(module) == "library":
            continue
        off = unitmap._va_to_off(pe, va)
        if off is None:
            continue
        spans.append((len(blob), len(blob) + length, module.split("|")[-1]))
        blob += pe.data[off:off + length]
    return bytes(blob), spans


def module_at(spans, off):
    lo, hi = 0, len(spans) - 1
    while lo <= hi:
        mid = (lo + hi) // 2
        s, e, name = spans[mid]
        if off < s:
            hi = mid - 1
        elif off >= e:
            lo = mid + 1
        else:
            return name
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ref", default="GameServer.exe")
    ap.add_argument("--linked", default="build/GameServer.exe")
    ap.add_argument("--map", default="build/GameServer.map")
    ap.add_argument("--inventory", default="analysis/target/unit_functions.tsv")
    ap.add_argument("--list", default="")
    args = ap.parse_args()

    if not (os.path.exists(args.map) and os.path.exists(args.linked)):
        raise SystemExit("linked build/map missing: run `MAP=1 scripts/build.sh`")

    ref = PE.from_file(args.ref)
    ours = PE.from_file(args.linked)
    blob, spans = library_block(ours, args.map)
    relocs = {r + ref.header()["image_base"] for r, _t in ref.relocations()}

    rows = []
    with open(args.inventory, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 6 and p[0] != "unit":
                rows.append((p[0], int(p[2], 16), int(p[3], 16)))

    import collections
    per_unit = collections.Counter()
    names = {}
    matched = 0
    for unit, start, end in rows:
        off = unitmap._va_to_off(ref, start)
        if off is None:
            continue
        sig = ref.data[off:off + min(end - start, MAX_SIG)]
        pat = masked_pattern(start, sig, relocs)
        if pat is None:
            continue
        m = pat.search(blob)
        if m:
            matched += 1
            per_unit[unit] += 1
            names.setdefault(unit, []).append(
                (f"{start:#010x}", module_at(spans, m.start())))

    print(f"library block      : {len(blob)} bytes, {len(spans)} modules")
    print(f"matched as library : {matched}/{len(rows)} reference functions")
    print("\nper unit:")
    for u, n in per_unit.most_common(20):
        print(f"  {u:16} {n}")
    if args.list:
        for u in args.list.split(","):
            print(f"\n{u}:")
            for a, mod in sorted(names.get(u, [])):
                print(f"  {a} {mod}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
