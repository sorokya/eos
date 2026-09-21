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
  `c0w32.obj`, then the units in link order), and **where a library's members
  land is set by where its `.lib` appears in the link line** — after the comma
  they are appended after every object, but listed inline in the OBJFILES field
  they are laid out there (established with a synthetic `ilink32` probe: the
  same `.lib` moved between the object field and the library field relocates its
  members). The reference interleaves its libraries: `vcldb50.lib` sits between
  `Itemground` and `Itemchest` (a 109,228-byte block, vs our pulled 109,236) and
  `vcl50.lib`/`vclbde50.lib`/`vcle50.lib` between `Weaponmap` and `Banned`
  (~486,572 bytes), with `import32.lib`/`cp32mt.lib` searched last. Listing them
  after the comma, as the build did until now, pushes every library member to the
  end and shifts the post-`Itemground` unit RVAs by up to ~600 KB; the inline
  placement in `scripts/build.sh` brings every export RVA within ~4 KB of the
  reference. The `Skillvalue`/`Npcvalue` inversion was bisected to *scanning*
  `vcldb50.lib` inline: any position inverts the pair, the VCL block alone does
  not, and the order is invariant both to the command line and to swapping the
  two units (which have no reference to each other). **Resolved** by extracting
  `vcldb50`'s four needed members (`DbLogDlg`, `DBCommon`, `DbConsts`, `Db`) with
  `tlib` and listing them as explicit objects instead of the `.lib`;
  unreferenced COMDATs are dropped exactly as when the `.lib` is pulled, so the
  block is byte-identical (109,236 B vs the reference's 109,228) and the export
  order now matches 133/133. **Size residuals, measured at the last non-zero
  byte of each section** (raw sizes are `FileAlignment`-rounded and overstate the
  difference): `.text` **+0x1FC (508 B)**, `.data` **+0x1D4 (468 B)**, `.rsrc`
  **−0x238 (568 B)**, `.reloc` +0x3C. The `.text`/`.data` surpluses move together,
  as extra compiler-emitted COMDATs contribute both code and RTTI/EH metadata.
  Two tools settle what they are *not*: `scripts/libcompare.py` locates all 307 of
  our CODE modules in the reference (**absent=0** — we pull no module the
  reference lacks) and `scripts/objfuncs.py` does the same per function for one
  unit (`Npcvalue`: **found=87, absent=0** — the unit emits no function the
  reference lacks). So the surplus is not an extra module or function; it is a
  COMDAT *placement/size* difference (which side keeps a shared STL helper, plus
  the alignment padding each module carries). `libcompare --sizes` points at
  cp32mt's iostream/locale members (`allstl`, `ios`, `iostream`, `char`,
  `charby`, `rwlocale`, `stdexcept`) as the cluster where ours is largest, but the
  reference's per-member boundaries are inferred from our own module starts, so
  treat it as a pointer, not a measurement.
  **Map caveat:** the detailed `ilink32 -s` map is truncated by a deterministic
  Wine fault (`Unhandled illegal instruction`) before the final ~2.4 KB of modules
  are written — with or without `-v`, and with a response file — so the map's last
  entry reaches RVA offset `0x158d54` while `.text` content runs to `0x1596bc`.
  Block/unit accounting is unaffected; the cp32mt tail is not. Units within a
  contiguous run are otherwise contiguous and a unit's `code_span` is its real
  code size, confirmed on `Serial`: our linked `SERIAL.OBJ` is `0x1A94` bytes
  against a reference span of `0x1AF0`.
  **Caveat:** every linked module
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
| Static libraries | `vcl50.lib vcldb50.lib vclbde50.lib import32.lib cp32mt.lib` (from `Lib/Debug`) | minimal VCL+BDE link resolves; Debug variant byte-matched. **`cw32mt.lib` must not be listed**: its `crtlst_[iel].c` stubs of `___CRTL_VCL_Init`/`_Exit`/`___CRTL_VCLLIB_Linkage` beat `cp32mt.lib`'s real `crtlvcl.cpp` and kill the whole VCL init chain (see AGENTS.md) |
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
| Players | `0x00010de4` | in-progress |
| Player | `0x00011f64` | byte-exact |
| Playerskill | `0x00011fd0` | byte-exact |
| Playerinventory | `0x0001203c` | byte-exact |
| Serial | `0x00013b4c` | byte-exact |
| Settings | `0x00015d88` | byte-exact |
| Logins | `0x000166ec` | byte-exact |
| Packets | `0x00074648` | in-progress |
| Mysqlcontrols | `0x00078180` | byte-exact |
| Itemvalue | `0x0007824c` | byte-exact |
| Itemvalues | `0x0007a848` | byte-exact |
| Npc | `0x0007ab00` | byte-exact |
| Mapchest | `0x0007ad1c` | byte-exact |
| Mapcontrol | `0x00087d38` | in-progress |
| Mapobject | `0x00087dc4` | byte-exact |
| Mapwarp | `0x00087e64` | byte-exact |
| Map | `0x00088474` | byte-exact |
| Itemground | `0x000884d8` | byte-exact |
| Itemchest | `0x000a2ff8` | byte-exact |
| Skillvalues | `0x000a5400` | byte-exact |
| Npcvalues | `0x000a8e90` | byte-exact |
| Skillvalue | `0x000a8f64` | byte-exact |
| Npcvalue | `0x000a9ac8` | byte-exact |
| Npcdrop | `0x000a9b34` | byte-exact |
| Jukeboxcontrol | `0x000aa9a8` | in-progress |
| Jukebox | `0x000aaa70` | byte-exact |
| Newscontrol | `0x000aaf5c` | byte-exact |
| Doorcontrol | `0x000ab108` | byte-exact |
| Msgboardcontrol | `0x000ae180` | byte-exact |
| Msgboard | `0x000ae35c` | byte-exact |
| Npccontrol | `0x000b11f4` | in-progress |
| Gamecontrol | `0x000b15a0` | byte-exact |
| Shopvalues | `0x000b49b4` | byte-exact |
| Shopitem | `0x000b4a20` | byte-exact |
| Shopvalue | `0x000b546c` | byte-exact |
| Shopcraft | `0x000b5500` | byte-exact |
| Chestcontrol | `0x000b5bc4` | byte-exact |
| Weaponmap | `0x000b5c60` | byte-exact |
| Banned | `0x0012cb2c` | byte-exact |
| Effectcontrol | `0x0012db00` | byte-exact |
| Eventcontrol | `0x0012e204` | byte-exact |
| Wedding | `0x0012e35c` | byte-exact |
| Weddings | `0x001303d0` | in-progress |
| Learnvalue | `0x00130e78` | byte-exact |
| Learnvalues | `0x0013326c` | byte-exact |
| Learnitem | `0x00133300` | byte-exact |
| Mysqlthread | `0x00133818` | byte-exact |
| Mysqltask | `0x001342bc` | byte-exact |
| Innvalues | `0x00135d94` | byte-exact |
| Innvalue | `0x00135ecc` | byte-exact |
| ClassValues | `0x00137460` | byte-exact |
| Classvalue | `0x00137510` | byte-exact |
| Playerquest | `0x001375b4` | byte-exact |
| Questtype | `0x001376a0` | byte-exact |
| Quest | `0x00137ac0` | byte-exact |
| Questengine | `0x0013c7fc` | in-progress |
| Queststate | `0x0013cd28` | byte-exact |
| Filecache | `0x0013e248` | byte-exact |
| Killcounter | `0x0013e3a0` | byte-exact |
| Playercommand | `0x0013e494` | byte-exact |
| Killcounters | `0x0013f5e4` | byte-exact |
| Questcounter | `0x001406d4` | byte-exact |
| Questcounterlist | `0x0014082c` | byte-exact |
| Questcounters | `0x00141e84` | byte-exact |

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
  search paths `Lib;Lib/Obj;Lib/Debug;Lib/Release`, libraries `vcl50.lib
  vcldb50.lib vclbde50.lib import32.lib cp32mt.lib` (the Debug VCL/BDE
  set was established later — see the library-linkage finding above;
  `cw32mt.lib` was dropped once its `crtlst_[iel].c` stubs were found to
  disable the VCL init chain). A minimal
  VCL+BDE link (`make sanity`) reproduces every reference header field (`0x010e`,
  subsystem, base, alignments, stack/heap) and the section order.
- Deterministic timestamp: `normalize_pe.py` rewrites `TimeDateStamp` at
  `e_lfanew + 8` (`0x208`) **and** the `TimeDateStamp` of every
  `IMAGE_RESOURCE_DIRECTORY` in `.rsrc` (ilink32 stamps those too, which made
  back-to-back relinks differ); verified to restore the reference MD5 from a
  copy differing only in those fields, and a no-op on the reference itself. Clock control (`libfaketime`) remains an
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

**Resource findings (2026-09).** `make extract` (`scripts/extract_res.py`)
reconstructs a `.res` from the reference's embedded `.rsrc`: it walks the tree
and writes each entry with the exact type, ordinal/name and language. The
`.res` entry format was recovered from `brcc32`'s own output (numeric type/name
= `WORD 0xFFFF` + ordinal; string names are NUL-terminated UTF-16; the first
entry must have all-zero flags or `ilink32` rejects the file with "Unsupported
16bit resource"). The reference `.rsrc` holds 38 entries: 7 `RT_CURSOR`, 1
`RT_ICON`, 18 `RT_STRING` blocks, 4 named `RT_RCDATA` (`DVCLAL`, `TGUI`,
`TLOGINDIALOG`, `TPASSWORDDIALOG`), 7 `RT_GROUP_CURSOR` and the `MAINICON`
group. Only the application owns `TGUI` and `MAINICON`; the rest come from the
VCL/BDE library `.res`.

**The linker's resource merge cannot be controlled from our link line.** The
library `.res` are referenced by *absolute* path inside the `.lib`, so they
cannot be shadowed via `-L`; an explicit `.res` never overrides a duplicate
(the library's is kept, and `ilink32` auto-generates `DVCLAL` in a temp
`TMP1.$$$` that wins), and `RT_STRING` blocks are renumbered into the linker's
global string pool in library order. Providing the reference's exact payloads
therefore does *not* reproduce `.rsrc`: linking the extracted `.res` leaves the
15 string blocks and `DVCLAL` wrong. `-Rr` ("replace resources") does not change
precedence.

Consequently `.rsrc` is downstream of the same link-composition difference that
leaves `.text` 512 B large: our rebuilt `.rsrc` is `0x6000` raw against the
reference's `0x6200`, so a post-link byte copy (`scripts/inject_rsrc.py`,
geometry-gated) cannot apply until the rest of the image matches. The byte-copy
step is the planned mechanism once `.text`/`.data` are exact.

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
| Itemvalue byte-exact | 1 | done | ctor 15, deleting-dtor 11 match; `ItemValue` = `id` at 0 plus the widened `EifRecord` fields in memory (`sizeof` 0x58, see `src/Itemvalue.h`), offsets pinned by the `Itemvalues` parser stores and `Get*` accessors. The earlier "self `ItemValue *` at 8" reading was wrong: offset 8 is `special` |
| eo-protocol reference | 0 | done | `ref/eo-protocol` submodule added; `xml/pub/server/protocol.xml` gives the pub record layouts/field names for the `*values` parsers |
| ClassValues byte-exact (6/6 fns) | 1 | done | ctor 43, dtor 26, GetByIndex 75, AddClass 66, DecodeInt 85 all match; `LoadClasses` 596/596 and `size` 9/9 now match too |
| LoadClasses (state) | 1 | done | 546 -> 300 -> 0 mismatches. Root cause was the EH scope structure: a `try`/`catch` around the file-read with `h`/`size`/`buf` outside the `try`, inline `DecodeInt` fields in `AddClass`, a loop-carried `total` temp (nested block), a dead `int version` store, `field_0 = file - 1`, and a one-call `size()` accessor |
| Itemvalues byte-exact (21/21 fns) | 1 | done | ctor 47, `LoadItems` 1243 (includes the `try`/`catch` file read, `field_14->Clear()` + `Clear(self)` reset, `namelen+off` inline `DecodeNumber` record args), `AddItem` 167, `Clear` 40, `GetRecordSlot`/`GetByIndex`/`GetCount`, 11 `Get*` accessors, `DecodeNumber` 85 — all 0 mismatches. `values` is `std::vector<ItemValue *>`; the two result-pair structs need an empty user ctor plus a single anonymous aggregate member to reproduce the folded ctor (0x44f58c) and the memcpy-style return |
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
| Settings byte-exact (39/39 fns) | 1 | done | ctor 139 (`0x413b6c`), dtor 38, 28 accessors, `LoadConfig` 870, `SetIniPath` 177, `CloseIni` 8, `ReadIniInt` 224, `ReadIniString` 291, `ReadIniBool` 331, `SetWorldCommunication` 7 - all 0 mismatches. Layout (0x8c) pinned by the ctor stores and the `LoadConfig` key->field map; field names come from the observable INI keys, the runtime string "World communication changed to: " (`+0x78`) and the observed clamps. The three `ReadIni*` parsers each wrap the scan loop in a `try`/`catch(...)` (like `Serial::ReadKey`); `ReadIniBool` takes/returns `char` (not `bool`), and the six boolean getters return `char` so bcc32 emits a plain `mov al,[..]` instead of `cmp`/`setne`. Two original quirks preserved: `maxquests` defaults to `max_maps`, and the `loginprotection` default argument is `join_message`. `Mainform.cpp` now includes `Settings.h` and calls `Settings::Get*`; `verify_units.py` re-joins a Ghidra prologue-split fragment (`ReadIniBool` `0x4158c4` / `FUN_004158ca`) |
| Gamecontrol byte-exact (7/7 fns) | 1 | done | ctor 12 (`0x4b1214`), dtor 11, `Exp_RequiredForLevel` 22, `Combat_CalcArmorPen` 86, `Combat_CalcHitRate` 86, `Combat_CalcElementMult` 47, `Combat_ElementScore` 24 - all 0 mismatches. The class is 8 bytes (`operator new(8)` at `0x401f96`) with an empty ctor and explicit empty dtor; no field is ever read or written, so the two dwords stay unnamed. Codegen facts: `Exp_RequiredForLevel` is a written-out product `(level*1.1)*(level*1.1)*(level*1.1)*100` (a `pow(...,3)` call does not appear - bcc32 would emit a real `pow` call), `Combat_ElementScore` uses `atk+atk+target` (not `2*atk`), the two combat curves are two independent `if`s (not `else if`) with named branch-scope `double` locals and compound assignment (`result -= rating`), and the elemental step is `0.01L * result + 1` (the reference loads a `long double` 0.01) |
| Mainform + Settings + Gamecontrol wired | 2 | done | `Mainform.cpp` includes the real `Settings.h`/`Gamecontrol.h` and drops both size/ctor stubs; the private controller pointer is `server_ctrl` again (the prior rename to `server` collided with the DFM-published `TServerSocket *server`, which is unpinnable). All 8 handlers re-verified after the change |
| Newscontrol byte-exact (5/5 fns) | 1 | done | ctor 20 (`0x4aaa90`), dtor 11, `LoadNews` 23, `Get` 54, `LoadFile` 248 - all 0 mismatches. 4-byte class (single `TStringList *ini_file`); leaf unit. `Get` returns `"NULL"` unless `index < Count`, else `Strings[index]` (assignment, not concatenation). `LoadFile` reads `./config/news.ini`, stripping lines shorter than 2 chars and `#` comments. `LoadNews` must inline the literal (a named local adds an EH scope marker) |
| Logins byte-exact (10/10 fns) | 1 | done | ctor 68 (`0x415da8`), dtor 52, `AddReservedName` 61, `Tick` 47, `HandleAddress` 126, `AddLogin` 46, `SetReservedName` 53, `ConnectionLog_CheckIP` 54, plus 2 compiler COMDATs (`TList::TList`, `LoginEntry::$bdtr`) - all 0 mismatches. 0x10 bytes: `Mysqlcontrols*`, `TList *list_a`, `TList *reserved_names`, `String`. `reserved_names` holds `{String name; String value}` and `list_a` holds `{String address; int count}`. `ConnectionLog_CheckIP` is the one non-static operation (the static form changes the loop-body operand order). Reserved names `vult-r`/`aengie`/`angel` seeded with `"new"` |
| Mysqlcontrols 26/28 hand-written | 2 | in-progress | Byte-exact: `Free`, the two pending-write loaders `FUN_004752d4`/`FUN_00475b20`, `Db_GetString`/`Db_GetInt`, `FUN_00476bfc`/`FUN_00476c14`, `GetResultCount`, `Mysql_SubmitQuery`(+`_FromCallback`), `Query`, `Mysql_ExecDirect`(+`_FromCallback`), `ExecDrop`, `Db_GetActiveConnectionCount`, `FUN_004772a0`, `FUN_004772b8`, `SpamGuard_CheckCooldown`, `Server_GetUptime`, `TestConnection`, `Database_CanReconnect`, `Mysql_SanitizeString`, `Db_SanitizeString`, `NormalizePlayerText`, `FUN_00477a00` (base-253 decoder) and `FUN_004762c8` (status `UPDATE` builder). Remaining two: the constructor (`worker_thread = new Mysqlthread(...)` uses the Delphi `TThread` new-helper, which needs the Mysqlthread unit's VMT global - not byte-exact in isolation) and `Connect` (~592 insns; `INSERT ... VALUES` / `SHOW TABLE STATUS` builders need expression-order convergence). The ~45 `std::vector`/allocator COMDATs are emitted from the member declarations |
| Mysqlcontrols decoder/parsers byte-exact | 2 | done | `Mysql_SanitizeString` 115 and `Db_SanitizeString` 203 must use the `AnsiLowerCase`/`AnsiUpperCase` free functions (not `String::LowerCase()`); the sanitize whitelist is the positive form. `FUN_00477a00` concats a **bare** `char` (`reversed = reversed + value[i]`, no `String(...)` cast) so bcc32 does not reuse the ctor's `eax`, is wrapped in `try`/`catch(...)`, and uses signed `int u`, a `bool done` flag, `'9'-u+'0'`/`'z'-u+'a'` transforms and three independent `if`s. `FUN_004762c8` appends each field as `s + "label = " + IntToStr(v) + ","`, stores the differences in named locals, updates `last_query_time` (not `connected_time`), and guards with `if (elapsed >= refresh)` |
| Mysqlthread + Mysqltask reconstructed | 2 | in-progress | `Mysqltask` object (0x533838..0x5342bc): `mySQLtask` ctor/dtor and `mySQLbuffer` ctor/dtor/`EnqueueTask`/`HasPendingTask` plus all 16 `std::vector<mySQLtask*>`/allocator COMDATs - 21/21 byte-exact. `Mysqlthread` object (0x533320..0x533818): the TThread-derived `MySQLthread` ctor 44, `OnResult` 18, deleting dtor 23 and the vector COMDATs match; `Execute` (179) remains (field-store operand order + ebx/esi/edi frame). Real class names are `MySQLthread`, `mySQLtask`, `mySQLbuffer` (RTTI, capital SQL). |
| Mysqlcontrols ctor byte-exact | 2 | done | The ctor needed a user-declared (empty) `~Mysqlcontrols()`: that is what arms the function-body EH scope `8` at entry and reproduces the reference's 5-marker stream. With it, and `Free` calling `::operator delete(self)` directly (the reference deleting-dtor shape), both the ctor (79/79) and `Free` (11/11) are exact. `connect_string` is built in one expression `... + "'" + IntToStr(version_major) + "." + ... + "',0,0,0,0,'no data','no data')"`, and each `SHOW TABLE STATUS` table check sits in its own `try`/`catch(...)`. `Connect`'s code is instruction-for-instruction identical (605/605); only one EH cleanup record differs (111 marker-level mismatches). |
| Mysql code reconciled and linked | 2 | done | `Mysqlcontrols.h` now includes `Mysqltask.h`/`Mysqlthread.h` and uses `mySQLbuffer *thread_queue` / `MySQLthread *worker_thread`; the ctor uses `new MySQLthread(GUI->mysession, GUI->mysql, GUI->myquery, thread_queue, false)` (which emits the class VMT load), SubmitQuery/ExecDirect call `thread_queue->EnqueueTask`, IsTaskPending calls `thread_queue->HasPendingTask`; `Mysqlthread.cpp` dropped its `mySQLtask`/`mySQLbuffer` shims and includes `Mysqltask.h`. `make build` links. Full verify 242/246. |
| Filecache byte-exact (18/18) | 2 | done | ctor 57, dtor 38, `CheckCacheFile` 35, `FUN_0053d0e8` 134, `LoadPlayerCache` 192 (`./cache/players.chk`), `LoadGuildCache` 157 (`./cache/guilds.chk`), `FUN_0053d754` 68, `Database_FlushCache` 541, plus `FilecacheEntry`/`FilecacheEntryB` ctors/dtors and the 6 vector COMDATs - all 0 mismatches. `Filecache` owns `FilecacheEntry`/`FilecacheEntryB` (0x18/0x14); `Database_FlushCache` is a free function. `Mysqlcontrols` now includes `Filecache.h` and calls `Filecache::{CheckCacheFile,LoadPlayerCache,LoadGuildCache}`. |
| Queststate byte-exact (12/12) | 2 | done | `QuestState` ctor 65 / deleting dtor 43, `QuestAction` ctor 27 / dtor 29, `QuestRule` ctor 40 / dtor 34, plus the `std::vector<QuestAction*>`/`<QuestRule*>` COMDATs - all 0 mismatches. RTTI names are `QuestState`/`QuestAction`/`QuestRule`; the `QuestAction`/`QuestRule` ctors/dtors are emitted in this unit's span (Questengine only references them). |
| Npc byte-exact (2/2) | 2 | done | `Npc(short,short,short,short,short,int,short)` 115 (Ghidra `Npc_Init`) and the deleting dtor 34 - 0 mismatches. sizeof 0x94; a user-declared empty `~Npc()` is required (the implicit one omits the EH scope marker). `Npc_Init` derives `act_ticks` from `spawn_type` and needs `TDateTime now = Now(); nDeath_ms = DateTimeToTimeStamp(now);` (the inline form emits `fstp [esp]` and the wrong frame). |
| Small record units byte-exact (Mapobject, Mapwarp, Questtype, Playerquest, Playercommand, Wedding) | 2 | done | `Mapobject` {short x,y,value,ticks} sizeof 8; `Mapwarp` {short from_x,from_y; int dest_map,level; short to_x,to_y} sizeof 0x10; `Questtype` {int value; String name} size 8; `PlayerQuest` (0x14); `PlayerCommand` {int action,arg; String text} size 12; `Wedding` sizeof 0x28. Each is a trivial ctor + user destructor; all 2/2 byte-exact. |
| Killcounters byte-exact (9/9) | 2 | done | ctor 36, dtor 34, `Clear` 45, `IncrementAndGet` 117, `Add` 78, `Get` 84, `Init` 98, `Extract` 68, `Save` 153 - all 0 mismatches, plus ~24 `std::vector<KillCounter>`/allocator COMDATs. sizeof 0x368. The WIP `Init(this)` tail is now present. |
| Msgboard byte-exact (3/3) | 2 | done | `Msgboard()` 24, `Msgboard(short,String,String,String)` 65, `~Msgboard` 34. Layout (16): `short id`@0, `String poster`@4, `subject`@8, `message`@12. |
| Mapchest byte-exact (2/2) | 2 | done | `Mapchest(short,short,short)` 34 and `~Mapchest` 26 plus the `std::vector<Itemchest>` COMDATs. Layout (0x28): x/y/key_id + `char updated` + `std::vector<Itemchest> slots`; sizeof(Itemchest) recovered as 0x2c. Itemchest is a size-correct placeholder pending its own unit. |
| Itemchest byte-exact (2/2) | 2 | done | `MapItem(int item_id)` 46 and the deleting dtor `$bdtr$qv` 26, at `0x4a2fac..0x4a2ff8` (the module's code is exactly these two functions). RTTI names the class `MapItem` (not `Itemchest`); `verify_units.py` maps unit `Itemchest` to prefix `@@MapItem@`. The unit's `Initialize`/`Finalize` stubs carry no init body. The 109 KB module span from `units.tsv` is an artifact of four unexported BDE/VCL library members (init stubs at `0x4888f8`, `0x48d3c0`, `0x48d5d8`, `0x4a2fa4`, counters `0x58bbc0..0x58bbcc`) that lack the prologue form of the module stub and so were not segmented; the `FUN_*` functions before `0x4a2fac` are those library members, not this unit. |
| Effectcontrol byte-exact (4/4) | 2 | done | ctor 46, `$bdtr` 11, `Tick` 945, `AppendEncoded` 100. Layout (0x48): `aState_countdown[4]`/`aState_value[4]`/`aState_extra[4]` + broadcast gate + `char *encode_scratch` + Settings/Mapcontrol/Players/Server. Ctor order `(Mapcontrol*,Players*,Server*,Settings*)`. `AppendEncoded` returns String by value. |
| Npccontrol partial (8/13) | 2 | in-progress | Byte-exact: ctor 49, `$bdtr` 26, `Npc_GetDistance` 43, `Npc_IsWithinRange` 41, `Npc_DoMove` 84, `Npc_ValidateMove` 86, `Npc_Wander` 253, `Packet_AppendEncoded` 100. Remaining: `NpcControl_Tick` (6064 B), `Npc_AttackPlayer` (converged except local-slot order and an 8-byte pair push), `Npc_ChaseTarget`. Layout (0x44). |
| Weddings partial (7/8; class is WeddingController) | 2 | in-progress | RTTI class name is `WeddingController` (unit/file `Weddings`). Byte-exact: ctor 34, dtor 26, `Has` 36, `Add` 78, `Confirm` 295, `BroadcastPriestLine` 52, `BothPresent` 40, `AppendEncoded` 100, plus 21 vector COMDATs. `Tick` (1071) differs in 4 hunks (510 marker-equal mismatches) - dead `sete` sequences bcc32 emits that our structurally-identical source does not. Layout (0x2c): `char *encode_scratch` + `Players*` + `Server*` + `std::vector<Wedding*>`. Mainform now uses `WeddingController`. |
| Map byte-exact (8/8) | 2 | done | `Map(short,int,int)` ctor 204 + deleting dtor 85 + 5 element-vector ctors + `std::vector<bool> tile_bits` ctor - all 0 mismatches. sizeof 0x160. RTTI names the class `MapContainer` (and elements `MapWarp`/`MapObject`/`MapChest`) where the repo spells `Map`/`Mapwarp`/... - a `.data`/RTTI-spelling item for the final MD5, not codegen. |
| Doorcontrol byte-exact (3/3) | 2 | done | ctor 15, dtor 11, `Tick` 97. Layout 0x0c: `TTimeStamp last_tick` + `Mapcontrol *`. Tick iterates `map_control->maps`/`map->tile_specs` (`std::vector` begin/end emit naturally) and closes doors (spec 9->7, 0xb->10). |
| Chestcontrol byte-exact (5/5) | 2 | done | ctor 29, dtor 11, `Tick` 338, `AppendEncoded` 100, `InRange` 29. sizeof 0x14. |
| Eventcontrol byte-exact (4/4) | 2 | done | ctor 29, dtor 11, `Tick` 401, `AppendEncoded` 100. sizeof 0x14. `Player_Warp`'s position is a by-value `TPoint` (8 bytes), not two shorts. |
| Player byte-exact (9/9 source + 17 COMDATs) | 2 | done | `Player(void*)` 388, `~Player` 173, `HpPercent` 35, `CountPartyMembers` 20, `AddPartyMember` 27, `IsPartyMember` 21, `ClearPartyRoster` 22, `UpdateBaseStats` 40, `CalculateHP_TP_SP` 130, plus the `vector<PlayerInventory/PlayerSkill/PlayerQuest/PlayerCommand>` COMDATs. sizeof 0x3f8. `CountPartyMembers` is the one non-static operation. |
| Msgboardcontrol partial (10/19) | 2 | in-progress | Byte-exact: ctor 80, dtor 72, `AppendEncoded` 100, `ClearBoard` 19, `DeletePost` 48, `CountPosts` 56, `SetPostLimit` 63, `GetBoard` 51, `SetDecodeSource` 33, `DecodeNumber` 75, plus ~48 vector COMDATs. Remaining: `AddPost`, `GetPost`, `BuildBoardName`, `BuildBoardData`, `LoadBoard`, `ReadToken`, `ReadRest`, `LoadBoards`, `SaveBoards`. `Msgboard`'s ctor first param must become `int` (not `short`) to unblock `AddPost`/`LoadBoard`. sizeof 0x3c4 (inline `vector<Msgboard> boards[8]` + `String[8]` + int[32] arrays). |
| Quest byte-exact (14/14) | 2 | done | `Quest(int)` ctor 35 and `~Quest` 31 plus the 12 `std::vector<QuestState*>`/allocator COMDATs emitted from the `states` member - all 0 mismatches. Layout (0x34): `quest_id`@0, `String name`@4, `version`@8, `field_0xc`@0xc, `char loaded`@0x10, `std::vector<QuestState*> states`@0x14. No Quest methods beyond ctor/dtor. |
| Mainform wired to the reconstructed controllers | 2 | done | `Mainform.cpp` includes `Settings.h`/`Gamecontrol.h`/`Logins.h`/`Newscontrol.h`/`Mysqlcontrols.h` and drops all five stubs; call sites become `Settings::Get*`, `Mysqlcontrols::{TestConnection,Connect,Free,FUN_004762c8,Db_GetActiveConnectionCount}`, `Logins::{HandleAddress,Tick}`. All 8 handlers re-verified (0 mismatches) and `make build` links |
| compare_asm canonicalizes package symbols | 1 | tool | `canon()` now maps a bare-symbol memory operand (bcc32 emits `[_GUI]` for a `__declspec(package)` global; the reference shows the resolved address) to `[ADDR]`. This unblocked scoring `Mysqlcontrols` (10/28 -> 19/28 reported) and is a no-op for register/offset operands |
| *values family byte-exact (13 units) | 2 | done | `Classvalues`/`Classvalue`, `Itemvalues`/`Itemvalue`, `Skillvalues`/`Skillvalue`, `Npcvalues`/`Npcvalue`, `Shopvalues`/`Shopvalue`/`Shopcraft`, `Learnvalues`/`Learnvalue`/`Learnitem`, `Innvalues`/`Innvalue` — every source function scores 0 mismatches (parsers, ctors/dtors, accessors, encode/decode). Most were reconstructed in parallel by subagents; the shared idioms are the try/catch file read, inline `DecodeNumber` record arguments, and the empty-ctor + anonymous-aggregate result-pair shape |
| Shared value decoder done | 1 | done | The base-253 decoder is reconstructed per unit (each *values class has its own DecodeNumber, 83-85 instrs) and the base-253 encoder per container (EncodeNumber, 134 instrs) - all byte-exact. The decoder is per-class source (not one shared function), confirmed by the distinct mangled names and identical 287-byte bodies with 16-byte multiplier spacing |
| Classvalue byte-exact | 1 | done | ctor 19 + dtor 24 match; ECF element layout (0x1C) pinned by the owning vector |
| Ctor/dtor family byte-exact | 1 | done | `Itemground`, `Npcdrop`, `Playerskill`, `Playerinventory` — all ctors and deleting-dtors match (6 units total with `Weaponmap`/`Shopitem`) |
| Full skeleton links | 1 | done | `make build`: 133 exports, working PE |
| Full skeleton runs functionally | 1 | todo | launch and exercise |
| All units C++-verified | 2 | todo | `make functions` at 100% |
| `.rsrc`/`.reloc` match | 3 | todo | section diff |
| Full MD5 match, reproducible | 4 | todo | `md5 -q` from clean tree |
| `Players_Add` byte-exact (last `verify_units` failure) | 2 | done | 113/113, 0 mismatched (`0x4081c8`). Root cause was a mis-modelled type, not a bcc ordering quirk: `Socket_GetRemoteIP` is the VCL member `TCustomWinSocket::GetRemoteAddress` (`0x4cf070` — its body pushes `SizeOf(Addr)`, `&Addr` and `FSocket=[this+4]` before the `getpeername` thunk). Written as the real property `socket->RemoteAddress`, the two `AnsiString` temporaries are member-return results and the reference's LIFO destructor order appears; the free `String __fastcall Socket_GetRemoteIP(void*)` wrapper (still declared in `Player.h` for `Player.cpp`/`Packets.cpp`) binds a hidden return temporary instead and yields FIFO. `Players_FindByName` is the control: same two-temporary `==` shape, FIFO in both, byte-exact. |

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

## Outstanding cross-unit externs and stubs

Declarations that remain in one unit because no owner header declares them yet.
Each must be resolved (defined in its owner, or shown to be a compiler/library
COMDAT) before the final link; none may be guessed away.

| Symbol | Declared in | Owner / status |
| --- | --- | --- |
| `Players_Iter_Begin` / `Players_Iter_End` | `src/Players.h` (declaration only) | Players unit (`0x407f28`/`0x407f34`), the out-of-line `std::vector<Player*>::begin`/`end` COMDATs, but **no definition exists anywhere** (`Players.cpp` itself calls `players.begin()/.end()`). The tree still links (`scripts/build.sh`; `MAP=1`) because every remaining call site is in a Packets/Npccontrol function that `ilink32` discards as an unreferenced COMDAT — the map carries no `Players_Iter` reference — so the gap is latent, not link-breaking. Chestcontrol/Effectcontrol/Eventcontrol are already converted; Packets/Npccontrol must be converted the same way. |
| `Character_BuildSaveQuery` | Packets | Players unit, `0x40902c` (14592 B), not yet reconstructed. |
| `Map_GetTileSpecObject` | Packets | Mapcontrol `0x47c114`; only caller is `Chair_Execute`. Returns `MapObject` by value. Name not yet settled in the owner. |
| `Map_InitBlank` (`0x487e84`), `FUN_0048835c` (`0x48835c`) | Mapcontrol | Map unit. `Map_InitBlank` returns `MapContainer` by value and takes `(map_id, width, height)`; `FUN_0048835c` is its destructor (`(&map, 2)`). Needed by `Mapcontrol_LoadMap`; no owner header declares them yet. |
| `MapVector_End` (`0x47ecd4`), `MapVector_Insert` (`0x47ece0`) | Mapcontrol | Mapcontrol itself: the `std::vector<MapContainer>::end` / `insert` COMDATs. Called as `MapVector_Insert(map_control, MapVector_End(map_control), &map)` (the vector is `map_control->maps` at offset 0). No header declares them. |
| `Sock_Send` | Packets | Library/RTL helper `0x4cf568`; not in `analysis/target/unit_functions.tsv` (so not a unit function). |
| `MysqlCallback_Dispatch` | Mysqlthread | Packets `0x450618` (36735 B), not yet reconstructed. |
| `Mainform_GetServer` | (resolved) | Mainform `0x4027b4`: `mov eax,[ebp+8]; mov eax,[eax+0x310]; ret`, i.e. `form->server_ctrl`. Defined in `Mainform.cpp` and declared in `Mainform.h`; the MySQLthread/Players call sites keep the reference's out-of-line call (the accessor is **not** inlined by the original build). |
| `extern TGUI **MAINFORM` | Packets, Players | Global `0x58b60c` (initialised to `&GUI` at `0x58bb70`); no owner declaration anywhere. |
| `EO_ByteRange_FromString` (`0x5432d8`) | Packets (`src/Packets.h`) | No owner: the address is **not a function row** in `unit_functions.tsv` — it sits in a module the inventory classifies as library inside GUI's span. **Verified identity: `0x5432d8` is the RTL `std::basic_string<char,...>::basic_string(const char*, const Allocator&)` constructor** (the surrounding `cw32mt.lib` index strings are `basic_string`/`char_traits`/`allocator`; it returns `this` at `0x543403`). `Client_SendEncoded` now calls it by source (`std::basic_string<char> range(out.c_str())`); the `src/Packets.h` free-function declaration survives only because `Player_HandlePacket` still uses it and must be converted the same way. |
| `Coords_IsWithinTwo` (`0x473124`, `FUN_00473124`) | Packets (defined, `src/Packets.cpp`) | A `Coords_IsAdjacent` clone with threshold `<= 2`; fully reconstructed and byte-exact (sheet row 496) but **no declaration and no caller** — either dead in the original or its caller is unreconstructed. |
| `FUN_0044f73c` / `FUN_0044f710` | Packets (`src/Packets.h`) | No definition, only `_Stub`s. The reference rows (`0x44f710`/`0x44f73c`) are the out-of-line `std::basic_string<char>::begin`/`end` COMDATs (mangled `...basic_string$c...@begin/end$qv`), not application functions; call `.begin()`/`.end()` directly. |
| `FUN_00462374` | Packets (`src/Packets.cpp`) | Declared `bool(Server*, Player*, String)`; only `FUN_00462374_Stub(int, int)` exists — the declaration and the stub disagree on arity and types. Sheet row 374 `stubbed`. |

The same-class scan of the compiler listings also resolved, in editable units,
three more free declarations that are really container/class members: Mapcontrol
`FUN_004813c8` = `std::vector<bool>::size` (`0x4813c8`), `FUN_0048441c` =
`std::vector<bool>::resize` (`0x48441c`) and `FUN_004a9e74` =
`JukeBoxController::Add` (`0x4a9e74`). Calling the real members emits the
reference COMDATs; the `resize` call also pulls in the whole `vector<bool>`
helper block (+13 byte-exact). Still outstanding (all in the off-limits Packets
unit unless noted): free declarations that are really `Gamecontrol::Combat_Calc*`,
`ItemValues::GetElement`, `Player::HpPercent` and `Player::IsPartyMember`;
`Server_BroadcastToMapExceptSelf`, `Sock_Send`, and the stubbed `FUN_00462374`,
`FUN_0047060c`, `FUN_004708d4`; Mainform's unimplemented `FUN_00403080`; Packets'
`FUN_004731d0`/`FUN_00473540` and `Server_RemovePlayer` (`0x41728c`); and the data
global `TGUI **MAINFORM` (`0x58b60c`), still without a definition.

Resolved since (all in Packets, byte-exact): `Player_FireQuestTriggers` (182 B,
now a definition rather than a bare declaration), the unnamed broadcast helpers
`FUN_00463750`/`FUN_004639b8`/`FUN_00463be8`/`FUN_00463d40`, `Talk_PlayerWhisper`
(492 B), `FUN_004738b0` (110 B, server start-time rate limit) and
`FUN_004728f8` (74 B, sent-byte counters). `Player_FireQuestTriggers` also
instantiated `std::vector<PlayerQuest>::erase` (`0x44f914`), which became
byte-exact with it.

## Tracker artifact exclusions (verified)

`scripts/track.py`'s artifact pass (`_classify_artifacts`) removes compiler/RTL
rows that look like work but have no hand-written source definition. Three
proof-shaped rules, each verified against the reference bytes and the Borland
sources under `ref/`. The pass runs last and only touches rows that are
otherwise `stubbed`/`unimplemented`/`mismatched` with no matched source
function, so no previously byte-exact row changes status. Row-level diff of
`analysis/target/functions.tsv` (the pass changed exactly these five rows):

| Address | Unit | Size | Before | After | Rule |
| --- | --- | ---: | --- | --- | --- |
| `0x00450110` | Packets | 10 B | app / stubbed | library / n/a | 1 |
| `0x00471e54` | Packets | 78 B | app / stubbed | library / n/a | 2 |
| `0x004a87a0` | Npcvalues | 36 B | app / unimplemented | comdat_cross / n/a | 3 |
| `0x004a87c4` | Npcvalues | 91 B | app / unimplemented | comdat_cross / n/a | 3 |
| `0x004a8820` | Npcvalues | 53 B | app / unimplemented | comdat_cross / n/a | 3 |

Totals: `1795/1801 rows, 666,662/668,128 bytes` -> `1795/1796 rows,
666,662/667,860 bytes`. The numerator is unchanged; the denominator loses the
five rows and their 268 bytes. `make track` is deterministic (three consecutive
runs produce identical TSVs) and `make verify` stays 496/496.

**Rule 1 — RTL data-sentinel accessor (`library`).** The row is a leaf
constant-address getter (`push ebp; mov ebp,esp; mov eax,<imm>; pop ebp; ret`)
whose immediate is a base-relocated address in a data section and every
reference to that data object lies outside the per-unit function inventory.
`0x450110` returns `0x5857ac`, the zeroed `__nullref` sentinel. Its 28 other
references are all at `0x542939..0x54b4ff` — the trailing library region, never
an application row. The accessor is Borland's
`std::basic_string<char>::__getNullRep()` (`ref/Borland5/Include/string.stl:751`;
the static member is defined in
`ref/Borland5/Source/RTL/source/stl/string/string.cpp:41`; the symbol
`@std@%basic_string$c...@__getNullRep$qv` is in `cw32mt.lib`/`cp32mt.lib`). The
reference's live copy is the *library* one (it carries a frame); our build also
emits a frameless `__getNullRep` COMDAT in `Packets.asm` — a separate,
unrelated emission that does not match this row.

**Rule 2 — Borland-header local-static guard (`library`).** The row's body opens
with the bcc32 function-local-`static` initialization guard
(`cmp byte ptr [flag],0` / conditional jump / initialize / `inc byte ptr [flag]`),
and its canonical form, with compiler-static (`$name`) operands folded to `ADDR`,
equals a function our build emits from a file under the Borland `Include/` tree.
`0x471e54` is `std::deque<char>::__buffer_size()`; the bcc32 listing records its
provenance as `...\Borland\CBuilder5\Include\deque.cc`. The guard is emitted for
the function's `static` local and has no standalone source definition.

**Rule 3 — cross-unit compiler COMDAT (`comdat_cross`).** The row's canonical
body has at least two candidate emitters across the tree, every one a
`@@std@`/`@@__rwstd@` template symbol; the row's own unit emits no free
candidate; and it is reached only from already-reproduced (`byte-exact`)
functions or from other rows this rule already excluded. The three rows are
`std::vector<NpcDropItem>::clear` (`0x4a87a0`), `::erase(first,last)`
(`0x4a87c4`) and `std::copy<NpcDropItem*>` (`0x4a8820`), emitted by the
**Npcvalue** unit (`src/Npcvalue.h:44` declares `std::vector<NpcDropItem> drops`;
`src/Npcvalue.cpp:12-22` is `drops.clear(); talk_lines.clear();`) but placed by
the linker inside the `Npcvalues` module range. The element type is proven from
the reference call graph: the two callers of the clear are `NpcValue::NpcValue()`
(`0x4a8f84`) and `NpcValue::NpcValue(int)` (`0x4a917c`), which push `this+0x3c`
(the `drops` member) before calling, and the erase/copy are its callees.
Npcvalue's corresponding copies are the `vector<NpcDropItem>` functions in its
own range (`0x4a901c..0x4a95d1`). This exclusion is a bookkeeping decision: the
bytes are produced automatically by the owning unit's template use, so a sound
per-row attribution would mark the three rows byte-exact (`1798/1799`); under
the tracker's rule that the numerator is not inflated by re-attributing
cross-unit template bodies, they are excluded from the hand-written denominator
instead. No sound *exclusion* of the RTL/header rows would be possible without
their proofs above; no artificial `auto`-style catch-all is used.

## Known-unconverged functions

Tracked so they are not mistaken for done:

- `Questengine::LoadQuest` / `ParseToken` — bcc local-slot/temp allocation.
- `Weddings::Tick` — three dead `sete` blocks; markers match 57/57.
- Packets' `0x45ddf0`/`0x45ddfc`/`0x45de08`/`0x45de14` — eight byte-identical
  11-byte container accessors exist in the reference
  (`mov eax,[ebp+8]; mov eax,[eax+4|8]; ret`). We emit four named ones plus the
  `std::vector<T>::begin/end` COMDATs, and the sheet's per-unit greedy 1:1
  assignment cannot allocate every identical row; the four left over are COMDATs
  that appear as the still-stubbed owners use their containers, not hand-written
  functions. `GroundItemPtrVector_Count` (`0x44f8b0`) is the `(end - begin)`
  ptrdiff over 4-byte elements, i.e. a `T** - T**` subtraction.
- `Packets`: `Player_Warp` (1 instruction: temp construction order in the
  `do_leave` Avatar-Remove build); `Client_SendEncoded` is **CONVERGED**:
  `compare_asm.py` gives **359 ref / 359 our instructions, 0 mismatched** in the
  UNFLAGGED reading (frame `-0x88` exactly, 20/20 EH markers identical). The
  resolution was that `0x5432d8` is not a user function at all — it is the RTL
  `std::basic_string<char,...>::basic_string(const char*, const Allocator&)`
  constructor (the `cw32mt.lib` member whose surrounding index strings are
  `basic_string`/`char_traits`/`allocator`), and the allocator temp at
  `[ebp-0x80]` is the constructor's default `Allocator()` argument. The source is
  therefore direct construction, `std::basic_string<char> range(out.c_str());`
  (no `EOEncodedObj`, no default construction, no allocator temp slot of its own).
  Three further forms were required in the same function: the `>20000` log block
  is a bare `if` (no `else` — the then-block falls through to the send body), the
  guard is `if (!player->removing) { ... }` (this yields `jne <epilogue>` rather
  than `if (removing) return;`, which duplicated the parameter-cleanup epilogue),
  `out.Insert(family_byte, ...)` uses the implicit `char -> String` conversion
  (the explicit `String(family_byte)` pushes the ctor's `eax` instead of the
  stack temp's address), and the final argument is `*(char **)&out` (not
  `out.c_str()`, which emits an out-of-line call the reference inlines to the
  data-pointer load). Historical search log follows: (**partial, 219/359,
  frame now EXACT at `-0x88` (delta 0)**:
  the normal path is written — `String out` from `String((char)action)` + family +
  `data`, the range/encode block (`EOEncodedObj` + `std::vector<char> range`,
  `EO_ByteRange_FromString`, `EO_Encode_Interleave` with
  `FUN_0044f73c`/`FUN_0044f710`), the `+/-0x80` loop and the final
  `EO_EncodeNumber(...).Insert(...,1)`; a first-cut `>20000` **log path** is in
  place (stamp/`" "`/`" EndlServ "`/`IntToStr` pair/`"Too large encoded packet
  dropped: "`/`","`/`"error.log"` temporaries) but over-declares locals — frame is
  now `-160` vs the reference's `-136`, so it needs trimming to the spec's 25-slot
  map (`/tmp/sendencoded_spec.md`); the trailing `0x4728f8`/`Sock_Send` pair is not
  yet written;
  `EO_ByteRange_FromString` + `EOEncodedObj` + `FUN_0044f73c`/`FUN_0044f710` are
  declared in `src/Packets.h`. The frame surplus was the log path: the reference
  binds only **four** `String` locals there, not the spec's eleven — trimming to
  `stamp`/`tag`/`msg`/`path` made the frame match exactly. Residual: the EH
  counter/arming offsets are `-0x54`/`-0x60` in the reference vs `-0x48`/`-0x54`
  in ours. Offset sets: reference `0x4..0x30` (12 slots) then `0x40/44/48/4c/54/
  60/70/74/75/76/80/84/88`; ours `0x24..0x40` (8 slots) then
  `0x48/50/54/58/5c/64/68/6c/70/78/7c/7d/84/88` — the reference keeps 12 locals in
  the `0x4..0x30` band (the log-path Strings) where ours keeps 8 in `0x24..0x40`,
  so the band placement (not the total) is the residual. Rebuilding the log path
  with the spec's 11 Strings overshot the frame to `-0xa4` (28 B too big), so the
  log path really has ~4 locals, not 11 — the spec's §1 count is wrong; only the
  band placement (locals declared so they land at `-0x4`..) is still open.
  **13-defect audit applied**: `EO_Decode_Deinterleave` args swapped
  (`FUN_0044f710` then `FUN_0044f73c`), `player->client_encryption_multiple`
  (`+0x20`) not `server_encryption_multiple`, `void *obj` no longer zero-init,
  `(char)0x80` in the strip loop, case `0x1c` accumulator `""`, count width 1,
  `Client_SendEncoded(PacketAction(5), PacketFamily(0x1a))`, case `0x1c`/`0x33`
  sub-guards folded into `if (family == N && action == 1)`, case `0x33`
  `SubString(1,2)`, `Client_SendEncoded(PacketAction(3), PacketFamily(0x33))`,
  and the invented `player->flush_queue = 1` removed. Frame `-0xac` -> `-0xa4`.
  **Exact log path applied** (`DateToStr(Now())` + `" "` + `TimeToStr(Now())` +
  `" EndlServ "` + `"Too large encoded packet dropped: " + IntToStr(action) +
  "," + IntToStr(family)` + `"\n"`, then `fopen("error.log","a")`/`fprintf("%s")`/
  `fclose`), plus `if (player->removing) return;`, the trailing
  `FUN_004728f8(server, out.Length()); Sock_Send(player->socket, out.c_str());`,
  the `EO_Encode_Interleave` arg order (`FUN_0044f710` then `FUN_0044f73c`) and
  `int c = (unsigned char)out[i];`. Result: **379 our / 359 ref, 312 mismatched**
  (was 334), but the frame overshot to `-0xa0` vs the reference's `-0x88`
  (24 B too big) — the exact chain's temporaries exceed the reference's, so the
  layout is still not exact. **Link-defect fixed**: `src/Packets.h`'s
  `Client_SendEncoded` declaration widened `unsigned char` -> `PacketAction`/
  `PacketFamily` (and `#include "Protocol.h"` added to `Packets.h`, which did not
  have it); the 35 call sites and the definition now share the single symbol
  `@@Client_SendEncoded$qp6Serverp6Player12PacketAction12PacketFamily...`.
  **Signature unified on `unsigned char`** (the `ucuc` form, per the sheet: the
  enum-typed declaration cost 10 byte-exact callers): the definition now matches
  the declaration and the build emits ONE symbol
  `@@Client_SendEncoded$qp6Serverp6Playerucuc...`; sheet stays 1678/1809.
  **`range` re-typed to a POD** (`struct EOByteRange { void *f0, *f4; char *data;
  void *fc, *f10, *f14; }`, 24 B, no ctor) and `out[i] += (char)0x80` folded:
  frame `-0xa0` -> **`-0x88` (delta 0)**, instructions 355/359, mismatches
  **312 -> 245**, aligned prefix 5. Residual: the EH counter/arming sit at
  `-0x44`/`-0x50` (ours) vs `-0x54`/`-0x60` (ref) — a 16 B band-placement
  difference still. `Server_BroadcastToParty` has the same defect class (`ucuc`
  declaration vs `ii` definition) with a second `ucuc` declaration in another
  unit's `.cpp` was fixed in this unit's two declarations (`src/Packets.h` and
  `src/Packets.cpp` now both `unsigned char action, unsigned char family`), giving
  ONE `ucuc` symbol and no byte-exact loss. **The five container COMDATs are
  already present** in the build (`FUN_0044f6c8`/`710`/`73c`/`778`/`814`), so
  item 2's concern does not apply. **`range` re-typed to `std::basic_string<char>`**
  (the reference has no `vector<char>`; `0x44f6c8/710/73c` are its dtor/begin/end
  over a 12-byte `__string_ref` header, and `0x5432d8` is the rep-initialiser):
  `EO_ByteRange_FromString(std::basic_string<char>*, ...)`, with
  `(char *)range.begin()` / `(char *)range.end()` replacing the `FUN_0044f7xx`
  calls. Frame stayed exact `-0x88`, but instructions 355 -> 373, mismatches
  **245 -> 224**, aligned prefix **5 -> 19**; first residual is the log String at
  `-4` (ref) vs `-0x24` (ours), i.e. the log locals' declaration order. The
  **12-byte `EOByteRange { void*, void*, std::basic_string<char> }` wrapper was
  tried and overshot the frame to `-0x90`** (8 B too big, 253 mismatched), so the
  bare 4-byte `std::basic_string<char> range;` (frame `-0x88`, 224 mismatched,
  prefix 19) is retained; `#include <string>` added to `Packets.h` and
  `FUN_0044f6ec(&obj)` replaced by plain `&obj` with `EOEncodedObj obj;`. The
  remaining step is the log locals' **declaration order** (first log `String` at
  `-0x04` in the reference vs `-0x24` in ours; no `/tmp` logorder note exists yet),
  frame must stay `-0x88`. **Applying the logorder note's steps regressed**: hoisting
  `String msg;` to function scope and adding `char action_byte`/`char family_byte`
  moved the frame to `-0x90` (8 B over) and mismatches 224 -> 369, so it was
  reverted to the frame-exact form (bare `std::basic_string<char> range`, `msg`
  declared inside the log `if`, 224 mismatched, prefix 19). The band order
  therefore cannot be fixed by declaration placement alone in this form. The
  follow-up hypothesis ("move the whole log `if` block before the `out`
  declaration") is **already satisfied** — that is the current source order — yet
  the band is still reversed, so bcc assigns the block-scoped `msg` after the
  function-scoped `out` regardless. Remaining named candidates: the per-`Insert`
  temp count and the `Now()`/formatter stack handling. The real log sequence
  (formatters/joins/append trio) is still approximated.
  Blocked on the
  `EO_ByteRange_FromString` RTL helper ABI). `EO_Encode_Interleave` is
  byte-exact (`std::stack<char>`/`std::deque<char>`, by-value `String` return);
  `Player_HandlePacket` (`0x41794c`, 52,589 instructions, 33.9% of app bytes) is
  **prologue written** (220 instructions): header parse, the `+/-0x80` loop,
  `EO_ByteRange_FromString(&range, data.c_str(), (EOEncodedObj*)FUN_0044f6ec(&obj))`,
  `data = EO_Decode_Deinterleave(server, player->server_encryption_multiple,
  FUN_0044f73c(&range), FUN_0044f710(&range))`, `action`/`family` via
  `EO_DecodeByte`, `size` via `EO_DecodeNumber(String(data[3]))`, `size -=
  player->sequence`, `data.Delete(1,3)`, and the 3-slot scan
  `server->ping_history[i] == size` (offset `+0xa8`; the prep's `character_slots`
  name is `ping_history` in our header). The two `(*MAINFORM)->field_370/field_36c`
  stores are written: `extern TGUI **MAINFORM;` was hoisted into
  `src/Mainform.h` (beside `extern PACKAGE TGUI *GUI;`) and the four ad-hoc
  per-unit declarations removed (`Players.cpp`, `Packets.cpp`, `Npccontrol.cpp`,
  `Mapcontrol.cpp`); all four units still compile. `FUN_00472944` (the server
  byte-accounting helper) is now **byte-exact** (23/23) and its stub row removed
  (148 `_Stub` left). **Case `0x33` is written** (`if (family == 0x33)`: `action !=
  1`/`!logged_in`/`data.Length() < 2` guards, `EO_DecodeNumber(server,
  data.SubString(2,1))` -> `player->player_id` match, `Players::Players_GetById`,
  `Player_SerializePaperdoll` -> `Client_SendEncoded(PacketFamily(0x33),
  PacketAction(3), s)`, `player->flush_queue = 1`); `Player_SerializePaperdoll`
  declared locally. 391 instructions written, frame `-0x8c` vs the reference's
  split `-0x1f48`, aligned prefix 3/391 (the split frame gates alignment). Cases
  **Case `0x1c` is written** (`if (family == 0x1c)`: `action != 1`/`!logged_in`/
  `data.Length() < 3` guards, `String names = " "`, the loop
  `EO_DecodeNumber(server, data.SubString(i+3,1))` -> `*NpcRange_Lookup(NULL,
  server, player, id)` with the `Length() > 0` insert, `cnt < 1 -> false`,
  `EO_EncodeNumber(server, cnt, 2)` inserted at 1, `Client_SendEncoded(
  PacketFamily(0x1c), PacketAction(5), names)`); `NpcRange_Lookup` declared
  locally. 591 instructions written, frame `-0xac`, aligned prefix 3/591.
  **Case `0xd` (Shop) is written**: one `if (family == PacketFamily_Shop)` with
  four chained action sub-blocks `Create`/`Buy`/`Sell`/`Open`; `Create` reads the
  four `ShopValues::GetCraftIngredient1..4` records, validates held amounts with
  `Players::Players_GetItemAmount`, removes each present ingredient and appends
  `EO_EncodeNumber` pairs to the reply; `Buy`/`Sell` clamp `amount > 100` to a
  `Banned::AddBan` (1200s) and price via `ShopValues::GetBuyPrice`/`GetSellPrice`;
  `Open` resolves the npc tile with the Mapcontrol `FUN_0047c6c0`/`FUN_0047c634`
  pair, checks `NpcValues::GetType == NpcType_Shop`, stores
  `player->session_token` and sends `ShopValues::BuildOpenData`. Every guard and
  builder argument uses the `src/Protocol.h` enums. Frame `-0x190`
  (delta `+0xe6c`, best stack-search `+0x38`) vs the reference's split `-0x1f48`;
  aligned prefix 3/2111 (the split frame plus the twelve unwritten earlier cases
  gate alignment). Structural diff of the case body alone is 1492/1565 instructions
  exact; the residual is EH scope-marker values and one register mirror. All four
  `Client_SendEncoded` sites still gate final acceptance. Note the reference's
  return polarity: not-logged-in, short length and the `>100` ban return `false`,
  but a session-token/shop-id mismatch, an unknown ingredient or an unaffordable
  price return `true` (the packet is consumed and the socket kept). Cases
  `0xb`, `0xc`, `0x4` not started.
  Frame `-0x7c` vs the reference's split `-4092`/`-3912` (true `-0x1f48`), aligned
  prefix 3/220. Chunks `0x33`/`0x1c` not started. Earlier note: the header-parse prefix is written (`FUN_00472944(server,
  data.Length())`; `data.Length() < 4 -> false`; `packet_count++`/`sequence++`
  with the `> 9` wrap; the `+/-0x80` byte loop over `data[i]` for `i = 1..Length()`).
  Current frame `-0x34` vs the reference's split `-4092`/`-3912` (true `-0x1f48`),
  aligned prefix 3/87 — the split frame needs the 8,008 bytes of locals, so
  nothing aligns until the family chain exists. `/tmp/handlepacket_prep.md` has the 187-instruction
  prologue (frame `0x1f48`, split `add esp,-4092`/`push eax`/`add esp,-3912`; 13
  locals; header parse `data.Length() < 4` guard, the `+/-0x80` byte loop,
  `EO_ByteRange_FromString`/`EO_Decode_Deinterleave`, `EO_DecodeByte` x2,
  `EO_DecodeNumber`, `data.Delete(1,3)`, the `character_slots` search,
  `(*MAINFORM)->field_370/field_36c` stores) and chunk 1 (family `0x19`,
  `0x42a507`, 62 instructions, 0 TODOs: `Refresh_BuildReply` +
  `Client_SendEncoded`). Scoring needs `--frame-wild`/`--stack-search` because of
  the split frame.
  `NpcRange_Lookup` is **byte-exact (290/290)** — the pointer-return spelling was
  a different calling convention; changing the declaration and definition to a
  **by-value `String` return** (tail `return fragment;`, callers updated, case
  `0x1c` now `String name = NpcRange_Lookup(server, player, id);`) closed the
  missing `arm 0x80` and the hidden-return-pointer sequence. (Historical: it was
  at 22 mismatched (286/290) — the arming-form fix
  (`Npc **iter;` declared bare, `for (iter = …)` inside the `try`) removed the
  spurious loop-entry arm.) `Server_BuildOnlineNames` stays at **13 mismatched** (the
  `try`/`catch` form made it worse, 45, and was reverted).
  `EO_Decode_Deinterleave` is **byte-exact (238/238)** — RTTI `0x472853`
  (`queue<char,deque<char,allocator<char> > >`) confirmed the third container as a
  `std::queue<char>` and frame is exact (`-0x164`, delta 0). The residual was the
  merge body: the reference reads the `front()` value once into `c`, calls
  `woven.pop()` immediately, and computes `v`/`value` from `c` (not a second
  `front()`), *and* the source carries a **second post-loop drain** of the stack
  into `packet_buffer`. Both were needed before the whole `238/238` aligned and the
  function scored byte-exact. (Historical: it sat at 118 mismatched (221/281) with
  the drain/merge boundary as the residual, and the frame/band item was the
  missing post-loop drain.)
  `Walk_Execute`, `Spell_Execute`, the reply builders, and
  `Player_HandlePacket` (deferred: one 226 KB function).
- `Mapcontrol` `FUN_00482834` (`0x482834`) and `Mapcontrol_LoadMap` (`0x484e28`)
  — the loader. Matches the reference instruction-for-instruction through index
  775 and beyond: the whole fixed 46-byte EMF header plus section 1 (the NPC
  section). Sections `0x48353e`, `0x483751`, `0x483b10` (inner `0x483bfd`) and
  `0x483f9b` (inner `0x484088`) remain; all 56 decode/delete blocks are
  extracted as a spec (every decode is `0x486c38` `Pub_DecodeNumber_Map`, every
  section delete is the RTL helper `0x55949c`). Byte-exactness is gated on the
  local set being complete: the EH arming word/counter sit at
  `[ebp-0x160]`/`[ebp-0x154]` in the reference but at `[ebp-0xf8]`/`[ebp-0xec]`
  until every section temporary (out to `-0x1f0`) exists, so displacements
  reconcile only at the end.
  **The NPC-section local is `NpcValue`** (`src/Npcvalue.h`, 0x7c bytes), filled
  by `NpcValue npc_value = NpcValues::GetNpc(npc_values, id);` —
  `FUN_004a8858` is `NpcValues::GetNpc` (declared `src/Npcvalues.h:110`), and the
  by-value return is why its arg list looks like `(L, table, id)`. Other
  `FUN_...` sightings resolve to `NpcPtrVector_Count` (0x4810a0),
  `Npc::Npc` (0x47a868), `Map_AddNpc` (0x4844c0) and `Map_NpcIter_End`
  (0x45ddfc) — all already reconstructed.
  Pinned forms: `local_c` before the `try`; header/section blocks are single
  statements with anonymous temps (`dest = Pub_DecodeNumber_Map(map_control,
  map_buf.SubString(s, n))`), not named `tN` locals; `file_handle`/`size`/`buf`/
  `count` at function scope outside the `try`; the loop index stays in the `for`
  head;   `map->start_map` is `unsigned short`; `map_buf.Delete(1, 0x2e)`. **CORRECTION to an
  earlier note here: `SubString` and `Delete` use the SAME register convention —
  `ecx = len`, `edx = start` — they are not reverses.** Two independent
  investigations established it from the bytes (`data.SubString(1, 2)` emits
  `mov ecx,2; mov edx,1`, and `Delete(1, len)` emits `ecx=len, edx=1`), and the
  earlier "the reverse of `SubString`" claim is wrong. The practical consequence:
  write both in natural `(start, len)` source order, and when diffing, an
  `ecx`/`edx` pair swapped relative to that is a genuine argument-order bug — 60
  such `SubString` calls were found reversed in the two Mapcontrol loaders. And
  **`Npc`'s
  ctor takes its index as `int`, not `short`** — the call site stores the
  argument with a 4-byte `mov dword ptr [slot],eax`, where a `short` truncates.
  (`src/Npc.h`/`src/Npc.cpp` were changed together; the ctor body stays
  byte-exact at 115/115 and the mangled name becomes `@@Npc@$bctr$qissssis`.)
- `NpcControl_Tick` (`0x4ae45c`) — CONVERGED (1583/1583); listed here only for
  the facts it pinned. The broadcaster block is wrapped in
  `try { ... } catch (...) { }` (scope-marker stream 39/39 identical); the frame
  must be `-236`, which requires ONE shared `Player **player` iterator across the
  broadcaster and respawn loops; the target-selection branch is
  `if (target != NULL) { <party> } else { <player_targets> }`; the scan loop
  assigns `distance = d; target = *it;`; the party loop tests `member != NULL &&
  member->map_id == target->map_id` before calling `Npc_GetDistance`;
  `Player::map_id` (+0xdc) is an `int` (no `(short)` cast); the broadcast guard is
  `4 <= x`; the non-respawn path is an early `return`; and the chase condition is
  written `if (11 < distance) continue;` (operand order and polarity are
  observable). Its stored range end `0x4afc0c` truncates the epilogue; the real
  end is `0x4afc17`.
- `Mapcontrol` `FUN_00482834` — the BODY IS COMPLETE (sections 1-5 written, 1500
  of 1506 instructions; `Mapcontrol_LoadMap` is still a stub). The residual is a
  set of missing one-instruction **re-arms of the enclosing cleanup scope
  `0x2c`** at `0x483cb5`, `0x483ed1`, `0x484140` and ~3 more. Proven by inserting
  a bare arming instruction at each suspected point: after the `code` decode the
  divergence moves 1108 -> 1227, and adding the inner-loop back-edge re-arm moves
  it 1227 -> 1346, so each gap is exactly one marker and the stream is otherwise
  in phase. Our descriptor already emits no-op terminator entries of the same
  shape as the reference's, so this is a cleanup-table topological difference;
  revisit once the whole frame exists. Section-5 layout pinned:
  `lock_key = dec(SubString(2,7))` (slot `-0x1f0`) and
  `Mapcontrol_AddWarp(map_control, map, spec, tile_x, dec(2,2), dec(1,6),
  dec(1,4), dec(1,5))`.
- Packets `EO_Encode_Interleave` (`0x470ef0`) — **CONVERGED (210/210)**. Listed
  here for the facts it pinned. ABI: application code over `std::stack<char>` /
  `std::deque<char>` (the RTTI names are embedded in the code region — `0x471358`
  is `stack<char,deque<char,allocator<char> > > *`, `0x4713f4` is
  `deque<char,allocator<char> > *`); adding `<deque>`/`<stack>` and using those
  types materialises ~60 helper COMDATs, which alone moved the sheet from 1607 to
  1669 byte-exact. **Signature is `String EO_Encode_Interleave(Server *server,
  int multiple, char *begin, char *end)` — a BY-VALUE `String` return (hidden
  result at `[ebp+8]`, epilogue is the result clause `arm 0x50; result = data;
  arm 0x5c; push eax; dtor data; pop eax; arm 0x50; inc`), not a `String *`
  out-parameter.** Frame `-284`. Body: `try { ... } catch (...) { len = 0; }`,
  `for (; begin != end; begin++)` driving the parameter itself (not a copy),
  `char c = *begin; int value = (unsigned char)c;` (zero-extend before `idiv`),
  push onto the stack when `value % multiple == 0` else drain the stack into the
  deque and `push_back`, one post-loop drain, then a final loop that emits
  **front and back alternately** (`if (!woven.empty()) { buffer[len++] =
  woven.front(); woven.pop_front(); } if (!woven.empty()) { buffer[len++] =
  woven.back(); woven.pop_back(); }`), then
  `String data(server->packet_buffer, len); return data;`. Its stored byte count
  is short — the real end is `0x4711fd`.
- Packets `EO_Decode_Deinterleave` (`0x472414`) — **CONVERGED (238/238)**. Listed
  for the facts it pinned. Same container set as `EO_Encode_Interleave`: a
  `std::stack<char>` (`0x471318`), two `deque<char,allocator<char> >` bases
  (`0x471388`, RTTI `0x4713f4`) and a `std::queue<char>` (`0x472810`, RTTI
  `0x472853` `queue<char,deque<char,allocator<char> > >`); scope word
  `[ebp-0x138]`. Three merge-body shapes mattered: the `front()` value is read
  once (`char c = woven.front(); woven.pop();` immediately, then
  `unsigned char v = c; int value = v;` — the second read is the local `c`, **not**
  a second `front()` call), and the source has **two** drains of the stack into
  `packet_buffer`: the one inside the non-divisible `else` and a second one after
  the merge loop (flushing the trailing divisible bytes). The stored byte count is
  short — the real end is `0x472790` (the tsv `end` `0x47277a` is mid-instruction).
- `MysqlCallback_Dispatch` (`0x450618`, 36,735 B) — reconnaissance map. 15
  top-level cases for query kinds `0x40,0x42,0x44,0x45,0x47,0x48,0x49,0x4b,0x4d,
  0x4e,0x4f,0x50,0x51,0x52,0x53`, reached by a linear `if` chain (no jump table);
  58 distinct callees over 1,610 call sites; frame `0x7bc`. The prologue resolves
  `Players_GetById(server->players, query_result[1])`, NULL-checks it and matches
  `+0x10` against `query_result[2]`; each case re-reads the request packet
  (`PacketReader_Init` + `EO_GetBreakByte`), runs `Mysql_SubmitQuery_FromCallback`
  / `Mysql_ExecDirect_FromCallback` plus `Db_GetString`/`Db_GetInt`/
  `Db_SanitizeString`, packs a reply with `EO_EncodeNumber`, sends it through
  `Client_SendEncoded`, then jumps to the shared epilogue. **CORRECTION to an
  earlier note here: the DB string locals are plain `AnsiString`, not Rogue-Wave
  `std::string`.** The `0x559308`/`0x40240c`/`0x40243c` cluster resolves through
  `build/GameServer.map` to `LStrClr(AnsiString&)` with the length read at
  `Data-4`; the `std::string` reading was wrong. Also note the `0x58b60c`
  singleton with a virtual call at vtable `+0x14c` (unknown class), and that the
  stored range end `0x459597` truncates case `0x53` and the epilogue (true end
  `0x45961e`).
- Packets volume reconnaissance (frames, dispatch arms, callees and risks, so the
  transcription passes do not rediscover them):
  - `Attack_Execute` (`0x467980`) — **byte-exact (2984/2984)**, true end
    `0x46a990` (the stored `0x46a93f` truncates the epilogue at the `ret`; the
    `0x46a990..0x46a9a0` bytes are a data constant and `Math_Abs` starts at
    `0x46a9a0`). Frame `-0x1c4` exact; all 93 EH scope markers identical. 2 dispatch
    arms over `action == 10` (else `return 0`) covering 8 regions with two nested
    loop pairs (player sweep `0x467da9`/`0x468844`, NPC sweep
    `0x468897`/`0x46a87a`); 55 distinct callees over 428 sites; **no `std::string`**
    (AnsiString throughout); no embedded stubs. The `--frame-wild --strict-operands`
    reading before the fix was **10 hunks** (2984 ref / 2980 our instructions,
    aligned prefix 171): the whole instruction-count gap was **four missing stores**
    — `offset_x = caster->x;` / `offset_y = caster->y;` are emitted **twice** at both
    the declaration (`0x467c79`/`0x467c7f`, `0x467c97`/`0x467c9d`) and the
    PK-branch reset (`0x468853`/`0x468859`, `0x468868`/`0x46886e`). bcc duplicates
    the store only for a **chained self-assignment**, so the source is
    `int offset_x = offset_x = caster->x;` (and the reset spelling); the six
    remaining hunks were register-allocation mirrors downstream of the misalignment
    and resolved with it. Transcribe the BINARY, not the Rust: `attack.rs` uses a
    cooldown of `< 48`, but the binary requires `elapsed >= 0x2c` (44) at
    `0x467b0d`. **`disasm.txt` desynchronises at `0x46797f`** — regenerate that
    range from the image before trusting it. Pinned forms: both damage calls target
    `Combat_CalcArmorPen` (`0x4b12a8`); `element_resistances` is `short[7]` at
    `Player+0x160` (target table `[2,1,6,3,4,5]`); the crit is
    `damage += damage / 2`; the bounded damage block sits inside
    `if (RandRange(100) < hit_rate)`. Only residual strict-operand label:
    `0x46a083` calls the folded EH thunk `0x44f58c` (our `MapCoord::MapCoord` vs the
    reference's aliased `std::allocator<PlayerInventory>` COMDAT name) — same
    address, name-only.
  - `Spell_Execute` (`0x46a9b0`) — **byte-exact, converged.** Frame `-0x2c0`,
    true end `0x46ee3c` (the stored `0x46edd9` truncates the tail), all 150 EH
    scope markers match. 6 dispatch arms for actions `1, 30, 31, 33, 10`
    (fall-through `return 0`; Rust names Request/TargetSelf/TargetOther/
    TargetGroup/Use); 54 callees over 597 sites; **no `std::string`**; no
    embedded stubs. Final: **4079 ref / 4079 our instructions, 0 mismatched**,
    aligned prefix 4079 (unflagged and `--strict-operands`). The 32 "floor"
    hunks from the previous pass were **not** floor — they were one structural
    error plus its cascades:
    - The NPC `skill_type==1` damage body (from the `chase_target_id` guard
      through the not-dead `reply`) is wrapped in a positive
      `if (skill_type == 1 && type_info.type > 0 && type_info.type < 6) { ... }`
      with a trailing `return 1;` inside the loop — not the negative
      `if (skill_type != 1 || type <= 0 || type >= 6) return 1;` we had. The
      wrapper is the reference's source shape: it makes bcc emit the far shared
      `return 1` tail at `0x46dce2` and, crucially, changes the **scope** the
      allocator sees, which flips the `-156`/`-160` swap (the
      `Refresh_BuildReply` temporary vs the not-dead `reply`). 23 hunks + the
      2 chase-guard hunks collapsed at once.
    - The Player-branch `map_type` guard is likewise the positive
      `if (map_type == 3) { ... }` with its body falling through to the
      `skill_type==1` block's `return 1` (no inner trailing `return`; the
      reference's `0x46c2b7` shared tail). Inlining the negative guard had also
      made the pkt destructor save/restore `eax`.
    - The NPC arm's terminal `return 0;` is absent — the arm falls through to
      the `PacketAction_TargetGroup` check (the reference's shared tail).
    Fixed earlier in the same convergence: declaring the damage `pkt` in the
    `skill_type==1` scope (slot `-112` -> `-104`; hunks 64 -> 32),
    `(unsigned short)(*iter)->boss > 0` at both `boss` guards, and
    `(unsigned short)...->child_npc_id` (`movzx`, not `movsx`). Arm-1 callees
    remain `Player_HasSpellId` `0x40cc80`, `Server_BroadcastNearby` `0x463f34`,
    `EO_EncodeNumber` `0x470b9c`, `EO_DecodeNumber` `0x470de8`,
    `SkillValues::GetCastTime` `0x4a5268`; `Now`/`DateTimeToTimeStamp` are the
    library `0x520500`/`0x51ff7c`. Only residual `--strict-operands` label:
    `0x46c437` calls the folded EH thunk `0x44f58c` (our `MapCoord::MapCoord` vs
    the reference's aliased `std::allocator<PlayerInventory>` COMDAT name) — same
    address, name-only.
- `Packets`: `Player_HandlePacket` (`0x41794c`) — **byte-exact, converged.**
  `analysis/target/functions.tsv` ends the row at `0x0044ef54`, but the body
  continues to `0x44f58b`; the next real function is the folded EH thunk at
  `0x44f58c`. Comparing against the stored end truncates the tail and reports a
  positional cascade, not real errors. Use
  `compare_asm.py build/Packets.asm Player_HandlePacket
  '@@Player_HandlePacket$qp6Serverp6Player17System@AnsiString' 0x41794c 0x44f58c`.
  Final state: **52,584 ref / 52,584 our instructions, 0 mismatched**, prologue
  frame `-0xffc` on both sides (true local frame `-0x1f48`), aligned prefix
  **52,584**. The EH cleanup table is **structurally identical** to the reference
  (1,372 entries, 16,472 B, zero `prev`/`flags`/`extra` differences): the Guild
  `PacketAction_Accept` branch passes its concatenation directly to
  `Client_SendEncoded` (no named `msg`), and `PacketAction_Request`'s
  `tag[1]`/`name[1]` comparison uses two named `String` locals.

  The last 17 hunks (aligned prefix 28,765, frame and stack map already exact)
  were the five `weight_current` adjust sites: the `-=` at `L4009` and the four
  trade-loop `player/target->weight_current = field ∓ GetWeight(...) * amount`
  assignments at `L4810`/`L4814`/`L4832`/`L4836`. The reference emits an
  extract-op-store sequence (`mov ecx,[edx+0x13c]; sub ecx,eax;
  mov eax,[player]; mov [eax+0x13c],ecx`), not bcc's in-place
  `sub [edx+0x13c],eax` fold. **The trigger is the product's type**: an
  `int * int` product lets bcc's `ModifyAssign` peephole fold the statement,
  while an **unsigned** product (`int - unsigned`) defeats it. Writing the
  multiplier's second operand as `(unsigned int)` reproduces the reference at all
  five sites (no frame or ECT change). Established with `tests/weight_probe.cpp`:
  every signed spelling (compound `-=`, expanded `= field - ...`, parentheses,
  reference alias, pointer/array/cast access, getter, swapped multiply order)
  folds; only `unsigned int amount` produces the reference's load/op/reload/store.
  Fixed this pass 17 hunks → 0 (prefix 28,765 → 52,584). `make verify` 496/496;
  `make track` 1794/1801 rows, 649,213/668,128 bytes. The 105 residual
  `--strict-operands` labels are a separate, pre-existing class (wrong direct
  callees: `begin`/`end` swaps, wrong vector element types, and the folded-thunk
  name alias) and do not affect the canonicalized row.

### Not reachable from source — do not spend sessions on these

Two residues are proven to be bcc internals rather than source-form differences.
They are recorded here so no future pass mistakes them for work:

- **`Party_ShareExp`** (`0x46690c`, 13 mismatched, 1198 B). Every *mnemonic*
  matches in order; the only differences are register/operand choices, and the
  reference loops are **not** structurally different (the earlier claim that they
  receive different allocations is wrong): both reference loops are
  *index-first* — loop1 `i→edx, player→ecx`, loop2 `i→eax, player→edx` — while
  both of ours are *base-first* — loop1 `player→edx, i→ecx`, loop2
  `player→eax, i→edx`. The same one-register shift appears in loop2's
  `member`/`player` comparisons and in `member->experience += exp` (reference
  loads the loop-derived value first; we load `player`/`exp` first). So a single
  whole-function allocator shift flips all 13, not a per-loop spelling.
  - **Driver is loop2's String code, as register pressure — and the function
    sits exactly ONE statement over the threshold.** Deleting the whole
    `if (levelup > 0)` stats block *or* the three `pkt.Insert` calls flips both
    loops to the reference pattern. Correcting an earlier note here: deleting a
    *single* insert flips it too — verified for the third `pkt.Insert` and for
    the fourth `stats.Insert`, either one on its own. Seven `X.Insert(
    EO_EncodeNumber(...), X.Length() + 1)` statements are one too many; six give
    the reference's order. The reference has all seven, so the shift is a bcc
    artifact, not a missing/extra statement.
  - **It is not the stack layout.** The flip tracks the statement count, not the
    slot: `member` at `[ebp-100]` with six inserts is reference-order, while
    `[ebp-96]` with seven is ours, and padding the frame with extra `int` locals
    (member down to `[ebp-104]`) keeps our order. Frame size, slot assignment and
    the EH marker stream are byte-identical to the reference either way. It is
    also not a per-*file* counter: inserting another String-using function
    immediately before `Party_ShareExp` changes nothing, and neither does source
    line layout (collapsing the wrapped statements onto single lines).
  - **bcc is source-order-sensitive in loop1 and saturated in loop2.** Swapping
    the operands of loop1's `member->map_id == player->map_id` *does* swap the
    emitted load order (and breaks the match, 13 -> 15). The same swap in loop2's
    two comparisons changes nothing at all. That is the cleanest evidence that
    the loop2 sites are not a spelling problem: the compiler has stopped
    following the source there. It also gives a cheap probe for any future
    attempt -- if loop1 stops responding to an operand swap, the state changed.
  - **No compiler flag reaches it.** Beyond the flags already in CFLAGS:
    `-3 -4 -5 -6`, `-r-`, `-O -Oc -Ov`, `-Vx -Ve -Vb -Vv`, `-b-`, `-K`, `-Ff`,
    `-Z`, `-vi-` all leave the 13 untouched (`-vi` makes it far worse, which is
    the expected confirmation that `-v`'s inline suppression is required).
  - **Probe matrix (~180 variants, all byte-preserving) leaves the 13 unchanged:**
    accumulate forms (`+=`, `= x + y`, `= y + x`), comparison operand swaps,
    `&&`-wrapper vs split `if` vs `continue`, positive guards, `for`/`while`/
    `do-while`, literal vs macro bound, `i++`/`++i`, `members++`/
    `members = members + 1`, lookup spellings (`*(i + arr)`, `i[arr]`,
    `(arr)[i]`), `String x = expr` vs `String x(expr)`, `bool`/`int` init forms,
    declaration-without-init + later assign, `register`/`const`/`volatile`,
    `sizeof` no-ops, unread locals at every scope, separate named locals, and
    `unsigned` field/element types. Only byte-*changing* edits flip the pattern
    (e.g. adding a local), and those break the byte match. This pass added:
    `String pkt("")` / `String stats(expr)` direct-init (identical or worse),
    `exp += exp / members` / `exp /= members` (much worse -- 298 mismatched, so
    the `exp = exp / members` spelling is pinned), and rewriting loop2's guard as
    a positive `&&` with the body nested (identical).
  - Frame, EH scope-marker stream, and ECT are byte-identical; `--strict-operands`
    is now clean (the else branch was corrected to call
    `Server_BroadcastToPartyOnMap` `0x463750`, the reference's callee, instead of
    an undefined `Server_BroadcastToMapExceptSelf`). Source order in the unit and
    the enclosing function order do not change it either.
- **The Mapcontrol loaders' re-arm marks** (`FUN_00482834` 1566/1572,
  `Mapcontrol_LoadMap` 1551/1552) — see the section above; the missing mark is the
  try-body scope terminator, emitted at any scale at a fixed offset, and its
   position cannot be moved by any tested source form because the try's opening
   position is pinned by the file-open failure path.

## Progress measurement — read the BYTES, not the function count

`make track` reports both. As of the artifact pass (above):
**1795/1796 (99.9%) of application functions are byte-exact, and
666,662/667,860 (99.8%) of application BYTES.** The single remaining
non-byte-exact row is `Party_ShareExp` (1,198 bytes, `mismatched`, a proven bcc
register-allocation floor; see below). Historical note: before
`Player_HandlePacket` (228,416 B, 33.9% of all application code) converged the
function percentage badly understated progress, so quote the byte figure.

Consequence for planning: with the artifact rows excluded, the only outstanding
row is `Party_ShareExp`; the historical backlog (the deferred
`Player_HandlePacket`, `MysqlCallback_Dispatch` 36,735 B, `Spell_Execute`
17,449 B, `Attack_Execute` 12,223 B, `Login_SendCharacterList` 9,419 B,
`Walk_Execute` 5,466 B, `Player_ApplyQuestActions` 5,458 B and the two parked
Mapcontrol loaders, 14,296 B combined) is converged.

### `Player_HandlePacket` (`0x41794c`) — investigated, and tractable

Reconnaissance result (recorded here because it decides the project's remaining
shape):

- **The stored size clips the function.** The true range is
  `0x41794c..0x44f58c` — **228,416 bytes, 52,589 instructions**, 8,704 call sites,
  2,213 branches, 45 back-edges. The stored end `0x44ef54` clips 1,592 B; the
  shared epilogue is at `0x44f583` and `Exception_InstallFrame` starts at
  `0x44f58c`.
- **Shape: a flat 40-case linear `if`/`else` chain on the packet family**
  (`cmp dword ptr [ebp-0x18b4], N; jne next`) — **no jump table**; a shared
  prologue parses the header into family `[ebp-0x18b4]` / action `[ebp-0x18b0]`,
  and a shared epilogue closes it.
- **~85% of it is genuine inline logic**, not delegation: the call/instruction
  ratio is 0.12–0.20 in every case, so only ~4.7 KB is call instructions and
  ~224 KB is code that must actually be transcribed. The work is proportional to
  the byte count.
- **267 distinct callees, 245 resolved**; all 22 unresolved are RTL
  `AnsiString`/EH helpers except `0x4cf070` (`Socket_GetRemoteIP`). **No project
  helper is missing.**
- **`AnsiString` throughout** (zero `basic_string` in the map's names), so no
  `std::string` hazard. No unidentified type is needed for the dispatch.
  `(*MAINFORM)` is used 103× with the now-named `TGUI` members
  (`item_values` 41, `npc_values` 30, `skill_values` 17, `game_control` 7,
  `myquery` 3).
- **Verdict: reconstructible by the same method that converged `NpcControl_Tick`;
  no structural blocker.** It decomposes into **43 independently verifiable
  chunks** (prologue, the 40 family cases, the final `else`, the epilogue); the
  smallest useful first chunk is family `0x19` (`0x42a507..0x42a607`, 62
  instructions, 256 B, 8 calls). The two dominant cases are family `0x12`
  (7,688 instructions, 34 KB) and `0x27` (6,825 instructions, 30 KB), each ~4–5×
  `NpcControl_Tick`. **The risk is scale, not structure** — the whole function is
  roughly 33× the largest unit completed so far.
- One open risk: the Mapcontrol no-op-re-arm artifact was **not** observed on a
  1,891-instruction slice, but that is not a clean bill — the artifact is only
  detectable per chunk with `compare_asm.py` once a body exists.
- **Family `0x13` (Warp, `0x42f868..0x430721`, 884 instr) is written** and scores
  878/884 (99.3%) structural. The six unmatched instructions are three register-
  order differences at each of the two `server->players->by_id[player->read_len]`
  sites (the reference evaluates the index before the base). The case body uses
  `Map_ReadRawFile`, `Client_SendRaw`, `Mapcontrol_GetByIndex`,
  `Mapcontrol_inc/dec_player_count`, `Player_SerializeAvatar`,
  `Refresh_BuildReply` and `Player_FireQuestTriggers`; it is inserted in the
  reference emission order between `Door` (`0x22`) and `Book` (`0x33`).

## HandlePacket / Client_SendEncoded: an emitted-COMDAT side effect worth knowing

Removing `std::vector<char> range;` from `Client_SendEncoded` was CORRECT -- the
reference emits no constructor for `range` (it is only passed to
0x5432d8/0x44f73c/0x44f710/0x44f6c8), which is why declaring it as a `std::vector`
added an `allocator<char>` temp and a 16-byte band shift. But it had a side effect
that cost one byte-exact row (1678 -> 1677): our source was materialising five
`std::vector<char>` member COMDATs that the reference DOES have --

  `0x44f6c8` (35 B, the scalar/deleting destructor), `0x44f710` (43 B),
  `0x44f73c` (60 B), `0x44f778` (155 B), `0x44f814` (155 B)

-- and with no `std::vector<char>` anywhere in `src/` (grep now finds none) they are
no longer emitted, so those rows read `stubbed`. This is the inverse of the
phenomenon that produced the project's biggest single jump (using
`std::stack<char>`/`std::deque<char>` materialised ~60 COMDATs at once): a container
declared for convenience keeps COMDATs alive that the reference may or may not have.

The correct resolution is NOT to re-add the vector to `Client_SendEncoded` (that
would re-introduce the allocator temp and the band shift), but to find where the
reference legitimately instantiates `std::vector<char>` in the Packets unit and use
it there. Candidates to check: the bodies of `EO_ByteRange_FromString` (0x5432d8) and
its helpers 0x543480, and any other Packets function that builds a byte buffer. Until
that is done, those five rows are the known cost of the layout fix, and they are
counted in `stubbed`.

Also outstanding from this stretch, both proven with evidence and both small:
- `Server_BroadcastToParty`: the DEFINITION is already right (`unsigned char`); the
  two DECLARATIONS are wrong (`src/Packets.h:119-120`, `src/Packets.cpp:39-40` say
  `int action, int family`). The reference's callee reads its own params with BYTE
  loads (463712 `mov al, byte ptr [ebp+0x14]`, 463716 `mov dl, byte ptr [ebp+0x10]`),
  so the ABI is 1-byte and the declarations must change.
- `MapVector_End`: the same class -- `src/Mapcontrol.cpp:29` declares
  `void *MapVector_End(void*)` (`qpv`) while `src/Packets.h:92`/`src/Packets.cpp:349`
  declare `MapContainer *MapVector_End(Mapcontrol *)` (`qp10Mapcontrol`); the
  reference (functions.tsv:299, 0x44f9b0) is `qp10Mapcontrol`, so Mapcontrol.cpp:29 is
  the divergent one.

## `--strict-operands`: the canonical comparison cannot see a wrong literal or callee

`compare_asm.py` canonicalizes addresses by design, so a **wrong string literal** and a
**wrong direct call target** both pass as `0 mismatched`. The sheet's "byte-exact" is
therefore *canonicalized* equality, not byte equality. `--strict-operands` closes that
gap: after the canonical pass it compares the value behind each address — a direct
`call` by **callee identity** (never by address), an immediate or `offset` landing in
`.data`/`.rdata`/`.bss` by the **literal bytes**, and a non-address immediate
numerically. Relocatable operands are deliberately not compared (a non-call `.text`
target, an absolute `[0x…]` slot); an unclassifiable operand is counted, not guessed.
The default path is unchanged.

It found real bugs immediately, all now fixed:
- **Eight swapped literals in `Login_SendCharacterList`** (`questblob`/`questblob2`,
  `invblob`/`invblob2`, `invblob3`/`invblob4`, `skillblob`/`skillblob2`). bcc evaluates
  a `+` chain **right-to-left**, so the first literal emitted is the RHS; our halves
  were reversed and the serialized blob came out reversed. A `+` chain and an Insert
  accumulator order their operands differently — that is the lever.
- **`Players_Add` called the wrong function**: `Settings::GetMaxConnections` where the
  reference calls `Settings::GetMaxClones` (`0x413f34`) at `0x4082a3`.
- Four wrong literals, each decoded from the image: `FUN_0047badc` `"N"` not `"x"`;
  `Newscontrol_Get` `""` not `"NULL"`; `Eventcontrol_Tick` `") seconds or be send to
  jail."` not `"…jail!"`; `Questengine::ParseToken` `")"` not `"}"`.
- `MysqlCallback_Dispatch` had three more, which is why it was not *truly* exact
  despite `7912/7912`: a closing literal `"',"` written as `"'"` (emitting
  `('TAG''name',…)`), and two `Mysql_ExecDirect` calls that must be
  `Mysql_ExecDirect_FromCallback` (`0x47703c`, not `0x476f54` — the Acquire/Resume
  variant, a self-deadlock), plus four `IntToStr` sites needing the `0x403010`
  overload (the out-of-line copy of the header's `inline AnsiString
  IntToStr(unsigned int)`), not `@Sysutils@IntToStr$qqri`.

**How to read the flag's remaining output.** It reports ~1100 further call-target
differences, and a triage established they are a TOOL ARTIFACT, not defects: the flag
compares our callee against `functions.tsv`'s `source_name`, and `track.py` assigns
that label by canonical shape — because canonicalization wildcards call targets, the
label is **arbitrary among identically-shaped reference functions**. The authoritative
check is the **callee's body**. Of 1140 sampled rows, 1110 had byte-identical callee
bodies and **zero were real wrong-container calls**. A wrong element type *would*
matter — ilink32 does **not** fold byte-identical COMDATs (a controlled link kept two
`vector<…>::size` instantiations at distinct addresses) — so the rule is: compare the
callee bodies with non-address immediates kept; equal → benign, different with a forced
caller pairing → real.

**Latent link failures found by that triage** (not container issues): `Chestcontrol.cpp`
declared `MapItemVector_Begin`/`_End` without defining them, and
`MapchestVector_Begin/End` and `MapwarpVector_Begin/End` were likewise undefined. The
reference callees are the owning containers' STL members — `std::vector<MapItem>` at
`0x47bcf4`/`0x47bd00`, `std::vector<MapChest>` at `0x47bcdc`/`0x47bce8`, and
`std::vector<MapWarp>` at `0x47b1d4`/`0x47b130` — and the callee bodies are byte-identical
to the reference. Resolved by calling the real members at the call sites
(`map->chest_list.begin()/.end()`, `chest->slots.begin()/.end()`,
`map_iter->arena_spawn_list.begin()/.end()`) and deleting the six declarations.
`Players_Iter_Begin`/`_End` (the same class of out-of-line `std::vector<Player*>::begin/end`
COMDAT) remain declared without a definition in five units and still need the Players
owner to resolve them.

## Risks and mitigations

| Risk | Mitigation |
| --- | --- |
| VCL/BDE codegen cannot be reproduced | Use unmodified `ref/Borland5` libs and headers; verify per-section, not per-function, for library regions |
| Linker nondeterminism | Fix timestamp and compare sections; treat whole-file MD5 as the final gate |
| Resource/DFM mismatch | Reconstruct from the embedded payloads, byte-diff `.rsrc` |
| Scale (65 units, 1.7 MB image) | Unit inventory + per-unit status; smallest-falsifiable-test discipline |
| EH pervasive in TASM | C++-first; use TASM only for EH-free functions or last-resort fidelity |
| Source-form ambiguity | Match codegen per function; record the form in the header |
