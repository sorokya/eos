# PLAN — Rebuilding `GameServer.exe` Byte-for-Byte

This is the working roadmap for the decompilation. It records what is already
proven about the target, the strategy used to reconstruct it, the phases and their
exit criteria, and the current status of the translation-unit inventory.

## Objective

Produce original-style Borland C++Builder 5 source that, compiled with the
toolchain in `ref/Borland5` and linked with `ilink32`, yields a PE image whose MD5
is exactly:

```
075fb5db1d6369bf488cd2ad0c715ddd
```

Success is a single, reproducible `make`-style command (running in the Docker/Wine
container) that emits `build/GameServer.exe` matching the reference, with no manual
byte patching other than an explicitly documented deterministic timestamp step.

## Strategy: C++-first, TASM fallback

Reconstruction is C++-first, with byte-exactness as a final convergence phase:

1. Each translation unit is reconstructed in Borland C++ from the Ghidra
   decompilation and the reference disassembly, then compiled with `bcc32`.
2. Functional equivalence (a linkable, runnable server) comes first; per-function
   byte convergence follows, unit by unit.
3. TASM is used surgically: as a byte-fidelity fallback for individual functions
   the compiler cannot be coaxed to reproduce, and as stubs that keep the build
   linkable while units are partially reconstructed.
4. Library code (RTL, VCL, BDE) is not reimplemented; it comes from the unmodified
   `.lib`/`.obj` files in `ref/Borland5/Lib`. The whole-image MD5 is the final
   arbiter.

Rationale: C++ exception handling is lowered onto SEH and is pervasive in this
image (the `FS:[0x0]` frame pattern appears in well over a thousand functions), so
hand-authoring EH frames in TASM is impractical. The compiler emits EH, RTTI,
strings and vtables correctly once the source matches.

## Findings (Phase 0/1)

- **Unit table.** The 65 `@@Unit@Initialize`/`@Finalize` pairs give the unit link
  order, and each `Initialize` stub sits at the **end** of its unit's code segment
  (the preceding function ends in `ret` + `90 90` padding). Units are contiguous in
  the observed range. `scripts/units.py` emits the table.
- **Symbol-name freedom.** The shipped image is stripped, so internal function
  symbol names are not observable. Only the 133 export names, RTTI/type-name
  strings, and DFM method names must match for byte-exactness. C++/TASM interop can
  therefore use arbitrary internal names as long as our declarations agree.
- **Calling convention and unit structure (resolved).** bcc32's default member
  convention is `__cdecl` with `this` at `[ebp+8]`, matching the reference. But
  **source form is visible in codegen**: an instance method and a free function
  with an explicit object pointer are not byte-equivalent (the assignment register
  order mirrors). The reference's unit operations match the **free-function form**,
  so model them as `static` members (scoped name, no `this`) or free functions
  taking an explicit object pointer. `Serial`'s constructor *is* a member (the
  call site is `operator new` then a ctor call), so a unit mixes a member
  constructor with free-function-style operations.
- **Ghidra database.** Read-only. 7,425 functions exported; ~14% carry non-default
  names, so names are hints only. The project also contains synthetic analysis
  blocks at `0x60000000+` (`*_reasm`) that are not part of the PE and must be
  ignored by reference comparisons.
- **`String` == `AnsiString`.** `String` is `typedef AnsiString` (sysmac.h) and
  the two compile to identical code (`tests/string_types.cpp`, 0 normalized
  instruction differences). Natural `String` operations lower to the RTL
  primitives the reference calls; the operation-to-symbol mapping is recorded in
  [AGENTS.md](AGENTS.md#stringansistring-abi-verified).
- **Compiler flags (resolved).** The reference build uses `-D__CODEGUARD__`
  (CodeGuard compile-time checks, which disables the `dstring.h` inlines so
  `AnsiString::Length()` is an out-of-line call — 1,726 sites), `-v` (debug info,
  which also disables C++ inline expansion so trivial RTL methods such as the
  `AnsiString()` default constructor are called out-of-line — 3,008 sites; the
  debug data goes to a `.tds` and the exe stays stripped), and `-Od` (no
  optimization). Recorded in [AGENTS.md](AGENTS.md#compiler-flags).
- **Candidate units.** Spans between `Initialize` stubs are not unit sizes: large
  gaps contain real code (e.g. `Packets`) and possibly interleaved library objects,
  to be resolved with a linker map.
- **Skeleton build.** `make build` compiles all 66 units (currently stubs) and
  links a working PE whose export table has the same 133 names as the reference.
  Unit order follows `units.tsv`, but the linker places one unit (`Quest`)
  differently while code segments are empty; this placement artifact is to be
  re-checked once units carry real code.
- **Main unit.** `GUI.cpp` is the project main: it defines the `TGUI` form class,
  a `PACKAGE` global `TGUI *GUI`, and `WinMain`. With `PACKAGE` on the global (but
  not the class) the linker emits exactly the reference's `_GUI` export and no
  extra class ctor/RTTI exports. Ordinary units use `#pragma package(smart_init)`
  to emit their `@@Unit@Initialize`/`@Finalize` pair.
- **`Serial` convergence (Phase 1 pilot).** 10 of 13 `Serial` functions are
  byte-exact, including `Validate` (467/467) and `DecodeString` (290/290). The
  remaining three (`SetIniPath` 155, `ReadKey` 254, ctor 246 mismatches) differ
  only in **EH scope structure**: the reference's `mov word ptr [ebp-N], imm`
  marker stream nests one more scope than our source reproduces, and its
  per-statement `AnsiString` temporaries occupy distinct slots. The source-form
  rules learned here (explicit `String(ch)` casts, `n >= 1` guards, EH marker
  streams as a nesting fingerprint, parenthesization adding temporaries, named
  macros for encoded literals) are recorded in
  [AGENTS.md](AGENTS.md#codegen-fingerprints-verified-while-converging-serial).

## Confirmed target facts

All facts below were derived during repository initialization; each lists the
method used to establish it.

| Fact | Value | How established |
| --- | --- | --- |
| MD5 | `075fb5db1d6369bf488cd2ad0c715ddd` | `md5` on the reference |
| Size | 1,733,120 bytes | filesystem |
| Format | PE32 GUI, `coff-i386` | `objdump`, `pefile` |
| Linker | 5.0, PE executable, GUI subsystem | PE header fields |
| Link flags | `-Tpe -aa -c -Gn -j -v` | scratch link reproduces header `0x010e` and section geometry |
| Static libraries | `import32.lib cw32mt.lib vcl50.lib vcldb50.lib vclbde50.lib` | minimal VCL+BDE link resolves; set/order finalized in Phase 1 |
| Header characteristic | `0x010e` | PE header; reproduced with `ilink32 -v` |
| Timestamp | `0x50CBB124` at file offset `0x208` | PE header (`e_lfanew + 8`) |
| Entry point | `0x00401000` | PE header |
| Image base | `0x00400000` | PE header |
| Section order | `.text .data .tls .rdata .idata .edata .rsrc .reloc` | PE section table |
| Alignment | file `0x200`, section `0x1000`, headers `0x600` | PE header |
| Imports | 8 DLLs, 413 functions | import directory |
| Exports | 133 symbols | export directory |
| Main unit | `GUI` (`_GUI` export) | export table |
| Forms | `TGUI`, `TLoginDialog`, `TPasswordDialog` | `RT_RCDATA` DFM resources |
| Startup | `c0w32.obj` (built from `c0ntw.asm` → `c0nt.asm`) | `WinMain` reference in `c0w32.obj`; RTL sources |
| Threading | multithreaded (TLS, threads, critical sections) | imports and `.tls` section |

Reproducibility of the link configuration was confirmed by building a scratch
Windows GUI object and a minimal VCL+BDE app and linking them with
`-Tpe -aa -c -Gn -j -v`:

- Section names, order, base addresses, and alignments matched the reference.
- `-v` produced header characteristic `0x010e`; omitting it produced `0x030e`.
- No debug directory was emitted, matching the reference.
- The minimal VCL+BDE link resolved with `import32.lib cw32mt.lib vcl50.lib
  vcldb50.lib vclbde50.lib` and the three `Lib` search paths, reproducing every
  reference header field.

The exact object selection from those libraries is validated in Phase 1 by
matching the linked object set against the reference.

## Proposed repository layout

```
src/        reconstructed C++ per translation unit (Unit.cpp / Unit.h)
asm/        TASM per translation unit (intermediate, authoritative early)
forms/      reconstructed .dfm resources (from the embedded DFMs)
res/        .rc and generated .res, icons, cursors
scripts/    docker wrapper, metadata extraction, comparators, normalizer
tests/      harness fixtures (for example the minimal VCL+BDE link check)
Makefile    build driver (image, analyze, disasm, sanity, compare, normalize)
build/      generated objects and linked output (gitignored)
analysis/   disassembly exports and extracted metadata (large dumps gitignored)
```

File base names must equal the original unit names recovered from the export table,
because those names drive the exported symbols in `.edata`.

## Translation-unit inventory

65 units expose module `Initialize`/`Finalize` exports; `GUI` is the main unit.
Each `Finalize` RVA is `Initialize + 0x10`. `GUI` has no module initializer export,
so its `_GUI` public symbol RVA is shown instead. Status legend: `not-started`,
`in-progress`, `functional`, `byte-exact`.

| Unit | Init RVA | Status |
| --- | --- | --- |
| GUI (main) | `_GUI@0x18bb70` | not-started |
| Mainform | `0x00007928` | not-started |
| Players | `0x00010de4` | not-started |
| Player | `0x00011f64` | not-started |
| Playerskill | `0x00011fd0` | not-started |
| Playerinventory | `0x0001203c` | not-started |
| Serial | `0x00013b4c` | in-progress |
| Settings | `0x00015d88` | not-started |
| Logins | `0x000166ec` | not-started |
| Packets | `0x00074648` | not-started |
| Mysqlcontrols | `0x00078180` | not-started |
| Itemvalue | `0x0007824c` | not-started |
| Itemvalues | `0x0007a848` | not-started |
| Npc | `0x0007ab00` | not-started |
| Mapchest | `0x0007ad1c` | not-started |
| Mapcontrol | `0x00087d38` | not-started |
| Mapobject | `0x00087dc4` | not-started |
| Mapwarp | `0x00087e64` | not-started |
| Map | `0x00088474` | not-started |
| Itemground | `0x000884d8` | not-started |
| Itemchest | `0x000a2ff8` | not-started |
| Skillvalues | `0x000a5400` | not-started |
| Npcvalues | `0x000a8e90` | not-started |
| Skillvalue | `0x000a8f64` | not-started |
| Npcvalue | `0x000a9ac8` | not-started |
| Npcdrop | `0x000a9b34` | not-started |
| Jukeboxcontrol | `0x000aa9a8` | not-started |
| Jukebox | `0x000aaa70` | not-started |
| Newscontrol | `0x000aaf5c` | not-started |
| Doorcontrol | `0x000ab108` | not-started |
| Msgboardcontrol | `0x000ae180` | not-started |
| Msgboard | `0x000ae35c` | not-started |
| Npccontrol | `0x000b11f4` | not-started |
| Gamecontrol | `0x000b15a0` | not-started |
| Shopvalues | `0x000b49b4` | not-started |
| Shopitem | `0x000b4a20` | not-started |
| Shopvalue | `0x000b546c` | not-started |
| Shopcraft | `0x000b5500` | not-started |
| Chestcontrol | `0x000b5bc4` | not-started |
| Weaponmap | `0x000b5c60` | not-started |
| Banned | `0x0012cb2c` | not-started |
| Effectcontrol | `0x0012db00` | not-started |
| Eventcontrol | `0x0012e204` | not-started |
| Wedding | `0x0012e35c` | not-started |
| Weddings | `0x001303d0` | not-started |
| Learnvalue | `0x00130e78` | not-started |
| Learnvalues | `0x0013326c` | not-started |
| Learnitem | `0x00133300` | not-started |
| Mysqlthread | `0x00133818` | not-started |
| Mysqltask | `0x001342bc` | not-started |
| Innvalues | `0x00135d94` | not-started |
| Innvalue | `0x00135ecc` | not-started |
| Classvalues | `0x00137460` | not-started |
| Classvalue | `0x00137510` | not-started |
| Playerquest | `0x001375b4` | not-started |
| Questtype | `0x001376a0` | not-started |
| Quest | `0x00137ac0` | not-started |
| Questengine | `0x0013c7fc` | not-started |
| Queststate | `0x0013cd28` | not-started |
| Filecache | `0x0013e248` | not-started |
| Killcounter | `0x0013e3a0` | not-started |
| Playercommand | `0x0013e494` | not-started |
| Killcounters | `0x0013f5e4` | not-started |
| Questcounter | `0x001406d4` | not-started |
| Questcounterlist | `0x0014082c` | not-started |
| Questcounters | `0x00141e84` | not-started |

## Phases

### Phase 0 — Toolchain and harness

Goal: a repeatable environment and measurement pipeline, plus the exact link line.

Delivered:

- `gameserver-borland-wine` built from `docker/Dockerfile`; all four tools
  (`bcc32`, `tasm32`, `ilink32`, `brcc32`) verified under Wine.
- `scripts/`: `borland.sh` container wrapper (exports `$B`/`$BZ`), `pe.py` and
  `extract_target.py` for metadata and DFM extraction, `compare_pe.py` for
  three-level comparison plus structural (imports/exports/relocations) diffs,
  `normalize_pe.py` for timestamp/header normalization, `disasm.sh` for a linear
  disassembly, `units.py` for the translation-unit table, and
  `compare_functions.py` for per-function byte scoring against the Ghidra
  inventory.
- Ghidra inventory exported read-only to `analysis/ghidra/functions.tsv`
  (7,425 functions; synthetic `0x60000000+` blocks ignored).
- `Makefile` targets `image`, `analyze`, `units`, `functions`, `struct`,
  `disasm`, `sanity`, `compare`, `normalize`, `unit`, `unit-asm`, `clean`.
- Target metadata archived under `analysis/target/` (regenerable, gitignored):
  headers, sections, imports, exports, resources, the `TGUI`, `TLoginDialog`,
  `TPasswordDialog`, and `DVCLAL` payloads, and a 452,937-line linear `.text`
  disassembly.
- Link configuration verified: `-Tpe -aa -c -Gn -j -v`, startup `c0w32.obj`,
  search paths `Lib;Lib/Obj;Lib/Release`, libraries `import32.lib cw32mt.lib
  vcl50.lib vcldb50.lib vclbde50.lib`. A minimal VCL+BDE link (`make sanity`)
  reproduces every reference header field (`0x010e`, subsystem, base,
  alignments, stack/heap) and the section order.
- Deterministic timestamp: `normalize_pe.py` rewrites `TimeDateStamp` at
  `e_lfanew + 8` (`0x208`); verified to restore the reference MD5 from a copy
  differing only in that field. Clock control (`libfaketime`) remains an
  alternative.

Outstanding, carried into Phase 1: the exact library set/order and compiler
options, fixed by matching the linked object set against the reference during
unit reconstruction.

Exit criteria: tools run reproducibly; the harness reports per-section and
whole-file diffs; a documented draft link line yields the reference header
(`0x010e`, timestamp normalized) and section geometry. Met.

### Phase 1 — C++ reconstruction and linkable skeleton

Goal: a buildable C++ project that grows unit by unit toward the reference.

- Reconstruct each unit in C++ (`src/Unit.cpp` / `Unit.h`) from the Ghidra
  decompilation and reference disassembly, resolving the exact source form per
  function by matching codegen.
- Use link-driven stub generation to keep the build linkable: link, collect the
  unresolved externals, stub them, and record what remains.
- Reconstruct forms (`forms/*.dfm`) from the `TGUI`, `TLoginDialog`, and
  `TPasswordDialog` RCDATA payloads and the resource script/`.res`.
- Track per-unit progress in the inventory below.

Exit criteria: a full C++ build links against the validated library set and runs
(functionally equivalent), with any remaining gaps explicitly stubbed.

### Phase 2 — Byte convergence, unit by unit

Goal: drive every function to the reference bytes.

- Use `scripts/compare_functions.py` to score functions, and a linker map
  (`ilink32 -s`) to attribute objects to addresses and confirm the original
  library set and order.
- Converge through source changes; where a function resists, inject TASM sourced
  from the reference disassembly.
- Confirm `.text`, `.data`, `.edata`, and `.reloc` equality.

Exit criteria: all functions byte-identical; the linked image matches the
reference apart from the timestamp.

### Phase 3 — Resources, forms, and application glue

Goal: identical resource and startup behavior.

- Reproduce `.dfm` forms exactly and regenerate the `.res` with `brcc32`.
- Reconstruct the project glue: `GUI` main unit, form hierarchy, event wiring, and
  any `.rc` inputs (icons, cursors, version/`DVCLAL`).
- Confirm `.rsrc` and `.reloc` equality.

Exit criteria: `.rsrc`, `.edata`, and `.reloc` match byte-for-byte.

### Phase 4 — Whole-image fidelity and reproducibility

Goal: the acceptance criterion, reproducibly.

- Link the final image and compare the whole-file MD5 to the reference.
- Integrate the deterministic timestamp mechanism into the build.
- Provide a single entry command and document it in [AGENTS.md](AGENTS.md).
- Run the full verification from a clean `build/` to prove reproducibility.

Exit criteria: `md5 -q build/GameServer.exe` equals
`075fb5db1d6369bf488cd2ad0c715ddd` from a clean checkout, repeatably.

## Milestones

| Milestone | Phase | Status | Evidence |
| --- | --- | --- | --- |
| Harness reports section diffs | 0 | done | `compare_pe.py` on self/perturbed images |
| Reference header reproduced | 0 | done | `make sanity`: `0x010e`, section geometry |
| Deterministic timestamp | 0 | done | `normalize_pe.py` restores reference MD5 |
| Unit table recovered | 0 | done | `scripts/units.py` -> `units.tsv` (65 + GUI) |
| Per-function diff tooling | 1 | done | `compare_functions.py` self-test 7384/7384 |
| Serial skeleton compiles | 1 | done | `make unit UNIT=Serial` |
| Serial accessors byte-exact | 1 | done | 6 accessors + `ReloadIni` + `GetDisplayCode` (54/54) match |
| Serial logic reconstructed | 1 | done | `DecodeString` + `Validate` + `GetDisplayCode` + `ReloadIni` and 6 accessors byte-exact (10/13) |
| Serial remaining | 1 | partial | `SetIniPath` 155, `ReadKey` 254, ctor 246 — EH scope-structure only |
| Full skeleton links | 1 | done | `make build`: 133 exports, working PE |
| Full skeleton runs functionally | 1 | todo | launch and exercise |
| All units C++-verified | 2 | todo | `make functions` at 100% |
| `.rsrc`/`.reloc` match | 3 | todo | section diff |
| Full MD5 match, reproducible | 4 | todo | `md5 -q` from clean tree |

## Open questions

These require evidence and must not be answered by guessing:

- Source form per function: instance method vs free function with explicit object
  pointer (the reference `SetCounter` matches the latter). Must be resolved by
  matching codegen before byte convergence.
- Exact compiler options (optimization level, RTTI/exception settings) used for the
  original build; current evidence favours defaults, but `SetCounter` shows source
  form, not flags, is the first-order difference.
- Exact library set and link order (VCL, BDE, OLE, sockets) that reproduces the
  reference's statically included objects, and whether the reference's `.tds`
  debug side-effect of `-v` was discarded.
- Whether the `GUI` unit itself defines `TGUI` or whether the form lives in
  `Mainform`; the DFM is named `TGUI` and both units exist.
- Whether units are contiguous or library objects interleave between them; resolve
  with the `ilink32 -s` detailed map.

## Risks and mitigations

| Risk | Mitigation |
| --- | --- |
| VCL/BDE codegen cannot be reproduced | Use unmodified `ref/Borland5` libs and headers; verify per-section, not per-function, for library regions |
| Linker nondeterminism | Fix timestamp and compare sections; treat whole-file MD5 as the final gate |
| Resource/DFM mismatch | Reconstruct from the embedded payloads, byte-diff `.rsrc` |
| Scale (65 units, 1.7 MB image) | Unit inventory + per-unit status; smallest-falsifiable-test discipline |
| EH pervasive in TASM | C++-first; use TASM only for EH-free functions or last-resort fidelity |
| Source-form ambiguity | Match codegen per function; record the form in the header |
