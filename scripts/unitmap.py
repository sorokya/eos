#!/usr/bin/env python3
"""Attribute code addresses to modules and translation units.

The reference image is stripped and has no linker map, so per-unit ranges come
from the export table: `units.py` gives each unit's Initialize stub and its
`[previous unit end, this unit end)` span (the stub sits at the end of the unit's
code). For the *reconstructed* image `MAP=1 scripts/build.sh` runs `ilink32 -s`,
whose "Detailed map of segments" section names the object or library member
behind every range.

Every linked module also emits a refcount-guarded initializer stub, but `units.py`
only sees modules exported as `@@Unit@Initialize`; units compiled without
`#pragma package(smart_init)` (and statically pulled library members) are
invisible to it. Scanning the stubs recovers every module boundary, and testing a
module's bytes against the Borland libraries classifies it: library members are
not reconstructed, so a neighbouring unit's span must not absorb them (`Banned`'s
487 KB export-derived span, for example, holds eight library modules and its real
code is only the last ~64 KB).

Modes (may be combined):

  --map FILE           emit `module -> address range` attribution for a rebuild
  --pe FILE --stubs    scan a PE and emit per-module rows with a classification
  --units FILE --functions FILE
                       emit per-unit reference function inventories

Output columns:

  map     : va_start  va_end  section  class  kind  module
  stubs   : module_start  module_end  init_va  counter  name  class
  units   : unit  order  func_start  func_end  size  name
"""

import argparse
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402

MAP_ROW = re.compile(
    r"\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})H?\s+"
    r"C=(\S+)\s+S=(\S+)\s+G=(\S+)\s+M=(\S+)")
SECTION_ROW = re.compile(
    r"\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]+)H\s+(\S+)\s+(\S+)")

# units.tsv records RVAs; the Ghidra inventory records absolute addresses.
IMAGE_BASE = 0x00400000
STUB_PROLOGUE = b"\x55\x8b\xec"
REFCOUNT_LO, REFCOUNT_HI = 0x58B000, 0x58D000
SIG_LEN = 16


def module_kind(module: str) -> str:
    up = module.upper()
    if "C0W32" in up or "C0NTW" in up:
        return "startup"
    if "BUILD\\OBJ" in up or "BUILD/OBJ" in up:
        return "unit"
    return "library"


def parse_map(path: str):
    """Yield (va_start, length, section, class, module) for every mapped row."""
    bases: dict[int, int] = {}
    rows = []
    with open(path, encoding="latin1") as fh:
        for line in fh:
            m = MAP_ROW.match(line)
            if m:
                seg, off, length, cls, sec, _grp, mod = m.groups()
                rows.append((int(seg, 16), int(off, 16), int(length, 16),
                             sec, cls, mod))
                continue
            m = SECTION_ROW.match(line)
            if m:
                seg, base, _length, sec, cls = m.groups()
                if cls in ("CODE", "DATA", "BSS", "TLS"):
                    bases[int(seg, 16)] = int(base, 16)
    out = []
    for seg, off, length, sec, cls, mod in rows:
        base = bases.get(seg)
        if base is None:
            continue
        out.append((base + off, length, sec, cls, mod))
    return out


def load_functions(path: str):
    funcs = []
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 3:
                continue
            try:
                addr = int(parts[0], 16)
                size = int(parts[1])
            except ValueError:
                continue
            funcs.append((addr, size, parts[2]))
    funcs.sort()
    return funcs


def load_units(path: str):
    units = []
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) < 7 or p[0] == "-":
                continue
            try:
                init = int(p[2], 16) + IMAGE_BASE
            except ValueError:
                continue
            fin = int(p[3], 16) + IMAGE_BASE if p[3] not in ("", "-") else None
            start = int(p[5], 16) + IMAGE_BASE if p[5] not in ("", "-") else None
            refcount = int(p[4], 16) if p[4] not in ("", "-") else None
            units.append({"unit": p[1], "order": int(p[0]), "init": init,
                          "finalize": fin, "prev_end": start,
                          "refcount": refcount})
    return units


def _off_to_va(pe: PE, off: int):
    for sec in pe.sections:
        if sec.raw_pointer <= off < sec.raw_pointer + sec.raw_size:
            return IMAGE_BASE + sec.virtual_address + (off - sec.raw_pointer)
    return None


def find_stubs(pe: PE):
    """Locate module initializer/finalizer stubs in a PE.

    A stub begins `push ebp; mov ebp,esp` then either `inc dword ptr [counter]`
    (`ff 05`), `sub dword ptr [counter],1` (`83 2d`) or `add dword ptr
    [counter],1` (`83 05`), with the counter in the module-refcount data range.
    Returns (va, counter, 'init'|'final') sorted by address.
    """
    data = pe.data
    out = []
    for i in range(3, len(data) - 8):
        if data[i - 3:i] != STUB_PROLOGUE:
            continue
        op = data[i:i + 2]
        counter = None
        kind = None
        if op == b"\xff\x05":
            counter, kind = struct.unpack_from("<I", data, i + 2)[0], "init"
        elif op == b"\x83\x2d" and data[i + 6] == 0x01:
            counter, kind = struct.unpack_from("<I", data, i + 2)[0], "init"
        elif op == b"\x83\x05" and data[i + 6] == 0x01:
            counter, kind = struct.unpack_from("<I", data, i + 2)[0], "final"
        if counter is None or not (REFCOUNT_LO <= counter <= REFCOUNT_HI):
            continue
        va = _off_to_va(pe, i - 3)
        if va is not None:
            out.append((va, counter, kind))
    return sorted(out)


def find_modules(pe: PE, units_by_refcount: dict):
    """Segment a PE into modules using the Initialize stubs as end markers.

    Each module's code ends at its Initialize stub, optionally followed by a
    Finalize stub at +0x10. Returns a list of dicts with the module's code span,
    stub address, refcount counter and the exported unit name (if any).
    """
    stubs = find_stubs(pe)
    starts = {va for va, _c, kind in stubs}
    inits = [(va, c) for va, c, kind in stubs if kind == "init"]
    modules = []
    prev_end = None
    for va, counter in inits:
        tail = 0x20 if (va + 0x10) in starts else 0x10
        modules.append({"start": prev_end, "end": va, "init": va,
                        "counter": counter, "name": units_by_refcount.get(counter, "")})
        prev_end = va + tail
    return modules


def load_libs(root: str) -> bytes:
    """Concatenate every .lib under root (used for byte-signature matching)."""
    blob = b""
    for dp, _dirs, files in os.walk(root):
        for fn in sorted(files):
            if fn.lower().endswith(".lib"):
                with open(os.path.join(dp, fn), "rb") as fh:
                    blob += fh.read()
    return blob


def classify_modules(pe: PE, modules, funcs, lib_blob: bytes, sample: int = 60) -> None:
    """Tag each unnamed module `library` if its code matches the Borland libs.

    A module whose bytes appear in the libraries is a statically linked library
    member and is not reconstructed; anything else is a genuine unknown unit.
    Signatures are sampled evenly across the module so a run of wrappers at the
    start cannot bias the verdict.
    """
    for mod in modules:
        if mod["name"]:
            continue
        cand = []
        for addr, size, _name in funcs:
            if mod["start"] is not None and mod["start"] <= addr < mod["end"] \
                    and size >= SIG_LEN:
                off = _va_to_off(pe, addr)
                if off is not None:
                    cand.append(pe.data[off:off + SIG_LEN])
        if not cand:
            mod["class"] = "unknown"
            continue
        if len(cand) > sample:
            stride = len(cand) / sample
            cand = [cand[int(i * stride)] for i in range(sample)]
        hits = sum(1 for s in cand if s in lib_blob)
        mod["class"] = "library" if hits >= 3 and hits * 2 >= len(cand) else "unknown"


def _va_to_off(pe: PE, va: int):
    rva = va - IMAGE_BASE
    try:
        return pe._rva_to_off(rva)
    except ValueError:
        return None


def emit_map(rows, out) -> int:
    """Emit one row per module range, clamped to the next range in its section.

    ilink32's `Length` column is the module's total contribution while interleaved
    COMDATs mean the row's start may be followed by another module, so the ranges
    as printed can overlap; clamping to the next start recovers the real layout.
    """
    print("\t".join(("va_start", "va_end", "section", "class", "kind", "module")),
          file=out)
    counts = {"startup": 0, "unit": 0, "library": 0}
    bytes_ = {"startup": 0, "unit": 0, "library": 0}
    for sec in dict.fromkeys(r[2] for r in rows):
        section_rows = sorted(r for r in rows if r[2] == sec)
        for i, (va, length, _sec, cls, mod) in enumerate(section_rows):
            if cls != "CODE":
                continue
            end = va + length
            if i + 1 < len(section_rows):
                end = min(end, section_rows[i + 1][0])
            if end <= va:
                continue
            kind = module_kind(mod)
            counts[kind] += 1
            bytes_[kind] += end - va
            print(f"{va:#010x}\t{end:#010x}\t{sec}\t{cls}\t{kind}\t{mod}", file=out)
    return counts, bytes_


def emit_modules(modules, out) -> int:
    print("\t".join(("module_start", "module_end", "init_va", "counter",
                    "name", "class")), file=out)
    for m in modules:
        start = "" if m["start"] is None else f"{m['start']:#010x}"
        cls = m["name"] and "unit" or m.get("class", "")
        print(f"{start}\t{m['end']:#010x}\t{m['init']:#010x}\t"
              f"{m['counter']:#010x}\t{m['name']}\t{cls}", file=out)
    return len(modules)


def load_modules(path: str):
    """Read a module table written by --stubs and return its exclusion ranges."""
    excl = []
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) < 6 or p[5] not in ("library", "unknown") or p[0] == "":
                continue
            excl.append((int(p[0], 16), int(p[1], 16)))
    return excl


def emit_units(units, funcs, excl, out) -> int:
    print("\t".join(("unit", "order", "func_start", "func_end", "size", "name")),
          file=out)
    covered = 0
    for u in units:
        start = u["prev_end"]
        end = (u["finalize"] + 0x10) if u["finalize"] is not None else None
        if start is None or end is None:
            continue
        for addr, size, name in funcs:
            if not (start <= addr < end):
                continue
            if any(a <= addr < b for a, b in excl):
                continue
            print(f"{u['unit']}\t{u['order']}\t{addr:#010x}\t{addr + size:#010x}\t"
                  f"{size}\t{name}", file=out)
            covered += 1
    return covered


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--map", metavar="FILE",
                    help="ilink32 -s detailed map of a rebuilt image")
    ap.add_argument("--units", metavar="FILE",
                    help="translation-unit table (analysis/target/units.tsv)")
    ap.add_argument("--functions", metavar="FILE",
                    help="Ghidra function inventory (analysis/ghidra/functions.tsv)")
    ap.add_argument("--pe", metavar="FILE", help="PE to scan (with --stubs)")
    ap.add_argument("--stubs", action="store_true",
                    help="emit per-module rows with a classification (needs --pe)")
    ap.add_argument("--modules", metavar="FILE",
                    help="module table from --stubs, so --units can exclude "
                         "library/unknown module ranges")
    ap.add_argument("--libs", metavar="DIR", default="ref/Borland5/Lib",
                    help="Borland library tree for module classification")
    ap.add_argument("-o", "--out", default="-", help="output TSV ('-' = stdout)")
    args = ap.parse_args()

    if args.stubs and not args.pe:
        ap.error("--stubs requires --pe")

    out = sys.stdout if args.out == "-" else open(args.out, "w", encoding="utf-8")
    try:
        if args.map:
            rows = parse_map(args.map)
            counts, bytes_ = emit_map(rows, out)
            print(f"map {args.map}: {sum(counts.values())} code ranges "
                  f"({counts['unit']} unit, {counts['startup']} startup, "
                  f"{counts['library']} library; "
                  f"{bytes_['unit']} unit bytes)", file=sys.stderr)
        if args.stubs:
            by_ref = {}
            if args.units:
                for u in load_units(args.units):
                    by_ref[u["refcount"]] = u["unit"]
            pe = PE.from_file(args.pe)
            modules = find_modules(pe, by_ref)
            if args.functions:
                funcs = load_functions(args.functions)
                lib_blob = load_libs(args.libs)
                classify_modules(pe, modules, funcs, lib_blob)
            n = emit_modules(modules, out)
            named = sum(1 for m in modules if m["name"])
            print(f"stubs {args.pe}: {n} modules ({named} exported units, "
                  f"{n - named} unexported)", file=sys.stderr)
        if args.units and args.functions:
            units = load_units(args.units)
            funcs = load_functions(args.functions)
            excl = load_modules(args.modules) if args.modules else []
            covered = emit_units(units, funcs, excl, out)
            print(f"units {args.units}: {covered} reference functions attributed "
                  f"to {len(units)} units ({len(excl)} excluded module ranges)",
                  file=sys.stderr)
        elif args.units and not args.stubs:
            print("--units needs --functions (or use --stubs)", file=sys.stderr)
            return 2
    finally:
        if out is not sys.stdout:
            out.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
