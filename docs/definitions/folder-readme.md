# README

The `README.md` that every folder of a descent tree carries.

- **For:** telling a reader what the level covers and what every child holds, so they pick the next step from what they
  read and never from a name alone.
- **Reader:** a person or an agent descending the tree.
- **Kind:** current, hand-written, on every branch.
- **May appear:** in every folder of a descent tree (a repo's `docs/` and every folder under it; an org docs tree's root
  and every folder under it). Never in a records folder. A repo's top-level `README.md` is a different file, generated
  from `CONTENT.md` (see `content.md`) and not governed here. In an org docs tree (root `VERSION`) the root `README.md`
  is this kind, hand-written: the org's top level, with its repo list as an `org-repos` generated block (see
  `generated.md`). An org docs repo has no README generator and no `CONTENT.md`.

## Rules

1. It opens with one paragraph saying what this level covers.
2. After the paragraph come the child lines, and nothing else. A child is every file and every subfolder of the folder
   except `README.md` itself and a `VERSION` file. Every child is listed, by exact case, and nothing is listed that does
   not exist.
3. A child line has exactly one form, and it is the only accepted one. A file:

   ```
   - [<name>](<name>) - <one line>
   ```

   A folder, with the trailing slash in both the text and the target:

   ```
   - [<name>/](<name>/) - <one line>
   ```

   `<one line>` says what the child holds, in one line, starting after a space-hyphen-space.
4. A link to a child names the child exactly, relative to the folder. No other link form counts as a child line, bar
   the exception in rule 8 and the `org-repos` lines of `generated.md`.
5. The order is the author's; the checker does not read it.
6. The checker skips fenced code blocks and inline code when it reads links and child lines.
7. Generated marker lines (see `generated.md`) are allowed in a README, and its child lines may sit inside or outside a
   generated block.
8. The one exception to the child-line form is a `source/` README (see `source.md`): it also lists the code files of
   the mirrored source folder, each as `- [<name>](<path>) - <one line>`, where `<path>` is the relative path from the
   README to the file and `<name>` is the file's base name exactly.
9. The literal text `to write` stands where a person has still to write, as in a skeleton `tarn-check adopt` writes
   (see `version.md`). A line of the opening paragraph that reads exactly `to write`, and a child line whose one line
   is exactly `to write`, are warned as `prose-to-write`, never refused. A line inside a generated block is warned by
   the rules of `generated.md` instead, so no line is warned twice. Version 1's `--quick` skips nothing, so these
   warnings and the block warnings of `generated.md` are given under it as without it.
