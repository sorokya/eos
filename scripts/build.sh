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
REF="${REF:-GameServer.exe}"
OUT="${OUT:-build/GameServer.exe}"
MAP="${MAP:-}"
CFLAGS="${CFLAGS:--D__CODEGUARD__ -v -Od -tWM -k}"
# A VCL application must link the *package* RTL (cp32mt.lib), not cw32mt.lib:
# cw32mt.lib carries the stubbed-out crtlst_[iel].c versions of
# ___CRTL_VCL_Init / ___CRTL_VCL_Exit / ___CRTL_VCLLIB_Linkage, while
# cp32mt.lib carries the real crtlvcl.cpp.  With cw32mt.lib listed first the
# stubs win, nothing ever references __InitVCL, vcle50.lib|VCLINIT is never
# pulled, and with it nothing references @Sysinit@VclInit/VclExit -- so the
# SysInit communals stay out of the image and the head after c0w32 is 56
# bytes instead of the reference's 368.
# ... and the VCL libraries must precede import32.lib/cp32mt.lib, which is the
# C++Builder 5 default ALLLIB order ($(LIBRARIES) import32.lib cp32mt.lib).
# The order is observable: Delphi threadvars are addressed as [tls + <offset of
# this module's _TLS segment>], a link-time fixup.  The reference's System unit
# sits at offset 0x0c, i.e. after ScktComp(4) + Classes(8) and *before* the
# 0xa4-byte cp32mt block; searching cp32mt first puts System at 0xb0 instead
# and every System routine that touches a threadvar then differs from the
# reference.
#
# The libraries are listed *inline in the object list*, not after the comma.
# ilink32 lays explicit objects out contiguously in command-line order and
# places a library's pulled members at the point the .lib appears in the
# OBJFILES field, so a library's position is observable in the final layout.
# The reference's placement (established by matching the block sizes in
# analysis/target/modules.tsv against the pulled per-library sizes, and
# verified by comparing export RVAs against a relink) is:
#   vcldb50.lib between Itemground and Itemchest  (~109,228 B block),
#   vcl50.lib + vclbde50.lib + vcle50.lib between Weaponmap and Banned
#   (~486,572 B block),
# with import32.lib + cp32mt.lib searched last so cp32mt's members append
# after all units.  Putting the VCL libraries after the comma instead pushes
# every library member to the end and shifts the post-Itemground unit RVAs by
# up to ~600 KB.
LIB_DB='"Z:\borland\Lib\Debug\vcldb50.lib"'
LIB_VCL='"Z:\borland\Lib\Debug\vcl50.lib" "Z:\borland\Lib\Debug\vclbde50.lib" "Z:\borland\Lib\Debug\vcle50.lib"'
VLIB="${VLIB:-import32.lib cp32mt.lib}"
LINKFLAGS="${LINKFLAGS:--Tpe -aa -c -Gn -j -v}"
MAPARG=""

UNITS=()
while IFS= read -r u; do
  [ -n "$u" ] && UNITS+=("$u")
done < <(tail -n +2 "$UNITS_TSV" | cut -f2 | grep -v '^GUI$')

mkdir -p build/obj

# The linked resource (.res) is reconstructed from the reference image's .rsrc
# by `make extract`; brcc32 is no longer used. Generate it on first build.
if [ ! -f build/GameServer.res ]; then
  echo "extracting resources from $REF (make extract) ..."
  python3 scripts/extract_res.py "$REF" -o build/GameServer.res
fi

{
  echo 'set -e'
  echo 'mkdir -p build/obj'
  if [ -z "${LINK_ONLY:-}" ]; then
  for u in "${UNITS[@]}"; do
    echo "wine \"\$B\\Bin\\bcc32.exe\" $CFLAGS -c -obuild/obj/$u.obj src/$u.cpp"
  done
  fi
  echo "L=\"${LPATH:--L\$BZ\\Lib -L\$BZ\\Lib\\Obj -L\$BZ\\Lib\\Debug -L\$BZ\\Lib\\Release}\""

  OBJS="${HEADOBJ-" \"Z:\\borland\\Lib\\Obj\\sysinit.obj\""}" 
  for u in "${UNITS[@]}"; do
    if [ "$u" = "Itemchest" ]; then
      OBJS+=" $LIB_DB"
    fi
    if [ "$u" = "Banned" ]; then
      OBJS+=" $LIB_VCL"
    fi
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
# Reproduce the reference .rsrc byte-for-byte once the image geometry matches
# (the linker's merged resources/genuine DVCLAL cannot be driven from the .res).
python3 scripts/inject_rsrc.py --ref "$REF" --target "$OUT" || true
echo "built $OUT"
