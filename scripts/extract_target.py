#!/usr/bin/env python3
"""Extract reference-PE metadata into a directory tree.

Produces human-readable listings plus the raw embedded DFM payloads, so the
harness can compare a rebuilt image against the reference without any
third-party packages.
"""

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402

RT_NAMES = {
    1: "RT_CURSOR", 2: "RT_BITMAP", 3: "RT_ICON", 4: "RT_MENU",
    5: "RT_DIALOG", 6: "RT_STRING", 7: "RT_FONTDIR", 8: "RT_FONT",
    9: "RT_ACCELERATOR", 10: "RT_RCDATA", 11: "RT_MESSAGETABLE",
    12: "RT_GROUP_CURSOR", 14: "RT_GROUP_ICON", 16: "RT_VERSION",
    24: "RT_MANIFEST",
}


def write(path: str, text: str) -> None:
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pe", help="reference PE file")
    ap.add_argument("-o", "--out", default="analysis/target", help="output directory")
    args = ap.parse_args()

    pe = PE.from_file(args.pe)
    os.makedirs(args.out, exist_ok=True)

    h = pe.header()

    # Header listing -------------------------------------------------------
    lines = [f"{k} = {v:#x}" if isinstance(v, int) else f"{k} = {v}" for k, v in h.items()]
    write(os.path.join(args.out, "headers.txt"), "\n".join(lines) + "\n")

    # Section table --------------------------------------------------------
    lines = [f"{'name':8} {'vsize':>10} {'vaddr':>10} {'rawsize':>10} {'rawptr':>10} "
             f"{'chars':>10} {'raw_md5':32}"]
    for s in pe.sections:
        lines.append(f"{s.name:8} {s.virtual_size:#10x} {s.virtual_address:#10x} "
                     f"{s.raw_size:#10x} {s.raw_pointer:#10x} {s.characteristics:#10x} "
                     f"{pe.section_md5(s)}")
    write(os.path.join(args.out, "sections.txt"), "\n".join(lines) + "\n")

    # Imports --------------------------------------------------------------
    by_dll: dict[str, list] = {}
    for imp in pe.imports:
        by_dll.setdefault(imp.dll, []).append(imp)
    lines = []
    for dll in sorted(by_dll):
        lines.append(f"{dll} ({len(by_dll[dll])} functions)")
        for imp in by_dll[dll]:
            lines.append(f"    {imp.name}" if imp.name else f"    #{imp.ordinal}")
    write(os.path.join(args.out, "imports.txt"), "\n".join(lines) + "\n")

    # Exports --------------------------------------------------------------
    lines = []
    for exp in pe.exports:
        lines.append(f"{exp.ordinal:5} {exp.rva:#010x} {exp.name or ''}")
    write(os.path.join(args.out, "exports.txt"), "\n".join(lines) + "\n")

    # Resources ------------------------------------------------------------
    lines = []
    for res in pe.resources:
        kind = RT_NAMES.get(res.type_id, res.type_name or str(res.type_id))
        label = res.name if res.name is not None else res.id
        lines.append(f"{kind:16} {label!s:20} lang={res.lang:<6} size={len(res.data):<8} rva={res.rva:#010x}")
    write(os.path.join(args.out, "resources.txt"), "\n".join(lines) + "\n")

    # Raw DFM payloads (RT_RCDATA) ----------------------------------------
    dfm_dir = os.path.join(args.out, "dfm")
    os.makedirs(dfm_dir, exist_ok=True)
    for res in pe.resources:
        if res.type_id != 10:
            continue
        label = res.name if res.name is not None else str(res.id)
        with open(os.path.join(dfm_dir, f"{label}.dfm"), "wb") as fh:
            fh.write(res.data)

    # Machine-readable summary --------------------------------------------
    summary = {
        "file": os.path.abspath(args.pe),
        "size": len(pe.data),
        "md5": pe.whole_file_md5(),
        "headers": h,
        "sections": [
            {"name": s.name, "virtual_size": s.virtual_size, "virtual_address": s.virtual_address,
             "raw_size": s.raw_size, "raw_pointer": s.raw_pointer, "characteristics": s.characteristics,
             "md5": pe.section_md5(s)}
            for s in pe.sections
        ],
        "imports": [{"dll": i.dll, "name": i.name, "ordinal": i.ordinal} for i in pe.imports],
        "exports": [{"ordinal": e.ordinal, "name": e.name, "rva": e.rva} for e in pe.exports],
        "resources": [
            {"type_id": r.type_id, "type_name": r.type_name, "id": r.id, "name": r.name,
             "lang": r.lang, "size": len(r.data), "rva": r.rva}
            for r in pe.resources
        ],
    }
    with open(os.path.join(args.out, "metadata.json"), "w", encoding="utf-8") as fh:
        json.dump(summary, fh, indent=2)

    print(f"extracted {os.path.abspath(args.pe)} -> {args.out}")
    print(f"  sections  : {len(pe.sections)}")
    print(f"  imports   : {len(pe.imports)} functions in {len(by_dll)} DLLs")
    print(f"  exports   : {len(pe.exports)}")
    print(f"  resources : {len(pe.resources)}")
    print(f"  md5       : {pe.whole_file_md5()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
