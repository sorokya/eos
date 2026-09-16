#!/usr/bin/env python3
"""Compare one function from a bcc32 `-S` listing against a reference range.

Instruction operands are canonicalized so that absolute addresses (relocated
call targets, string literals, EH tables) compare equal, while registers, stack
offsets and small constants must match exactly.

Usage:
    compare_asm.py ASM FUNCTION MANGLED_PREFIX REF_START REF_END [--ref-bin PATH]

Example:
    compare_asm.py build/Serial.asm GetDisplayCode '@Serial@GetDisplayCode' \
        0x412c7c 0x412d38
"""

import argparse
import re
import subprocess
import sys


def canon(ins: str) -> str:
    s = ins.lower().strip()
    s = re.sub(r"<[^>]*>", "", s)
    s = re.sub(r"\[[^\]]*@[^\]]*\]", "[ADDR]", s)
    s = re.sub(r"\boffset\s+\S+", "ADDR", s)
    s = re.sub(r"\bcall\s+(?!dword|word|byte|\[)(\S+)", "call ADDR", s)
    s = re.sub(r"0x[0-9a-f]+", lambda m: str(int(m.group(0), 16)), s)

    def num(m):
        v = int(m.group(0))
        if v > 0x7FFFFFFF:
            v -= 0x100000000
        return "ADDR" if abs(v) >= 0x10000 else str(v)

    s = re.sub(r"-?\b\d+\b", num, s)
    s = re.sub(r"(dword|word|byte|qword) ptr ", "", s)
    s = re.sub(r"\s*,\s*", ",", s)
    s = re.sub(r"\s+\[", "[", s)
    s = re.sub(r"\s*([+\-])\s*", r"\1", s)
    s = re.sub(r"\s+", " ", s).strip()
    s = re.sub(r"^(j\w+|loop)\s+.*$", r"\1 ADDR", s)
    return s


def parse_our(path: str, mangled_prefix: str):
    txt = open(path, encoding="latin1").read()
    m = re.search(r"(?m)^" + re.escape(mangled_prefix) + r"\$q[^\n]*\n(.*?)endp", txt, re.S)
    if not m:
        return None
    skip = {"dw", "dd", "db", "dt", "public", "extrn", "segment", "ends", "proc",
            "endp", "end", "align", "assume", "org", "equ", "_data", "_text",
            "_bss", "_tls", "_rdata"}
    out = []
    for line in m.group(1).split("\n"):
        s = line.strip()
        if not s or s[0] in ";?@":
            continue
        if s.split()[0].lower() in skip:
            continue
        out.append(canon(s))
    return out


def parse_ref(ref_bin: str, start: int, end: int):
    txt = subprocess.check_output(
        ["objdump", "-d", "-M", "intel", f"--start-address={start}", f"--stop-address={end}", ref_bin],
        stderr=subprocess.DEVNULL).decode("latin1")
    out = []
    for line in txt.split("\n"):
        m = re.match(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+(\S+)\s*(.*)$", line)
        if m:
            out.append(canon(f"{m.group(1)} {m.group(2)}"))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("asm")
    ap.add_argument("function")
    ap.add_argument("mangled_prefix")
    ap.add_argument("ref_start", type=lambda v: int(v, 0))
    ap.add_argument("ref_end", type=lambda v: int(v, 0))
    ap.add_argument("--ref-bin", default="GameServer.exe")
    args = ap.parse_args()

    our = parse_our(args.asm, args.mangled_prefix)
    if our is None:
        print(f"function {args.mangled_prefix} not found in {args.asm}")
        return 2
    ref = parse_ref(args.ref_bin, args.ref_start, args.ref_end)
    while our and our[-1] == "nop":
        our.pop()
    while ref and ref[-1] == "nop":
        ref.pop()
    # Callee-saved register pushes in the prologue are a register-allocation
    # choice; ignore which ones are saved.
    for seq in (our, ref):
        head = seq[:8]
        seq[:] = [x for x in head if x not in ("push ebx", "push esi", "push edi")] + seq[8:]

    n = max(len(ref), len(our))
    mismatches = 0
    for i in range(n):
        r = ref[i] if i < len(ref) else "<none>"
        o = our[i] if i < len(our) else "<none>"
        mark = "  " if r == o else "!!"
        if r != o:
            mismatches += 1
        print(f"{mark} {i:3d} ref: {r:44s} our: {o}")
    print(f"\n{args.function}: {len(ref)} ref / {len(our)} our instructions, "
          f"{mismatches} mismatched")
    return 0 if mismatches == 0 and len(ref) == len(our) else 1


if __name__ == "__main__":
    raise SystemExit(main())
