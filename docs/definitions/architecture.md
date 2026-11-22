# Architecture

The top of a descent tree: how the parts fit and talk.

- **For:** answering general questions high up, such as every communication method between all the parts. At an org it
  covers how the repos relate; at a repo it covers what the repo is for, what it owns, what it does not, and how its
  parts relate.
- **Reader:** anyone who needs the shape of a system before any part of it.
- **Kind:** current. It describes what is, never what will be, and is pruned when a fact stops applying.
- **May appear:** in a descent tree only, as an `architecture/` folder (with a `README.md` and prose files, one per
  topic) at an org docs tree's root, or as the repo docs tree's own `README.md` and topic folders.

## Rules

1. It is more general than anything below it. A detail that belongs to one part goes down into that part's docs, and
   the architecture links to it.
2. A topic that outgrows the size budget (see `category.md`) becomes a folder with its own `README.md`.
3. It never links to a plan or a task as the place an answer lives. A plan describes what will be; the docs describe
   what is.
4. A public tree never names a private repo or path.
