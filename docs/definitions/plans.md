# plans

The `plans/` folder: approved plans for building a spec.

- **For:** recording what was approved and then executed.
- **Reader:** builders and auditors.
- **Kind:** record.
- **May appear:** once per org docs tree, in the records half. Never in a repo docs tree: a plan is never committed into
  a repo's `docs`.

## Rules

1. One file per plan, named `YYYY-MM-DD-topic.md`.
2. A plan is approved before it turns into code.
3. A record is never rewritten. Where a plan is superseded, one line under its title says so and names the replacement;
   that marker is the one permitted addition, and it never touches the body.
4. The descent tree never links to a plan as the place an answer lives.
