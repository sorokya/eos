#!/usr/bin/env python3
"""Content coverage between the reference and a linked rebuild, layout-independent.

`compare_asm.py` and the tracking sheet answer "is this function right?".  This
answers the two questions that remain once every function is right but the
image is still the wrong size:

  --missing   reference functions whose bytes appear NOWHERE in our image
              (code the reference links and we do not)
  --surplus   chunks of our image whose bytes appear NOWHERE in the reference
              (code we link and the reference does not)

Both directions mask the bytes that move with the layout -- base-relocation
slots and the 4-byte operand of `call`/`jmp rel32` -- and then search the whole
of the other image, so a function is found wherever the linker happened to put
it.  That is the point: by the time only ordering differs, an address-based
diff is useless and a name-based diff cannot see a missing library module.

IMPORTANT: mask with real instruction boundaries, from objdump, not by scanning
for 0xE8/0xE9 bytes.  A displacement byte that happens to be 0xE8 (`lea
edx,[ebp-0x18]` = 8d 55 e8) desynchronises a byte-scanning walker and makes
byte-exact functions look missing -- `libmatch.masked_pattern` has that bug and
is only safe on the short signatures it uses.

Usage:
    scripts/coverage.py --missing
    scripts/coverage.py --surplus 0x48e65c-0x48f278      # one module's range
    scripts/coverage.py --surplus MAPCONTROL             # by map module name
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
MAP_RE = re.compile(r'\s*0001:([0-9A-F]{8}) ([0-9A-F]{8}) C=CODE\s+S=_TEXT\s+'
                    r'G=\S+\s+M=(\S+)')


class Image:
    """A linked image's .text plus everything needed to mask it."""

    def __init__(self, path):
        self.path = path
        self.pe = PE.from_file(path)
        sec = [s for s in self.pe.sections if s.name == '.text'][0]
        self.text = self.pe.data[sec.raw_pointer:sec.raw_pointer + sec.raw_size]
        self.base = self.pe.header()["image_base"] + sec.virtual_address
        self.relocs = {r + self.pe.header()["image_base"]
                       for r, _t in self.pe.relocations()}
        self.rel32 = set()
        self.insn = {}
        out = subprocess.run(
            ['objdump', '-d', f'--start-address={self.base}', path],
            capture_output=True, text=True).stdout
        for line in out.splitlines():
            m = INSN_RE.match(line)
            if not m:
                continue
            raw = m.group(2).split()
            va = int(m.group(1), 16)
            self.insn[va] = (len(raw), raw[0])
            if len(raw) == 5 and raw[0] in ('e8', 'e9'):
                self.rel32.add(va + 1)

    def end(self):
        """VA just past the last non-zero .text byte."""
        i = len(self.text)
        while i > 0 and self.text[i - 1] == 0:
            i -= 1
        return self.base + i

    def pattern(self, va, sig, minimum=8):
        """Regex for sig, with relocation slots and rel32 operands wildcarded."""
        parts, i, literal = [], 0, 0
        while i < len(sig):
            if va + i in self.relocs or va + i in self.rel32:
                n = min(4, len(sig) - i)        # a slot the range cuts short
                parts.append(b'.{%d}' % n)      # still must not be literal
                i += n
            else:
                parts.append(re.escape(sig[i:i + 1]))
                literal += 1
                i += 1
        if literal < minimum:
            return None
        return re.compile(b''.join(parts), re.DOTALL)

    def slice(self, lo, hi):
        return self.text[lo - self.base:hi - self.base]

    def modules(self):
        """[(va, end, name)] for the rebuild, from `MAP=1 scripts/build.sh`."""
        rows = []
        path = os.path.join(os.path.dirname(self.path), 'GameServer_map.map')
        if not os.path.exists(path):
            return rows
        for line in open(path):
            m = MAP_RE.match(line)
            if not m:
                continue
            name = m.group(3)
            name = name.split('|')[-1] if '|' in name else \
                name.split('\\')[-1].replace('.OBJ', '')
            rows.append((int(m.group(1), 16) + 0x401000, name))
        rows.sort()
        return [(va, rows[i + 1][0] if i + 1 < len(rows) else self.end(), nm)
                for i, (va, nm) in enumerate(rows)]


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
            rows.append((p[ix['unit']], start, end, p[ix['ref_name']],
                         p[ix['kind']], p[ix['status']]))
    return rows


def report_missing(ref, ours, tsv):
    miss, untestable = [], 0
    for unit, start, end, name, kind, status in inventory(tsv):
        if start < ref.base or end > ref.end():
            continue
        sig = ref.slice(start, end)
        if not sig:
            continue
        rx = ref.pattern(start, sig)
        if rx is None:
            untestable += 1
            continue
        if not rx.search(ours.text):
            miss.append((start, end - start, kind, unit, name, status))
    print(f'reference functions with no counterpart anywhere in {ours.path}: '
          f'{len(miss)} ({sum(m[1] for m in miss)} bytes); '
          f'{untestable} too short to test')
    for start, size, kind, unit, name, status in sorted(miss, key=lambda m: -m[1]):
        print(f'  {start:#x} {size:6d} {kind:12s} {unit:18s} {name}  [{status}]')


def split_functions(img, lo, hi):
    """Function starts in [lo,hi): after a ret, skipping 0x90 padding."""
    bounds, va = [lo], lo
    while va < hi:
        length, op = img.insn.get(va, (1, None))
        if op in ('c3', 'c2'):
            nxt = va + length
            while nxt < hi and img.text[nxt - img.base] == 0x90:
                nxt += 1
            if nxt < hi and nxt != bounds[-1]:
                bounds.append(nxt)
            va = nxt
            continue
        va += length or 1
    bounds.append(hi)
    return list(zip(bounds, bounds[1:]))


def report_surplus(ref, ours, lo, hi, label):
    total = 0
    chunks = split_functions(ours, lo, hi)
    for a, b in chunks:
        sig = ours.slice(a, b)
        rx = ours.pattern(a, sig)
        if rx is None or rx.search(ref.text):
            continue
        print(f'  SURPLUS {a:#x} size {b - a:#x} ({b - a})')
        total += b - a
    print(f'{label}: {total:#x} surplus bytes over {len(chunks)} chunks')


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ref', default='GameServer.exe')
    ap.add_argument('--linked', default='build/GameServer.exe')
    ap.add_argument('--inventory', default='analysis/target/functions.tsv')
    ap.add_argument('--missing', action='store_true')
    ap.add_argument('--surplus', metavar='RANGE|MODULE',
                    help='"0xLO-0xHI", a map module name, or "all"')
    args = ap.parse_args()
    if not args.missing and not args.surplus:
        ap.error('pick --missing and/or --surplus')

    ref, ours = Image(args.ref), Image(args.linked)
    print(f'{args.ref}: .text ends {ref.end():#x};  '
          f'{args.linked}: .text ends {ours.end():#x}  '
          f'(delta {ours.end() - ref.end():+#x})')
    if args.missing:
        report_missing(ref, ours, args.inventory)
    if args.surplus:
        if '-' in args.surplus:
            lo, hi = (int(v, 16) for v in args.surplus.split('-'))
            report_surplus(ref, ours, lo, hi, args.surplus)
        else:
            want = args.surplus.upper()
            for va, end, name in ours.modules():
                if want == 'ALL' or name.upper() == want:
                    report_surplus(ref, ours, va, end, name)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
