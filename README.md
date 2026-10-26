# cc — Claude Code account switcher

Switch Claude Code between a **work** and a **personal** account (or any number
of accounts) without logging out and back in. Each profile is its own
`CLAUDE_CONFIG_DIR`, so logins, settings, plugins and session history never mix,
and two profiles can run side by side in different Warp tabs.

A status line shows **[WORK]** in red or **[PERSONAL]** in green at the bottom
of every session so you always know which account you are talking to.

## Install (macOS, zsh)

```sh
git clone https://github.com/jethromayuk/claude-code-cc.git ~/claude-code-cc && ~/claude-code-cc/install.sh
exec zsh
```

`install.sh` puts `cc` in `~/.local/bin`, the helpers in `~/.config/cc`, and
adds one `source` line to `~/.zshrc`. Re-running it is safe.

## Set up your accounts

```sh
cc add work        # opens Claude Code's normal /login; sign in with the work account, then /exit
cc add personal    # same, with the personal account
```

Each `cc add` copies your existing `~/.claude/settings.json` into the new
profile as a starting point and installs the status line into it. Your original
`~/.claude` is left untouched.

## Daily use

```sh
cc use personal    # plain `claude` now uses the personal account, in every tab
cc use work        # ...and now the work account
cc which           # which one is active
cc list            # all profiles, * marks the active one

claude-work        # run the work account explicitly, regardless of what is active
claude-personal    # same for personal
cc run work -p "summarise this diff"   # one-off with a specific profile
```

The `claude-<name>` commands exist for every profile and are created when your
shell starts. After `cc add` of a new profile, run `exec zsh` once to get its command.

## How it works

* `cc` is a bash script that keeps `~/.config/cc/profiles/<name>/` per profile
  and writes the active name to `~/.config/cc/active`.
* `cc.zsh` defines a `claude` shell function. If `CLAUDE_CONFIG_DIR` is not
  already set, it reads the active profile and exports that directory before
  running the real `claude`. An explicitly exported `CLAUDE_CONFIG_DIR` always wins.
* `statusline.sh` is wired into each profile's `settings.json` as Claude Code's
  `statusLine` command. It derives the profile name from `CLAUDE_CONFIG_DIR` and
  prints it with the model, directory and git branch.

Colours: names containing `work` are red, `personal`/`home` green, others blue.
Override by writing a 256-colour number into `<profile dir>/.cc-color`.

## Notes

* **Settings are per profile.** Edit `~/.config/cc/profiles/work/settings.json`
  for work-only MCP servers or permissions. To share one settings file between
  profiles, replace a profile's `settings.json` with a symlink to the other.
* **`~/.claude` still exists** as the "default" profile. If you never set an
  active profile, plain `claude` behaves exactly as before.
* **Anthropic's terms** allow separate accounts for separate purposes, not for
  pooling usage limits. This tool is for the former.
* To uninstall: remove the `source` line from `~/.zshrc`, then
  `rm -rf ~/.config/cc ~/.local/bin/cc`.
