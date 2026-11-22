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

## Adopting

`tarn-check adopt repo --standard-ref <commit> [--write] [<repo root>]` and
`tarn-check adopt org --standard-ref <commit> --snapshot <file> --repository <owner>/<name> [--write] [<repo root>]`
start a tree in one step. `<commit>` is the full 40-character `intisy/tarn` commit the workflow caller pins, since
everything is pinned by commit while publishing is halted. `org` fills its `org-repos` block from the repos snapshot,
so it requires `--snapshot` and `--repository`; `repo` refuses them, since nothing it writes reads them. A first
argument `adopt` always means this command, so a repo root of that name is checked as `./adopt`.

1. `adopt repo` plans `docs/VERSION` (`tarn 1`), `docs/README.md`, one `README.md` per folder of the `docs/source/`
   mirror (see `source.md`) with its `source-children` block filled, and `.github/workflows/docs.yml`. `adopt org`
   plans the root `VERSION`, the root `README.md` with its `org-repos` block filled, and the same caller.
2. A planned README is a skeleton: the paragraph and every hand-written child text read `to write` (see
   `folder-readme.md` rule 9), except the line of `docs/README.md` that names `source/`, whose text is fixed. Its child
   lines list every child the folder already has, so an adopted tree is refused only for what a kept file leaves wrong.
3. The caller is the thin caller of `intisy/tarn/.github/workflows/check-docs.yml` at `<commit>`: `mode: check` on
   push and pull request, `mode: warn` on a weekly schedule and on a manual run, with `standard-ref` the same commit.
4. It prints one line per planned file, in the order above: `create <path>` for a file it writes, `keep <path>` for
   one that exists already under any case, which it never opens for writing. `adopt org` then prints
   `does-not-follow <owner>/<name>` for each repo its `org-repos` block marks as not following, sorted by name.
5. Without `--write` nothing is written. With it, the `create` files are written, so adopting twice changes nothing:
   the second plan lists only `keep` lines.
6. It exits 0 when the plan is carried out, 1 when the repo already declares the other kind of tree (a `docs/VERSION`
   for `org`, a root `VERSION` reading `tarn <n>` for `repo`) or a file cannot be written, and 2 on bad usage.
7. An adopted tree with nothing kept passes `tarn-check` with only warned `prose-to-write` lines, and `snapshot-absent`
   when it is checked without a snapshot.
