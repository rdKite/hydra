#!/usr/bin/env bash
# Creates Content/Looks/PP_Look.uasset headlessly: the post-process material behind the look
# variants (HydraLookVariant 1|2|3 in the game). Only needed again after a change of the HLSL in
# Scripts/create_look_material.py.
# Usage: Scripts/create_look_material.sh        Needs a built editor target (Scripts/build.sh).
# Exit code 0 = material written. Log: Saved/Logs/CreateLookMaterial.log
set -u
source "$(dirname "$0")/env.sh"
SCRIPT="$(cygpath -w "$ROOT/Scripts/create_look_material.py")"
"$EDITOR_CMD" "$PROJ" -run=pythonscript "-script=$SCRIPT" -unattended -nosplash -nullrhi -log=CreateLookMaterial.log > /dev/null 2>&1
grep -hE "create_look_material|Error:" "$ROOT/Saved/Logs/CreateLookMaterial.log" | sed 's/^\[[^]]*\]\[[^]]*\]//' | grep -v "LogPython: Error: $" | head -20
[ -f "$ROOT/Content/Looks/PP_Look.uasset" ] && grep -q "create_look_material: saved" "$ROOT/Saved/Logs/CreateLookMaterial.log"
