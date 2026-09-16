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

## Strategy: TASM-first

The reconstruction is authored as assembly before C++:

1. Each application translation unit is first expressed as Turbo Assembler source
   that produces **byte-identical object content** to the reference's slice of the
   image (code, data, relocations, symbol names).
2. That assembly is the reference against which the C++ version is judged. The C++
   form is derived from the proven assembly and must compile, with `bcc32`, to the
   same object bytes.
3. Library code (RTL, VCL, BDE) is not reimplemented. It is supplied by the
   unmodified `.lib`/`.obj` files in `ref/Borland5/Lib`. The whole-image MD5 is the
   final arbiter.

This order front-loads the hard, unambiguous work (exact bytes) and makes the
high-level reconstruction measurable at every step.

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
`asm-verified`, `cpp-verified`.

| Unit | Init RVA | Status |
| --- | --- | --- |
| GUI (main) | `_GUI@0x18bb70` | not-started |
| Mainform | `0x00007928` | not-started |
| Players | `0x00010de4` | not-started |
| Player | `0x00011f64` | not-started |
| Playerskill | `0x00011fd0` | not-started |
| Playerinventory | `0x0001203c` | not-started |
| Serial | `0x00013b4c` | not-started |
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
  three-level comparison, `normalize_pe.py` for timestamp/header normalization,
  and `disasm.sh` for a linear disassembly.
- `Makefile` targets `image`, `analyze`, `disasm`, `sanity`, `compare`,
  `normalize`, `clean`.
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

### Phase 1 — Disassembly and TASM scaffold

Goal: reassemble every application unit to its reference object content.

- Recover function boundaries, call targets, and data references; map code/data
  extents to units using export RVAs and the linker's object ordering.
- Emit TASM source per unit, preserving original mangled symbol names and segment
  placement.
- Reconstruct read-only data: string literals, virtual tables, RTTI, form class
  tables, and static initializer/finalizer tables.
- Reconstruct forms (`forms/*.dfm`) from the `TGUI`, `TLoginDialog`, and
  `TPasswordDialog` RCDATA payloads, and the resource script/`res`.
- Assemble each unit and compare object bytes to the reference slice; refine until
  they match.
- Link the full TASM scaffold and drive `.text`, `.data`, `.edata`, and `.rsrc`
  toward byte-equality.

Exit criteria: the scaffold links to a binary whose sections match the reference
byte-for-byte (timestamp aside), or all remaining differences are isolated and
documented as library-provided.

### Phase 2 — C++ lifting, unit by unit

Goal: replace each TASM unit with C++ that compiles to the same bytes.

- Derive headers: class layouts, member offsets, virtual table order, and
  `__published` declarations, from the proven assembly.
- Reconstruct `Unit.cpp`; compile with `bcc32` and compare object bytes to the
  assembly baseline.
- Flip units from `asm/` to `src/` one at a time, keeping the whole-image diff
  clean after each flip.
- Determine compiler options (optimization, register conventions, RTTI/exceptions)
  by matching code generation.

Exit criteria: every unit is `cpp-verified`; the linked image still matches.

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
| First unit assembles identically | 1 | todo | object byte diff |
| All units assemble identically | 1 | todo | aggregate diff |
| Scaffold links to reference sections | 1 | todo | section diff |
| First unit compiles identically | 2 | todo | object byte diff |
| All units C++-verified | 2 | todo | aggregate diff |
| `.rsrc`/`.reloc` match | 3 | todo | section diff |
| Full MD5 match, reproducible | 4 | todo | `md5 -q` from clean tree |

## Open questions

These require evidence and must not be answered by guessing:

- Exact compiler options (optimization level, `-O`/`-Od`, RTTI, exception handling)
  used for the original build.
- Exact library set and link order (VCL, BDE, OLE, sockets) that reproduces the
  reference's statically included objects, and whether the reference's `.tds`
  debug side-effect of `-v` was discarded.
- Whether the `GUI` unit itself defines `TGUI` or whether the form lives in
  `Mainform`; the DFM is named `TGUI` and both units exist.
- Whether the reference link used `-v` with debug-stripped objects, or a separate
  strip step (the observed header `0x010e` matches `ilink32 -v` with no debug
  directory).

## Risks and mitigations

| Risk | Mitigation |
| --- | --- |
| VCL/BDE codegen cannot be reproduced | Use unmodified `ref/Borland5` libs and headers; verify per-section, not per-function, for library regions |
| Linker nondeterminism | Fix timestamp and compare sections; treat whole-file MD5 as the final gate |
| Resource/DFM mismatch | Reconstruct from the embedded payloads, byte-diff `.rsrc` |
| Scale (65 units, 1.7 MB image) | Unit inventory + per-unit status; smallest-falsifiable-test discipline |
| Hidden data dependencies | TASM-first scaffold forces every relocated reference to be accounted for |
