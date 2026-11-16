#!/usr/bin/env bash
# Sourced by the other scripts. Resolves the engine and the project file.
# The engine path is local and comes from the environment variable UE_ROOT
# (e.g. D:\dev\Epic Games\UE_5.8); there is no default on purpose.
if [ -z "${UE_ROOT:-}" ]; then
  echo "UE_ROOT is not set. Point it to the Unreal Engine 5.8 folder, e.g." >&2
  echo "  setx UE_ROOT \"D:\dev\Epic Games\UE_5.8\"   (then open a new terminal)" >&2
  exit 2
fi
UE="$(cygpath -u "$UE_ROOT")"
if [ ! -f "$UE/Engine/Build/BatchFiles/Build.bat" ]; then
  echo "UE_ROOT does not point to an engine: $UE_ROOT" >&2
  exit 2
fi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJ="$(cygpath -w "$ROOT/Hydra.uproject")"
EDITOR="$UE/Engine/Binaries/Win64/UnrealEditor.exe"
EDITOR_CMD="$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
