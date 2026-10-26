# cc.zsh — source this from ~/.zshrc
# Makes plain `claude` follow the active cc profile, and defines a
# `claude-<name>` command for every profile.

: "${CC_HOME:=$HOME/.config/cc}"
export CC_HOME

# Plain `claude` -> active profile. An explicitly exported CLAUDE_CONFIG_DIR
# always wins, so `CLAUDE_CONFIG_DIR=... claude` still works as before.
claude() {
  if [[ -z "${CLAUDE_CONFIG_DIR:-}" && -r "$CC_HOME/active" ]]; then
    local _cc_active _cc_dir
    _cc_active="$(<"$CC_HOME/active")"
    _cc_dir="$CC_HOME/profiles/$_cc_active"
    if [[ -n "$_cc_active" && -d "$_cc_dir" ]]; then
      CLAUDE_CONFIG_DIR="$_cc_dir" command claude "$@"
      return
    fi
  fi
  command claude "$@"
}

# One function per profile: claude-work, claude-personal, ...
_cc_define_profile_fns() {
  local d n
  for d in "$CC_HOME"/profiles/*(N/); do
    n="${d:t}"
    eval "claude-${n}() { CLAUDE_CONFIG_DIR=\"$d\" command claude \"\$@\"; }"
  done
}
_cc_define_profile_fns

# Completion for cc.
if (( $+functions[compdef] )); then
  _cc() {
    local -a cmds profiles
    cmds=(add use which list run path rm statusline help)
    profiles=("$CC_HOME"/profiles/*(N/:t))
    if (( CURRENT == 2 )); then
      _describe 'command' cmds
    elif (( CURRENT == 3 )); then
      case "$words[2]" in
        use|run|path|rm|statusline) _describe 'profile' profiles ;;
      esac
    fi
  }
  compdef _cc cc
fi
