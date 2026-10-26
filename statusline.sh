#!/usr/bin/env bash
# Claude Code status line: shows which cc profile this session is signed in as,
# plus model, directory and git branch. Claude Code pipes session JSON on stdin.
input="$(cat)"

CC_HOME="${CC_HOME:-$HOME/.config/cc}"
profile="default"
case "${CLAUDE_CONFIG_DIR:-}" in
  "$CC_HOME"/profiles/*) profile="$(basename "$CLAUDE_CONFIG_DIR")" ;;
  "") ;;
  *) profile="$(basename "$CLAUDE_CONFIG_DIR")" ;;
esac

# Colour per profile so WORK and PERSONAL never look alike.
# Override by putting a 256-colour number in <profile dir>/.cc-color
color=""
[ -n "${CLAUDE_CONFIG_DIR:-}" ] && [ -f "$CLAUDE_CONFIG_DIR/.cc-color" ] && color="$(tr -dc '0-9' < "$CLAUDE_CONFIG_DIR/.cc-color")"
if [ -z "$color" ]; then
  case "$profile" in
    work|*work*|job|office)      color=196 ;;  # red
    personal|home|me|*personal*) color=34  ;;  # green
    default)                     color=244 ;;  # grey
    *)                           color=33  ;;  # blue
  esac
fi

if command -v python3 >/dev/null 2>&1; then
  read -r model cwd <<<"$(printf '%s' "$input" | python3 -c '
import json, sys
try:
    d = json.load(sys.stdin)
except Exception:
    d = {}
m = (d.get("model") or {}).get("display_name") or ""
c = (d.get("workspace") or {}).get("current_dir") or (d.get("cwd") or "")
print(m.replace(" ", "_") or "-", c or "-")
')"
else
  model="$(printf '%s' "$input" | sed -n 's/.*"display_name":"\([^"]*\)".*/\1/p' | head -1)"
  cwd="$(printf '%s' "$input" | sed -n 's/.*"current_dir":"\([^"]*\)".*/\1/p' | head -1)"
fi
model="${model//_/ }"
[ "$model" = "-" ] && model=""
[ "$cwd" = "-" ] && cwd=""

branch=""
if [ -n "$cwd" ] && [ -d "$cwd" ]; then
  branch="$(git -C "$cwd" symbolic-ref --short HEAD 2>/dev/null || true)"
fi

upper="$(printf '%s' "$profile" | tr '[:lower:]' '[:upper:]')"
esc=$'\033'
out="${esc}[1;38;5;${color}m[${upper}]${esc}[0m"
[ -n "$model" ]  && out="$out ${esc}[2m$model${esc}[0m"
[ -n "$cwd" ]    && out="$out ${esc}[36m$(basename "$cwd")${esc}[0m"
[ -n "$branch" ] && out="$out ${esc}[35m($branch)${esc}[0m"
printf '%s' "$out"
