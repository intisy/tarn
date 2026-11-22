# Source

The `source/` folder: the bottom of a repo's descent tree, a mirror of its source folders.

- **For:** explaining each source folder in the place a reader looking for that code would look, and linking down to its
  files. `src/host/description/` has a doc at `source/src/host/description/README.md` that explains that folder and
  links to its files.
- **Reader:** someone who has found the right repo and topic and now needs the code.
- **Kind:** current. The child lists inside are generated (see `generated.md`); the prose is hand-written.
- **May appear:** once, as `docs/source/` of a repo docs tree. Never in an org docs tree.

## Rules

1. The mirror is one for one. Every source folder that holds code has a folder at the same relative path under
   `source/`, with a `README.md` that explains it. The path is the folder's path from the repo root. The root itself is
   `docs/source/README.md`, which is both `source/`'s folder README and the doc for the code files directly at the
   repo root.
2. A `source/` folder naming a source folder that no longer exists is refused.
3. A folder holds code when it directly contains at least one file with a code extension, or when any folder under it
   does. Code extensions in version 1: `.c`, `.h`, `.cpp`, `.hpp`, `.ts`, `.js`, `.mjs`, `.java`, `.kt`, `.rs`, `.go`,
   `.py`, `.lua`, `.sh`, `.ps1`, `.cmake`, and files named `CMakeLists.txt`. Matching is case-sensitive as
   listed, and `CMakeLists.txt` is matched exactly. A folder holding none of these is not mirrored.
4. The mirror ignores what is not the repo's own source: everything the repo's root `.gitignore` excludes (only that file counts in version 1), plus a fixed list
   in every repo: `build/`, `node_modules/`, `vendor/`, `siblings/` and `.git/`, at any depth, and the top-level `docs/` itself.
5. A mirrored folder's `README.md` has two kinds of child line, both `- [<name>](<path>) - <one line>`: its own doc
   subfolders in the folder form of `folder-readme.md` (`[<name>/](<name>/)`), one per source subfolder that holds
   code, and the code files directly in the source folder, linked by the relative path from the README to the file,
   with `<name>` equal to the file's base name exactly. Non-code files and ignored folders are not listed. This is the
   one exception to the child-line form.
6. A source doc is stale when its folder has commits newer than the doc's last commit by more than 14 days. That is a
   warning, never a refusal.
