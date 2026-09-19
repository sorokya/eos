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
# For `cmp a, b` the flags are those of `a - b` (Intel syntax: destination
# first).  The C relation is the same whether the CPU used the signed or the
# unsigned view, so `jl` and `jb` both mean `a < b`; the negated spellings
# (`jnge`, `jnae`, ...) are *aliases for the same condition*, not negations.
CMP_REL = {
    "je": "==", "jz": "==", "jne": "!=", "jnz": "!=",
    "jl": "<", "jnge": "<",
    "jle": "<=", "jng": "<=",
    "jg": ">", "jnle": ">",
    "jge": ">=", "jnl": ">=",
    "jb": "<", "jnae": "<", "jc": "<",
    "jbe": "<=", "jna": "<=",
    "ja": ">", "jnbe": ">",
    "jae": ">=", "jnb": ">=", "jnc": ">=",
    "js": "<", "jns": ">=",
}
# `test a, b` sets flags from `a & b`: only the zero/sign comparisons are
# meaningful; everything else is left undetermined (no guess).
TEST_REL = {
    "je": "== 0", "jz": "== 0", "jne": "!= 0", "jnz": "!= 0",
    "js": "< 0", "jns": ">= 0",
}


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
# Signature parsing (param types for the type-inference pass)
# ---------------------------------------------------------------------------

SIG_DECL_RE = re.compile(r"^\s*//\s*([A-Za-z_]\w*)\s*\(([^)]*)\)\s*$")
SIG_STUB_RE = re.compile(r"^\s*//\s*STUB\(0x([0-9a-fA-F]+)[^)]*\)\s*(\w+)\s*-\s*ref:\s*(.*)$")


def parse_param_types(text):
    """'Server *server, int *query_result' -> ['Server *', 'int *']."""
    text = text.strip()
    if not text or text == "void":
        return []
    out = []
    depth = 0
    cur = ""
    for ch in text:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    out.append(cur)
    types = []
    for part in out:
        part = part.strip()
        m = re.match(r"^(.*?)([A-Za-z_]\w*)\s*(\[[^\]]*\])?$", part)
        if not m:
            types.append(part)
            continue
        typ = m.group(1).strip()
        if m.group(3):
            typ += m.group(3)
        types.append(typ or "int")
    return types


def load_stub_sigs(src_dir):
    """addr -> param types and name -> param types, from src/*.cpp comments."""
    by_addr = {}
    by_name = {}
    if not os.path.isdir(src_dir):
        return by_addr, by_name
    for fn in sorted(os.listdir(src_dir)):
        if not fn.endswith(".cpp"):
            continue
        for line in open(os.path.join(src_dir, fn), errors="replace"):
            m = SIG_STUB_RE.match(line)
            if m:
                body = m.group(3).strip()
                pm = re.search(r"\(([^)]*)\)", body)
                if pm:
                    by_addr[int(m.group(1), 16)] = parse_param_types(pm.group(1))
                continue
            m = SIG_DECL_RE.match(line)
            if m:
                by_name.setdefault(m.group(1), parse_param_types(m.group(2)))
    return by_addr, by_name


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


CLASS_RE = re.compile(r"^\s*(?:class|struct)\s+(\w+)")
FIELD_TYPES = {}


CLASS_TYPE_RE = re.compile(r"([A-Za-z_]\w*)\s*(\*+)?\s*(?://|$)")


def field_type_of(line, name):
    """Extract the declared type of `name` from a header field line."""
    head = line.split("//")[0]
    m = re.match(r"\s*(.+?)[\s*&]+%s\b" % re.escape(name), head)
    if not m:
        return None
    typ = m.group(1).strip()
    stars = ""
    rest = head[m.end(1):]
    stars = re.findall(r"\*", rest.split(name)[0]) if name in rest else []
    typ = re.sub(r"\s+", " ", typ)
    if typ in ("", "return"):
        return None
    return typ + "*" * len(stars)


def load_class_fields(src_dir):
    """class name -> {offset: field name} from the `// +0xNN` comments in src/*.h."""
    out = {}
    if not os.path.isdir(src_dir):
        return out
    for fn in sorted(os.listdir(src_dir)):
        if not fn.endswith(".h"):
            continue
        cur = None
        depth = 0
        pending = None          # class name awaiting its `{`
        for line in open(os.path.join(src_dir, fn), errors="replace"):
            stripped = line.strip()
            if cur is None:
                m = CLASS_RE.match(line)
                if m and not stripped.endswith(";"):
                    pending = m.group(1)
                elif "{" in line and pending:
                    cur = pending
                    pending = None
                    depth = 0
                    out.setdefault(cur, {})
                else:
                    if ";" in line:
                        pending = None
                    continue
            depth += line.count("{") - line.count("}")
            fm = FIELD_RE.search(line)
            if fm:
                head = line[: fm.start()].rstrip()
                im = IDENT_RE.search(head)
                if im:
                    name = im.group(1)
                    if name not in ("int", "char", "bool", "short", "void",
                                    "unsigned", "long", "return"):
                        off = int(fm.group(1), 16)
                        # first definition wins; skip if a different class
                        # already claimed this offset with another name
                        if out[cur].get(off, name) == name:
                            out[cur].setdefault(off, name)
                            ft = field_type_of(line, name)
                            if ft:
                                FIELD_TYPES.setdefault(cur, {}).setdefault(off, ft)
            if depth <= 0 and "}" in line:
                cur = None
                pending = None
    return out


def base_class(type_str):
    if not type_str:
        return None
    return re.sub(r"[\s*&]+$", "", type_str.strip())


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
    def __init__(self, fields, sigs, idents, start, params=None, class_fields=None):
        self.class_fields = class_fields or {}
        self.params = params or []
        self.slot_types = {}      # ebp offset -> type
        self.reg_types = {}       # register -> type
        self.reg_expr = {}        # register -> rendered expression
        self.push_stack = []      # expressions pushed since the last call
        self.branch_targets = set()
        self.unique_offset = {}
        self.slot_name = {}
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

    def selfcheck(self):
        """Invariant: no emitted conditional may carry an unknown relation/operand."""
        bad = []
        for ln in self.lines:
            t = ln.strip()
            if not t.startswith("if ("):
                continue
            if "..." in t or ", ..." in t or t.startswith("if (/*"):
                bad.append(t)
        return bad

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

    def field_type(self, type_str, disp):
        cls = base_class(type_str)
        if not cls:
            return None
        return FIELD_TYPES.get(cls, {}).get(disp)

    def field_expr(self, op):
        d = self.deref(op)
        if not d:
            return None
        base, index, scale, disp = d
        if index or base == "ebp" or disp < 0:
            return None  # array indexing / stack local - not a scalar field
        if base in self.reg_types:
            cls = base_class(self.reg_types[base])
            nm = self.class_fields.get(cls, {}).get(disp) if cls else None
            if nm is None:
                nm = self.unique_offset.get(disp)
            if nm:
                return "%s->%s" % (base, nm)
            return "/*%s*/field_0x%x" % (base, disp)
        if disp in self.unique_offset:
            return "/*%s*/%s" % (base, self.unique_offset[disp])
        return None

    def typed_offset(self, op):
        """(reg, disp) for a typed indirect operand, else None."""
        d = self.deref(op)
        if not d:
            return None
        base, index, scale, disp = d
        if index or base == "ebp" or disp < 0 or base not in self.reg_types:
            return None
        return (base, disp)


    def render(self, op):
        """Best-effort expression for an operand (typed where possible)."""
        op = op.strip()
        d = self.deref(op)
        if d:
            base, index, scale, disp = d
            if base == "ebp" and not index:
                nm = self.slot_name.get(disp)
                if nm:
                    return nm
                return "local_0x%x" % (-disp if disp < 0 else disp)
            fe = self.field_expr(op)
            if fe:
                return fe
            if base in self.reg_types:
                return "%s->field_0x%x" % (base, disp)
            return None
        if re.match(r"^0x[0-9a-fA-F]+$|^\d+$", op):
            return op
        if re.match(r"^\w+$", op):
            if op in self.reg_expr:
                return self.reg_expr[op]
            return op
        return None

    # -- main walk ---------------------------------------------------------
    def walk(self, insns):
        end = insns[-1][0] + 1 if insns else self.start
        for k, t in enumerate(self.params):
            self.slot_types[8 + 4 * k] = t
        for a, mn, ops, raw in insns:
            m = re.match(r"0x([0-9a-fA-F]+)", ops)
            if mn.startswith("j") and m:
                self.branch_targets.add(int(m.group(1), 16))
        slot_name = {8 + 4 * k: ("a%d" % k) for k in range(len(self.params))}
        self.slot_name = slot_name
        # offsets that name exactly one field across every class -> safe hint
        seen = {}
        for cls, tbl in self.class_fields.items():
            for off, nam in tbl.items():
                if off not in seen:
                    seen[off] = nam
                elif seen[off] != nam:
                    seen[off] = None
        self.unique_offset = {o: n for o, n in seen.items() if n}
        for off, nam in self.fields.items():
            if off not in self.unique_offset:
                continue
        i = 0
        while i < len(insns):
            addr, mn, ops, raw = insns[i]
            self.n += 1
            carry = self.pending_cmp
            self.pending_cmp = None
            if addr in self.branch_targets:
                self.reg_types = {}
                self.reg_expr = {}
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
                self.emit_jcc(addr, mn, ops, raw, carry)
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
                self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "cmp")
                i += 1
                continue

            # --- field load/store -----------------------------------------
            if mn in ("mov", "movzx", "movsx"):
                if self.emit_reg_move(addr, mn, ops):
                    i += 1
                    continue
            if mn in ("mov", "movzx", "movsx", "add", "sub", "inc", "dec", "or", "and", "xor"):
                if self.emit_mem(addr, mn, ops):
                    i += 1
                    continue

            # --- ret / epilogue -------------------------------------------
            if mn == "ret":
                self.emit(addr, "return /* eax */;  // 0x%x" % addr, "ret")
                i += 1
                continue
            if mn == "push":
                expr = self.render(ops)
                if expr is not None:
                    self.push_stack.append(expr)
                self.emit(addr, "// push %s  // 0x%x" % (ops, addr), "stack")
                i += 1
                continue
            if mn == "pop":
                self.emit(addr, "// pop %s  // 0x%x" % (ops, addr), "stack")
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
                self.push_stack = []
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
        args = list(reversed(self.push_stack))
        self.push_stack = []
        argtext = None
        if args and all(a is not None for a in args):
            argtext = ", ".join(args)
        if tgt in ANSI_OPS:
            kind, sig = ANSI_OPS[tgt]
            self.emit(addr, "// AnsiString::%s  // 0x%x" % (sig, addr), "ansi_" + kind)
            return
        if tgt in HELPERS:
            self.emit(addr, "// %s  // 0x%x" % (HELPERS[tgt], addr), "helper")
            return
        # a call clobbers caller-saved registers
        for r in ("eax", "ecx", "edx"):
            self.reg_types.pop(r, None)
            self.reg_expr.pop(r, None)
        nm = self.callee_name(tgt)
        if nm:
            if argtext is not None:
                self.emit(addr, "%s(%s);  // 0x%x" % (nm, argtext, addr), "call_args")
            else:
                self.emit(addr, "%s(...);  // 0x%x" % (nm, addr), "call_named")
        else:
            self.todo(addr, "call 0x%x (unidentified)" % tgt)

    def split_ops(self, ops):
        """Split an Intel operand list on the top-level comma (bracket-aware)."""
        depth = 0
        parts = []
        cur = ""
        for ch in ops:
            if ch == "[":
                depth += 1
            elif ch == "]":
                depth -= 1
            if ch == "," and depth == 0:
                parts.append(cur)
                cur = ""
            else:
                cur += ch
        parts.append(cur)
        return [p.strip() for p in parts]

    def fold_condition(self, cmn, cops, jmn):
        """Return a proven C condition string, or None (never guess)."""
        parts = self.split_ops(cops)
        if len(parts) != 2:
            return None
        oa, ob = parts[0], parts[1]
        a = self.render(oa)
        if a is None:
            return None
        if cmn == "test":
            rel = TEST_REL.get(jmn)
            if rel is None:
                return None
            # `test a, a` is the canonical "a against zero" self-test
            if oa == ob:
                b = "0"
                rel = "== 0" if rel in ("== 0",) else ("!= 0" if rel == "!= 0" else rel)
                return "%s %s" % (a, rel)
            b = self.render(ob)
            if b is None:
                return None
            return "(%s & %s) %s" % (a, b, rel)
        rel = CMP_REL.get(jmn)
        if rel is None:
            return None
        b = self.render(ob)
        if b is None:
            return None
        return "%s %s %s" % (a, rel, b)

    def emit_jcc(self, addr, mn, ops, raw, carry):
        tgt = int(ops.split()[0], 16)
        expr = None
        if carry:
            caddr, cmn, cops = carry
            expr = self.fold_condition(cmn, cops, mn)
        if expr is None:
            # never emit a conditional we cannot prove
            self.todo(addr, "condition not determined (%s after %s)"
                      % (mn, carry[1] if carry else "no cmp"))
            self.hit("jcc_todo")
            return
        self.emit(addr, "if (%s) goto L_0x%x;  // 0x%x"
                  % (expr, tgt, addr), "jcc_cond")

    def emit_reg_move(self, addr, mn, ops):
        m = re.match(r"(\w+),\s*(.+)", ops)
        if not m:
            return False
        dst, src = m.group(1), m.group(2).strip()
        # lea reg,[typed field] -> address expression (not a field load)
        if mn == "lea":
            self.reg_types.pop(dst, None)
            self.reg_expr.pop(dst, None)
            expr = self.field_expr(src)
            if expr:
                self.reg_expr[dst] = "&%s" % expr
                self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "lea_field")
                return True
            self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "local")
            return True
        # mov reg, [typed field]
        fe = self.field_expr(src)
        if fe:
            self.reg_types.pop(dst, None)
            self.reg_expr[dst] = fe
            to = self.typed_offset(src)
            if to:
                ft = self.field_type(self.reg_types.get(to[0], ""), to[1])
                if ft and "'" not in ft and "[" not in ft:
                    self.reg_types[dst] = ft
            self.emit(addr, "%s = %s;  // 0x%x" % (dst, fe, addr), "field")
            return True
        # mov reg, [ebp+N]  (argument / local slot)
        d = self.deref(src)
        if d and d[0] == "ebp" and not d[1]:
            off = d[3]
            self.reg_types.pop(dst, None)
            self.reg_expr.pop(dst, None)
            if off in self.slot_types and ("*" in self.slot_types[off]):
                self.reg_types[dst] = self.slot_types[off]
            if off in self.slot_name:
                self.reg_expr[dst] = self.slot_name[off]
            self.emit(addr, "// %s %s  // 0x%x" % (mn, ops, addr), "local")
            return True
        # mov reg, reg
        if re.match(r"\w+$", src):
            if src in self.reg_types:
                self.reg_types[dst] = self.reg_types[src]
            else:
                self.reg_types.pop(dst, None)
            if src in self.reg_expr:
                self.reg_expr[dst] = self.reg_expr[src]
            else:
                self.reg_expr.pop(dst, None)
            self.emit(addr, "%s = %s;  // 0x%x" % (dst, src, addr), "regmove")
            return True
        # mov reg, imm
        if re.match(r"(0x[0-9a-fA-F]+|\d+)$", src):
            self.reg_types.pop(dst, None)
            self.reg_expr.pop(dst, None)
            self.emit(addr, "%s = %s;  // 0x%x" % (dst, src, addr), "imm")
            return True
        # other memory operand
        self.reg_types.pop(dst, None)
        self.reg_expr.pop(dst, None)
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
    ("EO_Encode_Interleave", 0x470EF0, 0x4711E7,
     "void EO_Encode_Interleave(char *data, int len, char *out)"),
    ("Walk_BuildReply", 0x45D874, 0x45DDF0,
     "AnsiString *Walk_BuildReply(AnsiString *out, Server *server, Player *player)"),
    ("Player_SerializePaperdoll", 0x460068, 0x4607C8,
     "String Player_SerializePaperdoll(AnsiString *out, Server *server, Player *player)"),
]


def run_range(start, end, args, fields, sigs, idents, verbose, params=None,
              class_fields=None):
    if args.objdump:
        insns = regen_with_objdump(start, end)
    else:
        insns = load_listing(args.listing, start, end)
    if not insns:
        insns = regen_with_objdump(start, end)
    d = Draft(fields, sigs, idents, start, params=params,
              class_fields=class_fields)
    d.walk(insns)
    rec = sum(v for k, v in d.stats.items() if k != "todo")
    return d, rec, len(insns)


def load_start_names(path):
    """start VA -> ref_name from functions.tsv (both keys are plain names)."""
    out = {}
    try:
        with open(path, errors="replace") as fh:
            next(fh, "")
            for line in fh:
                c = line.rstrip("\n").split("\t")
                if len(c) < 6:
                    continue
                try:
                    out[int(c[2], 16)] = c[5]
                except ValueError:
                    continue
    except OSError:
        pass
    return out


functions_start_name = {}


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
    ap.add_argument("--sig", default=None,
                    help="explicit signature, e.g. 'int F(Server *server, int *qr)'")
    ap.add_argument("--out", default=None)
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args(argv)

    fields = load_field_names(args.src)
    class_fields = load_class_fields(args.src)
    sigs = load_sigs(args.sigs)
    idents = load_idents(args.idents)
    stub_by_addr, stub_by_name = load_stub_sigs(args.src)
    global functions_start_name
    functions_start_name = load_start_names(args.sigs)

    def params_for(a, b):
        if args.sig:
            pm = re.search(r"\(([^)]*)\)", args.sig)
            return parse_param_types(pm.group(1)) if pm else []
        if a in stub_by_addr:
            return stub_by_addr[a]
        # functions.tsv start -> ref_name, then the name-keyed stub comment
        for addr, nm in sigs.items():
            if addr == a and nm in stub_by_name:
                return stub_by_name[nm]
        if a in functions_start_name:
            nm = functions_start_name[a]
            if str(nm) in stub_by_name:
                return stub_by_name[str(nm)]
        return []

    if args.selftest:
        print("selftest: fields=%d sigs=%d idents=%d" % (len(fields), len(sigs), len(idents)),
              file=sys.stderr)
        total_r = total_n = 0
        for name, a, b, sig in SELFTEST:
            pm = re.search(r"\(([^)]*)\)", sig)
            d, rec, n = run_range(a, b, args, fields, sigs, idents, not args.quiet,
                                  params=parse_param_types(pm.group(1)) if pm else [],
                                  class_fields=class_fields)
            total_r += rec
            total_n += n
            kinds = ", ".join("%s=%d" % kv for kv in sorted(d.stats.items()))
            conds = [l.strip() for l in d.lines if l.strip().startswith("if (")]
            bad = d.selfcheck()
            print("%-28s %d/%d = %5.1f%%   todos=%d  conditions=%d unresolved=%d\n    %s"
                  % (name, rec, n, 100.0 * rec / max(n, 1), d.todos, len(conds),
                     d.stats.get("jcc_todo", 0), kinds),
                  file=sys.stderr)
            for c in conds:
                print("      %s" % c, file=sys.stderr)
            if bad:
                print("      !! UNSAFE CONDITION: %s" % bad[0], file=sys.stderr)
                raise SystemExit(3)
        print("TOTAL %d/%d = %.1f%%" % (total_r, total_n, 100.0 * total_r / max(total_n, 1)),
              file=sys.stderr)
        return 0

    if args.start is None or args.end is None:
        ap.error("--start and --end are required (or use --selftest)")

    d, rec, n = run_range(args.start, args.end, args, fields, sigs, idents, True,
                          params=params_for(args.start, args.end),
                          class_fields=class_fields)
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
