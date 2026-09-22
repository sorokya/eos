#!/usr/bin/env python3
"""Within-module function order: reference vs rebuild.

`funcdiff.py` walks reference -> rebuild and asks "are these bytes somewhere in
our image"; `layoutdiff.py` compares *module* start addresses.  Neither sees a
reordering *inside* a module: every function can be byte-exact and every module
can start at its reference RVA while the functions inside one module sit in a
different order, which moves every byte of that module.

`ilink32` lays a module's COMDATs out in the order the object defines them, and
bcc32 defines them in source order (with each function's implicitly generated
helpers flushed right after it).  The reference's per-function addresses
therefore record the original source order, and this script compares it against
ours.

Reference order comes from `analysis/target/functions.tsv` (make unitmap &&
make track); our order comes from the linked image's TD32 debug file, via
`tdsfuncs.py`, which names every COMDAT including the compiler-generated ones.
Functions are matched by mangled name, so only names present on both sides take
part; a name only one side has is reported separately by `comdatdiff.py`.

Usage:
    MAP=1 scripts/build.sh
    scripts/orderdiff.py [--unit Packets] [--verbose]

Exits non-zero when any unit's order differs.
"""
import argparse
import collections
import csv
import difflib
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


def reference_order(path):
    units = collections.defaultdict(list)
    with open(path) as fh:
        for row in csv.DictReader(fh, delimiter='\t'):
            if row['kind'] == 'stub':
                continue
            name = row['source_name'].lstrip('@')
            if not name:
                continue
            units[row['unit']].append((int(row['start'], 16), name))
    return {u: [n for _, n in sorted(v)] for u, v in units.items()}


def rebuild_order(tds):
    cmd = [sys.executable, os.path.join(HERE, 'tdsfuncs.py')]
    if tds:
        cmd.append(tds)
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    if out.returncode != 0:
        raise SystemExit(out.stderr.strip() or 'tdsfuncs.py failed')
    units = collections.defaultdict(list)
    for line in out.stdout.splitlines():
        parts = line.split('\t')
        if len(parts) < 5 or not parts[0].startswith('0x'):
            continue
        module = parts[3]
        if not module.startswith('src/') or not module.endswith('.cpp'):
            continue
        units[module[4:-4]].append((int(parts[0], 16), parts[4].lstrip('@')))
    return {u: [n for _, n in sorted(v)] for u, v in units.items()}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--functions', default=os.path.join(ROOT, 'analysis/target/functions.tsv'))
    ap.add_argument('--tds', default=None, help='TD32 file (default: build/GameServer.tds)')
    ap.add_argument('--unit', action='append', help='restrict to these units')
    ap.add_argument('--verbose', action='store_true', help='print every moved function')
    args = ap.parse_args()

    ref = reference_order(args.functions)
    ours = rebuild_order(args.tds)

    rows = []
    for unit in ref:
        if args.unit and unit not in args.unit:
            continue
        if unit not in ours:
            continue
        common = set(ref[unit]) & set(ours[unit])
        r = [n for n in ref[unit] if n in common]
        o = [n for n in ours[unit] if n in common]
        if r == o:
            continue
        sm = difflib.SequenceMatcher(None, r, o, autojunk=False)
        moved = sum(i2 - i1 for tag, i1, i2, _, _ in sm.get_opcodes() if tag != 'equal')
        rows.append((moved, unit, r, o, sm))

    matched = sum(1 for u in ref if u in ours and (not args.unit or u in args.unit))
    print('units compared            : %d' % matched)
    print('units out of order        : %d' % len(rows))
    if not rows:
        print('\nevery unit\'s functions are in the reference order')
        return 0

    rows.sort(reverse=True)
    print('\n%-20s %6s %6s %8s  first divergence' % ('unit', 'funcs', 'moved', 'in-order'))
    for moved, unit, r, o, sm in rows:
        ops = [x for x in sm.get_opcodes() if x[0] != 'equal']
        tag, i1, i2, j1, j2 = ops[0]
        want = r[i1][:44] if i1 < len(r) else '(end)'
        got = o[j1][:44] if j1 < len(o) else '(end)'
        print('%-20s %6d %6d %7.1f%%  at index %d' % (unit, len(r), moved, 100.0 * sm.ratio(), i1))
        print('%-20s %s ref: %s' % ('', ' ' * 21, want))
        print('%-20s %s our: %s' % ('', ' ' * 21, got))
        if args.verbose:
            for tag, i1, i2, j1, j2 in ops:
                for n in r[i1:i2]:
                    print('        - %s' % n)
                for n in o[j1:j2]:
                    print('        + %s' % n)
    return 1


if __name__ == '__main__':
    raise SystemExit(main())
