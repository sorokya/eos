#!/usr/bin/env python3
"""Reorder one unit's function definitions into the reference's order.

`ilink32` lays a module's COMDATs out in the order its object defines them, and
bcc32 defines them in source order, so the reference's per-function addresses
record the original source order (see `orderdiff.py`).  Bringing a unit's
source into that order is a pure move of whole definitions -- no code changes --
but doing it by hand across 200 functions is not practical, so this does it.

How a source definition is tied to a mangled name: `bcc32 -S` emits, for every
function, a `<mangled> proc near` label followed by `?debug L <line>` markers
that carry the source line.  The first such marker inside a proc is that
function's first line, which lands inside exactly one top-level `{...}` block of
the source file.  That gives an exact source-block -> mangled-name map with no
signature parsing.

Definitions that the reference does not name (helpers the linker drops, compiler
COMDATs) keep their position relative to the definition they follow.  Top-level
chunks that are not function definitions (includes, globals, forward
declarations) stay in their own slots and are never moved.

Usage:
    scripts/reorder_unit.py Effectcontrol            # show the plan
    scripts/reorder_unit.py Effectcontrol --apply    # rewrite src/Effectcontrol.cpp
    scripts/reorder_unit.py --all                    # plan for every unit
"""
import argparse
import csv
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

PROC_RE = re.compile(r'^(@[^\s]+)\s+proc\s+near\s*$')
LINE_RE = re.compile(r'^\t\?debug L (\d+)')


def listing(unit, refresh):
    """Return build/<unit>.asm, compiling it when missing or stale."""
    src = os.path.join(ROOT, 'src', unit + '.cpp')
    asm = os.path.join(ROOT, 'build', unit + '.asm')
    if refresh or not os.path.exists(asm) or os.path.getmtime(asm) < os.path.getmtime(src):
        cmd = ('wine "$B\\Bin\\bcc32.exe" -v -Od -tWM -k -S -obuild/%s.asm src/%s.cpp' % (unit, unit))
        r = subprocess.run([os.path.join(HERE, 'borland.sh'), cmd],
                           cwd=ROOT, capture_output=True, text=True)
        if not os.path.exists(asm):
            raise SystemExit('bcc32 -S failed for %s:\n%s' % (unit, r.stdout + r.stderr))
    return asm


def name_lines(asm):
    """mangled name -> (first source line, position in the listing).

    Within one source definition the name whose line is the *lowest* is the
    definition itself (its signature); the compiler-generated COMDATs it pulled
    in carry the line of the statement that needed them, which is further down.
    The listing position is kept only as a tie-break.
    """
    out = {}
    cur = None
    seq = 0
    for line in open(asm, errors='replace'):
        m = PROC_RE.match(line.rstrip('\n'))
        if m:
            cur = m.group(1).lstrip('@')
            continue
        if cur:
            m = LINE_RE.match(line.rstrip('\n'))
            if m:
                out.setdefault(cur, (int(m.group(1)), seq))
                seq += 1
                cur = None
    return out


# Names bcc32 generates rather than the unit's author: template instantiations
# (they carry `%`) and members of the RTL/VCL namespaces. Within one source
# definition these share the definition's line numbers, so they must not be
# mistaken for the definition itself when deciding where the block belongs.
LIB_NS = ('std@', '__rwstd@', 'System@', 'Sysutils@', 'Classes@', 'Forms@',
          'Dialogs@', 'Scktcomp@', 'Controls@', 'Graphics@', 'Db@', 'Dbtables@',
          'Inifiles@', 'Stdctrls@', 'Extctrls@', 'Comobj@', 'Typinfo@', 'Math@',
          'Registry@', 'Syncobjs@', 'Appevnts@', 'Menus@', 'Variants@', '$b')


def author_written(name):
    return '%' not in name and not name.startswith(LIB_NS)


def reference_order(unit, path):
    order = []
    with open(path) as fh:
        for row in csv.DictReader(fh, delimiter='\t'):
            if row['unit'] != unit or row['kind'] == 'stub':
                continue
            name = row['source_name'].lstrip('@')
            if name:
                order.append((int(row['start'], 16), name))
    return [n for _, n in sorted(order)]


def chunks(path):
    """Split a .cpp into top-level items.

    A *definition* item is a brace-balanced top-level block plus the signature
    and the comment block immediately above it; anything else (includes,
    pragmas, globals, forward declarations, stray comments) is a separate item
    that never moves.  Line numbers are 1-based and inclusive.
    """
    lines = open(path).read().split('\n')

    def code(s):
        s = re.sub(r'\\.', '', s)
        s = re.sub(r'"[^"]*"', '""', s)
        s = re.sub(r"'[^']*'", "''", s)
        return re.sub(r'//.*', '', s)

    # a preprocessor directive and its backslash continuations carry no braces
    # the compiler sees here: a multi-line #define body can be full of them.
    skip = [False] * len(lines)
    i = 0
    while i < len(lines):
        if lines[i].lstrip().startswith('#'):
            while True:
                skip[i] = True
                if not lines[i].rstrip().endswith('\\') or i + 1 >= len(lines):
                    break
                i += 1
        i += 1

    # find the top-level brace blocks first
    blocks = []
    depth = 0
    open_at = None
    for i, raw in enumerate(lines):
        if skip[i]:
            continue
        for ch in code(raw):
            if ch == '{':
                if depth == 0:
                    open_at = i
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0 and open_at is not None:
                    blocks.append((open_at, i))
                    open_at = None
    # widen each block upwards over its signature and leading comment block
    items = []
    prev_end = -1
    for open_at, close_at in blocks:
        start = open_at
        while start - 1 > prev_end:
            t = lines[start - 1].strip()
            if not t or t.startswith('#') or t.endswith(';') or t.endswith('}'):
                break
            start -= 1
        while start - 1 > prev_end and lines[start - 1].lstrip().startswith('//'):
            start -= 1
        if start - 1 > prev_end:
            items.append((prev_end + 2, start, False))
        # `};` closes a type/aggregate definition, not a function body
        is_def = not lines[close_at].strip().endswith(';')
        items.append((start + 1, close_at + 1, is_def))
        prev_end = close_at
    if prev_end + 1 < len(lines):
        items.append((prev_end + 2, len(lines), False))
    return lines, items


def plan(unit, args):
    src = os.path.join(ROOT, 'src', unit + '.cpp')
    asm = listing(unit, args.refresh)
    n2l = name_lines(asm)
    ref = reference_order(unit, args.functions)
    lines, items = chunks(src)
    spans = [(a, b) for a, b, _ in items]
    movable = {i for i, (_, _, f) in enumerate(items) if f}

    # source block index -> names defined in it
    block_of = {}
    for name, (ln, seq) in n2l.items():
        for idx, (a, b) in enumerate(spans):
            if a <= ln <= b and idx in movable:
                block_of.setdefault(idx, []).append((ln, seq, name))
                break
    for idx in block_of:
        block_of[idx].sort()

    rank = {n: i for i, n in enumerate(ref)}
    func_slots = sorted(movable)
    # a block's key is the best (lowest) reference rank of the names it defines;
    # blocks the reference does not name inherit the previous block's key so they
    # stay put relative to it
    keys = {}
    last = -1.0
    bump = 0
    for idx in func_slots:
        named = [n for _, _, n in block_of.get(idx, []) if n in rank]
        own = [n for n in named if author_written(n)] or named
        if own:
            last = float(rank[own[0]])
            bump = 0
            keys[idx] = last
        else:
            bump += 1
            keys[idx] = last + bump / 1000.0
    order = sorted(func_slots, key=lambda i: (keys[i], i))
    return src, lines, spans, func_slots, order, block_of


def render(lines, spans, func_slots, order):
    out = []
    pos = 0
    replacement = dict(zip(func_slots, order))
    for idx, (a, b) in enumerate(spans):
        take = replacement.get(idx, idx)
        ta, tb = spans[take]
        chunk = lines[ta - 1:tb]
        out.extend(chunk)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('unit', nargs='*')
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--refresh', action='store_true', help='always recompile the -S listing')
    ap.add_argument('--functions', default=os.path.join(ROOT, 'analysis/target/functions.tsv'))
    args = ap.parse_args()

    units = args.unit
    if args.all:
        out = subprocess.run([sys.executable, os.path.join(HERE, 'orderdiff.py')],
                             capture_output=True, text=True, cwd=ROOT)
        units = [l.split()[0] for l in out.stdout.splitlines()
                 if l and not l.startswith(' ') and l.split()[0] not in
                 ('units', 'unit') and os.path.exists(os.path.join(ROOT, 'src', l.split()[0] + '.cpp'))]
    if not units:
        raise SystemExit('name a unit, or pass --all')

    for unit in units:
        src, lines, spans, func_slots, order, block_of = plan(unit, args)
        moved = sum(1 for a, b in zip(func_slots, order) if a != b)
        print('%-18s %3d definitions, %3d move' % (unit, len(func_slots), moved))
        if not moved:
            continue
        if args.apply:
            text = '\n'.join(render(lines, spans, func_slots, order))
            open(src, 'w').write(text if text.endswith('\n') else text + '\n')
            print('   rewrote %s' % src)
        else:
            for a, b in zip(func_slots, order):
                if a != b:
                    print('   slot %d <- %s' % (a, ','.join(n for _, _, n in block_of.get(b, [(0, 0, '?')]))[:70]))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
