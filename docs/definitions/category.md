# Category

A category folder of a descent tree, the prose files in it, and the size budget every descent file obeys.

- **For:** one area of a project, with no overlap with another category, so a reader never has to guess which folder
  holds an answer.
- **Reader:** anyone descending from the architecture toward the code.
- **Kind:** current.
- **May appear:** in a descent tree only, at any depth below the root.

## Rules

1. A category is a folder holding a `README.md` (see `folder-readme.md`), prose `.md` files and subfolders, under any name.
   Nothing else is allowed in it: no code, no plans, no tasks, no generated output except marked generated blocks (see
   `generated.md`).
2. Two categories never cover the same question. At an org, a category links down into a repo's own docs, and a repo's
   docs links down toward its source.
3. A prose file is one topic. It goes deeper than the README above it and never repeats it.
4. Links inside one tree may go in any direction. A link that leaves the tree goes downward only: an org's docs to a
   repo's docs, a repo's deepest docs to its source files. It crosses into another repo or org only where the target
   has a version file (see `version.md`).
5. A link must resolve. A public tree never links to, or names, a private repo or path.
6. A path in a living doc follows a move in the same commit; a path in a record keeps the form it had.
7. A docs tree holds no code, build output or secrets, and nothing a generator can recompute outside a marked generated
   block. A one-off prompt or a folder that no longer exists is deleted, not kept.

## Size budget

A descent-tree file over 400 lines is a warning. A file that grows past it should become a folder with its own
`README.md`, since a doc that grows too large usually means the thing it explains is badly structured. A justified
exception is recorded by a first line of the file that reads `<!-- size: <reason> -->`, which silences the warning for
that file. The budget does not apply to records (see `records.md`).
