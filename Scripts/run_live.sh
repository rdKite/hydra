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
# The run takes about a minute and a half. Both windows read a copy of data/
# (Saved/Live/data), so the script can change values during the run without touching the
# real files. The AI opponent starts passive and is driven by dev commands, so every
# situation is the same in every run; it is switched to active at the end.
# Screenshots are only taken in quiet moments or long phases: the first one of a process
# stalls it for a few hundred milliseconds (hence the warm-up shot at 0.3 s).
#
# What happens when (seconds on the dev clock, which starts when both players are in).
# H = host, C = client, AI stands at (0, 600) facing -Y; "front" = (0, 440), "back" = (0, 760).
#    1    baseline
#    2.5  H: three light attacks from the front (chain), 7.2 a fourth after the chain ended
#    5    C: attack into the air (miss), 6 dodge, 9 block
#    8.5  H: heavy attack released early, 10.5 heavy attack fully charged (telegraph screenshots)
#   12.4  H: jump
#   13.6  H: light attack on the back (weak point), 14.5 charged heavy on the back: AI dies, respawns
#   20.5  AI blocks: H light and heavy into the block (block cost, rebound), 24 from behind,
#   25.6  block break at empty stamina
#   27.5  AI charges a heavy attack: a light hit does not interrupt it (AI hits H),
#   30    again: a heavy hit interrupts it
#   33    AI attacks, H dodges out of the hit area
#   35    lock-on, 37.5 aim assist
#   39    latency step 150 ms: H runs away from C, C's attack is timed by what C sees
#         (lag compensation: position)
#   47    H faces C and raises its block 170 ms after it sees C's attack start - too late
#         for C to have seen it (lag compensation: block arc), 49.2 latency off
#   50    Hydra.ReloadValues with edited values (bigger stamina bar, low turn rate):
#         aim assist and lock-on turn at the turn rate; a raised block faces the camera
#         direction (no lock-on needed) and turns slower
#   59    precise movement: backwards with the facing kept, then without it (turns around)
#   63    AI active for a few seconds
#   71    quit
set -u
source "$(dirname "$0")/env.sh"
PYTHON="$UE/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
TIMEOUT_S="${HYDRA_LIVE_TIMEOUT:-300}"

[ -f "$ROOT/Content/Maps/Arena.umap" ] || { echo "Content/Maps/Arena.umap missing - run Scripts/create_arena_map.sh"; exit 1; }

LIVE="$ROOT/Saved/Live"
rm -rf "$LIVE" "$ROOT/Saved/Screenshots/Live"
mkdir -p "$LIVE/data" "$ROOT/Saved/Logs"
cp "$ROOT"/data/*.json "$LIVE/data/"
rm -f "$ROOT/Saved/Logs/LiveHost.log" "$ROOT/Saved/Logs/LiveClient.log"
DATA_DIR="$(cygpath -w "$LIVE/data")"

H="1:HydraStats;1.5:HydraTeleport|0|440|90;2:HydraStats"
H+=";2.5:HydraAttack|0;3.3:HydraAttack|0;4.1:HydraAttack|0;4.6:HydraStats;7:HydraStats;7.2:HydraAttack|0;8:HydraResetAll"
H+=";8.5:HydraAttack|300;9.3:HydraStats;10:HydraResetAll;10.5:HydraAttack|3000;11:HydraStats;12.4:HydraJump;12.75:HydraStats"
H+=";13:HydraResetAll;13:HydraTeleport|0|760|-90;13.6:HydraAttack|0;14.5:HydraAttack|700;17:HydraStats;19.8:HydraStats"
H+=";20:HydraResetAll;20:HydraTeleport|0|440|90;20.5:HydraAI|block|1;21:HydraStats;21.2:HydraAttack|0;22.2:HydraAttack|300;23.2:HydraStats"
H+=";23.5:HydraTeleport|0|760|-90;24:HydraAttack|0"
H+=";25:HydraTeleport|0|440|90;25:HydraAI|stamina|5;25.6:HydraAttack|0;26.6:HydraAI|block|0"
H+=";27:HydraResetAll;27.5:HydraAI|attack|3000;27.7:HydraAttack|0;29.5:HydraResetAll"
H+=";30:HydraAI|attack|3000;30.2:HydraAttack|300;32:HydraStats"
H+=";32.5:HydraResetAll;33:HydraAI|attack|0;33.05:HydraDodge;34:HydraStats"
H+=";35:HydraLockOn;36:HydraStats;36.2:HydraLockOn"
H+=";37:HydraResetAll;37:HydraTeleport|0|440|70;37.5:HydraAttack|0;38.3:HydraStats"
H+=";39:Hydra.LatencyStep|150;39.5:HydraResetAll;39.5:HydraTeleport|-1000|-1000|180;43.5:HydraStats;45:HydraLook|180|-10;45:HydraMove|0|1|1.5;46.8:HydraStats"
H+=";47:HydraResetAll;47:HydraTeleport|-1060|-1000|0;47:HydraLook|0|-10;47.3:HydraAutoBlock|170;49.2:Hydra.LatencyStep|0"
H+=";50:Hydra.ReloadValues;51:HydraStats;51.2:HydraResetAll;51.2:HydraTeleport|0|440|62;52:HydraAttack|0;53:HydraStats"
H+=";53.5:HydraTeleport|0|440|-90;53.8:HydraStats;54:HydraLook|90|-10;54.3:HydraLockOn;55.3:HydraStats;55.6:HydraLockOn"
H+=";56:HydraTeleport|0|440|-90;56:HydraLook|-90|-10;56.3:HydraBlock|1;56.8:HydraStats;57.3:HydraLook|90|-10;58.3:HydraStats;58.6:HydraBlock|0"
H+=";59:HydraResetAll;59:HydraTeleport|-1000|-1000|0;59:HydraLook|0|-10;59.3:HydraPrecise|1;59.4:HydraStats;59.5:HydraMove|0|-1|1;60.8:HydraStats"
H+=";61:HydraPrecise|0;61.1:HydraMove|0|-1|1;62.4:HydraStats"
H+=";63:HydraResetAll;63:HydraTeleport|0|0|90;63.2:Hydra.AI|1;67:HydraStats;70.5:HydraStats"

C="1:HydraStats;2:HydraStats;4.6:HydraStats;5:HydraAttack|0;5.4:HydraStats;6:HydraDodge;7:HydraStats;9:HydraBlock|1;9.5:HydraStats;10:HydraBlock|0"
C+=";11:HydraStats;12.75:HydraStats;17:HydraStats;19.8:HydraStats;21:HydraStats;38.3:HydraStats"
C+=";39.5:HydraTeleport|-900|-1000|180;41:Hydra.PacketLoss|0;43.5:HydraStats;44:HydraAutoAttack|170;46.8:HydraStats;48:HydraAttack|0;51:HydraStats;60.8:HydraStats;67:HydraStats"

COMMON=(-game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound "-HydraDataDir=$DATA_DIR" -HydraWaitPlayers=2 -HydraQuit=71)

# Git Bash would rewrite arguments that look like Unix paths (/Game/Maps/Arena).
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'

"$EDITOR" "$PROJ" "/Game/Maps/Arena?listen" "${COMMON[@]}" -WinX=0 -WinY=40 -log=LiveHost.log -HydraAIPassive \
  -HydraShotName=host -HydraShot=0.3,11.2,17.2,20.8,36.1,70 "-HydraExec=$H" > /dev/null 2>&1 &
HOST=$!
sleep 6
"$EDITOR" "$PROJ" 127.0.0.1 "${COMMON[@]}" -WinX=640 -WinY=120 -log=LiveClient.log \
  -HydraShotName=client -HydraShot=0.3,11.3,20.9,70.1 "-HydraExec=$C" > /dev/null 2>&1 &
CLIENT=$!

# Watchdog: never hang (first start compiles shaders and can take a while).
( sleep "$TIMEOUT_S"; kill $HOST $CLIENT 2> /dev/null ) &
WATCHDOG=$!

# While the run is in its first part (the reload is at 50 s): change the copied values.
(
  for _ in $(seq 1 "$TIMEOUT_S"); do
    grep -q "HydraDev: clock started" "$ROOT/Saved/Logs/LiveHost.log" 2> /dev/null && break
    sleep 1
  done
  sleep 15
  "$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live.py")" --edit-values "$(cygpath -w "$LIVE/data/values.json")"
) &
EDITOR_JOB=$!

wait $HOST $CLIENT
kill $WATCHDOG $EDITOR_JOB 2> /dev/null

"$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live.py")" "$(cygpath -w "$ROOT")"
