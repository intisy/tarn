# TASKS

`TASKS.md`: the queue of every task a layer has ever had.

- **For:** being the complete record of the work, open and closed, so the reasoning behind a decision taken months ago is
  in the entry that recorded it and nowhere else.
- **Reader:** everyone; a session reads the open set at its start and writes back what changed at its end.
- **Kind:** record, append-only. Nothing leaves it.
- **May appear:** once per org docs tree, in the records half. Never in a repo docs tree, and never linked from the
  descent tree as the place an answer lives.

## Rules

1. The section is the state: `## Active`, `## Done` and `## Cancelled`, and a task moves between them when its state
   changes. A layer may add a further named section for a state the three do not cover. Each entry carries a
   `- state:` line, which stays authoritative.
2. Finished work moves to done and stays. A task leaves the active set only as cancelled, and a cancellation carries
   `cancelled-because`.
3. Only the human partner cancels a task. An agent may propose it.
4. Ids are never reused.
5. Every task names where its detail lives. The entry carries the reasoning and the outcome; the design lives in a
   spec the entry points at.
6. The open set is read without reading the whole file:
   `awk '/^### /{h=$0} /^- state: (open|active|blocked)/{print h}' TASKS.md`.
7. The file's own header is the format, so a new org copies it.
