#!/usr/bin/env python3
"""Compare one *case* of a large function against a reference address range.

`compare_asm.py` diffs one whole bcc32 `-S` function against a reference range.
That fails for a function built case by case: `Player_HandlePacket` is a single
226 KB function (one enormous `if`/`else` chain), and until every case exists its
frame size and instruction stream do not line up with the reference past the first
missing case, so a whole-function comparison desynchronises and reports thousands
of bogus mismatches.  This tool locates one reference *slice* inside the
reconstructed function and compares only that slice.

Design (see scripts/README.md for the full rationale):

  * The slice is found by **matching the anchor sequence** - the first
    `--anchor-len` (default 6) canonicalized reference instructions - against our
    function's instruction stream.  The anchor canonicalization is robust to the
    function-wide facts a partially reconstructed function cannot yet match:
    `[ebp±N]` local/argument offsets fold to `[ebp±SLOT]` and the value of an EH
    scope marker (`mov word ptr [ebp-N], imm`) folds to `MARK`.  Both are
    function-global (frame layout, cleanup-table byte offset) and cannot match
    until the whole function is complete.
  * Matching is a first-class, reported step.  The tool prints the anchor, the
    matched reference address, our instruction index and source line, and the
    *number of candidate matches*.  A short anchor is not unique wherever a
    case-dispatch prologue repeats (the first six canonical instructions of
    `Player_HandlePacket`, `cmp [ebp-SLOT],18` / `jne` / ..., occur verbatim in
    several `if (action == PacketAction_List)` cases), so the tool **extends the
    anchor one reference instruction at a time until exactly one candidate
    remains**, or the whole reference slice has been consumed.  Extension is
    evidence, never a tie-break: the returned anchor is a longer *exact*
    sequence.  **If the whole reference slice still matches more than one place
    it refuses (exit 2) with the candidate list**, because a wrong alignment
    that reports a plausible-looking mismatch count is the worst possible
    failure mode.
  * The comparison reuses `compare_asm`'s canonicalization, alias folding and
    strict-operand machinery (imported, not forked), so the two tools cannot
    drift apart.  By default the per-case comparison keeps opcodes, registers,
    constants, operand order and instruction count exact while wildcarding the
    function-global `[ebp±N]` slot numbers and EH marker values.  Passing
    `--frame-wild` or `--stack-delta` turns the slot wildcard off and reproduces
    `compare_asm`'s slot handling instead (frame wildcard / explicit shift), so
    the two tools are interchangeable for those flags.  EH marker *values* stay
    wildcarded in every mode (a partial reconstruction cannot match a
    function-global cleanup-table offset).
  * `--strict-operands` adds the value-behind-the-address check - a wrong string
    literal or a wrong direct callee is flagged - exactly as in `compare_asm`.

The default per-case mode is a **progress instrument, never an acceptance
criterion**: acceptance remains the unflagged whole-function `compare_asm.py`
comparison and, at the end, the whole-file MD5.

Usage:
    compare_case.py ASM UNIT FUNC REF_START REF_END [--strict-operands]
                    [--frame-wild] [--stack-delta D]

Example:
    compare_case.py build/Packets.asm Packets Player_HandlePacket \
        0x44e180 0x44f2c4

`UNIT` and `FUNC` name the reference function in
`analysis/target/functions.tsv`; the tool resolves its Borland-mangled
`source_name` for the listing.  A `FUNC` that already looks mangled (`@...`) is
used verbatim.
"""

import argparse
import difflib
import os
import re
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import compare_asm as ca  # noqa: E402  (path is fixed up immediately above)


ANCHOR_DEFAULT = 6

# The value half of an EH scope marker (`mov word ptr [ebp-N], imm`) is a byte
# offset into the function's cleanup table.  Like an address it is a
# function-global quantity, so per-case comparison folds it away.  Match it on
# the canonical text (`mov[ebp-N],imm`) of an instruction `marker_of` proved is
# a marker - never on a dword local store that only looks like one.
MARKER_VALUE_RE = re.compile(r",\d+$")
EBP_SLOT_RE = re.compile(r"\[ebp([+-])\d+\]")


def wild_marker(canon_ins: str, raw: str) -> str:
    if ca.marker_of(raw):
        return MARKER_VALUE_RE.sub(",MARK", canon_ins)
    return canon_ins


def wild_slots(canon_ins: str) -> str:
    return EBP_SLOT_RE.sub(r"[ebp\1SLOT]", canon_ins)


def case_norm(raw: str, slot_wild: bool) -> str:
    """Canon + the per-case relaxations (EH marker value, optionally slots)."""
    x = wild_marker(ca.canon(raw), raw)
    return wild_slots(x) if slot_wild else x


def resolve_prefix(func: str, unit: str, functions_tsv: str):
    """Resolve UNIT + FUNC to the Borland-mangled symbol in the listing.

    A FUNC already carrying a mangled prefix (`@...`) is returned unchanged.
    Otherwise the row for UNIT whose ref_name or source_name is FUNC supplies
    its `source_name`.  Returns (prefix, candidates); candidates lists
    `ref_name` for every matching row, so an ambiguous name can be reported
    rather than guessed.
    """
    if func.startswith("@"):
        return func, [func]
    rows = []
    try:
        with open(functions_tsv, encoding="utf-8") as fh:
            next(fh, None)
            for line in fh:
                p = line.rstrip("\n").split("\t")
                if len(p) < 8 or p[0] != unit:
                    continue
                if func in (p[5], p[7]):
                    rows.append((p[5], p[7]))
    except OSError:
        return func, []
    if len(rows) == 1:
        source = rows[0][1] or rows[0][0]
        return source, [rows[0][0]]
    return None, [r[0] for r in rows]


def locate(our_norm, ref_norm, anchor_len: int):
    """Return (anchor, candidate_index_list) for the *shortest unique* anchor.

    The anchor starts as the first `anchor_len` canonicalized reference
    instructions.  When that prefix matches more than one place it is
    **extended** one reference instruction at a time, keeping only the
    candidates that continue to match, until exactly one remains or the whole
    reference slice has been consumed.

    Extension is evidence, not a tie-break: the returned anchor is a longer
    *exact* sequence, so a slice whose code is genuinely repeated still ends
    with more than one candidate and the caller refuses it.  This is what
    disambiguates the case-dispatch prologue of `Player_HandlePacket`, whose
    first six canonical instructions (`cmp [ebp-SLOT],18` / `jne` / ... ) occur
    verbatim in several `if (action == PacketAction_List)` cases; the sites
    diverge a few instructions later, so a longer anchor is unique while the
    six-instruction default is not.  Taking `cands[0]` at the default length
    would silently align to the wrong case.
    """
    if not ref_norm:
        return [], []
    n = min(anchor_len, len(ref_norm))
    anchor = ref_norm[:n]
    cands = [i for i in range(len(our_norm) - n + 1)
             if our_norm[i:i + n] == anchor]
    m = n
    while len(cands) > 1 and m < len(ref_norm):
        want = ref_norm[m]
        cands = [i for i in cands
                 if i + m < len(our_norm) and our_norm[i + m] == want]
        m += 1
    return ref_norm[:m], cands


def structural_hunks(ref, our):
    """Alignment hunks that change structure (not same-length substitutions).

    A `replace` of equal length is a value substitution (a constant or a
    wildcarded marker); an `insert`, a `delete`, or a `replace` of unequal
    length changes the instruction count and is what a per-case comparison must
    surface as a real structural difference.
    """
    sm = difflib.SequenceMatcher(None, ref, our, autojunk=False)
    return [op for op in sm.get_opcodes()
            if op[0] in ("insert", "delete")
            or (op[0] == "replace" and (op[2] - op[1]) != (op[4] - op[3]))]


def parse_our_with_lines(asm: str, prefix: str):
    """(out, calls, mk, raw, lines) for our function, or (None, ...)."""
    out, calls, mk, raw = ca._parse_our(asm, prefix)
    if out is None:
        return None, [], [], [], []
    lines = []
    try:
        _, _, (_, line_list, _) = ca.parse_our_lines(asm, prefix)
        if len(line_list) == len(out):
            lines = line_list
    except Exception:
        lines = []
    return out, calls, mk, raw, lines


def count_mismatches(ref, our):
    """(mismatch count, [(index, ref, our), ...]) for positional comparison."""
    n = max(len(ref), len(our))
    pairs = []
    mism = 0
    for i in range(n):
        r = ref[i] if i < len(ref) else "<none>"
        o = our[i] if i < len(our) else "<none>"
        if r != o:
            mism += 1
        pairs.append((i, r, o))
    return mism, pairs


def trim_trailing_nops(seq, raws):
    """Drop trailing `nop` padding from a canonical stream and its raw partner."""
    seq, raws = list(seq), list(raws)
    while seq and seq[-1] == "nop":
        seq.pop()
        raws.pop()
    return seq, raws


def trim_prologue_pushes(seq, raws):
    """Drop callee-saved saves from the first 8 instructions, as compare_asm does.

    Only applied when the located slice is the function start (index 0): a
    mid-function case slice does not begin at the prologue, so a literal
    `push ebx` there is body code and must stay.
    """
    head = min(8, len(seq))
    keep = [i for i in range(head)
            if seq[i] not in ("push ebx", "push esi", "push edi")]
    if len(keep) == head:
        return list(seq), list(raws)
    return ([seq[i] for i in keep] + list(seq[head:]),
            [raws[i] for i in keep] + list(raws[head:]))


# ---------------------------------------------------------------------------
# Self-test
#
# Synthetic streams prove the properties that make the locator safe to trust:
# (a) an anchor whose *whole* reference slice is repeated is refused, (b) a
# perturbed instruction is reported, (c) a correct slice reports zero
# mismatches, (d) a repeated short prefix is extended until it is unique
# instead of silently taking the first candidate, and (e) a bcc32 `-S` function
# body with an in-function data region (a jump/exception table emitted as
# `dd`/`db` between real instructions) parses to the real instructions only.
# It runs without a build, so it can gate the tool in any environment.

_SELFTEST_REF = [
    "push ebp",
    "mov ebp,esp",
    "cmp[ebp-SLOT],35",
    "jne ADDR",
    "lea eax,[ebp+SLOT]",
    "call ADDR",
    "xor eax,eax",
    "ret",
]

# A synthetic bcc32 `-S` listing: two blocks of real instructions with a data
# region (the `@1 label dword` / `dd` / `db` jump table) in between.  The data
# region must contribute no instructions; if the parser mistook its
# instruction-shaped bytes for code it would inject duplicate anchors.
_SELFTEST_LISTING = """\
_TEXT\tsegment
@Selftest$qv proc near
\tpush ebp
\tmov ebp, esp
\t?debug L 10
\tcmp dword ptr [ebp-4], 18
\tjne @3
\txor eax, eax
@1 label dword
\tdd 0
\tdd @2
\tdb 90h
@2:
\tret
@3:
\tret
@Selftest$qv endp
_TEXT\tends
\tend
"""
_SELFTEST_LISTING_CODE = [
    "push ebp",
    "mov ebp,esp",
    "cmp[ebp-4],18",
    "jne ADDR",
    "xor eax,eax",
    "ret",
    "ret",
]


def selftest() -> int:
    failures = []

    # (a) the whole reference slice is repeated: extension cannot help, so the
    #     two candidates must survive and the caller must refuse.
    stream = list(_SELFTEST_REF) + ["mov eax,7"] + list(_SELFTEST_REF)
    anchor, cands = locate(stream, _SELFTEST_REF, ANCHOR_DEFAULT)
    if cands == [0, 9] and len(anchor) == len(_SELFTEST_REF):
        print("selftest (a) fully repeated slice: 2 candidates at [0, 9] -> "
              "refused")
    else:
        failures.append(f"(a) expected candidates [0, 9] at full length, "
                        f"got {cands} at length {len(anchor)}")

    # A unique anchor must yield exactly one candidate at the requested length.
    unique = list(_SELFTEST_REF)
    anchor, cands = locate(unique, _SELFTEST_REF, ANCHOR_DEFAULT)
    if cands == [0] and len(anchor) == ANCHOR_DEFAULT:
        print("selftest (a) unique anchor: 1 candidate at [0]")
    else:
        failures.append(f"(a) expected candidates [0] at length "
                        f"{ANCHOR_DEFAULT}, got {cands} at length {len(anchor)}")

    # (b) a deliberately perturbed instruction is reported.
    perturbed = list(unique)
    perturbed[2] = "cmp[ebp-SLOT],36"
    mism, _ = count_mismatches(_SELFTEST_REF, perturbed)
    if mism == 1:
        print("selftest (b) perturbed instruction: 1 mismatch reported")
    else:
        failures.append(f"(b) expected 1 mismatch, got {mism}")

    # (c) the correct slice reports zero mismatches.
    mism, _ = count_mismatches(_SELFTEST_REF, unique)
    if mism == 0:
        print("selftest (c) correct slice: 0 mismatches")
    else:
        failures.append(f"(c) expected 0 mismatches, got {mism}")

    # (d) a repeated short prefix, uniquified only by a few later instructions:
    #     the locator must extend the anchor, not take the first candidate.
    prefix = list(_SELFTEST_REF[:6])
    tail_a = ["add ecx,1", "ret"]
    tail_b = ["sub ecx,1", "ret"]
    ref_slice = prefix + tail_a
    repeat_stream = ref_slice + ["mov eax,7"] + prefix + tail_b
    anchor, cands = locate(repeat_stream, ref_slice, ANCHOR_DEFAULT)
    if cands == [0] and len(anchor) == len(ref_slice) - 1:
        print(f"selftest (d) repeated prefix: anchor extended {ANCHOR_DEFAULT} "
              f"-> {len(anchor)}, 1 candidate at [0]")
    else:
        failures.append(f"(d) expected 1 candidate at [0] with anchor length "
                        f"{len(ref_slice) - 1}, got {cands} at length "
                        f"{len(anchor)}")

    # (e) an in-function data region must not be treated as instructions.
    try:
        fd, tmp = tempfile.mkstemp(suffix=".asm")
        os.close(fd)
        with open(tmp, "w", encoding="latin1") as fh:
            fh.write(_SELFTEST_LISTING)
        try:
            out, calls, mk, raw = ca._parse_our(tmp, "@Selftest$qv")
            out2, _, _, raw2, lines2 = parse_our_with_lines(tmp, "@Selftest$qv")
        finally:
            os.unlink(tmp)
        if out == _SELFTEST_LISTING_CODE and out2 == _SELFTEST_LISTING_CODE:
            if len(out2) == len(lines2) == len(raw2):
                print("selftest (e) in-function data region: excluded "
                      f"({len(out)} real instructions parsed)")
            else:
                failures.append("(e) instruction/line/raw lengths disagree: "
                                f"{len(out2)}/{len(lines2)}/{len(raw2)}")
        else:
            failures.append(f"(e) expected {_SELFTEST_LISTING_CODE}, got {out}")
    except OSError as exc:
        failures.append(f"(e) could not exercise the parser: {exc}")

    if failures:
        for f in failures:
            print(f"selftest FAIL: {f}", file=sys.stderr)
        return 3
    print("selftest OK (5/5)")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("asm", nargs="?")
    ap.add_argument("unit", nargs="?")
    ap.add_argument("func", nargs="?")
    ap.add_argument("ref_start", nargs="?", type=lambda v: int(v, 0))
    ap.add_argument("ref_end", nargs="?", type=lambda v: int(v, 0))
    ap.add_argument("--ref-bin", default="GameServer.exe")
    ap.add_argument("--functions-tsv", default="analysis/target/functions.tsv",
                    help="reference function inventory (mangled source_name) used "
                         "to resolve UNIT+FUNC and for strict call-target identity")
    ap.add_argument("--ghidra-tsv", default="analysis/ghidra/functions.tsv",
                    help="read-only Ghidra inventory, used to label strict library "
                         "call targets")
    ap.add_argument("--anchor-len", type=int, default=ANCHOR_DEFAULT,
                    help="minimum number of leading canonicalized reference "
                         "instructions used to locate the slice (default "
                         "%(default)s); the anchor is extended one reference "
                         "instruction at a time until it matches exactly once or "
                         "the whole reference slice is consumed")
    ap.add_argument("--strict-operands", action="store_true",
                    help="STRICT comparison: additionally compare the VALUE behind "
                         "each canonicalized address operand - string/data literal "
                         "bytes, direct call targets (by callee identity) and "
                         "non-address immediates. The default mode canonicalizes "
                         "addresses and therefore cannot see a wrong literal or a "
                         "wrong callee; this mode can.")
    ap.add_argument("--frame-wild", action="store_true",
                    help="progress mode: wildcard the prologue frame instruction "
                         "(and matching epilogue release) and turn off the per-case "
                         "slot wildcard, so slot handling matches compare_asm.py. "
                         "NEVER an acceptance criterion.")
    ap.add_argument("--stack-delta", type=lambda v: int(v, 0), default=None,
                    help="progress mode: shift our `[ebp-N]` by this explicit delta "
                         "and turn off the per-case slot wildcard (compare_asm.py "
                         "--stack-delta semantics). NEVER an acceptance criterion.")
    ap.add_argument("--markers", action="store_true",
                    help="always print the EH scope marker streams")
    ap.add_argument("--diff", action="store_true",
                    help="always print the aligned instruction diff")
    ap.add_argument("--calls", action="store_true",
                    help="print the call symbol names in our listing")
    ap.add_argument("--selftest", action="store_true",
                    help="run the locator (repeated-slice refusal, anchor "
                         "extension), perturbation, clean-slice and in-function "
                         "data-region self-test and exit (does not touch the "
                         "build)")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if None in (args.asm, args.unit, args.func, args.ref_start, args.ref_end):
        ap.error("asm, unit, func, ref_start and ref_end are required")
    if args.ref_end <= args.ref_start:
        print("REF_END must be greater than REF_START")
        return 2
    if args.anchor_len < 2:
        print("--anchor-len must be at least 2 (a one-instruction anchor cannot "
              "be unique)")
        return 2

    prefix, rows = resolve_prefix(args.func, args.unit, args.functions_tsv)
    if prefix is None:
        if rows:
            print(f"FUNC {args.func!r} is ambiguous in unit {args.unit}: "
                  f"{', '.join(rows)}", file=sys.stderr)
        else:
            print(f"FUNC {args.func!r} not found in unit {args.unit} in "
                  f"{args.functions_tsv}", file=sys.stderr)
        return 2

    out, calls, _mk, raw, lines = parse_our_with_lines(args.asm, prefix)
    if out is None:
        print(f"function {prefix} not found in {args.asm}")
        return 2
    ref, _ref_mk, ref_raw = ca._parse_ref(args.ref_bin, args.ref_start,
                                          args.ref_end)
    if not ref:
        print(f"reference range {args.ref_start:#x}..{args.ref_end:#x} is empty")
        return 2

    print(f"compare_case: {args.asm}  unit {args.unit}  func {args.func}")
    print(f"  symbol: {prefix}")
    print(f"  reference range: {args.ref_start:#x}..{args.ref_end:#x} "
          f"({len(ref)} instructions)")

    # --- locate: first-class, reported, and loud on failure ---
    our_anchor = [case_norm(r, True) for r in raw]
    ref_anchor = [case_norm(r, True) for (_, r) in ref_raw]
    base_len = min(args.anchor_len, len(ref_anchor))
    anchor, cands = locate(our_anchor, ref_anchor, args.anchor_len)
    print(f"\nanchor: {len(anchor)} canonical instruction(s), "
          f"{len(cands)} candidate match(es)")
    if len(anchor) > base_len:
        print(f"  (extended from {base_len} to {len(anchor)} instructions to "
              f"disambiguate repeated code)")
    for k, a in enumerate(anchor):
        print(f"  {k + 1}: {a}")
    if len(cands) != 1:
        if not cands:
            print(f"anchor did not match anywhere in {prefix}. Refusing to "
                  f"compare (the case may not be written yet, or the leading "
                  f"instructions differ).")
        else:
            shown = ", ".join(str(c) for c in cands[:20])
            print(f"anchor is ambiguous even at the full reference slice "
                  f"({len(cands)} matches at indices {shown}); refusing to "
                  f"compare the wrong slice — the requested code is genuinely "
                  f"repeated. Narrow the reference range to a non-repeated "
                  f"region.")
        return 2
    idx = cands[0]
    line = lines[idx] if idx < len(lines) else "?"
    print(f"  matched reference start {args.ref_start:#x} at our instruction "
          f"index {idx} (source line L{line})")
    print(f"  our first instruction: {raw[idx].strip()}")

    # --- build the compared streams ---
    slot_wild = not (args.frame_wild or args.stack_delta is not None)
    n = len(ref)
    ref_cmp = [case_norm(r, slot_wild) for (_, r) in ref_raw]
    our_cmp = [case_norm(r, slot_wild) for r in raw[idx:idx + n]]
    ref_raw_t = list(ref_raw)
    our_raw_t = list(raw[idx:idx + n])
    ref_cmp, ref_raw_t = trim_trailing_nops(ref_cmp, ref_raw_t)
    our_cmp, our_raw_t = trim_trailing_nops(our_cmp, our_raw_t)
    if idx == 0:
        ref_cmp, ref_raw_t = trim_prologue_pushes(ref_cmp, ref_raw_t)
        our_cmp, our_raw_t = trim_prologue_pushes(our_cmp, our_raw_t)

    if slot_wild:
        print("\nmode: per-case structural (PROGRESS instrument; NOT an "
              "acceptance criterion)")
        print("  [ebp±N] slot numbers and EH scope marker values are wildcarded "
              "(function-global facts a partial function cannot match)")
    else:
        print("\nmode: compare_asm-compatible slot handling (PROGRESS instrument; "
              "NOT an acceptance criterion)")
        print("  EH scope marker values are wildcarded")

    if args.frame_wild:
        ref_cmp, frame_ref = ca.frame_wild(ref_cmp)
        our_cmp, frame_our = ca.frame_wild(our_cmp)
        if frame_ref is not None and frame_our is not None:
            d = frame_our - frame_ref
            print(f"frame: ref -{abs(frame_ref):#x} ours -{abs(frame_our):#x} "
                  f"(delta {d:#x})")
        else:
            print("frame: no prologue in the located slice; --frame-wild has no "
                  "effect")
    if args.stack_delta is not None:
        our_cmp = ca.shift_slots(our_cmp, args.stack_delta)
        print(f"stack delta applied: {args.stack_delta:+#x}")
        print(f"slot order  ref: {ca.slot_sequence(ref_cmp)}")
        print(f"slot order ours: {ca.slot_sequence(our_cmp)} (ordering is strict)")

    mism, pairs = count_mismatches(ref_cmp, our_cmp)
    for i, r, o in pairs:
        mark = "  " if r == o else "!!"
        print(f"{mark} {i:3d} ref: {r:44s} our: {o}")

    struct = structural_hunks(ref_cmp, our_cmp)
    print(f"\n{args.unit}.{args.func} case {args.ref_start:#x}..{args.ref_end:#x}: "
          f"{len(ref_cmp)} ref / {len(our_cmp)} our instructions, "
          f"{mism} mismatched")
    if struct:
        print(f"structural: {len(struct)} insertion/deletion hunk(s) "
              f"(see the aligned diff)")
    else:
        print("structural: clean (no insertions or deletions; only same-length "
              "substitutions)")
    if slot_wild:
        print("NOTE: the per-case slot wildcard makes this a PROGRESS "
              "instrument; acceptance is the unflagged whole-function "
              "compare_asm.py comparison plus the whole-file MD5.")

    if args.calls:
        print("calls: " + (", ".join(sorted(set(calls))) or "(none)"))
    warn = ca.call_warnings(calls)
    if warn:
        print("WARNING: array new/delete calls present (verify against the "
              "reference; scalar forms canonicalize identically): "
              + ", ".join(sorted(set(warn))))
    if args.diff or struct:
        ca.print_diff(ref_cmp, our_cmp)

    ref_mk = [ca.marker_of(r) for (_, r) in ref_raw_t if ca.marker_of(r)]
    our_mk = [ca.marker_of(r) for r in our_raw_t if ca.marker_of(r)]
    ca.print_markers(ref_mk, our_mk, force=args.markers or mism > 0)

    strict_diffs = []
    strict_counts = None
    if args.strict_operands:
        # EH scope markers are wildcarded above; their value is a function-global
        # cleanup-table offset, not a literal this slice can verify.  Blank the
        # marker instructions for the strict pass so they are not reported as
        # immediate differences (they carry no other address-bearing operand).
        sref = [(a, "nop" if ca.marker_of(r) else r) for (a, r) in ref_raw_t]
        sour = ["nop" if ca.marker_of(r) else r for r in our_raw_t]
        blobs = ca.parse_data_blobs(args.asm)
        ctx = ca.StrictContext(args.ref_bin, args.functions_tsv, args.ghidra_tsv,
                               args.asm, blobs)
        strict_diffs, strict_counts = ca.strict_compare(
            ctx, sref, sour, ref_cmp, our_cmp)
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
    ok = (mism == 0 and len(ref_cmp) == len(our_cmp) and not struct
          and not strict_bad)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
