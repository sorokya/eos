# AGENTS.md — Working Instructions

This file tells human contributors and AI agents how to work in this repository.
Read [README.md](README.md) for the project overview and [PLAN.md](PLAN.md) for the
roadmap before making changes.

## Mission

Make `GameServer.exe` rebuildable from reconstructed source, using the Borland
C++ 5.5 toolchain, such that the linked output is byte-for-byte identical to the
reference binary:

```
MD5 (GameServer.exe) = 075fb5db1d6369bf488cd2ad0c715ddd
```

## Hard rules

1. **The reference binary is ground truth.** The provided `GameServer.exe` must
   never be modified. Treat it as read-only input.
2. **Never guess machine-code bytes.** Every instruction, branch target, and data
   value must come from a disassembly of the reference or a verified reassembly
   diff. If you cannot prove a byte, do not write it.
3. **Never invent file or symbol names.** Unit names come from the export table,
   form names from the DFM resources, and library functions from the toolchain.
   Do not name a source file unless it corresponds to observed evidence.
4. **Verify, then record.** A change is only done when the relevant comparison
   (assembly bytes, object bytes, or final MD5) passes. Record the verification
   method and result alongside the change.
5. **Do not commit** unless the user explicitly asks. Do not amend, force-push,
   or rewrite history.
6. **Do not add comments** to reconstructed source unless they document a verified
   fidelity fact. Keep the code free of speculative annotation.
7. **Do not fabricate progress.** If a phase is partial, say so and leave evidence
   of what remains.

## Environment and toolchain

All Borland tools are Windows binaries and run under Wine in the Docker image built
from `docker/Dockerfile`. The image bakes a `WINEARCH=win32` prefix at
`/wineprefix`.

```sh
docker build -t gameserver-borland-wine docker
```

### Canonical container invocation

The toolchain must appear at the path recorded in its config files
(`C:\Program Files (x86)\Borland\CBuilder5`) or the compiler will not find its
headers and libraries. Prefer the wrapper, which sets this up:

```sh
scripts/borland.sh '<command>'
```

It mounts the toolchain and the repository, and exports:

- `$B` — Borland root with spaces, for invoking tools.
- `$BZ` — the same tree via `Z:\borland` (space-free). Use it for `-L`, object
  and library arguments: `ilink32` mishandles spaces passed on its command line.

Example:

```sh
scripts/borland.sh 'wine "$B\Bin\bcc32.exe" -c -obuild/x.obj build/x.cpp'
```

The equivalent raw invocation, if the wrapper is not used:

```sh
docker run --rm --platform linux/amd64 \
  -v "$PWD/ref/Borland5:/wineprefix/drive_c/Program Files (x86)/Borland/CBuilder5:ro" \
  -v "$PWD:/work" \
  gameserver-borland-wine bash -lc '<command>'
```

### Verified tool commands

These were confirmed working during Phase 0 (versions shown in the tool
banners):

```sh
# Compiler — bcc32 5.5
wine "$B\Bin\bcc32.exe" -c -ohello.obj hello.cpp

# Assembler — TASM 5.3
wine "$B\Bin\tasm32.exe" /q /ml /m2 hello.asm

# Linker — Turbo Incremental Link 5.00
# Use $BZ for -L / object / library arguments: ilink32 mishandles spaces.
wine "$B\Bin\ilink32.exe" -Tpe -aa -c -Gn -j -v \
    -L"$BZ\Lib" -L"$BZ\Lib\Obj" -L"$BZ\Lib\Release" \
    "$BZ\Lib\c0w32.obj" hello.obj, hello.exe,, import32.lib cw32mt.lib

# Resource compiler — brcc32 5.40
wine "$B\Bin\brcc32.exe" GameServer.rc
```

Notes:
- `bcc32` and `ilink32` read `bcc32.cfg` / `ilink32.cfg` from the directory the
  executables live in; that is why the mount path above matters.
- `ilink32` did not resolve libraries from its `.cfg` when run outside the
  toolchain directory in testing; pass the three `-L"$BZ\Lib..."` paths
  explicitly. `Lib/Obj` is required for VCL `.res` inputs such as `Controls.res`.
- Wine prints assorted warnings; judge success by the exit code and the produced
  artifact, not by stderr being empty.

### Link configuration

Phase 0 established the following, verified by comparing a scratch build
against the reference header and section table:

- Link type `-Tpe` (PE executable), `-aa` (Windows GUI subsystem).
- `-c` case-sensitive linking, `-Gn` no state files, `-j` object search path.
- **`-v` is required**: it leaves the `IMAGE_FILE_DEBUG_STRIPPED` (`0x200`) bit
  clear, producing header characteristic `0x010e` exactly like the target. Without
  `-v`, `ilink32` emits `0x030e`.
- Default section order and alignment already match the target
  (`.text .data .tls .rdata .idata .edata .rsrc .reloc`, align `0x200`/`0x1000`,
  base `0x00400000`).
- Startup object is `c0w32.obj` (Windows GUI startup; built from
  `ref/Borland5/Source/RTL/source/startup/c0ntw.asm`, which includes `c0nt.asm`).
- The application is multithreaded and statically linked, using `cw32mt.lib`
  (multithreaded static RTL) plus the release-mode VCL/BDE libraries in
  `ref/Borland5/Lib/Release`.

The Phase 0 harness validated this link line against the reference header and
section geometry (`make sanity`):

```sh
ilink32 -Tpe -aa -c -Gn -j -v \
    -L"$BZ\Lib" -L"$BZ\Lib\Obj" -L"$BZ\Lib\Release" \
    "$BZ\Lib\c0w32.obj" <objects>,
    GameServer.exe,,
    import32.lib cw32mt.lib vcl50.lib vcldb50.lib vclbde50.lib
```

The library set covers the components observed in the target (`vcl50.lib` for
VCL/forms/sockets, `vcldb50.lib` for the `Db` unit, `vclbde50.lib` for
`DBTables`). Library objects are pulled on demand, so unused libraries do not
affect the bytes; the exact set and order are finalized in Phase 1 by matching
the linked object set. The three `-L` directories mirror the paths in
`ilink32.cfg`; `Lib/Obj` is required for the VCL `.res` files (for example
`Controls.res`).

## Verification protocol

Fidelity is proven at three levels, from cheapest to most expensive. Always run the
cheapest comparison that can falsify your change before moving on.

1. **Assembly/object-level.** Assemble or compile the touched unit and compare the
   resulting `.obj`/`.text` bytes against the reference slice. Use
   `objdump -d`, `tdump.exe`, or a binary diff.
2. **Section-level.** Link and compare each PE section (`.text`, `.data`,
   `.idata`, `.edata`, `.rsrc`, `.reloc`) byte-for-byte against the reference.
   `objdump -h` and `objdump -p` report sizes, addresses, and the import/export
   tables; a section mismatch localizes the failure.
3. **Whole-file.** Final acceptance is the MD5 comparison:

   ```sh
   md5 -q build/GameServer.exe
   # must equal: 075fb5db1d6369bf488cd2ad0c715ddd
   ```

### Determinism

The reference header timestamp is `0x50CBB124` (2012-12-14 23:07:16 UTC), but
`ilink32` writes the current clock, so a fresh link will not match by itself. Two
acceptable approaches, in order of preference:

1. **Clock control.** Run the build under a fixed time (e.g. `libfaketime` set to
   2012-12-14 23:07:16 UTC) so the linker emits the original timestamp.
2. **Post-link normalization.** Rewrite the four `TimeDateStamp` bytes with
   `scripts/normalize_pe.py`, which locates the field at `e_lfanew + 8` (file
   offset `0x208` for the reference; never a fixed `0x8`). It can also normalize
   the COFF characteristics word.

The normalization step is implemented and verified: taking a copy of the
reference with only the timestamp changed, `normalize_pe.py` restores the
reference MD5 exactly. Whatever mechanism is chosen must be part of the
documented build, not a manual fix-up.

## Harness

`scripts/` and the `Makefile` provide the measurement pipeline; see
`scripts/README.md` for the full list. Make targets: `image`, `analyze`,
`disasm`, `sanity`, `compare`, `normalize`, `clean`.

- `scripts/borland.sh '<command>'` — run a tool in the container (exports `$B`,
  `$BZ`).
- `scripts/pe.py`, `scripts/extract_target.py` — dependency-free PE metadata and
  resource/DFM extraction.
- `scripts/compare_pe.py` — three-level comparison; exits non-zero on any diff.
- `scripts/normalize_pe.py` — deterministic timestamp/header normalization.
- `scripts/disasm.sh` — linear `.text` disassembly.

`analysis/` and `build/` are generated and gitignored.

## Source and reconstruction conventions

- Mirror the original translation units exactly, one file per unit, using the unit
  names recovered from the export table (lowercase file base, e.g. unit `Players`
  becomes `Players.cpp` / `Players.h`). The file base name must match the original,
  because it drives exported symbol names and therefore `.edata`.
- Keep the Borland dialect and ABI: 32-bit, `__fastcall`/register conventions as
  the original used, VCL types (`AnsiString`, `TForm`, etc.) rather than STL
  substitutes.
- Reconstruct forms from the embedded DFM resources, not by hand-guessing layout.
  The `TGUI`, `TLoginDialog`, and `TPasswordDialog` resources are available in the
  reference `.rsrc`.
- Prefer the smallest construct that the compiler lowers to the observed bytes.
  Do not "improve" code; match it.
- Keep generated artifacts out of version control (see `.gitignore`); commit only
  source, forms, resources, and build scripts when asked.

## Workflow for a single translation unit

1. Identify the unit's symbol range from the export table and its code/data extents
   in the disassembly.
2. Produce the TASM representation and prove it assembles to the reference bytes
   for that unit.
3. Derive the header (class layout, member offsets, virtual table order) and the
   `.cpp` from the proven assembly.
4. Compile with `bcc32` and compare the object/`.text` bytes to the assembly
   baseline.
5. Link into the full image and re-check the affected sections; run the MD5
   comparison.
6. Update the unit's status in [PLAN.md](PLAN.md#translation-unit-inventory) and
   stop.

## Pitfalls to avoid

- **Wrong mount path.** Building with the toolchain mounted at `/borland` works for
  the banners but fails header/library lookup because `*.cfg` hardcodes
  `C:\Program Files (x86)\Borland\CBuilder5`.
- **Spaces in `ilink32` arguments.** Pass library/object paths via `$BZ`
  (`Z:\borland\...`), not `$B`. Spaces on `ilink32`'s command line cause it to
  misparse paths (for example "Unable to open file 'LIB.OBJ'").
- **Missing VCL resources.** `Lib/Obj` must be on the library search path; the
  VCL `.lib` files reference `.res` inputs such as `Controls.res`.
- **Missing `-v`.** Produces header `0x030e` instead of `0x010e`.
- **Non-deterministic timestamp.** Guarantees an MD5 mismatch even when all code
  and data are correct.
- **Library drift.** The RTL/VCL `.lib`/`.obj` files in `ref/Borland5/Lib` and the
  Borland `Include/` headers must be used unmodified; substituting another
  C++Builder install will change code generation and layout.
- **Assuming file names.** Only `GUI` (main unit) and the 65 units in the export
  inventory are confirmed. Do not assume a unit exists because a feature suggests
  it.

## Reference docs in `ref/`

- `Borland_C++_Version_5_Programmers_Guide_1997.pdf` — compiler/linker semantics
  and language behavior for this exact toolchain generation.
- `calling_conventions.pdf` — Agner Fog: register/stack rules, Borland name
  mangling (section 8.2), 32-bit memory models, object formats, and unwinding.
- `Borland5/Source/RTL/` — RTL and startup sources, including `c0nt.asm`.
