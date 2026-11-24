# Category

A category folder of a descent tree, the prose files in it, and the size budget every descent file obeys.

- **For:** one area of a project, with no overlap with another category, so a reader never has to guess which folder
  holds an answer.
- **Reader:** anyone descending from the architecture toward the code.
- **Kind:** current.
- **May appear:** in a descent tree only, at any depth below the root.

## Rules

1. A category is a folder holding a `README.md` (see `folder-readme.md`), prose `.md` files and subfolders, under any
   name. Nothing else is allowed in it: no code, no plans, no tasks, no generated output except marked generated blocks
   (see `generated.md`).
2. Two categories never cover the same question. At an org, a category links down into a repo's own docs, and a repo's
   docs links down toward its source.
3. A prose file is one topic. It goes deeper than the README above it and never repeats it.
4. Links inside one tree may go in any direction. A link that leaves the tree goes downward only: an org's docs to a
   repo's docs, a repo's deepest docs to its source files. It crosses into another repo or org only where the target
   has a version file (see `version.md`); the checker refuses a link into a repo the repos snapshot marks `not` as
   `link-into-nonstandard`.
5. A link must resolve. A public tree never links to, or names, a private repo or path; the checker refuses a repo the
   repos snapshot marks `private`, named in a repo it marks `public`, as `public-names-private`.
6. A path in a living doc follows a move in the same commit; a path in a record keeps the form it had.
7. A docs tree holds no code, build output or secrets, and nothing a generator can recompute outside a marked generated
   block. A one-off prompt or a folder that no longer exists is deleted, not kept.

## The repos snapshot

Rules 4 and 5 depend on facts outside the tree: which repos are private and which follow the standard. The checker
never calls the network to learn them. The docs workflow makes a snapshot of them with the provider's API and hands it
over as `tarn-check --snapshot <file> --repository <owner>/<name>`, where the second names the repo being checked.

1. The snapshot is a text file of one line per repo, `<owner>/<name> <public|private> <follows|not|archived>`, and
   nothing else: no JSON, no comments, no blank lines, words separated by exactly one space (no tab, no trailing
   space). A byte order mark and CRLF line endings are tolerated. `<owner>` and `<name>` hold letters, digits, `.`, `_`
   and `-`. `follows` means the repo has a version file of either kind (see `version.md`). `archived` means the repo is
   archived on the provider, whatever else is true of it: it exists, with the visibility stated, for every rule below,
   and it counts as not following, since an archived repo is never migrated.
   It is the one state that is left out of the `org-repos` map and its warnings (see `generated.md`).
2. Bad usage, exit 2: a snapshot that cannot be read, a line in another form, a repo listed twice (ignoring case), a
   `--snapshot` without `--repository` or the reverse, and a `--repository` the snapshot does not list. The visibility
   of the repo being checked is read from its own line.
3. A doc names a repo when two adjacent segments of a slash-separated word in its text spell `<owner>/<name>`. So
   `<owner>/<name>` alone names it, and so does a path into or through it (`<owner>/<name>/plans/x.md`,
   `GitHub/<owner>/<name>`) and any address of it (`https://github.com/<owner>/<name>?tab=readme`), clone URLs
   included: a `.git` ending the name is not part of it, and `:` separates words, so
   `https://github.com/<owner>/<name>.git` and `git@github.com:<owner>/<name>.git` name it. Each segment is
   whole: `<owner>/<name>-more` and `my-<owner>/<name>` name nothing, and a sentence's closing full stop is not part
   of the word. Names are compared ignoring case, as GitHub does. Fenced code blocks and inline code are read too,
   since a name in code is published all the same; only the link rules skip code.
4. In a repo the snapshot marks `public`, naming a repo it marks `private` is refused as `public-names-private`, once
   per repo per line. A private repo may name any repo.
5. A link whose target is `https://github.com/<owner>/<name>` (or `http://`, with or without `www.`, in any case),
   with anything after it, a clone URL's `.git` ending the name included, into a repo the snapshot marks `not` is
   refused as `link-into-nonstandard`. A link into a repo marked `follows`, into the repo being checked, and into
   anything the snapshot does not list (an outside site) passes, and so do the lines of an `org-repos` block (see
   `generated.md`), whose job is to list the repos that do not follow yet. Links inside fenced or inline code are not
   links and are skipped.
   A known limit of version 1: the snapshot lists the repos of the checked repo's owner only, so a link into a repo
   of another owner passes whether or not that repo follows, since the checker never calls the network to find out.
6. The rules read every file the descent walk reads: each README and prose file of the tree, and a repo's
   `CONTENT.md`.
7. Without a snapshot both rules are skipped, an `org-repos` block is left as it is, and the report says so in one
   line, `warned .:0 snapshot-absent`.

## Size budget

A descent-tree file over 400 lines is a warning. A file that grows past it should become a folder with its own
`README.md`, since a doc that grows too large usually means the thing it explains is badly structured. A justified
exception is recorded by a first line of the file that reads `<!-- size: <reason> -->`, which silences the warning for
that file. The budget does not apply to records (see `records.md`).
