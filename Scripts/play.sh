#!/usr/bin/env bash
# Starts the game for playing by hand (no automation): host mode with a second client window.
#
# Usage: Scripts/play.sh            host window plus one client window on this machine
#        Scripts/play.sh host       only the host (listen server)
#        Scripts/play.sh client     only a client that joins 127.0.0.1
#        Further arguments go to the game, e.g. Scripts/play.sh both -HydraQuit=20
#
# Needs a built editor target and the arena map (Scripts/build.sh, Scripts/create_arena_map.sh).
# Logs: Saved/Logs/PlayHost.log, Saved/Logs/PlayClient.log
set -u
source "$(dirname "$0")/env.sh"
MODE="${1:-both}"

[ -f "$ROOT/Content/Maps/Arena.umap" ] || { echo "Content/Maps/Arena.umap missing - run Scripts/create_arena_map.sh"; exit 1; }

# Git Bash would rewrite arguments that look like Unix paths (/Game/Maps/Arena).
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'
WINDOW=(-game -windowed -ResX=1280 -ResY=720 -nosplash "${@:2}")

if [ "$MODE" = both ] || [ "$MODE" = host ]; then
  "$EDITOR" "$PROJ" "/Game/Maps/Arena?listen" "${WINDOW[@]}" -WinX=0 -WinY=40 -log=PlayHost.log > /dev/null 2>&1 &
fi
if [ "$MODE" = both ]; then
  sleep 6
fi
if [ "$MODE" = both ] || [ "$MODE" = client ]; then
  "$EDITOR" "$PROJ" 127.0.0.1 "${WINDOW[@]}" -WinX=640 -WinY=120 -log=PlayClient.log > /dev/null 2>&1 &
fi
wait
