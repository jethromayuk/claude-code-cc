#!/usr/bin/env bash
# Installs cc into ~/.local/bin and the helper files into ~/.config/cc,
# then wires cc.zsh into ~/.zshrc. Safe to re-run.
set -eu
here="$(cd "$(dirname "$0")" && pwd)"
CC_HOME="${CC_HOME:-$HOME/.config/cc}"
BIN="$HOME/.local/bin"
ZSHRC="${ZDOTDIR:-$HOME}/.zshrc"

mkdir -p "$BIN" "$CC_HOME/profiles"
install -m 755 "$here/cc" "$BIN/cc"
install -m 755 "$here/statusline.sh" "$CC_HOME/statusline.sh"
install -m 644 "$here/cc.zsh" "$CC_HOME/cc.zsh"

line='source "$HOME/.config/cc/cc.zsh"'
if ! grep -Fq 'cc/cc.zsh' "$ZSHRC" 2>/dev/null; then
  printf '\n# Claude Code profile switcher\n%s\n' "$line" >> "$ZSHRC"
  echo "added to $ZSHRC: $line"
fi
case ":$PATH:" in
  *":$BIN:"*) ;;
  *) printf 'export PATH="$HOME/.local/bin:$PATH"\n' >> "$ZSHRC"; echo "added ~/.local/bin to PATH in $ZSHRC" ;;
esac

cat <<MSG

Installed. Next:
  exec zsh                 # reload your shell (or open a new Warp tab)
  cc add work              # sign in with your work account
  cc add personal          # sign in with your personal account
  cc use personal          # pick which one plain 'claude' uses
  claude-work / claude-personal   # or be explicit, any time
MSG
