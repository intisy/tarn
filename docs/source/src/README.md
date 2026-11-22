The checker, `tarn-check`. `check_tree` reads the version file to tell a repo docs tree (`docs/VERSION`, rooted at `docs/`) from an org docs tree (a root `VERSION`, rooted at the repo root), finds every source folder that holds code, regenerates the marked blocks of the descent tree (with `--write` it rewrites the stale ones first, so the walk reads the fresh files), walks the descent tree folder by folder, checks every descent file against the repos snapshot when `--snapshot` and `--repository` give one (and warns once that it did not when they do not), and adds each finding to a report as `refused|warned <path>:<line> <rule> <text>`, where line 0 means the whole file or folder. Every path is relative to the repo root with forward slashes, and every name is compared byte for byte with the real directory entry, so a case mismatch is caught on a case-insensitive disk too. The program exits 0 when nothing is refused, 1 when something is, and 2 on bad usage.

<!-- generated: source-children -->
- [boundary.c](../../../src/boundary.c) - the two snapshot rules: a public tree naming a private repo by any two adjacent segments of a path (in code too), and a GitHub link into a repo that does not follow tarn
- [boundary.h](../../../src/boundary.h) - the snapshot check of one document
- [check.c](../../../src/check.c) - `check_tree`: the version files, the tree kind, the checked repo's line in the snapshot, the order of the checks (blocks first, then the walk), and the `snapshot-absent` warning
- [check.h](../../../src/check.h) - `check_tree`, its options and its results, and the opening and closing of the context every check shares
- [context.h](../../../src/context.h) - what every rule module knows about the tree being checked, the snapshot and the checked repo's line in it included
- [directory.c](../../../src/directory.c) - the one place that talks to the OS: folder listings that never follow a folder link, and file reads, on Windows and POSIX
- [directory.h](../../../src/directory.h) - a folder listing, sorted by byte order, with names exactly as stored
- [generate.c](../../../src/generate.c) - the marked blocks: finding them in a file, comparing them with the regenerated text, and rewriting the stale ones in place, byte for byte outside the block
- [generate.h](../../../src/generate.h) - `generate_blocks`, the regeneration `check_tree` runs before the walk
- [ignore.c](../../../src/ignore.c) - the source mirror's ignore list: the root `.gitignore` and the fixed folders
- [ignore.h](../../../src/ignore.h) - the ignore patterns and the two questions asked of them
- [links.c](../../../src/links.c) - the dead-link rule: every link resolved from its file, anchors dropped, case compared exactly
- [links.h](../../../src/links.h) - the link check of one document
- [main.c](../../../src/main.c) - the command line: `tarn-check [--quick] [--write] [--snapshot <file> --repository <owner>/<name>] [<repo root>]`, the snapshot's loading, and exit 2 for bad usage
- [memory.c](../../../src/memory.c) - allocation that aborts instead of returning NULL
- [memory.h](../../../src/memory.h) - the allocation helpers
- [org_repos.c](../../../src/org_repos.c) - the lines of an `org-repos` block, built from the snapshot, with the text of each kept by name, and the form every such line must have
- [org_repos.h](../../../src/org_repos.h) - the block lines, the form test and the placeholder test
- [path.c](../../../src/path.c) - relative paths: joining, normalizing, and looking one up byte for byte
- [path.h](../../../src/path.h) - the path helpers and the states of an exact lookup
- [readme.c](../../../src/readme.c) - a README's form and its child lines, including the code-file lines of a `source/` README, and skipping the lines of an `org-repos` block, which are not child lines
- [readme.h](../../../src/readme.h) - the children a README must list and where the README sits
- [report.c](../../../src/report.c) - the report: its lines, their format and the exit code
- [report.h](../../../src/report.h) - a finding and the report that holds them
- [rules.c](../../../src/rules.c) - the kinds a descent folder may hold, the code extensions, and the size budget
- [rules.h](../../../src/rules.h) - the kind of an entry and the rules read from it
- [snapshot.c](../../../src/snapshot.c) - the repos snapshot: reading its lines, refusing a malformed or repeated one, and finding a repo ignoring case
- [snapshot.h](../../../src/snapshot.h) - a snapshot line, the snapshot, and the name helpers the rules share
- [source_children.c](../../../src/source_children.c) - the lines of a `source-children` block, built from the real source folder, with the text of each kept by name
- [source_children.h](../../../src/source_children.h) - the block lines and the placeholder test
- [string_list.c](../../../src/string_list.c) - a growable list of strings
- [string_list.h](../../../src/string_list.h) - the string list
- [text.c](../../../src/text.c) - a file as lines (BOM and CR removed), fenced code, inline code, links, the marker comments and the lines inside an `org-repos` block
- [text.h](../../../src/text.h) - the text and Markdown helpers
- [walk.c](../../../src/walk.c) - the walk of the descent tree, the scan of the source tree, and the mirror between them
- [walk.h](../../../src/walk.h) - the three walks `check_tree` runs, and the mirror lookup the generator reuses
<!-- end generated -->
