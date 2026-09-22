# scripts/

Harness for the GameServer decompilation. Everything is dependency-free on the
host (Python standard library + `objdump`); all Borland tools run under Wine in
the Docker image.

## Runner

- **`borland.sh '<command>'`** — run a command inside the container with the
  Borland tree mounted at the path its `.cfg` files expect and the repository at
  `/work`. Exports:
  - `$B` — Borland root with spaces (`C:\Program Files (x86)\Borland\CBuilder5`),
    used to invoke tools (their DLLs live beside the `.exe`).
  - `$BZ` — the same tree via a space-free wine path (`Z:\borland`). Use it for
    `-L`, object and library arguments; `ilink32` mishandles spaces on its
    command line.

  ```sh
  scripts/borland.sh 'wine "$B\Bin\bcc32.exe" -c -obuild/x.obj build/x.cpp'
  ```

## PE tooling

All PE parsing uses `pe.py`, a small standard-library reader (headers, sections,
imports, exports, resource tree). No `pip` packages required.

- **`extract_target.py PE -o DIR`** — write `headers.txt`, `sections.txt`,
  `imports.txt`, `exports.txt`, `resources.txt`, `metadata.json`, and the raw
  `dfm/*.dfm` payloads.
- **`units.py PE`** — derive the translation-unit table from the exported
  `@@Unit@Initialize`/`@Finalize` pairs (order, code-end addresses, module
  refcount addresses) into `analysis/target/units.tsv`.
- **`unitmap.py`** — attribute code addresses to modules, three modes:
  - `--map FILE` parses an `ilink32 -s` map (`MAP=1 scripts/build.sh`) and names
    the object or library member behind every range. This is how the link layout
    was established: explicit objects are contiguous in command-line order, and a
    library's pulled members are laid out where its `.lib` sits in the link line
    — inline in the OBJFILES field places them there, while listing it after the
    comma appends them at the end (see `layoutdiff.py` and AGENTS.md).
  - `--pe FILE --stubs --units FILE [--functions FILE] [--libs DIR]` segments a
    PE at every module initializer stub, including modules `units.py` cannot see
    (no `@@Unit@Initialize` export), and classifies each unnamed module by
    byte-matching its functions against the Borland libraries (`--libs`, default
    `ref/Borland5/Lib`): `library` if it matches a lib member, `unknown`
    otherwise. Writes `analysis/target/modules.tsv`.
  - `--units FILE --functions FILE [--modules FILE]` splits the reference `.text`
    into per-unit function inventories (`analysis/target/unit_functions.tsv`),
    excluding the `library`/`unknown` module ranges listed in `--modules` so a
    unit's span cannot absorb a neighbouring module.
- **`compare_pe.py REF CANDIDATE [--ignore-timestamp] [--struct]`** — whole-file
  hash, header-field diffs, per-section geometry/hash diff, and the first
  differing byte offset; `--struct` adds imports/exports/relocations diffs. Exits
  non-zero on any difference.
- **`layoutdiff.py [--ref modules.tsv] [--map map] [--tol N] [--strict]`** — the
  reference module layout (`analysis/target/modules.tsv`) vs our ilink32 map:
  per-unit start-RVA delta, size delta and order inversions, plus the size and
  position of the contiguous library blocks (the interleave points). Requires
  `MAP=1 scripts/build.sh`. `--strict` exits non-zero on any inversion or a start
  delta above `--tol`; without it the report is informational. A reference unit
  span that absorbs library code (e.g. `Banned`, whose first ~58 KB is the RTL
  `System` member) is annotated and excluded from the comparison.
- **`tdsinfo.py [TDS] [--seg N] [--image-base 0x400000]`** — read the module map
  out of the Borland TD32/TDS debug file ilink32 writes with `-v`. The `.tds` is
  CodeView-4 style (signature `FB0A` = C++Builder): a 4-byte signature, a
  32-bit directory offset, a directory of `(type, module_index, offset, size)`
  entries (`0x120` Module, `0x130` Names), and per module a set of code/data/TLS
  segments `(seg_index, flags, offset, length)`. It gives the same per-module
  layout as `ilink32 -s` but is **written in full** (the Wine fault truncates the
  `-s` map), and it names library modules by source unit (`DB.pas`,
  `Controls.pas`). The RTL (`cp32mt.lib`) members carry no debug info, so they do
  not appear here; `--seg 1` prints the `_TEXT` (code) layout with VAs.
- **`libcompare.py [--libs-only|--objs-only] [--only-unmatched] [--threshold R]`**
  — for every CODE module in the ilink map, take its bytes from our `.text` and
  locate them in the reference `.text`, wildcarding absolute addresses and rel32
  call/jmp/jcc operands. It answers "do we pull a module the reference does not",
  which is the practical form of the `.text` surplus question: a module whose
  bytes have no home in the reference is one we pull and the reference doesn't.
  The per-module `ratio` is computed over the whole member, so it is only
  meaningful when the member has no internal insertion; treat low ratios on
  large units as an alignment artifact, not divergence. Requires
  `MAP=1 scripts/build.sh`.
- **`objfuncs.py --tdump FILE [--ref GameServer.exe]`** — the per-function form
  of `libcompare`: parse a `tdump` listing of one `.obj`, extract every `_TEXT`
  segment's name and bytes (the 16-byte hex group is fixed width, so slice before
  the ASCII column), wildcard relocation placeholders and rel32/absolute
  operands, and locate each function in the reference `.text`. A function with no
  home in the reference is one the unit emits and the reference does not. On
  `Npcvalue` it reports found=87, absent=0: every emitted function exists in the
  reference, so the unit's +216 B span delta is distributional (COMDAT
  placement), not extra code. Diagnostic, not a gate.
- **`compare_functions.py FUNCTIONS_TSV REF CANDIDATE [--mask-reloc] [--list N]`**
  — per-function byte comparison using the Ghidra inventory; masks base-relocation
  words when layouts are not yet identical.
- **`funcdiff.py [--ref] [--linked] [--inventory] [--unit U] [--function F]`**
  — the whole-tree byte differential (`make funcdiff`). Masks only what the
  *layout* moves — the four bytes of every base relocation and the 4-byte
  displacement of a direct `call`/`jmp`/`jcc` — then asks whether each reference
  function's remaining bytes appear anywhere in our image. It exists because
  `make verify` / `compare_asm.py` canonicalise branch targets, so a function
  whose relative branch lands elsewhere still reads `byte-exact`; this is what
  found the `Weddings_Tick` inverted `jne`/`je`, `ParseToken`'s missing early
  `return`, `Jukeboxcontrol_TryPlayTrack`'s misplaced `break` and
  `Player_EvaluateQuestRules`'s `continue` chain. Three masking traps it avoids:
  objdump's whole-image pass desynchronises on data in `.text` (so each function
  is decoded from its own start — capstone when importable, else per-function
  `objdump`), a relocation is a 4-byte slot but `pe.relocations()` reports only
  its first byte (each slot is expanded), and Ghidra's `end` can truncate a final
  `e8` displacement (decode 8 bytes past it). A function that matches only in a
  different module is reported as *placed elsewhere* (byte-identical, layout
  only). Exits non-zero when any reference function differs, so it can gate a
  build.
- **`tdsfuncs.py [TDS] [--seg-base A]`** — *our* linked function inventory
  (`make tdsfuncs`), read out of `build/GameServer.tds`: name, virtual address
  and exact COMDAT length for every linked function, template/RTL COMDATs
  included. The reference is stripped but the rebuild is not, and that asymmetry
  is what makes a rebuild→reference differential possible at all. Reads the TD32
  `AlignSym` subsections (`S_GPROC32`/`S_LPROC32`); Delphi modules emit the same
  records with empty names, which are kept because the address and length are
  what the byte differential needs. (`tdsinfo.py` reads the module table from the
  same file.)
- **`comdatdiff.py [--module I] [--unit U] [--body VA:LEN]`** — the opposite
  direction from `funcdiff` (`make comdatdiff`): COMDATs *we* emit that the
  reference does not have in that module. bcc32 emits every function as a
  COMDAT and `ilink32` keeps the copy from the **first object that defines
  one**, so which translation unit odr-uses a template member is observable in
  the layout — and a hand-written helper with the same body as a container
  accessor is a redundant second copy of it. It groups `tdsfuncs.py`'s inventory
  by *masked* body (relocation slots and direct-branch displacements zeroed, so
  byte-identical instantiations of different types fall together) and compares
  masked occurrence counts per module against the reference. A count mismatch is
  an exact per-module byte surplus or deficit; the *identity* of a byte-identical
  body cannot be resolved by counting, so both sides' names and addresses are
  printed. `--body VA:LEN` lists every masked copy of one body in both images —
  that is how `Players_ActiveCount`/`vector<Player*>::size`,
  `Mysqlcontrols::Free`/`~Mysqlcontrols`, `Mapcontrol_GetCount`/
  `vector<ChestItem>::size` and the nine two-int pair constructors were each
  shown to be one function in the reference. Exits non-zero on any mismatch.
  See PLAN.md, "COMDAT ownership".
- **`typenames.py [--all] [-o TSV]`** — the RTTI type-name oracle
  (`make typenames`). bcc32 writes the **spelled** type name into every
  `__tpdsc__` and those records live inside `.text`, so the reference is stripped
  of symbols but not of its class names: every class, pointer, reference, array
  and container type the original named is readable verbatim, and a
  reconstructed name that differs is a byte difference *and* a size difference
  (the descriptor's length tracks the name's). Descriptors are found by requiring
  a base relocation exactly 4 bytes before the name (pointer/reference/`vector`
  forms) or 8 (class/array forms) plus a strict C++ type-name shape; runs under
  four characters are dropped as code noise. It reports the names each image has
  and the other does not — pair the two leftover lists and each pair is a class to
  rename. That is how `Logins::ReservedName`/`Logins::LoginEntry` were shown to be
  `Asocketvip`/`Asocketblock` (at namespace scope, no `Logins::` prefix) and
  `Server` to be `Packets`. See PLAN.md, "The RTTI type-descriptor name oracle".
- **`normalize_pe.py PE [--timestamp V] [--characteristics V] [-o OUT]`** — the
  documented deterministic post-link step: rewrite the volatile `TimeDateStamp`
  (at `e_lfanew + 8`) and optionally the COFF characteristics word.
- **`extract_res.py PE [-o OUT.res] [--dump DIR] [--types LIST]`** — reconstruct
  a Borland `.res` from the reference's embedded `.rsrc` (`make extract`): every
  entry is written with the exact type, ordinal/name and language, so the file
  reflects the reference resources. `--dump` also writes each payload plus a
  `.rc`. The `.res` entry format was recovered from `brcc32` output (numeric
  type/name is `WORD 0xFFFF` + ordinal; string names are NUL-terminated UTF-16;
  the first entry must have all-zero flags, or ilink32 reports "Unsupported
  16bit resource").
- **`inject_rsrc.py --ref REF --target TARGET`** — copy the reference's `.rsrc`
  raw payload over a rebuilt image's `.rsrc`. The linker merges library
  resources by absolute path and auto-generates `DVCLAL`, so its `.rsrc` cannot
  be driven from the application `.res`; this step reproduces the section
  byte-for-byte. It refuses unless the two `.rsrc` sections have identical RVA
  and raw size, which only holds once the rest of the image matches.
- **`disasm.sh [PE] [OUT]`** — linear Intel-syntax disassembly of `.text` via
  host `objdump`.

The Ghidra function inventory (`analysis/ghidra/functions.tsv`) is exported from
the read-only Ghidra project; see [AGENTS.md](../AGENTS.md) for how it is
produced.

## Build

- **`mkstubs.py`** — create a stub `src/<Unit>.cpp` for every unit in
  `units.tsv` (skips existing files). Ordinary units get
  `#pragma package(smart_init)`; `GUI` gets the main-unit form/global/WinMain
  shell.
- **`build.sh`** — compile every unit in `src/` (main unit first, then
  `units.tsv` order) and link `build/GameServer.exe`, then apply the timestamp
  normalization. `MAP=1` adds `ilink32 -s` and writes
  `build/GameServer_map.map` (the detailed segment map that `unitmap.py --map`,
  `libmatch.py` and `track.py` consume; the plain `build/GameServer.map` is the
  276-byte summary and carries no module rows).
  Environment knobs for link experiments — a full build is ~4 min, a relink
  ~12 s, so use these when only the link line is under test:
  - `LINK_ONLY=1` — skip the compiles and the `.rc`, reuse `build/obj`.
  - `VLIB=...` — the libraries searched *last* (default `import32.lib
    cp32mt.lib`). The VCL/BDE libraries are not here: they are listed **inline in
    the OBJFILES field** at the reference's interleave points (`vcldb50.lib`
    before `Itemchest`, `vcl50.lib`/`vclbde50.lib`/`vcle50.lib` before `Banned`),
    because `ilink32` lays a library's members out where its `.lib` appears. The
    VCL libraries must come first and `cw32mt.lib` must never appear — see
    AGENTS.md for both.
  - `LPATH=...` — the `-L` search path (`Lib\Debug` must precede
    `Lib\Release`; swapping them drops ~85 KB of `.text` and is wrong).
  - `HEADOBJ=...` — the object(s) listed before the units, i.e. between
    `c0w32.obj` and `Mainform.obj` (default: `Lib\Obj\sysinit.obj`).
- **`build_asm.sh`** — emit a bcc32 `-S` listing for every unit into
  `build/<Unit>.asm` (one container run). Incremental: a unit is recompiled only
  when its `.cpp` or any header is newer than its listing (`FORCE=1` to rebuild
  all). Compiles run in parallel; `JOBS=N` overrides the container's CPU count.
  Input to `verify_units.py`.

## Make targets

```sh
make image     # build the docker/Wine image
make analyze   # extract reference metadata + unit table into analysis/target/
make units     # rebuild the unit table only
make unitmap   # attribute reference code to units + list module boundaries
make functions # per-function byte score vs the Ghidra inventory
make struct    # structural diff (imports/exports/relocations)
make disasm    # linear .text disassembly into analysis/target/
make sanity    # compile+link a minimal VCL+BDE app to validate the toolchain
make stubs     # generate stub units for units not yet reconstructed
make build     # build build/GameServer.exe from src/
make unit UNIT=Serial     # compile one reconstructed unit
make unit-asm UNIT=Serial # emit bcc32 assembly for one unit
make unitmap   # re-segment modules + rebuild the per-unit function inventory
make track     # regenerate analysis/target/functions.tsv + the README status block
               #   (run `MAP=1 scripts/build.sh` first for exact library classification)
make verify    # emit stale asm + score every source function vs the reference
               #   JOBS=N parallel compiles/objdump (default: CPU count); incremental
make case-selftest # self-test the per-case comparison locator (no build required)
make format    # apply the project clang-format style to src/*.cpp, src/*.h
make format-check # check the style without modifying files
make compare   # compare build/GameServer.exe against the reference
make normalize # apply the deterministic timestamp step
make clean     # remove build/
```

## Function-level diff

- **`compare_asm.py ASM FUNCTION MANGLED_PREFIX REF_START REF_END`** — diff one
  bcc32 `-S` function against a reference address range, canonicalizing relocated
  addresses and branch targets while requiring registers, stack offsets and small
  constants to match exactly. The per-function byte-fidelity loop. `--lines`
  annotates the diff with the source lines from our listing and summarises the
  mismatches per line; a function absent from the listing is reported and the
  tool exits 2 (the `--lines` parser tolerates both `name proc` and
  `name$q... proc` label forms).

  **The default comparison is address-canonicalized and therefore cannot see a
  wrong literal or a wrong direct callee.** A reference to `"',"` and one to
  `"'"` both render as `mov edx,ADDR`; a call to `Mysql_ExecDirect` and one to
  `Mysql_ExecDirect_FromCallback` both render as `call ADDR`. A function the
  sheet calls `byte-exact` is only *canonicalized*-exact, which is exactly the
  distinction the final MD5 will not forgive. `--strict-operands` re-reads the
  raw operands of the instruction pairs the canonical pass already accepted and
  compares the value behind each address:
  - a direct `call <addr>` target is compared by **callee identity**, never by
    address. An application function's identity is its Borland-mangled symbol
    from `analysis/target/functions.tsv` (matched exactly, or via the recovered
    `ref_name` for a renamed COMDAT accessor such as `Players_Iter_Begin`); a
    reference row still named `FUN_…` is left unclassified rather than guessed.
    A library/RTL target cannot be named from the stripped reference, so its
    identity is established by **consistency of the symbol's own call sites**
    (one `@@…` symbol can denote only one function): a symbol that aligns with
    two distinct reference bodies is reported, the majority alignment taken as
    its identity and the minority sites as the defects. Each is labelled with
    the Ghidra name of the reference address.
  - an immediate address that lands in a data section (`.data`/`.rdata`/`.bss`/
    `.tls`) is a string/data literal: the bytes at that address are compared.
  - an immediate address in `.text` that is not a call (an EH table, RTTI or
    vtable pointer) and an absolute memory operand (`[0x…]`) are relocated slots
    and are treated as layout-dependent (never flagged).
  - any other immediate is compared numerically (the canonical pass wildcards
    every value ≥ `0x10000`).
  An operand the tool cannot classify is **counted, not guessed**: it produces
  no mismatch, and the summary prints the calls/literals/immediates compared and
  the unclassified-operand count. Default output and exit status are unchanged
  when the flag is absent; with it, any operand difference makes the exit
  non-zero. This is how the `questblob`/`questblob2` literal swaps, the
  `GetMaxClones`/`GetMaxConnections` call, the `IntToStr`/`AnsiString_AppendInt`
  sites and the vector-element-type call differences were found inside functions
  the sheet calls `byte-exact`.

  `--frame-wild` is a **progress instrument** for a partially written function:
  bcc only sizes the frame correctly once every local exists, so until then the
  prologue's `add esp,-N` differs and every `[ebp-N]` local shifts, and the
  unflagged comparison reports "thousands mismatched" even when the written body
  matches. With `--frame-wild` the prologue frame instruction (and an
  identical-magnitude epilogue release, if the compiler uses one) is a wildcard
  on both sides; the actual sizes are printed as
  `frame: ref -0x1c4 ours -0xac (delta 0x118)` and the report adds
  `aligned prefix: K / N`. **Only the frame size is relaxed** — registers,
  non-frame stack offsets, small constants, operand order, instruction count and
  the mismatch count are all still exact, and the flag is ignored for acceptance:
  final acceptance remains the unflagged comparison plus the whole-file MD5.
  With the flag off the output is byte-identical to before (verified on every
  converged range); with it on, a converged range still scores 0 mismatched, so
  the flag can never invent a match it cannot justify.
  `--stack-wild` (which implies `--frame-wild`) is the companion progress mode
  for the stack *offsets*. Once the frames differ by `D`, every `[ebp-N]` local
  differs and a correct body scores as mismatched from the first frame-relative
  instruction onward. `--stack-wild` derives `D = |ref frame| - |our frame|`,
  rewrites our `[ebp-N]` to `[ebp-(N+D)]` before comparing, and prints
  `stack delta applied: +0x118 (our frame 0xac vs ref 0x1c4)` plus the distinct
  `[ebp-N]` slot sequence of both sides. **Ordering stays strict**: the shift is
  a fixed numeric offset applied per instruction, so a body that uses the same
  slots in a different order still mismatches (the shifted numbers differ), and
  the printed slot sequences expose the correspondence. `[ebp+N]` arguments and
  `[esp+N]` operands are **not** shifted, and registers, constants, operand
  order and instruction count stay exact. Because the frame delta includes the
  EH-frame overhead the *local-area* delta can differ from it, so `--frame-wild`
  also prints `implied stack delta: +0xe8 (retry with --stack-delta 0xe8)` — the
  delta the first divergent slot pair implies — `--stack-delta D` applies an
  explicit delta, and `--stack-search` removes the retry loop entirely: it sweeps
  the candidate deltas (`-0x400`..`+0x400` in 4-byte steps, plus the
  frame-derived and implied ones), scores each, and prints the best delta with
  its aligned prefix and mismatch count plus the runner-up deltas — then the
  normal reading for the winner. The search also states whether the win is
  **decisive** or a **tie** (how many deltas reach the same prefix, i.e. whether
  the commitment beyond it is in the bodies rather than the stack), prints the
  winner's **local-area delta** alongside the frame-derived one (the difference
  is the EH-frame overhead), and classifies the **first divergence** —
  `stack-offset` / `operand/register` / `opcode/structure` / `instruction count`
  — with both instructions.

  Condition-code aliases are folded before comparing, because bcc32 and objdump
  spell the same condition differently (`jge`/`jnl`, `jg`/`jnle`, `jl`/`jnge`,
  `ja`/`jnbe`, `jb`/`jnae`/`jc`, `jae`/`jnb`/`jnc`, `jbe`/`jna`, `je`/`jz`,
  `jne`/`jnz`, `js`/`jns`, and the `setcc`/`cmovcc` equivalents): 90 spellings
  collapse to one representative per condition class (48 classes). Only the
  mnemonic is folded — operands are untouched, and genuinely different conditions
  (`jge` vs `jnge`, `ja` vs `jb`, `je` vs `jne`) are never merged.

  Every tolerant mode is a **progress instrument, never an acceptance criterion**
  — final acceptance remains the unflagged comparison plus the whole-file MD5.
- **`compare_case.py ASM UNIT FUNC REF_START REF_END [--strict-operands]
  [--frame-wild] [--stack-delta D]`** — diff one *case* (an address-range slice)
  of a large function against the reference. It exists because some functions are
  built case by case: `Player_HandlePacket` is a single 226 KB function (one
  enormous `if`/`else` chain) and the whole-function `compare_asm.py` cannot align
  past its first missing case, so a per-case instrument is the only way to score
  one case at a time. `UNIT`/`FUNC` name the reference function in
  `analysis/target/functions.tsv`; the tool resolves its Borland-mangled
  `source_name` for the listing (a `FUNC` already starting with `@` is used
  verbatim).

  **Locating the slice is a first-class, reported, loud step.** The tool finds
  the slice by matching the first `--anchor-len` (default 6) canonicalized
  reference instructions against our function's instruction stream, and prints
  the anchor, the matched reference start address, our instruction index and
  source line, and the **number of candidate matches**. The anchor folds the
  function-global facts a partially reconstructed function cannot yet match:
  `[ebp-N]`/`[ebp+N]` offsets become `[ebp-SLOT]`, and an EH scope marker
  (`mov word ptr [ebp-N], imm`) has its value folded to `MARK` (the frame layout
  and the cleanup-table byte offset are function-global). A short anchor is
  ambiguous wherever a case-dispatch prologue repeats — the first six canonical
  instructions of `Player_HandlePacket` (`cmp [ebp-SLOT],18` / `jne` / ... )
  occur verbatim in several `if (action == PacketAction_List)` cases — so the
  tool **extends the anchor one reference instruction at a time, keeping only
  the candidates that continue to match, until exactly one remains** or the whole
  reference slice has been consumed. Extension is evidence, not a tie-break: the
  anchor returned is a longer *exact* sequence, so taking the first of several
  equally short matches is impossible. **It refuses (exit 2, explicit message)
  only when even the full reference slice matches zero or more than one place** —
  a wrong alignment that reports a plausible-looking mismatch count is the worst
  possible failure mode, so genuine repeats are still never silently resolved.
  The self-test (`--selftest`; `make case-selftest`) proves the fully-repeated
  refusal, the anchor extension that uniquifies a repeated short prefix, the
  perturbed-instruction report, the zero-mismatch correct slice, and that an
  in-function data region (a jump/exception table emitted as `dd`/`db` between
  real instructions) is not parsed as instructions.

  **The comparison reuses `compare_asm`'s canonicalization, alias folding and
  strict-operand machinery** (imported, not forked, so the two cannot drift
  apart). By default it keeps opcodes, registers, constants, operand order and
  instruction count exact while wildcarding the function-global `[ebp-N]` slot
  numbers and EH marker values; `--strict-operands` adds the same
  value-behind-the-address check as `compare_asm.py` (reusing its
  `StrictContext`/`strict_compare`), so a wrong literal or a wrong direct callee
  is still flagged. That default is a **progress instrument, never an acceptance
  criterion**, and is labelled as such in the output: a partial function's frame
  and cleanup offsets cannot match until the function is complete, and the slot
  wildcard means a reordered local within the case is not caught. Acceptance
  remains the unflagged whole-function `compare_asm.py` comparison and, finally,
  the whole-file MD5. EH markers are blanked for the strict pass because their
  value is intentionally wildcarded, so they are not reported as immediates.

  Passing `--frame-wild` or `--stack-delta D` turns the per-case slot wildcard
  off and reproduces `compare_asm.py`'s slot handling instead (frame wildcard /
  explicit `[ebp-N]` shift), so the two tools are interchangeable for those
  flags; EH marker *values* remain wildcarded in every mode, since a partial
  reconstruction cannot match a function-global cleanup-table offset. `--diff`
  prints the aligned diff, `--markers` the EH scope marker streams, and `--calls`
  the call symbols. The report ends with the instruction counts, the positional
  mismatch count, and a **structural** verdict — `structural: clean` when every
  alignment hunk is a same-length substitution, otherwise the number of
  insertion/deletion hunks:

  ```sh
  python3 scripts/compare_case.py build/Packets.asm Packets Player_HandlePacket \
      0x44e180 0x44f2c4
  python3 scripts/compare_case.py build/Packets.asm Packets Player_HandlePacket \
      0x44e180 0x44f2c4 --strict-operands
  ```
- **`verify_units.py [UNIT ...] [--ref-bin PATH] [--asm-dir DIR]`** — the
  whole-tree equivalent: for every unit's listing, match each function in the
  unit's namespace against that unit's reference ranges from
  `analysis/target/unit_functions.tsv`, printing per-unit matched/total and
  exiting non-zero on any unmatched source function. Unreferenced `$bdtr`
  COMDATs are counted separately (the linker drops them). `objdump` is invoked
  per range (a single whole-image sweep desynchronises on data embedded in
  `.text` and decodes wrong boundaries) and the calls run in parallel
  (`-j N`, default CPU count).
- **`libmatch.py [--list UNIT,...]`** — identify library members by matching
  reference functions against the *linked build*: the Borland libraries are the
  same code compiled by the same toolchain, so linking the project (with
  `MAP=1 scripts/build.sh`) yields a library block that can be searched directly,
  and the map names each module. Far more precise than the raw `.lib` blob
  (`build/GameServer_map.map` is required).
- **`track.py [--summary] [--readme FILE] [--reclassify]`** — the central
  per-function status sheet: `analysis/target/functions.tsv` (generated,
  gitignored) with one row per reference function and columns
  `unit order start end size ref_name kind source_name status note`.
  `kind` is `app` (must be written), `comdat` (compiler-emitted once the owning
  type is used), `library` (statically linked RTL/VCL/BDE, matched by a
  matching a library module of the linked build (`libmatch.py`; falls back to a
  reloc-masked signature over `ref/Borland5/Lib`), `stub` (a module
  initializer boundary), `merged` (the ret-less body half of a function
  Ghidra split after its prologue; the head row's source function already
  covers its bytes, so it is not a separate target), or `comdat_cross` (a
  compiler-emitted template COMDAT provably owned by a *different* translation
  unit than the module range containing the row; excluded from the denominator
  like `library`). Run `MAP=1
  scripts/build.sh` first for the exact classification; without it the fallback
  is used and the cache records which. `status` is `byte-exact` / `mismatched` /
  `unimplemented` / `deferred` / `n/a`, decided from the `build/<Unit>.asm`
  listings with the same canonicalisation as `verify_units.py`; each source
  function can satisfy only one reference function. `--readme` rewrites the
  generated block in README.md. The classification is cached in
  `analysis/target/function_kinds.tsv`; `--reclassify` recomputes it. A cached
  `comdat` whose name no longer matches the current `COMDAT_PAT` is re-derived
  on load, so a pattern change (dropping `^thunk_`, which trapped library
  forwarders as `comdat` before the library test) takes effect without a full
  reclassify. A row whose function is emitted by a *different* translation unit
  (the reference placed the surviving COMDAT copy in this unit's span) falls
  back to a global pool, but only when the canonical sequence has exactly one
  candidate there; otherwise the row is left unmatched and the ambiguity is
  reported (a wrong cross-unit match would hide real work).

  A final **artifact pass** (`_classify_artifacts`) excludes compiler/RTL
  rows that are not hand-written targets, each by a proof rather than an
  address list: (1) an RTL **data-sentinel accessor** — a leaf
  `mov eax,<data address>` getter whose data target is referenced only from
  outside the unit inventory (Borland's `basic_string::__getNullRep` /
  `__nullref`); (2) a **Borland-header local-static guard** — the
  `cmp byte ptr [flag],0` / `inc byte ptr [flag]` idiom whose folded canonical
  form matches a function the build emits from a file under the Borland
  `Include/` tree; (3) a **cross-unit compiler COMDAT** — a row whose every
  candidate is a `@@std@`/`@@__rwstd@` symbol, whose own unit emits no free
  candidate, and which is reached only from already-reproduced code. See
  PLAN.md "Tracker artifact exclusions" for the exact rows and proofs.

## Transcription aids

- **`asm2cpp.py --start ADDR --end ADDR [--selftest]`** — draft a C++ skeleton for
  a reference address range. It walks `analysis/target/disasm.txt` (or
  regenerates the range with `--objdump` when the listing has desynchronised),
  loads the field-offset comments from `src/*.h` and the function inventory from
  `analysis/target/functions.tsv`, and emits one address-commented line per
  instruction: recognised idioms (the `AnsiString` RTL ops, `EO_DecodeNumber`/
  `EO_EncodeNumber`/`EO_GetBreakByte`/`PacketReader_*`, EH scope markers and the
  temp counter, header-named field loads/stores, branch conditions) get a named
  draft line, everything else gets a `// TODO(0xADDR): <raw instruction>`.

  ```sh
  python3 scripts/asm2cpp.py --start 0x46ac32 --end 0x46dd4b --objdump -o /tmp/arm1f.cpp
  python3 scripts/asm2cpp.py --selftest
  ```

  **Type inference.** `--sig 'int F(Server *server, Player *player, int action)'`
  (or the `// Name(...)` / `// STUB(0xADDR, ...) ref:` comments already in
  `src/*.cpp`) seeds the x86 cdecl argument slots `[ebp+8+4k]`. The tool then
  tracks register copies within a basic block (invalidated at branch targets and
  calls) and resolves indirect operands against per-class field tables parsed
  from the `// +0xNN` comments in `src/*.h` — including the declared field types,
  so `mov edx,[ebp+0xc]; mov eax,[edx]` … chains type through pointer fields.
  A typed base yields `base->field_name`; an unknown offset on a typed base
  yields `base->field_0xNN`; an untyped base is never guessed. Call arguments
  are rendered from the pushes (`SomeFn(server, player->map_id)`) and `jcc` after
  a `cmp`/`test` becomes a real condition (`if (elapsed < 0x2c)`).

  **Condition folding (safety property).** A `cmp a, b` / `test a, b`
  immediately followed by a `jcc` is folded into a real C condition with the
  **true** relation. For `cmp a, b` the flags are those of `a - b` (Intel
  syntax: destination first), so `jl/jle/jg/jge` and the unsigned
  `jb/jbe/ja/jae` families both mean the corresponding `a <op> b` (the CPU
  signedness does not change the C operator); `je/jne` are `==`/`!=`; the
  negated spellings (`jnge`, `jnae`, …) are *aliases for the same condition*,
  not negations. `test a, b` sets flags from `a & b`, so only the zero/sign
  forms are emitted — `a == 0`/`a != 0` for the `test a, a` self-test,
  `(a & b) == 0` / `!= 0` / `< 0` / `>= 0` otherwise — and every other `jcc`
  after `test` is left undetermined. Operands are rendered through the typing
  and field-resolution machinery.

  **Operand identity.** Parameters and locals are rendered distinctly: a
  positive `[ebp+N]` argument slot is named by the **declared parameter name**
  when the signature is known (`--sig` or the `// STUB(…) - ref:` comments),
  else `arg_0xN`; a negative `[ebp-N]` local is always `local_0xN`. Two
  different storage locations can therefore never render to the same
  identifier (`selfcheck()` asserts this). A field name is attached only to a
  **typed** base register (`base->field_name`, from the per-class tables built
  from the `// +0xNN` comments); an untyped base renders only the proven byte
  offset (`base->field_0xNN`), never a class name. An argument list is emitted
  only when its **arity is known and the pushes agree** (from the stub
  signatures, header prototypes, and the `__fastcall`/`__thiscall` register
  conventions); a convention or arity mismatch, or a duplicate register
  consumed by a later argument, yields
  `// TODO(addr): argument count not determined` with no argument list.

  **The tool emits a condition only when it can prove it.** If the flag-setting
  instruction is not the immediately preceding one, is an `fcom`/`fnstsw`/`sahf`
  x87 predicate, the `jcc` relation is undetermined, or either operand cannot be
  rendered, it writes `// TODO(addr): condition not determined` and **no `if`**.
  An absent `if` is safe; an inverted one is not.

  **Its output is a draft, never an authority.** It never fabricates a constant,
  offset or argument: anything it cannot trace is a TODO comment. Every line
  must be verified against the reference with `compare_asm.py` (and the whole
  function with `make unit`/`make verify`) before it is committed.
  `--selftest` runs it over three already-converged ranges
  (`EO_Encode_Interleave`, `Walk_BuildReply`, `Player_SerializePaperdoll`),
  prints the classification rate, the idiom breakdown, and the `arity_todo`,
  `holes`, `slots`, `arity_mismatch`, `arity_contradicted` and `untyped_field`
  counters, plus every
  emitted condition;
  it aborts (exit 3) if any conditional carries an unknown relation/operand, two
  storage locations collide, or a rendered argument list contradicts the
  reconciled arity. All three classify at 100%. Operand identity was audited against the
  converged `src/` on six ranges (`EO_Encode_Interleave`, `Walk_BuildReply`,
  `Player_SerializePaperdoll`, `NpcControl_Tick`, `Refresh_BuildReply`,
  `Players_Add`) with zero slot collisions and zero unsafe conditions.

  **Arity inference.** The callee-arity table is built from every available
  source, most authoritative first: (a) the
  `// STUB(0xADDR, N bytes) Name - ref: RET Name(TYPE a, TYPE b, ...)` comments
  in `src/*.cpp` (a full C++ arity), (b) prototypes and definitions in `src/`,
  (c) the `functions.tsv` name mapped onto (b). A prototype's arity is
  reconciled with the Borland calling convention — `__cdecl` pushes all
  arguments, `__fastcall` passes the first two in `ecx`/`edx`, `__thiscall`
  passes the object in `ecx`, and neither register argument is pushed by the
  caller — so a call is rendered only when the declared arity and the observed
  pushes agree under the convention. Variadic (`...`) signatures have no
  known pushed count and always fall back to a TODO. The push stack excludes
  the prologue's callee-saved register saves (`push ebp/ebx/esi/edi`), the
  hidden return-buffer pointer of a `String`-returning call (the `push eax`
  after a default `AnsiString()` construction), and the pushes consumed by an
  argument-taking RTL helper; the RTL helpers that take their operand in
  `eax`/`edx` (default ctor, `Length`, `c_str`, copy ctor, `~AnsiString`,
  `__InitExceptBlockLDTC`) are transparent to the stack.

  **Desync fallback.** The stored listing is a linear sweep of the whole image,
  so a range whose head follows embedded data can start mid-instruction. The
  tool checks each range's own raw-byte column (an instruction at A with N bytes
  must be followed by A+N, and the range must begin at its requested start) and,
  when it disagrees, regenerates the range from `GameServer.exe` with `objdump`
  and says so on stderr. This fixes the documented `Players_Add` head at
  `0x4081c8` (the listing began at `0x4081ca` with a spurious one-byte
  instruction); it never fires on the three selftest ranges.

  **Container-iterator typing.** The out-of-line iterator/record accessors
  (`Players_Iter_Begin`/`_End`, `Map_NpcIter_Begin`/`_End`, the `*Vector_Begin`/
  `_End` and `*PtrVector_*` families) return an element pointer, and their
  element class is derived programmatically — from an explicit `T **` return in
  a `src/` prototype, from the `// STUB` reference signatures, or from the
  `std::vector<E>` member the accessor iterates (matched by the longest element
  class name contained in the accessor's own name). The element type is
  propagated through the standard copy tracking: an accessor call types its
  `eax` return, a store to a local records the local's pointer type, a load
  restores it, `mov reg,[reg2]` on a `T **` base yields `T *`, and an iterator
  advance (`add reg,4`/`inc reg`) preserves the type. A `T **` base has no
  field at any offset, so it never produces a name; an absolute address
  (`[0x5…]`) and an untyped base likewise never do. `untyped_field` counts the
  emitted `reg->field_0xNN` operands an iterator type could not name — the
  iterators cut it by more than half across the converged ranges
  (`Walk_BuildReply` 32→14, `Refresh_BuildReply` 129→34, `NpcControl_Tick`
  358→53, the `Attack_Execute` head 97→54) with no wrong name or object.

  **Push-window and esp reconciliation.** The argument stack is closed across
  `push imm; call …; pop` helper sequences: a helper that takes its operand on
  the stack (`operator new`/`operator delete`) consumes the push window instead
  of leaking its size into the next call, while the RTL helpers that take their
  operand in `eax`/`edx` (`Now`, `DateTimeToTimeStamp`, the `AnsiString` ctors/
  dtors/ops) stay transparent. After the push window closes, the rendered count
  is reconciled against the caller's own cleanup — `add esp, N`, a run of
  `pop reg`, or the shared epilogue — minus the hidden return dword when the
  callee returns a `String` by value. The esp adjust is the callee's actual
  consumed argument bytes, so when the rendered count and the stack disagree the
  tool emits `// TODO(addr): arity not determined (rendered N, stack says M)` and
  **no argument list**. Argument expressions are **frozen at push time**, so two
  pushes of the same register at different points print distinct expressions
  (`Server_InViewRing(server, (player)->x, (player)->y, (*ecx)->x, (*eax)->y)`).

  **Safety properties, stated explicitly:** a condition is emitted only when it
  is provable; a field NAME is emitted only when the base's class is proven; an
  argument list is emitted only when the declared arity, the pushes and the
  caller's esp adjust all agree; a storage location is rendered distinctly from every other; a
  field access is emitted only for a typed base; an argument list is emitted
  only when the declared arity and the observed pushes agree under the calling
  convention — otherwise a TODO, never a guess.
