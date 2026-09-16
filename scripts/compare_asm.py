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


# Mnemonic aliases for the same opcode: objdump and bcc32 name some conditions
# differently (both encode 0x9D), which would otherwise show as a mismatch.
MNEMONIC_ALIASES = {
    "setnl": "setge", "setnle": "setg", "setnb": "setae", "setnbe": "seta",
    "setnge": "setl", "setng": "setle", "setnae": "setb", "setna": "setbe",
    "setpe": "setp", "setpo": "setnp",
    "jge": "jnl", "jg": "jnle", "jae": "jnb", "ja": "jnbe",
    "jbe": "jna", "jb": "jnae", "jl": "jnge", "jle": "jng",
    "cmovnl": "cmovge", "cmovnle": "cmovg", "cmovae": "cmovnb",
}


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
    parts = s.split(" ", 1)
    if parts[0] in MNEMONIC_ALIASES:
        s = MNEMONIC_ALIASES[parts[0]] + (" " + parts[1] if len(parts) > 1 else "")
    return s


# EH scope markers are `mov word ptr [ebp-N], imm`, where imm is the current
# cleanup scope's byte offset. The marker *sequence* encodes block nesting, so
# printing it side by side is the quickest way to spot a missing/extra scope.
# Match the raw text so the word-sized marker is not confused with a dword data
# store to a local (canon drops the size).
MARKER_RAW = re.compile(
    r"^mov\s+word\s+ptr\s+\[ebp\s*-\s*(0x[0-9a-f]+|\d+)\]\s*,\s*(0x[0-9a-f]+|\d+)\s*$",
    re.I)


def marker_of(raw: str):
    m = MARKER_RAW.match(raw.strip())
    if not m:
        return None
    off = int(m.group(1), 0) if m.group(1).lower().startswith("0x") else int(m.group(1))
    val = int(m.group(2), 0) if m.group(2).lower().startswith("0x") else int(m.group(2))
    return f"[ebp-{off}]={val}"


def print_markers(ref, our, force: bool) -> None:
    if not force and ref == our:
        return
    print("\nEH scope markers:")
    print(f"  ref ({len(ref)}): {' '.join(ref) or '(none)'}")
    print(f"  our ({len(our)}): {' '.join(our) or '(none)'}")
    for i in range(max(len(ref), len(our))):
        r = ref[i] if i < len(ref) else "<none>"
        o = our[i] if i < len(our) else "<none>"
        if r != o:
            print(f"  first difference at marker {i}: ref {r} vs our {o}")
            break


def print_diff(ref, our, context: int = 3) -> None:
    """Aligned diff of the two instruction streams.

    The positional comparison cascades: one extra instruction shifts everything
    after it, so every later line reports a mismatch. A sequence alignment shows
    the real insertions/deletions instead, which is what identifies the source
    construct to add or remove.
    """
    import difflib
    sm = difflib.SequenceMatcher(None, ref, our, autojunk=False)
    hunks = [op for op in sm.get_opcodes() if op[0] != "equal"]
    if not hunks:
        return
    print(f"\naligned diff ({len(hunks)} hunk(s)):")
    for tag, i1, i2, j1, j2 in hunks:
        lo = max(0, i1 - context)
        print(f"  @@ ref[{i1}:{i2}] our[{j1}:{j2}] {tag}")
        for x in ref[lo:i1]:
            print(f"     ref  {x}")
        if tag in ("delete", "replace"):
            for x in ref[i1:i2]:
                print(f"   - ref  {x}")
        if tag in ("insert", "replace"):
            for x in our[j1:j2]:
                print(f"   + our  {x}")


def parse_our(path: str, mangled_prefix: str):
    txt = open(path, encoding="latin1").read()
    m = re.search(r"(?m)^" + re.escape(mangled_prefix) + r"\$q[^\n]*\n(.*?)endp", txt, re.S)
    if not m:
        return None, []
    skip = {"dw", "dd", "db", "dt", "public", "extrn", "segment", "ends", "proc",
            "endp", "end", "align", "assume", "org", "equ", "_data", "_text",
            "_bss", "_tls", "_rdata"}
    out = []
    calls = []
    mk = []
    for line in m.group(1).split("\n"):
        s = line.strip()
        if not s or s[0] in ";?@":
            continue
        if s.split()[0].lower() in skip:
            continue
        cm = re.match(r"(?i)^call\s+(\S+)", s)
        if cm:
            calls.append(cm.group(1))
        mm = marker_of(s)
        if mm:
            mk.append(mm)
        out.append(canon(s))
    return out, calls, mk


# Array forms call a different RTL routine than the scalar forms; because call
# targets are canonicalized they compare equal, so a wrong choice only shows up
# at link time. Flag the family explicitly.
ARRAY_NEW_DELETE = ("$bnwa$", "$bnw$", "$bdea$", "$bde$", "operator_new_",
                    "@$bnwa", "@$bdea")


def call_warnings(calls):
    warn = []
    for c in calls:
        low = c.lower()
        if any(tok in low for tok in ("bnwa", "bnw$", "bdea", "bde$")):
            warn.append(c)
    return warn


def parse_ref(ref_bin: str, start: int, end: int):
    txt = subprocess.check_output(
        ["objdump", "-d", "-M", "intel", f"--start-address={start}", f"--stop-address={end}", ref_bin],
        stderr=subprocess.DEVNULL).decode("latin1")
    out = []
    mk = []
    for line in txt.split("\n"):
        m = re.match(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+(\S+)\s*(.*)$", line)
        if m:
            raw = f"{m.group(1)} {m.group(2)}"
            mm = marker_of(raw)
            if mm:
                mk.append(mm)
            out.append(canon(raw))
    return out, mk


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("asm")
    ap.add_argument("function")
    ap.add_argument("mangled_prefix")
    ap.add_argument("ref_start", type=lambda v: int(v, 0))
    ap.add_argument("ref_end", type=lambda v: int(v, 0))
    ap.add_argument("--ref-bin", default="GameServer.exe")
    ap.add_argument("--calls", action="store_true",
                    help="print the call symbol names in our listing")
    ap.add_argument("--markers", action="store_true",
                    help="always print the EH scope marker streams")
    ap.add_argument("--diff", action="store_true",
                    help="always print the aligned instruction diff")
    args = ap.parse_args()

    our, calls, our_mk = parse_our(args.asm, args.mangled_prefix)
    if our is None:
        print(f"function {args.mangled_prefix} not found in {args.asm}")
        return 2
    ref, ref_mk = parse_ref(args.ref_bin, args.ref_start, args.ref_end)
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
    # Call symbols are visible only on our side (the reference is stripped), so
    # print them and flag the array new/delete family, whose wrong choice
    # canonicalizes away.
    warn = call_warnings(calls)
    if args.calls:
        print("calls: " + (", ".join(sorted(set(calls))) or "(none)"))
    if warn:
        print("WARNING: array new/delete calls present (verify against the "
              "reference; scalar forms canonicalize identically): "
              + ", ".join(sorted(set(warn))))
    if args.diff or mismatches > 0:
        print_diff(ref, our)
    print_markers(ref_mk, our_mk, force=args.markers or mismatches > 0)
    return 0 if mismatches == 0 and len(ref) == len(our) else 1


if __name__ == "__main__":
    raise SystemExit(main())
