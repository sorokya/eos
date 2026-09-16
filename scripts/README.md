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
- **`compare_pe.py REF CANDIDATE [--ignore-timestamp]`** — whole-file hash,
  header-field diffs, per-section geometry/hash diff, and the first differing
  byte offset. Exits non-zero on any difference.
- **`normalize_pe.py PE [--timestamp V] [--characteristics V] [-o OUT]`** — the
  documented deterministic post-link step: rewrite the volatile `TimeDateStamp`
  (at `e_lfanew + 8`) and optionally the COFF characteristics word.
- **`disasm.sh [PE] [OUT]`** — linear Intel-syntax disassembly of `.text` via
  host `objdump`.

## Make targets

```sh
make image     # build the docker/Wine image
make analyze   # extract reference metadata into analysis/target/
make disasm    # linear .text disassembly into analysis/target/disasm.txt
make sanity    # compile+link a minimal VCL+BDE app to validate the toolchain
make compare   # compare build/GameServer.exe against the reference
make normalize # apply the deterministic timestamp step
make clean     # remove build/
```
