## What this is

A standard for documentation you descend like folders. A reader starts at an org's top, picks the one category that can
hold the answer, and goes deeper: each level is more detailed than the last, from the architecture at the top to a
mirror of the source tree at the bottom. Any question about a project can be answered by descending its docs, without
opening code until the last step.

The standard defines every kind of file a docs tree may hold, versions those definitions, and ships `tarn-check`, a
checker that refuses a tree that breaks them and warns about drift no single push can show.

## What it is not

It is not a docs website or renderer: GitHub renders Markdown. It does not document code outside a repo or org that
adopts it, and it does not rewrite records such as plans, specs and decisions.

## Versions

Every docs tree declares the version it follows in one small file, and the checker judges it against that version, so a
new version never breaks a tree that has not moved. Version 1 is the first.
