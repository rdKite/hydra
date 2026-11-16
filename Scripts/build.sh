#!/usr/bin/env bash
# Builds the editor target (the one used for tests and for -game runs).
# Usage: Scripts/build.sh            Exit code 0 = build succeeded.
set -u
source "$(dirname "$0")/env.sh"
mkdir -p "$ROOT/Saved"
"$UE/Engine/Build/BatchFiles/Build.bat" HydraEditor Win64 Development "-Project=$PROJ" -WaitMutex > "$ROOT/Saved/Build.log" 2>&1
CODE=$?
grep -E "error|Result:|Total execution" "$ROOT/Saved/Build.log" | head -40
exit $CODE
