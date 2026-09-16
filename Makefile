# GameServer.exe decompilation build driver (Phase 0 harness).
#
# Every Borland tool runs inside the Docker/Wine image; host-side steps use
# python3 and objdump. See AGENTS.md for the toolchain details.

IMAGE     ?= gameserver-borland-wine
PYTHON    ?= python3
REF       ?= GameServer.exe

# The reference was compiled with CodeGuard compile-time checks
# (__CODEGUARD__), source-level debug info (-v, which also disables C++ inline
# expansion), and no optimization (-Od). Keep this in sync with scripts/build.sh.
CFLAGS    ?= -D__CODEGUARD__ -v -Od

# Link configuration validated in Phase 0 against the reference header and
# section geometry (see PLAN.md). -v is required for header characteristic
# 0x010e; the static VCL+BDE library set covers the components observed in the
# target. Exact library order is finalized in Phase 1.
LINKFLAGS ?= -Tpe -aa -c -Gn -j -v
VLIB      ?= import32.lib cw32mt.lib vcl50.lib vcldb50.lib vclbde50.lib

.PHONY: image analyze units unitmap functions struct disasm sanity compare normalize build stubs unit unit-asm clean

image:
	docker build -t $(IMAGE) docker

analyze:
	$(PYTHON) scripts/extract_target.py $(REF) -o analysis/target
	$(PYTHON) scripts/units.py $(REF)

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

disasm:
	scripts/disasm.sh

# Sanity-check the toolchain and link configuration with a minimal VCL+BDE app.
sanity:
	mkdir -p build
	scripts/borland.sh 'wine "$$B\Bin\bcc32.exe" -c -obuild/vcl_link.obj tests/vcl_link.cpp'
	scripts/borland.sh 'L="-L$$BZ\Lib -L$$BZ\Lib\Obj -L$$BZ\Lib\Release"; wine "$$B\Bin\ilink32.exe" $(LINKFLAGS) $$L "$$BZ\Lib\c0w32.obj" "Z:\work\build\vcl_link.obj", "Z:\work\build\vcl_link.exe",, $(VLIB)'

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

clean:
	rm -rf build
