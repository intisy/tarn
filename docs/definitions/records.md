# records

The `records/` folder, and the records half of an org docs tree as a whole.

- **For:** keeping what was done and measured: finished task reports and measurement runs.
- **Reader:** auditors.
- **Kind:** record.
- **May appear:** once per org docs tree, in the records half. A repo docs tree refuses it.

## Rules

1. The records half allows only the named files: `TASKS.md`, `DECISIONS.md`, `HANDOFF.md`, `FAQ.md`, `RULES.md`,
   `plans/`, `specs/` and `records/`, plus a `CLAUDE.md` holding one `@` import of `RULES.md`.
2. A record is moved into `records/` byte for byte and never edited afterward, not for style, not for a renamed path.
   A move is checked by hash before and after.
3. The size budget (see `category.md`) does not apply to the records half.
4. For a public org the records half lives in a private repo beside the public descent tree; for a private org both
   halves share one repo.
