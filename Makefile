# GameServer.exe decompilation build driver (Phase 0 harness).
#
# Every Borland tool runs inside the Docker/Wine image; host-side steps use
# python3 and objdump. See AGENTS.md for the toolchain details.

IMAGE     ?= gameserver-borland-wine
PYTHON    ?= python3
REF       ?= GameServer.exe

# Link configuration validated in Phase 0 against the reference header and
# section geometry (see PLAN.md). -v is required for header characteristic
# 0x010e; the static VCL+BDE library set covers the components observed in the
# target. Exact library order is finalized in Phase 1.
LINKFLAGS ?= -Tpe -aa -c -Gn -j -v
VLIB      ?= import32.lib cw32mt.lib vcl50.lib vcldb50.lib vclbde50.lib

.PHONY: image analyze disasm sanity compare normalize clean

image:
	docker build -t $(IMAGE) docker

analyze:
	$(PYTHON) scripts/extract_target.py $(REF) -o analysis/target

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

clean:
	rm -rf build
