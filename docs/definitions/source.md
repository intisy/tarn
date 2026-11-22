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
   warning, never a refusal: `source-doc-stale` on the doc folder, with the day count and both dates. Each side counts
   only the files directly in its folder, since a subfolder has a doc of its own: the folder's code files (rule 3) and
   every file of its doc folder. A commit's time is its author date, since a rebase resets every committer date and
   would make a rebased doc look fresh. The checker asks the repo's git for the times, and skips:
   - a folder for which git is missing, fails, or knows no commit on either side (a file never committed);
   - a folder whose path, or the repo root's path, holds a character it cannot quote safely for the shell: `"`, `'`,
     `%`, `!` or a control character, and on Windows any non-ASCII character;
   - a folder whose name holds a character git would read as a pattern: `*`, `?` or `[`, and `\` outside Windows;
   - every folder in a shallow clone, whose one commit dates every file alike. The report then says so once, as
     `warned .:0 staleness-skipped`. A run that wants the check fetches the full history first.
   Under `--quick` the checker asks git nothing, so neither warning is given; it is the only thing `--quick` skips.
