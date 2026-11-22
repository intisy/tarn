# Version

The file that declares which version of the standard a docs tree follows, and how versions ship.

- **For:** letting the checker judge a tree against the version it declares, so a new version never breaks a tree that
  has not moved.
- **Reader:** the checker, and anyone asking which version a tree follows.
- **Kind:** current.
- **May appear:** as `docs/VERSION` in a repo docs tree, or as `VERSION` at the root of an org docs tree. Never both in
  one tree.

## Rules

1. The file is exactly one line, `tarn <n>`, where `<n>` is a whole number, and nothing else. Version 1 reads
   `tarn 1`. A CRLF line ending and a missing final newline are tolerated.
2. A repo with `docs/VERSION` is a repo docs tree rooted at `docs/`: descent kinds only, records refused.
3. A repo with a root `VERSION` reading `tarn <n>` is an org docs tree rooted at the repo root: descent kinds plus the
   named records (`TASKS.md`, `DECISIONS.md`, `HANDOFF.md`, `FAQ.md`, `RULES.md`, `plans/`, `specs/`, `records/`). A
   public org splits this across two repos: the public one carries the descent tree, a private one the records.
4. At the root of an org tree the checker ignores every entry whose name starts with `.` (`.github/`, `.gitignore`,
   `.gitattributes`), `LICENSE`, and `VERSION`. In a repo tree it reads `docs/` and the root `CONTENT.md` only. The
   root `README.md` of an org tree is a descent README, not a generated one.
5. A `VERSION` file is not a child for `folder-readme.md`: a README need not list it.
6. A tree declaring an older version is a pin. A pin is temporary and names the task that ends it; the checker warns it
   as `older-version` on line 1 of the version file until it moves, and refuses a version newer than it knows as
   `version-unknown`.
7. Another repo or org counts as following the standard when its tree has a version file of either kind. The repos
   snapshot marks such a repo `follows` (see `category.md`).
8. Each version ships a change list that says exactly what to change, and, where the change is mechanical, an automatic
   upgrade. Version 1 has nothing to upgrade from, so the upgrade command arrives with version 2.
