#!/usr/bin/env bash
# Emit a bcc32 `-S` listing for every unit into build/<Unit>.asm.
#
# Used by `make verify`: the listings feed scripts/verify_units.py, which scores
# each source function against the reference. One container run compiles all
# units, mirroring scripts/build.sh.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

UNITS_TSV="${UNITS_TSV:-analysis/target/units.tsv}"
CFLAGS="${CFLAGS:--D__CODEGUARD__ -v -Od}"

mkdir -p build
rm -f build/*.asm

{
  echo 'set -e'
  echo 'mkdir -p build'
  echo "wine \"\$B\\Bin\\bcc32.exe\" $CFLAGS -S -obuild/GUI.asm src/GUI.cpp"
  tail -n +2 "$UNITS_TSV" | cut -f2 | grep -v '^GUI$' | while IFS= read -r u; do
    echo "wine \"\$B\\Bin\\bcc32.exe\" $CFLAGS -S -obuild/$u.asm src/$u.cpp"
  done
} > build/asm_inner.sh

echo "emitting asm for all units ..."
scripts/borland.sh 'bash build/asm_inner.sh'
echo "wrote build/*.asm"
