# AiOS

AiOS is a purpose-driven, self-building operating system.  This repository is
its constitution: the purposes, the invariants, the petitions that admit every
component, the index of what exists, the specs that say what each thing should
do, the tests that can prove it wrong, and the sensors that keep proving it in
production.  Nothing runs anywhere unless a signed petition in this repo says
why.  The full design is in design.md; read it before doing anything.

## Rules that bind you as much as any component

- Nothing enters this repo without a reason that traces to a purpose.
- Source only.  Nothing pre-compiled.  Reviewed before entry.  No steady-state
  dependency on anyone upstream.
- Tests are written to fail.  A sensor or test that cannot go red is
  decoration.
- Petitions are structured data, not prose: codes that resolve to entries in
  the index, the specs, the tests and the sensors, each defined exactly once.
- Failures name codes, never raw tool output.  "missing CAP.TTY.RAW", not
  "undefined reference".
- Admit the smallest reviewable unit a failure asked for, never the package
  upstream ships it in.
- Nothing gets installed on the jig without a row in design.md's tool table,
  and the row is not complete until "how it gets reduced" is filled in.
- You propose.  The referee decides.  You never install.
- Linux is read, never booted.  A segment is lifted from it into its own pool
  directory with the hands it held cut; the cut is recorded in the as-built
  spec.  Nothing enters as "the kernel".
- Every lifted segment arrives naked.  Upstream wrote no spec and no test for
  it; we write both before it is admitted, and the size of that job is what
  keeps segments small.
- Petitions add, never swap.  Segments talk only through declared doors, so a
  later petition stacks on an earlier one without removing anything.  The
  loader is separate from the program from petition 0000 on: a tenant is a
  tenant even when it is the only one.
- A kernel appears only when a petition asks for two things that must share.
  Until then the loader is the whole landlord.
- Every commit is signed.  An unsigned commit is not part of the constitution.
- Two spaces after a period.

## Layout

    constitution/   tenets and purposes; the part no vote can amend
    schema/         the petition format and the relation rules
    petitions/      one TOML file per petition, numbered
    index/          CMP.* entries: what exists, what it depends on, what it exposes
    specs/          SPEC.* clauses: as-intended behaviour, one testable sentence each
    tests/          TST.* entries: each bound to the spec clause it can turn red
    sensors/        SNS.* entries: question, threshold, consumer
    pool/           admitted source, one directory per CMP code at a version

## How we work

- Chris decides and judges whether what was built matches the purpose.  Claude
  digs, including reading kernel source, and stages files.  Chris signs and
  commits; the key only works from his Windows shell.
- Every time an item or sub-item in plan/brief.md moves to [done], Claude drafts
  a LinkedIn post for it: short, plain English, no project vocabulary, numbered
  "AiOS, part N", tagged #AiOS.  The post is the body of the commit message that
  moves the item to done, under the subject line, so the history and the series
  are the same record.
