#!/usr/bin/env bash
# Produce a linear disassembly of the reference image for offline analysis.
#
# This is a raw listing (host `objdump`). Phase 1 uses a function-aware
# disassembler (e.g. Ghidra headless) driven by the recovered symbol map; this
# script is the quick, dependency-free view used during setup.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PE="${1:-$REPO_ROOT/GameServer.exe}"
OUT="${2:-$REPO_ROOT/analysis/target/disasm.txt}"

mkdir -p "$(dirname "$OUT")"
objdump -D -M intel --section=.text "$PE" > "$OUT"
printf '%s: %s lines\n' "$OUT" "$(wc -l < "$OUT")"
