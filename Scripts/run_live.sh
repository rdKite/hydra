#!/usr/bin/env bash
# Scripted live run: starts a host (listen server) and a client window, drives both with
# automated input, takes screenshots and checks the result from the logs.
#
# Usage: Scripts/run_live.sh           Needs a built editor target and the arena map
#                                      (Scripts/build.sh, Scripts/create_arena_map.sh).
# Exit code 0 = every check passed.
# Output: Saved/Logs/LiveHost.log, Saved/Logs/LiveClient.log, Saved/Screenshots/Live/*.png
#
# Don't click into the windows while it runs (the first click captures the mouse).
# The run takes about a minute. Both windows read a copy of data/ (Saved/Live/data), so the
# script can change values during the run without touching the real files.
#
# What happens when (seconds on the dev clock, which starts when both players are in):
#    1  baseline            2  both spend 40 stamina    2.3 / 3 / 5  cost, delay, regeneration
#    6  both move           8  positions                8.5  host: aim assist on the dummy
#   10  both lock on        11.5 lock-on state, screenshots   12.5 release
#   14  host: latency step 100 ms     16  client: packet loss off     19  measured ping
#   20  latency off         (meanwhile the script edits the copied values.json)
#   23  host: Hydra.ReloadValues      24  new stamina maximum on both sides
#   24.5 host: camera to the dummy, 25 lock on with the lowered turn rate
#   25.5 camera is on target, character still turning    27.5 character has caught up
#   29  quit
set -u
source "$(dirname "$0")/env.sh"
PYTHON="$UE/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
TIMEOUT_S="${HYDRA_LIVE_TIMEOUT:-240}"

[ -f "$ROOT/Content/Maps/Arena.umap" ] || { echo "Content/Maps/Arena.umap missing - run Scripts/create_arena_map.sh"; exit 1; }

LIVE="$ROOT/Saved/Live"
rm -rf "$LIVE" "$ROOT/Saved/Screenshots/Live"
mkdir -p "$LIVE/data" "$ROOT/Saved/Logs"
cp "$ROOT"/data/*.json "$LIVE/data/"
rm -f "$ROOT/Saved/Logs/LiveHost.log" "$ROOT/Saved/Logs/LiveClient.log"
DATA_DIR="$(cygpath -w "$LIVE/data")"

HOST_EXEC="1:HydraStats;2:HydraSpendStamina|40;2.3:HydraStats;3:HydraStats;5:HydraStats"
HOST_EXEC+=";6:HydraMove|1|0.55|1;8:HydraStats;8.5:HydraAimAssist;9:HydraStats"
HOST_EXEC+=";10:HydraLockOn;11.5:HydraStats;12.5:HydraLockOn;13:HydraStats"
HOST_EXEC+=";14:Hydra.LatencyStep|100;19:HydraStats;20:Hydra.LatencyStep|0"
HOST_EXEC+=";23:Hydra.ReloadValues;24:HydraStats;24.5:HydraLook|45|-10;25:HydraLockOn;25.5:HydraStats;27.5:HydraStats"

CLIENT_EXEC="1:HydraStats;2:HydraSpendStamina|40;2.3:HydraStats;5:HydraStats"
CLIENT_EXEC+=";6:HydraMove|0|-1|1;8:HydraStats;9:HydraStats"
CLIENT_EXEC+=";10:HydraLockOn;11.5:HydraStats;12.5:HydraLockOn;13:HydraStats"
CLIENT_EXEC+=";16:Hydra.PacketLoss|0;19:HydraStats;24:HydraStats;27.5:HydraStats"

COMMON=(-game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound "-HydraDataDir=$DATA_DIR" -HydraWaitPlayers=2 -HydraQuit=29)

# Git Bash would rewrite arguments that look like Unix paths (/Game/Maps/Arena).
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'

"$EDITOR" "$PROJ" "/Game/Maps/Arena?listen" "${COMMON[@]}" -WinX=0 -WinY=40 -log=LiveHost.log \
  -HydraShotName=host -HydraShot=11.6,27.6 "-HydraExec=$HOST_EXEC" > /dev/null 2>&1 &
HOST=$!
sleep 6
"$EDITOR" "$PROJ" 127.0.0.1 "${COMMON[@]}" -WinX=640 -WinY=120 -log=LiveClient.log \
  -HydraShotName=client -HydraShot=11.7,27.7 "-HydraExec=$CLIENT_EXEC" > /dev/null 2>&1 &
CLIENT=$!

# Watchdog: never hang (first start compiles shaders and can take a while).
( sleep "$TIMEOUT_S"; kill $HOST $CLIENT 2> /dev/null ) &
WATCHDOG=$!

# While the run is between its first part and the reload at 23 s: change the copied values.
(
  for _ in $(seq 1 "$TIMEOUT_S"); do
    grep -q "HydraDev: clock started" "$ROOT/Saved/Logs/LiveHost.log" 2> /dev/null && break
    sleep 1
  done
  sleep 10
  "$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live.py")" --edit-values "$(cygpath -w "$LIVE/data/values.json")"
) &
EDITOR_JOB=$!

wait $HOST $CLIENT
kill $WATCHDOG $EDITOR_JOB 2> /dev/null

"$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live.py")" "$(cygpath -w "$ROOT")"
