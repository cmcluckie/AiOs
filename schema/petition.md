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

- TABLE is one of PUR, CAP, PART, MACHINE, TOOL, SPEC, TST, SNS.  PART,
  MACHINE and TOOL together are the component tables (until 2026-10-13, one
  table, @@component@@; findings 0014).
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
`review`; reviews set `approved` or `rejected`; time sets `expired`.

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
10. `status = "approved"` requires a valid detached review from every seat key, none
    rejecting, none countering, all for the petition's current commit.  See
    schema/signing.md.
11. The built image contains no component not listed, exposes no CAP not granted,
    and the VM definition has no device no CAP requires.  Checked against the
    artifact, not the petition.
12. A program touches no hardware: no port I/O and no privileged instruction in
    its own compiled code.  Everything it does to the machine goes through a
    system call, so its capabilities are the whole of what it can do.  Checked
    by the referee after the build, on the program's own assembly.

## What is deliberately not here

- Prose.  Reasons are SPEC catalogue numbers, not sentences.  The sentence lives in the
  spec file, once.
- Priorities or weights.  Those come from the purpose agent, outside the
  petition.
- Build instructions.  The builder derives them from the component catalogue entries.
