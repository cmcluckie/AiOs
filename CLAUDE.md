# AiOs

AiOs is a purpose-driven, self-building operating system.  The name is
AiOs, lower-case s, everywhere: posts, docs, code.  Parts 1 to 12 went out as
"AiOS" and stay that way; nothing new uses it (findings 0014).  This repository is
its constitution: the purposes, the invariants, the petitions that admit every
component, the catalogue of what exists, the specs that say what each thing should
do, the tests that can prove it wrong, and the sensors that keep proving it in
production.  Nothing runs anywhere unless a signed petition in this repo says
why.  The full design is in design.md; read it before doing anything.

## Rules that bind you as much as any component

- Nothing enters this repo without a reason that traces to a purpose.
- Source only.  Nothing pre-compiled.  Reviewed before entry.  No steady-state
  dependency on anyone upstream.
- Tests are written to fail.  A sensor or test that cannot go red is
  decoration.
- Petitions are structured data, not prose: catalogue numbers that resolve to entries in
  the catalogue, the specs, the tests and the sensors, each defined exactly once.
- Failures name catalogue numbers, never raw tool output.  "missing CAP.TTY.RAW", not
  "undefined reference".
- Admit the smallest reviewable unit a failure asked for, never the package
  upstream ships it in.
- Rust is the default language (tenet 11, decision 0097).  A new piece starts in
  Rust; C only for a reason the petition names: lifted from Linux, linked
  straight against a lifted C part where a Rust boundary costs more, the jig's
  toolchain cannot build Rust for the target yet, or a reason Chris accepts.
  What moves off the jig into an agent starts in Rust.
- Nothing gets installed on the jig without a row in design.md's tool table,
  and the row is not complete until "how it gets reduced" is filled in.
- You propose.  The referee decides.  You never install.
- Linux is read, never booted.  A part is lifted from it into its own
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
- Every lifted part arrives naked.  Upstream wrote no spec and no test for
  it; we write both before it is admitted, and the size of that job is what
  keeps parts small.
- Petitions add, never swap.  Parts talk only through declared interfaces, so a
  later petition stacks on an earlier one without removing anything.  The
  loader is separate from the program from petition 0000 on: a program is a
  program even when it is the only one.
- A nucleus appears only when a petition asks for two things that must share.
  Until then the loader is the whole nucleus.
- Every commit is signed.  An unsigned commit is not part of the constitution.
- Two spaces after a period.

## Layout

    constitution/   the rule book for AiOs: purposes, keys, verbs, sources, human-only
    schema/         the petition format and the relation rules
    petitions/      one TOML file per petition, numbered
    index/          part/, machine/, tool/ and cap/ entries: what exists, what it depends on, what it exposes
    specs/          SPEC.* clauses: as-intended behaviour, one testable sentence each
    tests/          TST.* entries: each bound to the spec clause it can turn red
    sensors/        SNS.* entries: question, threshold, consumer
    pool/           admitted source we wrote, one directory per code at a version; GPL-2.0-only; AGT.referee@1 is the first agent here
    lifted/         admitted source from upstream, same layout, each with origin.toml; GPL-2.0-only, Linux's
    flavors/        one TOML file per flavor: the petitions one machine runs
    findings/       what the schema or plan could not express, and what was decided
    plan/           the project plan, brief and full
    agent/          the layered mechanic: director and builders, in Rust, run in WSL beside Ollama; the jig only runs its tests (findings 0027)
    jig/            the gen-zero build host, declared before it existed; aios, the builder placeholder; test, mutate, upstream
    pi/             the bare repository that refuses unsigned pushes
    posts/          the LinkedIn series: per day, one HTML file to paste from and its image

## Terms

We are inventing language.  These are the words, each defined once.  Use them
exactly; when a new one is needed, add it here in the same commit.

- **Constitution.**  This repository.  The only authority on what a machine
  contains and why.  Every commit is signed.
- **Tenet.**  One of the eleven beliefs in design.md that no vote can amend.
  The architects' (this file's side), never the referee's: a tenet is a
  belief, a rule is a check.  Moved out of constitution/ on 2026-10-15.
- **Two rule books, two readers.**  This file is the rule book for the two
  architects, Chris and Claude: how AiOs gets designed, enforced by Chris
  telling Claude to redo it.  The constitution is the rule book for AiOs:
  what enters the system, enforced by the referee (the deterministic rules)
  or the committee (the judgment ones, which it must turn into
  deterministic rules or `ask`).  "The constitution is CLAUDE.md, but for
  AiOs."  The referee reads only the constitution, never this file,
  design.md or the definition of done.  A practice of ours may be copied
  into the constitution, as a separate rule with its own number.  Test for
  anything unsure: who refuses if it's broken?  Chris: here.  Referee or
  committee: constitution.  Nobody: a note, not a rule.  Decided by Chris
  by voice, 2026-10-15.
- **Purpose (PUR).**  One sentence saying what a machine is for.  The first
  thing a petition names.
- **Petition.**  A bill of materials in TOML: the purpose, the capabilities it
  needs with a spec clause as the reason for each, the components, the tests
  and sensors.  No prose.  The one door.
- **Flavor.**  The collection of parts defined by each application's
  petition.  flavors/<name>.toml names the petitions; the parts come from them.
  Every flavor is also an entry in .vscode/launch.json so it appears in the
  Run and Debug dropdown; adding a flavor means adding that entry.
  "Build flavor hello" is what pressing play means.  A flavor with more
  programs than the loader can run is refused until rung 9.6.
- **Catalogue number.**  The name a petition uses for anything:
  `TABLE.path@version`.  Nine tables: PUR purpose, KEY signing key, CAP
  capability, PART part, MACHINE machine, TOOL tool, SPEC clause, TST test, SNS
  sensor.  Each catalogue number is defined exactly once, in one file.  Called
  a "code" until 2026-10-23 (findings 0014); the TOML key is still `code`.  AGT, for agents, comes when the first one is admitted.
  Until 2026-10-13 parts, machines and tools shared one table, CMP; Chris
  split it because language matters (findings 0014).  Older findings say CMP.
- **Sentence.**  One plain line on every catalogue entry saying what the
  thing is, for people; rule 21 refuses an entry without one.  A
  capability's is `touches`.  Decided by Chris 2026-10-16.
- **Catalogue.**  Everything that can be named, filed by catalogue number:
  index/cap/, index/part/, index/machine/ and index/tool/.  The folder is
  index/ (formerly the index); it kept the name so published links keep
  working (findings 0014).  It holds descriptions, not code: like a card
  catalogue, each entry says what a thing is, what it needs and where its
  code lives (lifted/ or pool/).
- **Capability (CAP).**  One thing a component may touch.  Small, finite
  vocabulary.
- **Component.**  Whatever a petition asks for, as one word for three
  tables: a part (PART: a driver, the kernel, a program), a machine (MACHINE:
  a VM definition) or a tool (TOOL: a scaffold the jig lends, like the
  compiler).  Each catalogue entry says what it depends on, what it exposes, what
  it requires, and where its source is.
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
- **Historian.**  Run on demand by the quartermaster, the referee or Chris.
  Looks up the Linux commit a part was lifted from (its origin record),
  checks whether any later commit touched those lines (mechanical: the only
  half the referee calls), and if one did, reads the change and says whether
  the part needs to change (AI).  Which branches AiOs follows (mainline,
  stable, long-term support) will be a setting of AiOs, and the historian
  will weigh a fix by the branch it is on.  Reads source through the shared
  source reader it shares with the quartermaster, never its own (findings
  0015, plan 7.0).  Reports, never merges.  Today: `jig/upstream`, the
  mechanical half, run by hand.
- **Part.**  What the nucleus is made of.  The source a component catalogue number
  points to: a piece lifted out of Linux on its own with the hands it held cut
  (a lifted part, in lifted/), or one we wrote (in pool/).  A catalogue number names it;
  the part is the source.  The first serial part is 57 lines.  Called a
  segment until 2026-10-12, when Chris renamed it: "segment" already means x86
  memory segmentation, which the nucleus sets up at boot.  Older findings and
  posts say segment.
- **Lift, cut.**  Lifting is taking a part from Linux with line-numbered
  provenance.  Cutting is removing what it held onto (the tty layer, the
  device model, the allocator).  Both are recorded in the part's header.
- **Nucleus.**  The OS at the centre: the kernel, the drivers lifted from
  Linux, the system calls and the programs, everything that runs on the machine.
  Everything else in AiOs (the jig, referee, builder, tester, historian,
  quartermaster, committee) surrounds the nucleus and builds it.  The word is
  Per Brinch Hansen's: "The Nucleus of a Multiprogramming System" (RC 4000,
  1969), a small centre everything else was built around.  Chosen over "core",
  which collides with processor cores and with CoreOS.  Named by Chris on
  2026-10-12.
  There is one nucleus.  A launch names the applications it runs; the first
  launch of that combination builds it (parts loaded, compiled, booted, the
  applications loaded), and later launches of the same combination reuse what
  was built.  Testing several flavors is the same process run several times,
  never "several nuclei".
- **Loader.**  PART.loader: takes over from the machine, runs one program
  through its system calls, halts when it exits.  The whole nucleus, for now.
- **Landlord, tenant.**  Retired 2026-10-15.  Was a metaphor for the nucleus
  and its programs; Chris retired it because a story word beside a defined one
  sends readers down rabbit holes.  Say nucleus and program.  Older findings
  and posts say landlord.
- **System call.**  How a program asks the nucleus for something: write,
  exit, alloc, read, ms, load, save.  The system call table is the table of
  function pointers the loader hands a program, the only thing a program
  sees of the machine.  Later petitions append entries; nothing ever moves.
  The term Unix has used since 1971.  Called "the door" until 2026-10-23
  (findings 0014); the code still says door.h, as the kernel is still
  called the loader.  "Door" now means only the one way in: a petition.
- **Gate.**  PART.gate: runs every program in ring 3, the processor's least
  privileged level, and gives it one way to the nucleus: the system call, as
  `int 0x80`, with Linux's numbers where Linux has one.  Whatever else a
  program reaches for (the nucleus's memory, the screen's, a port) the
  processor refuses, and the nucleus prints a Guru Meditation line.  Page
  tables do the fencing, not the processor's older way of cutting memory
  (Chris's ruling, 2026-10-18).  Part 18, findings 0021.
- **Marked for Rust.**  A piece in C that tenet 11 says should be Rust, listed
  in findings 0028 with what it does, why it is C and that Rust is the target.
  Not rewritten for that alone: it moves to Rust when it is next touched for
  its own reasons, or when a petition says so.  Kept apart from TECH-DEBT n,
  which marks lines in the source and is checked by a test; marking for Rust
  changes no source.  Named by Chris on 2026-10-29.
- **Tech debt.**  A line that is there on purpose and has to go.  It is marked
  `TECH-DEBT n` in a comment in the source and listed under the same number in
  the findings, with what replaces it and when it goes; a test checks the two
  lists match.  Nothing else uses the mark.  First entry: the system call
  numbers 1000 to 1003 (findings 0021 #5).
- **Extras.**  constitution/extras.toml: what each flavor's kernel exposed
  that none of its petitions asked for, when the referee first held the
  built kernel to rule 11 (decision 0065).  Debt by another name, listed by
  flavor and capability; the referee refuses anything not on it and any line
  that is no longer true, so it only shrinks.
- **Switched off unless asked.**  A part that can do more than a flavor asks
  compiles the rest out: the builder sets GRANT_ for each capability the
  petitions ask for (CAP.fs.write@2 is GRANT_FS_WRITE), the part's entry
  names the capability and a symbol in `off_unless_asked`, and the referee
  checks the symbol is in the image exactly when it was asked for.  Linux's
  #ifdef CONFIG_, with the petition as the configuration.
- **Program.**  A flat binary linked with the loader's program.ld, entered at
  byte 0 with the system call table.  Hello, Hanoi, the rungs.
- **Machine (MACHINE).**  The VM definition: one serial port,
  as little else as the hypervisor allows.  MACHINE.serial-only.
- **Hypervisor.**  QEMU with KVM.  Scaffold, through the door later.
- **Jig.**  Gen zero: the NixOS host that builds and tests but is not the
  system.  Bloat on purpose; every tool on it has a row in design.md's tool
  table with how it leaves.
- **Scaffold.**  A component the jig provides for now (the compiler, the
  hypervisor), marked `scaffold = true`, rebuilt when its pool version is
  admitted.
- **Referee.**  Checks that every petition asks only for what is in the
  catalogue, and that every capability it asks for has a test; then runs the
  tester itself, on the exact code it is approving, and refuses anything that
  does not pass.  It never takes a results file on trust, and it never decides
  how a test works, only whether it went green.  Checks against the rule book
  Chris wrote.  No model, no judgment.  Nothing is cached until it says yes.
  Fails loud, naming a rule and a catalogue number.  Built: AGT.referee@1,
  pool/AGT.referee@1/referee.c, in C, the first agent in the pool (moved out
  of jig/aios on 2026-10-15, a built thing lives in the pool, not the jig;
  rewritten from shell to C the same night, because the referee is permanent
  code): `check` (rules 1, 9, 19, the build gate), `hardware` (rule 12,
  against constitution/instructions.toml), `rules` (2 to 8 on every petition,
  written to tests/referee-results.txt, findings 0017), `lines` (rule 13),
  `artifact` (rule 11 on the kernel as linked, run by the builder) and
  `probes` (rule 22, where test programs boot; both decision 0065).
  `pool/AGT.referee@1/referee` builds it and hands over.  The decision ledger
  is `jig/ledger`, a script, because it reads git history; inside AiOs that
  job is admission.  The builder calls the referee before and after every
  build.  Running the tester is planned.  The referee is permanent, the one agent never stripped: it
  decides what is stripped.  It runs where nobody types: today the jig, which
  runs the ledger on every commit since the last run before any test
  (`jig/ledger --commit`), so a check skipped on a PC is refused there;
  when the jig goes, the referee runs inside AiOs, at admission.  The rule
  names the condition, never the machine.  Decided by Chris, 2026-10-15.
- **Builder.**  Turns a checked flavor into an image, deterministically.
  Decides nothing and validates nothing itself.  Pressing play runs, in this
  order: the referee on the petitions (stop on any failure); the builder;
  the referee on the image (rule 11), which then runs the tester against
  that exact image; the cache, which takes only an image the referee said
  yes to after the build, so a broken builder cannot skip it.  The builder
  works from a queue, a file anyone can read: it takes the next flavor,
  builds everything it can (each part compiled on its own, then linked),
  sends the full list of what is missing to the quartermaster by catalogue
  number, and stops.  It never waits on the answer and never carries on with
  it.  When the committee admits what was missing, the flavor goes back in
  the queue and the whole order runs again from the referee, recompiling
  only what changed (plan 5.6, 5.7).  Nothing in AiOs runs without a defined
  trigger.  Today: `jig/aios build`, started by hand, which calls the referee
  first, and on the linked kernel (rule 11's parts and capabilities, decision
  0065); the referee running the tester on the image is plan 3.7.
- **Decision ledger.**  Every choice an agent makes among options, written
  down: the options, the default, what was chosen, the reason and who chose.
  The referee checks it against what actually changed, so a change with no
  matching decision is a silent decision and a fault.  A short list of
  decision kinds (licence, purpose, petitions, capabilities, dependencies,
  tests, these rules and the words in them) is human-only.  A petition is
  built only when a decision under a key names it (decision 0051).  Built small (part 15): the
  list is constitution/human-only.toml, by path; a decision is one TOML file in
  decisions/ (what was asked, the options, what was chosen, who decided, which
  paths it covers, and who drafted it); `jig/ledger` refuses any change to
  a human-only path that no decision in the same change names, decided by a
  key in constitution/keys/.  It runs before every commit, from the
  pre-commit hook in .githooks/ (once per clone: `git config core.hooksPath
  .githooks`).  An agent may draft a decision; until commits are verified
  against keys (plan 3.5), the proof that a human decided it is Chris's
  signature on the commit that carries it.  Findings 0016 says what it cannot
  see yet.
- **Verb, noun.**  A verb is a doing word; a noun is the name of a thing.
  In AiOs the verbs are the few things an agent may do to the repository
  (lift, add, remove, pin, retire, rerun, record, ask), listed in
  constitution/verbs.toml, changed only by a signed human commit.  The nouns
  are the catalogue numbers, and a noun enters only when a signed petition
  creates the thing it names.  A committee seat or the mechanic speaks to the
  referee only in a numbered list of verb-noun lines (rule 13, `referee
  lines`); a verb or noun the referee does not know is a failure and a
  finding, never a green light.  The verbs and the catalogue are kept apart
  so the door that admits a part never admits a power.  Decided by Chris on
  2026-10-14.
- **Agent.**  Anything in the ring around the nucleus that acts on it: the
  referee, builder, tester, historian, quartermaster, mechanic, committee.
  The older sense of the word, as in mail transfer agent: something that acts
  on someone's behalf.  Some agents use models, some never will (the referee
  and builder).  The nucleus is made of parts; the agents surround it.
  Named by Chris on 2026-10-13.
- **Quartermaster, tester, mechanic, committee, seat, historian, cartographer,
  watchman, gateway.**  The agents and the governing body, defined in
  design.md.  The tester and historian exist today as jig scripts
  (`jig/test`, `jig/mutate`, `jig/upstream`); the rest do not yet.  The
  quartermaster reads source through the shared source reader, the same one
  the historian uses (findings 0015).  It drafts every petition: an
  application's, because no human could; a part's, when the builder sends
  it something missing; and one for every fix the mechanic proposes, which
  it reads as new code through the same source reader.  All code enters
  through the quartermaster, whoever wrote it.  The mechanic never
  shares a prompt with the quartermaster, the director or any builder, and its
  prompt is written to find what theirs would miss; they may share a model
  (decision 0057), and the scorecard says when they did, since the same model
  checking its own work tends to share its blind spots (findings 0027).  When a test fails, the mechanic proposes a fix to the code, never
  to the test (tests are human-only).  It reads the test's red_when, the
  spec it binds and the part's as-built spec; the gap between the spec and
  the as-built is the bug.  If the test disagrees with its spec, the test is
  wrong, and that goes to Chris.  The committee's six seats (security,
  performance, portability, experience, stability, purpose-fit) come from
  the BSD family (OpenBSD, FreeBSD, NetBSD, and Darwin under macOS), each a synthetic personality modelled on the
  people who maintain one, as a tribute; each on its own model; it votes and
  never drafts.  Chris is the committee until it exists (design.md).  Every petition it drafts goes to the
  committee (Chris until the committee exists), including a change to an
  existing one.
- **Director, builder (of the layered mechanic), part plan, job, result note,
  question note, stuck, fold.**  The layered mechanic in agent/ (findings 0027).
  Strategic (Chris and Claude) writes a part plan: the tests that say a part is
  done, the commit it starts from and its limits.  A director, one per part,
  turns it into jobs, one red test each, for builders; a builder here is a fresh
  model session that does one job and is gone, not the kernel's builder, which
  never uses a model.  The program writes a result note from what the tests
  printed on the jig, and the director sees only that.  Stuck is the program's
  call, from the notes: the same failures, or no drop, run after run.  A stuck
  part, one out of tries or time, or a question from the director stops with a
  question note for strategic.  A fold is a conversation cut down to fit the card:
  old answers dropped and said so, or a director's older turns made into one
  standing that names every job and test.  Named by Chris on 2026-10-27 and
  2026-10-28.
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
- When there's a choice between options that would each do the job, the
  default is the one with the history: the 8254 over a newer clock, Minix
  over a newer filesystem.  Say which one has the history when offering the
  choice.  Chris still decides.

## Definition of done

A component is done, and a post about it may be written, only when all of
these are true.  `sh jig/donecheck` checks the ones a script can check and
names the component and the rule that failed.

1. It has a catalogue entry, and the entry names its source directory.
2. It has an as-built spec: what it does, every port, register and
   instruction it touches, every byte it writes, its size.  Hand-written until
   the generator exists (plan 7.1), and saying so.
3. It has at least one spec clause, written by a human, one sentence, precise
   enough to fail.
4. Every spec clause has at least one test, and every test names what turns
   it red and a time limit.
5. Every test has been run by `sh jig/test` and passes in tests/results.txt,
   and `sh jig/mutate` has shown it can go red (or findings say why it cannot
   yet).  A test with no runner says so, in the catalogue and in the post's
   addendum.  On the jig, after a push, from Chris's shell (the jig's address comes
   from `(Get-VMNetworkAdapter -VMName jig).IPAddresses`; it changes when the
   host restarts, and the jig does not start on its own):
   `ssh chris@ADDRESS "cd aios && git checkout -- tests/results.txt
   tests/mutations-results.txt tests/referee-results.txt && git pull -q && sh
   jig/test && sh jig/mutate; sh pool/AGT.referee@1/referee rules > /dev/null"`, then `scp` the
   three files back.  The checkout throws away the last run's copies,
   which only ever come from a run.
6. If any line came from upstream, it lives in lifted/ with an origin record
   that `jig/upstream verify` passes.
7. Every earlier program on the ladder still runs under it, unchanged.
8. `sh jig/ledger` passes: every human-only change in the work is named
   by a decision in decisions/, drafted for Chris to decide.
9. The post's HTML file and its image exist and `jig/postcheck` passes,
   including "As built" and "Addendum: the tests".
10. Every commit for it is in the ledger with its post.

## The series

The series is for learning together, not showing off.  A reader should finish
each post able to follow what was done at the same level.  `sh jig/postcheck`
checks every post against the rules below and fails naming the file and the
rule; run it before asking Chris to commit anything in posts/.

**The underlying message.**  The OS is the demo.  The point is how we get to
safe, automated, agentic development.  Every post teaches one piece of that in
plain words: what the piece is, what it checks, and what it stops the AI from
getting away with.  The article's closing section names the piece this rung
added (part 9: time is measured, never assumed, and sabotage proves it).
Don't pitch the whole idea until a rung goes from petition to signed commit
with no steering.  Until then, each post adds one word to the vocabulary.

**When.**  Every time an item or sub-item in plan/brief.md moves to [done],
Claude drafts a post for it.  The post is also the body of the commit that
moves the item to done.  One post every Tuesday and Thursday at 7:30 AM
Pacific, from 2026-11-03 (decision 0109; every second day from part 7 to part
12, decision 0010): the date is the day it is meant to go out, or, once it has,
the day it did.  The folder runs ahead of LinkedIn on purpose.  When the
schedule moves, the files are renamed to their new dates and README.md's table
and the ledger's post keys follow.

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

**The post (section `post`).**  The story told short, 150 to 220 words, in
Chris's voice, two spaces after periods (decision 0010; part 1 reached
thousands of people and the one-line teasers after it reached a few hundred).
1. The first line: `AiOs, part N (Kernel code: K lines).` then a one-sentence
   pull quote from the article in double quotes, then `...`.
2. Three or four short paragraphs telling the actual story from the article:
   what the rung was, what got built, what broke or surprised him, what the
   fix or decision was.  Real specifics: line counts, the bug, the test that
   went red, the history.  Pick the one or two moments that make it a story;
   don't summarize the article.  Someone who never clicks still gets it.
3. One closing sentence connecting it to how he thinks about building
   software or running product.  No preaching.
4. `#AiOs` on its own line (`#AiOS` for parts 1 to 12, matching their name).
No bullet points, and never open with "In this part" or "Today I".  Links
go after the closing sentence and before the hashtag, each on its own line
with a few words saying what it is (decision 0085).  From part 13 every post
ends with the same three links, in this order: the part's findings file, the
one commit worth opening, and the whole repo, each `What it is: link`
(decisions 0109, 0110; the petition where a part has no findings; LinkedIn rewrites them to
its own short links, and that is fine).  The links are not counted in the
150 to 220 words.  jig/postcheck checks all of it from part 7.  K is the lines of code taken from Linux (lifted/, the .c files, no blanks, no
comments, as the as-built specs record them) on the day the post goes out:
57 from part 1, 69 from part 6.  LinkedIn attaches the article to it.

**The article (section `article`), top to bottom.**
1. `<p><em>The story behind AiOs, part N.</em></p>`
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
whether to edit the live one to match.  Parts 1 to 6 are live.  Their posts in
the story format are in posts/drafts/linkedin-posts.md; when Chris edits a live
post to match, its file takes the new body in the same change.
