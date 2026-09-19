#!/usr/bin/env python3
"""asm2cpp.py - draft C++ transcription aid for the GameServer reconstruction.

Given a reference address range, walk the disassembly and emit a *draft* C++
skeleton that names the idioms this project has already verified (the
AnsiString RTL table, the EO_* packet helpers, header-declared field offsets,
basic control flow).  Every emitted line carries the reference address it came
from; anything the tool cannot resolve with confidence is emitted as a
`// TODO(addr): <raw instruction>` comment instead of being guessed.

THE OUTPUT IS A DRAFT.  It is a starting point, never an authority.  Verify with
`python3 scripts/compare_asm.py` before committing anything.
"""

import argparse
import bisect
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DISASM = os.path.join(REPO, "analysis", "target", "disasm.txt")
SIGS = os.path.join(REPO, "analysis", "target", "functions.tsv")
SRC = os.path.join(REPO, "src")

# ---------------------------------------------------------------------------
# Verified idiom tables (see AGENTS.md "String/AnsiString ABI (verified)").
# ---------------------------------------------------------------------------

ANSI_OPS = {
    0x40240C: ("ctor_default", "AnsiString()"),
    0x5591A8: ("ctor_cstr", "AnsiString(const char*)"),
    0x559278: ("ctor_char", "AnsiString(char)"),
    0x5591E0: ("ctor_copy", "AnsiString(const AnsiString&)"),
    0x559338: ("assign", "operator="),
    0x5593F0: ("ne", "operator!="),
    0x5593D8: ("eq", "operator=="),
    0x559308: ("dtor", "~AnsiString()"),
    0x559594: ("substr", "SubString(int,int)"),
    0x559474: ("insert", "Insert(const AnsiString&,int)"),
    0x40243C: ("length", "Length()"),
    0x55967C: ("join2", "operator+ (2-way)"),
    0x55934C: ("join3", "operator+ (3-way)"),
    0x402460: ("datap", "c_str()"),
}

EO_CALLS = {
    0x470DE8: "EO_DecodeNumber",
    0x470B9C: "EO_EncodeNumber",
    0x4728E0: "EO_GetBreakByte",
    0x472990: "PacketReader_Init",
    0x472A08: "PacketReader_GetBreakString",
    0x472B60: "PacketReader_GetBreakStringAt",
    0x4728F8: "PacketReader_AddByte",
    0x46A9A0: "Math_Abs",
}

HELPERS = {
    0x551B20: "/* float->int truncate */ (int)",
    0x520500: "Now()",
    0x51FF7C: "DateTimeToTimeStamp",
    0x54BA98: "__InitExceptBlockLDTC",
    0x541ED0: "operator delete",
    0x541F04: "operator new",
    0x55623D: "__CatchCleanup",
    0x55921C: "AnsiString RTL helper (0x55921c)",
    0x559C04: "GetSystemTime/GetLocalTime",
}

# Conditional-jump mnemonic -> C comparison when the preceding cmp/test is
# `cmp a, b` (signed unless noted).
JCC_SIGNED = {
    "je": "==", "jne": "!=", "jz": "==", "jnz": "!=",
    "jl": "<", "jnge": "<", "jle": "<=", "jng": "<=",
    "jg": ">", "jnle": ">", "jge": ">=", "jnl": ">=",
    "jb": "<", "jnae": "<", "jbe": "<=", "jna": "<=",
    "ja": ">", "jnbe": ">", "jae": ">=", "jnb": ">=",
}
JCC_UNSIGNED = set("jb jnae jbe jna ja jnbe jae jnb".split())


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

INSN_RE = re.compile(r"^\s*([0-9a-fA-F]+):\s+((?:[0-9a-fA-F]{2} )+)\s*(.*)$")
RAW_RE = re.compile(r"^\s*([0-9a-fA-F]+):\s+(.*)$")


def parse_listing(text):
    """Return [(addr, mnemonic, operands, raw_bytes)] from a bcc/objdump listing."""
    out = []
    for line in text.splitlines():
        m = INSN_RE.match(line)
        if m:
            addr = int(m.group(1), 16)
            raw = m.group(2).strip()
            rest = m.group(3).strip()
        else:
            m = RAW_RE.match(line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            raw = ""
            rest = m.group(2).strip()
        parts = rest.split(None, 1)
        if not parts:
            continue
        mn = parts[0]
        ops = parts[1].strip() if len(parts) > 1 else ""
        out.append((addr, mn, ops, raw))
    return out


def load_listing(path, start, end):
    with open(path, "r", errors="replace") as fh:
        text = fh.read()
    return [i for i in parse_listing(text) if start <= i[0] < end]


def regen_with_objdump(start, end):
    import subprocess
    exe = os.path.join(REPO, "GameServer.exe")
    cmd = ["objdump", "-d", "-M", "intel", "--no-show-raw-insn",
           "--start-address=0x%x" % start, "--stop-address=0x%x" % end, exe]
    txt = subprocess.run(cmd, capture_output=True, text=True).stdout
    return [i for i in parse_listing(txt) if start <= i[0] < end]


# ---------------------------------------------------------------------------
# Header field-offset table
# ---------------------------------------------------------------------------

FIELD_RE = re.compile(r"//\s*\+0x([0-9a-fA-F]+)")
IDENT_RE = re.compile(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*;?\s*$")


def load_field_names(src_dir):
    """offset -> set(names) parsed from the `// +0xNN` comments in src/*.h."""
    table = {}
    if not os.path.isdir(src_dir):
        return table
    for fn in sorted(os.listdir(src_dir)):
        if not fn.endswith(".h"):
            continue
        for line in open(os.path.join(src_dir, fn), errors="replace"):
            m = FIELD_RE.search(line)
            if not m:
                continue
            head = line[: m.start()]
            im = IDENT_RE.search(head.rstrip())
            if not im:
                continue
            name = im.group(1)
            if name in ("int", "char", "bool", "short", "void", "unsigned", "long"):
                continue
            table.setdefault(int(m.group(1), 16), set()).add(name)
    return table


def field_name(table, off):
    names = table.get(off)
    if not names:
        return None
    name = sorted(names)[0]
    if name.startswith("field_0x"):
        return None
    return name


# ---------------------------------------------------------------------------
# Callee identity
# ---------------------------------------------------------------------------

def load_sigs(path):
    names = {}
    try:
        with open(path, errors="replace") as fh:
            next(fh, "")
            for line in fh:
                c = line.rstrip("\n").split("\t")
                if len(c) < 6:
                    continue
                try:
                    names[int(c[2], 16)] = c[5]
                except ValueError:
                    continue
    except OSError:
        pass
    return names


def load_idents(path):
    """Accept a markdown table (| `0xADDR` | name | ... |) or `addr name` lines."""
    idents = {}
    if not path:
        return idents
    try:
        fh = open(path, errors="replace")
    except OSError:
        return idents
    with fh:
        for line in fh:
            m = re.match(r"\s*\|\s*`?(0x[0-9a-fA-F]+)`?\s*\|", line)
            if m:
                cells = [c.strip() for c in line.strip().strip("|").split("|")]
                if len(cells) >= 2 and cells[1] and cells[1] not in ("identity", "---"):
                    idents[int(m.group(1), 16)] = cells[1]
                continue
            m = re.match(r"\s*(0x[0-9a-fA-F]+)\s+(\S.*?)\s*$", line)
            if m:
                idents[int(m.group(1), 16)] = m.group(2)
    return idents


# ---------------------------------------------------------------------------
# Emitter
# ---------------------------------------------------------------------------

class Draft:
    def __init__(self, fields, sigs, idents, start):
        self.fields = fields
        self.sigs = sigs
        self.idents = idents
        self.start = start
        self.lines = []
        self.stats = {}
        self.todos = 0
        self.n = 0
        self.pending_cmp = None
        self.tmp = 0
        self.depth = 0

    def hit(self, kind):
        self.stats[kind] = self.stats.get(kind, 0) + 1

    def emit(self, addr, text, kind=None):
        self.lines.append("    " * self.depth + text)
        if kind:
            self.hit(kind)

    def todo(self, addr, text):
        self.lines.append("    " * self.depth + "// TODO(0x%x): %s" % (addr, text))
        self.todos += 1
        self.hit("todo")

    def callee_name(self, addr):
        if addr in self.idents:
            return self.idents[addr]
        if addr in self.sigs:
            return self.sigs[addr]
        if addr in EO_CALLS:
            return EO_CALLS[addr]
        return None

    # -- operand helpers ---------------------------------------------------
    def deref(self, op):
        """Describe a memory operand as (base, index, scale, disp)."""
        m = re.search(r"\[([^\]]+)\]", op)
        if not m:
            return None
        expr = m.group(1).replace(" ", "")
        base = None
        index = None
        scale = 1
        disp = 0
        for sign, term in re.findall(r"([+-]?)([^+-]+)", expr):
            if not term:
                continue
            mm = re.match(r"(\w+)\*(\d+)$", term)
            if mm:
                index, scale = mm.group(1), int(mm.group(2))
                continue
            if re.match(r"^(0x[0-9a-fA-F]+|\d+)$", term):
                v = int(term, 16) if term.startswith("0x") else int(term)
                disp += -v if sign == "-" else v
                continue
            if base is None:
                base = term
            else:
                index = term
        return (base, index, scale, disp)

    def field_expr(self, op):
        d = self.deref(op)
        if not d:
            return None
        base, index, scale, disp = d
        if index or base == "ebp" or disp < 0:
            return None  # array indexing / stack local - not a scalar field
        nm = field_name(self.fields, disp)
        if nm:
            return "/*%s*/%s" % (base, nm)
        return None

    # -- main walk ---------------------------------------------------------
    def walk(self, insns):
        end = insns[-1][0] + 1 if insns else self.start
        i = 0
        while i < len(insns):
            addr, mn, ops, raw = insns[i]
            self.n += 1
            text = ("%s %s" % (mn, ops)).strip()
            nxt = insns[i + 1] if i + 1 < len(insns) else None

            # --- calls ----------------------------------------------------
            m = re.match(r"0x([0-9a-fA-F]+)", ops)
            if mn == "call" and m:
                tgt = int(m.group(1), 16)
                self.emit_call(addr, tgt, ops)
                i += 1
                continue

            # --- AnsiString inlined length: cmp dword [reg],0 / mov reg,[reg-4]
            if mn == "cmp" and re.search(r"dword ptr \[\w+\],\s*0$", ops):
                self.emit(addr, "// Length() != 0 (inlined)  // 0x%x" % addr, "ansi_inline_len")
                i += 1
                continue
            if mn == "mov" and re.search(r"\[\w+\s*-\s*0x4\]", ops):
                self.emit(addr, "// [Data-4] = Length() (inlined)  // 0x%x" % addr, "ansi_inline_len")
                i += 1
                continue

            # --- control flow ---------------------------------------------
            if mn.startswith("j") and re.match(r"0x[0-9a-fA-F]+", ops) and mn != "jmp":
                self.emit_jcc(addr, mn, ops, raw)
                i += 1
                continue
            if mn == "jmp" and re.match(r"0x[0-9a-fA-F]+", ops):
                tgt = int(ops.split()[0], 16)
                self.emit(addr, "// jmp 0x%x  // 0x%x" % (tgt, addr), "branch")
                self.pending_cmp = None
                i += 1
                continue
            if mn == "jmp":
                self.emit(addr, "// jmp %s  // 0x%x" % (ops, addr), "branch")
                i += 1
                continue

            # --- EH arming / temp counter (documented fingerprints) -------
            if mn == "mov" and re.match(r"word ptr \[ebp - 0x[0-9a-f]+\], 0x", ops):
                self.emit(addr, "// EH arm scope (marker)  // 0x%x" % addr, "eh_scope")
                i += 1
                continue
            if mn in ("inc", "dec", "add") and re.match(r"(dword|byte) ptr \[ebp - 0x[0-9a-f]+\]", ops):
                self.emit(addr, "// EH temp-counter %s  // 0x%x" % (mn, addr), "eh_counter")
                i += 1
                continue

            # --- comparisons feeding a later branch ------------------------
            if mn in ("cmp", "test"):
                self.pending_cmp = (addr, mn, ops)
                self.emit(addr, "// cmp: %s  // 0x%x" % (ops, addr), "cmp")
                i += 1
                continue

            # --- field load/store -----------------------------------------
            if mn in ("mov", "movzx", "movsx", "add", "sub", "inc", "dec", "or", "and", "xor"):
                if self.emit_mem(addr, mn, ops):
                    i += 1
                    continue

            # --- ret / epilogue -------------------------------------------
            if mn == "ret":
                self.emit(addr, "return /* eax */;  // 0x%x" % addr, "ret")
                i += 1
                continue
            if mn in ("push", "pop"):
                self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "stack")
                i += 1
                continue
            if mn in ("nop", "int3"):
                self.emit(addr, "// nop  // 0x%x" % addr, "nop")
                i += 1
                continue

            # --- register / immediate moves -------------------------------
            if mn in ("mov", "movzx", "movsx", "lea"):
                if self.emit_reg_move(addr, mn, ops):
                    i += 1
                    continue
            # --- stack adjustment / pointer arithmetic --------------------
            if mn in ("add", "sub") and re.match(r"esp,\s*-?0x", ops):
                self.emit(addr, "// stack adjust %s  // 0x%x" % (ops, addr), "stack_adjust")
                i += 1
                continue
            # --- x87 ------------------------------------------------------
            if mn.startswith("f") or mn in ("wait",):
                self.emit(addr, "// x87: %s  // 0x%x" % (text, addr), "x87")
                i += 1
                continue
            # --- bool materialisation -------------------------------------
            if mn in ("sete", "setne", "setl", "setg", "setle", "setge", "setb", "seta"):
                self.emit(addr, "// %s (materialise bool)  // 0x%x" % (text, addr), "setcc")
                i += 1
                continue
            # --- arithmetic -------------------------------------------------
            if mn in ("imul", "idiv", "div", "mul", "neg", "not", "shl", "shr", "sar",
                      "cdq", "adc", "sbb", "xchg"):
                self.emit(addr, "// %s  // 0x%x" % (text, addr), "arith")
                i += 1
                continue

            # --- any remaining memory operand is a stack local/arg ---------
            if "[ebp - " in ops or "[ebp + " in ops:
                self.emit(addr, "// %s  // 0x%x" % (text, addr), "local")
                i += 1
                continue
            if " ptr [" in ops:
                self.emit(addr, "// %s  // 0x%x" % (text, addr), "mem")
                i += 1
                continue
            # --- remaining register-form arithmetic ------------------------
            if re.match(r"\w+, \w+$", ops) and mn in ("xor", "add", "sub", "and", "or",
                                                        "cmp", "mov", "test", "imul",
                                                        "shl", "shr", "sar"):
                self.emit(addr, "// %s  // 0x%x" % (text, addr), "regmove")
                i += 1
                continue
            if mn in ("inc", "dec", "neg", "not") and re.match(r"\w+$", ops):
                self.emit(addr, "// %s  // 0x%x" % (text, addr), "regmove")
                i += 1
                continue
            if "fs:[0x0]" in ops:
                self.emit(addr, "// EH restore fs:[0]  // 0x%x" % addr, "eh_restore")
                i += 1
                continue
            if False:
                self.emit(addr, "// %s  // 0x%x" % (text, addr), "regmove")
                i += 1
                continue

            self.todo(addr, text)
            i += 1

    def emit_call(self, addr, tgt, ops):
        if tgt in ANSI_OPS:
            kind, sig = ANSI_OPS[tgt]
            self.emit(addr, "// AnsiString::%s  // 0x%x" % (sig, addr), "ansi_" + kind)
            return
        if tgt in HELPERS:
            self.emit(addr, "// %s  // 0x%x" % (HELPERS[tgt], addr), "helper")
            return
        # local (intra-range) call = a control structure, not an idiom
        nm = self.callee_name(tgt)
        if nm:
            self.emit(addr, "%s(...);  // 0x%x" % (nm, addr), "call_named")
        else:
            self.todo(addr, "call 0x%x (unidentified)" % tgt)

    def emit_jcc(self, addr, mn, ops, raw):
        tgt = int(ops.split()[0], 16)
        cond = JCC_SIGNED.get(mn, "?")
        if self.pending_cmp:
            caddr, cmn, cops = self.pending_cmp
            self.emit(addr,
                      "if (/* %s */ ... %s ...) goto L_0x%x;  // 0x%x (cond from 0x%x)"
                      % (cops.replace("dword ptr ", "").replace("byte ptr ", ""),
                         cond, tgt, addr, caddr),
                      "jcc_with_cmp")
        else:
            self.emit(addr, "if (/* flags */) goto L_0x%x;  // 0x%x (%s)"
                      % (tgt, addr, mn), "jcc")
        self.pending_cmp = None

    def emit_reg_move(self, addr, mn, ops):
        m = re.match(r"(\w+),\s*(.+)", ops)
        if not m:
            return False
        dst, src = m.group(1), m.group(2).strip()
        if src.startswith("[") and "ebp" in src:
            self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "local")
            return True
        if re.match(r"(0x[0-9a-fA-F]+|\d+)$", src):
            self.emit(addr, "%s = %s;  // 0x%x" % (dst, src, addr), "imm")
            return True
        if mn == "lea" and self.deref(dst):
            pass
        if re.match(r"\w+$", src):
            self.emit(addr, "%s = %s;  // 0x%x" % (dst, src, addr), "regmove")
            return True
        if src.startswith("[") or dst.startswith("["):
            self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "mem")
            return True
        return False

    def emit_mem(self, addr, mn, ops):
        m = re.match(r"(\w+),\s*(.+)", ops)
        if not m:
            return False
        dst, src = m.group(1), m.group(2).strip()
        ds = self.field_expr(dst)
        ss = self.field_expr(src)
        if ds or ss:
            self.emit(addr, "%s = %s;  // 0x%x"
                      % (ds or ("/*%s*/" % dst), ss or src, addr), "field")
            return True
        if "[ebp - " in dst or "[ebp - " in src or "[ebp + " in dst or "[ebp + " in src:
            self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "local")
            return True
        return False


# ---------------------------------------------------------------------------
# Selftest
# ---------------------------------------------------------------------------

SELFTEST = [
    ("EO_Encode_Interleave", 0x470EF0, 0x4711E7),
    ("Walk_BuildReply", 0x45D874, 0x45DDF0),
    ("Player_SerializePaperdoll", 0x460068, 0x4607C8),
]


def run_range(start, end, args, fields, sigs, idents, verbose):
    if args.objdump:
        insns = regen_with_objdump(start, end)
    else:
        insns = load_listing(args.listing, start, end)
    if not insns:
        insns = regen_with_objdump(start, end)
    d = Draft(fields, sigs, idents, start)
    d.walk(insns)
    rec = sum(v for k, v in d.stats.items() if k != "todo")
    return d, rec, len(insns)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--start", type=lambda s: int(s, 0))
    ap.add_argument("--end", type=lambda s: int(s, 0))
    ap.add_argument("--listing", default=DISASM)
    ap.add_argument("--ref-bin", default=None, help="(accepted; objdump is used)")
    ap.add_argument("--objdump", action="store_true",
                    help="regenerate the range from GameServer.exe via objdump")
    ap.add_argument("--sigs", default=SIGS)
    ap.add_argument("--idents", default=None)
    ap.add_argument("--src", default=SRC)
    ap.add_argument("--out", default=None)
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args(argv)

    fields = load_field_names(args.src)
    sigs = load_sigs(args.sigs)
    idents = load_idents(args.idents)

    if args.selftest:
        print("selftest: fields=%d sigs=%d idents=%d" % (len(fields), len(sigs), len(idents)),
              file=sys.stderr)
        total_r = total_n = 0
        for name, a, b in SELFTEST:
            d, rec, n = run_range(a, b, args, fields, sigs, idents, not args.quiet)
            total_r += rec
            total_n += n
            kinds = ", ".join("%s=%d" % kv for kv in sorted(d.stats.items()))
            print("%-28s %d/%d = %5.1f%%   todos=%d\n    %s"
                  % (name, rec, n, 100.0 * rec / max(n, 1), d.todos, kinds),
                  file=sys.stderr)
        print("TOTAL %d/%d = %.1f%%" % (total_r, total_n, 100.0 * total_r / max(total_n, 1)),
              file=sys.stderr)
        return 0

    if args.start is None or args.end is None:
        ap.error("--start and --end are required (or use --selftest)")

    d, rec, n = run_range(args.start, args.end, args, fields, sigs, idents, True)
    if args.out:
        with open(args.out, "w") as fh:
            fh.write("\n".join(d.lines) + "\n")
    else:
        sys.stdout.write("\n".join(d.lines) + "\n")

    print("asm2cpp: %d instructions, %d recognised (%.1f%%), %d TODOs"
          % (n, rec, 100.0 * rec / max(n, 1), d.todos), file=sys.stderr)
    for k, v in sorted(d.stats.items()):
        print("    %-18s %d" % (k, v), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
