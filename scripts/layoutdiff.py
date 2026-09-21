#!/usr/bin/env python3
"""Compare the reference's module layout against a linked build's ilink32 map.

The reference's module boundaries (analysis/target/modules.tsv, from
`scripts/unitmap.py --pe --stubs`) give the target layout: which module lives
at which RVA, in which order, and how the pulled library members interleave
with the units.  `MAP=1 scripts/build.sh` writes the same information for our
build as ilink32's detailed segment map, naming every object and library
member.

This tool lines the two up and reports, per translation unit, the start-RVA
delta and the size delta, any order inversions between units, and the size and
position of the contiguous library blocks (the interleave points).  It is a
diagnostic: byte identity is still the whole-file MD5.

Usage:
    scripts/layoutdiff.py [--ref analysis/target/modules.tsv] \
                          [--map build/GameServer_map.map] [--tol N] [--strict]
"""
import argparse
import re
import sys

IMAGE_BASE = 0x400000
TEXT_RVA = 0x1000  # .text VirtualAddress; map offsets are segment-relative


def load_reference(path):
    """Return the reference module rows in address order."""
    rows = []
    with open(path) as fh:
        header = fh.readline().rstrip("\n").split("\t")
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) < len(header):
                continue
            start, end, init, counter, name, cls = p[:6]
            if not start:
                continue
            rows.append({
                "start": int(start, 16) - IMAGE_BASE,
                "end": int(end, 16) - IMAGE_BASE,
                "rva": int(start, 16) - IMAGE_BASE,
                "size": int(end, 16) - int(start, 16),
                "name": name,
                "cls": cls,
            })
    return rows


def load_map(path):
    """Return the linked build's CODE modules in layout order."""
    rx = re.compile(r"0001:([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})\s+C=CODE\s+.*?M=(\S+)")
    rows = []
    with open(path, errors="replace") as fh:
        for line in fh:
            m = rx.search(line)
            if not m:
                continue
            off, size, mod = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
            base = mod.split("\\")[-1]
            if "|" in base:
                lib, member = base.split("|", 1)
                rows.append({"rva": off + TEXT_RVA, "size": size,
                             "lib": lib.upper(), "member": member.upper(),
                             "unit": None})
            else:
                rows.append({"rva": off + TEXT_RVA, "size": size,
                             "lib": None, "member": None,
                             "unit": base[:-4].upper() if base.upper().endswith(".OBJ")
                                     else base.upper()})
    return rows


def lib_blocks(rows, is_lib):
    """Group consecutive library rows into (start, end, size, members) blocks."""
    blocks = []
    cur = None
    for r in rows:
        if is_lib(r):
            if cur is None:
                cur = [r["rva"], r["rva"] + r["size"], r["size"], 1]
            else:
                cur[1] = r["rva"] + r["size"]
                cur[2] += r["size"]
                cur[3] += 1
        elif cur is not None:
            blocks.append(tuple(cur))
            cur = None
    if cur is not None:
        blocks.append(tuple(cur))
    return blocks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", default="analysis/target/modules.tsv")
    ap.add_argument("--map", dest="map_path", default="build/GameServer_map.map")
    ap.add_argument("--tol", type=int, default=0,
                    help="max |start delta| (bytes) tolerated by --strict")
    ap.add_argument("--strict", action="store_true",
                    help="exit non-zero on any order inversion or delta > --tol")
    args = ap.parse_args()

    try:
        ref = load_reference(args.ref)
    except OSError as e:
        sys.exit(f"layoutdiff: cannot read {args.ref}: {e}")
    try:
        ours = load_map(args.map_path)
    except OSError as e:
        sys.exit(f"layoutdiff: cannot read {args.map_path}: {e} "
                 "(run `MAP=1 scripts/build.sh`)")

    ref_units = [r for r in ref if r["cls"] == "unit"]
    ref_by_name = {r["name"].upper(): r for r in ref_units}
    our_by_unit = {r["unit"]: r for r in ours if r["lib"] is None}

    print(f"layoutdiff: {args.ref} vs {args.map_path}")
    print(f"image base 0x{IMAGE_BASE:x}, .text RVA 0x{TEXT_RVA:x}\n")

    print("unit layout (reference order):")
    print(f"  {'ref_rva':>9} {'our_rva':>9} {'delta':>7} "
          f"{'ref_span':>9} {'our_span':>9} {'d_span':>7}  unit")
    max_delta = 0
    matched = 0
    for r in ref_units:
        key = r["name"].upper()
        o = our_by_unit.get(key)
        span = r["end"] - r["start"]
        if o is None:
            print(f"  0x{r['start']:07x} {'--':>9} {'--':>7} "
                  f"{span:9d} {'--':>9} {'--':>7}  {r['name']} (missing)")
            continue
        matched += 1
        delta = r["start"] - o["rva"]
        max_delta = max(max_delta, abs(delta))
        note = ""
        if span - o["size"] > 4096:
            note = "  (reference span absorbs library code; not comparable)"
        print(f"  0x{r['start']:07x} 0x{o['rva']:07x} {delta:+7d} "
              f"{span:9d} {o['size']:9d} {o['size']-span:+7d}  {r['name']}{note}")

    # Order inversions among the units we actually placed.
    rank_ref = {r["name"].upper(): i for i, r in enumerate(ref_units)}
    placed = [r for r in ours if r["lib"] is None and r["unit"] in rank_ref]
    inv = []
    for i in range(len(placed)):
        for j in range(i + 1, len(placed)):
            a, b = placed[i]["unit"], placed[j]["unit"]
            if rank_ref[a] > rank_ref[b]:
                inv.append((a, b))
    print("\norder inversions (ours vs reference):")
    if not inv:
        print("  none")
    for a, b in inv:
        print(f"  {a} placed before {b} in our build "
              f"(reference: {b} 0x{ref_by_name[b]['start']:07x} before "
              f"{a} 0x{ref_by_name[a]['start']:07x})")

    rb = lib_blocks(ref, lambda r: r["cls"] in ("library", "unknown"))
    ob = lib_blocks(ours, lambda r: r["lib"] is not None)
    print(f"\nlibrary blocks (reference {len(rb)}, ours {len(ob)}):")
    for tag, blocks in (("ref", rb), ("our", ob)):
        for k, (s, e, size, n) in enumerate(blocks):
            print(f"  {tag} #{k}  0x{s:07x}-0x{e:07x}  {size:8d} B  {n} modules")

    print(f"\nsummary: {matched}/{len(ref_units)} units matched, "
          f"{len(inv)} inversion(s), max |start delta| = {max_delta} B")

    if args.strict and (inv or max_delta > args.tol):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
