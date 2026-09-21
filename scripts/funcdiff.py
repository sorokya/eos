#!/usr/bin/env python3
"""Which reference functions' bytes differ from the linked rebuild?

`make verify` and `compare_asm.py` canonicalise branch targets, so a function
whose relative branch lands somewhere else still reads as byte-exact.  This tool
masks only what the *layout* moves -- the four bytes of every base relocation and
the 4-byte displacement of a direct `call`/`jmp`/`jcc` -- and then asks whether
each reference function's remaining bytes appear anywhere in our image.

Three masking traps make the naive version lie; this tool avoids all three:

  * `objdump`'s whole-image linear pass desynchronises on data embedded in
    `.text`, so its `rel32` set is wrong past the first data blob.  Each
    function is therefore decoded from its own start (capstone when importable,
    else `objdump --start-address`).
  * a base relocation is a 4-byte slot but `pe.relocations()` reports only its
    first byte -- each slot is expanded to all four bytes.
  * Ghidra's function `end` can truncate the final `e8` displacement, whose first
    byte then lies inside the range -- decode with 8 bytes of slack past `end`.

A reference function with no masked match anywhere is a real byte difference
(the `Party_ShareExp` register-allocation mirrors are the canonical example).
A function that matches only in another module is a *placement* difference, not a
byte difference; that distinction is reported when `analysis/target/modules.tsv`
and the linked module scan are available.

Usage:
    scripts/funcdiff.py                       # all functions
    scripts/funcdiff.py --unit Packets
    scripts/funcdiff.py --function Party_ShareExp
    scripts/funcdiff.py --list 20             # cap the printed rows

Exit status is non-zero when any reference function has no masked match, so it
can gate a build.
"""
import argparse
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from pe import PE  # noqa: E402

INSN_RE = re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(\S+)')

try:  # capstone is optional; the tool falls back to per-function objdump.
    import capstone

    _CS = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    _CS.detail = False
except Exception:  # pragma: no cover - exercised only without capstone
    _CS = None


def load(path):
    pe = PE.from_file(path)
    sec = [s for s in pe.sections if s.name == '.text'][0]
    text = pe.data[sec.raw_pointer:sec.raw_pointer + sec.raw_size]
    base = pe.header()['image_base'] + sec.virtual_address
    relocs = set()
    for r, _t in pe.relocations():
        a = pe.header()['image_base'] + r
        relocs.update(range(a, a + 4))
    return pe, text, base, relocs


def rel32_offsets(path, text, base, start, end):
    """Layout-dependent displacement byte offsets, relative to `start`.

    Decoded from the function's own start so embedded data cannot desync the
    walk, and with 8 bytes of slack so a final truncated `e8` is still seen.
    """
    offs = set()
    lo, hi = start, end + 8
    if _CS is not None:
        for insn in _CS.disasm(text[lo - base:hi - base], lo):
            b = insn.bytes
            o = insn.address - start
            if b[0] in (0xE8, 0xE9) and len(b) == 5:
                offs.update(range(o + 1, o + 5))
            elif b[0] == 0x0F and len(b) == 6 and 0x80 <= b[1] <= 0x8F:
                offs.update(range(o + 2, o + 6))
        return offs
    out = subprocess.run(['objdump', '-d', '-Mintel',
                          f'--start-address={lo}', f'--stop-address={hi}', path],
                         capture_output=True, text=True).stdout
    for line in out.splitlines():
        m = INSN_RE.match(line)
        if not m:
            continue
        va = int(m.group(1), 16)
        raw = m.group(2).split()
        if raw[0] in ('e8', 'e9') and len(raw) == 5:
            offs.update(range(va + 1 - start, va + 5 - start))
        elif raw[0] == '0f' and len(raw) == 6 and 0x80 <= int(raw[1], 16) <= 0x8F:
            offs.update(range(va + 2 - start, va + 6 - start))
    return offs


def runs(lit):
    out = []
    if not lit:
        return out
    s = prev = lit[0]
    for i in lit[1:]:
        if i == prev + 1:
            prev = i
            continue
        out.append((s, prev))
        s = prev = i
    out.append((s, prev))
    return out


def find(sig, lit, size, lo, hi, text, base, relocs):
    """Exact masked matches whose start is in [lo, hi)."""
    if lo is None or hi is None:
        return set()
    hits = set()
    sto = max(lo - base, 0)
    eno = min(hi - base, len(text) - size)
    for a, b in sorted(runs(lit), key=lambda r: -(r[1] - r[0]))[:6]:
        anchor = sig[a:b + 1]
        pos = text.find(anchor, sto)
        while pos != -1 and pos <= eno + a:
            cand = pos - a
            if cand >= 0 and cand + size <= len(text):
                ok = True
                for i in lit:
                    if (base + cand + i) in relocs:
                        continue
                    if text[cand + i] != sig[i]:
                        ok = False
                        break
                if ok:
                    hits.add(base + cand)
            pos = text.find(anchor, pos + 1)
    return hits


def inventory(path):
    rows = []
    with open(path, encoding='utf-8') as fh:
        hdr = next(fh).rstrip('\n').split('\t')
        ix = {n: i for i, n in enumerate(hdr)}
        for line in fh:
            p = line.rstrip('\n').split('\t')
            if len(p) < len(hdr):
                continue
            try:
                start, end = int(p[ix['start']], 16), int(p[ix['end']], 16)
            except ValueError:
                continue
            rows.append((start, end, p[ix['unit']], p[ix['ref_name']],
                         p[ix['kind']], p[ix['status']]))
    return rows


def module_scan(path):
    """[(start, end)] for the linked image, from `unitmap.py --pe --stubs`."""
    out = subprocess.run([sys.executable, os.path.join(HERE, 'unitmap.py'),
                          '--pe', path, '--stubs'], capture_output=True, text=True)
    rows = []
    for line in out.stdout.splitlines()[1:]:
        f = line.split('\t')
        if len(f) < 2:
            continue
        rows.append((int(f[0], 16) if f[0].strip() else None,
                     int(f[1], 16) if f[1].strip() else None))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ref', default='GameServer.exe')
    ap.add_argument('--linked', default='build/GameServer.exe')
    ap.add_argument('--inventory', default='analysis/target/functions.tsv')
    ap.add_argument('--modules', default='analysis/target/modules.tsv')
    ap.add_argument('--unit', default='')
    ap.add_argument('--function', default='')
    ap.add_argument('--min-size', type=int, default=8)
    ap.add_argument('--max-size', type=int, default=1 << 20)
    ap.add_argument('--list', type=int, default=0, help='cap printed rows')
    ap.add_argument('--no-modules', action='store_true',
                    help='global search only; do not split out placement')
    args = ap.parse_args()

    _rpe, rtext, rbase, rrel = load(args.ref)
    _ope, otext, obase, orel = load(args.linked)

    ours_mod = [] if args.no_modules else module_scan(args.linked)
    ref_mod = []
    if ours_mod:
        for line in open(args.modules, encoding='utf-8').read().splitlines()[1:]:
            f = line.split('\t')
            if len(f) < 2:
                continue
            ref_mod.append((int(f[0], 16) if f[0].strip() else None,
                            int(f[1], 16) if f[1].strip() else None))

    def module_of(ranges, a):
        for i, (s, e) in enumerate(ranges):
            if s is not None and s <= a < e:
                return i
        return None

    differ, placed, tested = [], [], 0
    for start, end, unit, name, kind, status in inventory(args.inventory):
        if args.unit and unit != args.unit:
            continue
        if args.function and name != args.function:
            continue
        size = end - start
        if size < args.min_size or size > args.max_size:
            continue
        sig = rtext[start - rbase:end - rbase]
        r32 = rel32_offsets(args.ref, rtext, rbase, start, end)
        lit = [i for i in range(size)
               if (start + i) not in rrel and i not in r32]
        if len(lit) < 6:
            continue
        tested += 1
        ho = set()
        i = module_of(ref_mod, start)
        if i is not None and i < len(ours_mod):
            ho = find(sig, lit, size, ours_mod[i][0], ours_mod[i][1],
                      otext, obase, orel)
        if ho:
            continue
        g = find(sig, lit, size, obase, obase + len(otext), otext, obase, orel)
        if g and not args.no_modules:
            placed.append((start, size, unit, name, sorted(g)[0]))
        else:
            differ.append((start, size, unit, name, kind, status))

    print(f'reference functions tested : {tested}')
    print(f'differ (no masked match)   : {len(differ)}')
    rows = sorted(differ, key=lambda z: -z[1])
    if args.list:
        rows = rows[:args.list]
    for start, size, unit, name, kind, status in rows:
        print(f'   {start:#010x} {size:6d} {kind:10s} {unit:16s} {name}  [{status}]')
    if placed:
        print(f'placed elsewhere (byte-identical, other module): {len(placed)}')
        prows = sorted(placed, key=lambda z: -z[1])
        if args.list:
            prows = prows[:args.list]
        for start, size, unit, name, at in prows:
            print(f'   {start:#010x} {size:6d} {unit:16s} {name}  -> {at:#x}')
    return 1 if differ else 0


if __name__ == '__main__':
    raise SystemExit(main())
