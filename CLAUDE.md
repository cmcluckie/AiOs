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
- Linux is read, never booted.  A segment is lifted from it into its own
  directory under lifted/ with the hands it held cut; the cut is recorded in
  the as-built spec.  Nothing enters as "the kernel".
- Ours and theirs never share a directory.  pool/ holds what we wrote, MIT.
  lifted/ holds what came from upstream, under its own licence, each part with
  an origin.toml that records exactly enough to lift it again: repository,
  commit, commit date, file, blob id, line ranges, who lifted it and when, and
  what was cut.  A component with one lifted line lives in lifted/.
- Upstream is watched, never merged.  jig/upstream verify proves every origin
  record against the upstream clone; jig/upstream watch names every upstream
  commit that touched a lifted line since.  A change that matters is a new
  version of the part through the door; petitions on the old version keep
  building what they built.
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
    pool/           admitted source we wrote, one directory per CMP code at a version; MIT
    lifted/         admitted source from upstream, same layout, each with origin.toml; its own licence
    flavors/        one TOML file per flavor: the petitions one machine runs
    findings/       what the schema or plan could not express, and what was decided
    plan/           the project plan, brief and full
    jig/            the gen-zero build host, declared before it existed; aios, the referee and builder placeholder
    pi/             the bare repository that refuses unsigned pushes
    posts/          the LinkedIn series, one post per day with its image

## Terms

We are inventing language.  These are the words, each defined once.  Use them
exactly; when a new one is needed, add it here in the same commit.

- **Constitution.**  This repository.  The only authority on what a machine
  contains and why.  Every commit is signed.
- **Tenet.**  One of the ten rules in constitution/tenets.md that no vote can
  amend.
- **Purpose (PUR).**  One sentence saying what a machine is for.  The first
  thing a petition names.
- **Petition.**  A bill of materials in TOML: the purpose, the capabilities it
  needs with a spec clause as the reason for each, the components, the tests
  and sensors.  No prose.  The one door.
- **Flavor.**  The set of petitions one machine runs, as flavors/<name>.toml.
  Every flavor is also an entry in .vscode/launch.json so it appears in the
  Run and Debug dropdown; adding a flavor means adding that entry.
  "Build flavor hello" is what pressing play means.  A flavor with more
  programs than the loader can run is refused until rung 9.6.
- **Code.**  A name a petition can use: `TABLE.path@version`.  Seven tables:
  PUR purpose, KEY signing key, CAP capability, CMP component, SPEC clause,
  TST test, SNS sensor.  Each code is defined exactly once, in one file.
- **Index.**  The catalogue of codes: what can be named.  index/cap/ and
  index/cmp/.
- **Capability (CAP).**  One thing a component may touch.  Small, finite
  vocabulary.
- **Component (CMP).**  One thing that exists: a program, a driver, the loader,
  a toolchain, a machine.  Its index entry says what it depends on, what it
  exposes, what it requires, and where its source is.
- **Spec (SPEC).**  One as-intended sentence precise enough to fail.  Written
  by a human.  The as-built spec is the other half: derived from the source,
  says what the code actually does.
- **Test (TST).**  Binds one spec clause and names the condition that turns it
  red.  A test that cannot go red is decoration.
- **Sensor (SNS).**  A test that keeps running in production: a question, a
  threshold, a consumer.
- **Pool.**  Admitted source we wrote, one directory per component at a
  version.  Being in git is not being in the pool; a signed admission is.
- **Lifted.**  lifted/: admitted source that came from upstream, same layout as
  the pool, each part with an origin record.  Ours and theirs never mix.
- **Origin.**  origin.toml beside a lifted part: repository, commit, date, file,
  blob id, line ranges, who and when, what was cut.  Enough to lift again.
- **Historian.**  Watches upstream against every origin record and reports
  what moved; never merges.  Today: `jig/upstream`.
- **Segment.**  A piece lifted out of Linux into the pool on its own, with the
  hands it held cut.  The serial segment is 57 lines.
- **Lift, cut.**  Lifting is taking a segment from Linux with line-numbered
  provenance.  Cutting is removing what it held onto (the tty layer, the
  device model, the allocator).  Both are recorded in the segment's header.
- **Loader.**  CMP.loader: takes over from the machine, runs one program
  through the door, halts when it exits.  The whole landlord, for now.
- **Landlord, tenant.**  A kernel is a landlord: it shares a machine between
  tenants.  With one tenant there is nothing to share, so there is no kernel,
  only a loader.  A program is a tenant even when it is the only one.
- **Door.**  The table of function pointers the loader hands a program.  The
  only thing a program sees of the machine.  Later petitions append entries;
  nothing ever moves.
- **Program.**  A flat binary linked with the loader's program.ld, entered at
  byte 0 with the door.  Hello, Hanoi, the rungs.
- **Machine.**  The VM definition, a CMP of kind hardware: one serial port,
  as little else as the hypervisor allows.  CMP.machine.serial-only.
- **Hypervisor.**  QEMU with KVM.  Scaffold, through the door later.
- **Jig.**  Gen zero: the NixOS host that builds and tests but is not the
  system.  Bloat on purpose; every tool on it has a row in design.md's tool
  table with how it leaves.
- **Scaffold.**  A component the jig provides for now (the compiler, the
  hypervisor), marked `scaffold = true`, rebuilt when its pool version is
  admitted.
- **Referee.**  Checks that every code resolves and every rule holds.  No
  model, no judgment.  Fails loud, naming a rule and a code.  Today: `jig/aios
  check`.
- **Builder.**  Turns a checked flavor into an image, deterministically.
  Decides nothing.  Today: `jig/aios build`.
- **Quartermaster, tester, mechanic, committee, seat, historian, cartographer,
  watchman, gateway.**  The agents and the governing body, defined in
  design.md.  None exist yet.
- **Admission.**  Two signed commits that set an entry's status to admitted.
  Until then an entry is draft and a petition naming it is pending.
- **Well-formed, buildable.**  The referee's two phases.  Well-formed: every
  code resolves or is listed as pending.  Buildable: everything admitted and
  rules 1 to 7 hold.
- **Finding.**  Something the schema or plan could not express, recorded in
  findings/ with what was decided.  Findings are the project's most useful
  output.
- **Ledger, bloat figure.**  The list of lines in an image that no petition
  justifies, with why each is there.  findings/0002.md is 0000's.
- **The ladder, rung.**  The sequence of petitions after 0000, one textbook
  program each, each needing exactly one new thing from the machine.
- **The Pi.**  The bare repository that is the authority, verifying every
  signature on push.  GitHub is a mirror.
- **Gen zero, one, two.**  The jig; the first things living entirely by
  petition; a machine whose purpose is to build other instances of itself.
- **Grow, prove, strip, seal, molt.**  The lifecycle, in design.md.

## How we work

- Chris decides and judges whether what was built matches the purpose.  Claude
  digs, including reading kernel source, and stages files.  Chris signs and
  commits; the key only works from his Windows shell.
- Every time an item or sub-item in plan/brief.md moves to [done], Claude drafts
  a LinkedIn post for it: short, plain English, no project vocabulary, numbered
  "AiOS, part N", tagged #AiOS.  The post lives in posts/ as
  posts/YYYY-MM-DD.md with the image beside it as posts/YYYY-MM-DD.png, same
  name, one post per day: the date is the day it is meant to go out, the next
  free day after the last post in the folder.  Chris posts one a day at most,
  so the folder runs ahead of LinkedIn and that is fine; nobody is watching
  the repo yet.  The post is also the body of the commit that moves the item to
  done.  posts/commits.md is the ledger: every commit Claude asks for gets a
  row naming the post it belongs to, and the hash once Chris reports it; a
  commit is never requested without a row, and a row is never closed without a
  hash.  Post format, learned from part 1:
    - First line: "AiOS, part N." followed by a one-sentence pull quote from
      the body in quotes and an ellipsis.  LinkedIn shows two lines above
      "see more" when an image is attached; the quote is the hook.
    - Posted with an image: real terminal output from the real build, never a
      mock-up, so people can see what was made.  Claude produces the image;
      it is the .png beside the post.
    - Single spaces after periods in the post itself; LinkedIn collapses
      double spaces anyway.
    - Ends: "Come along!" on its own line, blank line, "#AiOS", then
      "Here is the repo on GitHub: <link>".  LinkedIn rewrites URLs to
      lnkd.in, so the label says where the link goes.
    - Plain English.  No project vocabulary (petition, referee, jig, CAP)
      without a plain-words gloss in the same sentence.
    - Honest about state: emulation vs the jig, hand-checked vs recorded by
      a runner, draft vs admitted.
