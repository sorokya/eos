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

The base is `debian:trixie` (Wine 10.0). Its compiler/linker output is byte-identical
to the earlier `debian:bookworm` (Wine 8.0) image — `make verify` 496/496 and
`make track` 1795/1796 both hold under either — so the upgrade is safe. Two
toolchain quirks are worth recording:

- **`ilink32 -s -v` faults under Wine at the end of the link** ("Unhandled illegal
  instruction"), with both Wine 8.0 and Wine 10.0. It writes the *complete* detailed
  segment map first, then dies, leaving a corrupt exe. `-s` without `-v` and `-m`
  both link cleanly but only produce the 276-byte summary map, so the detailed map
  is only obtainable from the faulting combination. `MAP=1 scripts/build.sh` handles
  this: it runs the detailed-map link tolerantly under a separate output base
  (`build/GameServer_map.exe` → `build/GameServer_map.map`) and then relinks
  `build/GameServer.exe` cleanly, so both artifacts are always valid.
- **`tlib.exe` mis-parses a command line containing `(`** — i.e. anything invoked
  through `$B` (`C:\Program Files (x86)\Borland\CBuilder5`): it fails with
  `Error: unexpected char '(' in command line` even with no arguments. Invoke it
  through the paren-free `$BZ` path instead, e.g.
  `wine "$BZ\Bin\tlib.exe" "$BZ\Lib\Debug\vcl50.lib" '*sysinit*'` to extract a
  library member. `bcc32`/`ilink32`/`brcc32`/`tdump` parse their command lines
  normally and are unaffected.

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
- The application is multithreaded and statically linked, using the C++
  *package* RTL (`cp32mt.lib`) plus the **Debug** VCL/BDE libraries in
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
    import32.lib cp32mt.lib vcl50.lib vcldb50.lib vclbde50.lib
```

**`cp32mt.lib`, not `cw32mt.lib`.** A VCL application must link the package
RTL. `cw32mt.lib` carries the stubbed-out `crtlst_i.c` / `crtlst_e.c` /
`crtlst_l.c` versions of `___CRTL_VCL_Init`, `___CRTL_VCL_Exit` and
`___CRTL_VCLLIB_Linkage` (each a bare `ret`); `cp32mt.lib` carries the real
`crtlvcl.cpp`. `c0w32.obj` references all three. With `cw32mt.lib` listed
first the stubs win and the whole VCL init chain never starts: nothing
references `__InitVCL`, so `vcle50.lib|VCLINIT` is never pulled, so nothing
references `@Sysinit@VclInit` / `@Sysinit@VclExit`, so the SysInit communals
stay out of the image entirely. That is what the missing 377-byte block at
`0x401150` was; with `cw32mt.lib` dropped the head matches the reference
exactly and the image goes from 9,216 bytes too large to 1,024 too small.
Note that the SysInit code is emitted from *communal* (`COMDEF`) definitions
in `Lib/Obj/sysinit.obj`, so only the communals something actually references
are emitted -- in the reference that is exactly `_16393`, `_16394`,
`VclInit`, `VclExit`, `Finalization`, `initialization`.

The library set covers the components observed in the target (`vcl50.lib` for
VCL/forms/sockets, `vcldb50.lib` for the `Db` unit, `vclbde50.lib` for
`DBTables`). Library objects are pulled on demand, so unused libraries do not
affect the bytes; the exact set and order are finalized in Phase 2 by matching
the linked object set. The `-L` directories mirror the paths in `ilink32.cfg`;
`Lib/Obj` is required for the VCL `.res` files (for example `Controls.res`).

### Compiler flags

Four flags are required for byte fidelity, all passed by `scripts/build.sh`
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

- **`-tWM`** (multithreaded RTL target, defines `__MT__`). The reference's
  iostream/RTTI layouts are the multithreaded ones (`basic_streambuf` 68,
  `basic_filebuf` 100, `basic_ofstream` 200 bytes, each carrying the
  `_RWSTD_MULTI_THREAD` mutex) and the link uses the multithreaded RTL
  (`cp32mt.lib`); without
  `__MT__` stdcomp.h picks the single-threaded layouts and any unit using
  iostreams (e.g. `Msgboardcontrol::SaveBoards`) diverges.

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

The COFF header is not the only clock stamp: ilink32 also writes the current
time into the `TimeDateStamp` of **every** `IMAGE_RESOURCE_DIRECTORY` in
`.rsrc` (45 of them here; the reference carries `0x418F00E9` in all). Until
this was normalized, two back-to-back relinks of identical objects produced
different `.rsrc` bytes and different whole-file MD5s. `normalize_pe.py` now
rewrites them by default (`--resource-timestamp=-1` opts out).

The normalization step is implemented and verified: taking a copy of the
reference with only the timestamp changed, `normalize_pe.py` restores the
reference MD5 exactly, and it is a byte-level no-op on the reference itself.
Two successive `LINK_ONLY=1 scripts/build.sh` runs now produce identical
files. Whatever mechanism is chosen must be part of the
documented build, not a manual fix-up.

## Harness

`scripts/` and the `Makefile` provide the measurement pipeline; see
`scripts/README.md` for the full list. Make targets: `image`, `analyze`, `units`,
`functions`, `struct`, `disasm`, `sanity`, `compare`, `normalize`, `unit`,
`unit-asm`, `verify`, `format`, `format-check`, `clean`.

- `make verify` compiles every unit to a bcc32 `-S` listing
  (`scripts/build_asm.sh`) and scores every function in a unit's own namespace
  against that unit's reference ranges (`scripts/verify_units.py`), reporting
  per-unit matched/total and failing on any unmatched source function.
  It is incremental (a unit recompiles only when its `.cpp` or any header is
  newer than its listing; `FORCE=1` forces all) and parallel
  (`JOBS=N`, default the container's CPU count). `scripts/verify_units.py`
  invokes `objdump` once per reference range and runs them in parallel: a single
  whole-image sweep desynchronises on data embedded in `.text`, so it must not
  be replaced by one pass over the image.

- Library members are identified by matching reference functions against the
  *linked* build (`scripts/libmatch.py`, `MAP=1 scripts/build.sh`): the Borland
  libraries are the same code compiled by the same toolchain, so a match inside a
  named library module of the linked image is exact and gives the member name.
  This is how a named unit's span that absorbs stub-less library members is
  classified correctly (Banned's first ~57 KB is the RTL `System` member, not
  application code). Without a linked build the classifier falls back to a
  reloc-masked signature over `ref/Borland5/Lib`, which is complete but noisier;
  the cache records which mode produced it.

- `make track` regenerates the central per-function status sheet
  (`analysis/target/functions.tsv`, generated) and the status block in
  `README.md`: one row per reference function with its unit, address, name,
  `kind` (app / comdat / library / stub) and `status` (byte-exact / mismatched /
  unimplemented / deferred). It is the source of truth for per-function
  progress; `PLAN.md` keeps phases, risks and the deliberate exclusions.
  `$bdtr` (deleting-destructor) COMDATs that nothing references are reported
  separately — the linker drops them, so the reference has no range for them.
  This is the whole-tree check; `compare_asm.py` remains the per-function tool.

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

  **Always regenerate the map and the exe together.** The map describes one
  particular link; any later relink (with or without `MAP`) changes
  `build/GameServer.exe` while leaving `build/GameServer_map.map` behind, and
  mixing a stale map with a fresh exe silently produces wrong attributions —
  e.g. "this unit is only 56 bytes" when it is not. `MAP=1 scripts/build.sh`
  emits both from the same link for exactly this reason.
- `scripts/compare_pe.py` — whole-file/header/section comparison, plus
  `--struct` for imports, exports and relocations.
- `scripts/compare_functions.py` — per-function byte scoring against the Ghidra
  inventory (`--mask-reloc` for layout-independent scoring).
- `scripts/compare_asm.py` — diff one function from a bcc32 `-S` listing against a
  reference address range; registers, stack offsets and small constants must match,
  addresses and branch targets are canonicalized. Use this to drive a function to
  byte-match before moving on. **The default comparison is address-canonicalized
  and therefore cannot see a wrong literal or a wrong direct callee**: a reference
  to `"',"` and one to `"'"` both render as `mov edx,ADDR`, and a call to
  `Mysql_ExecDirect` and one to `Mysql_ExecDirect_FromCallback` both render as
  `call ADDR`. `--strict-operands` adds the value behind each canonicalized
  address: string/data-literal bytes are compared (a data address read from the
  reference vs our `offset <label>+N` blob), a direct `call` target is compared by
  callee identity (the reference row's mangled symbol from
  `analysis/target/functions.tsv`, with the project's renamed COMDAT accessors such
  as `Players_Iter_Begin` accepted via `ref_name`, and unidentified `FUN_` rows
  left unclassified rather than guessed; library targets are resolved by the
  consistency of a symbol's own call sites and labelled with the Ghidra name), and
  non-address immediates are compared numerically. A relocated operand
  (`[0x…]` slot, an EH/vtable pointer, an address in `.text`) is treated as
  layout-dependent and never flagged, and an operand it cannot classify is
  counted, not guessed. Default output and exit status are unchanged when the
  flag is absent; with it, any operand difference makes the exit non-zero. This
  is how the `questblob`/`questblob2` literal swaps, the
  `GetMaxClones`/`GetMaxConnections` call and the vector-element-type call
  differences were found in functions the sheet calls `byte-exact` (meaning
  *canonicalized*-exact). `--frame-wild` is a **progress** mode for a
  partially written function: bcc sizes the frame only once every local exists, so
  until then the prologue `add esp,-N` differs and the unflagged comparison shows
  thousands of mismatches even when the written body matches. The flag wildcards
  the prologue frame (and an identical-magnitude epilogue release) on both sides,
  prints `frame: ref … ours … (delta …)`, and reports the `aligned prefix`. It
  relaxes **nothing else** — registers, non-frame stack offsets, constants,
  operand order and instruction count stay exact — and it is never an acceptance
  criterion: final acceptance is the unflagged comparison plus the whole-file MD5.
  `--stack-wild` (implies `--frame-wild`) is the companion progress mode for the
  stack *offsets*: with frames differing by `D`, every `[ebp-N]` differs and a
  correct body still mismatches. It derives `D = |ref frame| - |our frame|`,
  rewrites our `[ebp-N]` to `[ebp-(N+D)]`, prints `stack delta applied: …` and
  the distinct `[ebp-N]` slot sequence of both sides, then compares. Ordering is
  still strict (a reordered local list shifts to different numbers and still
  mismatches); `[ebp+N]` arguments and `[esp+N]` operands are not shifted, and
  registers, constants, operand order and instruction count stay exact. The frame
  delta includes the EH-frame overhead, so the local-area delta can differ from
  it; `--frame-wild` also prints the `implied stack delta` (the delta the first
  divergent slot pair implies) and `--stack-delta D` applies an explicit one.
  `--stack-search` sweeps candidate deltas (`-0x400`..`+0x400` step 4, plus the
  frame-derived and implied ones) and prints the best delta with its aligned
  prefix and mismatch count plus the runner-ups, whether the win is decisive or
  a tie, the winner's local-area delta versus the frame-derived one, and a
  classification of the first divergence (`stack-offset` / `operand/register` /
  `opcode/structure`), then the normal reading.
  Condition-code aliases (`jge`/`jnl`, `jl`/`jnge`, `jb`/`jnae`/`jc`, … and the
  `setcc`/`cmovcc` forms) are canonicalized to one representative per class
  before comparing (mnemonic only — operands are untouched and distinct
  conditions are never merged). Both tolerant modes are progress instruments,
  never acceptance criteria.
- `scripts/asm2cpp.py` — draft C++ from a reference address range: one
  address-commented line per instruction, recognising the documented AnsiString
  operations, `EO_*` calls, field accesses (names resolved from the `// +0xNN`
  comments in `src/*.h`, with container-iterator typing so `(*iter)->field`
  resolves), control flow with the actual `cmp`/`test` + `jcc` conditions, EH
  scope arming and local parsing. **Its output is a DRAFT to verify with
  `compare_asm.py`, never an authority.** Four safety properties are enforced and
  printed by `--selftest`: a condition is emitted only when its relation and
  operands are provable (otherwise a TODO and no `if`); two distinct `[ebp±N]`
  storage locations never render to the same identifier; a field NAME is emitted
  only when the base's class is proven; an argument list is emitted only when the
  declared arity, the pushes and the caller's `add esp` adjust all agree. It
  changes often — pin a hash before relying on a run, and never run it against a
  copy another session is editing.
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
- **Put the `AnsiString` operand first in a concatenation.** A string literal on
  the **left** of `+` binds the `char *` + `AnsiString` operator, emitting a
  different call than the reference's 3-argument `AnsiString` + `AnsiString`
  concat. Write `EO_EncodeNumber(...) + "NO"`, not `"NO" + EO_EncodeNumber(...)`
  (verified in `Player_HandlePacket` case `0x3`).
- **Register allocation is a whole-function property; do not "fix" a residual by
  re-ordering a comparison.** A controlled probe showed the same source
  (`player->level < item->level_requirement`) compiles to different register
  assignments depending only on the surrounding function's locals; the mirrored
  form (`item->level_requirement > player->level`) produces identical bytes, and
  the same class appears in a plain assignment (`player->field_0x80 = rule;`)
  where no operand-order freedom exists. A comparison/assignment register mirror
  in a *partially written* function is therefore bcc register allocation against an
  incomplete frame, not a source-form error — match the operands and move on;
  the whole-function comparison is the only authority (established while writing
  `Player_HandlePacket`, `tests/guard_probe.cpp`).
- **A "register-allocation floor" is usually a cascade — do not accept one until
  the *first* divergence is explained.** Settling five large functions showed
  that a handful of real source differences each masqueraded as dozens or
  thousands of register/slot mirrors, because bcc's temporary free-list and slot
  numbering are global. `Player_HandlePacket` went 205 → 0 hunks, `Spell_Execute`
  32 → 0, `Attack_Execute` 10 → 0, `EO_Decode_Deinterleave` and `FUN_00462374` to
  0, all after earlier passes had recorded them as floors. Always locate the
  first divergence (by instruction *count/opcode*, not by register names) and fix
  that; re-measure; only then classify what is left.
- **An unsigned product defeats bcc's in-place fold.** `player->w = player->w -
  GetWeight(1) * amount;` folds to `sub [mem],eax`; casting the product —
  `player->w = player->w - GetWeight(1) * (unsigned int)amount;` — makes bcc keep
  the result in a register and re-evaluate the object pointer for the store:
  `mov ecx,[edx+0x13c]; sub ecx,eax; mov eax,[player]; mov [eax+0x13c],ecx`.
  This was the last 17 hunks of `Player_HandlePacket` (verified by
  `tests/weight_probe.cpp`; ~14 signed spellings all fold).
- **A declared-and-initialized but unread scalar still consumes a frame slot.**
  `int behavior_id = type_info.behavior_id; player->session_token =
  type_info.behavior_id;` (the later use re-reads the *field*, so the local is
  never read) emits one store the reference has. `bcc32 -Od` allocates every
  declared local, so a missing slot at a constant `+4` delta across a whole
  function is often exactly this. Adding it collapsed the last 166 hunks of
  `Player_HandlePacket` (`tests/statopen_probe.cpp`).
- **Chained self-assignment emits a duplicate store.** `int offset_x = offset_x =
  caster->x;` is the only form that reproduces the reference's doubled
  `mov [offset_x],reg` (separate/comma declarations, casts, parens and two
  consecutive assignments all emit one store). This made `Attack_Execute`
  byte-exact.
- **Concatenation operand order determines temporary slot allocation.** `A + B`
  and `B + A` are the same bytes *except* that bcc allocates `A`'s temporary then
  `B`'s, so a transposed pair permutes the free-list and cascades for thousands of
  instructions. Fixing one transposed `data.SubString(1,4) + EO_EncodeNumber(...)`
  in `Player_HandlePacket` moved the aligned prefix 9,511 → 28,765.
- **A positive `&&` wrapper is not equivalent to a negative guard-with-`return`.**
  `if (skill_type != 1 || type <= 0 || type >= 6) return 1;` and
  `if (skill_type == 1 && type > 0 && type < 6) { … }` differ in which exit tail
  the return shares and how the body's temporaries are scoped. Rewriting the NPC
  damage body of `Spell_Execute` as the positive wrapper removed all 32 of its
  remaining hunks.
- **An `if` whose body returns still needs its `else`** where the reference emits
  a jump over the else body (`Face`, `Chair`, `Sit` all needed this).
- **A trailing `return` inside a block the reference lets fall through must be
  deleted** — six such sites in `Player_HandlePacket` accounted for a 16-instruction
  surplus each.
- **Source order of two emitted regions is observable.** Moving the Guild Create
  guards ahead of the banned-word chain removed a 140-instruction relocation.
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
- **A destructible temporary forces an arms-then-re-arm pair.** bcc32 arms a
  cleanup scope for every statement that binds a **destructible temporary**
  (`String`/`AnsiString`, a class with a destructor), then immediately re-arms
  the **enclosing** scope before the next statement; the no-op terminator entry
  (`flags=5, extra=0, action=0`, whose `scope` field chains back outward) is its
  descriptor signature. This is a property of the temporary, *not* of `try` and
  *not* of loops — a variant with no `try` still re-arms, just renumbered. Two
  consecutive `String x = decode(...);` statements in a loop body produce the
  `arm tempA, re-arm enclosing, arm tempB, re-arm enclosing` sequence. A plain
  **scalar** decode (`int code = decode(...)`) never arms a cleanup scope at all,
  which is the usual reason a marker stream is one arming short. Falsified as
  causes: temporaries in a `for`-condition, single-statement `for` bodies, and
  declarations without initialisers (they arm nothing). Established by an
  exhaustive 28-variant probe matrix; see `tests/rearm_probe.cpp`.
- **A block-scoped declaration *with an initializer* arms the enclosing scope;
  a bare `{ }` does not.** A plain block owns nothing destructible, so it emits
  no marker — but `int sit_state = 0;` declared inside an `if` and
  `int cheat_x = ...; int cheat_y = ...;` inside an `else` each arm the
  enclosing scope *after* the initializer store, producing one marker each. This
  is what `Refresh_BuildReply` needed for its missing `arm 0x20` at `0x45e973`
  and `0x45ee06`; the frame slot order then pins the declaration positions
  (`count`, `iter`, `sit_state`, `niter`, `cheat_x`, `cheat_y`, `iiter` =
  `232, 236, 240, 244, 248, 252, 256`). Before reaching for a `try`/`catch` or a
  result clause, test whether the missing marker is just a scoped initializer.
- **Passing a value to a `String` parameter: write the implicit conversion, not an
  explicit `String(...)`.** For a value converted to a `String` argument (e.g. an
  `int`/`char` from `EO_GetBreakByte`, or a `char` element), the reference
  materializes the temporary on the stack and passes its address
  (`lea ecx,[ebp-N]; push ecx`). Writing the explicit `String(x)` instead makes
  bcc32 reuse the constructor's return in `eax` (`push eax`), which is the sole
  difference in `Client_SendRaw`, `Face_Execute` and `PacketReader_GetBreakString`.
  (This is the argument-position counterpart of the `s + ch` -> `s + String(ch)`
  rule above, which applies to concatenation, not to arguments.)
- **Parentheses can add a temporary.** `String x = (expr);` may introduce an
  EH-recorded temporary that `String x = expr;` does not (the frame grows), so
  expression parenthesization is observable in codegen.
- **A struct local with an empty user constructor triggers the folded EH thunk.**
  bcc32 emits its `___InitExceptBlockLDTC` frame-only constructor for a local whose
  type has a user-declared empty constructor and an anonymous aggregate member
  (the pattern already used for `ItemElement`/`ItemSpecXY`/`MapCoord`); the linker
  folds every copy to `0x44f58c`, giving `lea eax,[&local]; push eax; call 0x44f58c;
  pop ecx` instead of the direct `mov eax,<desc>; call 0x54ba98`. A function that
  declares such a local gets the thunk; one that only uses scalars does not. This is
  what distinguishes `Player_Respawn`/`Player_CheckIdleWarp` from `Chair_Execute`.
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
  To confirm it on a reference function, read its EH frame descriptor (`mov
  eax,<desc>; call __InitExceptBlockLDTC` at the entry): the descriptor's entries
  have a `dw 3` flags word when a catch is present, and the catch's handler emits
  `mov word ptr [ebp-N],imm; call 0x55623d` (`__CatchCleanup`). `tests/scope_probe.cpp`
  isolates the effect: function `A` (no try) arms `0x14,0x8,0x20`; function
  `B` (the same body wrapped in `try { ... } catch (...) {}`) additionally arms
  `0x2c` (the try body) and `0x28` (the catch), and its ECT gains a flags-3
  entry. `NpcRange_Lookup` is a concrete case: the reference has a flags-3 entry
  and a `call 0x55623d` at `0x462328`, and wrapping its outer `if` in
  `try`/`catch` reproduces the reference's `0x20` arming the source otherwise
  lacks. (Do not assume every "extra scope arming" is a try/catch: check the ECT.
  `Server_BuildOnlineNames` and `Party_EncodeMemberList` have no flags-3 entry
  and no `0x55623d` call, so their extra armings are the returned-append clause
  described next.)
- **A returned `AnsiString` append arms a result clause.** When a function returns
  an `AnsiString` *by value* and its returned expression appends to a string
  (`return *out += names;`, or `String result = ...; result += names; return
  result;`), bcc32 materialises the result and then guards the destructor of the
  appended temporary:
  ```
  arm S;  <append>;  mov eax,[dst];  arm S+0xc;
  push eax;  <destroy the appended temporary>;  pop eax;  arm S;  inc [scope]
  ```
  i.e. the destination pointer is saved across the destructor and the result
  clause is re-armed and incremented. The scope ids are cleanup-table byte
  offsets, so they grow with the function — match the *sequence*, not the
  number; the tell is the second arming (`S+0xc`) between the append and the
  `push eax`. This is **not** a hidden `try`/`catch`: wrapping the append in
  `try { ... } catch (...) {}` gives the save/restore but no arms *and* adds a
  flags-3 cleanup entry the reference lacks. Nor is it a plain append: a
  `void`- or pointer-returning function with the same statements emits no arms
  (a pointer return may show the save/restore alone). `tests/append_return_probe.cpp`
  isolates it — `A`/`B`/`C` (by-value) arm the `S, S+0xc, S` triple (`A` arms
  `0x20,0x2c,0x20`) while `D` (void) and `E` (pointer) do not and `F`
  (try/catch) arms the catch instead. The mangler
  does not encode an `AnsiString` by-value return, so `@@F$qp17System@AnsiString...`
  reads the same whether `F` returns `void` or `String`; check the body, not the
  symbol. The three Packets residuals are this idiom — the returned `String` is
  the append destination — which is why `Server_BuildOnlineNames`,
  `Message_BuildServerStatus` and `Party_EncodeMemberList` carry the `0x38/0x44`,
  `0x80/0x8c` and `0x74/0x80` pairs.
- **A class with a member container: an *empty* destructor is the whole function.**
  bcc generates the scalar/deleting destructor itself — `call vector<Player*>
  ::~vector(this, 2)` for the member, then the `operator delete` tail folded into
  the same function. `Players::~Players() { }` (nothing in the body) reproduces
  the reference's 24 instructions exactly; writing `players.clear()` or calling
  the member's destructor by hand gives a different form.
- **Applying the returned-`String` builder recipe** (`Walk_BuildReply`,
  `Player_SerializeAvatar`). The accumulator is not always initialised from `""`:
  `Player_SerializeAvatar` starts it from a field (`String out = player->name;`).
  The first insert can also sit *outside* the `try`, and the catch is then empty
  (`catch (...) { }`, no `buf += ""`). Read the catch handler (`call 0x55623d`
  `__CatchCleanup`) before assuming the shape, and check the EH cleanup table for
  the flags-3 clause entry to confirm a catch exists at all.
- **A `T** - T**` size difference divides by the element size.** bcc lowers the
  ptrdiff with its generic signed-division sequence, so `(end - begin)` over
  4-byte elements emits `sub` / `test` / `jns` / `add 3` / `sar 2`, not a bare
  `sub`/`sar`. `GroundItemPtrVector_Count` (`0x44f8b0`) is exactly this; casting
  both sides to `char *` removes the scale and the match.
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

Outstanding work is tracked in PLAN.md: "Outstanding cross-unit externs and
stubs" (declarations left because no owner header declares them) and
"Known-unconverged functions". Add to or remove from those lists whenever you
introduce or resolve such an item, and never let a cross-unit declaration
diverge from its owner header — a signature mismatch forms a distinct overload
that does not link.

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

`--frame-wild` scores a *partially* written function. The frame size is a
function-wide fact, so a missing local changes `add esp,-N` at instruction 2 and
shifts every `[ebp-N]`, making the unflagged score meaningless until the whole
function exists (this is why a large function is unverifiable block by block).
The flag wildcards the frame instruction on both sides, prints the size
difference, and reports the aligned prefix — the count of leading instructions
that genuinely match. It is a **progress** instrument, never an acceptance one:
an aligned prefix that stops early means the *bodies* diverge, not the frame
(check the first mismatching instruction). `--stack-wild` adds the stack-offset
half of the same idea: it shifts our `[ebp-N]` by the frame delta so the shared
lower part of the frame compares, while keeping the slot *order* strict. Neither
flag is an acceptance criterion; acceptance is still the unflagged
`compare_asm.py` result and, finally, the whole-file MD5.

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
