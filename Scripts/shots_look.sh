#!/usr/bin/env bash
# Look variants side by side (M5 preparation): takes the same shots with each look variant
# (1 plain, 2 toon, 3 ink) and writes a page that shows them next to each other.
#
# Usage: Scripts/shots_look.sh        Needs a built editor target, the arena map, the imported
#                                     models and Content/Looks/PP_Look (Scripts/create_look_material.sh).
# Output: Saved/Screenshots/Look/*.png and Saved/Screenshots/Look/index.html (open it in a browser)
#
# Shots: the slime (the player) in front of a humanoid that blocks, the same humanoid winding
# up a heavy attack (the telegraph), and the arena from a corner. In the game the variants
# are switched with the console command HydraLookVariant 1|2|3 (no argument: the next one).
set -u
source "$(dirname "$0")/env.sh"
OUT="$ROOT/Saved/Screenshots/Look"
rm -rf "$OUT"
mkdir -p "$ROOT/Saved/Logs"
rm -f "$ROOT/Saved/Logs/Look.log"

# Per variant: close shot with the block, close shot with the charged heavy attack, the arena.
X="1:HydraTeleport|120|420|100;1.2:HydraAI|block|1"
T=2
SHOTS="0.3"
for LOOK in 1 2 3; do
  X+=";$T:HydraLookVariant|$LOOK;$T:HydraResetAll;$T:HydraTeleport|120|420|100;$T:HydraAI|block|1"
  SHOTS+=",$((T + 1))"
  X+=";$((T + 1)).3:HydraAI|block|0;$((T + 1)).5:HydraAI|attack|3000"
  SHOTS+=",$((T + 2)).3"
  X+=";$((T + 3)):HydraTeleport|-1650|-1650|45;$((T + 3)).1:HydraLook|45|-14"
  SHOTS+=",$((T + 4))"
  T=$((T + 5))
done

export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'
"$EDITOR" "$PROJ" "/Game/Maps/Arena?listen?Form=slime" -game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound -HydraAIPassive \
  -log=Look.log -HydraShotDir=Look -HydraShotName=shot -HydraShot=$SHOTS -HydraQuit=$((T + 1)) "-HydraExec=$X" > /dev/null 2>&1

cd "$OUT" 2> /dev/null || { echo "No screenshots were written (see Saved/Logs/Look.log)"; exit 1; }
MISSING=0
{
  echo '<!doctype html><meta charset="utf-8"><title>hydra - look variants</title>'
  echo '<style>body{font-family:sans-serif;background:#222;color:#eee;margin:16px}table{border-collapse:collapse}td,th{padding:6px;text-align:center}img{width:100%;max-width:620px;display:block}</style>'
  echo '<h1>Look variants</h1><p>The same three shots with each variant. In the game: console command <code>HydraLookVariant 1|2|3</code>.</p>'
  echo '<table><tr><th></th><th>1 plain</th><th>2 toon</th><th>3 ink</th></tr>'
  ROW=0
  for LABEL in "characters (block)" "telegraph (heavy attack charging)" "arena"; do
    echo "<tr><th>$LABEL</th>"
    for LOOK in 1 2 3; do
      BASE=$((2 + (LOOK - 1) * 5))
      case $ROW in 0) NAME="shot_$((BASE + 1)).png" ;; 1) NAME="shot_$((BASE + 2)).3.png" ;; *) NAME="shot_$((BASE + 4)).png" ;; esac
      [ -f "$NAME" ] || { echo "missing: $NAME" >&2; MISSING=1; }
      echo "<td><img src=\"$NAME\" alt=\"look $LOOK\"></td>"
    done
    echo '</tr>'
    ROW=$((ROW + 1))
  done
  echo '</table>'
} > index.html
grep -c "HydraLook look=" "$ROOT/Saved/Logs/Look.log" | sed 's/^/look switches: /'
echo "Page: $(cygpath -w "$OUT/index.html")"
exit $MISSING
