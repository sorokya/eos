# GameServer.exe decompilation build driver (Phase 0 harness).
#
# Every Borland tool runs inside the Docker/Wine image; host-side steps use
# python3 and objdump. See AGENTS.md for the toolchain details.

IMAGE     ?= gameserver-borland-wine
PYTHON    ?= python3
REF       ?= GameServer.exe

# The reference was compiled with CodeGuard compile-time checks
# (__CODEGUARD__), source-level debug info (-v, which also disables C++ inline
# expansion), no optimization (-Od), the multithreaded RTL target (-tWM) and
# standard stack frames (-k). Keep this in sync with scripts/build.sh.
CFLAGS    ?= -D__CODEGUARD__ -v -Od -tWM -k

# Parallel compile jobs for `make verify` (empty = the container's CPU count).
JOBS      ?=

# Link configuration. The reference's library code byte-matches the Debug VCL/BDE
# libraries, not Release (see PLAN.md), so Lib/Debug precedes Lib/Release on the
# search path; same-named .libs there win.
# VLIB here is only the tail library list used by the `sanity` test link.
# `scripts/build.sh` places the VCL/BDE libraries *inline* in the object list at
# the reference's interleave points (see AGENTS.md / scripts/README.md).
LINKFLAGS ?= -Tpe -aa -c -Gn -j -v
# cp32mt.lib, not cw32mt.lib: the latter carries the stubbed-out
# ___CRTL_VCL_Init/_Exit/___CRTL_VCLLIB_Linkage (crtlst_[iel].c) and, listed
# first, kills the whole VCL init chain -- see scripts/build.sh.
VLIB      ?= vcl50.lib vcldb50.lib vclbde50.lib import32.lib cp32mt.lib

CLANG_FORMAT ?= clang-format
SRC          := $(wildcard src/*.cpp src/*.h)

.PHONY: image analyze extract units track unitmap functions struct layout disasm sanity compare normalize build stubs unit unit-asm verify case-selftest format format-check clean

image:
	docker build -t $(IMAGE) docker

analyze:
	$(PYTHON) scripts/extract_target.py $(REF) -o analysis/target
	$(PYTHON) scripts/units.py $(REF)

# Reconstruct the linked resource (.res) and the payloads from the reference
# image's embedded .rsrc. Derived artifacts; not committed.
extract:
	$(PYTHON) scripts/extract_res.py $(REF) -o build/GameServer.res --dump build/res

# Central per-function status sheet (analysis/target/functions.tsv) and the
# generated README status block. Depends on --units output, so run `make unitmap`
# first. Caches the library/comdat classification in
# analysis/target/function_kinds.tsv (--reclassify to rebuild it).
track:
	$(PYTHON) scripts/track.py --readme README.md

units:
	$(PYTHON) scripts/units.py $(REF)

# Attribute reference code to units and list every module boundary, classifying
# unexported modules as library (byte-matched against the Borland libs) or
# unknown. See scripts/README.md.
unitmap:
	$(PYTHON) scripts/unitmap.py --pe $(REF) --stubs --units analysis/target/units.tsv --functions analysis/ghidra/functions.tsv -o analysis/target/modules.tsv
	$(PYTHON) scripts/unitmap.py --units analysis/target/units.tsv --functions analysis/ghidra/functions.tsv --modules analysis/target/modules.tsv -o analysis/target/unit_functions.tsv

# Per-function byte comparison against the Ghidra inventory (layout independent).
# Informational: differences are expected until reconstruction converges.
functions:
	-$(PYTHON) scripts/compare_functions.py analysis/ghidra/functions.tsv $(REF) build/GameServer.exe --mask-reloc

# Structural comparison (imports/exports/relocations).
struct:
	-$(PYTHON) scripts/compare_pe.py $(REF) build/GameServer.exe --ignore-timestamp --struct

# Reference module layout vs our ilink32 map: per-unit RVA/order and the
# library interleave points. Requires `MAP=1 scripts/build.sh` first.
layout:
	$(PYTHON) scripts/layoutdiff.py

disasm:
	scripts/disasm.sh

# Sanity-check the toolchain and link configuration with a minimal VCL+BDE app.
sanity:
	mkdir -p build
	scripts/borland.sh 'wine "$$B\Bin\bcc32.exe" -c -obuild/vcl_link.obj tests/vcl_link.cpp'
	scripts/borland.sh 'L="-L$$BZ\Lib -L$$BZ\Lib\Obj -L$$BZ\Lib\Debug -L$$BZ\Lib\Release"; wine "$$B\Bin\ilink32.exe" $(LINKFLAGS) $$L "$$BZ\Lib\c0w32.obj" "Z:\work\build\vcl_link.obj", "Z:\work\build\vcl_link.exe",, $(VLIB)'

# Level-3 comparison of a rebuilt image against the reference.
compare:
	$(PYTHON) scripts/compare_pe.py $(REF) build/GameServer.exe --ignore-timestamp

# Documented deterministic post-link step (see AGENTS.md).
normalize:
	$(PYTHON) scripts/normalize_pe.py build/GameServer.exe

# Generate stub units for every translation unit (skips existing sources).
stubs:
	$(PYTHON) scripts/mkstubs.py

# Build the full application: compile all src/ units, link, normalize timestamp.
build:
	scripts/build.sh

# Compile one reconstructed unit to an object:  make unit UNIT=Serial
unit:
	scripts/borland.sh 'wine "$$B\Bin\bcc32.exe" $(CFLAGS) -c -obuild/$(UNIT).obj src/$(UNIT).cpp'

# Emit bcc32 assembly for one unit (codegen inspection):  make unit-asm UNIT=Serial
unit-asm:
	scripts/borland.sh 'wine "$$B\Bin\bcc32.exe" $(CFLAGS) -S -obuild/$(UNIT).asm src/$(UNIT).cpp'

# Emit a bcc32 -S listing for every unit, then score every source function
# against its unit's reference range (see scripts/verify_units.py).
verify:
	JOBS=$(JOBS) scripts/build_asm.sh
	$(PYTHON) scripts/verify_units.py $(if $(JOBS),-j $(JOBS),)

# Self-test the per-case comparison locator (no build required): proves the
# anchor refuses a fully repeated slice, extends a repeated short prefix until
# unique, reports a perturbed instruction, gives zero mismatches on a correct
# slice, and excludes an in-function data region from the instruction stream.
# See scripts/compare_case.py.
case-selftest:
	$(PYTHON) scripts/compare_case.py --selftest

# Apply the project code style (.clang-format) in place.
format:
	$(CLANG_FORMAT) -i $(SRC)

# Non-mutating style check; fails if any source is not formatted.
format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(SRC)

clean:
	rm -rf build
