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
- The whole repository is GPL-2.0-only, Linux's licence: every image we build
  compiles Linux's lines in with ours, so the whole is a work based on Linux
  (findings 0009).  A new upstream source with an incompatible licence is a
  finding before it is a lift.
- Ours and theirs never share a directory.  pool/ holds what we wrote.
  lifted/ holds what came from upstream, keeping its upstream notices, each part with
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
    posts/          the LinkedIn series: per day, one HTML file to paste from and its image

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
  commits; the key only works from his Windows shell.  Nothing reaches main
  unsigned.
- posts/commits.md is the ledger.  Every commit Claude asks for gets a row
  naming the post it belongs to, and the hash once Chris reports it.  A commit
  is never requested without a row; a row is never closed without a hash.
  Commits that belong to different posts are never mixed into one commit.

## Definition of done

A component is done, and a post about it may be written, only when all of
these are true.  `sh jig/donecheck` checks the ones a script can check and
names the component and the rule that failed.

1. It has an index entry, and the entry names its source directory.
2. It has an as-built spec: what it does, every port, register and
   instruction it touches, every byte it writes, its size.  Hand-written until
   the generator exists (plan 7.1), and saying so.
3. It has at least one spec clause, written by a human, one sentence, precise
   enough to fail.
4. Every spec clause has at least one test, and every test names what turns
   it red and a time limit.
5. Every test has been run by `sh jig/test` and passes in tests/results.txt,
   and `sh jig/mutate` has shown it can go red (or findings say why it cannot
   yet).  A test with no runner says so, in the index and in the post's
   addendum.
6. If any line came from upstream, it lives in lifted/ with an origin record
   that `jig/upstream verify` passes.
7. Every earlier program on the ladder still runs under it, unchanged.
8. The post's HTML file and its image exist and `jig/postcheck` passes,
   including "As built" and "Addendum: the tests".
9. Every commit for it is in the ledger with its post.

## The series

The series is for learning together, not showing off.  A reader should finish
each post able to follow what was done at the same level.  `sh jig/postcheck`
checks every post against the rules below and fails naming the file and the
rule; run it before asking Chris to commit anything in posts/.

**When.**  Every time an item or sub-item in plan/brief.md moves to [done],
Claude drafts a post for it.  The post is also the body of the commit that
moves the item to done.  One post per day: the date is the day it is meant to
go out, the next free day after the last post.  Chris posts one a day at most,
so the folder runs ahead of LinkedIn on purpose.

**Files.**  Two per post, same date, same name, in posts/:
- `YYYY-MM-DD.html`: everything Chris pastes into LinkedIn, in one file, in
  the order he pastes it.  He opens it in a browser and each section has a
  Copy button; LinkedIn keeps the formatting when pasted from a rendered page.
- `YYYY-MM-DD.png`: its image, 2400 by 1350.  One idea from the post made
  visual, built only from things that really happened: real output, real
  code, real numbers, drawn nicely (a transcript with the bug circled, Life's
  real grid as cells, Hanoi's real board at a real move).  Dark background,
  `AiOs · part N` top left in green, one big line in Chris's voice bottom
  left with a smaller line under it.  Anything exaggerated says so on the
  image.  Claude produces it.

Chris schedules the articles on LinkedIn; they are listed at
https://www.linkedin.com/article/manage/scheduled/ and edited from there.

**Voice.**  Chris's, every time.  Read posts/voice.md before drafting anything,
and update it in the same change whenever he corrects a draft.  When the AI
chose something, say so.  Never name his employer, its products or colleagues.

**The HTML file, top to bottom.**  Three sections, each `<section
id="...">` with a label and a Copy button outside the copied part:
1. `post`: the LinkedIn post that carries the article, as plain text.
2. `title`: the article's title, nothing else.
3. `article`: the article body.

**The post (section `post`).**  One line, nothing else: `AiOS, part N (Kernel
code: K lines).` then a one-sentence pull quote from the article in double
quotes, then `...`.  Single spaces after periods (LinkedIn collapses doubles).
K is the lines of code taken from Linux (lifted/, the .c files, no blanks, no
comments, as the as-built specs record them) on the day the post goes out:
57 from part 1, 69 from part 6.  LinkedIn attaches the article to it.

**The article (section `article`), top to bottom.**
1. `<p><em>The story behind AiOS, part N.</em></p>`
2. Prose sections, each under an `<h2>`, in this order where they apply: the
   problem (the textbook exercise and why it was picked, including if the AI
   picked it); what we built (how it works, in words, a few lines of code at
   most); what changed underneath and why; what we found in Linux, what was
   taken and what was cut; what nobody asked for.  A story, not a spec; keep
   it fun.  LinkedIn has two heading sizes, so only `<h2>` and `<h3>`.
3. `<h2>Read along</h2>`: the files to read, in order, numbered, each a link
   to the file at the post's last commit (`main` until it exists), one or two
   sentences each on what to look for.
4. `<h2>Try this</h2>`: one small change a reader can make and run, and what
   they'll see.  Claude runs it before it is published.
5. `<h2>As built</h2>`: a link to the as-built spec of every component the
   post added or changed.  A component with no as-built spec is a finding,
   and the post is not ready.
6. `<h2>Addendum: the tests</h2>`: every test for those components, grouped
   by component, each with what it checks (its spec clause), what turns it
   red, and its honest status from tests/results.txt and
   tests/mutations-results.txt: passed by the runner (and under what: the
   jig, KVM, or software emulation), failed, or no runner yet, and how many
   mutations it has caught.  A component with no test is not
   ready to post.  The addendum must list them all.
7. `<h2>Code on GitHub</h2>`, every link labelled:
   `<a href="<compare URL from the commit before the post's first to its
   last>">Every line that changed, on one page</a>.`  If the post's first
   commit is the repo's first, instead `<a href="<tree URL at its last
   commit>">The repo exactly as it stood when <what was done></a>.`  Then
   `The commits behind this post, in order:` and a numbered list, one per
   commit, from the ledger, each `<a href="<commit URL>"><subject></a>`.
   Then `<a href="https://github.com/cmcluckie/aios">The whole repo</a>.`
   Until a commit exists, its item and the compare link read
   `(hash when committed)` with no link.

**Honest.**  Every number and quote in a post is true of the repo at that
commit and checked against its source.  Say emulation vs the jig, checked by
hand vs recorded by a runner, written down vs actually run.  Jokes can
exaggerate; facts can't.

**Already posted.**  A post that is live on LinkedIn keeps its body exactly as
posted unless Chris changes it there too.  Its ending (the follow-along line
and the links) is kept to the current format in the repo, and Chris decides
whether to edit the live one to match.  Part 1 is live.
