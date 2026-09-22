#!/usr/bin/env python3
"""Predict the package-init order ilink32 writes into _INIT_/_EXIT_, and diff it.

The `.data` tables at 0x55b206/0x55b524 list each unit's Initialize/Finalize in
package-init order (6-byte entries `{0, 0x1e, addr}`). ilink32 derives that
order; the rule, verified against our link and synthetic probes, is:

  * a post-order DFS over units, roots in link order, children visited in
    link order;
  * `u` depends on `v` when a fixup in *any* COMDAT copy `u` carries (kept or
    discarded) targets a symbol whose first definer in link order is `v`;
    fixups to type descriptors (`@$xt$...`) do not count.

The catch is that bcc32 names EH tables `@_$DC<n>$`, `@_$ECT<n>$`, `@_$CH<n>$`
with a per-translation-unit counter, so two units' copies of the same inline
function's tables are different symbols unless their counters happen to agree
-- and any EH-bearing function added to or removed from a unit (even one the
linker drops) renames every later table in it. `--why A B` prints the fixups
that make unit A depend on unit B.

Usage:
    scripts/initorder.py                 # model vs our image vs the reference
    scripts/initorder.py --why Filecache Learnvalues
"""
import argparse
import os
import struct
import sys


def _idx(b, j):
    v = b[j]
    return ((((v & 0x7f) << 8) | b[j + 1]), j + 2) if v & 0x80 else (v, j + 1)


def parse_obj(path):
    """(comdef names, {(kind, source): [target names]}) for one Borland OMF object."""
    d = open(path, 'rb').read()
    i, names, comdefs, segs, lnames, fix, cur = 0, [''], [None], [''], [''], {}, None
    while i < len(d):
        t = d[i]
        ln = struct.unpack_from('<H', d, i + 1)[0]
        body = d[i + 3:i + 3 + ln - 1]
        i += 3 + ln
        if t == 0x96:                                   # LNAMES
            j = 0
            while j < len(body):
                n = body[j]
                lnames.append(body[j + 1:j + 1 + n].decode('latin1'))
                j += 1 + n
        elif t in (0x98, 0x99):                         # SEGDEF
            si, _ = _idx(body, 1 + (4 if t == 0x99 else 2))
            segs.append(lnames[si])
        elif t in (0x8c, 0xb0, 0xb4):                   # EXTDEF / COMDEF / LEXTDEF
            j = 0
            while j < len(body):
                n = body[j]
                nm = body[j + 1:j + 1 + n].decode('latin1')
                j += 1 + n
                _, j = _idx(body, j)
                if t == 0xb0:
                    j += 1
                    v = body[j]
                    j += 1 if v <= 0x80 else {0x81: 3, 0x84: 4, 0x88: 5}[v]
                    comdefs.append(nm)
                names.append(nm)
        elif t in (0xa0, 0xa1):                         # LEDATA: remember the source
            si, _ = _idx(body, 0)
            cur = ('C', comdefs[si - 0x4000]) if si >= 0x4000 else \
                  ('S', segs[si] if si < len(segs) else str(si))
        elif t in (0x9c, 0x9d):                         # FIXUPP
            j = 0
            while j < len(body):
                b0 = body[j]
                if not b0 & 0x80:                       # THREAD subrecord
                    method, dbit = (b0 >> 2) & 7, b0 & 0x40
                    j += 1
                    if (method < 4 or not dbit) and method in (0, 1, 2):
                        _, j = _idx(body, j)
                    continue
                j += 2
                fd = body[j]
                j += 1
                if not fd & 0x80 and ((fd >> 4) & 7) < 4:
                    _, j = _idx(body, j)
                tgt = None
                if not fd & 0x08:
                    ti, j = _idx(body, j)
                    if fd & 3 == 2 and ti < len(names):
                        tgt = names[ti]
                    elif fd & 3 == 0 and ti >= 0x4000:
                        tgt = comdefs[ti - 0x4000]
                if not fd & 0x04:
                    j += 4 if t == 0x9d else 2
                if tgt is not None and cur is not None:
                    fix.setdefault(cur, []).append(tgt)
    return comdefs[1:], fix


def link_order(units_tsv):
    rows = [l.split('\t')[1] for l in open(units_tsv).read().split('\n')[1:] if l]
    return ['GameServer'] + [u for u in rows if u != 'GUI']


def model(order, objs):
    pos = {u: i for i, u in enumerate(order)}
    owner = {}
    for u in order:
        for c in objs[u][0]:
            owner.setdefault(c, u)

    def deps(u):
        out = {}
        for (kind, src), targets in objs[u][1].items():
            if kind == 'S' and src.startswith('$$'):
                continue
            for t in targets:
                if t.startswith('@$xt$'):
                    continue
                v = owner.get(t)
                if v and v != u:
                    out.setdefault(v, []).append((src, t))
        return out

    seq, seen = [], set()

    def dfs(u):
        if u in seen:
            return
        seen.add(u)
        for v in sorted(deps(u), key=pos.get):
            dfs(v)
        seq.append(u)

    sys.setrecursionlimit(10000)
    for r in order:
        dfs(r)
    return [u for u in seq if u != 'GameServer'], deps


def image_order(path):
    """Unit names in _INIT_ order, read from the image via its export table."""
    b = open(path, 'rb').read()
    e = struct.unpack_from('<I', b, 0x3c)[0]
    n = struct.unpack_from('<H', b, e + 6)[0]
    opt = struct.unpack_from('<H', b, e + 20)[0]
    off, secs = e + 24 + opt, {}
    for _ in range(n):
        nm = b[off:off + 8].rstrip(b'\0').decode()
        secs[nm] = struct.unpack_from('<IIII', b, off + 8)[1:]
        off += 40
    erva = struct.unpack_from('<I', b, e + 24 + 96)[0]
    va, _rs, ro = secs['.edata']
    o = erva - va + ro
    nnames = struct.unpack_from('<I', b, o + 24)[0]
    af, nf, of = struct.unpack_from('<III', b, o + 28)
    init = {}
    for k in range(nnames):
        np_ = struct.unpack_from('<I', b, nf - va + ro + 4 * k)[0]
        ordn = struct.unpack_from('<H', b, of - va + ro + 2 * k)[0]
        fa = struct.unpack_from('<I', b, af - va + ro + 4 * ordn)[0]
        nm = b[np_ - va + ro:].split(b'\0')[0].decode()
        if nm.endswith('@Initialize'):
            init[0x400000 + fa] = nm[2:-len('@Initialize')]
    dva, drs, dro = secs['.data']
    data = b[dro:dro + min(drs, 0x2000)]
    out, j = [], 0
    while True:
        j = data.find(b'\x00\x1e', j)
        if j < 0 or j + 6 > len(data):
            break
        a = struct.unpack_from('<I', data, j + 2)[0]
        if a in init:
            out.append(init[a])
            j += 6
        else:
            j += 1
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--units', default='analysis/target/units.tsv')
    ap.add_argument('--obj', default='build/obj')
    ap.add_argument('--ref', default='GameServer.exe')
    ap.add_argument('--ours', default='build/GameServer.exe')
    ap.add_argument('--why', nargs=2, metavar=('UNIT', 'DEP'))
    args = ap.parse_args()
    order = link_order(args.units)
    files = {f[:-4].lower(): f for f in os.listdir(args.obj) if f.lower().endswith('.obj')}
    objs = {u: parse_obj(os.path.join(args.obj, files[u.lower()])) for u in order}
    predicted, deps = model(order, objs)
    if args.why:
        for src, t in deps(args.why[0]).get(args.why[1], []):
            print(f'{src}  ->  {t}')
        return 0
    ref = image_order(args.ref)
    ours = image_order(args.ours) if os.path.exists(args.ours) else None
    print('model == our image' if predicted == ours else 'model != our image (objects newer than the link?)')
    if predicted == ref:
        print('model == reference: the init order matches')
        return 0
    k = next(i for i, (x, y) in enumerate(zip(predicted, ref)) if x != y)
    print(f'first difference at {k}:')
    print('  model    :', ' '.join(predicted[max(0, k - 2):k + 6]))
    print('  reference:', ' '.join(ref[max(0, k - 2):k + 6]))
    return 1


if __name__ == '__main__':
    raise SystemExit(main())
