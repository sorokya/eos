#!/usr/bin/env python3
"""Where does the rebuild emit a COMDAT the reference does not, and vice versa?

`funcdiff.py` answers "does each *reference* function's masked body appear in
our image".  It cannot see the opposite error -- a COMDAT we emit that the
reference does not have, or one we emit in the wrong module -- and that is
what the residual `.text` size difference is made of: bcc32 emits every
function as a COMDAT, `ilink32` keeps the copy from the *first* object that
defines it, so which translation unit odr-uses a template member is
observable in the linked layout.

This tool takes our function inventory from the TDS (`tdsfuncs.py`), groups it
by *masked body* (relocation slots and direct-branch displacements zeroed, so
byte-identical instantiations of different types fall in one group), and then
counts the masked occurrences of each body inside the matching module of both
images.  A count mismatch is a real, per-module COMDAT surplus or deficit in
bytes; the identity of a byte-identical body cannot be resolved by counting,
so both the reference and our names/addresses are printed for the reader.

Module i of the reference (`analysis/target/modules.tsv`) is paired with module
i of the rebuild (`unitmap.py --pe --stubs`); the two scans agree on count.

    scripts/comdatdiff.py                    # every module with a delta
    scripts/comdatdiff.py --module 15        # one module, by index
    scripts/comdatdiff.py --unit Mapcontrol  # one module, by reference name
    scripts/comdatdiff.py --body 0x480c14:36 # locate one body in both images

Exit status is non-zero when any module has a count mismatch.
"""
import argparse
import collections
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import funcdiff as F          # noqa: E402  (load/find/rel32_offsets)
import tdsfuncs               # noqa: E402


def read_modules(path):
    rows = []
    for line in open(path, encoding='utf-8').read().splitlines()[1:]:
        f = line.split('\t')
        if len(f) < 6:
            continue
        rows.append((int(f[0], 16) if f[0].strip() else None,
                     int(f[1], 16) if f[1].strip() else None, f[4]))
    return rows


def scan_modules(exe):
    out = subprocess.run([sys.executable, os.path.join(HERE, 'unitmap.py'),
                          '--pe', exe, '--stubs'], capture_output=True, text=True)
    rows = []
    for line in out.stdout.splitlines()[1:]:
        f = line.split('\t')
        if len(f) < 2:
            continue
        rows.append((int(f[0], 16) if f[0].strip() else None,
                     int(f[1], 16) if f[1].strip() else None,
                     f[4] if len(f) > 4 else ''))
    return rows


def module_of(mods, addr):
    for i, (s, e, _n) in enumerate(mods):
        if s is not None and s <= addr < e:
            return i
    return None


def masked(exe, text, base, relocs, va, length):
    """(signature, literal offsets) for one function, or None if unusable."""
    if va - base < 0 or va - base + length > len(text) or length < 8:
        return None
    sig = text[va - base:va - base + length]
    r32 = F.rel32_offsets(exe, text, base, va, va + length)
    lit = [i for i in range(length) if (va + i) not in relocs and i not in r32]
    if len(lit) < 6:
        return None
    return sig, lit


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ref', default='GameServer.exe')
    ap.add_argument('--linked', default='build/GameServer.exe')
    ap.add_argument('--tds', default='build/GameServer.tds')
    ap.add_argument('--modules', default='analysis/target/modules.tsv')
    ap.add_argument('--module', type=int, action='append', default=[])
    ap.add_argument('--unit', default='')
    ap.add_argument('--body', default='',
                    help='VA:LENGTH of one of our functions; list every masked '
                         'copy of its body in both images')
    args = ap.parse_args()

    _rp, rtext, rbase, rrel = F.load(args.ref)
    _op, otext, obase, orel = F.load(args.linked)
    ref_mod = read_modules(args.modules)
    our_mod = scan_modules(args.linked)
    funcs = tdsfuncs.unique(tdsfuncs.parse(args.tds)[0])
    named = {va: (ln, nm) for va, ln, nm, _m, _k in funcs}

    def rname(i):
        return (ref_mod[i][2] or f'#{i}') if i is not None and i < len(ref_mod) else 'tail'

    if args.body:
        va, _, ln = args.body.partition(':')
        va, ln = int(va, 0), int(ln or named.get(int(va, 0), (0,))[0])
        m = masked(args.linked, otext, obase, orel, va, ln)
        if m is None:
            sys.exit(f'{va:#x}+{ln}: no maskable body')
        sig, lit = m
        ours = sorted(F.find(sig, lit, ln, obase, obase + len(otext), otext, obase, orel))
        refs = sorted(F.find(sig, lit, ln, rbase, rbase + len(rtext), rtext, rbase, rrel))
        print(f'body {va:#x} length {ln}: ours {len(ours)}, reference {len(refs)}')
        for a in ours:
            print(f'   ours {a:#010x} {rname(module_of(our_mod, a)):18s} '
                  f'{named.get(a, (0, "<unnamed>"))[1]}')
        for a in refs:
            print(f'   ref  {a:#010x} {rname(module_of(ref_mod, a))}')
        print('ours per module:',
              dict(collections.Counter(rname(module_of(our_mod, a)) for a in ours)))
        print('ref  per module:',
              dict(collections.Counter(rname(module_of(ref_mod, a)) for a in refs)))
        return 0

    wanted = set(args.module)
    if args.unit:
        wanted |= {i for i, (_s, _e, n) in enumerate(ref_mod) if n == args.unit}
    if not wanted:
        wanted = set(range(min(len(ref_mod), len(our_mod))))

    bad = 0
    for i in sorted(wanted):
        rs, re_, rn = ref_mod[i]
        os_, oe, _ = our_mod[i]
        if rs is None or os_ is None:
            continue
        delta = (oe - os_) - (re_ - rs)
        groups = collections.defaultdict(list)
        for va, ln, nm, _mod, _k in funcs:
            if not (os_ <= va < oe):
                continue
            m = masked(args.linked, otext, obase, orel, va, ln)
            if m is None:
                continue
            sig, lit = m
            key = (ln, bytes(sig[k] if k in set(lit) else 0 for k in range(ln)))
            groups[key].append((va, nm, lit))
        rows, net = [], 0
        for (ln, _canon), members in groups.items():
            va, nm, lit = members[0]
            sig = otext[va - obase:va - obase + ln]
            n_ref = len(F.find(sig, list(lit), ln, rs, re_, rtext, rbase, rrel))
            n_our = len(F.find(sig, list(lit), ln, os_, oe, otext, obase, orel))
            if n_our != n_ref:
                net += (n_our - n_ref) * ln
                rows.append((ln, n_our, n_ref, members))
        if not rows and not delta:
            continue
        bad += bool(rows)
        print(f'=== {i} {rn or "#" + str(i)}: ref {re_ - rs}  ours {oe - os_}  '
              f'delta {delta:+d} ===')
        for ln, n_our, n_ref, members in sorted(rows, key=lambda z: -z[0] * abs(z[1] - z[2])):
            print(f'   body {ln:5d} B  ours {n_our:3d}  ref {n_ref:3d}  '
                  f'({(n_our - n_ref) * ln:+d} bytes)')
            for va, nm, _l in members:
                print(f'        {va:#010x} {nm[:96]}')
        print(f'   net from count mismatches: {net:+d}')
    return 1 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main())
