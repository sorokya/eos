#!/usr/bin/env python3
"""Per-function byte comparison between the reference and a rebuilt image.

Function ranges come from a Ghidra inventory TSV (address, size, name, ...).
Because an early rebuild will not yet have the same layout as the reference,
`--mask-reloc` ignores the 4-byte words that carry base relocations in either
image, giving a layout-independent code-fidelity score. The default is a strict
byte comparison.

Exit code is non-zero when any function differs (strict mode) or any function
has unmasked differences (masked mode).
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402


def load_functions(path: str) -> list[tuple[int, int, str]]:
    out = []
    with open(path, encoding="utf-8") as fh:
        header = next(fh)
        for line in fh:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 3:
                continue
            out.append((int(parts[0], 16), int(parts[1]), parts[2]))
    return out


def read_rva(pe: PE, rva: int, size: int) -> bytes | None:
    try:
        off = pe._rva_to_off(rva)
    except ValueError:
        return None
    if off + size > len(pe.data):
        return None
    return pe.data[off:off + size]


def masked_offsets(reloc_rvas: set[int], va: int, size: int) -> set[int]:
    mask = set()
    for a in reloc_rvas:
        if va <= a < va + size:
            base = a - va
            for i in range(4):
                mask.add(base + i)
    return mask


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("functions", help="Ghidra inventory TSV")
    ap.add_argument("reference")
    ap.add_argument("candidate")
    ap.add_argument("--mask-reloc", action="store_true",
                    help="ignore bytes carrying base relocations (layout independent)")
    ap.add_argument("--min-size", type=int, default=1)
    ap.add_argument("--list", type=int, default=0, help="list up to N differing functions")
    ap.add_argument("--name-filter", default="")
    args = ap.parse_args()

    ref = PE.from_file(args.reference)
    cand = PE.from_file(args.candidate)
    base = ref.header()["image_base"]

    ref_reloc = {r for r, _ in ref.relocations()}
    cand_reloc = {r for r, _ in cand.relocations()}

    funcs = load_functions(args.functions)
    identical = differing = missing = skipped = not_in_ref = 0
    compared_bytes = identical_bytes = 0
    diffs = []

    for va, size, name in funcs:
        if size < args.min_size:
            skipped += 1
            continue
        if args.name_filter and args.name_filter not in name:
            continue
        rva = va - base
        rb = read_rva(ref, rva, size)
        if rb is None:
            # Not part of the reference image (e.g. synthetic analysis blocks).
            not_in_ref += 1
            continue
        cb = read_rva(cand, rva, size)
        if cb is None:
            missing += 1
            diffs.append((va, size, name, -1))
            continue
        mask = set()
        if args.mask_reloc:
            mask |= masked_offsets(ref_reloc, va, size)
            mask |= masked_offsets(cand_reloc, va, size)
        first = -1
        same = True
        for i in range(size):
            if i in mask:
                continue
            compared_bytes += 1
            if rb[i] != cb[i]:
                same = False
                if first < 0:
                    first = i
            else:
                identical_bytes += 1
        if same:
            identical += 1
        else:
            differing += 1
            diffs.append((va, size, name, first))

    total = identical + differing + missing
    pct = (100.0 * identical / total) if total else 0.0
    byte_pct = (100.0 * identical_bytes / compared_bytes) if compared_bytes else 0.0
    mode = "masked(reloc)" if args.mask_reloc else "strict"
    print(f"mode        : {mode}")
    print(f"functions   : {total} ({skipped} skipped by --min-size, {not_in_ref} not in reference)")
    print(f"identical   : {identical} ({pct:.1f}%)")
    print(f"differing   : {differing}")
    print(f"missing     : {missing}")
    print(f"bytes       : {identical_bytes}/{compared_bytes} identical ({byte_pct:.1f}%)")

    diffs.sort(key=lambda d: (d[3] < 0, d[3]))
    show = args.list if args.list else 0
    for va, size, name, first in diffs[:show] if show else []:
        if first < 0:
            print(f"  {va:#010x} {size:6d} {name:40s} MISSING")
        else:
            print(f"  {va:#010x} {size:6d} {name:40s} first-diff+{first:#x}")
    if show and len(diffs) > show:
        print(f"  ... {len(diffs) - show} more")

    return 0 if differing == 0 and missing == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
