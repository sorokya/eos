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
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import unitmap  # noqa: E402  (load_libs / _masked_pattern / _va_to_off)
import libmatch  # noqa: E402  (linked-build library matching)

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


def load_stub_registry(path):
    """(unit, start) of functions that have a generated placeholder."""
    seen = set()
    if os.path.exists(path):
        with open(path, encoding="utf-8") as fh:
            next(fh, None)
            for line in fh:
                p = line.rstrip("\n").split("\t")
                if len(p) >= 2:
                    try:
                        seen.add((p[0], int(p[1], 16)))
                    except ValueError:
                        pass
    return seen


def load_stub_addrs(path):
    """Every module boundary: the Initialize stub and its Finalize at +0x10."""
    addrs = set()
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 6:
                try:
                    addrs.add(int(p[2], 16))
                    addrs.add(int(p[2], 16) + 0x10)
                except ValueError:
                    pass
    return addrs


def parse_rows(ca, ref_bin, rows, jobs):
    """Parse each row's reference range with objdump, in parallel.

    Per range, not one sweep over the image: a sweep desynchronises on data
    embedded in .text and decodes the wrong boundaries (the same trap
    verify_units avoids).
    """
    work = [(r["_lo"], r["_hi"]) for r in rows]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(lambda se: ca.parse_ref(ref_bin, se[0], se[1])[0], work))
    for r, ins in zip(rows, out):
        r["_ins"] = ins


def range_forms(ins, list_canon, truncate):
    """Canonical forms of reference range [lo,hi).

    `truncate=False` returns only the whole range; `truncate=True` also returns
    every `ret`/`ret N` prefix, because a reference function's recorded end is
    sometimes short of its real epilogue (the last function before the EH
    cleanup table, or a Ghidra split).
    """
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
            if "_Stub" in name:
                continue          # placeholders from scripts/genstubs.py
            ins, _calls, _mk = ca.parse_our(path, name)
            if ins is None:
                continue
            names[unit].append(name)
            by_unit[unit].setdefault(tuple(list_canon(ins)), []).append(name)
    return by_unit, names


def classify(rows, stub_addrs, pe, libs_dir, linked=None, map_path=None):
    """Assign kind to every row (library / comdat / stub / app).

    The library test matches the reference function against the LINKED build's
    library modules (libmatch): those members are the same code compiled by the
    same toolchain, so a hit is exact and comes with a member name. It covers the
    members the current build pulls in; a member our (still incomplete) units do
    not reference yet will show as `app` -- the safe direction, since that only
    overstates the remaining work. When no linked build is available it falls
    back to the raw `.lib` blob signature, which is complete but noisier.
    """
    relocs = {rva + unitmap.IMAGE_BASE for rva, _t in pe.relocations()}
    blob = spans = None
    if linked is not None and map_path and os.path.exists(map_path):
        blob, spans = libmatch.library_block(linked, map_path)
    lib_blob = None
    for row in rows:
        if row["start"] in stub_addrs:
            row["kind"] = "stub"
        elif COMDAT_PAT.search(row["name"]):
            row["kind"] = "comdat"
        elif row["name"].startswith("@"):
            row["kind"] = "library"
        else:
            # Primary: inside a library module of the linked build (exact).
            hit = False
            off = unitmap._va_to_off(pe, row["start"])
            if blob is not None:
                sig = pe.data[off:off + min(row["size"], libmatch.MAX_SIG)]
                pat = libmatch.masked_pattern(row["start"], sig, relocs)
                m = pat.search(blob) if pat else None
                hit = bool(m and libmatch.module_at(spans, m.start()))
                if hit:
                    row["_lib_exact"] = True
            # Fallback: the raw .lib blob, for members the (incomplete) build does
            # not pull in yet. Longer signature: a 16-byte window matches all over
            # a 50 MB blob, and a false "library" would hide real work.
            if not hit and off is not None and row["size"] >= 24:
                if lib_blob is None:
                    lib_blob = unitmap.load_libs(libs_dir)
                sig = pe.data[off:off + min(row["size"], 24)]
                hit = sig in lib_blob or _masked(row["start"], sig, relocs, lib_blob)
            row["kind"] = "library" if hit else "app"


def _extend_library_runs(rows, run=0x1000):
    """Grow library classification from exact hits across a contiguous run.

    Library members are laid out in dense runs, and a hit inside a named library
    module of the linked build is exact. A function within `run` bytes of such a
    hit is library too -- this recovers the members the (still incomplete) build
    does not pull in yet, without the false positives a bare 16-byte signature
    produces in a 50 MB blob. Reproduced functions are restored afterwards.
    """
    by_unit = defaultdict(list)
    for r in rows:
        by_unit[r["unit"]].append(r)
    for group in by_unit.values():
        anchors = sorted(r["start"] for r in group if r.get("_lib_exact"))
        if not anchors:
            continue
        import bisect
        barriers = sorted(r["start"] for r in group if r["status"] == "byte-exact")
        for r in group:
            if r["kind"] in ("stub", "comdat"):
                continue
            if r["status"] == "byte-exact":
                continue
            i = bisect.bisect_left(anchors, r["start"])
            near = min((abs(r["start"] - anchors[j]) for j in (i - 1, i)
                        if 0 <= j < len(anchors)), default=None)
            # A reproduced function is application code: never reach across one.
            blocked = any(abs(r["start"] - b) <= run
                          and min(r["start"], b) < b < max(r["start"], b)
                          for b in barriers)
            if near is not None and near <= run and not blocked:
                r["kind"] = "library"


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
    """(kinds, mode); `mode` records how the verdicts were produced so a run
    with a linked build present does not reuse fallback verdicts."""
    kinds = {}
    mode = ""
    if os.path.exists(path):
        with open(path, encoding="utf-8") as fh:
            for line in fh:
                if line.startswith("#mode="):
                    mode = line.strip()[6:]
                    continue
                p = line.rstrip("\n").split("\t")
                if len(p) >= 3 and p[0] != "unit":
                    try:
                        kinds[(p[0], int(p[1], 16))] = (p[2],
                                                        len(p) > 3 and p[3] == "exact")
                    except ValueError:
                        pass
    return kinds, mode


def save_kinds(rows, path, mode):
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(f"#mode={mode}\n")
        fh.write("unit\tstart\tkind\texact\n")
        for r in sorted(rows, key=lambda r: (r["unit"], r["start"])):
            exact = "exact" if r.get("_lib_exact") else "-"
            fh.write(f"{r['unit']}\t{r['start']:#010x}\t{r['kind']}\t{exact}\n")


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
    for k in ("mismatched", "stubbed", "unimplemented", "deferred"):
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
    for k in ("byte-exact", "mismatched", "stubbed", "unimplemented",
              "deferred"):
        if st[k]:
            out.append(f'    "{k}" : {st[k]}')
    out += ["```", "",
            "| Unit | functions | byte-exact | stubbed | mismatched | unimplemented |",
            "| --- | ---: | ---: | ---: | ---: | ---: |"]
    for unit in sorted(per, key=lambda u: (-sum(per[u].values()), u)):
        t = sum(per[unit].values())
        out.append(f"| {unit} | {t} | {per[unit]['byte-exact']} | "
                   f"{per[unit]['stubbed']} | {per[unit]['mismatched']} | "
                   f"{per[unit]['unimplemented']} |")
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
    ap.add_argument("--stubs", default="analysis/target/stubs.tsv",
                    help="registry written by scripts/genstubs.py")
    ap.add_argument("--reclassify", action="store_true")
    ap.add_argument("--ref", default="GameServer.exe")
    ap.add_argument("--asm-dir", default="build")
    ap.add_argument("--libs", default="ref/Borland5/Lib")
    ap.add_argument("--linked", default="build/GameServer.exe",
                    help="linked build used to identify library members")
    ap.add_argument("--map", default="build/GameServer.map",
                    help="its ilink32 -s map (MAP=1 scripts/build.sh)")
    ap.add_argument("--out", default="analysis/target/functions.tsv")
    ap.add_argument("--readme", default="")
    ap.add_argument("-j", "--jobs", type=int, default=0,
                    help="parallel objdump jobs (0 = CPU count)")
    ap.add_argument("--summary", action="store_true")
    args = ap.parse_args()

    ca = load("compare_asm.py")
    vu = load("verify_units.py")
    rows = load_inventory(args.inventory)
    units = {r["unit"] for r in rows}
    stub_addrs = load_stub_addrs(args.modules)

    linked_ok = os.path.exists(args.linked) and os.path.exists(args.map)
    mode = "linked" if linked_ok else "blob"
    kinds, have = ({}, "") if args.reclassify else load_kinds(args.kinds_cache)
    fresh = have == mode and all((r["unit"], r["start"]) in kinds for r in rows)
    pe = load("pe.py").PE.from_file(args.ref)
    if not fresh:
        linked = load("pe.py").PE.from_file(args.linked) if linked_ok else None
        classify(rows, stub_addrs, pe, args.libs,
                 linked=linked, map_path=args.map)
        save_kinds(rows, args.kinds_cache, mode)
    else:
        for r in rows:
            r["kind"], r["_lib_exact"] = kinds[(r["unit"], r["start"])]

    by_unit, names = our_functions(ca, vu.canon, args.asm_dir, units)
    groups = defaultdict(list)
    for r in rows:
        groups[r["unit"]].append(r)
    # A row's range ends at the next row of the same unit (any kind), which keeps
    # it tight when library members inside the span are excluded; the last one
    # uses its recorded end.
    for _u, group in groups.items():
        group.sort(key=lambda r: r["start"])
        for i, r in enumerate(group):
            r["_lo"] = r["start"]
            r["_hi"] = group[i + 1]["start"] if i + 1 < len(group) else r["end"]
    parse_rows(ca, args.ref, rows, args.jobs or (os.cpu_count() or 4))
    for unit, group in groups.items():
        group.sort(key=lambda r: r["start"])
        pool = by_unit.get(unit, {})
        used = set()

        def take(r, truncate, nxt=None):
            forms = range_forms(r["_ins"], vu.canon, truncate)
            # Ghidra sometimes splits a function after its prologue; a short
            # ret-less fragment followed by its successor is one function.
            if (nxt is not None and "ret" not in r["_ins"]
                    and len(r["_ins"]) <= 6 and nxt["_lo"] == r["_hi"]):
                forms.append(tuple(vu.canon(r["_ins"] + nxt["_ins"])))
            for cand in forms:
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
        def next_of(r):
            i = group.index(r)
            return group[i + 1] if i + 1 < len(group) else None

        pending = [r for r in group if r["kind"] not in ("library", "stub")]
        for r in pending:
            if take(r, truncate=False, nxt=next_of(r)):
                r["status"] = "byte-exact"
        for r in pending:
            if r.get("status") == "byte-exact":
                continue
            if take(r, truncate=True, nxt=next_of(r)):
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
        # A row the heuristic called `library` may still be application code we
        # have reproduced (the smoother can pull a function at the edge of a
        # library run across the line). Give the leftovers to those rows; a match
        # is proof of application code, so it flips the kind back.
        for r in group:
            if r["kind"] == "library":
                if take(r, truncate=False) or take(r, truncate=True):
                    r["status"], r["kind"] = "byte-exact", "app"
                else:
                    r["status"], r["source_name"] = "n/a", ""
        for r in group:
            if r["kind"] == "stub":
                r["status"], r["source_name"] = "n/a", ""

    _extend_library_runs(rows)

    # A generated placeholder is not a reconstruction, but it is more than
    # "nothing yet": report it as `stubbed` so the sheet distinguishes the two.
    stubbed = load_stub_registry(args.stubs)
    for r in rows:
        if (r["unit"], r["start"]) in stubbed and r["status"] in (
                "unimplemented", "mismatched"):
            r["status"] = "stubbed"

    # A function we have reproduced is application code by definition; never let
    # the (heuristic) library vote hide it.
    for r in rows:
        if r["status"] == "byte-exact":
            r["kind"] = "app" if r["kind"] != "comdat" else "comdat"

    write_tsv(rows, args.out)
    print("\n".join(summary(rows)), file=sys.stderr)
    if args.readme:
        update_readme(args.readme, readme_block(rows))
    if args.summary:
        print("\n".join(summary(rows)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
