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
   - `org-repos`: the org's list of repos, one line each, built from the repos snapshot (see `category.md`) and
     marking which do not follow the standard yet. It lists every repo the snapshot gives for the owner of the repo
     being checked, except that repo itself; when that repo is public it lists the public repos only, so the block
     never names a private repo in a public tree.
3. Inside a block every line is `- [<name>](<path>) - <text>`, sorted by byte order of `<name>` as
   written, a folder's trailing slash included (so `a.c` sorts before `a/`). `<text>` is kept from the line with the
   same `<name>` already in the block; a new line gets the literal text `to write`, which the checker
   warns about as `prose-to-write`. `org-repos` lines are
   `- [<owner>/<name>](https://github.com/<owner>/<name>) - <text>`, sorted by `<owner>/<name>`, and a repo the
   repos snapshot marks `not` has ` (does not follow tarn yet)` appended by the generator. That marker is not part of
   the kept `<text>`, so it goes when the repo starts to follow, and a placeholder followed by it still warns.
4. The checker skips fenced code blocks and inline code when it reads markers and lines.
5. The checker regenerates each block and compares it with the file. A `source-children` block is regenerated from
   the source folder, and a difference is refused as `generated-block-stale` at the block's first line. An `org-repos`
   block is regenerated from the repos snapshot when one is given and left as it is when none is. It goes stale when
   the org changes, which no push to the docs repo can see, so its difference is warned and never refused: a repo the
   snapshot gives for the block (rule 2) that the block lacks is `org-misses-repo` at the block's first line, and a line
   naming a repo the snapshot does not list at all is `org-names-gone-repo` at that line. A difference that is neither,
   such as the order, a kept text or the marker of rule 3, is one warned `generated-block-stale` on the block. A private
   repo inside a public block is still refused as `public-names-private` (see `category.md`). `tarn-check --write`
   rewrites every stale block in place instead, keeping the file's line endings and byte order mark and touching
   nothing outside a block, and then checks as usual. Each line of an `org-repos` block must have the `org-repos` form
   of rule 3, snapshot or not, or it is refused as `generated-line-form`. Its lines are not child lines of the README,
   and the `link-into-nonstandard` rule of `category.md` does not apply to them.
6. Blocks do not nest, and an unmatched marker is refused: `generated-nested` for a start inside a block,
   `generated-unterminated` for a start with no end, `generated-marker-unmatched` for an end with no start, and
   `generated-block-duplicate` for a second block of the same kind in one file. A `<what>` that is not one of the two kinds is refused as
   `generated-kind-unknown`, and `source-children` outside a README under `docs/source/`, or `org-repos` anywhere
   but the root README of an org docs tree, as `generated-kind-misplaced`. A file with a refused marker is never rewritten.
7. Generated output never appears outside a marked block.
