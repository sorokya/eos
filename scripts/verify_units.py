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
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))

# Deleting destructors (`$bdtr`) are COMDATs: when nothing `delete`s the class,
# ilink32 drops them, so no reference range exists. Absence is expected and the
# link output is unaffected; other missing functions are real failures.
#
# `NpcValues::ClearDrops` is the same case for a different reason: the reference
# has no range for it either, because nothing calls it there.  It is only in the
# reconstruction because its translation unit has to *define* the
# vector<NpcDropItem> clear/erase/copy COMDATs -- the reference places them in
# Npcvalues (0x4a87a0/0x4a87c4/0x4a8820) while their only caller lives in
# Npcvalue, so Npcvalues.obj must define and win them.  See PLAN.md.
BENIGN_MISS = re.compile(r"\$bd[et]r?\$|\$bdt\$|@NpcValues@ClearDrops\$")

# A unit's C++ class name is fixed by the RTTI type-name table in the reference,
# which need not match the unit's file base name (that base is fixed by the
# `@@Unit@Initialize` export). Map each such unit to the class-name prefix its
# functions actually use.
UNIT_CLASS_ALIASES = {
    "Mainform": ["TGUI"],
    "Map": ["ChestItem"],
    "Mapwarp": ["MapWarp"],
    "Mapobject": ["MapObject"],
    "Mapchest": ["MapChest"],
    "Itemchest": ["MapItem"],
    "Msgboard": ["MsgBoard"],
    "Msgboardcontrol": ["MsgBoardController"],
    "Chestcontrol": ["ChestController"],
    "Doorcontrol": ["DoorController"],
    "Effectcontrol": ["EffectController"],
    "Eventcontrol": ["EventController"],
    "Npccontrol": ["NpcController"],
    "Jukeboxcontrol": ["JukeBoxController"],
    "Filecache": ["FileCache", "TopPlayer", "TopGuild"],
    "Killcounters": ["KillCounters"],
    "Questtype": ["QuestType"],
    "Questcounter": ["QuestCounter"],
    "Questcounterlist": ["QuestCounterList"],
    "Questcounters": ["QuestCounters"],
    "Packets": ["Packets"],
    "Banned": ["Asocketban"],
    "Itemground": ["ItemObj"],
    "Learnitem": ["LearnItemVal"],
    "Npcdrop": ["NpcDropItem"],
    "Shopcraft": ["ShopCraftVal"],
    "Shopitem": ["ShopItemVal"],
    "Weaponmap": ["WeaponMapper"],
    "Weddings": ["Wedding", "WeddingController"],
    "Gamecontrol": ["Game"],
    "Mapcontrol": ["MapContainer"],
    "Mysqlcontrols": ["mySQLdb"],
    "Newscontrol": ["NewsTopics"],
    "Questengine": ["QuestContainer"],
    "Serial": ["SerialKey"],
    "Logins": ["Asocketvip", "Asocketblock"],
}


def _load(name):
    spec = importlib.util.spec_from_file_location(
        name, os.path.join(HERE, name + ".py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def load_compare():
    return _load("compare_asm")


def canon(seq):
    """Drop trailing padding and ignore which callee-saved regs are pushed."""
    seq = list(seq)
    while seq and seq[-1] == "nop":
        seq.pop()
    head = seq[:8]
    return [x for x in head if x not in ("push ebx", "push esi", "push edi")] + seq[8:]


def reference_pool(ca, ref_bin, ranges, parsed=None, merged=None):
    """Canonical instruction lists for every plausible cut of every range.

    `objdump` must be invoked per range (not once over the whole image): a linear
    sweep desynchronises on data embedded in `.text` and then decodes the wrong
    boundaries, whereas starting at each function re-syncs. The calls are I/O
    bound, so they are issued in parallel by `parse_ranges`.
    """
    pool = set()
    for i, (start, end) in enumerate(ranges):
        ins = parsed[i]
        pool.add(tuple(canon(ins)))
        for j, x in enumerate(ins):
            if x == "ret" or x.startswith("ret "):
                pool.add(tuple(canon(ins[:j + 1])))
        # Ghidra occasionally splits a function after its prologue (for example
        # Settings_ReadIniBool at 0x4158c4 -> FUN_004158ca). A ret-less fragment
        # no longer than a prologue is re-joined with its immediate successor.
        if ("ret" not in ins and len(ins) <= 6 and i + 1 < len(ranges)
                and ranges[i + 1][0] == end):
            pool.add(tuple(canon(merged[i])))
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


def parse_ranges(ca, ref_bin, ranges, merged_ranges, jobs):
    """Parse every range with objdump, in parallel (each call re-syncs)."""
    work = list(ranges) + list(merged_ranges)
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(lambda se: ca.parse_ref(ref_bin, se[0], se[1])[0], work))
    return out[:len(ranges)], out[len(ranges):]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("units", nargs="*", help="unit names (default: all build/*.asm)")
    ap.add_argument("--ref-bin", default="GameServer.exe")
    ap.add_argument("--asm-dir", default="build")
    ap.add_argument("--tsv", default="analysis/target/unit_functions.tsv")
    ap.add_argument("-j", "--jobs", type=int, default=0,
                    help="parallel objdump jobs (0 = CPU count)")
    args = ap.parse_args()

    ca = load_compare()
    ranges = unit_ranges(args.tsv)

    # objdump is invoked per range (a whole-image sweep desyncs); issue them all
    # in parallel, including the merged-range case.
    jobs = args.jobs or (os.cpu_count() or 4)
    parsed_by_unit, merged_by_unit = {}, {}
    for unit, rs in ranges.items():
        merged = [(s, rs[i + 1][1]) for i, (s, e) in enumerate(rs)
                  if i + 1 < len(rs) and rs[i + 1][0] == e]
        parsed_by_unit[unit] = (rs, merged)
    # Flatten every range across every unit into one parallel batch.
    work = [(u, se) for u, (rs, mg) in parsed_by_unit.items()
            for se in list(rs) + list(mg)]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        done = list(ex.map(lambda it: ca.parse_ref(args.ref_bin, it[1][0], it[1][1])[0],
                           work))
    buckets = defaultdict(list)
    for (u, _se), ins in zip(work, done):
        buckets[u].append(ins)
    for unit, (rs, mg) in parsed_by_unit.items():
        b = buckets[unit]
        parsed_by_unit[unit] = b[:len(rs)]
        merged_by_unit[unit] = b[len(rs):]

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
        pool = reference_pool(ca, args.ref_bin, ranges[tsv_unit],
                              parsed_by_unit[tsv_unit], merged_by_unit[tsv_unit])
        txt = open(asm, encoding="latin1").read()
        own = [f"@@{tsv_unit}@".lower()]
        own += [f"@@{c}@".lower() for c in UNIT_CLASS_ALIASES.get(tsv_unit, ())]
        ok = n = 0
        bad = []
        benign = []
        for name in re.findall(r"(?m)^(\S+)\s+proc\s+near", txt):
            # Placeholders from scripts/genstubs.py: not reconstructions, so they
            # are excluded from the score (track.py reports them as `stubbed`).
            if "_Stub" in name:
                continue
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
