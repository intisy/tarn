---
name: tarn
description: Read and write docs under the tarn standard. Use when a repo or org has a docs/VERSION or a root VERSION reading "tarn <n>", when asked where a doc belongs, or when tarn-check reports refusals.
---

# Docs under the tarn standard

The standard is defined in `docs/definitions/` of `intisy/tarn`, one file per kind of file a docs tree may hold
(https://github.com/intisy/tarn/tree/main/docs/definitions). Those files are the rules; this skill only says which one
to open. When a rule here and a definition seem to differ, the definition wins.

## Installing

Copy this `skill/` folder into your agent's skills directory under the skill's name, for Claude Code
`~/.claude/skills/tarn/`, so the folder holds `SKILL.md`. Nothing else is needed; the checker is built separately (see
Checking).

## Reading

Start at the top README of the tree and descend: each README names its children, so open the one child that can hold
the answer and stop at the first level that gives it. Which file is the top, and when a tree counts as following the
standard, is in `version.md`; the shape of every README is in `folder-readme.md`; the bottom of a repo, the source
mirror, is in `source.md`; when a link into another repo or org may be followed is in `category.md`.

## Writing

Find the kind of the file first, then open its definition and follow it:

- a README, and the one accepted form of its child lines: `folder-readme.md`;
- prose in a category folder, and the size budget: `category.md`;
- the doc of a source folder: `source.md`;
- a generated block (never edit one by hand; fix its source and run the writer): `generated.md`;
- the root `CONTENT.md` of a repo: `content.md`;
- queue, decisions, handoff, FAQ, rules, plans, specs and records, and which tree may hold them: `records.md` and the
  definition named after each.

A file whose kind has no definition is not allowed in a docs tree.

## Checking

Build `tarn-check` from `intisy/tarn` (`cmake -S . -B build && cmake --build build`) and run it on the repo root; the
usage it prints on a bad argument lists its options, and the definitions describe what each rule it reports means. Each line reads `refused|warned <path>:<line> <rule> <text>`; fix every
`refused` before finishing.
