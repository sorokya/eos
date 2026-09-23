# GameServer.exe decompilation build driver (Phase 0 harness).
#
# Every Borland tool runs inside the Docker/Wine image; host-side steps use
# python3 and objdump. See AGENTS.md for the toolchain details.

IMAGE     ?= gameserver-borland-wine
PYTHON    ?= python3
REF       ?= GameServer.exe

# The reference's codegen corresponds to source-level debug info (-v, which also
# disables C++ inline expansion), no optimization (-Od), the multithreaded RTL
# target (-tWM) and standard stack frames (-k). __CODEGUARD__ must NOT be defined
# (see AGENTS.md). Keep this in sync with scripts/build.sh.
CFLAGS    ?= -v -Od -tWM -k

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

.PHONY: image analyze extract units track unitmap functions funcdiff rawdiff initorder orderdiff comdatdiff tdsfuncs typenames struct layout libcompare disasm sanity compare normalize build stubs unit unit-asm verify case-selftest format format-check clean

image:
	docker build -t $(IMAGE) docker

analyze:
	$(PYTHON) scripts/extract_target.py $(REF) -o analysis/target
	$(PYTHON) scripts/units.py $(REF)

# Reconstruct the application-owned resources (the ones the app's own .res
# supplied) from the reference image's embedded .rsrc. The VCL/BDE resources are
# linked from the toolchain, not extracted: including them here only creates
# duplicates the linker resolves in favour of the library. Derived; not committed.
extract:
	$(PYTHON) scripts/extract_res.py $(REF) -o build/GameServer.res --only "TGUI,MAINICON,3:1" --dump build/res

# Central per-function status sheet (analysis/target/functions.tsv) and the
# generated README status block. Depends on --units output, so run `make unitmap`
# first, and on the detailed link map, so run `MAP=1 scripts/build.sh` too (the
# library/comdat classification matches against the linked build; without the
# map it falls back to the noisier .lib blob). Caches the classification in
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

# The whole-tree byte differential: every reference function whose bytes are not
# found (masked) in our image. Unlike `functions`, it masks rel32 displacements
# and decodes each function from its own start, so it sees the branch-target
# differences `make verify` canonicalises away. Exits non-zero when any differ.
# Raw (unmasked) byte differential of .text and .data, attributed to modules.
# Meaningful once every module is at its reference RVA; exits non-zero on any
# difference.
rawdiff:
	$(PYTHON) scripts/rawdiff.py --section .text
	$(PYTHON) scripts/rawdiff.py --section .data

# Predicted package-init order (from build/obj) vs the reference's _INIT_ table.
initorder:
	$(PYTHON) scripts/initorder.py

funcdiff:
	$(PYTHON) scripts/funcdiff.py

# Within-module function order: every function can be byte-exact and every
# module at its reference RVA while the functions inside one module sit in a
# different order. Neither `funcdiff` nor `layout` can see that; this does.
orderdiff:
	$(PYTHON) scripts/orderdiff.py

# The opposite direction: COMDATs *we* emit that the reference does not have in
# that module. `funcdiff` walks reference -> rebuild and so cannot see a surplus
# or a misplaced COMDAT; this reads our own linked inventory out of the TDS and
# compares masked body counts per module. Exits non-zero on any mismatch.
comdatdiff:
	$(PYTHON) scripts/comdatdiff.py

# Our linked function inventory (name, VA, exact COMDAT length) from the TDS.
tdsfuncs:
	$(PYTHON) scripts/tdsfuncs.py

# The RTTI type-name oracle: every class name both images spell out in their
# `__tpdsc__` records, with the module that owns each. A name only the reference
# has (paired with one only we have) is a class the reconstruction misnamed.
typenames:
	$(PYTHON) scripts/typenames.py -o analysis/target/typenames.tsv

# Structural comparison (imports/exports/relocations).
struct:
	-$(PYTHON) scripts/compare_pe.py $(REF) build/GameServer.exe --ignore-timestamp --struct

# Reference module layout vs our ilink32 map: per-unit RVA/order and the
# library interleave points. Requires `MAP=1 scripts/build.sh` first.
layout:
	$(PYTHON) scripts/layoutdiff.py

# Locate each of our modules in the reference image ("do we pull a module the
# reference does not"). Requires `MAP=1 scripts/build.sh` first.
libcompare:
	$(PYTHON) scripts/libcompare.py

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
