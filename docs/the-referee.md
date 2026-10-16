# The referee

The one agent in AiOs with authority, and the one with no intelligence.  It reads the constitution and the catalogue and says yes or no, naming a rule and a catalogue number.  No model, no judgment.  This document is the reader's version of part 15 of the series and of posts/drafts/part-15-referee-questions.md, the working notes; the repo is the authority where they disagree.

## What it is

- Deterministic only.  Same input, same answer, no opinion anywhere.  A rule that needs an opinion is not the referee's: it is the committee's, whose job is to turn each judgment into a rule the referee can hold, or to `ask` a human.  Referee, committee, human, in that order; every step down is logged in decisions/.
- Permanent.  The first agent built, the one never stripped: it decides what is stripped.  It runs where nobody types: today the jig, which runs the ledger on every commit before any test; after the jig, inside AiOs at admission.  The rule names the condition, never the machine.
- Two rule books, two readers.  CLAUDE.md is the architects' (Chris and Claude): how AiOs gets designed, enforced by Chris telling Claude to redo it.  The constitution is AiOs's: what enters the system, enforced by the referee (the deterministic rules) or the committee (the judgment ones).  The referee reads only the constitution.  The test for anything unsure: who refuses if it is broken?  Chris: CLAUDE.md.  Referee or committee: constitution.  Nobody: a note, not a rule.
- Verbs and nouns.  A committee seat or the mechanic speaks to the referee only in a numbered list, one verb-noun line each.  The verbs (lift, add, remove, pin, retire, rerun, record, ask) live in constitution/verbs.toml and change only by a signed human commit.  The nouns are catalogue numbers, entering only when a signed petition creates the thing.  A verb or noun the referee does not know is a failure and a finding, never a green light.

## Where it is

- pool/AGT.referee@1/referee.c: the checks, in C, 556 lines.  `check FLAVOR` (rules 1, 9, 19), `hardware CODE SRC` (rule 12), `rules` (rules 2 to 8 and 21, written to tests/referee-results.txt), `lines FILE` (rule 13).
- pool/AGT.referee@1/referee: twelve lines of shell that build the C when the source is newer than the binary and hand over.
- jig/ledger: the decision ledger (rules on human-only paths, rule 20), a script because it reads git history; inside AiOs that job is admission.
- index/agent/referee@1.toml, its catalogue entry; constitution/purposes/0008@1.toml and petitions/0008.toml, its purpose and petition.
- Called by jig/aios (the builder) before and after every build, by jig/test on every commit since the last run, by .githooks/pre-commit, and by the four probes in tests/programs/.

## What it reads, and never writes

The constitution: constitution/human-only.toml (what only a human decides), constitution/keys/ (who may decide), constitution/verbs.toml (the eight verbs), constitution/instructions.toml (the fourteen instructions a program may never use), constitution/sources.toml (how much each upstream is trusted).  The catalogue: index/ (part, cap, machine, tool, agent), one file per thing, each with a plain sentence.  specs/, tests/, petitions/, decisions/.  It writes one file, tests/referee-results.txt.

## The rules

From schema/petition.md.  Built means a check exists and a mutation proves the check can fail.

A petition that breaks any of these cannot be built.  The referee reports the
rule number and the offending catalogue number, nothing else.

1. Every catalogue number referenced resolves to an entry at that version.
2. Every `[[capability]]` has a `reason` that is a SPEC code, and that SPEC
   clause belongs to a component listed in `[[component]]` (any role, not only
   primary).
3. Every SPEC clause of the primary component is bound by at least one TST.
4. Every CAP a component *requires* (per its catalogue entry) appears in this petition's
   `[[capability]]` list.  No implicit grants.
5. Every CAP in `[[capability]]` is *exposed* by some component in `[[component]]`.
   You cannot be granted what nothing provides.
6. Every component's run-time `depends` (per its catalogue entry) appear in
   `[[component]]`, transitively.  `build_depends` never appear in the image
   and never count toward this rule.  Nothing is pulled in silently.
7. Every component in `[[component]]` with `admitted = true` has a signed admission
   in its catalogue entry.  A build with any `admitted = false` component is a failure
   that names that catalogue number: that is the quartermaster's queue.
8. Every SNS names a SPEC clause and a consumer seat.  No consumer, no sensor.
9. `expires` is in the future at build time.
10. `status = "approved"` requires a decision of kind "vote" in decisions/ from
    every seat key, none rejecting, none countering, all for the petition's
    current commit, each in a commit signed by that seat.  One log for every
    choice, human or seat.  See schema/signing.md.
11. The built image contains no component not listed, exposes no CAP not granted,
    and the VM definition has no device no CAP requires.  Checked against the
    artifact, not the petition.
12. A program touches no hardware: no port I/O and no privileged instruction in
    its own compiled code.  Everything it does to the machine goes through a
    system call, so its capabilities are the whole of what it can do.  Checked
    by the referee after the build, on the program's own assembly.
13. A committee seat's or the mechanic's output is a numbered list, one line
    each, a verb then a noun and nothing else.  The verb is one of
    constitution/verbs.toml; the noun is a catalogue number that resolves.
    `record` and `ask` may name a verb that does not exist yet, as VERB.name,
    and that verb is never one that already exists.  Prose, an unknown verb or
    an unknown noun is refused, naming the line.  Checked by `pool/AGT.referee@1/referee lines`.
14. A verb acts only on a noun that already exists.  A thing that does not
    exist yet is asked for (`ask`), and a signed petition creates it.  How much
    a petition to lift from a source needs is the source's level in
    constitution/sources.toml: human (a signed petition, the default),
    committee (the committee alone, when the upstream commit is signed by a
    listed key; nothing until rule 10's signatures are verified), or open.
15. `remove` and `retire` make a new version of what they change and never
    edit the old one: petitions add, never swap.
16. An agent's key may decide exactly one kind of decision, a deviation (a
    rule in the constitution it looked at and did not follow, with what it
    built instead).  Every other kind is a human's.  An agent's key is made on
    the jig and never leaves it; one key per agent; constitution/keys marks it
    `role = "agent"`.
17. A program depends on a loader.  A program that depends on nothing cannot
    run; the loader is the nucleus it asks.
18. Every component in a build has its source in pool/ or lifted/.  A
    catalogue entry whose source directory is missing is refused.
19. Two or more programs on one machine need a loader that exposes
    CAP.cpu.share.  A loader that runs one program cannot run two.
    (17 to 19 were the referee's three unnumbered refusals until 2026-10-15.)
20. A gap in history closes with a decision that names the commit it amends
    (`amends = "<hash>"`).  The commit cannot change; the close sits beside it
    in the log, and `jig/ledger --commit` counts it.  The gap stays on the
    record.
21. Every catalogue entry carries one plain sentence saying what it is
    (`sentence`; a capability's is `touches`).  An entry without one is
    refused: a learning system's catalogue is read by people before it is read
    by the referee.  Decided 2026-10-16.

Status (2026-10-16): built and gating a build: 1, 9, 12, 17, 18, 19.  Built and reporting (gating after part 16): 2, 3, 4, 5, 6, 7, 8, 21.  Built, waiting for a committee: 13.  Ledger: human-only paths and 20.  Written, not yet checked: 10, 11, 14, 15, 16.

## How a change gets past it

1. A change: a petition asked for a part, a test went red, or a rule was decided.
2. The ledger (jig/ledger): a human-only file changed?  A decision in the same change names it, by a human's key.
3. The commit: Chris signs it.  On the jig the ledger runs again, on every commit since the last run.
4. The petition (referee check): every name is a file, not expired, two programs need a sharing kernel.
5. The builder (jig/aios build) compiles and links.
6. The assembly (referee hardware): the program's own machine instructions, against the forbidden list.
7. The tester (jig/test, jig/mutate): boots, types, compares; then breaks the code to prove the tests notice.
8. The paperwork (referee rules): rules 2 to 8 and 21 on every petition, to its own file.
9. The records: tests/results.txt, tests/mutations-results.txt, tests/referee-results.txt.
10. The post quotes the records.
11. Anything a committee says (referee lines): numbered verb-noun lines only.

## What it cannot see yet

findings/0016.md and findings/0017.md, in full.  In short: `by` on a decision is a claim until commit signatures are verified (plan 3.5); rule 12 reads assembly and raw bytes would get past it, and a memory-mapped device (the screen, part 16) is not an instruction at all, so the real fix is ring 3 (plan 9.13); the ledger knows paths, not meaning; the four petitions refused under rules 4 and 6 wait on the kernel being built from the petition (plan 5.8, part 16); nothing produces verb-noun lists yet because there is no committee; strictness per rule (plan 3.8) is decided, not built.

## The decisions behind it

decisions/0001 to 0009, each with what was asked, the options, what was chosen, and why.  The working notes with the reasoning: posts/drafts/part-15-referee-questions.md.  The open questions answered on 2026-10-15, all twenty-two, are there too.
