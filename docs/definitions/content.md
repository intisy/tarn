# CONTENT

The `CONTENT.md` at a repo's root: the hand-written prose the top-level README is generated from.

- **For:** holding the data a repo's README is rendered from, so the README is always up to date. A docs folder's
  `README.md` is a different file, hand-written and kept on every branch.
- **Reader:** the README generator; people read the generated README.
- **Kind:** current, hand-written.
- **May appear:** once, at the root of a repo with a repo docs tree. It sits beside `.github/docs-config.yml`, which
  names the repo's kind, title and description. In a repo tree the checker reads this file and `docs/` only, never other
  root files. An org docs tree (root `VERSION`) has no `CONTENT.md` and takes no README generator: its root `README.md`
  is hand-written (see `folder-readme.md`).

## Rules

1. It says what the repo is, what it is not, and what is needed to understand it. It does not repeat a standard file's
   purpose.
2. A project with no org of its own keeps everything it offers, docs included, in one repo.
3. Only a repo's top-level README is generated, and only on the default branch. A development branch carries `CONTENT.md`,
   never a rendered `README.md`.
