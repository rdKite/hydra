#!/usr/bin/env bash
# One-time setup per machine for the humanoid base model (Art/scripts/build_humanoid.py):
# installs the MPFB2 extension (MakeHuman for Blender, GPL; only a tool, nothing of its code
# ships with the game) and the MakeHuman system assets (CC0: skins, eyes, eyebrows, eyelashes)
# into the user's Blender. Downloads are pinned by SHA-256. Running it again is harmless.
#
# Usage: Scripts/setup_mpfb.sh      (BLENDER can point to blender.exe; default: newest in
#                                    C:\Program Files\Blender Foundation)
set -eu
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BLENDER="${BLENDER:-$(ls -d /c/Program\ Files/Blender\ Foundation/Blender*/blender.exe 2> /dev/null | sort -V | tail -1)}"
[ -x "$BLENDER" ] || { echo "Blender not found. Install it or set BLENDER to the path of blender.exe." >&2; exit 2; }

MPFB_URL="https://extensions.blender.org/download/sha256:c075b3e9b94b8e5556a5f03f4653602d02ea306de08126e8f55ed6dee28a2dad/add-on-mpfb-v2.0.18.zip"
MPFB_SHA="c075b3e9b94b8e5556a5f03f4653602d02ea306de08126e8f55ed6dee28a2dad"
ASSETS_URL="https://files.makehumancommunity.org/asset_packs/makehuman_system_assets/makehuman_system_assets_cc0.zip"
ASSETS_SHA="b542127a8e25547c7c29c19f2d1d2adb9a664c80396ecd694095dbc8028a0107"

CACHE="$ROOT/Saved/Downloads"
mkdir -p "$CACHE"
fetch() {   # url sha file
  if [ ! -f "$3" ] || [ "$(sha256sum "$3" | cut -d' ' -f1)" != "$2" ]; then
    echo "Downloading $(basename "$3") ..."
    curl -sSL -A "Blender" -o "$3" "$1"
  fi
  [ "$(sha256sum "$3" | cut -d' ' -f1)" = "$2" ] || { echo "Checksum mismatch for $3" >&2; exit 1; }
}
fetch "$MPFB_URL" "$MPFB_SHA" "$CACHE/mpfb-2.0.18.zip"
fetch "$ASSETS_URL" "$ASSETS_SHA" "$CACHE/makehuman_system_assets_cc0.zip"

"$BLENDER" -c extension install-file -r user_default -e "$(cygpath -w "$CACHE/mpfb-2.0.18.zip")" > "$CACHE/install_mpfb.log" 2>&1 \
  || { echo "Installing MPFB failed (see $CACHE/install_mpfb.log)"; exit 1; }
"$BLENDER" -b --python-exit-code 1 --python "$(cygpath -w "$ROOT/Art/scripts/install_mpfb_assets.py")" -- "$(cygpath -w "$CACHE/makehuman_system_assets_cc0.zip")" > "$CACHE/install_assets.log" 2>&1 \
  || { echo "Installing the MakeHuman assets failed (see $CACHE/install_assets.log)"; exit 1; }
grep -h "install_mpfb_assets:" "$CACHE/install_assets.log"
echo "MPFB and the MakeHuman system assets are installed."
