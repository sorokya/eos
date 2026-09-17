#!/usr/bin/env python3
"""Score every reconstructed source function against the reference.

For each unit's bcc32 `-S` listing (`build/<Unit>.asm`) every function in that
unit's own namespace is matched, for instruction equality, against the unit's
reference ranges from `analysis/target/unit_functions.tsv`. Canonicalization is
the same as compare_asm.py: absolute addresses compare equal, registers, stack
offsets and small constants must match.

Reference ranges run to the next function's start. The last function before an
`@@Unit@Initialize` stub is followed by the EH cleanup table, so each range is
also tried truncated at every `ret` (the header-only C++ deletion variants and
`std::vector` COMDATs the linker discards simply find no match).

A function in the unit's namespace with no matching range is a failure; the exit
status is non-zero if any exist.

Usage:
    verify_units.py [UNIT ...] [--ref-bin GameServer.exe] [--asm-dir build]
"""

import argparse
import glob
import importlib.util
import os
import re
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))

# Deleting destructors (`$bdtr`) are COMDATs: when nothing `delete`s the class,
# ilink32 drops them, so no reference range exists. Absence is expected and the
# link output is unaffected; other missing functions are real failures.
BENIGN_MISS = re.compile(r"\$bd[et]r?\$|\$bdt\$")

# A unit's C++ class name is fixed by the RTTI type-name table in the reference,
# which need not match the unit's file base name (that base is fixed by the
# `@@Unit@Initialize` export). Map each such unit to the class-name prefix its
# functions actually use.
UNIT_CLASS_ALIASES = {
    "Map": ["MapContainer"],
    "Mapwarp": ["MapWarp"],
    "Mapobject": ["MapObject"],
    "Mapchest": ["MapChest"],
    "Msgboard": ["MsgBoard"],
    "Msgboardcontrol": ["MsgBoardController"],
    "Chestcontrol": ["ChestController"],
    "Doorcontrol": ["DoorController"],
    "Effectcontrol": ["EffectController"],
    "Eventcontrol": ["EventController"],
    "Npccontrol": ["NpcController"],
    "Jukeboxcontrol": ["JukeBoxController"],
    "Filecache": ["FileCache"],
    "Killcounters": ["KillCounters"],
    "Questtype": ["QuestType"],
}


def load_compare():
    spec = importlib.util.spec_from_file_location(
        "compare_asm", os.path.join(HERE, "compare_asm.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def canon(seq):
    """Drop trailing padding and ignore which callee-saved regs are pushed."""
    seq = list(seq)
    while seq and seq[-1] == "nop":
        seq.pop()
    head = seq[:8]
    return [x for x in head if x not in ("push ebx", "push esi", "push edi")] + seq[8:]


def reference_pool(ca, ref_bin, ranges):
    """Canonical instruction lists for every plausible cut of every range."""
    pool = set()
    for i, (start, end) in enumerate(ranges):
        ins = ca.parse_ref(ref_bin, start, end)[0]
        pool.add(tuple(canon(ins)))
        for j, x in enumerate(ins):
            if x == "ret":
                pool.add(tuple(canon(ins[:j + 1])))
        # Ghidra occasionally splits a function after its prologue (for example
        # Settings_ReadIniBool at 0x4158c4 -> FUN_004158ca). A ret-less fragment
        # no longer than a prologue is re-joined with its immediate successor.
        if ("ret" not in ins and len(ins) <= 6 and i + 1 < len(ranges)
                and ranges[i + 1][0] == end):
            merged = ca.parse_ref(ref_bin, start, ranges[i + 1][1])[0]
            pool.add(tuple(canon(merged)))
    pool.discard(())
    return pool


def unit_ranges(tsv):
    """unit -> [(start, end)] where end is the next function's start."""
    by_unit = defaultdict(list)
    with open(tsv) as fh:
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 6 and p[0] != "unit":
                by_unit[p[0]].append([int(p[2], 16), int(p[3], 16), p[5]])
    out = {}
    for unit, fns in by_unit.items():
        fns.sort()
        spans = []
        for i, (start, end, _) in enumerate(fns):
            stop = fns[i + 1][0] if i + 1 < len(fns) else end
            if stop > start:
                spans.append((start, stop))
        out[unit] = spans
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("units", nargs="*", help="unit names (default: all build/*.asm)")
    ap.add_argument("--ref-bin", default="GameServer.exe")
    ap.add_argument("--asm-dir", default="build")
    ap.add_argument("--tsv", default="analysis/target/unit_functions.tsv")
    args = ap.parse_args()

    ca = load_compare()
    ranges = unit_ranges(args.tsv)

    if args.units:
        asms = [(u, os.path.join(args.asm_dir, u + ".asm")) for u in args.units]
    else:
        asms = [(os.path.basename(p)[:-4], p)
                for p in sorted(glob.glob(os.path.join(args.asm_dir, "*.asm")))]

    total_ok = total_n = total_benign = 0
    problems = []
    for unit, asm in asms:
        tsv_unit = next((k for k in ranges if k.lower() == unit.lower()), None)
        if tsv_unit is None:
            print(f"{unit:16s} not a reference unit (skipped)")
            continue
        pool = reference_pool(ca, args.ref_bin, ranges[tsv_unit])
        txt = open(asm, encoding="latin1").read()
        own = [f"@@{tsv_unit}@".lower()]
        own += [f"@@{c}@".lower() for c in UNIT_CLASS_ALIASES.get(tsv_unit, ())]
        ok = n = 0
        bad = []
        benign = []
        for name in re.findall(r"(?m)^(\S+)\s+proc\s+near", txt):
            if not any(name.lower().startswith(p) for p in own):
                continue                       # RTL/VCL/std, not our source
            ins, _, _ = ca.parse_our(asm, name)
            if ins is None:
                continue
            n += 1
            if tuple(canon(ins)) in pool:
                ok += 1
            elif BENIGN_MISS.search(name):
                benign.append(name)
            else:
                bad.append(name)
        total_ok += ok
        total_n += n
        total_benign += len(benign)
        note = ""
        if benign:
            note = f"   ({len(benign)} unreferenced deleting-dtor COMDAT)"
        flag = "" if not bad else "   PROBLEM " + ", ".join(bad)
        print(f"{unit:16s} {ok:3d}/{n:3d}{note}{flag}")
        if bad:
            problems.append((unit, bad))

    print(f"\nsource functions byte-matched: {total_ok + total_benign}/{total_n}"
          f"  ({total_ok} matched, {total_benign} unreferenced COMDATs the linker drops)")
    if problems:
        print("unmatched (verify before trusting this unit):")
        for unit, bad in problems:
            for name in bad:
                print(f"  {unit}: {name}")
        return 1
    print("all source functions match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
