#!/usr/bin/env python3
"""Generate stub translation units for the full build skeleton.

For every unit in `analysis/target/units.tsv`, create `src/<Unit>.cpp` (skipped if
it already exists). Ordinary units get `#pragma package(smart_init)`, which makes
the compiler emit the exported `@@Unit@Initialize`/`@Finalize` pair. The main unit
`GUI` gets a minimal form + global + WinMain shell.

The stubs exist to keep the build linkable and to reproduce the export table while
units are reconstructed; they are replaced unit by unit.
"""

import argparse
import os
import sys


def load_units(path: str) -> list[str]:
    units = []
    with open(path, encoding="utf-8") as fh:
        next(fh)
        for line in fh:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 2:
                continue
            units.append(parts[1].lstrip("@"))
    return units


STUB = "#pragma package(smart_init)\n"

GUI_CPP = """#include <vcl.h>
#pragma hdrstop

#include "GUI.h"

TGUI *GUI;

__fastcall TGUI::TGUI(TComponent* Owner) : TForm(Owner)
{
}

#pragma argsused
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Application->Initialize();
    Application->Run();
    return 0;
}
"""

GUI_H = """#ifndef GUIH
#define GUIH

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>

class TGUI : public TForm
{
__published:
private:
public:
    __fastcall TGUI(TComponent* Owner);
};

extern PACKAGE TGUI *GUI;

#endif
"""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--units", default="analysis/target/units.tsv")
    ap.add_argument("-o", "--out", default="src")
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()

    units = load_units(args.units)
    os.makedirs(args.out, exist_ok=True)
    created = skipped = 0
    for unit in units:
        cpp = os.path.join(args.out, f"{unit}.cpp")
        hdr = os.path.join(args.out, f"{unit}.h")
        if unit == "GUI":
            if args.force or not os.path.exists(cpp):
                with open(cpp, "w", encoding="utf-8") as fh:
                    fh.write(GUI_CPP)
                created += 1
            else:
                skipped += 1
            if args.force or not os.path.exists(hdr):
                with open(hdr, "w", encoding="utf-8") as fh:
                    fh.write(GUI_H)
            continue
        if os.path.exists(cpp) and not args.force:
            skipped += 1
            continue
        with open(cpp, "w", encoding="utf-8") as fh:
            fh.write(STUB)
        created += 1

    print(f"units: {len(units)}  created: {created}  skipped(existing): {skipped}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
