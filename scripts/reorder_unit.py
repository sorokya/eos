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

Which of a block's names *is* the definition -- as opposed to a template
instantiation or an RTL helper that merely carries its line numbers -- is
decided by the **object's** COMDAT order (`orderdiff.object_orders`), not by a
heuristic on the name: bcc32 emits a function and then the helpers it pulled in,
so the first of a block's names to appear in the object is the definition
itself.  Ranking by anything else (lowest line, lowest listing position, "looks
author-written") mis-attributes blocks whose only reference-named symbol is a
compiler COMDAT, and the plan then oscillates between two orders.

Definitions the reference does not name (helpers the linker drops) keep their
position relative to the definition they follow.  Top-level chunks that are not
function definitions (includes, globals, forward declarations, type bodies,
multi-line `#define`s) stay in their own slots and are never moved.

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
sys.path.insert(0, HERE)
import orderdiff  # noqa: E402  (same directory)

PROC_RE = re.compile(r'^(@[^\s]+)\s+proc\s+near\s*$')
LINE_RE = re.compile(r'^\t\?debug L (\d+)')
FILE_RE = re.compile(r'^\t\?debug\s+T\s+"([^"]+)"')


def listing(unit, refresh):
    """Return build/<unit>.asm, compiling it when missing or stale."""
    src = os.path.join(ROOT, 'src', unit + '.cpp')
    asm = os.path.join(ROOT, 'build', unit + '.asm')
    if refresh or not os.path.exists(asm) or os.path.getmtime(asm) < os.path.getmtime(src):
        cmd = ('wine "$B\\Bin\\bcc32.exe" -v -Od -tWM -k -S -obuild/%s.asm src/%s.cpp' % (unit, unit))
        # bcc32 -S on a multi-megabyte listing faults under Wine now and then --
        # the same intermittent fault scripts/build.sh documents for ilink32 -s.
        # One retry clears it.
        for attempt in (1, 2):
            before = os.path.getmtime(asm) if os.path.exists(asm) else -1
            r = subprocess.run([os.path.join(HERE, 'borland.sh'), cmd],
                               cwd=ROOT, capture_output=True, text=True)
            if os.path.exists(asm) and os.path.getmtime(asm) != before:
                break
        else:
            out = '\n'.join(l for l in (r.stdout + r.stderr).splitlines()
                             if 'XDG_RUNTIME_DIR' not in l and not l.startswith('Warning'))
            raise SystemExit('bcc32 -S produced no listing for %s:\n%s' % (unit, out[-1500:]))
    return asm


def name_lines(asm, src_name):
    """mangled name -> the first line bcc32 recorded inside its body, but only
    for functions whose provenance is this unit's own .cpp.

    `?debug L <line>` is relative to the file named by the last `?debug T`, and
    bcc32 emits T only when the file *changes* -- so a template instantiation
    pulled in from `Include/vector.h` carries line numbers from *that* file.
    Treating them as .cpp lines maps helpers into arbitrary source blocks, which
    is what made earlier ranking rules unstable. Anything not written in this
    .cpp is left unmapped: it belongs to no block and cannot influence the order.
    """
    out = {}
    cur = None
    here = True
    for line in open(asm, errors='replace'):
        line = line.rstrip('\n')
        m = FILE_RE.match(line)
        if m:
            here = m.group(1).replace('\\', '/').endswith(src_name)
            continue
        m = PROC_RE.match(line)
        if m:
            cur = m.group(1).lstrip('@')
            continue
        if cur:
            m = LINE_RE.match(line)
            if m:
                if here:
                    out.setdefault(cur, int(m.group(1)))
                cur = None
    return out


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
    """chunks_of() for a file on disk; returns (lines, items)."""
    return chunks_of(open(path).read())


def chunks_of(text):
    """Split a .cpp into top-level items.

    A *definition* item is a brace-balanced top-level block plus the signature
    and the comment block immediately above it; anything else (includes,
    pragmas, globals, forward declarations, stray comments) is a separate item
    that never moves.  Line numbers are 1-based and inclusive.
    """
    lines = text.split('\n')

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
    n2l = name_lines(asm, unit + '.cpp')
    objorder = orderdiff.object_orders([unit], args.refresh)[unit]
    ref = reference_order(unit, args.functions)
    lines, items = chunks(src)
    spans = [(a, b) for a, b, _ in items]
    movable = {i for i, (_, _, f) in enumerate(items) if f}

    # which block each name belongs to, by source line
    home = {}
    for name, ln in n2l.items():
        for idx, (a, b) in enumerate(spans):
            if a <= ln <= b and idx in movable:
                home[name] = idx
                break

    # A block's definition is the name whose recorded line is lowest -- that is
    # its signature, while the helpers it pulled in carry the line of the
    # statement that needed them, further down. Ties (a helper that landed on the
    # signature line of a three-line function) go to whichever the *object*
    # emits first, which is the definition: bcc32 emits a function and then its
    # helpers. Using either signal alone mis-attributes some blocks and the plan
    # then oscillates between two orders.
    pos = {n: i for i, n in enumerate(objorder)}
    block_of = {}
    for name in objorder:
        idx = home.get(name)
        if idx is not None:
            block_of.setdefault(idx, []).append(name)
    own = {}
    for idx, names in block_of.items():
        own[idx] = min(names, key=lambda n: (n2l[n], pos[n]))

    rank = {n: i for i, n in enumerate(ref)}
    func_slots = sorted(movable)
    # blocks the reference does not name inherit the previous block's key so
    # they stay put relative to it
    keys = {}
    last = -1.0
    bump = 0
    for idx in func_slots:
        n = own.get(idx)
        if n is not None and n in rank:
            last = float(rank[n])
            bump = 0
            keys[idx] = last
        else:
            bump += 1
            keys[idx] = last + bump / 1000.0
    order = sorted(func_slots, key=lambda i: (keys[i], i))
    return src, lines, spans, func_slots, order, block_of


def declarations(lines, items):
    """`<signature>;` for every free function defined here.

    Moving definitions can put a free function's callers above it, which C++
    will not accept. Declarations emit nothing, so adding one for every free
    function in the unit is codegen-neutral and removes the ordering constraint
    entirely. Member functions already have their declaration in the header.
    """
    out = []
    for a, b, isdef in items:
        if not isdef:
            continue
        sig = []
        for k in range(a - 1, b):
            t = lines[k]
            if t.lstrip().startswith(('//', '#')):
                continue
            sig.append(t)
            if '{' in t:
                break
        text = ' '.join(x.strip() for x in sig)
        if '{' in text:
            text = text[:text.index('{')]
        text = ' '.join(text.split())
        if not text or '::' in text:
            continue
        if text.startswith(('class ', 'struct ', 'enum ', 'union ', 'namespace ',
                            'extern "C"', 'typedef ')):
            continue
        if not re.search(r'\w\s*\(', text):
            continue
        out.append(text + ';')
    seen = set()
    uniq = []
    for d in out:
        if d not in seen:
            seen.add(d)
            uniq.append(d)
    return uniq


def with_declarations(text, decls):
    """Insert `decls` after `#pragma package(smart_init)` (or the last include),
    replacing any identical statement already present in that preamble."""
    lines = text.split('\n')
    try:
        at = lines.index('#pragma package(smart_init)')
    except ValueError:
        at = max((i for i, l in enumerate(lines) if l.startswith('#include')), default=0)
    body = lines[at + 1:]
    # drop declarations we are about to re-emit, wherever they sit above the
    # first definition, so repeated runs do not pile up duplicates
    want = set(decls)
    kept = []
    for l in body:
        if l.strip() in want:
            continue
        kept.append(l)
    return '\n'.join(lines[:at + 1] + [''] + decls + kept)


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
    ap.add_argument('--no-declare', action='store_true',
                    help='do not add forward declarations for free functions')
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
            if not args.no_declare:
                tlines, items = chunks_of(text)
                decls = declarations(tlines, items)
                if decls:
                    text = with_declarations(text, decls)
                    print('   %d forward declarations' % len(decls))
            open(src, 'w').write(text if text.endswith('\n') else text + '\n')
            print('   rewrote %s' % src)
        else:
            for a, b in zip(func_slots, order):
                if a != b:
                    print('   slot %d <- %s' % (a, ','.join(block_of.get(b, ['?']))[:70]))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
