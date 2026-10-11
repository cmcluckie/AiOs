# Petition schema

A petition is a bill of materials.  It carries no prose.  Every line is a code
that resolves to an entry defined exactly once in one of five tables, and the
referee's job is to check that every reference resolves and the relations hold.
Format is TOML.

## Codes

    <TABLE>.<path>@<version>

- TABLE is one of PUR, CAP, CMP, SPEC, TST, SNS.
- path is lowercase dotted segments: letters, digits, hyphen.  `tty.write`,
  `kernel.serial.8250`, `editor`.
- version is an integer, required on CMP and SPEC, optional elsewhere (absent
  means `@1`).  A code at a version is immutable once committed.  To change what
  a code means, publish the next version; petitions on the old version keep
  building what they built.

| Table | Lives in | One entry says |
| --- | --- | --- |
| PUR  | constitution/purposes/ | what a machine is for, in one sentence |
| CAP  | index/cap/   | one thing a component may touch; the finite vocabulary of permissions |
| CMP  | index/cmp/   | one component: source location, what it depends on (CMP), what it exposes (CAP), what it requires (CAP), its as-built spec |
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
    status      = "draft"                # draft | review | approved | rejected | expired | retired

    [[capability]]
    code        = "CAP.tty.write"
    reason      = "SPEC.hello.1@1"       # the clause that needs it; required
    scope       = "console0"             # table-specific narrowing; may be empty

    [[component]]
    code        = "CMP.hello@1"
    role        = "primary"              # primary | dependency | boot | toolchain
    admitted    = false                  # true only when index/cmp/<code>.toml exists and carries a signed admission

    [[test]]
    code        = "TST.hello.exact-bytes"

    [[sensor]]
    code        = "SNS.hello.boot-to-output"

## Relation rules the referee enforces

A petition that breaks any of these cannot be built.  The referee reports the
rule number and the offending code, nothing else.

1. Every code referenced resolves to an entry at that version.
2. Every `[[capability]]` has a `reason` that is a SPEC code, and that SPEC
   clause belongs to this petition's primary component.
3. Every SPEC clause of the primary component is bound by at least one TST.
4. Every CAP a CMP *requires* (per its index entry) appears in this petition's
   `[[capability]]` list.  No implicit grants.
5. Every CAP in `[[capability]]` is *exposed* by some CMP in `[[component]]`.
   You cannot be granted what nothing provides.
6. Every CMP's dependencies (per its index entry) appear in `[[component]]`,
   transitively.  Nothing is pulled in silently.
7. Every CMP in `[[component]]` with `admitted = true` has a signed admission
   in index/cmp/.  A build with any `admitted = false` component is a failure
   that names that code: that is the quartermaster's queue.
8. Every SNS names a SPEC clause and a consumer seat.  No consumer, no sensor.
9. `expires` is in the future at build time.
10. `status = "approved"` requires a valid detached review from every seat key, none
    rejecting, none countering, all for the petition's current commit.  See
    schema/signing.md.
11. The built image contains no CMP not listed, exposes no CAP not granted,
    and the VM definition has no device no CAP requires.  Checked against the
    artifact, not the petition.

## What is deliberately not here

- Prose.  Reasons are SPEC codes, not sentences.  The sentence lives in the
  spec file, once.
- Priorities or weights.  Those come from the purpose agent, outside the
  petition.
- Build instructions.  The builder derives them from the CMP index entries.
