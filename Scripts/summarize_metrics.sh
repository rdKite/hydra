#!/usr/bin/env bash
# Summary of a session's metrics CSV (hits, blocked hits, dodges, stamina use per player and
# enemy type, then per wave).
# Usage: Scripts/summarize_metrics.sh              the newest file in Saved/Metrics
#        Scripts/summarize_metrics.sh <file|dir>   a given file, or the newest in a folder
set -u
source "$(dirname "$0")/env.sh"
ARGS=()
[ $# -gt 0 ] && ARGS=("$(cygpath -w "$1")")
"$UE/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" "$(cygpath -w "$ROOT/Scripts/summarize_metrics.py")" "${ARGS[@]}"
