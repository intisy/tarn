# records

The `records/` folder, and the records half of an org docs tree as a whole.

- **For:** keeping what was done and measured: finished task reports and measurement runs.
- **Reader:** auditors.
- **Kind:** record.
- **May appear:** once per org docs tree, in the records half. A repo docs tree refuses it.

## Rules

1. The records half allows only the named files: `TASKS.md`, `DECISIONS.md`, `HANDOFF.md`, `FAQ.md`, `RULES.md`,
   `CLAUDE.md` (holding one `@` import of `RULES.md`), `plans/`, `specs/` and `records/`. This is the one place the
   list is written; `version.md` and the checker point here. Each is a record wherever it appears: below the root of
   an org docs tree, or anywhere in a repo docs tree, it is refused as `kind-not-allowed`.
2. A record is moved into `records/` byte for byte and never edited afterward, not for style, not for a renamed path.
   A move is checked by hash before and after.
3. The size budget (see `category.md`) does not apply to the records half.
4. For a public org the records half lives in a private repo beside the public descent tree; for a private org both
   halves share one repo. The private records repo is an org docs tree like any other (a root `VERSION`, see
   `version.md`), and its README lists its records as children, so it needs no kind of its own.
5. A public org docs tree holds no records. Checked with a repos snapshot that marks the tree's own repo `public`,
   every record at its root is refused as `record-in-public-tree`. Without a snapshot its visibility is unknown, so
   records pass.
