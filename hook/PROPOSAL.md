# Proposal: the session-start reminder

For `~/.claude/settings.json`. Nothing here is installed; add it yourself.

Build `tarn-check` first, from `intisy/tarn`:

```
cmake -S . -B build
cmake --build build --config Debug
```

Then add a SessionStart hook that runs the script. `TARN_CHECK` names the built program; without it the script looks for
`tarn-check` on `PATH`.

```json
"env": {
  "TARN_CHECK": "F:/Documents/GitHub/intisy/tarn/build/Debug/tarn-check.exe"
},
"hooks": {
  "SessionStart": [
    {
      "hooks": [
        {
          "type": "command",
          "command": "sh F:/Documents/GitHub/intisy/tarn/hook/session-start.sh"
        }
      ]
    }
  ]
}
```

If `settings.json` already has an `env` or `hooks` object, add the entries inside it instead of a second key.

The script walks up from the working directory to the nearest `docs/VERSION` or root `VERSION` reading `tarn <n>`, runs
`tarn-check --quick` on that folder, and prints at most 20 lines. In a tree without the standard it prints nothing, and it
always exits 0.
