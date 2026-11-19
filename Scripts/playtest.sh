#!/usr/bin/env bash
# Playtest kit: start a session for one form and print the metrics summary when it ends.
#
# Usage: Scripts/playtest.sh humanoid         play the humanoid against the waves (one window)
#        Scripts/playtest.sh slime            play the slime
#        Scripts/playtest.sh slime duo        with a second window (a humanoid client) for two players
#        Scripts/playtest.sh humanoid duo     ... the client is a slime then
#
# In the game: F1 opens the debug panel (form, level, skills, imprint, spawn enemies, waves
# on / off); the keys are on the top line of the HUD. Close the window(s) to end the session.
# Needs a built editor target (Scripts/build.sh).
set -u
cd "$(dirname "$0")/.."
FORM="${1:-humanoid}"
MODE="${2:-solo}"
case "$FORM" in
  humanoid) OTHER=slime ;;
  slime) OTHER=humanoid ;;
  *) echo "Usage: Scripts/playtest.sh humanoid|slime [duo]"; exit 2 ;;
esac

if [ "$MODE" = duo ]; then
  HOST_FORM="$FORM" CLIENT_FORM="$OTHER" WAVES=1 bash Scripts/play.sh both
else
  HOST_FORM="$FORM" WAVES=1 bash Scripts/play.sh host
fi
echo
bash Scripts/summarize_metrics.sh
