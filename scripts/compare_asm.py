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


def print_markers(ref, our, force: bool, our_lines=None) -> None:
    if not force and ref == our:
        return
    print("\nEH scope markers:")
    print(f"  ref ({len(ref)}): {' '.join(ref) or '(none)'}")
    if our_lines:
        shown = [f"{m}(L{ln})" for m, ln in zip(our, our_lines)]
        print(f"  our ({len(our)}): {' '.join(shown)}")
    else:
        print(f"  our ({len(our)}): {' '.join(our) or '(none)'}")
    for i in range(max(len(ref), len(our))):
        r = ref[i] if i < len(ref) else "<none>"
        o = our[i] if i < len(our) else "<none>"
        if r != o:
            where = f" (our line L{our_lines[i]})" if our_lines and i < len(our_lines) else ""
            print(f"  first difference at marker {i}: ref {r} vs our {o}{where}")
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


def parse_our_lines(path: str, mangled_prefix: str):
    """Like parse_our but also returns the source line for each instruction.

    bcc32's -S listing interleaves `?debug L n` directives before the code a
    source line produced, so the listing alone attributes every instruction to a
    source line. Aligning that against the reference shows which line's code the
    reference has and we do not (or vice versa) - the missing lens for functions
    with many temporaries.
    """
    txt = open(path, encoding="latin1").read()
    m = re.search(r"(?m)^" + re.escape(mangled_prefix) + r"\$q[^\n]*\n(.*?)endp",
                  txt, re.S)
    if not m:
        return None, [], []
    skip = {"dw", "dd", "db", "dt", "public", "extrn", "segment", "ends", "proc",
            "endp", "end", "align", "assume", "org", "equ", "_data", "_text",
            "_bss", "_tls", "_rdata"}
    out, calls, mk, lines, mk_lines = [], [], [], [], []
    cur = 0
    for line in m.group(1).split("\n"):
        s = line.strip()
        lm = re.match(r"\?debug\s+L\s+(\d+)", s)
        if lm:
            cur = int(lm.group(1))
            continue
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
            mk_lines.append(cur)
        out.append(canon(s))
        lines.append(cur)
    return out, calls, (mk, lines, mk_lines)


def print_line_summary(ref, our, our_lines, context: int = 3) -> None:
    """Report, per source line, the alignment hunks that involve it.

    Runs the same difflib alignment as print_diff but folds the ref-only
    instructions back onto the source line of the neighbouring our-instruction,
    so each 'the reference has code here that we do not' gap names a line.
    """
    import difflib
    sm = difflib.SequenceMatcher(None, ref, our, autojunk=False)
    per_line = {}
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        # the source line this hunk sits at is the line of the our-instruction
        # just before it (or after, if the hunk is a pure deletion)
        if j1 > 0:
            ln = our_lines[j1 - 1]
        elif j1 < len(our_lines):
            ln = our_lines[j1]
        else:
            ln = 0
        ref_only = i2 - i1 if tag in ("delete", "replace") else 0
        our_only = j2 - j1 if tag in ("insert", "replace") else 0
        e = per_line.setdefault(ln, [0, 0, 0])
        e[0] += 1
        e[1] += ref_only
        e[2] += our_only
    if not per_line:
        return
    print("\nby source line (line: hunks, ref-only instrs, our-only instrs):")
    for ln in sorted(per_line):
        h, ro, oo = per_line[ln]
        print(f"  L{ln:<4} hunks={h:<3} ref_only={ro:<4} our_only={oo}")
STACK_RE = re.compile(r"\[ebp-(\d+)\]")


def stack_offsets(ins: str):
    return STACK_RE.findall(ins)


def print_ref_markers_by_line(ref, our, our_lines, ref_mk) -> None:
    """Annotate the *reference's* scope markers with our source lines.

    The reference has no debug info, but the alignment maps each of its
    instructions to one of ours (and therefore to a source line). That shows
    which of our lines the reference's scope armings belong to, and whether it
    has armings on lines where ours has none.
    """
    import difflib
    sm = difflib.SequenceMatcher(None, ref, our, autojunk=False)
    # ref index -> our index
    ref2our = {}
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            for k in range(i2 - i1):
                ref2our[i1 + k] = j1 + k
        elif tag == "replace":
            for k in range(min(i2 - i1, j2 - j1)):
                ref2our[i1 + k] = j1 + k
    # ref indices that are markers
    mk_idx = [i for i, ins in enumerate(ref) if MARKER_RE_MATCH(ins)]
    if not mk_idx:
        return
    print("\nreference scope armings vs our lines:")
    by_line = {}
    for i in mk_idx:
        val = MARKER_RE_MATCH(ref[i]).group(2)
        j = ref2our.get(i)
        ln = our_lines[j] if (j is not None and j < len(our_lines)) else None
        by_line.setdefault(ln, []).append(val)
    for ln in sorted(by_line, key=lambda x: (x is None, x)):
        vals = " ".join(by_line[ln])
        print(f"  L{ln if ln is not None else '?':<5} ref arms: {vals}")


def MARKER_RE_MATCH(ins: str):
    return re.match(r"^mov\[ebp-(\d+)\],(\d+)$", ins)


def print_stack_map(ref, our) -> None:
    """Map reference stack slots to ours (a stackcmp-style comparison).

    When a function's instruction shapes match but the frame differs, the cause
    is stack-slot ordering: the compiler assigned the same objects to different
    offsets. Aligning the two streams pairs each reference [ebp-N] access with the
    ours it corresponds to, which shows both the constant delta (a frame-size
    difference) and any reordering (declaration order).
    """
    import difflib
    sm = difflib.SequenceMatcher(None, ref, our, autojunk=False)
    pairs = {}
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag not in ("equal", "replace"):
            continue
        for k in range(min(i2 - i1, j2 - j1)):
            for a, b in zip(stack_offsets(ref[i1 + k]), stack_offsets(our[j1 + k])):
                pairs.setdefault(int(a), {}).setdefault(int(b), 0)
                pairs[int(a)][int(b)] += 1
    if not pairs:
        return
    print("\nstack map (ref [ebp-N] -> our [ebp-N], by frequency):")
    deltas = {}
    for ro in sorted(pairs):
        best = sorted(pairs[ro].items(), key=lambda kv: -kv[1])
        ours = ", ".join(f"{oo}({n})" for oo, n in best[:3])
        if len(best) == 1:
            deltas[best[0][0] - ro] = deltas.get(best[0][0] - ro, 0) + 1
        print(f"  ref -{ro:<4} -> {ours}")
    if len(deltas) == 1:
        d = next(iter(deltas))
        print(f"  consistent delta: our = ref + {d} "
              f"(frame {d} bytes larger)" if d else "  consistent delta: 0")
    else:
        print(f"  deltas present: {sorted(deltas.items())} -> slot ordering differs")


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
    ap.add_argument("--stack", action="store_true",
                    help="print the reference-to-ours stack slot mapping")
    ap.add_argument("--lines", action="store_true",
                    help="annotate the diff with our source lines and summarise "
                         "mismatches per source line")
    args = ap.parse_args()

    our_lines = None
    our_mk_lines = None
    if args.lines:
        our, calls, (our_mk, our_lines, our_mk_lines) = parse_our_lines(args.asm, args.mangled_prefix)
    else:
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
    if args.stack:
        print_stack_map(ref, our)
    if our_lines is not None:
        print_ref_markers_by_line(ref, our, our_lines, our_mk)
    if our_lines is not None:
        print_line_summary(ref, our, our_lines)
    print_markers(ref_mk, our_mk, force=args.markers or mismatches > 0,
                  our_lines=our_mk_lines)
    return 0 if mismatches == 0 and len(ref) == len(our) else 1


if __name__ == "__main__":
    raise SystemExit(main())
