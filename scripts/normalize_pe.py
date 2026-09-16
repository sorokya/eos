#!/usr/bin/env python3
"""Deterministically normalize volatile PE header fields.

ilink32 stamps the output with the wall-clock time, so a faithful rebuild will
still differ from the reference in the four TimeDateStamp bytes. This tool is
the documented post-link step: it rewrites those bytes (and, if requested, the
COFF characteristics word) to the reference values.

Usage:
    normalize_pe.py PE [--timestamp 0x50CBB124] [--characteristics 0x10e] [-o OUT]

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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pe")
    ap.add_argument("--timestamp", type=lambda v: int(v, 0), default=REFERENCE_TIMESTAMP)
    ap.add_argument("--characteristics", type=lambda v: int(v, 0), default=None,
                    help=f"rewrite the COFF characteristics word (reference {REFERENCE_CHARACTERISTICS:#06x})")
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
