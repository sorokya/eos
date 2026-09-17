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
8. **The Ghidra project is read-only.** Use it for function boundaries,
   signatures, decompilation and cross-references, but do not rename, retype,
   comment, or otherwise modify the database. Its names are ad hoc and only ~14%
   populated; treat them as hints and verify against the reference bytes.

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
- The application is multithreaded and statically linked, using the C/C++ RTL
  (`cw32mt.lib`, `cp32mt.lib`) plus the **Debug** VCL/BDE libraries in
  `ref/Borland5/Lib/Debug`. Byte-signature matching showed the reference's
  library code matches `Lib/Debug` (`vcl50`, `vcldb50`, `vclbde50`) and not
  `Lib/Release` across every library module, and the reference carries Debug-only
  strings with zero Release-only ones — consistent with the debug compile flags.
  `Lib/Debug` therefore precedes `Lib/Release` on the search path.

The Phase 0 harness validated this link line against the reference header and
section geometry (`make sanity`):

```sh
ilink32 -Tpe -aa -c -Gn -j -v \
    -L"$BZ\Lib" -L"$BZ\Lib\Obj" -L"$BZ\Lib\Debug" -L"$BZ\Lib\Release" \
    "$BZ\Lib\c0w32.obj" <objects>,
    GameServer.exe,,
    import32.lib cw32mt.lib cp32mt.lib vcl50.lib vcldb50.lib vclbde50.lib
```

The library set covers the components observed in the target (`vcl50.lib` for
VCL/forms/sockets, `vcldb50.lib` for the `Db` unit, `vclbde50.lib` for
`DBTables`). Library objects are pulled on demand, so unused libraries do not
affect the bytes; the exact set and order are finalized in Phase 2 by matching
the linked object set. The `-L` directories mirror the paths in `ilink32.cfg`;
`Lib/Obj` is required for the VCL `.res` files (for example `Controls.res`).

### Compiler flags

Three flags are required for byte fidelity, all passed by `scripts/build.sh`
and `make unit`/`unit-asm` (`CFLAGS`):

- **`-D__CODEGUARD__`** (CodeGuard compile-time checks). Without it bcc32 inlines
  the `dstring.h` methods; with it they become out-of-line RTL calls
  (`@@System@AnsiString@Length$xqqrv`). The reference has 1,726 such calls and
  zero inline length reads. It affects only `dstring.h`, `stdio.h`, `stdlib.h`
  and adds no CodeGuard runtime imports (no `cg32.dll` in the reference).
- **`-v`** (source-level debug info). Per the RAD documentation, debug info also
  changes C++ inline expansion: with `-v` bcc32 does **not** expand inline
  functions, so trivial RTL methods (such as the `AnsiString()` default
  constructor) are called out-of-line, exactly as in the reference (3,008
  default-ctor calls). `ilink32 -v` writes the debug data to a `.tds` side file
  and leaves the exe stripped (no debug directory; characteristic `0x010e`).
- **`-Od`** (disable optimizations). The reference is unoptimized: parameters are
  reloaded from the stack rather than cached in callee-saved registers.

Codegen otherwise matches the reference (`__cdecl` members, RTTI on).

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
`scripts/README.md` for the full list. Make targets: `image`, `analyze`, `units`,
`functions`, `struct`, `disasm`, `sanity`, `compare`, `normalize`, `unit`,
`unit-asm`, `format`, `format-check`, `clean`.

- `make format` applies the project style (`.clang-format`: Allman braces,
  4-space indent, right-aligned pointers, 90 columns) to `src/*.cpp` and
  `src/*.h`; `make format-check` is the non-mutating equivalent.
  `SortIncludes` is deliberately off — include order is semantic here
  (`vcl.h`, then `#pragma hdrstop`, then the unit header). Formatting is
  whitespace-only and cannot affect codegen, but commit it separately from
  byte-level changes so the diffs stay reviewable.

- `scripts/borland.sh '<command>'` — run a tool in the container (exports `$B`,
  `$BZ`).
- `scripts/pe.py`, `scripts/extract_target.py` — dependency-free PE metadata and
  resource/DFM extraction.
- `scripts/units.py` — translation-unit table (order, end addresses, refcount).
- `scripts/unitmap.py` — attribute code addresses to modules. `--map` parses an
  `ilink32 -s` map (produced with `MAP=1 scripts/build.sh`) to name the object or
  library member behind each range; `--pe … --stubs` segments a PE at every
  module initializer stub and classifies unnamed modules as `library` (their
  bytes match the Borland libs) or `unknown`; `--units --functions --modules`
  turns the reference unit table into per-unit function inventories, excluding
  library/unknown module ranges.
- `scripts/compare_pe.py` — whole-file/header/section comparison, plus
  `--struct` for imports, exports and relocations.
- `scripts/compare_functions.py` — per-function byte scoring against the Ghidra
  inventory (`--mask-reloc` for layout-independent scoring).
- `scripts/compare_asm.py` — diff one function from a bcc32 `-S` listing against a
  reference address range; registers/offsets/constants must match, addresses and
  branch targets are canonicalized. Use this to drive a function to byte-match
  before moving on.
- `scripts/normalize_pe.py` — deterministic timestamp/header normalization.
- `scripts/disasm.sh` — linear `.text` disassembly.

`analysis/` and `build/` are generated and gitignored. `analysis/ghidra/functions.tsv`
is the read-only Ghidra function inventory.

## Source and reconstruction conventions

- Mirror the original translation units exactly, one file per unit, using the unit
  names recovered from the export table (lowercase file base, e.g. unit `Players`
  becomes `Players.cpp` / `Players.h`). The file base name must match the original,
  because it drives exported symbol names and therefore `.edata`.
- Keep the Borland dialect and ABI: 32-bit, `__cdecl` by default (`this` at
  `[ebp+8]`, matching the reference), VCL types (`AnsiString`, `TForm`, etc.)
  rather than STL substitutes.
- Unit operations compile in the **free-function form** in the reference: model
  them as `static` members (scoped name, no implicit `this`) or free functions
  taking an explicit object pointer. Constructors/destructors are members (the
  call site is `operator new` followed by a ctor call), so a unit mixes a member
  constructor with free-function-style operations.
- Reconstruct forms from the embedded DFM resources, not by hand-guessing layout.
  The `TGUI`, `TLoginDialog`, and `TPasswordDialog` resources are available in the
  reference `.rsrc`.
- Prefer the smallest construct that the compiler lowers to the observed bytes.
  Do not "improve" code; match it.
- Keep generated artifacts out of version control (see `.gitignore`); commit only
  source, forms, resources, and build scripts when asked.
- **Link layout.** `ilink32` lays out explicitly listed objects contiguously in
  command-line order (startup `c0w32.obj`, then the units in link order) and
  appends library members *after* them, so unit code is contiguous and a unit's
  `code_span` is its real size. Confirm against a build with
  `MAP=1 scripts/build.sh` and `scripts/unitmap.py --map`.
- **Unexported units.** `units.py` recognises only modules exported as
  `@@Unit@Initialize`; a unit compiled without `#pragma package(smart_init)` has a
  non-exported module stub and is invisible to it, so a neighbouring unit's span
  can straddle it. Scan stubs with `scripts/unitmap.py --pe … --stubs`, which also
  classifies each unnamed module: `library` if its bytes match a Borland lib
  member (with the reference's relocation slots masked, since library code is
  relocation-heavy), else `unknown`. **Do not reconstruct `library` members**;
  only `unknown` modules are candidate source units, and even those must not be
  given invented names. (Every unexported module in this image classifies as
  `library`.)
- **Functions are COMDATs.** bcc32 emits each function as a COMDAT, so `ilink32`
  discards unit functions nothing references; the linked skeleton therefore holds
  only the Initialize/Finalize stubs. Score functions from the compiler's `-S`
  listing (`compare_asm.py`), not the linked image, until the call graph is
  reproduced.

### String/AnsiString ABI (verified)

`String` is `typedef AnsiString String` (sysmac.h), and the two compile to
identical code (verified with `tests/string_types.cpp`; 0 normalized instruction
differences). Write natural `String` operations; they lower to the RTL primitives
the reference uses. Operation -> bcc32 symbol (`@System@AnsiString@...`):

| Operation | Symbol | Notes |
| --- | --- | --- |
| `s = x` | `$basg$qqrrx17System@AnsiString` | assignment |
| `s = "lit"` | `$bctr$qqrpxc` + `$basg` | temp + assign |
| `s += x` | `$badd$xqqrrx17System@AnsiString` | append |
| `s + x` | `$brplu$qqrrx17System@AnsiString` | concat (hidden ret ptr) |
| `s == x` | `$beql$xqqrrx17System@AnsiString` | equals |
| `String s = x` | `$bctr$qqrrx17System@AnsiString` | copy ctor |
| `String s = "lit"` | `$bctr$qqrpxc` | cstr ctor |
| `String s = c` (char) | `$bctr$qqrc` | char ctor |
| `String s = i` (int) | `$bctr$qqri` | int ctor |
| scope end | `$bdtr$qqrv` | destructor |
| `s.SubString(i,n)` | `SubString$xqqrii` | |
| `s.LowerCase()` | `LowerCase$xqqrv` | |
| `s.Trim()` | `Trim$xqqrv` | |
| `s.Pos(x)` | `Pos$xqqrrx17System@AnsiString` | |
| `s.Insert(x,i)` / `s.Delete(i,n)` | `Insert$qqrrx17System@AnsiStringi` / `Delete$qqrii` | |
| `s.SetLength(n)` | `SetLength$qqri` | |
| `s.ToInt()` | `ToInt$xqqrv` | |
| `s[i]` (read) | `ThrowIfOutOfRange$xqi` (or inlined) | **1-based** |
| `s[i] = c` | `ThrowIfOutOfRange` + `Unique$qqrv` | 1-based |
| `s.Length()` | inlined: `cmp Data,0` / `mov edx,[Data-4]` | no call |
| `IntToStr(i)` / `IntToHex(i,n)` | `@Sysutils@IntToStr$qqri` / `IntToHex$qqrii` | |
| exception frame | `@__InitExceptBlockLDTC` | same symbol in reference |

Ghidra's ad-hoc names (`AnsiString_Append`, `AnsiString_CharPtr`,
`AnsiString_Length`, `AnsiString_AppendDecimal`, ...) do not map one-to-one to
these; identify operations from the disassembly, not the name.

### Codegen fingerprints (verified while converging `Serial`)

These byte-level features distinguish source forms that look equivalent; use
them to pick the form that matches the reference.

- **Appending a single `char` to a `String`.** Write the cast explicitly, for
  example `s = s + String(ch)` (or `String(line[j])`). `s + ch` emits a
  `char`-constructor temporary that does not match; the explicit `String(...)`
  lets bcc32 reuse the constructor's `eax`, which is what the reference does
  (verified in `Serial::DecodeString`).
- **Positive-count guards lower two ways.** `if (n > 0)` compiles to
  `test eax,eax` / `jle`, while `if (n >= 1)` compiles to `dec eax` / `jl`. Same
  semantics, different bytes — the reference uses the `dec`/`jl` form, so write
  `>= 1` (verified in `Serial::Validate`, `Serial::ReadKey`).
- **EH scope markers are a structural fingerprint.** bcc32 emits
  `mov word ptr [ebp-N], imm` before EH-protected constructions and
  destructions; `imm` is the current cleanup scope's byte offset. The marker
  *sequence* encodes block nesting, so comparing marker streams is the quickest
  way to tell whether an `if`/`for`/block structure matches. Braces on an
  otherwise single-statement `if` add a scope (and a marker).
- **Parentheses can add a temporary.** `String x = (expr);` may introduce an
  EH-recorded temporary that `String x = expr;` does not (the frame grows), so
  expression parenthesization is observable in codegen.
- **Frame size is a fast filter.** A source-form guess that changes `add esp,-N`
  or the `[ebp-N]` offsets is wrong even if the instruction count looks close.
- **A `try`/`catch` around a body is a fingerprint, not an optional style.**
  A function that raises and catches inside itself emits extra EH scope markers
  (`mov word ptr [ebp-N], imm` armings the source form cannot otherwise produce)
  *and* an extra cleanup-table entry with flags `3` (typed, pointing at an RTTI
  descriptor) alongside the flags-`5` destructor entries for its locals — with
  the handler body appearing as a detached block (`result = 0; call ...`).
  Without the `try`, the scope ids stop short of the handler's and the frame is
  byte-identical, so the difference is invisible except in the marker stream.
  This is what closed `ClassValues::DecodeInt`; suspect it for any function whose
  marker stream ends one scope short and whose record lacks a flags-3 entry.
- **Encoded strings.** Where the reference carries an obfuscated literal (decoded
  through `Serial::DecodeString`), name a macro after the decoded text so intent
  is legible, e.g.
  `#define SERIAL_ENC_STR_CONFIG_SERIAL_INI "d1dqa>d-h,B8d9_0jpC"` (decodes to
  `./config/serial.ini`).

## Workflow for a single translation unit

1. Identify the unit's address range from `analysis/target/units.tsv` (the unit
   code ends at its `Initialize` stub) and its functions from the Ghidra
   inventory.
2. Reconstruct the header (class layout, member offsets, virtual table order) and
   the `.cpp` in Borland C++. Derive function order from the reference addresses.
3. Compile with `make unit UNIT=<Unit>` and inspect codegen with
   `make unit-asm UNIT=<Unit>`; resolve the source form per function by matching
   the reference bytes (see the codegen pitfall below).
4. Score progress with `make functions` (per-function, layout independent) once a
   linkable image exists.
5. Update the unit's status in [PLAN.md](PLAN.md#translation-unit-inventory) and
   stop.

### Driving one function to byte-match

`compare_asm.py ASM FUNC MANGLED_PREFIX REF_START REF_END` diffs a single bcc32
`-S` function against a reference address range. Registers, stack offsets and
small constants must match; relocated addresses and branch targets are
canonicalized, and callee-saved pushes plus trailing `nop` padding are trimmed.

```sh
make unit-asm UNIT=Serial
python3 scripts/compare_asm.py build/Serial.asm SetIniPath \
    '@Serial@SetIniPath' 0x41250c 0x41278c
```

Take `REF_END` from the next function's start; the unit's last function ends at
its `@@Unit@Initialize` stub. A `REF_END` that overlaps the following function
reports thousands of bogus mismatches.

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
- **Source form changes codegen.** An instance method and a free function with an
  explicit object pointer are not byte-equivalent for assignments: bcc32 emits the
  mirror register order. The reference's unit operations match the free-function
  form (verify with `make unit-asm`), so write them as `static` members/free
  functions; keep constructors as members.
- **`s + ch` vs `s + String(ch)`.** Appending a bare `char` produces a
  `char`-ctor temporary that the reference does not use; cast with `String(...)`.
- **`n > 0` vs `n >= 1`.** Both are correct C++ but compile to different bytes
  (`test`/`jle` vs `dec`/`jl`); the reference uses `>= 1`.
- **Synthetic Ghidra blocks.** The database contains `*_reasm` functions at
  `0x60000000+` that are not part of the PE; reference comparisons must ignore
  them.

## Reference docs in `ref/`

- `Borland_C++_Version_5_Programmers_Guide_1997.pdf` — compiler/linker semantics
  and language behavior for this exact toolchain generation.
- `calling_conventions.pdf` — Agner Fog: register/stack rules, Borland name
  mangling (section 8.2), 32-bit memory models, object formats, and unwinding.
- `Borland5/Source/RTL/` — RTL and startup sources, including `c0nt.asm`.
- `eo-protocol/` — git submodule (cirras/eo-protocol): the XML specification of
  the Endless Online pub file formats. Use it for record layouts and field names
  when reconstructing the `*values`/pub parsers: `xml/pub/server/protocol.xml`
  documents `ECF`/`EIF`/`ESF`/`EMF`/`EID`/`EDF`/`ETF` records (for example
  `ShopCraftRecord` = `short item_id, ShopCraftIngredientRecord ingredients[4]`).
  The XML layout is the on-disk format; the in-memory classes may expand fields
  (e.g. `ShopCraftVal` stores the four ingredients as two `int[4]` arrays).
