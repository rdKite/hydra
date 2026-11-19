#!/usr/bin/env bash
# Scripted live run for the test tools (debug panel and its console commands, phase-1.md
# section 27): the commands are driven on the host and on the client, and the result is
# checked on both. Same mechanics as Scripts/run_live.sh.
#
# Usage: Scripts/run_live_debug.sh     Needs a built editor target, the arena map and the
#                                      imported slime (Scripts/build_art.sh).
# Exit code 0 = every check passed.
# Output: Saved/Logs/DebugHost.log, Saved/Logs/DebugClient.log, Saved/Screenshots/LiveDebug/*.png
#
# What happens when (seconds on the dev clock). H = host (starts as a humanoid), C = client
# (slime). Practice mode with the passive opponent; the real data files.
#    1.5  H: HydraSetForm slime, then level 12, slam 7 with a half-full bar, projectile 40
#         (cut to the level), fire imprint 2, a milestone
#    3    C: level 20, projectile 15 with a half-full bar
#    4.3  H: HydraClearEnemies, HydraSpawn grunt 8 passive
#    5.5  H: spends stamina, HydraReset
#    6    H: opens the panel and drives it like the keys: level + 1, slam + 1; an attack
#         while it is open is ignored (game input is paused); panel closed again
#    6    C: HydraSpawn ember_slime 3 passive
#    9    H: waves on, then off
#   10.2  H: body preset and leg length of every character (looks of the humanoid base model)
#   11    quit
set -u
source "$(dirname "$0")/env.sh"
PYTHON="$UE/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
TIMEOUT_S="${HYDRA_LIVE_TIMEOUT:-300}"

[ -f "$ROOT/Content/Maps/Arena.umap" ] || { echo "Content/Maps/Arena.umap missing - run Scripts/create_arena_map.sh"; exit 1; }

rm -rf "$ROOT/Saved/Screenshots/LiveDebug"
mkdir -p "$ROOT/Saved/Logs"
rm -f "$ROOT/Saved/Logs/DebugHost.log" "$ROOT/Saved/Logs/DebugClient.log"

H="1:HydraStats;1.5:HydraSetForm|slime;2:HydraStats;2.3:HydraSetLevel|12;2.6:HydraSetSkill|slam|7|0.5;2.9:HydraSetSkill|projectile|40"
H+=";3.2:HydraSetImprint|2;3.5:HydraMilestone;4:HydraStats;4.3:HydraClearEnemies;4.6:HydraSpawn|grunt|8|1;5.2:HydraStats"
H+=";5.5:HydraSpendStamina|300;5.6:HydraReset;5.7:HydraStats"
H+=";6:HydraPanel|1;6.2:HydraPanelKey|down;6.4:HydraPanelKey|right;6.5:HydraStats;6.6:HydraPanelKey|down;6.8:HydraPanelKey|right;7.5:HydraAttack|0;8.4:HydraPanel|0;8.6:HydraStats"
H+=";9:HydraWaves|1;9.6:HydraWaves|0;10:HydraStats;10.2:HydraBody|male;10.3:HydraLegs|0.8;10.7:HydraStats"

C="1:HydraStats;2:HydraStats;3:HydraSetLevel|20;3.3:HydraSetSkill|projectile|15|0.5;4:HydraStats;5.2:HydraStats;6:HydraSpawn|ember_slime|3|1;6.5:HydraStats;8.6:HydraStats;10:HydraStats;10.7:HydraStats"

COMMON=(-game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound -HydraWaitPlayers=2 -HydraQuit=11)
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'

"$EDITOR" "$PROJ" "/Game/Maps/Arena?listen" "${COMMON[@]}" -WinX=0 -WinY=40 -log=DebugHost.log -HydraAIPassive \
  -HydraShotDir=LiveDebug -HydraShotName=host -HydraShot=0.3,7.2,10.3 "-HydraExec=$H" > /dev/null 2>&1 &
HOST=$!
sleep 6
"$EDITOR" "$PROJ" "127.0.0.1?Form=slime" "${COMMON[@]}" -WinX=640 -WinY=120 -log=DebugClient.log \
  -HydraShotDir=LiveDebug -HydraShotName=client -HydraShot=0.3,6.8 "-HydraExec=$C" > /dev/null 2>&1 &
CLIENT=$!

( sleep "$TIMEOUT_S"; kill $HOST $CLIENT 2> /dev/null ) > /dev/null 2>&1 &
WATCHDOG=$!
wait $HOST $CLIENT
kill $WATCHDOG 2> /dev/null

"$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live_debug.py")" "$(cygpath -w "$ROOT")"
