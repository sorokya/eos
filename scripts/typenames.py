#!/usr/bin/env python3
"""The RTTI type-name oracle: every class name the two images spell out.

bcc32 writes the **spelled** type name into each `__tpdsc__` it emits, and those
descriptors live inside `.text`, between functions.  The reference is stripped
of symbols, but not of these -- so every class, pointer, reference, array and
container type the original source named is readable verbatim, and a
reconstructed name that differs is both a byte difference and (because the
descriptor's length tracks the name's) a size difference.

Detection.  A descriptor's name is a NUL-terminated ASCII run whose header ends
in a *relocated* dword: the base-type pointer sits 4 bytes before the name for
the simple forms (pointer/reference/`vector<...>`) and 8 bytes before it for the
class and array forms.  Requiring a base relocation at exactly -4 or -8, plus a
strict C++ type-name shape, separates real descriptors from the printable runs
that fall out of ordinary code bytes.  Runs shorter than four characters are
dropped outright: every one of them is code noise (the shortest real name in
either image is `TGUI`), and they are numerous enough in the Delphi library
modules to bury the signal.  Two four-character runs survive in the library tail
of each image and are reported as leftovers; they are noise as well.

Output is one row per distinct name: the module that holds it on each side, and
a verdict -- `ok` (both), `REF-ONLY` (a name the reconstruction gets wrong or is
missing) or `OURS-ONLY` (a name we invented).  Pair the two leftover lists and
each pair names a type the reconstruction should rename; that is how
`Logins::ReservedName`/`Logins::LoginEntry` were shown to be `Asocketvip` and
`Asocketblock`, and `Server` to be `Packets`.

    scripts/typenames.py                  # summary + the two leftover lists
    scripts/typenames.py --all            # every name, both sides
    scripts/typenames.py -o analysis/target/typenames.tsv
"""
import argparse
import collections
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from pe import PE  # noqa: E402

RAW = re.compile(rb'[\x20-\x7e]{4,}\x00')
TYPE = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*'
                  r'(?:::[A-Za-z_][A-Za-z0-9_]*)*'
                  r'(?:<[^\x00]*>)?'
                  r'(?: [*&]+)?'
                  r'(?:\[\d+\])?$')
# Where the base-type pointer sits relative to the name: 4 for the simple
# forms, 8 for the class/array forms.
HEADER_OFFSETS = (4, 8)


def load(path):
    pe = PE.from_file(path)
    sec = [s for s in pe.sections if s.name == '.text'][0]
    base = pe.header()['image_base']
    text = pe.data[sec.raw_pointer:sec.raw_pointer + sec.raw_size]
    relocs = {base + r for r, _t in pe.relocations()}
    return text, base + sec.virtual_address, relocs


def descriptors(path):
    """[(va, name)] for every type descriptor in `.text`."""
    text, base, relocs = load(path)
    out = []
    for m in RAW.finditer(text):
        name = m.group()[:-1].decode('latin1')
        va = base + m.start()
        if not TYPE.match(name):
            continue
        if not any((va - d) in relocs for d in HEADER_OFFSETS):
            continue
        out.append((va, name))
    return out


def ref_modules(path):
    rows = []
    for line in open(path, encoding='utf-8').read().splitlines()[1:]:
        f = line.split('\t')
        if len(f) < 6:
            continue
        rows.append((int(f[0], 16) if f[0].strip() else None,
                     int(f[1], 16) if f[1].strip() else None, f[4]))
    return rows


def our_modules(exe):
    out = subprocess.run([sys.executable, os.path.join(HERE, 'unitmap.py'),
                          '--pe', exe, '--stubs'], capture_output=True, text=True)
    rows = []
    for line in out.stdout.splitlines()[1:]:
        f = line.split('\t')
        if len(f) < 2:
            continue
        rows.append((int(f[0], 16) if f[0].strip() else None,
                     int(f[1], 16) if f[1].strip() else None, ''))
    return rows


def locate(mods, ref_names, va):
    for i, (s, e, _n) in enumerate(mods):
        if s is not None and s <= va < e:
            n = ref_names[i] if i < len(ref_names) else ''
            return n or f'#{i}'
    return 'library-tail'


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ref', default='GameServer.exe')
    ap.add_argument('--linked', default='build/GameServer.exe')
    ap.add_argument('--modules', default='analysis/target/modules.tsv')
    ap.add_argument('--all', action='store_true', help='print every name, not just the leftovers')
    ap.add_argument('-o', '--output', help='write the full table as TSV')
    args = ap.parse_args()

    rmods = ref_modules(args.modules)
    rnames = [m[2] for m in rmods]
    omods = our_modules(args.linked)

    ref = descriptors(args.ref)
    our = descriptors(args.linked)
    ref_by = collections.defaultdict(list)
    our_by = collections.defaultdict(list)
    for va, n in ref:
        ref_by[n].append(locate(rmods, rnames, va))
    for va, n in our:
        our_by[n].append(locate(omods, rnames, va))

    rows = []
    for n in sorted(set(ref_by) | set(our_by), key=lambda s: (s.lower(), s)):
        r, o = ref_by.get(n), our_by.get(n)
        verdict = 'ok' if r and o else ('REF-ONLY' if r else 'OURS-ONLY')
        rows.append((n, ','.join(sorted(set(r))) if r else '',
                     ','.join(sorted(set(o))) if o else '', verdict))

    print(f'type descriptors: reference {len(ref)} ({len(ref_by)} distinct), '
          f'rebuild {len(our)} ({len(our_by)} distinct)')
    only_ref = [x for x in rows if x[3] == 'REF-ONLY']
    only_our = [x for x in rows if x[3] == 'OURS-ONLY']
    print(f'names only in the reference : {len(only_ref)}')
    print(f'names only in the rebuild   : {len(only_our)}\n')

    def dump(title, sel):
        if not sel:
            return
        print(title)
        for n, r, o, _v in sel:
            print(f'   {n:<52} ref={r or "-":<22} ours={o or "-"}')
        print()

    if args.all:
        dump('=== all type names ===', rows)
    else:
        dump('=== in the reference, not in the rebuild ===', only_ref)
        dump('=== in the rebuild, not in the reference ===', only_our)

    if args.output:
        os.makedirs(os.path.dirname(args.output) or '.', exist_ok=True)
        with open(args.output, 'w', encoding='utf-8') as fh:
            fh.write('name\tref_module\tour_module\tverdict\n')
            for n, r, o, v in rows:
                fh.write(f'{n}\t{r}\t{o}\t{v}\n')
        print(f'wrote {args.output} ({len(rows)} rows)')
    return 1 if (only_ref or only_our) else 0


if __name__ == '__main__':
    raise SystemExit(main())
