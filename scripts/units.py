#!/usr/bin/env python3
"""Recover the translation-unit table from the reference export table.

C++Builder emits @@Unit@Initialize / @@Unit@Finalize exports for every linked
translation unit. The Initialize routine is emitted at the *end* of the unit's
code segment, so sorting the Initialize RVAs recovers the original link order
and each unit's code-end address. Each stub guards a dedicated module reference
counter with `sub/add dword ptr [addr], 1`; the counter address is decoded from
the stub bytes.

Output columns: order, unit, init_rva, finalize_rva, refcount, prev_finalize_end,
code_span. The first unit's span is unknown until its start is established by a
link map.
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402

STUB_LEN = 16


def stub_refcount(pe: PE, rva: int):
    off = pe._rva_to_off(rva)
    b = pe.data[off:off + 9]
    if b[:3] == b"\x55\x8b\xec" and b[3] == 0x83 and b[4] == 0x2D:
        return struct.unpack_from("<I", b, 5)[0]
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pe", help="reference PE")
    ap.add_argument("-o", "--out", default="analysis/target/units.tsv")
    args = ap.parse_args()

    pe = PE.from_file(args.pe)

    pairs: dict[str, dict] = {}
    main_unit = None
    for e in pe.exports:
        if not e.name:
            continue
        if e.name.endswith("@Initialize") or e.name.endswith("@Finalize"):
            unit, _, kind = e.name.rpartition("@")
            unit = unit.lstrip("@")
            pairs.setdefault(unit, {})[kind] = e.rva
        elif e.name == "_GUI":
            main_unit = ("GUI", e.rva)

    units = [(u, d["Initialize"], d.get("Finalize")) for u, d in pairs.items() if "Initialize" in d]
    units.sort(key=lambda x: x[1])

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    prev_end = None
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write("order\tunit\tinit_rva\tfinalize_rva\trefcount\tprev_finalize_end\tcode_span\n")
        for i, (unit, init, fin) in enumerate(units, 1):
            refcount = stub_refcount(pe, init)
            span = "" if prev_end is None else f"{init - prev_end:d}"
            fh.write(f"{i}\t{unit}\t{init:#010x}\t{fin:#010x}\t"
                     f"{refcount:#010x}\t{('' if prev_end is None else f'{prev_end:#010x}')}\t{span}\n")
            prev_end = (fin + STUB_LEN) if fin is not None else (init + STUB_LEN)
        if main_unit:
            fh.write(f"-\t{main_unit[0]}\t{main_unit[1]:#010x}\t-\t-\t-\t-\n")

    print(f"wrote {args.out}: {len(units)} units + {'1 main' if main_unit else 'no main'}")
    print(f"  first unit : {units[0][0]} @ {units[0][1]:#010x}")
    print(f"  last unit  : {units[-1][0]} @ {units[-1][1]:#010x}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
