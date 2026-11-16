#!/usr/bin/env bash
# Scripted live run for the slime and the magic minimum (M3): the host plays the slime, the
# client a humanoid, the AI opponent is passive. Same mechanics as Scripts/run_live.sh.
#
# Usage: Scripts/run_live_slime.sh     Needs a built editor target, the arena map and the
#                                      imported slime (Scripts/build_art.sh).
# Exit code 0 = every check passed.
# Output: Saved/Logs/SlimeHost.log, Saved/Logs/SlimeClient.log, Saved/Screenshots/LiveSlime/*.png
#
# What happens when (seconds on the dev clock). AI stands at (0, 600) facing -Y.
#    1    baseline: forms, sizes, bars
#    2.5  slime slams the AI (light), 4 charged slam
#    6    AI hits the slime with its sword: slash is resisted
#    7.5  bolt without an element, 9 fully charged bolt from 6 m
#   11    fire awakened: 11.5 fire bolt - burning on the AI, fire field under it;
#         12.6 the client steps into the field and burns too; the stacks drop after 10 s
#   24    AI blocks: the fire bolt bursts in front of the shield
#   27.5  cast aimed 90 degrees to the side: the slime turns at its turn rate, the bolt
#         leaves along its facing
#   29.5  quit
set -u
source "$(dirname "$0")/env.sh"
PYTHON="$UE/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
TIMEOUT_S="${HYDRA_LIVE_TIMEOUT:-300}"

[ -f "$ROOT/Content/Maps/Arena.umap" ] || { echo "Content/Maps/Arena.umap missing - run Scripts/create_arena_map.sh"; exit 1; }
[ -f "$ROOT/Content/Characters/slime/SK_slime.uasset" ] || { echo "Content/Characters/slime missing - run Scripts/build_art.sh"; exit 1; }

# The run reads a copy of data/ in which everyone starts at level 0 without skill levels and
# the imprint gives no bonus: it measures the plain rules (see check_live.py --plain-start).
LIVE="$ROOT/Saved/LiveSlime"
rm -rf "$LIVE" "$ROOT/Saved/Screenshots/LiveSlime"
mkdir -p "$LIVE/data" "$ROOT/Saved/Logs"
cp "$ROOT"/data/*.json "$LIVE/data/"
"$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live.py")" --plain-start "$(cygpath -w "$LIVE/data/values.json")" > /dev/null
rm -f "$ROOT/Saved/Logs/SlimeHost.log" "$ROOT/Saved/Logs/SlimeClient.log"

H="1:HydraStats;1.5:HydraTeleport|0|500|90;2.5:HydraAttack|0;3.2:HydraStats;4:HydraAttack|3000;5.7:HydraStats"
H+=";6:HydraAI|attack|0;6.8:HydraStats"
H+=";7:HydraResetAll;7.5:HydraOffHand|0;8.3:HydraStats;8.5:HydraTeleport|0|0|90;9:HydraOffHand|3000;9.8:HydraStats;10.8:HydraStats"
H+=";11:HydraFire|1;11:HydraResetAll;11.5:HydraOffHand|0;14:HydraStats;23.5:HydraStats"
H+=";24:HydraResetAll;24:HydraAI|block|1;24.5:HydraOffHand|0;26:HydraStats;26.5:HydraAI|block|0"
H+=";27:HydraTeleport|0|0|0;27.2:HydraLook|90|-10;27.5:HydraOffHand|0;28.8:HydraStats"

C="1:HydraStats;3.2:HydraStats;10.8:HydraStats;12.6:HydraTeleport|80|600|180;14:HydraStats;16:HydraTeleport|400|0|180;23.5:HydraStats;26:HydraStats;27.9:HydraTeleport|250|0|180"

COMMON=(-game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound "-HydraDataDir=$(cygpath -w "$LIVE/data")" -HydraWaitPlayers=2 -HydraQuit=29.5)
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'

"$EDITOR" "$PROJ" "/Game/Maps/Arena?listen?Form=slime" "${COMMON[@]}" -WinX=0 -WinY=40 -log=SlimeHost.log -HydraAIPassive \
  -HydraShotDir=LiveSlime -HydraShotName=host -HydraShot=0.3,9.9,12.4,25.4 "-HydraExec=$H" > /dev/null 2>&1 &
HOST=$!
sleep 6
"$EDITOR" "$PROJ" 127.0.0.1 "${COMMON[@]}" -WinX=640 -WinY=120 -log=SlimeClient.log \
  -HydraShotDir=LiveSlime -HydraShotName=client -HydraShot=0.3,5.8,13.2,28.6 "-HydraExec=$C" > /dev/null 2>&1 &
CLIENT=$!

( sleep "$TIMEOUT_S"; kill $HOST $CLIENT 2> /dev/null ) > /dev/null 2>&1 &
WATCHDOG=$!
wait $HOST $CLIENT
kill $WATCHDOG 2> /dev/null

"$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live_slime.py")" "$(cygpath -w "$ROOT")"
