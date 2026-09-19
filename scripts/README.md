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
    was established: explicit objects are contiguous in command-line order and
    library members follow.
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
- **`compare_functions.py FUNCTIONS_TSV REF CANDIDATE [--mask-reloc] [--list N]`**
  — per-function byte comparison using the Ghidra inventory; masks base-relocation
  words when layouts are not yet identical.
- **`normalize_pe.py PE [--timestamp V] [--characteristics V] [-o OUT]`** — the
  documented deterministic post-link step: rewrite the volatile `TimeDateStamp`
  (at `e_lfanew + 8`) and optionally the COFF characteristics word.
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
  normalization. `MAP=1` adds `ilink32 -s` and writes `build/GameServer.map`
  (the detailed segment map that `unitmap.py --map` consumes).
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
  constants to match exactly. The per-function byte-fidelity loop.
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
  (`build/GameServer.map` is required).
- **`track.py [--summary] [--readme FILE] [--reclassify]`** — the central
  per-function status sheet: `analysis/target/functions.tsv` (generated,
  gitignored) with one row per reference function and columns
  `unit order start end size ref_name kind source_name status note`.
  `kind` is `app` (must be written), `comdat` (compiler-emitted once the owning
  type is used), `library` (statically linked RTL/VCL/BDE, matched by a
  matching a library module of the linked build (`libmatch.py`; falls back to a
  reloc-masked signature over `ref/Borland5/Lib`), or `stub` (a module
  initializer boundary). Run `MAP=1 scripts/build.sh` first for the exact
  classification; without it the fallback is used and the cache records which. `status` is `byte-exact` / `mismatched` /
  `unimplemented` / `deferred` / `n/a`, decided from the `build/<Unit>.asm`
  listings with the same canonicalisation as `verify_units.py`; each source
  function can satisfy only one reference function. `--readme` rewrites the
  generated block in README.md. The classification is cached in
  `analysis/target/function_kinds.tsv`; `--reclassify` recomputes it.

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

  **Its output is a draft, never an authority.** It never fabricates a constant,
  offset or argument: anything it cannot trace is a TODO comment. Every line
  must be verified against the reference with `compare_asm.py` (and the whole
  function with `make unit`/`make verify`) before it is committed.
  `--selftest` runs it over three already-converged ranges
  (`EO_Encode_Interleave`, `Walk_BuildReply`, `Player_SerializePaperdoll`) and
  prints the classification rate and the idiom breakdown.
