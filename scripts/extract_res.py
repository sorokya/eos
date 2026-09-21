#!/usr/bin/env python3
"""Reconstruct a Borland/Win32 .res from a PE's embedded .rsrc section.

The reference image's .rsrc is ground truth. This walks its resource tree and
writes an equivalent .res (the format ilink32 consumes) with the exact type,
id/name, language and payload of every entry, so the linked .rsrc is
byte-identical. The .res and any dumped payloads are derived artifacts and are
not committed.

.res entry layout (as emitted by brcc32 5.40):

    DWORD DataSize
    DWORD HeaderSize          (8 + len(header), header padded to a DWORD)
    Type   : WORD 0xFFFF, WORD ordinal   |  NUL-terminated UTF-16 name
    Name   : same
    <pad to DWORD>
    DWORD DataVersion
    WORD  MemoryFlags
    WORD  LanguageId
    DWORD Version
    DWORD Characteristics
    <DataSize bytes, padded to DWORD>

Usage:
    scripts/extract_res.py GameServer.exe -o build/GameServer.res [--dump DIR]
"""
import argparse
import os
import re
import struct
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


def _num(value: int) -> bytes:
    return struct.pack("<HH", 0xFFFF, value & 0xFFFF)


def _name(value: str) -> bytes:
    return value.encode("utf-16le") + b"\0\0"


def _pad(blob: bytes) -> bytes:
    return blob + b"\0" * ((-len(blob)) % 4)


def entry(type_id: int, name, lang: int, data: bytes, memflags: int = 0x30) -> bytes:
    header = (_num(type_id) if isinstance(type_id, int) else _name(type_id))
    header += (_num(name) if isinstance(name, int) else _name(name))
    header = _pad(header)
    header += struct.pack("<IHHII", 0, memflags, lang, 0, 0)
    return struct.pack("<II", len(data), len(header) + 8) + header + _pad(data)


def null_entry() -> bytes:
    return entry(0, 0, 0, b"", memflags=0)[:0x20]


def safe(text: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]", "_", text)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pe", help="reference PE file")
    ap.add_argument("-o", "--out", default="build/GameServer.res",
                    help="output .res path (gitignored)")
    ap.add_argument("--dump", default="",
                    help="also write each payload (and a .rc) to this directory")
    ap.add_argument("--types", default="",
                    help="comma-separated RT_* type ids to include (default all)")
    ap.add_argument("--only", default="",
                    help="restrict to resources by name or type:id, e.g. "
                         "'TGUI,DVCLAL,MAINICON,3:1'")
    args = ap.parse_args()

    pe = PE.from_file(args.pe)
    resources = pe.resources
    if args.types:
        want = {int(t) for t in args.types.split(",") if t}
        resources = [r for r in resources if r.type_id in want]
    if args.only:
        sel = [s for s in args.only.split(",") if s]

        def chosen(r):
            for s in sel:
                if ":" in s:
                    t, i = s.split(":", 1)
                    if r.type_id == int(t) and r.id == int(i):
                        return True
                elif r.name == s:
                    return True
            return False
        resources = [r for r in resources if chosen(r)]
    resources.sort(key=lambda r: (r.type_id,
                                  r.id if r.id is not None else 0,
                                  r.lang))
    blob = null_entry()
    for r in resources:
        name = r.id if r.id is not None else r.name
        blob += entry(r.type_id, name, r.lang, r.data)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as fh:
        fh.write(blob)

    if args.dump:
        os.makedirs(args.dump, exist_ok=True)
        lines = []
        for r in resources:
            t = RT_NAMES.get(r.type_id, str(r.type_id))
            who = r.name if r.name is not None else f"id{r.id}"
            fn = f"{t}_{safe(str(who))}.bin"
            with open(os.path.join(args.dump, fn), "wb") as fh:
                fh.write(r.data)
            token = r.name if r.name is not None else str(r.id)
            lines.append(f"LANGUAGE {r.lang & 0x3FF}, {(r.lang >> 10) & 0x3F}\n"
                         f'{token} {r.type_id} "{fn}"\n')
        with open(os.path.join(args.dump, "GameServer.rc"), "w", encoding="ascii") as fh:
            fh.write("".join(lines))

    print(f"wrote {args.out}: {len(resources)} resources, {len(blob)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
