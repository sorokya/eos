#!/usr/bin/env bash
# Run a command inside the Borland/Wine container.
#
# The Borland tree is mounted at the exact Windows path recorded in its
# bcc32.cfg / ilink32.cfg so include and library lookup works, and the
# repository is available at /work (the container working directory).
#
# Exported for the command:
#   $B   Borland root, with spaces:  C:\Program Files (x86)\Borland\CBuilder5
#   $BZ  Same tree via a space-free wine path:  Z:\borland
#
# Use $B to invoke tools (the loader finds the DLLs beside the .exe) and $BZ
# for -L / object / library arguments, because ilink32 mishandles spaces on
# its command line. Example:
#   scripts/borland.sh 'wine "$B\Bin\bcc32.exe" -c -obuild/x.obj build/x.cpp'
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${BORLAND_IMAGE:-gameserver-borland-wine}"
BORLAND_MOUNT="/wineprefix/drive_c/Program Files (x86)/Borland/CBuilder5"

if [ "$#" -eq 0 ]; then
  echo "usage: $0 '<command>'" >&2
  exit 2
fi

exec docker run --rm --platform linux/amd64 \
  -v "$REPO_ROOT/ref/Borland5:$BORLAND_MOUNT:ro" \
  -v "$REPO_ROOT:/work" \
  "$IMAGE" bash -lc "
    export B='C:\\Program Files (x86)\\Borland\\CBuilder5'
    ln -sfn '/wineprefix/drive_c/Program Files (x86)/Borland/CBuilder5' /borland
    export BZ='Z:\\borland'
    $*"
