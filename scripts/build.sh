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
CFLAGS="${CFLAGS:--v -Od -tWM -k}"
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
#   vclbde50.lib + vcl50.lib between Weaponmap and Banned
#   (~423,596 B block),
# and vcle50.lib at the very end, *after* cp32mt.lib: matching each module of
# ours to its home in the reference (`scripts/libcompare.py`) puts the
# reference's VCLBDE50 members (SMIntf 0x4b5c80, DbPwDlg 0x4b5ce8, Bde
# 0x4b6374, DbTables 0x4baf60) *before* VCL50's AppEvnts 0x4c9164 -- i.e. the
# original link listed vclbde50.lib first -- and the vcle50 members (DSTRING
# 0x5590dd, syssupp, vclinit, datetime, classcre) after cp32mt's tail
# (XX 0x555478).  Putting the VCL libraries after the comma instead pushes
# every library member to the end and shifts the post-Itemground unit RVAs by
# up to ~600 KB.
#
# vcldb50 is not listed as a .lib, though: scanning it inline perturbs the
# linker's package(smart_init) ordering (Skillvalue/Npcvalue come out inverted,
# invariant to the command line). Its four needed members -- DbLogDlg,
# DBCommon, DbConsts, Db -- are extracted with tlib and listed as explicit
# objects instead; unreferenced COMDATs are dropped exactly as when the .lib is
# pulled, so the block is byte-identical, and vcldb50.lib stays in the tail to
# resolve anything remaining.
#
# vcl50.lib is listed a second time, immediately after the vclbde50/vcl50 pair.
# The repeat pulls no member (everything it resolves was already pulled), so it
# changes neither the VCL block nor the tail, but ilink32's package(smart_init)
# tie-break is sensitive to the library scan list: without it Wedding is emitted
# before Eventcontrol (the reference has Eventcontrol 0x52db20 before Wedding
# 0x52e224, and their init counters 0x58c6b8/0x58c6bc).  The repeat restores the
# reference's export order while keeping vcle50 in the tail, i.e. .data at +48
# instead of +468.
LIB_VCL='"Z:\borland\Lib\Debug\vclbde50.lib" "Z:\borland\Lib\Debug\vcl50.lib" "Z:\borland\Lib\Debug\vcl50.lib"'
LIB_DB_OBJS='"Z:\work\build\res\vcldb\DbLogDlg.OBJ" "Z:\work\build\res\vcldb\DBCommon.OBJ" "Z:\work\build\res\vcldb\DbConsts.OBJ" "Z:\work\build\res\vcldb\Db.OBJ"'
VLIB="${VLIB:-vcldb50.lib import32.lib cp32mt.lib vcle50.lib}"
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
  python3 scripts/extract_res.py "$REF" -o build/GameServer.res --only "TGUI,MAINICON,3:1"
fi

{
  echo 'set -e'
  echo 'mkdir -p build/obj build/res/vcldb'
  if [ -z "${LINK_ONLY:-}" ]; then
  if [ -f src/GameServer.cpp ]; then
    echo "wine \"\$B\\Bin\\bcc32.exe\" $CFLAGS -c -obuild/obj/GameServer.obj src/GameServer.cpp"
  fi
  for u in "${UNITS[@]}"; do
    echo "wine \"\$B\\Bin\\bcc32.exe\" $CFLAGS -c -obuild/obj/$u.obj src/$u.cpp"
  done
  fi
  echo '( cd build/res/vcldb && for m in DbLogDlg DBCommon DbConsts Db; do wine "$BZ\\Bin\\tlib.exe" "$BZ\\Lib\\Debug\\vcldb50.lib" "*$m" >/dev/null 2>&1; done )'
  echo "L=\"${LPATH:--L\$BZ\\Lib -L\$BZ\\Lib\\Obj -L\$BZ\\Lib\\Debug -L\$BZ\\Lib\\Release}\""

  OBJS="${HEADOBJ-" \"Z:\\borland\\Lib\\Obj\\sysinit.obj\""}" 
  # The project's main unit is its own translation unit (src/GameServer.cpp, the
  # BCB project file), linked between sysinit.obj and Mainform.obj. The
  # reference's first module is WinMain immediately followed by the Exception
  # RTTI group bcc32 flushes at end of translation unit, which only happens when
  # WinMain is the last -- here the only -- function in its TU. The unit carries
  # no #pragma package(smart_init) (the reference exports no GUI Initialize /
  # Finalize pair) and, like a real BCB project file, declares the form through
  # USEFORM rather than including Mainform.h.
  #
  # The object's *name* is observable: ilink32 lays the cp32mt.lib block out
  # inline after DbConsts, ~96 KB before its reference position, when this
  # object is called GUI.obj -- the name of the exported form variable -- and at
  # its reference position under any other name. GameServer.obj is the name a
  # BCB project for GameServer.exe would produce.
  if [ -f src/GameServer.cpp ]; then
    OBJS+=" \"Z:\\work\\build\\obj\\GameServer.obj\""
  fi
  for u in "${UNITS[@]}"; do
    if [ "$u" = "Itemchest" ]; then
      OBJS+=" $LIB_DB_OBJS"
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
