#!/usr/bin/env bash
# Build the reconstructed application.
#
# Compiles every unit in src/ (in analysis/target/units.tsv link order, the
# main unit Mainform first) and links them into build/GameServer.exe, then
# applies the deterministic timestamp step.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

UNITS_TSV="${UNITS_TSV:-analysis/target/units.tsv}"
OUT="${OUT:-build/GameServer.exe}"
MAP="${MAP:-}"
CFLAGS="${CFLAGS:--D__CODEGUARD__ -v -Od -tWM}"
# A VCL application must link the *package* RTL (cp32mt.lib), not cw32mt.lib:
# cw32mt.lib carries the stubbed-out crtlst_[iel].c versions of
# ___CRTL_VCL_Init / ___CRTL_VCL_Exit / ___CRTL_VCLLIB_Linkage, while
# cp32mt.lib carries the real crtlvcl.cpp.  With cw32mt.lib listed first the
# stubs win, nothing ever references __InitVCL, vcle50.lib|VCLINIT is never
# pulled, and with it nothing references @Sysinit@VclInit/VclExit -- so the
# SysInit communals stay out of the image and the head after c0w32 is 56
# bytes instead of the reference's 368.
VLIB="${VLIB:-import32.lib cp32mt.lib vcl50.lib vcldb50.lib vclbde50.lib}"
LINKFLAGS="${LINKFLAGS:--Tpe -aa -c -Gn -j -v}"
MAPARG=""

UNITS=()
while IFS= read -r u; do
  [ -n "$u" ] && UNITS+=("$u")
done < <(tail -n +2 "$UNITS_TSV" | cut -f2 | grep -v '^GUI$')

mkdir -p build/obj

{
  echo 'set -e'
  echo 'mkdir -p build/obj'
  if [ -z "${LINK_ONLY:-}" ]; then
  for u in "${UNITS[@]}"; do
    echo "wine \"\$B\\Bin\\bcc32.exe\" $CFLAGS -c -obuild/obj/$u.obj src/$u.cpp"
  done
  echo "wine \"\$B\\Bin\\brcc32.exe\" -fo\"Z:\\work\\build\\GameServer.res\" res/GameServer.rc"
  fi
  echo "L=\"${LPATH:--L\$BZ\\Lib -L\$BZ\\Lib\\Obj -L\$BZ\\Lib\\Debug -L\$BZ\\Lib\\Release}\""

  OBJS="${HEADOBJ-" \"Z:\\borland\\Lib\\Obj\\sysinit.obj\""}" 
  for u in "${UNITS[@]}"; do
    OBJS+=" \"Z:\\work\\build\\obj\\$u.obj\""
  done
  echo "wine \"\$B\\Bin\\ilink32.exe\" $LINKFLAGS \$L \"\$BZ\\Lib\\c0w32.obj\" $OBJS, \"Z:\\work\\build\\GameServer.exe\", $MAPARG, $VLIB, , \"Z:\\work\\build\\GameServer.res\""
  if [ -n "$MAP" ]; then
    # The detailed segment map (-s) is only produced together with -v, and that
    # exact combination intermittently faults under Wine: ilink32 raises an
    # unhandled fault *after* writing the complete map, leaving a corrupt exe.
    # Run it tolerantly for the map, then relink without -s so the exe is
    # always valid. (The real fix is a newer Wine in docker/Dockerfile.)
    echo "wine \"\$B\\Bin\\ilink32.exe\" $LINKFLAGS -s \$L \"\$BZ\\Lib\\c0w32.obj\" $OBJS, \"Z:\\work\\build\\GameServer_map.exe\", $MAPARG, $VLIB, , \"Z:\\work\\build\\GameServer.res\" || true"
  fi
} > build/build_inner.sh

echo "building ${#UNITS[@]} units ..."
scripts/borland.sh 'bash build/build_inner.sh'

[ -f "$OUT" ] || { echo "link produced no $OUT" >&2; exit 1; }
python3 scripts/normalize_pe.py "$OUT" >/dev/null
python3 scripts/normalize_pe.py "$OUT" --characteristics 0x10e >/dev/null
echo "built $OUT"
