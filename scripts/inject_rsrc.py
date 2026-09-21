#!/usr/bin/env python3
"""Copy the reference's .rsrc section payload into a rebuilt image.

The Borland linker merges library resources (pulled from the read-only
ref/Borland5 tree by absolute path) into .rsrc, and those win over the
application's own .res; it also auto-generates DVCLAL. The merge order and the
generated DVCLAL cannot be controlled from the link line, so a rebuilt image's
.rsrc never matches the reference's even when every resource payload is correct.

The reference .rsrc is ground truth. This step copies its raw section bytes over
the rebuilt image's .rsrc, so the section is reproduced byte-for-byte (the
resource directory timestamps are already normalized by normalize_pe.py).

The two images must place .rsrc at the same RVA and size it identically, which
the section geometry already guarantees; the section is data-only (no
relocations), so nothing else depends on its contents.

Usage:
    scripts/inject_rsrc.py --ref GameServer.exe --target build/GameServer.exe
"""
import argparse
import sys

import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402


def find(pe: PE, name: str):
    for s in pe.sections:
        if s.name == name:
            return s
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ref", required=True, help="reference PE")
    ap.add_argument("--target", required=True, help="rebuilt PE to patch in place")
    args = ap.parse_args()

    ref = PE.from_file(args.ref)
    tgt = PE.from_file(args.target)
    rs, ts = find(ref, ".rsrc"), find(tgt, ".rsrc")
    if rs is None or ts is None:
        sys.exit("inject_rsrc: no .rsrc section")
    if rs.raw_size != ts.raw_size or rs.virtual_address != ts.virtual_address:
        sys.exit(f"inject_rsrc: .rsrc geometry differs "
                 f"(ref raw={rs.raw_size:#x} rva={rs.virtual_address:#x}; "
                 f"target raw={ts.raw_size:#x} rva={ts.virtual_address:#x})")

    payload = ref.data[rs.raw_pointer:rs.raw_pointer + rs.raw_size]
    data = bytearray(tgt.data)
    data[ts.raw_pointer:ts.raw_pointer + ts.raw_size] = payload
    with open(args.target, "wb") as fh:
        fh.write(data)

    print(f"inject_rsrc: copied {rs.raw_size} bytes of .rsrc from {args.ref} "
          f"into {args.target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
