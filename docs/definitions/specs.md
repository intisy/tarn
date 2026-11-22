# specs

The `specs/` folder: approved designs, one dated file each.

- **For:** recording the design that was approved before it was built.
- **Reader:** builders and auditors.
- **Kind:** record.
- **May appear:** once per org docs tree, in the records half. Never in a repo docs tree.

## Rules

1. One file per spec, named `YYYY-MM-DD-topic-design.md`.
2. A spec is approved before it turns into a plan or code.
3. A record is never rewritten. Where a spec is superseded, one line under its title says so and names the replacement;
   that marker is the one permitted addition, and it never touches the body.
4. The descent tree never links to a spec as the place an answer lives: a spec describes what will be, the docs describe
   what is.
