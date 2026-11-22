# RULES

`RULES.md`: the standing rules of a layer, each with its grounds.

- **For:** stating what a layer obeys and what breaking each rule costs, so a rule is not re-argued from scratch.
- **Reader:** everyone working in the layer.
- **Kind:** current. A rule that stops applying is deleted.
- **May appear:** once per org docs tree, in the records half. Never in a repo docs tree.

## Rules

1. Each rule carries its source and what breaking it costs. Its own header says how the sources are marked.
2. A rule lives at the highest layer where it is true. A lower layer points up by naming the file and the layer, never by
   copying the text; a duplicate is deleted, never kept in sync.
3. When a lower layer must differ from a higher one, it says so, says why, and names the rule it overrides. A silent
   difference is a defect.
4. A repo's rules file is imported by one line in the repo's `CLAUDE.md`, so a repo has exactly one rules file and no
   second place to look.
