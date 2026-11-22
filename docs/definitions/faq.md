# FAQ

`FAQ.md`: the questions asked about a project's design, each pointing at the docs that answer it.

- **For:** finding where an answer lives, never holding one. The answer is in the descent tree; the FAQ links to it.
- **Reader:** the human partner and any session asking "how does X work" or "why is it this way".
- **Kind:** current. It is pruned when a ruling changes or a gap closes.
- **May appear:** once per org docs tree, in the records half. Never in a repo docs tree.

## Rules

1. An entry is a question, in the asker's own words where they carry the point, and the links into the docs that answer
   it. The entry holds no answer of its own: an answer that exists only in the FAQ has no home in the tree and belongs
   in a doc.
2. An entry's Intended line is the ruling, binding whether or not code implements it. When the Intended line names work
   that is not built, it points at the task that closes the gap.
3. An entry's Today line says what is on disk, with the date and the doc or file it was measured against.
4. The agent adds an entry unasked when a design question is asked whose answer is worth knowing later. A one-off or
   purely conversational question is skipped.
5. When a ruling changes or a gap closes, the entry is rewritten in the same change. An Intended line that is no longer
   the design is a defect, not history.
