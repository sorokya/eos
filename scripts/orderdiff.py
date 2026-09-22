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
make track). Our order has two sources, and they agree exactly:

  * the linked image's TD32 debug file, via `tdsfuncs.py` -- the whole tree at
    once, but it needs a full `scripts/build.sh` (minutes); and
  * one unit's **object file**, via `--from-obj`. bcc32 emits each COMDAT as an
    OMF communal (`COMDEF ... virtual(_TEXT)`) and `ilink32` lays a module out in
    exactly that order, so a single `bcc32 -c` (seconds) answers the question for
    that unit with no link at all. This is the loop to iterate in; run the linked
    form to confirm.

Functions are matched by mangled name, so only names present on both sides take
part; a name only one side has is reported separately by `comdatdiff.py`.

Usage:
    scripts/orderdiff.py --from-obj --unit Packets     # fast, one compile
    MAP=1 scripts/build.sh && scripts/orderdiff.py     # whole tree, from the link

Exits non-zero when any unit's order differs.
"""
import argparse
import collections
import csv
import difflib
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


COMDAT_RE = re.compile(r"^\s*Name:\s*\d+:\s*'([^']+)'\s+virtual\(_TEXT\)")


def object_orders(units, refresh=False, cflags='-v -Od -tWM -k'):
    """{unit: [mangled COMDAT name, ...]} in the order each object defines them,
    which is the order ilink32 lays that module out in.

    Every unit is compiled and dumped in **one** container invocation: the
    round-trip costs seconds and doing it per unit dominates everything else.
    `tdump -m` keeps the names mangled -- without it they come back demangled and
    cannot be matched against the reference sheet.
    """
    script = []
    for unit in units:
        src = os.path.join(ROOT, 'src', unit + '.cpp')
        obj = os.path.join(ROOT, 'build', 'obj', unit + '.obj')
        if not os.path.exists(src):
            raise SystemExit('no src/%s.cpp' % unit)
        if refresh or not os.path.exists(obj) or os.path.getmtime(obj) < os.path.getmtime(src):
            script.append('wine "$B\\Bin\\bcc32.exe" %s -c -obuild/obj/%s.obj src/%s.cpp '
                          '>/dev/null 2>&1' % (cflags, unit, unit))
    for unit in units:
        script.append('echo "=== %s"' % unit)
        script.append('wine "$B\\Bin\\tdump.exe" -m -q build/obj/%s.obj 2>/dev/null' % unit)
    r = subprocess.run([os.path.join(HERE, 'borland.sh'), '\n'.join(script)],
                       cwd=ROOT, capture_output=True, text=True)
    out = {}
    cur = None
    for line in r.stdout.splitlines():
        if line.startswith('=== '):
            cur = line[4:].strip()
            out[cur] = []
            continue
        m = COMDAT_RE.match(line)
        if m and cur is not None:
            out[cur].append(m.group(1).lstrip('@'))
    missing = [u for u in units if not out.get(u)]
    if missing:
        raise SystemExit('no COMDAT records for: %s\n%s'
                         % (', '.join(missing), (r.stdout + r.stderr)[-2000:]))
    return out


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
    ap.add_argument('--from-obj', action='store_true',
                    help='read our order from the unit objects instead of the linked TDS '
                         '(one bcc32 -c per unit, no link)')
    ap.add_argument('--refresh', action='store_true', help='recompile the objects first')
    args = ap.parse_args()

    ref = reference_order(args.functions)
    if args.from_obj:
        want = args.unit or sorted(
            u for u in ref if os.path.exists(os.path.join(ROOT, 'src', u + '.cpp')))
        ours = object_orders(want, args.refresh)
    else:
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
