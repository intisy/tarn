# README

The `README.md` that every folder of a descent tree carries.

- **For:** telling a reader what the level covers and what every child holds, so they pick the next step from what they
  read and never from a name alone.
- **Reader:** a person or an agent descending the tree.
- **Kind:** current, hand-written, on every branch.
- **May appear:** in every folder of a descent tree (a repo's `docs/` and every folder under it; an org docs tree's root
  and every folder under it). Never in a records folder. The repo's top-level `README.md` is a different file, generated
  from `CONTENT.md` (see `content.md`) and not governed here.

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
4. A link to a child names the child exactly, relative to the folder. No other link form counts as a child line.
5. The order is the author's; the checker does not read it.
