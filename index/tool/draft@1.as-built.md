# TOOL.draft@1 as built

Hand-derived from jig/draft and draft/prompt.txt on 2026-10-20, pending the generator (plan 7.1).
A scaffold: a shell script standing in for the quartermaster's draft step.

## What it does
Gives one model a purpose and the path of a program's source, and keeps the
model's reply as a draft petition only if the referee can read it.  The model is
shown the purpose's sentence, the layout of a petition (from schema/petition.md),
one finished petition (petitions/0001.toml) and the catalogue, one line per entry.
It may look at the program only through the reader (default `jig/reader
--source v7`), by one tool that takes list, read, check or ports.  It decides
nothing: the referee does.

## How to ask
`draft PURPOSE SOURCE`, for example `draft PUR.0011@1 usr/src/cmd/cat.c`.
Exit 0 a draft was kept, 1 refused, 2 cannot run here.

## What it does with the reply
Lines that are code fences are dropped; the rest is given to `referee petition`.
If the referee accepts it, it is kept as out/draft/ID/petition.toml (ID is the
next free number in petitions/, or DRAFT_ID).  It is not put in petitions/: a
person reads it and commits it.  If the reply is empty, or the referee refuses
it, nothing is kept and the reason is printed.  After a refusal, and while tries
are left, the model is given the referee's reason and its own last reply and
asked again.

## Refusals
Exit 1: a wrong number of arguments; a purpose with no file or no sentence; no
free petition number; the last try refused.  Exit 2: no jq; no date six months
on; the reader or the referee cannot run here; the model call cannot run (cap
reached, no credentials; see jig/model).

## Everything it touches
- Reads: constitution/purposes, schema/petition.md, petitions/ (the number and
  the example), index/ (the catalogue), draft/prompt.txt,
  tests/fixtures/draft/petition-good.toml (to check the referee runs).
- Calls: jig/reader (as many times as the model asks, at most DRAFT_TURNS model
  calls in one try, default 25), jig/model (the paid call; the cap is
  MODEL_MAX_COST, default 0.25), the referee.
- Writes: out/draft/ID/petition.toml when a draft is kept; one run record in
  runs/ for every try, kept or not, through jig/runs (item draft-ID, with the
  prompt's fingerprint and the sitting).  A scratch folder, removed on exit.
- Settings: DRAFT_TRIES (default 1), DRAFT_TURNS, DRAFT_MAXTOKENS (8000 per
  reply), DRAFT_OUT, DRAFT_ID, DRAFT_TODAY, DRAFT_EXPIRES (default six months
  on), READER, REFEREE, MODEL_NAME (default bedrock:global.anthropic.claude-sonnet-4-6).
- Network: only through jig/model.  No ports, no registers.

## Size
135 lines in jig/draft (shell), 15 in draft/prompt.txt.  Needs jq, git, sed, awk.
