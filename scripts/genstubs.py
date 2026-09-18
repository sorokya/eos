#!/usr/bin/env python3
"""Generate compile-safe stubs for the reference functions not yet written.

Reads the tracking sheet (`analysis/target/functions.tsv`) and, for every
`app` function still `unimplemented`/`mismatched`, appends a marked stub to its
unit's `.cpp`, inside a generated block:

    // BEGIN GENERATED STUBS (scripts/genstubs.py)
    // STUB(0x00407b30, 934 bytes) Players_Tick - ref: void Players_Tick(Players *)
    void Players_Tick_Stub() {}
    // END GENERATED STUBS

The stub is a placeholder, not a reconstruction: its name carries a `_Stub`
suffix so it cannot collide with (or be mistaken for) the real symbol, and its
signature is only as good as Ghidra's inference, translated to compilable C++
(pointers become `void *`, sized unknowns map to their machine width). The
reference's inferred signature is kept in the comment as the starting point.

The stubs are unreferenced COMDATs, so they cannot change the linked image;
`verify_units.py` skips `*_Stub` and `track.py` reports those rows as `stubbed`.
Re-running replaces the block in place.
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

BEGIN = "// BEGIN GENERATED STUBS (scripts/genstubs.py)"
END = "// END GENERATED STUBS"

# Ghidra's inferred types -> the smallest compilable C++ equivalent.
PRIMITIVE = {
    "undefined": "char", "undefined1": "char", "undefined2": "short",
    "undefined4": "int", "undefined8": "long long",
    "byte": "unsigned char", "sbyte": "char", "ushort": "unsigned short",
    "uint": "unsigned int", "ulong": "unsigned long", "bool": "bool",
    "char": "char", "short": "short", "int": "int", "long": "long",
    "float": "float", "double": "double",
}


def load_sheet(path):
    rows = []
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 10 and p[0] != "unit":
                rows.append({"unit": p[0], "start": int(p[2], 16),
                             "size": int(p[4]), "name": p[5], "kind": p[6],
                             "status": p[8]})
    return rows


def load_signatures(path):
    sigs = {}
    with open(path, encoding="utf-8") as fh:
        next(fh, None)
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 4:
                sigs[p[0].lower()] = p[3]
    return sigs


def stub_name(hint):
    name = re.sub(r"[^A-Za-z0-9_]", "_", hint)
    if name[:1].isdigit():
        name = "S" + name
    return name + "_Stub"


def cpp_type(decl):
    """Translate one Ghidra parameter/return declaration to C++."""
    decl = decl.strip()
    if decl.endswith("*") or "*" in decl:
        return "void *"
    base = decl.replace("enum ", "").replace("struct ", "").strip()
    if base in PRIMITIVE:
        return PRIMITIVE[base]
    # An unknown named type: keep it a pointer-width scalar so it compiles.
    return "int"


def parse_signature(sig):
    """(return_type, [param_types]) from Ghidra's `ret name(params)`."""
    m = re.match(r"^(.*?)\s+([A-Za-z_]\w*)\s*\((.*)\)$", sig.strip())
    if not m:
        return "void", []
    ret, params = m.group(1).strip(), m.group(3).strip()
    types = []
    if params and params != "void":
        for p in params.split(","):
            p = p.strip()
            if not p:
                continue
            p = re.sub(r"\bparam_\d+\b|\bthis\b", "", p).strip()
            types.append(cpp_type(p))
    return cpp_type(ret) if ret not in ("void", "undefined") else "void", types


def render(unit, entries, sigs, existing):
    # Placeholders legitimately have unused parameters; without this the
    # warnings alone trip bcc32's 100-message cap (E2228).
    lines = [BEGIN, "#pragma warn -8057"]
    for e in entries:
        sig = sigs.get(f"{e['start']:08x}", "")
        ret, params = parse_signature(sig) if sig else ("void", [])
        name = stub_name(e["name"])
        if name in existing:
            continue
        args = ", ".join(f"{t} a{i}" for i, t in enumerate(params))
        body = "{}" if ret == "void" else "{ return 0; }"
        note = f" - ref: {sig}" if sig else ""
        lines.append(f"// STUB({e['start']:#010x}, {e['size']} bytes) "
                     f"{e['name']}{note}")
        lines.append(f"{ret} {name}({args}) {body}")
    lines.append("#pragma warn .8057")
    lines.append(END)
    return "\n".join(lines)


def splice(src, block):
    """Replace an existing generated block, else append one.

    Pure: the caller reads the file, this returns the new text. Reading and
    writing in one expression truncated the file before it was read.
    """
    if BEGIN in src and END in src:
        pre = src[:src.index(BEGIN)]
        post = src[src.index(END) + len(END):]
        return pre.rstrip() + "\n\n" + block + post
    return src.rstrip() + "\n\n" + block + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--tsv", default="analysis/target/functions.tsv")
    ap.add_argument("--ghidra", default="analysis/ghidra/functions.tsv")
    ap.add_argument("--src", default="src")
    ap.add_argument("--reg", default="analysis/target/stubs.tsv")
    args = ap.parse_args()

    rows = load_sheet(args.tsv)
    sigs = load_signatures(args.ghidra)
    todo = {}
    for r in rows:
        if r["kind"] == "app" and r["status"] in ("unimplemented", "mismatched"):
            todo.setdefault(r["unit"], []).append(r)

    total = 0
    reg = ["unit\tstart\tname\tstub"]
    for unit, entries in sorted(todo.items()):
        path = os.path.join(args.src, unit + ".cpp")
        if not os.path.exists(path):
            print(f"  no source for {unit}: {len(entries)} stubs skipped",
                  file=sys.stderr)
            continue
        # The generated block is replaced wholesale, so it need not be filtered
        # against names already present (that would erase it on a second run).
        entries.sort(key=lambda e: e["start"])
        block = render(unit, entries, sigs, set())
        original = open(path, encoding="latin1").read()
        updated = splice(original, block)
        # Never lose source: everything before the generated block must survive.
        keep = original[:original.index(BEGIN)] if BEGIN in original else original
        if not updated.startswith(keep.rstrip()):
            raise SystemExit(f"refusing to rewrite {path}: content would change")
        open(path, "w", encoding="latin1").write(updated)
        for e in entries:
            reg.append(f"{unit}\t{e['start']:#010x}\t{e['name']}\t"
                       f"{stub_name(e['name'])}")
        total += len(entries)
        print(f"{unit:16} {len(entries):4} stubs")

    with open(args.reg, "w", encoding="utf-8") as fh:
        fh.write("\n".join(reg) + "\n")
    print(f"\n{total} stubs written; registry {args.reg}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
