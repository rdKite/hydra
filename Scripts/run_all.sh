#!/usr/bin/env bash
# Everything that verifies the game from the command line, in one go:
# build + automation tests, then the two scripted live runs (combat, slime and magic).
# Usage: Scripts/run_all.sh        Exit code 0 = all green. Takes about four minutes.
set -u
cd "$(dirname "$0")/.."
FAILED=0
for STEP in run_tests.sh run_live.sh run_live_slime.sh; do
  echo "=== Scripts/$STEP"
  bash "Scripts/$STEP" | grep -vE "^ok  |Result=\{Success\}" || true
  [ "${PIPESTATUS[0]}" -eq 0 ] || { echo "*** Scripts/$STEP FAILED"; FAILED=1; }
done
[ "$FAILED" -eq 0 ] && echo "ALL GREEN" || echo "SOMETHING FAILED"
exit $FAILED
