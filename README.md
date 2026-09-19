# GameServer.exe — Byte-Exact Decompilation

Reconstruct original-style C++ source for `GameServer.exe` and rebuild it with the
Borland C++ 5.5 toolchain so that the linked executable is **byte-for-byte identical**
to the reference binary:

```
MD5 (GameServer.exe) = 075fb5db1d6369bf488cd2ad0c715ddd
```

The reference is a 32-bit Windows GUI application built with **Borland C++Builder 5**
(statically linked VCL, no runtime Borland packages). Work proceeds **TASM-first**:
every machine-code region is first reproduced as reassemblable Turbo Assembler source
and proven against the reference, then lifted, unit by unit, into C++ translation
units that recompile to the same bytes.

The detailed roadmap lives in [PLAN.md](PLAN.md). Working instructions for
contributors and AI agents live in [AGENTS.md](AGENTS.md).

## Target fingerprint

| Property              | Value                                                              |
| --------------------- | ----------------------------------------------------------------- |
| File                  | `GameServer.exe`                                                  |
| Size                  | 1,733,120 bytes                                                   |
| MD5                   | `075fb5db1d6369bf488cd2ad0c715ddd`                                |
| Format                | PE32, `coff-i386`, GUI subsystem                                  |
| Compiler              | Borland C++ 5.5 (`bcc32.exe`), 32-bit, static                     |
| Linker                | Turbo Incremental Link 5.00 (`ilink32.exe`)                       |
| Assembler             | Turbo Assembler 5.3 (`tasm32.exe`)                                |
| Resource compiler     | Borland Resource Compiler 5.40 (`brcc32.exe`)                     |
| Entry point           | `0x00401000`                                                      |
| Image base            | `0x00400000`                                                      |
| Sections              | `.text .data .tls .rdata .idata .edata .rsrc .reloc`              |
| File/section align    | `0x200` / `0x1000`                                                |
| Header characteristic | `0x010e`                                                          |
| Build timestamp       | `0x50CBB124` (2012-12-14 23:07:16 UTC)                            |
| Imports               | 8 DLLs, 413 functions (see below)                                 |
| Exports               | 133 (65 unit `Initialize`/`Finalize` pairs, `_GUI`, etc.)         |

Imported DLLs: `ADVAPI32.DLL`, `COMCTL32.DLL`, `GDI32.DLL`, `KERNEL32.DLL`,
`OLE32.DLL`, `OLEAUT32.DLL`, `USER32.DLL`, `WSOCK32.DLL`. No Borland runtime DLLs
(`rtl50`, `vcl50`, `cc3250`) are imported, confirming the RTL and VCL are linked
statically rather than through runtime packages.

The executable is a TCP game server: the main form `TGUI` ("Endless Advanced Server")
hosts a `TServerSocket`, a BDE `TSession`/`TDatabase`/`TQuery` stack targeting MySQL
(alias `endless_database`, database `endless_db`), a `TTimer`, and a
`TApplicationEvents`. Two additional forms exist: `TLoginDialog` ("Database Login")
and `TPasswordDialog` ("Enter password"). The compiler emitted per-unit
module initializers/finalizers as exports, which gives a complete inventory of the
original translation units (see [PLAN.md](PLAN.md#translation-unit-inventory)).

## Repository layout

```
.
├── README.md                     # this file
├── AGENTS.md                     # agent/contributor workflow and build commands
├── PLAN.md                       # phased decompilation plan and inventory
├── Makefile                      # build driver (image, analyze, disasm, sanity, ...)
├── GameServer.exe                # reference target (untracked, gitignored)
├── GameServer.exe.md5            # expected hash (tracked)
├── docker/
│   └── Dockerfile                # Wine + Borland runner image
├── scripts/                      # container wrapper and PE tooling (see scripts/README.md)
├── tests/                        # harness fixtures (minimal VCL+BDE link check)
└── ref/
    ├── Borland5/                 # Borland C++Builder 5 toolchain (gitignored)
    ├── Borland_C++_Version_5_Programmers_Guide_1997.pdf
    └── calling_conventions.pdf   # Agner Fog, calling conventions and name mangling
```

Reconstruction sources (`src/`, `asm/`, `forms/`, `res/`) are added as the plan
progresses; the intended structure is described in
[PLAN.md](PLAN.md#proposed-repository-layout). `build/` and `analysis/` are
generated and gitignored.

## Requirements

- Docker (the Borland tools are Windows binaries and run under Wine in the image).
- The provided `ref/Borland5` toolchain tree and `GameServer.exe` reference binary.
- No native Borland install is needed; everything runs in the container.

## Quickstart

Build the runner image once:

```sh
docker build -t gameserver-borland-wine docker
```

Run any Borland tool through the container wrapper, which mounts the toolchain at
the absolute path its `.cfg` files expect
(`C:\Program Files (x86)\Borland\CBuilder5`) and the repository at `/work`. It
exports `$B` for tool paths and `$BZ`, a space-free path for `ilink32` arguments:

```sh
scripts/borland.sh 'wine "$B\Bin\bcc32.exe" -c -obuild/unit.obj tests/vcl_link.cpp'
```

Verify a build result against the reference:

```sh
md5 -q build/GameServer.exe        # must equal GameServer.exe.md5
```

The mount path is significant: `bcc32.cfg` and `ilink32.cfg` reference
`C:\Program Files (x86)\Borland\CBuilder5`, so mounting the toolchain anywhere else
requires passing `-I`/`-L` explicitly. `ilink32` also mishandles spaces on its
command line, which is why the wrapper provides the space-free `$BZ` path.

Convenience targets cover the harness (`scripts/README.md` has the details):

```sh
make image     # build the docker/Wine image
make analyze   # extract reference metadata and DFMs into analysis/target/
make disasm    # linear .text disassembly into analysis/target/
make sanity    # compile+link a minimal VCL+BDE app to validate the toolchain
make compare   # compare build/GameServer.exe against the reference
make normalize # apply the deterministic timestamp step
```

See [AGENTS.md](AGENTS.md) for the full command reference and link configuration.

## References

- `ref/Borland_C++_Version_5_Programmers_Guide_1997.pdf` — language, compiler and
  linker behavior for the exact toolchain version in use.
- `ref/calling_conventions.pdf` (Agner Fog) — 32-bit register/stack conventions,
  Borland name mangling (section 8.2), object-file formats, and exception/stack
  unwinding rules needed to express optimized code as TASM and C++.
- `ref/Borland5/` — compiler, assembler, linker, resource compiler, runtime (`Lib/`),
  headers (`Include/`), and the RTL/C++Builder sources including the startup module
  `Source/RTL/source/startup/c0nt.asm`.

## Status

Reconstruction is C++-first (TASM as a surgical fallback). The figures below are
generated by `scripts/track.py` from `analysis/target/functions.tsv`: every
function in the reference's non-library modules is classified as application
code, a compiler-emitted COMDAT, or a statically linked library member, then
scored against the compiler's own `-S` listings for that unit. Regenerate with
`make unitmap && make track`; the sheet is the single source of truth for
per-function progress, and [PLAN.md](PLAN.md) records phases, risks, the
outstanding cross-unit externs and the known-unconverged functions.

<!-- BEGIN GENERATED STATUS -->

**1676/1809 (92.6%)** application functions byte-exact (1809 app + compiler COMDATs; 502 library members excluded).

**314,530/670,020 (46.9%)** application BYTES byte-exact. Function counts overstate progress while the deferred `Player_HandlePacket` (226,824 bytes) is outstanding: it alone is a third of the application's bytes.

```mermaid
pie showData
    title Application functions by status
    "byte-exact" : 1676
    "mismatched" : 11
    "stubbed" : 104
    "unimplemented" : 17
    "deferred" : 1
```

| Unit | functions | byte-exact | stubbed | mismatched | unimplemented |
| --- | ---: | ---: | ---: | ---: | ---: |
| Mapcontrol | 279 | 249 | 24 | 2 | 4 |
| Packets | 249 | 156 | 79 | 9 | 4 |
| Players | 160 | 159 | 1 | 0 | 0 |
| Questengine | 123 | 123 | 0 | 0 | 0 |
| Shopvalues | 86 | 86 | 0 | 0 | 0 |
| Npcvalues | 80 | 77 | 0 | 0 | 3 |
| Mysqlcontrols | 73 | 73 | 0 | 0 | 0 |
| Msgboardcontrol | 57 | 56 | 0 | 0 | 1 |
| Itemvalues | 44 | 44 | 0 | 0 | 0 |
| Learnvalues | 41 | 41 | 0 | 0 | 0 |
| Settings | 40 | 39 | 0 | 0 | 1 |
| Innvalues | 39 | 39 | 0 | 0 | 0 |
| Questcounters | 36 | 36 | 0 | 0 | 0 |
| Skillvalues | 34 | 34 | 0 | 0 | 0 |
| Jukeboxcontrol | 33 | 33 | 0 | 0 | 0 |
| Killcounters | 33 | 33 | 0 | 0 | 0 |
| Questcounter | 32 | 32 | 0 | 0 | 0 |
| Weddings | 31 | 31 | 0 | 0 | 0 |
| Classvalues | 29 | 29 | 0 | 0 | 0 |
| Learnvalue | 26 | 26 | 0 | 0 | 0 |
| Player | 26 | 26 | 0 | 0 | 0 |
| Mysqltask | 21 | 21 | 0 | 0 | 0 |
| Npcvalue | 21 | 21 | 0 | 0 | 0 |
| Filecache | 18 | 18 | 0 | 0 | 0 |
| Serial | 18 | 18 | 0 | 0 | 0 |
| Shopvalue | 16 | 16 | 0 | 0 | 0 |
| Quest | 14 | 14 | 0 | 0 | 0 |
| Npccontrol | 13 | 13 | 0 | 0 | 0 |
| Queststate | 12 | 12 | 0 | 0 | 0 |
| Banned | 11 | 7 | 0 | 0 | 4 |
| Logins | 10 | 10 | 0 | 0 | 0 |
| Map | 8 | 8 | 0 | 0 | 0 |
| Mysqlthread | 8 | 8 | 0 | 0 | 0 |
| Gamecontrol | 7 | 7 | 0 | 0 | 0 |
| Mapchest | 7 | 7 | 0 | 0 | 0 |
| Chestcontrol | 5 | 5 | 0 | 0 | 0 |
| Newscontrol | 5 | 5 | 0 | 0 | 0 |
| Effectcontrol | 4 | 4 | 0 | 0 | 0 |
| Eventcontrol | 4 | 4 | 0 | 0 | 0 |
| Doorcontrol | 3 | 3 | 0 | 0 | 0 |
| Killcounter | 3 | 3 | 0 | 0 | 0 |
| Learnitem | 3 | 3 | 0 | 0 | 0 |
| Msgboard | 3 | 3 | 0 | 0 | 0 |
| Questcounterlist | 3 | 3 | 0 | 0 | 0 |
| Weaponmap | 3 | 3 | 0 | 0 | 0 |
| Classvalue | 2 | 2 | 0 | 0 | 0 |
| Innvalue | 2 | 2 | 0 | 0 | 0 |
| Itemchest | 2 | 2 | 0 | 0 | 0 |
| Itemground | 2 | 2 | 0 | 0 | 0 |
| Itemvalue | 2 | 2 | 0 | 0 | 0 |
| Jukebox | 2 | 2 | 0 | 0 | 0 |
| Mapobject | 2 | 2 | 0 | 0 | 0 |
| Mapwarp | 2 | 2 | 0 | 0 | 0 |
| Npc | 2 | 2 | 0 | 0 | 0 |
| Npcdrop | 2 | 2 | 0 | 0 | 0 |
| Playercommand | 2 | 2 | 0 | 0 | 0 |
| Playerinventory | 2 | 2 | 0 | 0 | 0 |
| Playerquest | 2 | 2 | 0 | 0 | 0 |
| Playerskill | 2 | 2 | 0 | 0 | 0 |
| Questtype | 2 | 2 | 0 | 0 | 0 |
| Shopcraft | 2 | 2 | 0 | 0 | 0 |
| Shopitem | 2 | 2 | 0 | 0 | 0 |
| Skillvalue | 2 | 2 | 0 | 0 | 0 |
| Wedding | 2 | 2 | 0 | 0 | 0 |

<!-- END GENERATED STATUS -->

