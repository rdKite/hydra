#!/usr/bin/env bash
# Builds the editor target and runs the project's automation tests headlessly.
# Usage: Scripts/run_tests.sh [test filter=Hydra]
# Exit code 0 = build succeeded and all tests passed. Logs: Saved/Build.log, Saved/Logs/Tests.log
set -u
source "$(dirname "$0")/env.sh"
FILTER="${1:-Hydra}"

bash "$ROOT/Scripts/build.sh" || { echo "BUILD FAILED (see Saved/Build.log)"; exit 1; }

LOG="$ROOT/Saved/Logs/Tests.log"
rm -f "$LOG"
"$EDITOR_CMD" "$PROJ" "-ExecCmds=Automation RunTests $FILTER;Quit" \
  -unattended -nullrhi -nosplash -nosound "-testexit=Automation Test Queue Empty" -log=Tests.log > /dev/null 2>&1
[ -f "$LOG" ] || { echo "TESTS DID NOT RUN (no Saved/Logs/Tests.log)"; exit 1; }

grep -hE "Test Completed|LogAutomationController: Error" "$LOG" | sed 's/^\[[^]]*\]\[[^]]*\]//'
PASSED=$(grep -c "Result={Success}" "$LOG")
FAILED=$(grep -c "Result={Fail" "$LOG")
echo "passed=$PASSED failed=$FAILED"
[ "$FAILED" -eq 0 ] && [ "$PASSED" -gt 0 ]
