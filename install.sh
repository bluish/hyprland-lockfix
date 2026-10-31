#!/bin/sh
# Build lockfix.so against the installed Hyprland headers.
set -e
cd "$(dirname "$0")"
make -B
cat <<MSG
Built $PWD/lockfix.so. Load it now with:
  hyprctl plugin load $PWD/lockfix.so
and on every start from ~/.config/hypr/hyprland.lua (see examples/hyprland.lua).
MSG
