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
  merged   - the ret-less body half of a function Ghidra split after its
             prologue; the head row's source function already spans its bytes,
             so it is not a separate target
  comdat_cross - a compiler-emitted template COMDAT whose function is emitted
             by a different translation unit than the module range that
             contains the row (proven by the reference call graph); excluded
             from the hand-written denominator like `library`
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
# `^thunk_` is deliberately NOT here: Ghidra names library forwarder thunks
# `thunk_FUN_*`, and testing COMDAT before the library classification trapped
# them as `comdat` (in the denominator) before the library test could run.
# Without it they reach the library test / run extension and become `library`.
COMDAT_PAT = re.compile(
    r"(Vector_|DynArray_|PtrVector_|Iter_Begin|Iter_End|Iter_Front|"
    r"_ComputeInitialCapacity|_GrowCapacity|_InsertSlow|_InitBox|"
    r"NoOpCtor|GetCapacityEnd)")

# Functions deliberately left for later; 'deferred' rather than ordinary work.
# Player_HandlePacket was deferred until it was investigated; reconnaissance then
# proved it tractable (one `ret`, no internal call targets, 43 chunks) and it is now
# being written, so it must report `mismatched` like any other in-progress body
# rather than hiding 33.9% of the application's bytes behind a label meaning "not
# attempted".
DEFERRED = {}

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


def load_stub_registry(path, src="src"):
    """(unit, start) of functions that have a generated placeholder.

    The registry is a cache of the `*_Stub` definitions actually present in the
    unit sources; a row whose placeholder has since been deleted (because the
    function converged) must not keep reporting the function as `stubbed` -- that
    hides a `mismatched` body behind a stub. Validate each row against its unit
    source before trusting it, so a stale cache cannot mask real state.
    """
    seen = set()
    if not os.path.exists(path):
        return seen
    sources = {}

    def text(unit):
        if unit not in sources:
            p = os.path.join(src, unit + ".cpp")
            try:
                with open(p, encoding="utf-8") as fh:
                    sources[unit] = fh.read()
            except OSError:
                sources[unit] = ""
        return sources[unit]

    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) < 2:
                continue
            stub = p[3] if len(p) > 3 else ""
            if stub and stub not in text(p[0]):
                continue
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
    verify_units avoids). The raw (address, instruction) pairs are kept as
    well as the canonical form, so the artifact pass can read a direct `call`
    target without a second objdump sweep.
    """
    work = [(r["_lo"], r["_hi"]) for r in rows]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(lambda se: ca._parse_ref(ref_bin, se[0], se[1]), work))
    for r, (ins, _mk, raw) in zip(rows, out):
        r["_ins"] = ins
        r["_raw"] = raw


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
    """unit -> {canonical tuple: name}, the raw name list per unit, a
    global {canonical tuple: [(unit, name), ...]} pool, and per-function
    provenance.

    bcc32 emits a header-defined function (a template instantiation) once per
    translation unit that uses it, and the reference may place the surviving
    COMDAT copy in a *different* unit's span than the one that emits it now
    (Npcvalues' `vector<NpcDropItem>::clear`/`erase`/`copy` come from
    Npcvalue.cpp). The global pool lets a row fall back to a function another
    unit emits; `take` only uses it when the canonical sequence has exactly one
    candidate there, because the same generic body is emitted for many element
    types (30 copies of `vector<T>::clear`, 7 of `copy<T*>`), and a wrong
    cross-unit match would hide real work.

    `prov[(unit, name)]` is the `?debug T "..."` source file bcc32 recorded for
    the function (the listing carries it beside every `proc`). A function whose
    provenance is under the Borland `Include/` tree is RTL/STL header code, not
    rewritten application source.
    """
    by_unit = defaultdict(dict)
    names = defaultdict(list)
    glob = defaultdict(list)
    prov = {}
    for unit in sorted(units):
        path = os.path.join(asm_dir, unit + ".asm")
        if not os.path.exists(path):
            continue
        txt = open(path, encoding="latin1").read()
        for m in re.finditer(r"(?ms)^(\S+)\s+proc\s+near(.*?)^\S+\s+endp", txt):
            name, body = m.group(1), m.group(2)
            d = re.search(r'\?debug\s+T\s+"([^"]+)"', body)
            if d:
                prov[(unit, name)] = d.group(1)
        for name in re.findall(r"(?m)^(\S+)\s+proc\s+near", txt):
            if "_Stub" in name:
                continue          # placeholders from scripts/genstubs.py
            ins, _calls, _mk = ca.parse_our(path, name)
            if ins is None:
                continue
            names[unit].append(name)
            key = tuple(list_canon(ins))
            by_unit[unit].setdefault(key, []).append(name)
            glob[key].append((unit, name))
    return by_unit, names, glob, prov


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
            if r["kind"] in ("stub", "comdat", "merged"):
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


def _fold_stat(form):
    """Canonical form with bcc32's compiler-static operands (`$name`) folded to
    `ADDR`.

    bcc32 names a function-local static's storage `$name`; the reference
    disassembles the same operand as a resolved numeric address, which `canon`
    already turns into `ADDR`. Folding the symbol form makes the two compare.
    """
    return tuple(re.sub(r"\$[A-Za-z_][A-Za-z0-9_]*", "ADDR", x) for x in form)


def _strip_frame(form):
    """Drop the standard `push ebp`/`mov ebp,esp` prologue and `pop ebp`/`ret`
    epilogue, leaving the body instructions."""
    f = list(form)
    if f[:2] == ["push ebp", "mov ebp,esp"]:
        f = f[2:]
    if f[-2:] == ["pop ebp", "ret"]:
        f = f[:-2]
    return f


def _is_compiler_template(sym):
    return sym.startswith("@@std@") or sym.startswith("@@__rwstd@")


def _classify_artifacts(rows, pe, vu, by_unit, glob_pool, prov, used):
    """Exclude compiler/RTL-generated rows that are not hand-written targets.

    Three rules, each resting on a proof rather than an address list:

    * **RTL data-sentinel accessor** (`library`). A row whose body is exactly
      `mov eax,<data address>` and nothing else -- a leaf constant-address
      getter -- where the loaded address is a base-relocated slot in a data
      section and *every* other reference to that data object lies outside the
      per-unit function inventory (i.e. in library code). The Borland RTL's
      `basic_string::__getNullRep` (`ref/Borland5/Include/string.stl:751`,
      static member defined in
      `ref/Borland5/Source/RTL/source/stl/string/string.cpp:41`) is exactly this
      shape: it returns `&__nullref`, a sentinel referenced only from the
      trailing RTL region. `Packets` `FUN_00450110` is that accessor.

    * **Borland-header local-static guard** (`library`). A row whose body opens
      with the bcc32 function-local-`static` initialisation guard
      (`cmp byte ptr [flag],0` / conditional jump / ... / `inc byte ptr [flag]`)
      and whose canonical form (compiler-static operands folded) matches a
      function our build emits from a file under the Borland `Include/` tree.
      The compiler emits the guard for a `static` local; the row has no
      standalone source definition. `Packets` `FUN_00471e54` is the guard inside
      `std::deque<char>::__buffer_size()` (`Include/deque.cc`).

    * **Cross-unit compiler COMDAT** (`comdat_cross`). A row whose body is a
      generic template instantiation -- every candidate that emits it across
      the tree is a `@@std@`/`@@__rwstd@` symbol -- whose own unit emits no free
      candidate, and which is reached only from functions already reproduced
      (`byte-exact`) or from other rows excluded by this rule. This is the
      `Npcvalue` unit's `vector<NpcDropItem>::clear`/`::erase` and
      `std::copy<NpcDropItem*>`, which the reference placed inside the
      `Npcvalues` module range; the owning unit's own copies are counted under
      `Npcvalue`. The call-graph restriction is what makes the match sound where
      the bare canonical sequence is shared by many element types.
    """
    ranges = sorted((r["start"], r["end"]) for r in rows)

    def in_inventory(va):
        import bisect
        i = bisect.bisect_right([s for s, _e in ranges], va) - 1
        return i >= 0 and ranges[i][0] <= va < ranges[i][1]

    data_spans = []
    for s in pe.sections:
        if s.name in (".data", ".rdata", ".bss", ".tls"):
            lo = unitmap.IMAGE_BASE + s.virtual_address
            data_spans.append((lo, lo + s.virtual_size))

    def in_data(va):
        return any(lo <= va < hi for lo, hi in data_spans)

    refs_by_target = defaultdict(list)
    for rva, _t in pe.relocations():
        off = unitmap._va_to_off(pe, rva + unitmap.IMAGE_BASE)
        if off is None or off + 4 > len(pe.data):
            continue
        target = int.from_bytes(pe.data[off:off + 4], "little")
        refs_by_target[target].append(rva + unitmap.IMAGE_BASE)

    caller_map = defaultdict(list)
    for r in rows:
        for _a, ins in r.get("_raw", ()):
            m = re.match(r"call\s+0x([0-9a-f]+)", ins)
            if m:
                caller_map[int(m.group(1), 16)].append(r)

    folded_pool = defaultdict(list)
    for unit, d in by_unit.items():
        for form, names in d.items():
            ff = _fold_stat(form)
            for n in names:
                folded_pool[ff].append((unit, n, prov.get((unit, n), "")))

    unmatched = [r for r in rows if r["kind"] == "app"
                 and not r.get("source_name")
                 and r["status"] in ("unimplemented", "stubbed", "mismatched")]

    def exclude(r, kind):
        r["kind"], r["status"], r["source_name"] = kind, "n/a", ""

    for r in unmatched:
        form = tuple(vu.canon(r["_ins"]))

        # Rule 1: RTL data-sentinel accessor.
        if _strip_frame(form) == ["mov eax,ADDR"]:
            target = None
            for _a, ins in r.get("_raw", ()):
                m = re.match(r"mov\s+eax,\s*0x([0-9a-f]+)$", ins)
                if m:
                    target = int(m.group(1), 16)
            if (target is not None and in_data(target)
                    and refs_by_target.get(target)):
                outside = [v for v in refs_by_target[target]
                           if not (r["start"] <= v < r["end"])]
                if outside and all(not in_inventory(v) for v in outside):
                    exclude(r, "library")
                    continue

        # Rule 2: Borland-header local-static guard.
        if (form[:2] == ("push ebp", "mov ebp,esp") and form[2:3]
                and form[2].startswith("add esp,") and form[3:4] == ("cmp[ADDR],0",)
                and form[4:5] and form[4].startswith("j")
                and any(x.startswith("inc[") for x in form)):
            for _unit, name, path in folded_pool.get(_fold_stat(form), ()):
                if _is_compiler_template(name) and "Include" in path:
                    exclude(r, "library")
                    break
            if r["kind"] != "app":
                continue

    # Rule 3: cross-unit compiler COMDAT (fixed point so a cluster classifies in
    # call order).
    classified = set()
    changed = True
    while changed:
        changed = False
        for r in unmatched:
            if id(r) in classified or r["kind"] != "app":
                continue
            form = tuple(vu.canon(r["_ins"]))
            cands = glob_pool.get(form, ())
            if len(cands) < 2 or not all(
                    _is_compiler_template(n) for _u, n in cands):
                continue
            if any((r["unit"], n) not in used
                   for n in by_unit.get(r["unit"], {}).get(form, ())):
                continue          # the containing unit emits it: ordinary work
            callers = caller_map.get(r["start"], ())
            if not callers:
                continue
            if all(c["status"] == "byte-exact" or id(c) in classified
                   for c in callers):
                classified.add(id(r))
                changed = True
    for r in unmatched:
        if id(r) in classified:
            exclude(r, "comdat_cross")


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


def refresh_cached_kinds(rows, kinds):
    """Re-derive cached verdicts that depended on COMDAT_PAT.

    The kinds cache stores `classify`'s output. When COMDAT_PAT changes -- here
    dropping `^thunk_` so Ghidra-named library forwarders reach the library
    test -- every cached `comdat` whose name no longer matches is stale. Marking
    it `app` lets the normal pipeline re-derive it: the group loop tries to
    match it, then `_extend_library_runs` absorbs it into the surrounding exact
    library run (a contiguity proof), so the cache is corrected without
    rebuilding it from a possibly different linked image.
    """
    stale = 0
    for r in rows:
        kind, exact = kinds[(r["unit"], r["start"])]
        if kind == "comdat" and not COMDAT_PAT.search(r["name"]):
            kind, exact = "app", False
            stale += 1
        r["kind"], r["_lib_exact"] = kind, exact
    return stale


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
    by = defaultdict(int)
    for r in rows:
        if r["kind"] in ("app", "comdat"):
            st[r["status"]] += 1
            by[r["status"]] += int(r.get("size") or 0)
    app = sum(1 for r in rows if r["kind"] == "app")
    com = sum(1 for r in rows if r["kind"] == "comdat")
    done, total = st["byte-exact"], app + com
    pct = (100.0 * done / total) if total else 0.0
    # Function counts are misleading: the deferred Player_HandlePacket alone is a
    # third of the application's bytes. Report byte coverage too.
    btot = sum(by.values())
    bdone = by["byte-exact"]
    bpct = (100.0 * bdone / btot) if btot else 0.0
    lines = [f"application functions : {app}",
             f"compiler COMDATs      : {com}",
             f"library members       : {sum(1 for r in rows if r['kind']=='library')}",
             f"module stubs          : {sum(1 for r in rows if r['kind']=='stub')}",
             f"cross-unit COMDATs    : {sum(1 for r in rows if r['kind']=='comdat_cross')}",
             f"byte-exact            : {done}/{total} ({pct:.1f}% of app+comdat)",
             f"byte-exact BYTES      : {bdone}/{btot} ({bpct:.1f}% of app+comdat bytes)"]
    for k in ("mismatched", "stubbed", "unimplemented", "deferred"):
        if st[k]:
            lines.append(f"  {k:20}: {st[k]:5} functions  "
                         f"{by[k]:>9,} bytes  ({100.0*by[k]/btot:.1f}%)")
    return lines


def readme_block(rows):
    st = defaultdict(int)
    by = defaultdict(int)
    per = defaultdict(lambda: defaultdict(int))
    for r in rows:
        if r["kind"] not in ("app", "comdat"):
            continue
        st[r["status"]] += 1
        by[r["status"]] += int(r.get("size") or 0)
        per[r["unit"]][r["status"]] += 1
    done, total = st["byte-exact"], sum(st.values())
    pct = (100.0 * done / total) if total else 0.0
    btot = sum(by.values())
    bdone = by["byte-exact"]
    bpct = (100.0 * bdone / btot) if btot else 0.0
    lib = sum(1 for r in rows if r["kind"] == "library")
    php = next((r for r in rows if r["name"] == "Player_HandlePacket"), None)
    if php is not None and php["status"] != "byte-exact":
        php_note = (
            f"Function counts overstate progress badly: `Player_HandlePacket` "
            f"(228,416 bytes) is **33.9% of all application code** and is now being "
            f"written (its reconnaissance proved it tractable — one `ret`, no "
            f"internal call targets, 43 chunks), so it sits in `mismatched` and the "
            f"byte figure is the honest one until it converges.")
    else:
        php_note = (
            f"`Player_HandlePacket` (228,416 bytes, **33.9% of all application "
            f"code**) is now byte-exact, which is why the byte figure has moved "
            f"close to the function figure.")
    out = [READ_BEGIN, "",
           f"**{done}/{total} ({pct:.1f}%)** application functions byte-exact "
           f"({total} app + compiler COMDATs; {lib} library members excluded).",
           "",
           f"**{bdone:,}/{btot:,} ({bpct:.1f}%)** application BYTES byte-exact. "
           + php_note,
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
        refresh_cached_kinds(rows, kinds)

    by_unit, names, glob_pool, prov = our_functions(
        ca, vu.canon, args.asm_dir, units)
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
    # A source function corresponds to exactly one reference function, so a match
    # consumes it. `used` is keyed by (unit, name) and shared across units: the
    # cross-unit fallback below can consume another unit's function, and its own
    # group must then not reuse it.
    used = set()
    ambiguous = {}        # (unit, start) -> (row, candidate count), refused
    for unit, group in groups.items():
        group.sort(key=lambda r: r["start"])
        pool = by_unit.get(unit, {})

        def take(r, truncate, nxt=None):
            forms = range_forms(r["_ins"], vu.canon, truncate)
            merged_form = None
            # Ghidra sometimes splits a function after its prologue; a short
            # ret-less fragment followed by its successor is one function.
            if (nxt is not None and "ret" not in r["_ins"]
                    and len(r["_ins"]) <= 6 and nxt["_lo"] == r["_hi"]):
                merged_form = tuple(vu.canon(r["_ins"] + nxt["_ins"]))
                forms.append(merged_form)
            for cand in forms:
                if not cand:
                    continue
                for name in pool.get(cand, ()):
                    if (unit, name) not in used:
                        used.add((unit, name))
                        r["source_name"] = name
                        if cand == merged_form:
                            r["_merged_nxt"] = nxt
                        return True
            # Cross-unit fallback: the row's function may be emitted by another
            # translation unit (a COMDAT the reference placed in this unit's
            # span). The same generic body exists for many element types, so
            # accept only a globally unique candidate; otherwise report the
            # ambiguity and leave the row unmatched rather than mis-match it.
            for cand in forms:
                if not cand:
                    continue
                hits = [h for h in glob_pool.get(cand, ()) if h not in used]
                if len(hits) == 1:
                    cu, name = hits[0]
                    used.add((cu, name))
                    r["source_name"] = name
                    r["_cross_unit"] = cu
                    if cand == merged_form:
                        r["_merged_nxt"] = nxt
                    return True
                if len(hits) > 1:
                    ambiguous[(r["unit"], r["start"])] = (r, len(hits))
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
        # When the merge branch consumed a short head, the head's source
        # function covers the successor body's bytes too, so the successor is
        # not a separate reconstruction target. Mark it `merged` (excluded from
        # the denominator) unless it independently converged on its own.
        for r in pending:
            nxt = r.get("_merged_nxt")
            if (nxt is not None and nxt.get("status") != "byte-exact"
                    and nxt["kind"] not in ("library", "stub")):
                nxt["kind"], nxt["status"], nxt["source_name"] = "merged", "n/a", ""
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

    # A placeholder proves application code, so the (heuristic) run extension
    # must not hide it behind a library vote.
    for r in rows:
        if r["status"] == "stubbed" and r["kind"] == "library":
            r["kind"] = "app"

    # `_extend_library_runs` grows the library classification past the group
    # loop above, so the rows it newly marks keep a stale `unimplemented` unless
    # they are n/a'd here. A statically linked RTL/VCL/BDE member and a module
    # initializer stub have no source to write, whatever their earlier status.
    for r in rows:
        if r["kind"] in ("library", "stub") and r["status"] != "byte-exact":
            r["status"], r["source_name"] = "n/a", ""

    # A function we have reproduced is application code by definition; never let
    # the (heuristic) library vote hide it.
    for r in rows:
        if r["status"] == "byte-exact":
            r["kind"] = "app" if r["kind"] != "comdat" else "comdat"

    # Compiler/RTL-generated rows that are not hand-written targets (RTL
    # sentinel accessors, Borland-header local-static guards, cross-unit
    # template COMDATs). Run last: it needs every status settled so the
    # call-graph proof can see which callers are reproduced.
    _classify_artifacts(rows, pe, vu, by_unit, glob_pool, prov, used)

    # Cross-unit fallback refusals: a row whose function another unit emits but
    # whose canonical sequence is not globally unique is left unmatched and
    # reported, rather than guessed -- a wrong cross-unit match hides real work.
    unresolved = [(r, n) for r, n in ambiguous.values()
                  if r.get("status") not in ("byte-exact", "n/a")]
    if unresolved:
        print(f"cross-unit fallback: {len(unresolved)} ambiguous row(s) left "
              f"unmatched", file=sys.stderr)
        for r, n in sorted(unresolved,
                           key=lambda x: (x[0]["unit"], x[0]["start"])):
            print(f"  {r['unit']} {r['start']:#010x} {r['name']}: {n} candidate "
                  f"functions across units", file=sys.stderr)

    write_tsv(rows, args.out)
    print("\n".join(summary(rows)), file=sys.stderr)
    if args.readme:
        update_readme(args.readme, readme_block(rows))
    if args.summary:
        print("\n".join(summary(rows)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
