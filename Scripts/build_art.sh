#!/usr/bin/env bash
# Art pipeline, headless: Blender scripts build models, rigs, textures and placeholder
# animations and export FBX; a Python commandlet imports them into Content/Characters/.
#
# Usage: Scripts/build_art.sh              build and import everything (slime, wolf_pup)
#        Scripts/build_art.sh slime        only one model
#        Scripts/build_art.sh import       only the import into Unreal (after exporting
#                                          animations by hand with export_animations.py)
# Needs Blender (set BLENDER to blender.exe if it is not in the default place) and a built
# editor target. Exit code 0 = built and imported, sizes as expected.
# Logs: Saved/Logs/Blender_<name>.log, Saved/Logs/ImportArt.log; previews: Saved/ArtPreview/
set -u
source "$(dirname "$0")/env.sh"
WHAT="${1:-all}"
mkdir -p "$ROOT/Saved/Logs"

if [ "$WHAT" != import ]; then
  BLENDER="${BLENDER:-$(ls -d /c/Program\ Files/Blender\ Foundation/Blender*/blender.exe 2> /dev/null | sort -V | tail -1)}"
  [ -x "$BLENDER" ] || BLENDER="$(command -v blender || true)"
  [ -n "$BLENDER" ] || { echo "Blender not found. Install it or set BLENDER to the path of blender.exe." >&2; exit 2; }
  MODELS="slime wolf_pup"
  [ "$WHAT" = all ] || MODELS="$WHAT"
  for MODEL in $MODELS; do
    SCRIPT="$ROOT/Art/scripts/build_$MODEL.py"
    [ -f "$SCRIPT" ] || { echo "No such model script: Art/scripts/build_$MODEL.py" >&2; exit 2; }
    "$BLENDER" -b --python-exit-code 1 --python "$(cygpath -w "$SCRIPT")" > "$ROOT/Saved/Logs/Blender_$MODEL.log" 2>&1 \
      || { echo "BLENDER FAILED for $MODEL (see Saved/Logs/Blender_$MODEL.log)"; grep -E "Error|Traceback|line " "$ROOT/Saved/Logs/Blender_$MODEL.log" | tail -8; exit 1; }
    grep -hE "^build_$MODEL: (size|done)" "$ROOT/Saved/Logs/Blender_$MODEL.log"
  done
  [ "$WHAT" = all ] || export HYDRA_ART="$WHAT"
fi

SCRIPT="$(cygpath -w "$ROOT/Scripts/import_art.py")"
rm -f "$ROOT/Saved/Logs/ImportArt.log"
"$EDITOR_CMD" "$PROJ" -run=pythonscript "-script=$SCRIPT" -unattended -nosplash -nullrhi -log=ImportArt.log > /dev/null 2>&1
grep -hE "import_art:|Error: " "$ROOT/Saved/Logs/ImportArt.log" | sed 's/^\[[^]]*\]\[[^]]*\]//' | grep -v "LogPython: Error: $" | head -30
grep -q "import_art: done" "$ROOT/Saved/Logs/ImportArt.log" || { echo "IMPORT FAILED (see Saved/Logs/ImportArt.log)"; exit 1; }
