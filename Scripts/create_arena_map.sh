#!/usr/bin/env bash
# Creates Content/Maps/Arena.umap headlessly (Python commandlet, no editor UI).
# Usage: Scripts/create_arena_map.sh        Needs a built editor target (Scripts/build.sh).
# Exit code 0 = map written. Log: Saved/Logs/CreateArenaMap.log
set -u
source "$(dirname "$0")/env.sh"
SCRIPT="$(cygpath -w "$ROOT/Scripts/create_arena_map.py")"
"$EDITOR_CMD" "$PROJ" -run=pythonscript "-script=$SCRIPT" -unattended -nosplash -nullrhi -log=CreateArenaMap.log > /dev/null 2>&1
grep -hE "create_arena_map|Error:" "$ROOT/Saved/Logs/CreateArenaMap.log" | sed 's/^\[[^]]*\]\[[^]]*\]//' | head -20
[ -f "$ROOT/Content/Maps/Arena.umap" ] && grep -q "create_arena_map: saved" "$ROOT/Saved/Logs/CreateArenaMap.log"
