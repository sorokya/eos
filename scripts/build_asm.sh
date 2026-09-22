#!/usr/bin/env bash
# Emit a bcc32 `-S` listing for every unit into build/<Unit>.asm.
#
# Used by `make verify`: the listings feed scripts/verify_units.py, which scores
# each source function against the reference. One container run compiles the
# stale units in parallel (JOBS, default: the container's CPU count).
#
# Incremental: a unit is recompiled only when its .cpp *or any header* is newer
# than its existing listing, so a repeated `make verify` with no source change
# is a no-op. A header edit therefore invalidates every unit (they all share
# Project.h/Protocol.h), which is the conservative and correct behaviour.
# FORCE=1 recompiles everything.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

UNITS_TSV="${UNITS_TSV:-analysis/target/units.tsv}"
CFLAGS="${CFLAGS:--v -Od -tWM -k}"
FORCE="${FORCE:-0}"

mkdir -p build

# Unit list: the units in link order (the main unit Mainform comes first in
# units.tsv). `units.py` appends a spurious `GUI` row derived from the `_GUI`
# data export; there is no GUI unit - the export table has
# `@@Mainform@Initialize` and WinMain lives in the Mainform unit - so skip it.
{ tail -n +2 "$UNITS_TSV" | cut -f2 | grep -v '^GUI$'
  # the project main unit (src/GameServer.cpp) is not a reference unit -- it has
  # no Initialize/Finalize pair -- but its listing is needed to score the
  # functions the reference places at the head of the Mainform module.
  [ -f src/GameServer.cpp ] && echo GameServer
} | sort -u > build/asm_units.txt

# Newest header mtime: any unit older than this needs rebuilding.
newest_hdr=$(find src -name '*.h' -printf '%T@\n' 2>/dev/null | sort -rn | head -1 || true)
if [ -z "$newest_hdr" ]; then
  newest_hdr=$(find src -name '*.h' -exec stat -f '%m' {} + 2>/dev/null | sort -rn | head -1 || echo 0)
fi

stale=build/asm_stale.txt
: > "$stale"
while IFS= read -r u; do
  asm=build/$u.asm
  if [ "$FORCE" = "1" ] || [ ! -s "$asm" ] || [ "src/$u.cpp" -nt "$asm" ]; then
    echo "$u" >> "$stale"
  elif [ -n "$newest_hdr" ] && \
       [ "$(find src -maxdepth 1 -name '*.h' -newer "$asm" -print -quit)" ]; then
    echo "$u" >> "$stale"
  fi
done < build/asm_units.txt

n_stale=$(wc -l < "$stale" | tr -d ' ')
n_all=$(wc -l < build/asm_units.txt | tr -d ' ')
if [ "$n_stale" -eq 0 ]; then
  echo "build/*.asm up to date ($n_all units)"
  exit 0
fi

# The inner script runs inside the container; each unit compiles into its own
# log so parallel output stays readable and failures are attributable.
cat > build/asm_inner.sh <<EOF
set -euo pipefail
mkdir -p build/asm_log
work() {
  if ! wine "\$B\\Bin\\bcc32.exe" $CFLAGS -S -obuild/"\$1".asm src/"\$1".cpp \\
      >build/asm_log/"\$1".log 2>&1; then
    echo "FAIL \$1"
  fi
}
export -f work
xargs -P "\${JOBS:-\$(nproc)}" -I{} bash -c 'work "\$@"' _ {} < build/asm_stale.txt
echo ALLDONE
EOF

echo "emitting asm for $n_stale/$n_all units (JOBS=${JOBS:-auto}) ..."
scripts/borland.sh "JOBS=${JOBS:-} bash build/asm_inner.sh" | grep -v '^ALLDONE$' || true

if grep -lE 'Error E[0-9]+' build/asm_log/*.log >/dev/null 2>&1; then
  echo "compile errors:" >&2
  grep -HE 'Error E[0-9]+' build/asm_log/*.log >&2 || true
  exit 1
fi

missing=0
while IFS= read -r u; do
  [ -s "build/$u.asm" ] || { echo "no listing: $u" >&2; missing=1; }
done < build/asm_units.txt
[ "$missing" -eq 0 ] || exit 1

echo "wrote build/*.asm"
