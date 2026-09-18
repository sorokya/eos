#!/usr/bin/env python3
"""Central tracking sheet: every reference function, its unit, and its status.

Reads the per-unit reference inventory (`unitmap.py --units`, which already
follows the module table) and, for each reference function, records what it is
and how far the reconstruction has got.

`kind` classifies the function:
  app      - the original source defined it; we must reproduce it
  comdat   - a compiler-generated template/array helper (vector begin/end, grow,
             insert, RTTI, ...); it appears automatically once the owning type
             is declared and used, so it is not written by hand
  library  - a statically linked RTL/VCL/BDE member (its first bytes appear in
             the Borland .lib blob, relocation slots masked)
  stub     - a module initializer/finalizer stub (its address is a module
             boundary in modules.tsv); not application code
`app` (+ `comdat`) is the "to produce" denominator.

`status` (for app/comdat rows) comes from the compiler's own `-S` listings in
`build/<Unit>.asm`, canonicalised exactly as `compare_asm.py` and
`verify_units.py` do: registers, stack offsets and small constants must match;
addresses, branch targets and relocated symbols do not. A reference range is
also tried truncated at every `ret`/`ret N`, because the inventory's recorded
end is sometimes short of the real epilogue.

  byte-exact     a source function in the unit matches the reference range
  mismatched     the unit has a source function of that hint name but it differs
  unimplemented  the unit has no function of that hint name
  deferred       deliberately not attempted yet (see DEFERRED)
  n/a            library / module stub

Usage:
    track.py                          # write analysis/target/functions.tsv + summary
    track.py --summary                # summary only, to stdout
    track.py --readme README.md       # rewrite the generated block in README.md
    track.py --reclassify             # recompute the cached kind classification
"""

import argparse
import importlib.util
import os
import re
import subprocess
import sys
from bisect import bisect_left
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import unitmap  # noqa: E402  (load_libs / _masked_pattern / _va_to_off)

# A function is a compiler COMDAT if its hint name matches; the owning class's
# declaration makes the compiler emit it, so it is not reconstructed by hand.
COMDAT_PAT = re.compile(
    r"(Vector_|DynArray_|PtrVector_|Iter_Begin|Iter_End|Iter_Front|"
    r"_ComputeInitialCapacity|_GrowCapacity|_InsertSlow|_InitBox|"
    r"NoOpCtor|GetCapacityEnd|^thunk_)")

# Functions deliberately left for later; 'deferred' rather than ordinary work.
DEFERRED = {"Player_HandlePacket": "one 226 KB function, deferred"}

READ_BEGIN = "<!-- BEGIN GENERATED STATUS -->"
READ_END = "<!-- END GENERATED STATUS -->"

# Fallback code extent if the PE has no executable section.
TEXT_LO, TEXT_HI = 0x00401000, 0x0047A000


def load(path):
    spec = importlib.util.spec_from_file_location(
        os.path.basename(path)[:-3], os.path.join(HERE, os.path.basename(path)))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def load_inventory(path):
    rows = []
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 6 and p[0] != "unit":
                rows.append({"unit": p[0], "order": int(p[1]),
                             "start": int(p[2], 16), "end": int(p[3], 16),
                             "size": int(p[4]), "name": p[5]})
    return rows


def load_stub_addrs(path):
    addrs = set()
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 6:
                try:
                    addrs.add(int(p[2], 16))
                except ValueError:
                    pass
    return addrs


def text_range(pe):
    """[lo, hi) of the executable image, from the PE section table."""
    secs = [s for s in pe.sections if s.characteristics & 0x20000000]
    if not secs:
        return TEXT_LO, TEXT_HI
    return (unitmap.IMAGE_BASE + min(s.virtual_address for s in secs),
            unitmap.IMAGE_BASE + max(s.virtual_address + s.virtual_size
                                     for s in secs))


def ref_index(ca, ref_bin, lo, hi):
    """One objdump pass: [(address, per-instruction canonical form)] over .text."""
    txt = subprocess.check_output(
        ["objdump", "-d", "-M", "intel",
         f"--start-address={lo}", f"--stop-address={hi}", ref_bin],
        stderr=subprocess.DEVNULL).decode("latin1")
    seq = []
    for line in txt.split("\n"):
        m = re.match(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+(\S+)\s*(.*)$", line)
        if m:
            seq.append((int(m.group(1), 16), ca.canon(f"{m.group(2)} {m.group(3)}")))
    return seq


def range_forms(seq, addrs, lo, hi, list_canon, truncate):
    """Canonical forms of reference range [lo,hi).

    `truncate=False` returns only the whole range; `truncate=True` also returns
    every `ret`/`ret N` prefix, because a reference function's recorded end is
    sometimes short of its real epilogue (the last function before the EH
    cleanup table, or a Ghidra split).
    """
    i = bisect_left(addrs, lo)
    j = bisect_left(addrs, hi)
    ins = [c for _a, c in seq[i:j]]
    out = [tuple(list_canon(ins))]
    if truncate:
        for k, x in enumerate(ins):
            if x == "ret" or x.startswith("ret "):
                out.append(tuple(list_canon(ins[:k + 1])))
    return out


def our_functions(ca, list_canon, asm_dir, units):
    """unit -> {canonical tuple: name}, plus the raw name list per unit."""
    by_unit = defaultdict(dict)
    names = defaultdict(list)
    for unit in sorted(units):
        path = os.path.join(asm_dir, unit + ".asm")
        if not os.path.exists(path):
            continue
        txt = open(path, encoding="latin1").read()
        for name in re.findall(r"(?m)^(\S+)\s+proc\s+near", txt):
            ins, _calls, _mk = ca.parse_our(path, name)
            if ins is None:
                continue
            names[unit].append(name)
            by_unit[unit].setdefault(tuple(list_canon(ins)), []).append(name)
    return by_unit, names


def classify(rows, stub_addrs, lib_blob, pe):
    """Assign kind to every row (library / comdat / stub / app)."""
    relocs = {rva + unitmap.IMAGE_BASE for rva, _t in pe.relocations()}
    for row in rows:
        if row["start"] in stub_addrs:
            row["kind"] = "stub"
        elif COMDAT_PAT.search(row["name"]):
            row["kind"] = "comdat"
        elif row["name"].startswith("@"):
            row["kind"] = "library"
        else:
            n = min(row["size"], unitmap.SIG_LEN)
            off = unitmap._va_to_off(pe, row["start"]) if n >= 12 else None
            s = pe.data[off:off + n] if off is not None else None
            if s is not None and (s in lib_blob or
                                  (_masked(row["start"], s, relocs, lib_blob))):
                row["kind"] = "library"
            else:
                row["kind"] = "app"


def _masked(addr, sig, relocs, blob):
    """Reloc-masked lib-blob search (the same test unitmap uses per module).

    Signature classification is conservative on purpose: a `call rel32` to
    another function in the same section carries no base relocation, so masking
    only relocation slots misses some short library wrappers. Masking relative
    call operands too was tried and rejected -- it also matched application
    thunks (e.g. MapItem's constructor), i.e. it would hide real work. A false
    "app" only overstates the remaining work; a false "library" would hide it.
    """
    pat = unitmap._masked_pattern(addr, sig, relocs)
    return bool(pat is not None and pat.search(blob))


def load_kinds(path):
    kinds = {}
    if os.path.exists(path):
        with open(path, encoding="utf-8") as fh:
            next(fh, None)
            for line in fh:
                p = line.rstrip("\n").split("\t")
                if len(p) >= 3:
                    kinds[(p[0], int(p[1], 16))] = p[2]
    return kinds


def save_kinds(rows, path):
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("unit\tstart\tkind\n")
        for r in sorted(rows, key=lambda r: (r["unit"], r["start"])):
            fh.write(f"{r['unit']}\t{r['start']:#010x}\t{r['kind']}\n")


HEAD = ("unit", "order", "start", "end", "size", "ref_name", "kind",
        "source_name", "status", "note")


def write_tsv(rows, path):
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\t".join(HEAD) + "\n")
        for r in sorted(rows, key=lambda r: (r["order"], r["start"])):
            fh.write("\t".join((
                r["unit"], str(r["order"]), f"{r['start']:#010x}",
                f"{r['end']:#010x}", str(r["size"]), r["name"], r["kind"],
                r.get("source_name", ""), r["status"],
                DEFERRED.get(r["name"], ""))) + "\n")


def summary(rows):
    st = defaultdict(int)
    for r in rows:
        if r["kind"] in ("app", "comdat"):
            st[r["status"]] += 1
    app = sum(1 for r in rows if r["kind"] == "app")
    com = sum(1 for r in rows if r["kind"] == "comdat")
    done, total = st["byte-exact"], app + com
    pct = (100.0 * done / total) if total else 0.0
    lines = [f"application functions : {app}",
             f"compiler COMDATs      : {com}",
             f"library members       : {sum(1 for r in rows if r['kind']=='library')}",
             f"module stubs          : {sum(1 for r in rows if r['kind']=='stub')}",
             f"byte-exact            : {done}/{total} ({pct:.1f}% of app+comdat)"]
    for k in ("mismatched", "unimplemented", "deferred"):
        if st[k]:
            lines.append(f"  {k:20}: {st[k]}")
    return lines


def readme_block(rows):
    st = defaultdict(int)
    per = defaultdict(lambda: defaultdict(int))
    for r in rows:
        if r["kind"] not in ("app", "comdat"):
            continue
        st[r["status"]] += 1
        per[r["unit"]][r["status"]] += 1
    done, total = st["byte-exact"], sum(st.values())
    pct = (100.0 * done / total) if total else 0.0
    lib = sum(1 for r in rows if r["kind"] == "library")
    out = [READ_BEGIN, "",
           f"**{done}/{total} ({pct:.1f}%)** application functions byte-exact "
           f"({total} app + compiler COMDATs; {lib} library members excluded).",
           "", "```mermaid", "pie showData",
           "    title Application functions by status"]
    for k in ("byte-exact", "mismatched", "unimplemented", "deferred"):
        if st[k]:
            out.append(f'    "{k}" : {st[k]}')
    out += ["```", "",
            "| Unit | functions | byte-exact | mismatched | unimplemented |",
            "| --- | ---: | ---: | ---: | ---: |"]
    for unit in sorted(per, key=lambda u: (-sum(per[u].values()), u)):
        t = sum(per[unit].values())
        out.append(f"| {unit} | {t} | {per[unit]['byte-exact']} | "
                   f"{per[unit]['mismatched']} | {per[unit]['unimplemented']} |")
    out += ["", READ_END]
    return "\n".join(out)


def update_readme(path, block):
    s = open(path, encoding="utf-8").read()
    if READ_BEGIN not in s or READ_END not in s:
        raise SystemExit(f"{path}: generated block markers not found")
    open(path, "w", encoding="utf-8").write(
        s[:s.index(READ_BEGIN)] + block + s[s.index(READ_END) + len(READ_END):])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--inventory", default="analysis/target/unit_functions.tsv")
    ap.add_argument("--modules", default="analysis/target/modules.tsv")
    ap.add_argument("--kinds-cache", default="analysis/target/function_kinds.tsv")
    ap.add_argument("--reclassify", action="store_true")
    ap.add_argument("--ref", default="GameServer.exe")
    ap.add_argument("--asm-dir", default="build")
    ap.add_argument("--libs", default="ref/Borland5/Lib")
    ap.add_argument("--out", default="analysis/target/functions.tsv")
    ap.add_argument("--readme", default="")
    ap.add_argument("--summary", action="store_true")
    args = ap.parse_args()

    ca = load("compare_asm.py")
    vu = load("verify_units.py")
    rows = load_inventory(args.inventory)
    units = {r["unit"] for r in rows}
    stub_addrs = load_stub_addrs(args.modules)

    kinds = {} if args.reclassify else load_kinds(args.kinds_cache)
    fresh = all((r["unit"], r["start"]) in kinds for r in rows)
    pe = load("pe.py").PE.from_file(args.ref)
    if not fresh:
        classify(rows, stub_addrs, unitmap.load_libs(args.libs), pe)
        save_kinds(rows, args.kinds_cache)
    else:
        for r in rows:
            r["kind"] = kinds[(r["unit"], r["start"])]

    by_unit, names = our_functions(ca, vu.canon, args.asm_dir, units)
    ref_pe = load("pe.py").PE.from_file(args.ref)
    seq = ref_index(ca, args.ref, *text_range(ref_pe))
    addrs = [a for a, _c in seq]
    groups = defaultdict(list)
    for r in rows:
        groups[r["unit"]].append(r)
    for unit, group in groups.items():
        group.sort(key=lambda r: r["start"])
        pool = by_unit.get(unit, {})
        used = set()

        def take(r, truncate):
            for cand in range_forms(seq, addrs, r["start"], r["end"],
                                    vu.canon, truncate):
                if not cand:
                    continue
                for name in pool.get(cand, ()):
                    if name not in used:
                        used.add(name)
                        r["source_name"] = name
                        return True
            return False

        # Each source function corresponds to exactly one reference function, so
        # a match consumes it. Whole-range matches are taken first, then the
        # truncated forms (only one source function can stand behind a range).
        pending = [r for r in group if r["kind"] not in ("library", "stub")]
        for r in pending:
            if take(r, truncate=False):
                r["status"] = "byte-exact"
        for r in pending:
            if r.get("status") == "byte-exact":
                continue
            if take(r, truncate=True):
                r["status"] = "byte-exact"
            else:
                probe = r["name"].lower()
                same = [n for n in names.get(unit, []) if probe in n.lower()]
                if r["name"] in DEFERRED:
                    r["status"], r["source_name"] = "deferred", ""
                elif same:
                    r["status"], r["source_name"] = "mismatched", sorted(same)[0]
                else:
                    r["status"], r["source_name"] = "unimplemented", ""
        for r in group:
            if r["kind"] in ("library", "stub"):
                r["status"], r["source_name"] = "n/a", ""

    write_tsv(rows, args.out)
    print("\n".join(summary(rows)), file=sys.stderr)
    if args.readme:
        update_readme(args.readme, readme_block(rows))
    if args.summary:
        print("\n".join(summary(rows)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
