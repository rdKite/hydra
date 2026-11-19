#!/usr/bin/env bash
# Humanoid base model: every body variant in idle, block and attack, side by side (roadmap
# M5, pipeline test for the later character creation - do the joints sit right?).
#
# Usage: Scripts/shots_humanoid.sh    Needs a built editor target, the arena map and the
#                                     imported humanoid (Scripts/build_art.sh humanoid).
# Output: Saved/Screenshots/Humanoid/*.png and Saved/Screenshots/Humanoid/index.html
#
# Variants: the three body presets (shape keys imported as morph targets: neutral, male,
# female) and the neutral body with the leg bones at 80 % length (towards a dwarf). In the
# game: console commands HydraBody neutral|male|female and HydraLegs <factor>.
# The shots show the practice opponent from the front (and the player's character from
# behind): standing, block raised, heavy attack charging.
set -u
source "$(dirname "$0")/env.sh"
OUT="$ROOT/Saved/Screenshots/Humanoid"
[ -f "$ROOT/Content/Characters/humanoid/SK_humanoid.uasset" ] || { echo "Content/Characters/humanoid missing - run Scripts/build_art.sh humanoid"; exit 1; }
rm -rf "$OUT"
mkdir -p "$ROOT/Saved/Logs"
rm -f "$ROOT/Saved/Logs/Humanoid.log"

VARIANTS=("neutral 1" "male 1" "female 1" "neutral 0.8")
X="1:HydraTeleport|210|440|125"
T=2
SHOTS="0.3"
for V in "${VARIANTS[@]}"; do
  set -- $V
  X+=";$T:HydraResetAll;$T:HydraTeleport|210|440|125;$T:HydraBody|$1;$T:HydraLegs|$2"
  SHOTS+=",$((T + 1))"                                   # idle
  X+=";$((T + 1)).2:HydraAI|block|1"
  SHOTS+=",$((T + 2))"                                   # block
  X+=";$((T + 2)).2:HydraAI|block|0;$((T + 2)).5:HydraAI|attack|3000"
  SHOTS+=",$((T + 3)).3"                                 # heavy attack charging
  T=$((T + 5))
done

export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'
"$EDITOR" "$PROJ" "/Game/Maps/Arena?listen?Form=humanoid" -game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound -HydraAIPassive \
  -log=Humanoid.log -HydraShotDir=Humanoid -HydraShotName=shot -HydraShot=$SHOTS -HydraQuit=$((T + 1)) "-HydraExec=$X" > /dev/null 2>&1

cd "$OUT" 2> /dev/null || { echo "No screenshots were written (see Saved/Logs/Humanoid.log)"; exit 1; }
MISSING=0
{
  echo '<!doctype html><meta charset="utf-8"><title>hydra - humanoid base model</title>'
  echo '<style>body{font-family:sans-serif;background:#222;color:#eee;margin:16px}table{border-collapse:collapse}td,th{padding:6px;text-align:center}img{width:100%;max-width:470px;display:block}</style>'
  echo '<h1>Humanoid base model: body variants</h1><p>One mesh, one skeleton. Presets are shape keys (morph targets); the last column scales thigh and calf to 80 % at runtime. Console: <code>HydraBody neutral|male|female</code>, <code>HydraLegs 0.8</code>.</p>'
  echo '<table><tr><th></th><th>neutral</th><th>male</th><th>female</th><th>neutral, legs 80 %</th></tr>'
  ROW=0
  for LABEL in "idle" "block" "attack (heavy, charging)"; do
    echo "<tr><th>$LABEL</th>"
    for I in 0 1 2 3; do
      BASE=$((2 + I * 5))
      case $ROW in 0) NAME="shot_$((BASE + 1)).png" ;; 1) NAME="shot_$((BASE + 2)).png" ;; *) NAME="shot_$((BASE + 3)).3.png" ;; esac
      [ -f "$NAME" ] || { echo "missing: $NAME" >&2; MISSING=1; }
      echo "<td><img src=\"$NAME\" alt=\"variant $I\"></td>"
    done
    echo '</tr>'
    ROW=$((ROW + 1))
  done
  echo '</table>'
} > index.html
grep -c "HydraBodyLook who=ai" "$ROOT/Saved/Logs/Humanoid.log" | sed 's/^/body changes on the opponent: /'
echo "Page: $(cygpath -w "$OUT/index.html")"
exit $MISSING
