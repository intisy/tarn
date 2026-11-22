# Proposal: the FAQ wording for the global rules

For `~/.claude/CLAUDE.md`. Nothing here is applied; edit the file yourself.

Replace the whole section titled "Every project keeps a FAQ, and the agent adds to it unasked" (the heading and its four
bullets, up to the `## Commits` heading) with the text between the lines below.

The change from today's wording: an entry no longer holds the answer. It is a question plus the links into the docs that
answer it, because an answer held only in the FAQ has no home in the descent tree (`docs/definitions/faq.md`, rules 1
and 2). Intended and Today stay, but each is now a line that points, not prose that argues.

---

### Every project keeps a FAQ, and the agent adds to it unasked

- **Each project has one `FAQ.md`, in its records**, holding the questions I ask about its design. A project without
  one gets one the first time a question earns it. An org keeps it in the records half of its docs, never in a repo's
  docs tree.
- **An entry is a question and the links into the docs that answer it.** Write the question in my own words where they
  carry the point. The answer is not in the entry: it lives in the descent tree, and the entry links to it. An answer
  that exists only in the FAQ has no home, so write it into the right doc first and link it.
- **Add a question I ask without being told to**, whenever the answer is worth knowing later: a design question, a
  "how does X actually work", a "why is it this way". Skip the one-off and the purely conversational.
- **Each entry has two lines**: *Intended*, the ruling, binding whether or not code implements it, and *Today*, what is
  on disk, measured with the date and the doc or file read. When the Intended line names work that is not built, it
  points at the task that closes the gap.
- **A FAQ is a CURRENT-state document**, so the rule above binds it: when a ruling changes or a gap closes, rewrite the
  entry in the same change. An *Intended* line that is no longer the design is a defect, not history.

---
