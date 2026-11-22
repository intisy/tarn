# Generated

Blocks inside a doc that the checker writes, never a person.

- **For:** the mechanical parts of a doc that a program can recompute, so they cannot fall behind: the child list of a
  `source/` README and an org's list of its repos.
- **Reader:** everyone; only the checker writes them.
- **Kind:** current. A wrong block is fixed in its generator or its source, never in the block.
- **May appear:** inside a prose or README file of a descent tree.

## Rules

1. A generated block starts with the line `<!-- generated: <what> -->` and ends with the line `<!-- end generated -->`.
   Everything between them belongs to the checker.
2. `<what>` is one of two kinds in version 1:
   - `source-children`: the child lines of a `source/` folder's README, built from the real source folder.
   - `org-repos`: the org's list of repos, one child line each, built from the provider's list of the org's repos and
     marking which have no docs yet.
3. Inside a block every line is `- [<name>](<path>) - <text>`, sorted by byte order of `<name>` as
   written, a folder's trailing slash included (so `a.c` sorts before `a/`). `<text>` is kept from the line with the
   same `<name>` already in the block; a new line gets the literal text `to write`, which the checker
   warns about as `prose-to-write`. `org-repos` lines are
   `- [<owner>/<name>](https://github.com/<owner>/<name>) - <text>`, and a repo the visibility snapshot marks `not`
   has ` (does not follow tarn yet)` appended by the generator.
4. The checker skips fenced code blocks and inline code when it reads markers and lines.
5. The checker regenerates each block and fails on a difference between the file and the regenerated text, as
   `generated-block-stale` at the block's first line. `tarn-check --write` rewrites the stale blocks in place instead,
   keeping the file's line endings and byte order mark and touching nothing outside a block, and then checks as usual.
   In version 1 only `source-children` is regenerated; an `org-repos` block is accepted as it stands, and its lines are
   not child lines of the README.
6. Blocks do not nest, and an unmatched marker is refused: `generated-nested` for a start inside a block,
   `generated-unterminated` for a start with no end, and `generated-marker-unmatched` for an end with no start. A
   `<what>` that is not one of the two kinds is refused as `generated-kind-unknown`, and `source-children` outside a
   README under `docs/source/` as `generated-kind-misplaced`. A file with a refused marker is never rewritten.
7. Generated output never appears outside a marked block.
