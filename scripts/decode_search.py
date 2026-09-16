#!/usr/bin/env python3
"""Brute-force the source form of a function against a reference range.

Generates candidate C++ bodies for `ClassValues::DecodeInt`, compiles each with
bcc32 under Wine, and scores it against the reference by (a) the EH scope marker
stream and (b) the instruction count. The marker stream is the structural
fingerprint: it encodes block nesting and cleanup arming, which is exactly what
distinguishes otherwise-equivalent source forms.

Usage:
    scripts/decode_search.py
"""

import itertools
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import compare_asm as ca  # noqa: E402

REF_START, REF_END = 0x537330, 0x537460
TMP = "build/search"

HEADER = """#include <vcl.h>
#pragma hdrstop

class ClassValues
{
public:
    int DecodeInt(STRING_PARAM name);
};
"""

PARAM_FORMS = [
    ("byval", "String name"),
    ("constref", "const String &name"),
    ("ref", "String &name"),
]

# (name, declaration line, prologue expression)
BODY_FORMS = [
    ("copy_init", "String value_copy = name;"),
    ("copy_assign", "String value_copy;\n    value_copy = name;"),
    ("nocopy", ""),
]

CHAR_FORMS = [
    ("uchar", "unsigned char"),
    ("char", "char"),
    ("byte", "byte"),
]

LOOP_FORMS = ["for", "while", "while_true"]

ACC_FORMS = ["plus_eq", "assign_plus"]


def body(param, copy_form, char_type, loop, acc, var):
    _cm, decl = copy_form
    lines = []
    if decl:
        lines.append("    " + decl.replace("name", param))
    lines.append("    int result = 0;")
    if loop == "for":
        lines.append("    %s%si = 1; i <= %s.Length(); i++)" % (
            "for (int ", "", var))
        lines.append("    {")
        lines.append("        %s ch = %s[i];" % (char_type, var))
        lines.append("        if (ch == 0xFE || ch == 0) break;")
        lines.append("        int c = ch - 1;")
    else:
        lines.append("    int i = 1;")
        lines.append("    while (%s)" % ("true" if loop == "while_true" else "i <= %s.Length()" % var))
        lines.append("    {")
        lines.append("        if (%s.Length() < i) break;" % var)
        lines.append("        %s ch = %s[i];" % (char_type, var))
        lines.append("        if (ch == 0xFE || ch == 0) break;")
        lines.append("        int c = ch - 1;")
    for idx, mul in enumerate(("", "0xfd", "0xfa09", "0xf71ae5"), start=1):
        term = "c" if mul == "" else "c * " + mul
        if acc == "plus_eq":
            lines.append("        if (i == %d) result += %s;" % (idx, term))
        else:
            lines.append("        if (i == %d) result = result + %s;" % (idx, term))
    if loop != "for":
        lines.append("        i = i + 1;")
    lines.append("    }")
    lines.append("    return result;")
    return "\n".join(lines)


def compile_variant(source):
    os.makedirs(TMP, exist_ok=True)
    cpp = os.path.join(TMP, "v.cpp")
    asm = os.path.join(TMP, "v.asm")
    open(cpp, "w").write(source)
    subprocess.run(["scripts/borland.sh",
                    'wine "$B\\Bin\\bcc32.exe" -D__CODEGUARD__ -v -Od -S '
                    '-oZ:/work/%s Z:/work/%s' % (asm, cpp)],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return asm if os.path.exists(asm) else None


def main() -> int:
    ref, ref_mk = ca.parse_ref("GameServer.exe", REF_START, REF_END)
    print(f"reference: {len(ref)} instructions, {len(ref_mk)} markers")
    results = []
    total = (len(PARAM_FORMS) * len(BODY_FORMS) * len(CHAR_FORMS)
             * len(LOOP_FORMS) * len(ACC_FORMS))
    n = 0
    for (pname, pdecl), copy_form, (cname, ctype), loop, acc in itertools.product(
            PARAM_FORMS, BODY_FORMS, CHAR_FORMS, LOOP_FORMS, ACC_FORMS):
        n += 1
        var = "name" if copy_form[0] == "nocopy" else "value_copy"
        src = HEADER.replace("STRING_PARAM name", pdecl) + "\nint ClassValues::DecodeInt(" \
              + pdecl + ")\n{\n" + body(pname, copy_form, ctype, loop, acc, var) + "\n}\n"
        asm = compile_variant(src)
        if not asm:
            continue
        our, _calls, our_mk = ca.parse_our(asm, "@@ClassValues@DecodeInt")
        if our is None:
            continue
        same_mk = our_mk == ref_mk
        results.append((same_mk, abs(len(our) - len(ref)), len(our),
                        f"param={pname} copy={copy_form[0]} char={cname} "
                        f"loop={loop} acc={acc}"))
        if same_mk:
            print(f"  MARKERS MATCH: {results[-1][3]} ({len(our)} our vs {len(ref)} ref)")
    results.sort(key=lambda r: (not r[0], r[1]))
    print(f"\n{n} variants compiled; {len(results)} usable.")
    print("closest 10:")
    for same, dist, count, label in results[:10]:
        print(f"  markers={'MATCH' if same else 'no '} dcount={dist:3} n={count:3}  {label}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
