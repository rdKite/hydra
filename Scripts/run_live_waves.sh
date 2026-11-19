#!/usr/bin/env bash
# Scripted live run for progression and waves (M4): the host plays the slime, the client a
# humanoid, wave mode. Same mechanics as Scripts/run_live.sh.
#
# Usage: Scripts/run_live_waves.sh     Needs a built editor target, the arena map and the
#                                      imported slime (Scripts/build_art.sh).
# Exit code 0 = every check passed.
# Output: Saved/Logs/WavesHost.log, Saved/Logs/WavesClient.log,
#         Saved/Screenshots/LiveWaves/*.png, Saved/LiveWaves/metrics/metrics_*.csv
#
# The run reads a copy of data/ (Saved/LiveWaves/data) with a few numbers changed so that it
# is short and the same every time: two small waves, low thresholds for level, skill and
# imprint, weak humanoids (check_live_waves.py --edit). The rules are the game's; the checker
# works out what it expects from that copy. The enemies start passive and are switched to
# active at the end.
#
# What happens when (seconds on the dev clock; the first wave is already there at 0).
# H = host (slime), C = client (humanoid).
#    1    baseline: start levels, skill levels, wave 1 = one grunt at (-1500, 0)
#    2.2  the grunt attacks, H dodges out of the hit area: counts for the dodge skill
#    4    H slams the grunt twice: defeated. Wave cleared: skill XP credited, then nXP -
#         H reaches level 30 (cap, milestone), its slam skill rises; C levels up too
#    9.5  wave 2: an ember slime (element body) at (-1300, -1300) and a grunt
#   10.6  C hits the ember slime twice: fire damage back = contact with fire; 12.5 C starts
#         its fire imprint at its open milestone
#   13.2  everyone back to full health; 14 H slams it twice: imprint earned; 16 H evolves
#         into the fire elemental slime (level 0, skills 0 with instinct, fire imprint 1)
#   17    H defeats the ember slime with two more slams
#   19.5  enemies active: the grunt comes for the players
#   30    quit (the game mode writes the last metrics rows)
set -u
source "$(dirname "$0")/env.sh"
PYTHON="$UE/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
TIMEOUT_S="${HYDRA_LIVE_TIMEOUT:-300}"

[ -f "$ROOT/Content/Maps/Arena.umap" ] || { echo "Content/Maps/Arena.umap missing - run Scripts/create_arena_map.sh"; exit 1; }
[ -f "$ROOT/Content/Characters/slime/SK_slime.uasset" ] || { echo "Content/Characters/slime missing - run Scripts/build_art.sh"; exit 1; }

LIVE="$ROOT/Saved/LiveWaves"
rm -rf "$LIVE" "$ROOT/Saved/Screenshots/LiveWaves"
mkdir -p "$LIVE/data" "$LIVE/metrics" "$ROOT/Saved/Logs"
cp "$ROOT"/data/*.json "$LIVE/data/"
rm -f "$ROOT/Saved/Logs/WavesHost.log" "$ROOT/Saved/Logs/WavesClient.log"
"$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live_waves.py")" --edit "$(cygpath -w "$LIVE/data/values.json")" > /dev/null || exit 1

# In front of the grunt, 50 degrees to its right: the step back from there is clear of the cover.
H="1:HydraStats;1.5:HydraTeleport|-1416|-100|130;2.2:HydraAI|attack|0;2.25:HydraDodge;3.2:HydraStats"
H+=";3.5:HydraTeleport|-1416|-100|130;4:HydraAttack|0;4.9:HydraAttack|0;6.5:HydraStats;9.5:HydraStats"
H+=";13.2:HydraResetAll;13.5:HydraTeleport|-1215|-1215|225;14:HydraAttack|0;14.9:HydraAttack|0;15.8:HydraStats"
H+=";16:HydraChoose|evolve;16.6:HydraStats;17:HydraAttack|0;17.9:HydraAttack|0;19:HydraStats;19.5:Hydra.AI|1;29:HydraStats"

C="1:HydraStats;6.5:HydraStats;9.5:HydraStats;10:HydraTeleport|-1150|-1300|180;10.6:HydraAttack|0;11.5:HydraAttack|0;12.2:HydraStats"
C+=";12.5:HydraChoose|imprint;13:HydraStats;16.6:HydraStats;19:HydraStats;29:HydraStats"

COMMON=(-game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound "-HydraDataDir=$(cygpath -w "$LIVE/data")" -HydraWaitPlayers=2 -HydraQuit=30)
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'

"$EDITOR" "$PROJ" "/Game/Maps/Arena?listen?Form=slime?Waves=1" "${COMMON[@]}" -WinX=0 -WinY=40 -log=WavesHost.log -HydraAIPassive \
  "-HydraMetricsDir=$(cygpath -w "$LIVE/metrics")" -HydraShotDir=LiveWaves -HydraShotName=host -HydraShot=0.3,7,15.9,16.8,25.5 "-HydraExec=$H" > /dev/null 2>&1 &
HOST=$!
sleep 6
"$EDITOR" "$PROJ" 127.0.0.1 "${COMMON[@]}" -WinX=640 -WinY=120 -log=WavesClient.log \
  -HydraShotDir=LiveWaves -HydraShotName=client -HydraShot=0.3,13.2,16.9 "-HydraExec=$C" > /dev/null 2>&1 &
CLIENT=$!

( sleep "$TIMEOUT_S"; kill $HOST $CLIENT 2> /dev/null ) > /dev/null 2>&1 &
WATCHDOG=$!
wait $HOST $CLIENT
kill $WATCHDOG 2> /dev/null

"$PYTHON" "$(cygpath -w "$ROOT/Scripts/check_live_waves.py")" "$(cygpath -w "$ROOT")"
