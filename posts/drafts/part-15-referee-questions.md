# The referee deep dive, 2026-10-14: what was decided, what is open

Working notes for part 15.  The article does not go out until every item is settled.

## The frame, Chris's, 2026-10-14

The author will always be able to out-think the reviewer.  Nobody will ever solve that, and thinking AiOs might is naive.  The question with an answer is: how are the gaps kept few, and how fast does one close when a human has to decide?  Findings are the gap record, the decision ledger is the close record, molt is the system admitting a gap.  Two numbers, both from files in the repo: open findings, and time from a finding to the decision that closes it.  Never say it's easy.

## Decided tonight (in plan/brief.md)

- 3.4: rule 11 checked by manifest plus the image's symbol table.
- 3.5: admissions are SSH-signed commits verified against constitution/keys; reviews are detached SSH signatures.  "If it's good enough for Linus."
- 3.8: deviations are allowed but logged; strictness per rule in one human-only file; a deviation with no decision is a failure.
- 3.9: the rule book codified, one check and one mutation test per rule.
- 3.10: rule 12 measured, every part's hardware footprint compared to its as-built spec.
- The referee holds only deterministic rules.  A judgment goes to the committee, whose job is to turn it into a rule; what it cannot turn into a rule goes to a human.  All steps logged.
- Every check in the referee gets a mutation test.  Rule, to be built in (3.9).
- The three unnumbered refusals in jig/aios become rules with numbers.

## Decided later, by voice, 2026-10-14

- A committee's output is a numbered list, one line each, verb then noun.  The referee refuses any other shape, and any verb it does not know.  The referee fails on anything not in its catalogue; that fault is the design.
- Nouns are the catalogue.  A noun enters only when a signed petition creates the thing it names.  The list is large and that is fine.
- Verbs live in the constitution, beside the keys and roles, not in the catalogue.  Adding a verb is a constitution change: human-only, signed, rare enough to be a post.  Kept apart so the door that admits a part never admits a power.
- The verbs are few: lift, add, pin, retire, rerun, record, ask, or near that.  Start with the ones the referee can check.
- A committee can ask for a verb (record a finding) and never use one it asked for in the same output.  Ask and add are separate signed steps.
- Molt is the gap-closing step, not a growth step.  A finding is a complaint; it never becomes a petition on its own.  Someone asks, and someone names which rooms the fix may touch.  The sealed system may notice the smell.  It may not walk in.

## Found tonight

- Rules 2 and 3 hold in all eight petitions (checked by script, 36 capabilities).  The referee never checked them; they held by hand.
- Rules 4 and 6 fail in petitions 0004 to 0007: twenty short dependency lists, one shape, no code affected.
- Rule 8 fails in 0000 and 0004: the two sensors are a code and nothing else.

## Chris, 2026-10-14 23:36, next open items

- The referee refuses any committee or mechanic output that isn't a numbered list of one verb-noun line each.
- The verb list lives in the constitution next to keys and roles, starts with lift, add, remove, pin, retire, rerun, record and ask, and changes only by signed human commit.
- Nouns are catalogue numbers only, entering through a signed petition that creates the thing.
- A verb or noun the referee doesn't know is a failure and a finding, never a green light.
- The committee may use 'record' or 'ask' for a missing verb but never use that verb in the same output.

## Done 2026-10-15, early (decision 0003, findings 0017)

- `jig/aios rules`: rules 2 to 8 on every petition, writes tests/referee-results.txt.  SPEC.referee.2-8@1, TST.referee.2-8@1, seven mutations, each proven red by hand on Chris's machine; the jig run is the one that counts.
- Pushed back once: the four petitions were not edited to pass.  Rules 4 and 6 lead to rule 5, and nothing exposes CAP.hv.kvmclock@1.  That is a catalogue decision, not a list edit.

## Built 2026-10-15, early (decision 0004)

- Rule 13 in schema/petition.md; constitution/verbs.toml with the eight verbs; `jig/aios lines`; SPEC.referee.13@1; TST.referee.verb-noun@1 with three mutations, each proven red by hand.  Verb and noun defined in CLAUDE.md.

## Decided 2026-10-15, afternoon (rules 14 to 16, plan 3.11)

- A verb acts only on what exists; a lift is asked for and a petition creates the part.
- Trust by source, configurable, default human: constitution/sources.toml with human / committee / open; committee needs upstream signatures, so nothing until 3.5.
- Findings get catalogue numbers (FND.NNNN@1) so `record` has a noun.
- Agent keys may decide one kind: a deviation.  One key per agent, made on the jig, the private half never leaves it; a signature names the role that ran, not the brain behind it.  Locked in 2026-10-15.
- remove and retire make a new version, never edit the old.

- The referee is permanent and runs where nobody types.  Today: jig/test runs the ledger on every commit since the last run, before any test, and refuses a commit that fails.  Built 2026-10-15.  After the jig: the same check at admission inside AiOs.

## Open, in order

1. One rule book or three (tenets, petition rules, definition of done)?  Proposed: one, numbers 1 to 12 unchanged, the rest added after.
2. Must every tenet name the rules that enforce it?  Proposed: yes.
3. Does rule 12's instruction list move into the constitution?  Proposed: yes.
4. The referee's queue and the jig's signing key: where the key lives, whether the queue is a file like the builder's, whether a post may quote only a referee-written verdict.  (The second lock is built; this is the rest.)
5. Who exposes CAP.hv.kvmclock@1 (TOOL.qemu@1, the machines, or a new MACHINE entry), and does a machine whose spec says "no timer" expose CAP.dev.pit@1 because microvm forces the 8254?  Then the four petitions' lists, and rules 2 to 8 into the build gate.
6. Chris's questions 8 to 22 from the list, not yet discussed.
7. The article: add how Claude would break the model today, and the five tightenings.
