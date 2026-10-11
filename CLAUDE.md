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
