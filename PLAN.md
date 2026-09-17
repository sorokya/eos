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
- **RTTI type-name table (the class-name authority).** The image carries Borland's
  RTTI type-name strings, e.g. `std::vector<NpcValue,std::allocator<NpcValue> >`
  and `NpcValues *`. These are observable and must match. Recipe: scan the raw file
  for `std::vector<` to enumerate every element class (39 distinct), then confirm
  each declared class name appears as a NUL-delimited string. All `*values` names
  are confirmed — `ClassValue(s)`, `ItemValue(s)`, `SkillValue(s)`, `NpcValue(s)`,
  `ShopValue(s)`, `ShopItemVal`, `ShopCraftVal`, `LearnValue(s)`, `LearnItemVal`,
  `InnValue(s)` — plus `TGUI`, `PlayerSkill`, `PlayerInventory`. Names absent from
  the table (`GroundItem`, `ItemElement`, `ItemSpecXY`, `NpcDropInfo`,
  `NpcTypeInfo`, `ShopCraftIngredient`, `SkillDamage`, `SkillElement`,
  `WeaponmapEntry`) are POD/return helper structs with no RTTI, hence unobservable
  and free to name. Two spelling facts: the reference spells the jukebox element
  `JukeBox` (capital B), and `Serial`/`Weaponmap` are units, not RTTI classes.
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
- **Unit boundaries (resolved).** `MAP=1 scripts/build.sh` runs `ilink32 -s` and
  writes `build/GameServer.map`, whose "Detailed map of segments" section names
  the object or library member behind every range
  (`scripts/unitmap.py --map`). This established the layout rule: explicitly
  listed objects are laid out contiguously in command-line order (startup
  `c0w32.obj`, then the units in link order) and **library members are appended
  after them**. Units are therefore contiguous and a unit's `code_span` is its
  real code size, confirmed on `Serial`: our linked `SERIAL.OBJ` is `0x1A94`
  bytes against a reference span of `0x1AF0`. **Caveat:** every linked module
  emits a refcount-guarded initializer stub, but `units.py` sees only modules
  exported as `@@Unit@Initialize`; units compiled without
  `#pragma package(smart_init)` are invisible to it. Scanning the stubs
  (`scripts/unitmap.py --pe … --stubs`) finds **8 unexported modules, all inside
  `Banned`'s span**. Classifying them by byte-matching their functions against the
  Borland libraries — with the reference's relocation slots masked, since library
  code is relocation-heavy — marks **all 8 as `library`**. `Banned`'s real code is
  therefore only its last ~64 KB (508 functions), not 487 KB. The two smallest
  were initially misread as `unknown`; decompilation identifies them as the VCL
  units **MultiMon** (delay-loaded `GetMonitorInfo`/`MonitorFromWindow`, a
  USER32.DLL thunk) and **FlatSB** (comctl32 `FlatSB_*` thunks), both present as
  `multimon.pas`/`flatsb.pas` in `ref/Borland5/Source/Vcl/`: they are library
  members, not application units. No genuine unexported source unit has been
  found, and the other 64 unit spans are exact.
- **Library linkage (Debug confirmed, order open).** Byte-signature matching of
  the reference's library-region code against the Borland libs gives **0
  Release-only matches vs 65+ Debug-only** (500 code samples), and the reference
  carries **22 Debug-only strings vs 0 Release-only**; every one of the six
  library modules matches `Lib/Debug` far better than `Lib/Release` (e.g. 27 vs 3
  for one module). The original link therefore used the **Debug** VCL/BDE
  libraries (`vcl50`, `vcldb50`, `vclbde50`), consistent with the debug compile
  flags. `scripts/build.sh` and the `Makefile` now put `Lib/Debug` before
  `Lib/Release` and add `cp32mt.lib`; `make sanity` and `make build` pass.
  Remaining for Phase 2: the exact library *order* and reproducing the two-group
  placement, since the refcount counters split into two arrays (units 1–39 vs
  40–65) with a 300 KB library block between the groups.
- **Library placement (mechanism resolved).** `ilink32` places a library member at
  the position where the library is listed among the object arguments, not always
  at the end: inserting `cw32mt.lib` after the 39th object in a scratch link moved
  its members from the tail to immediately after unit 39 (verified with
  `scripts/unitmap.py --map`). The reference's layout — a ~300 KB VCL/BDE block
  between units 39 and 40, then units 40–65, then a ~98 KB RTL/VCL tail —
  therefore records where the original link listed each library: VCL/BDE after
  unit 39, RTL after unit 65. Reproducing it means listing libraries among the
  objects at those positions rather than only in the `libfiles` slot.
- **Components used (from the DFM resources).** The three forms reference
  `TServerSocket` (`scktcomp`), `TDatabase`/`TQuery`/`TSession` (BDE/DB) and
  `TApplicationEvents`, plus `StdCtrls`/`ExtCtrls` controls — consistent with the
  `vcl50` + `vcldb50` + `vclbde50` set. The two `unknown` modules match no
  Borland lib, so they are genuine application code, not library members.
- **Function linking (resolved).** bcc32 emits every function as a COMDAT
  (`virtual(_TEXT)` under `tdump -o`), so `ilink32` discards unit functions that
  nothing references. The skeleton therefore contains only the Initialize/Finalize
  stubs, and per-function scoring must use the compiler's `-S` output
  (`compare_asm.py`) rather than the linked image until the reference call graph
  is reproduced.
- **`Mainform`/`TGUI` form located, method table decoded (corrected).** The main
  form class is `TGUI` (from the embedded DFM and the class RTTI), but it belongs
  to the **`Mainform`** unit, not `GUI`: the class RTTI type data
  (`VA 0x4042b4`) names the unit `MainForm`, the VMT is at `VA 0x55c380`
  (instance size `0x378` = 888), and the unit exports `@@Mainform@Initialize`
  (`RVA 0x7928`). `GUI.cpp` is the project main unit (WinMain + the `PACKAGE`
  `GUI` global); the form's methods live in `Mainform.cpp`.
  The class's published tables sit in `.data`: the field table at
  `VA 0x55c460` (format `{dword offset; word; byte namelen; name}`, 10 fields)
  and the method table at `VA 0x55c513`. The method table format is
  `{word count; {word size; dword code; byte namelen; name}[count]}` (per
  `TObject::MethodAddress` in `ref/Borland5/Source/Vcl/system.pas`). Parsing it
  yields **eight** published methods; the task's earlier naive
  `{byte len; name; word; dword}` parse associated each name with the *previous*
  entry's address, so the addresses were shifted by one. The correct mapping
  (and the one that matters, confirmed against each handler's VCL signature) is:

  | handler | address | mapped callee |
  | --- | --- | --- |
  | `FormCreate` | `0x4015c8` | boot sequence (`Mainform_Init`) |
  | `serverClientError` | `0x4028e8` | `Players_MarkRemoving` |
  | `serverClientConnect` | `0x4027c4` | max-conns + remote IP + `Logins`/`Players` |
  | `serverClientDisconnect` | `0x402938` | `Server_RemovePlayer` + `Players_Remove` |
  | `serverClientRead` | `0x402988` | `Server_ClientRead` |
  | `timerTimer` | `0x402a20` | `Mainform_Tick` (master timer) |
  | `FormClose` | `0x4036c8` | `Server_Shutdown`; `Action = caNone` |
  | `ApplicationEvents1Exception` | `0x4036f0` | appends to `.\logs\error.log` |

  Field offsets are pinned by the field table and the handlers: published
  components `server`(TServerSocket) `0x2d0`, `mysql` `0x2d4`, `myquery` `0x2d8`,
  `mysession` `0x2dc`, `timer` `0x2e0`, `panel_connections` `0x2e4`,
  `panel_send` `0x2e8`, `panel_received` `0x2ec`, `panel_buffer` `0x2f0`,
  `ApplicationEvents1` `0x2f4`; private controller pointers begin at
  `settings` `0x2f8` (see `src/Mainform.h`). The `__published` order is
  observable: it drives the compiler-generated field table bytes.
  Source-form notes: the socket event methods take the `TCustomWinSocket*` in
  `ecx`; `serverClientError`'s `ErrorCode` reference is the **last** parameter
  (VCL order), because bcc32 pushes the stack arguments left-to-right, placing
  the final parameter at `[ebp+8]`. Appending to an `AnsiString` in
  `ApplicationEvents1Exception` is written as explicit
  `s.Insert(x, s.Length()+1)` (bcc32 lowers `s += x` to `$brplu`, which is not
  the reference's sequence). A `FILE *fp;` **declaration separate from its
  assignment** avoids an extra EH scope marker at the `fopen` site.
  `FormCreate` assigns the controllers directly (`field = new T(...)`), not via
  named locals (a local adds a temporary and an EH scope marker); the
  `server->Active = true` activation is wrapped in a `try`/`catch (...)` that
  calls `Application->Terminate()`. `timerTimer` is `void` (the event
  signature), not the `int` Ghidra inferred, and its GUI block passes the
  `FUN_004762c8` string arguments inline (right-to-left evaluation orders the
  pushes). Controller/subsystem classes with reconstructed headers
  (`ItemValues`, `NpcValues`, `SkillValues`, `LearnValues`, `ShopValues`,
  `InnValues`, `ClassValues`, `Jukeboxcontrol`, `Serial`) are included from
  their real units; the rest are size-and-ctor stubs in `Mainform.cpp`.
  **All eight handlers are byte-exact** (see milestones).

- **`ClassValues::LoadClasses` (byte-exact).** The ~600-instruction ECF table
  parser (`0x536100`), now byte-identical (596/596). Root causes found by
  deriving the scope structure upward from the reference's EH cleanup-record
  table (`__InitExceptBlockLDTC` table at `0x581360`, 12-byte entries `outer`/
  `kind`/`dtcMin`/`dtt`): (1) the file-read/parse region is wrapped in a
  `try { … } catch (…) { FileClose(h); field_10 = 0; }` with `h`, `size`, `buf`
  declared *outside* the `try` (the catch closes the handle, so `h` must be in
  scope); (2) the record-loop fields are **inline `DecodeInt(...)` arguments** to
  `AddClass` (one scope), not separate `short` locals (many scopes); (3) the
  header's `total`/`version` live in a nested block with `total` carried by a
  `int parsed = DecodeInt(…); total = parsed; num_classes = parsed;` temp (the
  loop-carried `total` is what opens the extra "empty" cleanup scope 128); and
  (4) `int version = DecodeInt(SubString(10,1))` is a dead store (`W8004`,
  matches the reference's unused `[ebp-0xbc]`). Two other fixes were required:
  `field_0 = file - 1` (not `file`), and a one-call `int size()` member returning
  `values.size()` (the reference routes `values.size()` through a dedicated
  accessor `0x53731c`, not an inline `add eax,0x1c`). The whole unit is now
  byte-exact: ctor 43, dtor 26, LoadClasses 596, AddClass 66, GetByIndex 75,
  `size` 9, DecodeInt 85 (all 0 mismatches).

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
| Static libraries | `import32.lib cw32mt.lib cp32mt.lib vcl50.lib vcldb50.lib vclbde50.lib` (from `Lib/Debug`) | minimal VCL+BDE link resolves; Debug variant byte-matched; order finalized in Phase 2 |
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
  vcldb50.lib vclbde50.lib` (later corrected to the Debug set — see the
  library-linkage finding) and the three `Lib` search paths, reproducing every
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
| Mainform | `0x00007928` | in-progress |
| Players | `0x00010de4` | not-started |
| Player | `0x00011f64` | not-started |
| Playerskill | `0x00011fd0` | byte-exact |
| Playerinventory | `0x0001203c` | byte-exact |
| Serial | `0x00013b4c` | byte-exact |
| Settings | `0x00015d88` | not-started |
| Logins | `0x000166ec` | not-started |
| Packets | `0x00074648` | not-started |
| Mysqlcontrols | `0x00078180` | not-started |
| Itemvalue | `0x0007824c` | byte-exact |
| Itemvalues | `0x0007a848` | byte-exact |
| Npc | `0x0007ab00` | not-started |
| Mapchest | `0x0007ad1c` | not-started |
| Mapcontrol | `0x00087d38` | not-started |
| Mapobject | `0x00087dc4` | not-started |
| Mapwarp | `0x00087e64` | not-started |
| Map | `0x00088474` | not-started |
| Itemground | `0x000884d8` | byte-exact |
| Itemchest | `0x000a2ff8` | not-started |
| Skillvalues | `0x000a5400` | byte-exact |
| Npcvalues | `0x000a8e90` | byte-exact |
| Skillvalue | `0x000a8f64` | byte-exact |
| Npcvalue | `0x000a9ac8` | byte-exact |
| Npcdrop | `0x000a9b34` | byte-exact |
| Jukeboxcontrol | `0x000aa9a8` | in-progress |
| Jukebox | `0x000aaa70` | byte-exact |
| Newscontrol | `0x000aaf5c` | not-started |
| Doorcontrol | `0x000ab108` | not-started |
| Msgboardcontrol | `0x000ae180` | not-started |
| Msgboard | `0x000ae35c` | not-started |
| Npccontrol | `0x000b11f4` | not-started |
| Gamecontrol | `0x000b15a0` | not-started |
| Shopvalues | `0x000b49b4` | byte-exact |
| Shopitem | `0x000b4a20` | byte-exact |
| Shopvalue | `0x000b546c` | byte-exact |
| Shopcraft | `0x000b5500` | byte-exact |
| Chestcontrol | `0x000b5bc4` | not-started |
| Weaponmap | `0x000b5c60` | byte-exact |
| Banned | `0x0012cb2c` | not-started |
| Effectcontrol | `0x0012db00` | not-started |
| Eventcontrol | `0x0012e204` | not-started |
| Wedding | `0x0012e35c` | not-started |
| Weddings | `0x001303d0` | not-started |
| Learnvalue | `0x00130e78` | byte-exact |
| Learnvalues | `0x0013326c` | byte-exact |
| Learnitem | `0x00133300` | byte-exact |
| Mysqlthread | `0x00133818` | not-started |
| Mysqltask | `0x001342bc` | not-started |
| Innvalues | `0x00135d94` | byte-exact |
| Innvalue | `0x00135ecc` | byte-exact |
| ClassValues | `0x00137460` | byte-exact |
| Classvalue | `0x00137510` | byte-exact |
| Playerquest | `0x001375b4` | not-started |
| Questtype | `0x001376a0` | not-started |
| Quest | `0x00137ac0` | not-started |
| Questengine | `0x0013c7fc` | not-started |
| Queststate | `0x0013cd28` | not-started |
| Filecache | `0x0013e248` | not-started |
| Killcounter | `0x0013e3a0` | byte-exact |
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
  search paths `Lib;Lib/Obj;Lib/Debug;Lib/Release`, libraries `import32.lib
  cw32mt.lib cp32mt.lib vcl50.lib vcldb50.lib vclbde50.lib` (the Debug VCL/BDE
  set was established later — see the library-linkage finding above). A minimal
  VCL+BDE link (`make sanity`) reproduces every reference header field (`0x010e`,
  subsystem, base, alignments, stack/heap) and the section order.
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
| Serial byte-exact | 1 | done | all 13 functions match (ctor 271, SetIniPath 177, ReadKey 291, Validate 467, DecodeString 290, ...) |
| Serial logic reconstructed | 1 | done | `DecodeString` + `Validate` + `GetDisplayCode` + `ReloadIni` and 6 accessors byte-exact (10/13) |
| Serial remaining | 1 | partial | `SetIniPath` and `ReadKey` byte-exact (try/catch fingerprint); ctor remains |
| Weaponmap byte-exact | 1 | done | all 3 functions match (ctor 12, deleting-dtor 11, `Combat_IsRangedWeapon` 21) |
| Shopitem byte-exact | 1 | done | both functions match (ctor 15, deleting-dtor 11) |
| Shopcraft byte-exact | 1 | done | ctor 28, deleting-dtor 11 match; `ShopCraftVal` = crafted `id` + 4 ingredient id/amount `int[4]` arrays (layout confirmed by the ctor stores; field names from the `ShopCraftRecord` spec) |
| Skillvalue byte-exact | 1 | done | ctor 23, deleting-dtor 29 match; `SkillValue` = `id` at 0, `String name`/`chant` at 4/8 (leading `EsfRecord` fields). Remaining `EsfRecord` fields pending the `Skillvalues` parser |
| Itemvalue byte-exact | 1 | done | ctor 15, deleting-dtor 11 match; `ItemValue` = `id` at 0 plus the widened `EifRecord` fields in memory (`sizeof` 0x58, see `src/Itemvalue.h`), offsets pinned by the `Itemvalues` parser stores and `Eif_Get*` accessors. The earlier "self `ItemValue *` at 8" reading was wrong: offset 8 is `special` |
| eo-protocol reference | 0 | done | `ref/eo-protocol` submodule added; `xml/pub/server/protocol.xml` gives the pub record layouts/field names for the `*values` parsers |
| ClassValues byte-exact (6/6 fns) | 1 | done | ctor 43, dtor 26, GetByIndex 75, AddClass 66, DecodeInt 85 all match; `LoadClasses` 596/596 and `size` 9/9 now match too |
| LoadClasses (state) | 1 | done | 546 -> 300 -> 0 mismatches. Root cause was the EH scope structure: a `try`/`catch` around the file-read with `h`/`size`/`buf` outside the `try`, inline `DecodeInt` fields in `AddClass`, a loop-carried `total` temp (nested block), a dead `int version` store, `field_0 = file - 1`, and a one-call `size()` accessor |
| Itemvalues byte-exact (21/21 fns) | 1 | done | ctor 47, `LoadItems` 1243 (includes the `try`/`catch` file read, `field_14->Clear()` + `Clear(self)` reset, `namelen+off` inline `DecodeNumber` record args), `AddItem` 167, `Clear` 40, `GetRecordSlot`/`GetByIndex`/`GetCount`, 11 `Eif_Get*` accessors, `DecodeNumber` 85 — all 0 mismatches. `values` is `std::vector<ItemValue *>`; the two result-pair structs need an empty user ctor plus a single anonymous aggregate member to reproduce the folded ctor (0x44f58c) and the memcpy-style return |
| Skillvalues byte-exact (13/13 fns) | 1 | done | ctor 43, deleting-dtor 26, `LoadSpells` 1053 (ESF parser, `try`/`catch` file read), `AddRecord` 121, `GetDamage`/`GetElement` 48 (pair structs need an empty user ctor + single anonymous aggregate member), `GetTargetType`/`GetSkillType`/`GetTpCost`/`GetHpHeal` 30, `GetCastTime` 32, `GetCount` 9, `DecodeNumber` 85 - all 0 mismatches. `values` is `std::vector<SkillValue>` |
| Shopvalues byte-exact (15/15 fns) | 1 | done | ctor 30, `LoadShops` 605 (ESF `./pub/dts001.esf`, `try`/`catch` file read, inline `DecodeNumber` record args), `Clear` 43, `GetCraftIngredient1..4` 81 each, `GetBuyPrice`/`GetSellPrice` 75 each, `BuildOpenData` 500, `GetRecordCount` 9, `EncodeNumber` 134, `DecodeNumber` 85, `AddTrade` 11, `AddCraft` 16 - all 0 mismatches. Added `ShopValue::AddTrade`/`AddCraft` (Shopvalue unit, 43/53, 0 mismatches). `ShopItemVal` extended to the 16-byte trade record (`item_id`/`buy_price`/`sell_price`/`max_amount`) without changing its ctor/dtor (15/11); `ShopValue` header fields pinned to `short min_level`(8)/`max_level`(0xa)/`class_requirement`(0xc) |
| Innvalues byte-exact (15/15 fns) | 1 | done | ctor 30, deleting-dtor 26, `LoadInns` 533 (`./pub/din001.eid`; single-file EID parser: behavior_id[short], namelen[char], name, spawn/sleep/alt fields, then 3 `InnQuestionRecord` question/answer pairs; `do { ... } while (data.Length() > 0xf)`, `try`/`catch` file read), `GetName` 51, `GetSleepMap` 21 / `GetSleepX` 11 / `GetSleepY` 11, `GetSpawnMap` 48 / `GetSpawnX` 40 / `GetSpawnY` 40 (alternate-spawn threshold logic), `GetQuestion` 98 (separator is the implicit `char`->`String` conversion `Insert((char)0xff, ...)`, not `String((char)0xff)`), `GetAnswer` 54, `Clear` 16, `GetCount` 9 (`unsigned`), `DecodeNumber` 83 - all 0 mismatches. `record_list` is `std::vector<InnValue>` (`loaded` at 0, `field_24` at 0x24); all `vector<InnValue>` helpers (`$bctr`, `__init`, `insert`/`__insert_aux`, `__construct`, `copy_backward`, `copy`, `allocate`, `uninitialized_copy`, `clear`/`erase`, `__rw_basis`) also match |
| Npcvalues byte-exact (15/15 fns) | 1 | done | ctor 53, deleting-dtor 26, `Pub_LoadNpcs` 806 (ENF `./pub/dtnNNN.enf`), `Pub_LoadDrops` 273 (EDF `./pub/dtd001.edf`), `Pub_LoadTalk` 258 (ETF `./pub/ttd001.etf`; all three `try`/`catch` file reads), `AddDrop` 35, `SetTalk` 45, `RollTalk` 99 (`String` by value, `int roll` local re-used, `RandRange(100) < talk_rate`), `GetDrop` 92 (`{item_id,amount}` pair, `sum >= roll`, `range > 0`), `AddNpc` 95, `GetNpc` 229 (by-value `NpcValue`, cached `NpcValue *value`), `GetExp` 33, `GetMaxHp` 32, `GetType` 44, `GetCount` 9, `DecodeNumber` 85 - all 0 mismatches. Member order `field_0,count,rid1,rid2` (parsed at +8/+0xc/+4); `record_list` is `std::vector<NpcValue>`; all `vector<NpcValue>`/`vector<NpcDropItem>`/`vector<AnsiString>` helpers match |
| Npcvalues/Npcvalue independently re-verified | 1 | done | Every source function matched to its reference span at 0 mismatches, including the ones whose earlier ranges were wrong: `NpcValue(int)` 56 (`0x4a917c`, not `0x4a9318`), `~NpcValue` 38 (`0x4a921c`), `AddDrop` 43 (`0x4a9298`), `GetNpc` 229 (`0x4a8858`), `DecodeNumber` 85 (`0x4a8d18`), and the `Npcdrop` ctor/dtor 15/11 after the `NpcDropItem` widening |
| Whole-tree byte-fidelity sweep | 1 | done | New `scripts/verify_units.py` (+ `scripts/build_asm.sh`, `make verify`) scores every function in each unit's namespace against that unit's reference ranges. Across the 23 byte-exact units all 138 source functions match: 135 exactly, plus 3 unreferenced `$bdtr` COMDATs (`Serial`, `ShopValues`, `LearnValues`) that the linker drops because nothing `delete`s those classes. Re-run after the clang-format pass confirms it is codegen-neutral |
| compare_asm overload selection + crash fix | 1 | done | `parse_our` now tries an exact full-name match before the `$q`-suffix form, so a suffix-qualified name such as `$bctr$qi` can be selected when it shares a prefix with `$bctr$qv`; a not-found function returns the 3-tuple `main` expects instead of raising `ValueError` |
| Npcvalue byte-exact (5/5 fns) | 1 | done | `NpcValue()` 53, `NpcValue(int)` 56, `~NpcValue()` 38, `AddDrop` 43, `AddTalkMessage` 38 - all 0 mismatches. Element `NpcValue` is 0x7c bytes (id/name/graphic_id/race/boss/child/npc_type/behavior_id/hp/tp/min_damage/max_damage/accuracy/evade/armor/return_damage/element/element_damage/element_weakness/element_weakness_damage/level/experience/talk_rate/has_talk/drops/talk_lines); `NpcDropItem` widened to 16 bytes (`item_id`/`min_amount`/`max_amount`/`rate`, no explicit padding - the compiler's implicit alignment is not copied by the copy ctor); `talk_lines` is `std::vector<AnsiString>`. All element vector helpers match |
| Killcounters layout solved (fixed array) | 2 | progress | `Killcounters` is `TStringList *field_0`(+0) / `String name`(+4) / `std::vector<KillCounter> buckets[27]`(+8) - a *fixed inline array*, not a DynamicArray or pointer. The ctor passes `&buckets` as a pre-allocated destination to the 7-argument `_vector_new_ldtc_(dest, 0x20, 27, flags, elem_ctor, flags, typedesc)`, which is what identified it (DynamicArray's AllocData uses malloc, a raw new[] uses operator new[], neither matches). `~Killcounters()` is now byte-exact; `Killcounters()` is 9 mismatched, missing only the ctor-tail `Init(this)` call (0x53f07c) |
| Killcounter byte-exact (3/3 fns) | 1 | done | `KillCounter(String)` 30, `KillCounter(String, int)` 31, `~KillCounter()` 23 - all 0 mismatches. `KillCounter` = `String name`(+0) / `int count`(+4); the one-argument ctor sets `count = 1`, the two-argument one stores its count. RTTI type name `std::vector<KillCounter, ...>`. Unlike Jukeboxcontrol, this class *does* have an explicit destructor in the reference |
| Jukeboxcontrol complete - 16 functions belong to Mapcontrol | 2 | done | The 16 reference functions still uncovered in the Jukeboxcontrol span are `std::vector<JukeBox>` template instantiations (element construct/copy `0x4aa20c`/`0x4aa290`, stride-0x18 destroy `0x4aa2c8`, `InsertSlow` `0x4a9f94`, `push_back` `0x4a9e74`, enlarge `0x4a9ee4`, vector deleting destructor `0x4a9e14`). They are NOT Jukeboxcontrol source: their only callers are in `Mapcontrol` (`FUN_00482834` at 0x483e2e and `Mapcontrol_LoadMap` at 0x486489), which pushes jukebox entities into the shared Jukeboxcontrol vector while parsing the EMF map. The linker places the COMDATs in the Jukeboxcontrol span because it is the first `std::vector<JukeBox>` user. Jukeboxcontrol's hand-written surface is therefore complete: ctor, BuildRecentTracksString, EncodeNumber, TryPlayTrack (no explicit destructor - the reference has none) |
| Jukeboxcontrol TryPlayTrack byte-exact | 2 | done | 89/89, 0 mismatches (`0x4aa71c`). Walks `recent_plays`; on a matching `id` it expires a stale entry (`Now()` vs `TDateTime` via two `DateTimeToTimeStamp` calls, `secs > 90` clears `playing`), then when `playing == 0` sets `played = true`, `playing = 1`, stamps `timer = Now()` and stores the track name into `+0x10`, and breaks. The `String track` parameter is used directly - an explicit `String t = track;` local adds a copy-ctor and a bigger frame. The reference's `lea edx,[ebp+16]; lea eax,[ebp+16]` copy-ctor pair is the by-value String parameter's own construction, not a local |
| Jukeboxcontrol EncodeNumber byte-exact | 2 | done | 100/100, 0 mismatches (`0x4aa848`). Base-253 encoder that writes `rem + 1` digits with a `0xfe` pad into `self->field_0` (the 8-byte ctor buffer) and returns `String(field_0, width)` via AnsiString_SetString. Source-form facts, each found by diffing: `value` is `unsigned int` (the code uses `div`/`fild qword`, not `idiv`), `quotient` is `unsigned int` (uses `jnb`), the quotient is stored through a named `double` local, `rem`/`c` are named locals, the pad is a named `char`, and `rem`/`c` are declared *inside* the try before `quotient` while the flag follows it - that exact declaration placement is what reproduces the EH marker stream and the -80 frame |
| Jukeboxcontrol BuildRecentTracksString byte-exact | 2 | done | 113/113, 0 mismatches (`0x4aa59c`). Walks `recent_plays` by `0x18`, skips entries whose `id` != arg, appends `EncodeNumber(self, id, 2)`, and when `playing` is set computes `secs` from `Now()` vs the entry's `TDateTime` via two `DateTimeToTimeStamp` calls and keeps the name only while `secs < 90`; otherwise clears `playing`. The two subtractions must be separate `int` locals (`days`, `ms`) before the `/1000 + *86400` sum or the frame is 8 bytes short. `EncodeNumber` is still a stub - its call-site shape is right, its body is not yet reconstructed |
| Jukeboxcontrol ctor byte-exact + disassembly partition | 2 | progress | `Jukeboxcontrol()` matches 0 mismatches; source is just `{ field_0 = operator new(8); recent_plays.clear(); }`. This resolved the InitStub puzzle: `InitStub`, `InitRecentPlays`, `ClearRecentPlays`, `InitBox`, `ComputeInitialCapacity`, `GrowCapacity` are the `std::vector<JukeBox>` member's own construction internals (library COMDATs the compiler emits for free), NOT source functions. Of the unit's 33 reference functions, 14 are such COMDATs already covered by the ctor alone; 19 real source functions remain (the `0x4a9e14`-`0x4aa534` group, `BuildRecentTracksString` 382, `TryPlayTrack` 299, and its own base-253 `EncodeNumber` 323) |
| Jukebox byte-exact (2/2 fns) | 1 | done | `JukeBox(short)` and `~JukeBox()` both 0 mismatches. `JukeBox` (RTTI name) = `short id`(+0), `char playing`(+2), `TDateTime timer`(+8), `AnsiString name`(+0x10); the ctor zero-inits the `TDateTime` via its out-of-line `__fastcall` default ctor (called with `this` in EAX - an inline `double` store or a `__cdecl` wrapper both fail) and the dtor destroys only the `AnsiString`, confirming the `TDateTime` is the unmanaged 8-byte member |
| Mainform/TGUI form located; method table corrected | 2 | done | Class RTTI (`VA 0x4042b4`) names unit `MainForm`; VMT `VA 0x55c380`, size `0x378`. Field table (`VA 0x55c460`) and method table (`VA 0x55c513`, format per `TObject::MethodAddress`) decoded; corrected the shifted handler addresses (`FormCreate` `0x4015c8`, `serverClientConnect` `0x4027c4`, `serverClientError` `0x4028e8`, `serverClientDisconnect` `0x402938`, `serverClientRead` `0x402988`, `timerTimer` `0x402a20`, `FormClose` `0x4036c8`, `ApplicationEvents1Exception` `0x4036f0`). `src/Mainform.h`/`.cpp` carry the `TGUI` class and handlers |
| Mainform handlers byte-exact (8/8) | 2 | done | `FormCreate` 885 (`0x4015c8`, boot sequence), `timerTimer` 424 (`0x402a20`, master tick), `FormClose` 15, `serverClientError` 26, `serverClientConnect` 83, `serverClientDisconnect` 25, `serverClientRead` 43, `ApplicationEvents1Exception` 202 - all 0 mismatches. Class layout (published fields + private controllers) pinned by the reference field table; `Mainform` unit now defines the whole `TGUI` surface |
| *values family byte-exact (13 units) | 2 | done | `Classvalues`/`Classvalue`, `Itemvalues`/`Itemvalue`, `Skillvalues`/`Skillvalue`, `Npcvalues`/`Npcvalue`, `Shopvalues`/`Shopvalue`/`Shopcraft`, `Learnvalues`/`Learnvalue`/`Learnitem`, `Innvalues`/`Innvalue` — every source function scores 0 mismatches (parsers, ctors/dtors, accessors, encode/decode). Most were reconstructed in parallel by subagents; the shared idioms are the try/catch file read, inline `DecodeNumber` record arguments, and the empty-ctor + anonymous-aggregate result-pair shape |
| Shared value decoder done | 1 | done | The base-253 decoder is reconstructed per unit (each *values class has its own DecodeNumber, 83-85 instrs) and the base-253 encoder per container (EncodeNumber, 134 instrs) - all byte-exact. The decoder is per-class source (not one shared function), confirmed by the distinct mangled names and identical 287-byte bodies with 16-byte multiplier spacing |
| Classvalue byte-exact | 1 | done | ctor 19 + dtor 24 match; ECF element layout (0x1C) pinned by the owning vector |
| Ctor/dtor family byte-exact | 1 | done | `Itemground`, `Npcdrop`, `Playerskill`, `Playerinventory` — all ctors and deleting-dtors match (6 units total with `Weaponmap`/`Shopitem`) |
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
- ~~Whether the `GUI` unit itself defines `TGUI` or whether the form lives in
  `Mainform`~~ Resolved: the class RTTI's unit name is `MainForm` and the class
  is exported through `@@Mainform@Initialize`, so `TGUI` is defined in
  `Mainform.cpp`; `GUI.cpp` is the project main unit.
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
