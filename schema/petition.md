# Petition schema, version 2

Version 1 was written before any entry existed and could not say what a
petition looks like while the things it names are still being admitted.
Version 2 adds the pending state and the two-phase check.  Findings 0000
records what forced each change.  Amended once since: role gained the
value `machine` for the VM definition and the hypervisor it runs on (finding 14).

A petition is a bill of materials.  It carries no prose.  Every line is a catalogue number
that resolves to an entry defined exactly once in one of five tables, and the
referee's job is to check that every reference resolves and the relations hold.
Format is TOML.

## Catalogue numbers

    <TABLE>.<path>@<version>

- TABLE is one of PUR, CAP, PART, MACHINE, TOOL, AGT, SPEC, TST, SNS, FND.  PART,
  MACHINE and TOOL together are the component tables (until 2026-10-13, one
  table, @@component@@; findings 0014).  FND is a finding, findings/NNNN.md,
  numbered so a verb-noun line can name one (2026-10-15).  AGT is an agent,
  index/agent/, a component like a part: the referee is AGT.referee@1.
- path is lowercase dotted segments: letters, digits, hyphen.  `tty.write`,
  `kernel.serial.8250`, `editor`.
- version is an integer, required on PART, MACHINE, TOOL and SPEC, optional elsewhere (absent
  means `@1`).  A catalogue number at a version is immutable once committed.  To change what
  a catalogue number means, publish the next version; petitions on the old version keep
  building what they built.

| Table | Lives in | One entry says |
| --- | --- | --- |
| PUR  | constitution/purposes/ | what a machine is for, in one sentence |
| CAP  | index/cap/   | one thing a component may touch; the finite vocabulary of permissions |
| PART | index/part/  | one part of the nucleus or a program: source location, what it depends on, what it exposes (CAP), what it requires (CAP), its as-built spec |
| MACHINE | index/machine/ | one VM definition: the devices it presents (CAP), its as-built spec |
| TOOL | index/tool/  | one scaffold the jig lends for now: the compiler, the hypervisor |
| SPEC | specs/       | one as-intended clause: a single sentence precise enough to fail |
| TST  | tests/       | one test: the SPEC clause it binds, and the condition under which it goes red |
| SNS  | sensors/     | one production sensor: the question, the threshold, the consumer, the SPEC clause it keeps proving |

## Petition fields

    [petition]
    id          = "0000"                 # four digits, unique, never reused
    purpose     = "PUR.0000@1"           # exactly one; a machine with several purposes has several petitions
    submitted   = 2026-10-10             # date
    submitter   = "chris"                # a key id from constitution/keys/
    expires     = 2027-04-10             # date; required; the referee refuses to build after it
    supersedes  = ""                     # a prior petition id, or empty
    status      = "draft"                # draft | pending | review | approved | rejected | expired | retired

    [[capability]]
    code        = "CAP.tty.write"
    reason      = "SPEC.hello.1@1"       # the clause that needs it; required
    scope       = "console0"             # table-specific narrowing; may be empty

    [[component]]
    code        = "PART.hello@1"
    role        = "primary"              # primary | dependency | boot | toolchain | machine
    admitted    = false                  # true only when the component's catalogue entry exists and carries a signed admission

    [[test]]
    code        = "TST.hello.exact-bytes"

    [[sensor]]
    code        = "SNS.hello.boot-to-output"

## Two phases

The referee checks a petition in two phases, and a petition is only
*well-formed* or *buildable*, never simply valid.

**Well-formed** (checked on every commit): the file parses; every catalogue number is
syntactically a catalogue number; every catalogue number either resolves to an entry at that version
or is listed in `[[component]]` with `admitted = false`; rules 2, 3, 8, 9 hold
against whatever entries exist.  A well-formed petition with unadmitted
components is in state `pending`, and the referee emits one line per
unadmitted catalogue number.  That list is the quartermaster's queue; it is not an
error.  Closes finding 3.

**Buildable** (checked before any build): well-formed, every component
`admitted = true` with a signed admission, and rules 1, 4, 5, 6, 7 hold
against the now-complete catalogue.  Rule 11 is checked after the build,
against the artifact.

`status` therefore gains a value: `draft | pending | review | approved |
rejected | expired | retired`.  The referee sets `pending`; a human sets
`review`; the seats' votes set `approved` or `rejected`; time sets `expired`.

## Who decides a petition (decision 0051)

A petition is a human decision, whoever drafted it.  `petitions/` is on the
human-only list (constitution/human-only.toml), so a change to a petition is
refused unless a decision in the same change names it.  The builder refuses to
build a flavor unless every petition it names is named, by its exact file, in
the `touches` of a decision whose `by` is a key in constitution/keys/
(SPEC.builder.4); a decision naming `petitions/*` decides nothing.  The
`submitter` line is not proof of who wrote a petition: a decision that decides
one says who drafted it, in `petition_drafted` (a person, Claude, or a model and
how it was called).  Until seats exist this is the root key reviewing alone, as
schema/signing.md says; rule 10's votes come later and add to it.

## Relation rules the referee enforces

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
    artifact, not the petition: `pool/AGT.referee@1/referee artifact`, which the
    builder runs on every kernel it links, from 2026-10-23 (decision 0065).  A
    part may switch a capability off unless asked (`off_unless_asked`: the
    builder sets GRANT_ for each capability asked for, and the named symbol is
    in the image only when it is on); the referee looks for the symbol with nm.
    What each kernel exposed unasked when the check began is listed by flavor
    in constitution/extras.toml, and that list only shrinks: a capability not
    asked for and not listed, or listed and no longer there, is refused.  The
    VM half of this rule is not checked yet.
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
22. A test program booted on a flavor's kernel makes only the system calls
    that flavor's petitions ask for (or constitution/extras.toml lists for it),
    unless it says it expects the call refused.  Every program written against
    the system call table says where it boots (`Boots on:`), and one that
    expects a refusal says which (`Expects refusal:`).  A program that has to
    succeed at something boots on a flavor allowed to do it, not on the flavor
    under test.  Checked by `pool/AGT.referee@1/referee probes`.  Decided
    2026-10-23 (decision 0065).

## What is deliberately not here

- Prose.  Reasons are SPEC catalogue numbers, not sentences.  The sentence lives in the
  spec file, once.
- Priorities or weights.  Those come from the purpose agent, outside the
  petition.
- Build instructions.  The builder derives them from the component catalogue entries.
