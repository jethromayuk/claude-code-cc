#!/usr/bin/env bash
# cc — switch Claude Code between named account profiles.
# Each profile is its own CLAUDE_CONFIG_DIR, so each has its own login,
# settings, plugins and session history. Nothing is copied between them.
set -eu

CC_HOME="${CC_HOME:-$HOME/.config/cc}"
PROFILES="$CC_HOME/profiles"
ACTIVE_FILE="$CC_HOME/active"
STATUSLINE="$CC_HOME/statusline.sh"

die()  { printf 'cc: %s\n' "$*" >&2; exit 1; }
note() { printf '%s\n' "$*" >&2; }

valid_name() { printf '%s' "$1" | grep -Eq '^[a-z0-9][a-z0-9_-]*$'; }
profile_dir() { printf '%s/%s' "$PROFILES" "$1"; }
have_profile() { [ -d "$(profile_dir "$1")" ]; }
active_name() { [ -f "$ACTIVE_FILE" ] && cat "$ACTIVE_FILE" || true; }

require_profile() {
  [ -n "${1:-}" ] || die "profile name required"
  have_profile "$1" || die "no profile '$1' (run: cc list)"
}

# Merge {"statusLine": {...}} into a profile's settings.json, creating it if needed.
install_statusline() {
  local dir="$1" settings="$1/settings.json"
  mkdir -p "$dir"
  [ -f "$settings" ] || printf '{}\n' > "$settings"
  if command -v python3 >/dev/null 2>&1; then
    python3 - "$settings" "$STATUSLINE" <<'PY'
import json, sys
path, script = sys.argv[1], sys.argv[2]
with open(path) as f:
    data = json.load(f) or {}
data["statusLine"] = {"type": "command", "command": script, "padding": 0}
with open(path, "w") as f:
    json.dump(data, f, indent=2)
    f.write("\n")
PY
  elif command -v jq >/dev/null 2>&1; then
    local tmp; tmp="$(mktemp)"
    jq --arg s "$STATUSLINE" '.statusLine = {type:"command", command:$s, padding:0}' "$settings" > "$tmp" && mv "$tmp" "$settings"
  else
    note "neither python3 nor jq found; add this to $settings by hand:"
    note "  \"statusLine\": {\"type\": \"command\", \"command\": \"$STATUSLINE\"}"
    return 0
  fi
  note "status line installed in $settings"
}

cmd_add() {
  local name="${1:-}"
  [ -n "$name" ] || die "usage: cc add <name>"
  valid_name "$name" || die "name must be lowercase letters, digits, - or _"
  have_profile "$name" && die "profile '$name' already exists"
  local dir; dir="$(profile_dir "$name")"
  mkdir -p "$dir"

  # Seed from the default ~/.claude settings so permissions, hooks etc. carry over.
  if [ -f "$HOME/.claude/settings.json" ]; then
    cp "$HOME/.claude/settings.json" "$dir/settings.json"
    note "copied ~/.claude/settings.json into profile '$name'"
  fi
  [ -f "$STATUSLINE" ] && install_statusline "$dir"

  # First profile becomes active automatically.
  [ -n "$(active_name)" ] || { mkdir -p "$CC_HOME"; printf '%s\n' "$name" > "$ACTIVE_FILE"; note "'$name' is now the active profile"; }

  note ""
  note "Launching Claude Code for profile '$name'. Sign in with the matching account,"
  note "then type /exit (or Ctrl+C twice) to come back."
  note ""
  CLAUDE_CONFIG_DIR="$dir" exec claude /login
}

cmd_use() {
  require_profile "${1:-}"
  mkdir -p "$CC_HOME"
  printf '%s\n' "$1" > "$ACTIVE_FILE"
  note "active profile: $1"
  if [ -n "${CLAUDE_CONFIG_DIR:-}" ] && [ "$CLAUDE_CONFIG_DIR" != "$(profile_dir "$1")" ]; then
    note "warning: CLAUDE_CONFIG_DIR is exported in this shell ($CLAUDE_CONFIG_DIR) and overrides the active profile"
  fi
}

cmd_which() {
  local a; a="$(active_name)"
  if [ -n "${CLAUDE_CONFIG_DIR:-}" ]; then
    case "$CLAUDE_CONFIG_DIR" in
      "$PROFILES"/*) printf '%s (from CLAUDE_CONFIG_DIR)\n' "$(basename "$CLAUDE_CONFIG_DIR")" ;;
      *) printf 'custom: %s\n' "$CLAUDE_CONFIG_DIR" ;;
    esac
  elif [ -n "$a" ]; then
    printf '%s\n' "$a"
  else
    printf 'default (~/.claude) — no active profile\n'
  fi
}

cmd_list() {
  local a d n; a="$(active_name)"
  [ -d "$PROFILES" ] || { note "no profiles yet (run: cc add <name>)"; return 0; }
  for d in "$PROFILES"/*/; do
    [ -d "$d" ] || continue
    n="$(basename "$d")"
    if [ "$n" = "$a" ]; then printf '* %s\n' "$n"; else printf '  %s\n' "$n"; fi
  done
}

cmd_run() {
  require_profile "${1:-}"
  local dir; dir="$(profile_dir "$1")"; shift
  CLAUDE_CONFIG_DIR="$dir" exec claude "$@"
}

cmd_path() {
  require_profile "${1:-}"
  profile_dir "$1"
}

cmd_rm() {
  require_profile "${1:-}"
  local dir; dir="$(profile_dir "$1")"
  printf "Delete profile '%s' and everything in %s? [y/N] " "$1" "$dir"
  read -r ans
  case "$ans" in y|Y|yes) ;; *) die "aborted" ;; esac
  rm -rf "$dir"
  [ "$(active_name)" = "$1" ] && rm -f "$ACTIVE_FILE"
  note "removed '$1'"
}

cmd_statusline() {
  [ -f "$STATUSLINE" ] || die "statusline script not found at $STATUSLINE (run install.sh)"
  if [ -n "${1:-}" ]; then
    require_profile "$1"; install_statusline "$(profile_dir "$1")"
  else
    local d
    for d in "$PROFILES"/*/; do [ -d "$d" ] && install_statusline "${d%/}"; done
  fi
}

cmd_help() {
  cat <<'H'
cc — switch Claude Code between account profiles

  cc add <name>        create a profile and sign in to it (opens claude /login)
  cc use <name>        make <name> the active profile; plain `claude` will use it
  cc which             show the active profile
  cc list              list profiles (* = active)
  cc run <name> [...]  run claude once with a specific profile
  cc path <name>       print the profile's CLAUDE_CONFIG_DIR
  cc rm <name>         delete a profile (asks first)
  cc statusline [name] (re)install the profile status line into settings.json

After `source ~/.config/cc/cc.zsh` you also get a `claude-<name>` command per profile.
Profiles live in ~/.config/cc/profiles/<name> (override with CC_HOME).
H
}

case "${1:-help}" in
  add)        shift; cmd_add "$@" ;;
  use|switch) shift; cmd_use "$@" ;;
  which|current) cmd_which ;;
  list|ls)    cmd_list ;;
  run)        shift; cmd_run "$@" ;;
  path)       shift; cmd_path "$@" ;;
  rm|remove)  shift; cmd_rm "$@" ;;
  statusline) shift; cmd_statusline "$@" ;;
  help|-h|--help) cmd_help ;;
  *) die "unknown command '$1' (try: cc help)" ;;
esac
