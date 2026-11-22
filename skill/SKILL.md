---
name: tarn-docs
description: Read and write docs under the tarn standard. Use when a repo or org has a docs/VERSION or a root VERSION reading "tarn <n>", when asked where a doc belongs, or when tarn-check reports refusals.
---

# Docs under the tarn standard

A tree that follows the standard has `docs/VERSION` (a repo) or a root `VERSION` reading `tarn <n>` (an org docs repo).
The definitions of each kind of file are in `docs/definitions/` of `intisy/tarn`.

## Reading

1. Open the top README of the tree: `docs/README.md`, or the root `README.md` of an org docs repo.
2. It opens with one paragraph, then one line per child. Pick the one child that can hold your answer.
3. Open that child. A folder has its own README with the same shape: descend again. A file is prose: read it.
4. Stop at the first level that answers the question. The bottom of a repo is `docs/source/`, which mirrors each source
   folder with a doc linking to its files.
5. A link into another repo or org is followed only where the target has its own version file.

## Writing

- A new doc goes in the folder whose README describes it, as a prose `.md` file, and that README gets one new child line
  in exactly this form: `- [<name>](<name>) - <one line>`; a folder is `- [<name>/](<name>/) - <one line>`.
- Explaining a source folder: its doc is `docs/source/<path of the folder>/README.md`. Create it when you add code to a
  folder that has none.
- Queue, decisions, handoff, FAQ, rules, plans, specs and records are named kinds that only an org docs tree holds.
  A repo docs tree holds descent kinds only.
- No other file kind is allowed in a docs tree. A doc over 400 lines means the code it explains wants splitting.
- Hand-written prose only outside `<!-- generated: <what> -->` and `<!-- end generated -->`: never edit a generated
  block, fix its source and run the writer.

## Checking

- `tarn-check <repo root>` checks the tree and exits 1 when something is refused.
- `tarn-check --write <repo root>` rewrites the stale generated blocks first, then checks.
- `tarn-check --quick <repo root>` skips the slow git staleness calls and nothing else.
- Build it from `intisy/tarn`: `cmake -S . -B build && cmake --build build`.
- A line reads `refused|warned <path>:<line> <rule> <text>`. Fix every `refused` before finishing.
