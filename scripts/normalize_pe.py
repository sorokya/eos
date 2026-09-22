#!/usr/bin/env python3
"""Deterministically normalize volatile PE header fields.

ilink32 stamps the output with the wall-clock time, so a faithful rebuild will
still differ from the reference in the four TimeDateStamp bytes. This tool is
the documented post-link step: it rewrites those bytes (and, if requested, the
COFF characteristics word) to the reference values.

The COFF header is not the only place ilink32 stamps the clock: every
IMAGE_RESOURCE_DIRECTORY in `.rsrc` carries its own TimeDateStamp, and ilink32
writes the current time into all of them. The reference carries 0x418F00E9 in
every one, so two otherwise identical relinks differ in `.rsrc` unless these
are normalized too; that is done by default here.

The import directory carries a third volatile: ilink32 never initialises the
TimeDateStamp and ForwarderChain words of its IMAGE_IMPORT_DESCRIPTORs, so
they hold whatever was in the linker's heap -- they differ between two links
of identical objects (and between `MAP=1`'s two links). Nothing reads them for
an unbound image. The reference's values are recorded below and restored by
default when the descriptor count matches (`--no-import-junk` opts out).

Usage:
    normalize_pe.py PE [--timestamp 0x50CBB124] [--characteristics 0x10e]
                       [--resource-timestamp 0x418F00E9] [--no-import-junk]
                       [-o OUT]

Without -o the file is rewritten in place. Values are printed before and after
and the result is re-read to confirm the patch.
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402

REFERENCE_TIMESTAMP = 0x50CBB124
REFERENCE_CHARACTERISTICS = 0x010E
REFERENCE_RESOURCE_TIMESTAMP = 0x418F00E9

# (TimeDateStamp, ForwarderChain) of each of the reference's eight import
# descriptors, in directory order; the terminating descriptor is all zeros.
REFERENCE_IMPORT_JUNK = [
    (0x076CBD7C, 0x00000001), (0x00007A4D, 0x07709924),
    (0x00000000, 0x00007A35), (0x00000002, 0x00000000),
    (0x00007A11, 0x07709924), (0x07719BB4, 0x0000004D),
    (0x07709924, 0x07719BB4), (0x0000004B, 0x00000005),
]


def resource_directory_offsets(pe):
    """File offsets of every IMAGE_RESOURCE_DIRECTORY in .rsrc, depth-first."""
    rsrc = next((s for s in pe.sections if s.name == ".rsrc"), None)
    if rsrc is None:
        return []
    body = pe.data[rsrc.raw_pointer:rsrc.raw_pointer + rsrc.raw_size]
    found, pending = [], [0]
    while pending:
        off = pending.pop()
        if off in found or off + 16 > len(body):
            continue
        found.append(off)
        named, ids = struct.unpack_from("<HH", body, off + 12)
        entry = off + 16
        for _ in range(named + ids):
            if entry + 8 > len(body):
                break
            _name, child = struct.unpack_from("<II", body, entry)
            entry += 8
            if child & 0x80000000:
                pending.append(child & 0x7FFFFFFF)
    return sorted(rsrc.raw_pointer + o for o in found)


def import_descriptor_offsets(pe, data):
    """File offsets of every non-terminating IMAGE_IMPORT_DESCRIPTOR."""
    opt = pe.coff_offset + 20
    rva = struct.unpack_from("<I", data, opt + 96 + 8)[0]
    sec = next((s for s in pe.sections
                if s.virtual_address <= rva < s.virtual_address + s.raw_size), None)
    if sec is None:
        return []
    off, out = rva - sec.virtual_address + sec.raw_pointer, []
    while any(data[off:off + 20]) and struct.unpack_from("<I", data, off + 12)[0]:
        out.append(off)
        off += 20
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pe")
    ap.add_argument("--timestamp", type=lambda v: int(v, 0), default=REFERENCE_TIMESTAMP)
    ap.add_argument("--characteristics", type=lambda v: int(v, 0), default=None,
                    help=f"rewrite the COFF characteristics word (reference {REFERENCE_CHARACTERISTICS:#06x})")
    ap.add_argument("--resource-timestamp", type=lambda v: int(v, 0),
                    default=REFERENCE_RESOURCE_TIMESTAMP,
                    help="rewrite every IMAGE_RESOURCE_DIRECTORY TimeDateStamp "
                         f"(reference {REFERENCE_RESOURCE_TIMESTAMP:#010x}); -1 leaves them alone")
    ap.add_argument("--no-import-junk", action="store_true",
                    help="leave the import descriptors' TimeDateStamp/ForwarderChain alone")
    ap.add_argument("-o", "--out", default=None)
    args = ap.parse_args()

    pe = PE.from_file(args.pe)
    data = bytearray(pe.data)
    ts_off = pe.header()["time_date_stamp_offset"]
    char_off = pe.coff_offset + 18

    before_ts = struct.unpack_from("<I", data, ts_off)[0]
    struct.pack_into("<I", data, ts_off, args.timestamp)
    print(f"TimeDateStamp @ {ts_off:#x}: {before_ts:#010x} -> {args.timestamp:#010x}")

    if args.characteristics is not None:
        before_ch = struct.unpack_from("<H", data, char_off)[0]
        struct.pack_into("<H", data, char_off, args.characteristics)
        print(f"Characteristics @ {char_off:#x}: {before_ch:#06x} -> {args.characteristics:#06x}")

    if args.resource_timestamp >= 0:
        dirs = resource_directory_offsets(pe)
        for off in dirs:
            struct.pack_into("<I", data, off + 4, args.resource_timestamp)
        if dirs:
            print(f"resource TimeDateStamp x{len(dirs)} -> {args.resource_timestamp:#010x}")

    if not args.no_import_junk:
        descs = import_descriptor_offsets(pe, data)
        if len(descs) == len(REFERENCE_IMPORT_JUNK):
            for off, (ts, fc) in zip(descs, REFERENCE_IMPORT_JUNK):
                struct.pack_into("<II", data, off + 4, ts, fc)
            print(f"import descriptor TimeDateStamp/ForwarderChain x{len(descs)} restored")
        else:
            print(f"import descriptors: {len(descs)} (reference {len(REFERENCE_IMPORT_JUNK)}); left alone")

    out = args.out or args.pe
    with open(out, "wb") as fh:
        fh.write(data)

    check = PE(data).header()
    assert check["time_date_stamp"] == args.timestamp, "timestamp patch failed"
    if args.characteristics is not None:
        assert check["characteristics"] == args.characteristics, "characteristics patch failed"
    print(f"wrote {out}  md5={PE(data).whole_file_md5()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
