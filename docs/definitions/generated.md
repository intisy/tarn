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
3. The checker regenerates each block and fails on a difference between the file and the regenerated text.
4. Blocks do not nest, and an unmatched marker is refused.
5. Generated output never appears outside a marked block.
