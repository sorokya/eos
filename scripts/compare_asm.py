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


# Condition-code aliases.  bcc32 and objdump spell the *same* condition
# differently depending on the comparison's source form, so before comparing we
# fold every mnemonic to one representative per condition class.  Only the
# mnemonic is folded - operands are untouched, and genuinely different
# conditions are never merged.
JCC_CLASSES = [
    ("jnl",  ["jge", "jnl"]),
    ("jnle", ["jg", "jnle"]),
    ("jng",  ["jle", "jng"]),
    ("jnge", ["jl", "jnge"]),
    ("jnb",  ["jae", "jnb", "jnc"]),
    ("jnbe", ["ja", "jnbe"]),
    ("jna",  ["jbe", "jna"]),
    ("jnae", ["jb", "jnae", "jc"]),
    ("je",   ["je", "jz"]),
    ("jne",  ["jne", "jnz"]),
    ("jp",   ["jp", "jpe"]),
    ("jnp",  ["jnp", "jpo"]),
    ("js",   ["js"]),
    ("jns",  ["jns"]),
    ("jo",   ["jo"]),
    ("jno",  ["jno"]),
]


def _build_aliases():
    out = {}
    for prefix in ("", "set", "cmov"):
        for rep, alts in JCC_CLASSES:
            for a in alts:
                # setcc/cmovcc drop the leading `j`
                key = a if not prefix else a[1:]
                val = rep if not prefix else rep[1:]
                out[prefix + key] = prefix + val
    return out


MNEMONIC_ALIASES = _build_aliases()


def canon(ins: str) -> str:
    s = ins.lower().strip()
    s = re.sub(r"<[^>]*>", "", s)
    s = re.sub(r"\[[^\]]*@[^\]]*\]", "[ADDR]", s)
    # A bare-symbol memory operand (e.g. bcc32's `[_GUI]` package slot) is a
    # relocated address; the reference disassembly shows it resolved (a number).
    # Registers are also bare identifiers, so exclude them.
    s = re.sub(
        r"\[([A-Za-z_@][A-Za-z0-9_@]*)\]",
        lambda m: m.group(0) if re.fullmatch(r"[a-z]{2,3}", m.group(1).lower())
        else "[ADDR]",
        s)
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
    # String-op operands are implicit: bcc32's listing prints a bare `rep movsd`
    # while objdump spells out `rep movsd es:[edi],ds:[esi]`. They are the same
    # instruction, so drop the operand text.
    s = re.sub(r"^(rep\s+(?:movs|stos|lods|scas|cmp)s?[bwd]?)\b.*$", r"\1", s)
    # `xchg` is symmetric: bcc32 and objdump may print the two operands in
    # either order (`xchg edx,eax` vs `xchg eax,edx`), and they are the same
    # one-byte instruction. Sort the operands so the order is not compared.
    m = re.match(r"^(xchg)\s+([^,]+),(.+)$", s)
    if m:
        a, b = sorted((m.group(2).strip(), m.group(3).strip()))
        s = f"{m.group(1)} {a},{b}"
    # A shift/rotate by one has an implicit operand: objdump prints `sar eax`
    # where bcc32 prints `sar eax,1`. They encode the same byte (`d1 f8`).
    m = re.match(r"^(rcl|rcr|rol|ror|sal|sar|shl|shr)\s+([^,]+)$", s)
    if m:
        s = f"{m.group(1)} {m.group(2)},1"
    return s


# EH scope markers are `mov word ptr [ebp-N], imm`, where imm is the current
# cleanup scope's byte offset. The marker *sequence* encodes block nesting, so
# printing it side by side is the quickest way to spot a missing/extra scope.
# Match the raw text so the word-sized marker is not confused with a dword data
# store to a local (canon drops the size).
MARKER_RAW = re.compile(
    r"^mov\s+word\s+ptr\s+\[ebp\s*-\s*(0x[0-9a-f]+|\d+)\]\s*,\s*(0x[0-9a-f]+|\d+)\s*$",
    re.I)


FRAME_RE = re.compile(r"^(add|sub)\s+esp,\s*(-?\d+)$")


def frame_size(seq):
    """(index, signed size) of the prologue frame instruction, or (None, None)."""
    for i, ins in enumerate(seq[:8]):
        m = FRAME_RE.match(ins)
        if m:
            v = int(m.group(2))
            return i, (-v if m.group(1) == "sub" else v)
    return None, None


def frame_wild(seq):
    """Canonicalize the prologue frame instruction to a wildcard.

    Only the prologue frame is touched: a later `add esp, N` is a cdecl call
    cleanup and stays comparable.  The epilogue's matching-magnitude release is
    wildcarded too when it is an `add esp, +N` (bcc normally restores with
    `mov esp, ebp`, which already compares equal).
    """
    idx, size = frame_size(seq)
    if idx is None:
        return seq, None
    out = list(seq)
    out[idx] = "add esp,FRAME"
    for j in range(len(out) - 1, idx, -1):
        m = FRAME_RE.match(out[j])
        if m and int(m.group(2)) == -size:
            out[j] = "add esp,FRAME"
            break
    return out, size


EBP_NEG_RE = re.compile(r"\[ebp-(\d+)\]")


def shift_slots(seq, delta):
    """Rewrite every `[ebp-N]` to `[ebp-(N+delta)]` (a negative slot only).

    `[ebp+N]` arguments and `[esp+N]` operands are left alone, and every other
    token is untouched, so the shift is the *only* relaxation.
    """
    if not delta:
        return list(seq)
    out = []
    for ins in seq:
        out.append(EBP_NEG_RE.sub(
            lambda m: "[ebp-%d]" % (int(m.group(1)) + delta), ins))
    return out


def score(ref, our):
    """(aligned prefix, mismatches) for two canonical instruction sequences."""
    n = max(len(ref), len(our))
    mm = sum(1 for i in range(n)
             if (ref[i] if i < len(ref) else "<none>")
             != (our[i] if i < len(our) else "<none>"))
    return aligned_prefix(ref, our), mm


def slot_sequence(seq):
    """The distinct `[ebp-N]` slots in order of first use (for the order check)."""
    seen = []
    for ins in seq:
        for m in EBP_NEG_RE.finditer(ins):
            v = int(m.group(1))
            if v not in seen:
                seen.append(v)
    return seen


def first_divergence(ref, our):
    """(index, ref_ins, our_ins, kind) for the first mismatch, or None."""
    n = min(len(ref), len(our))
    for i in range(n):
        if ref[i] != our[i]:
            a, b = ref[i], our[i]
            if a.split(" ", 1)[0] == b.split(" ", 1)[0]:
                if EBP_NEG_RE.sub("S", a) == EBP_NEG_RE.sub("S", b):
                    kind = "stack-offset (a [ebp-N] shift this delta did not reach)"
                else:
                    kind = "operand/register"
            else:
                kind = "opcode/structure"
            return i, a, b, kind
    if len(ref) != len(our):
        return min(len(ref), len(our)), (
            ref[min(len(ref), len(our))] if len(ref) > len(our) else "<none>"), (
            our[min(len(ref), len(our))] if len(our) > len(ref) else "<none>"), (
            "instruction count")
    return None


def implied_delta(ref, our):
    """The stack delta the first divergent slot pair implies, else None.

    Scans for the first index where both sides carry exactly one `[ebp-N]` and
    the instructions are otherwise identical; the difference of those two
    offsets is the local-area delta (which the frame delta can miss when the
    EH-frame overhead differs).  A hint for --stack-delta, never applied
    automatically.
    """
    for a, b in zip(ref, our):
        if a == b:
            continue
        ma, mb = EBP_NEG_RE.findall(a), EBP_NEG_RE.findall(b)
        if len(ma) != 1 or len(mb) != 1:
            continue
        if EBP_NEG_RE.sub("S", a) == EBP_NEG_RE.sub("S", b):
            return int(ma[0]) - int(mb[0])
    return None


def aligned_prefix(ref, our):
    n = 0
    for a, b in zip(ref, our):
        if a != b:
            break
        n += 1
    return n


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
    esc = re.escape(mangled_prefix)
    m = re.search(r"(?m)^" + esc + r"\s+proc[^\n]*\n(.*?)endp", txt, re.S)
    if not m:
        m = re.search(r"(?m)^" + esc + r"\$q[^\n]*\n(.*?)endp", txt, re.S)
    if not m:
        # (out, calls, (markers, lines, marker_lines)) - a 3-tuple even when
        # the function is absent, so main's unpacking never raises.
        return None, [], ([], [], [])
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


SKIP_DIRECTIVES = {"dw", "dd", "db", "dt", "public", "extrn", "segment", "ends",
                   "proc", "endp", "end", "align", "assume", "org", "equ", "_data",
                   "_text", "_bss", "_tls", "_rdata"}


def _parse_our(path: str, mangled_prefix: str):
    """Like parse_our but also returns the raw (pre-canon) instruction text."""
    txt = open(path, encoding="latin1").read()
    esc = re.escape(mangled_prefix)
    m = re.search(r"(?m)^" + esc + r"\s+proc[^\n]*\n(.*?)endp", txt, re.S)
    if not m:
        m = re.search(r"(?m)^" + esc + r"\$q[^\n]*\n(.*?)endp", txt, re.S)
    if not m:
        return None, [], None, None
    out, calls, mk, raw = [], [], [], []
    for line in m.group(1).split("\n"):
        s = line.strip()
        if not s or s[0] in ";?@":
            continue
        if s.split()[0].lower() in SKIP_DIRECTIVES:
            continue
        cm = re.match(r"(?i)^call\s+(\S+)", s)
        if cm:
            calls.append(cm.group(1))
        mm = marker_of(s)
        if mm:
            mk.append(mm)
        out.append(canon(s))
        raw.append(s)
    return out, calls, mk, raw


def parse_our(path: str, mangled_prefix: str):
    out, calls, mk, _raw = _parse_our(path, mangled_prefix)
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


# ---------------------------------------------------------------------------
# Strict operand comparison (--strict-operands)
#
# The default comparison canonicalizes every absolute address to `ADDR`, so a
# relocated operand matches by construction.  That is exactly what makes it
# blind to a wrong string literal or a wrong direct call target: the operand
# *bytes* differ while the canonical text is `ADDR`.  Strict mode re-reads the
# raw operands of the instructions the canonical pass already accepted and
# compares what the address means:
#
#   * a direct `call` target is compared by callee IDENTITY, never by address.
#     An application function's identity is its Borland-mangled symbol from
#     analysis/target/functions.tsv, compared to the symbol our listing emits.
#     A library/RTL target cannot be named from the stripped reference, so its
#     identity is established by *consistency*: a symbol our side emits for
#     exactly one function must align with one reference body.  A symbol that
#     aligns with two distinct reference addresses is reported, with the
#     majority alignment taken as the symbol's identity and the minority sites
#     (and the reference address each should have called) as defects.
#   * an immediate address that lands in a data section (.data/.rdata/.bss/.tls)
#     is a string/data literal: the bytes at that address are compared to our
#     `offset <label>+N` blob value.
#   * an immediate address in .text that is not a call (an EH table, RTTI or
#     vtable pointer) and an absolute memory operand (`[0x...]`) are relocated
#     slots and are treated as layout-dependent (not compared).
#   * a non-address immediate is compared numerically.  This adds value over the
#     canonical pass, which wildcards every value >= 0x10000.
#
# Anything that cannot be classified is counted as unclassified and produces no
# mismatch, so the mode never invents a difference it cannot justify.

DATA_SECTIONS = {".data", ".rdata", ".bss", ".tls"}
BRANCH_RE = re.compile(r"^(j\w+|loop|loope|loopne)$")


def _s32(v: int) -> int:
    """The signed 32-bit value a constant denotes on this target."""
    v &= 0xFFFFFFFF
    return v - 0x100000000 if v >= 0x80000000 else v


def _split_operands(text: str):
    out, depth, cur = [], 0, ""
    for ch in text:
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def _strip_size(text: str) -> str:
    return re.sub(r"^(?:dword|word|byte|qword)\s+ptr\s+", "", text.strip().lower())


def ref_addr_operands(ins: str):
    """Address-bearing operands of a reference instruction, in operand order."""
    parts = ins.split(None, 1)
    if len(parts) < 2 or parts[0].lower().startswith("rep"):
        return []
    mnem, rest = parts[0].lower(), re.sub(r"<[^>]*>", "", parts[1])
    out = []
    for op in _split_operands(rest):
        low = _strip_size(op)
        if mnem == "call" and re.fullmatch(r"0x[0-9a-f]+|\d+", low):
            out.append(("call", int(low, 0)))
            continue
        if mnem in ("jmp",) or BRANCH_RE.match(mnem):
            continue
        m = re.fullmatch(r"\[(0x[0-9a-f]+|\d+)\]", low)
        if m:
            out.append(("mem", int(m.group(1), 0)))
            continue
        m = re.fullmatch(r"-?(?:0x[0-9a-f]+|\d+)", low)
        if m:
            out.append(("imm", int(low, 0)))
    return out


def our_addr_operands(ins: str):
    """Address-bearing operands of our bcc32 instruction, in operand order."""
    parts = ins.split(None, 1)
    if len(parts) < 2 or parts[0].lower().startswith("rep"):
        return []
    mnem, rest = parts[0].lower(), parts[1]
    if BRANCH_RE.match(mnem) or mnem in ("jmp",):
        return []
    out = []
    for op in _split_operands(rest):
        low = _strip_size(op)
        if mnem == "call" and not low.startswith("["):
            out.append(("call", op.strip()))
            continue
        m = re.search(r"offset\s+([A-Za-z_@][A-Za-z0-9_@]*)(?:\+(\d+))?", op, re.I)
        if m:
            out.append(("sym", (m.group(1), int(m.group(2) or 0))))
            continue
        m = re.fullmatch(r"\[([A-Za-z_@][A-Za-z0-9_@]*)\]", low)
        if m:
            out.append(("memsym", m.group(1)))
            continue
        if not low.startswith("[") and re.fullmatch(r"-?(?:0x[0-9a-f]+|\d+)", low):
            out.append(("imm", int(low, 0)))
    return out


def parse_data_blobs(path: str):
    """Map each `label byte` block in a bcc32 -S listing to its raw bytes.

    The listing can emit the same label more than once (the browser-symbol
    segment repeats it); the longest block for a name wins, and the `; name+N:`
    comments pin each `db` run to its offset.
    """
    def block_bytes(lines, name):
        buf = bytearray()
        pos = 0
        for s in lines:
            cm = re.match(rf";\s*{re.escape(name)}\+(\d+):", s)
            if cm:
                pos = int(cm.group(1))
                continue
            if re.fullmatch(rf";\s*{re.escape(name)}:\s*", s):
                pos = 0
                continue
            dm = re.match(r"db\s+(.*)$", s)
            if not dm:
                continue
            body, i = dm.group(1), 0
            while i < len(body):
                c = body[i]
                if c == '"':
                    j = body.find('"', i + 1)
                    if j < 0:
                        break
                    chunk = body[i + 1:j].encode("latin1")
                    while len(buf) < pos:
                        buf.append(0)
                    buf[pos:pos + len(chunk)] = chunk
                    pos += len(chunk)
                    i = j + 1
                elif c in ", \t":
                    i += 1
                else:
                    m = re.match(r"\d+", body[i:])
                    if m:
                        while len(buf) < pos:
                            buf.append(0)
                        buf.append(int(m.group(0)) & 0xFF)
                        pos += 1
                        i += len(m.group(0))
                    else:
                        i += 1
        return bytes(buf)

    blobs = {}
    cur, lines = None, []
    for raw in open(path, encoding="latin1"):
        s = raw.strip()
        if cur is None:
            m = re.match(r"^(\S+)\s+label\s+(byte|dword|word)$", s)
            if m:
                cur, lines = m.group(1), []
            continue
        if re.search(r"\bends$", s):
            data = block_bytes(lines, cur)
            if len(data) >= len(blobs.get(cur, b"")):
                blobs[cur] = data
            cur, lines = None, []
            continue
        lines.append(s)
    return blobs


class StrictContext:
    """Reference PE + symbol tables + our data blobs for one strict run."""

    def __init__(self, ref_bin, functions_tsv, ghidra_tsv, asm_path,
                 data_blobs, verbose=False):
        import bisect
        import os
        import sys as _sys
        _sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        from pe import PE
        self._bisect = bisect
        self.verbose = verbose
        self.pe = PE.from_file(ref_bin)
        self.base = self.pe.header()["image_base"]
        self.app = []
        try:
            with open(functions_tsv, encoding="utf-8") as fh:
                next(fh, None)
                for line in fh:
                    p = line.rstrip("\n").split("\t")
                    if len(p) >= 9 and p[0] != "unit":
                        self.app.append((int(p[2], 16), int(p[3], 16), p[6],
                                         p[5], p[7]))
            self.app.sort()
        except OSError:
            pass
        self.app_starts = [a[0] for a in self.app]
        self.ghidra = []
        try:
            with open(ghidra_tsv, encoding="utf-8") as fh:
                next(fh, None)
                for line in fh:
                    p = line.rstrip("\n").split("\t")
                    if len(p) >= 3:
                        try:
                            self.ghidra.append((int(p[0], 16), p[2]))
                        except ValueError:
                            pass
            self.ghidra.sort()
        except OSError:
            pass
        self.ghidra_starts = [g[0] for g in self.ghidra]
        self.blobs = data_blobs

    def section_of(self, va):
        rva = va - self.base
        for s in self.pe.sections:
            if s.virtual_address <= rva < s.virtual_address + s.extent:
                return s.name.rstrip("\x00").strip().lower()
        return None

    def read_va(self, va, n=256):
        rva = va - self.base
        for s in self.pe.sections:
            if s.virtual_address <= rva < s.virtual_address + s.raw_size:
                off = s.raw_pointer + (rva - s.virtual_address)
                return self.pe.data[off:off + n]
        return None

    def app_at(self, va):
        i = self._bisect.bisect_right(self.app_starts, va) - 1
        if i >= 0:
            s, e, k, rn, sn = self.app[i]
            if s <= va < e and k in ("app", "comdat", "stub"):
                return k, rn, sn
        return None

    def ghidra_at(self, va):
        i = self._bisect.bisect_right(self.ghidra_starts, va) - 1
        if i >= 0:
            return self.ghidra[i][1]
        return None

    def ref_identity(self, va):
        a = self.app_at(va)
        if a:
            _k, rn, sn = a
            # The project sometimes renames a compiler-emitted COMDAT (for
            # example the std::vector accessors): the authoritative mangled
            # symbol is source_name, but our source calls the function under the
            # recovered ref_name.  Carry both so matching can accept either.
            # A name that is still a placeholder (source_name absent or itself a
            # FUN_ symbol) cannot be verified from the stripped reference, so it
            # is reported as `unknown` and never invents a mismatch.
            title = sn or rn
            if not sn or sn.startswith("@@FUN_") or sn.startswith("@@sub_"):
                return "unknown", title, va, rn
            return "app", title, va, rn
        return "lib", (self.ghidra_at(va) or "sub_%08x" % va), va, ""

    @staticmethod
    def symbol_matches(ident, sym):
        kind, key, _va, ref_name = ident
        if kind == "lib":
            return None
        if kind == "unknown":
            # A placeholder cannot prove a mismatch; only accept an explicit
            # @@<ref_name> alias, otherwise leave the site unclassified.
            if ref_name and re.match(r"^@@" + re.escape(ref_name) + r"(?:\$|$)",
                                     sym):
                return True
            return None
        if key and sym == key:
            return True
        # Project rename of a COMDAT: our listing emits @@<ref_name>$<args>.
        if ref_name and re.match(r"^@@" + re.escape(ref_name) + r"(?:\$|$)", sym):
            return True
        return False

    def our_literal(self, label, off):
        blob = self.blobs.get(label)
        if blob is None or off >= len(blob):
            return None
        b = blob[off:off + 256]
        z = b.find(b"\x00")
        return b[:z] if z >= 0 else b

    def ref_immediate(self, va):
        """('str', bytes) / ('coderef',) / ('num', va) for an immediate value."""
        sec = self.section_of(va)
        if sec in DATA_SECTIONS:
            b = self.read_va(va)
            if b is None:
                return None
            z = b.find(b"\x00")
            return "str", (b[:z] if z >= 0 else b)
        if sec == ".text":
            return "coderef", b""
        return "num", va


def strict_compare(ctx, ref_raw, our_raw, ref_canon, our_canon):
    """Return (differences, counts) for canonically-aligned raw operands.

    differences: list of (address, kind, ref_value, our_value, note)
    counts: dict with calls_checked / literals_checked / unclassified.
    """
    diffs = []
    counts = {"calls": 0, "literals": 0, "immediates": 0, "unclassified": 0}
    lib_sites = {}
    n = min(len(ref_raw), len(our_raw), len(ref_canon), len(our_canon))
    for i in range(n):
        if ref_canon[i] != our_canon[i]:
            continue
        rdesc = ref_addr_operands(ref_raw[i][1])
        odesc = our_addr_operands(our_raw[i])
        if len(rdesc) != len(odesc):
            if rdesc or odesc:
                counts["unclassified"] += 1
            continue
        addr = ref_raw[i][0]
        for (rk, rv), (ok, ov) in zip(rdesc, odesc):
            if rk == "call":
                if ok != "call":
                    counts["unclassified"] += 1
                    continue
                counts["calls"] += 1
                ident = ctx.ref_identity(rv)
                m = StrictContext.symbol_matches(ident, ov)
                if m is True:
                    continue
                if m is False:
                    diffs.append((addr, "call", f"{rv:#x} {ident[1]}", ov, ""))
                else:
                    lib_sites.setdefault(ov, []).append((addr, rv, ident[1]))
                continue
            if rk == "mem":
                continue
            if rk != "imm":
                counts["unclassified"] += 1
                continue
            rlit = ctx.ref_immediate(rv)
            if rlit is None:
                counts["unclassified"] += 1
                continue
            if rlit[0] == "str":
                if ok != "sym":
                    counts["unclassified"] += 1
                    continue
                ob = ctx.our_literal(ov[0], ov[1])
                if ob is None:
                    counts["unclassified"] += 1
                    continue
                counts["literals"] += 1
                if rlit[1] != ob:
                    diffs.append((addr, "literal", rlit[1], ob,
                                  f"ref data @{rv:#x}, our {ov[0]}+{ov[1]}"))
            elif rlit[0] == "num":
                if ok == "imm" and _s32(rv) != _s32(ov):
                    counts["immediates"] += 1
                    diffs.append((addr, "immediate", _s32(rv), _s32(ov), ""))
                elif ok != "imm":
                    counts["unclassified"] += 1
            else:  # coderef: relocated slot
                continue

    # Library callee identity by consistency: one our-symbol can denote only one
    # function, so if it aligns with two distinct reference addresses the
    # minority address is a wrong call.
    for sym, sites in lib_sites.items():
        regions = {}
        for a, va, gn in sites:
            regions.setdefault(va, []).append((a, gn))
        if len(regions) > 1:
            dom = max(regions, key=lambda v: len(regions[v]))
            dom_gn = regions[dom][0][1]
            for va, lst in regions.items():
                if va == dom:
                    continue
                for a, gn in lst:
                    diffs.append((a, "call(lib)",
                                  f"{va:#x} {gn}", sym,
                                  f"this symbol also targets {dom:#x} {dom_gn} "
                                  f"at {len(regions[dom])} site(s)"))
    # The reverse: one reference function reached through two of our symbols.
    by_va = {}
    for sym, sites in lib_sites.items():
        for a, va, gn in sites:
            by_va.setdefault(va, {}).setdefault(sym, []).append((a, gn))
    for va, syms in by_va.items():
        if len(syms) > 1:
            dom = max(syms, key=lambda s: len(syms[s]))
            for sym, lst in syms.items():
                if sym == dom:
                    continue
                for a, gn in lst:
                    diffs.append((a, "call(lib)", f"{va:#x} {gn}", sym,
                                  f"the same reference callee is reached as {dom} "
                                  f"at {len(syms[dom])} site(s)"))
    return diffs, counts


def parse_ref(ref_bin: str, start: int, end: int):
    out, mk, _raw = _parse_ref(ref_bin, start, end)
    return out, mk


def _parse_ref(ref_bin: str, start: int, end: int):
    """Like parse_ref but also returns (address, raw instruction) pairs."""
    txt = subprocess.check_output(
        ["objdump", "-d", "-M", "intel", f"--start-address={start}", f"--stop-address={end}", ref_bin],
        stderr=subprocess.DEVNULL).decode("latin1")
    out, mk, raw = [], [], []
    for line in txt.split("\n"):
        m = re.match(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+(\S+)\s*(.*)$", line)
        if m:
            ins = f"{m.group(2)} {m.group(3)}".strip()
            mm = marker_of(ins)
            if mm:
                mk.append(mm)
            out.append(canon(ins))
            raw.append((int(m.group(1), 16), ins))
    return out, mk, raw


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
    ap.add_argument("--frame-wild", action="store_true",
                    help="progress mode: treat the prologue frame size (and the "
                         "matching epilogue release) as a wildcard so a partially "
                         "written function can be scored. Every other check stays "
                         "strict. NEVER an acceptance criterion.")
    ap.add_argument("--stack-wild", action="store_true",
                    help="progress mode: also shift our `[ebp-N]` slots by the "
                         "frame delta (derived from --frame-wild) before comparing, "
                         "so a partially written body can be scored. Ordering, "
                         "registers, arguments, esp operands and constants stay "
                         "exact. NEVER an acceptance criterion.")
    ap.add_argument("--stack-delta", type=lambda v: int(v, 0), default=None,
                    help="progress mode: shift our `[ebp-N]` by this explicit delta "
                         "(instead of the frame-derived one) before comparing.")
    ap.add_argument("--stack-search", action="store_true",
                    help="progress mode: sweep candidate stack deltas and report "
                         "the one that maximises the aligned prefix (with the "
                         "runner-ups), then print the best reading. Implies "
                         "--frame-wild. NEVER an acceptance criterion.")
    ap.add_argument("--stack", action="store_true",
                    help="print the reference-to-ours stack slot mapping")
    ap.add_argument("--lines", action="store_true",
                    help="annotate the diff with our source lines and summarise "
                         "mismatches per source line")
    ap.add_argument("--strict-operands", action="store_true",
                    help="STRICT comparison: additionally compare the VALUE behind "
                         "each canonicalized address operand - string/data literal "
                         "bytes, direct call targets (by callee identity, never by "
                         "address) and non-address immediates. The default mode "
                         "canonicalizes addresses and therefore cannot see a wrong "
                         "literal or a wrong callee; this mode can. Default output "
                         "and exit status are unchanged when the flag is absent.")
    ap.add_argument("--functions-tsv", default="analysis/target/functions.tsv",
                    help="reference function inventory (mangled names) for strict "
                         "call-target identity")
    ap.add_argument("--ghidra-tsv", default="analysis/ghidra/functions.tsv",
                    help="read-only Ghidra inventory, used to label strict library "
                         "call targets")
    args = ap.parse_args()

    our_lines = None
    our_mk_lines = None
    our_raw = None
    ref_raw = None
    if args.lines:
        our, calls, (our_mk, our_lines, our_mk_lines) = parse_our_lines(args.asm, args.mangled_prefix)
    else:
        our, calls, our_mk, our_raw = _parse_our(args.asm, args.mangled_prefix)
    if our is None:
        print(f"function {args.mangled_prefix} not found in {args.asm}")
        return 2
    ref, ref_mk, ref_raw = _parse_ref(args.ref_bin, args.ref_start, args.ref_end)

    strict_diffs = []
    strict_counts = None
    if args.strict_operands:
        if our_raw is None:
            # --lines rebuilds our list through the line-aware parser, which does
            # not retain raw text; fall back to a second parse for the strict pass.
            our_raw = _parse_our(args.asm, args.mangled_prefix)[3]
        blobs = parse_data_blobs(args.asm)
        ctx = StrictContext(args.ref_bin, args.functions_tsv, args.ghidra_tsv,
                            args.asm, blobs)
        strict_diffs, strict_counts = strict_compare(
            ctx, ref_raw, our_raw, ref, our)
    while our and our[-1] == "nop":
        our.pop()
    while ref and ref[-1] == "nop":
        ref.pop()
    # Callee-saved register pushes in the prologue are a register-allocation
    # choice; ignore which ones are saved.
    for seq in (our, ref):
        head = seq[:8]
        seq[:] = [x for x in head if x not in ("push ebx", "push esi", "push edi")] + seq[8:]

    frame_ref = frame_our = None
    if args.stack_wild or args.stack_delta is not None or args.stack_search:
        args.frame_wild = True
    if args.frame_wild:
        ref, frame_ref = frame_wild(ref)
        our, frame_our = frame_wild(our)
        if frame_ref is not None and frame_our is not None:
            d = frame_our - frame_ref
            print(f"frame: ref -{abs(frame_ref):#x} ours -{abs(frame_our):#x} "
                  f"(delta {d:#x})")

    if args.stack_search:
        cands = list(range(-0x400, 0x401, 4))
        for extra in (0, abs(frame_ref) - abs(frame_our) if
                      (frame_ref is not None and frame_our is not None) else 0):
            if extra not in cands:
                cands.append(extra)
        d0 = implied_delta(ref, our) if not args.stack_wild else None
        if d0 is not None and d0 not in cands:
            cands.append(d0)
        results = []
        for d in cands:
            p, m = score(ref, shift_slots(our, d))
            results.append((p, -m, d))
        results.sort(reverse=True)
        best = results[0]
        print("stack search (prefix / mismatches by delta):")
        for p, nm, d in results[:8]:
            mark = "  <-- best" if (p, nm, d) == best else ""
            print(f"   delta {d:+#06x}  aligned prefix {p:5d}  mismatched {-nm}{mark}")
        bp = best[0]
        tied = [r for r in results if r[0] == bp]
        if len(tied) == 1:
            print(f"winner: decisive (no other delta reaches prefix {bp})")
        else:
            print(f"winner: tie - {len(tied)} deltas reach prefix {bp} "
                  f"({', '.join('%#x' % r[2] for r in tied[:6])}); the "
                  f"commitment beyond it is in the bodies, not the stack")
        if frame_ref is not None and frame_our is not None:
            fd = abs(frame_ref) - abs(frame_our)
            print(f"local-area delta: {best[2]:+#x} "
                  f"(frame-derived was {fd:+#x}; the {best[2] - fd:+#x} "
                  f"difference is the EH-frame overhead)")
        args.stack_delta = best[2]
        args.stack_wild = True
    if args.stack_wild or args.stack_delta is not None:
        if args.stack_delta is not None:
            delta = args.stack_delta
        else:
            if frame_ref is None or frame_our is None:
                print("--stack-wild needs both frame sizes (is a prologue present?)")
                return 2
            delta = abs(frame_ref) - abs(frame_our)  # our slots -> reference numbering
        our = shift_slots(our, delta)
        print(f"stack delta applied: {delta:+#x} "
              f"(our frame {abs(frame_our):#x} vs ref {abs(frame_ref):#x})")
        print(f"slot order  ref: {slot_sequence(ref)}")
        print(f"slot order ours: {slot_sequence(our)} "
              f"(after the shift; ordering is strict)")

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
    if args.frame_wild:
        print(f"aligned prefix: {aligned_prefix(ref, our)} / "
              f"{min(len(ref), len(our))}   (--frame-wild is a PROGRESS "
              f"instrument; run without it for acceptance)")
        fd0 = first_divergence(ref, our)
        if fd0:
            i, a, b, kind = fd0
            print(f"first divergence at index {i}: {kind}")
            print(f"   ref: {a}")
            print(f"   our: {b}")
        if not args.stack_wild:
            d = implied_delta(ref, our)
            if d is not None and d:
                print(f"implied stack delta: {d:+#x} "
                      f"(retry with --stack-delta {d:#x})")
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
    if args.strict_operands:
        print("\nstrict operands (--strict-operands): value behind each "
              "canonicalized address")
        print(f"  calls compared: {strict_counts['calls']}   "
              f"literals compared: {strict_counts['literals']}   "
              f"immediates compared: {strict_counts['immediates']}   "
              f"unclassified operands: {strict_counts['unclassified']}")
        if strict_diffs:
            print(f"  {len(strict_diffs)} operand difference(s):")
            for a, kind, rv, ov, note in strict_diffs:
                rs = rv.decode("latin1") if isinstance(rv, bytes) else rv
                os_ = ov.decode("latin1") if isinstance(ov, bytes) else ov
                extra = f"   ({note})" if note else ""
                print(f"    {a:#08x}: {kind} ref={rs!r} our={os_!r}{extra}")
        else:
            print("  no operand differences")
    strict_bad = bool(args.strict_operands and strict_diffs)
    return 0 if (mismatches == 0 and len(ref) == len(our)
                 and not strict_bad) else 1


if __name__ == "__main__":
    raise SystemExit(main())
