# AiOS: A Purpose-Driven, Self-Building OS

Pronounced AY-ohss, /ˈeɪ.oʊs/.  Linux, re-admitted one justified line at a time.

Oct 9, 2026 · Chris McLuckie

## Premise

We are not writing an operating system.  We are writing the system that decides what an operating system should contain, builds exactly that, proves it works, and keeps it working without waiting on anyone.

Every clean-slate OS of the last 30 years lost to Linux for the same reason: the kernel was never the hard part.  The moat is decades of drivers, bug fixes and ecosystem, and no design elegance beats it.  Fuchsia proved that even Google's money and engineers cannot buy their way past it; after a decade it runs hidden on smart displays while Google's real bet went back to Android.

The moat only matters if humans have to do the grunt work.  If agents can read source, write specs, write tests, and maintain drivers, the moat evaporates and the real question becomes what the system is *for*.  That question is one no OS today can answer, and it is the whole project.

"Portable" here means flexible: the same system grows differently on a drone, a music rig and a database server, from the same pool of parts, because each stated a different purpose.

## Tenets

These are the architects' tenets: what AiOs is for, and what no agent, app or committee vote can override.  The referee never reads them.  It reads the constitution's rules, each of which traces to one of these.  (Until 2026-10-15 this line said the referee enforces them; it never could.  A tenet is a belief; a rule is a check.)

1. **No purpose, no operating system.**  Nothing exists in the system unless it traces to a stated purpose.  A machine can have several purposes; each is written down.
2. **One door.**  The only way anything enters the system is an approved petition: apps, libraries, drivers, services, agents, tests, telemetry, the core itself.
3. **Source only.**  Nothing enters as pre-compiled code.  Everything is built from reviewed source, reproducibly, down to a bootstrappable toolchain.
4. **Reviewed before entry.**  Every component is reviewed against this machine's purpose and invariants before it is admitted.  Review is cumulative and collective, never a claim of "proven safe."
5. **No steady-state dependency on anyone else.**  Once source is inside, we own it.  Upstream is a supplier of suggestions that come through the same door, never an authority we wait on for a fix.
6. **Mechanism and policy stay apart.**  The OS in the centre provides levers.  It never decides when to pull them.
7. **Intelligence and authority never share a body.**  Agents propose; the referee decides; the referee has no intelligence to be fooled.
8. **A test is written to fail.**  A failed test is the most valuable signal in the system.  A sensor or test that cannot go red is decoration.
9. **Nothing is self-reported.**  No agent's certainty counts.  Confidence is earned by track record and judged by a committee with competing priorities.
10. **Everything expires.**  Petitions carry a sunset.  Unused grants are revoked and the component stripped.

## The base

We build on Linux source and Nix, and we write neither.

**Linux source is the parts pool.**  Roughly two-thirds of its 30 million lines are drivers and another large share is architecture support; the core is a few million lines.  A given purpose needs a sliver.  The problem is not the volume, it is that selection is woven through the source as `#ifdef` directives, so the file you review is a superposition of every possible build and nobody can honestly say they have audited "the kernel."  The answer is a refactor, not a rewrite: pull the conditionals out of the files and into the structure.  One component, one directory, one petition, no `#ifdef` inside.  Compose, don't configure.  The catalogue tells us which handful of components actually need that surgery; drivers, filesystems and protocols already come apart cleanly.

**Linux is read, never booted.**  A nucleus shares one machine between programs that do not trust each other.  A purpose with one program has nothing to separate, so it gets no nucleus; it gets a loader that takes over from the machine, runs the one program and halts, plus whatever drivers the purpose names, each lifted from Linux as its own part with the hands it held cut and recorded.  Linux wrote no spec and no test for most of what we lift, so every part arrives naked and we write both at the door.  Parts talk only through declared interfaces so a later petition stacks on an earlier one without removing anything; a nucleus appears only when a petition asks for two things that must share.  Findings 0001 records why the first build plan got this backwards.

**Nix is the builder.**  It already turns a declaration into exactly that set of components, reproducibly, with rollback for free.  We write the translator from approved petitions to a Nix configuration.

**A bootstrappable toolchain is the floor.**  Source-only means answering "who compiled your compiler?"  Guix already does this from a few hundred bytes of auditable machine code upward; we adopt that chain rather than inventing one.

**Distributions go away.**  A distribution is a pre-baked guess about what you will need.  We replace the guess with the purpose: a distribution of one, built per machine from the same pool.  Docker does not escape this; every image still starts from someone's base-image guess, on a host that is a full general-purpose OS.

Not written from scratch: the kernel, drivers, the compiler, anything a proven recipe already covers.  Those are brought in through the door, reviewed, and owned.

## The three missing pieces

The project's real deliverables are the three things Linux does not have: a test suite that defines correct behaviour, as-built specs derived from code, and a catalogue of those specs.  The constitution stands on these; without them a petition has nothing to name and the committee nothing to judge against.

**Tests are the executable half of the constitution.**  Purpose says what the machine is for; the tests say what "working as designed" means, precisely enough to fail.  When code can be regenerated on demand, code stops being the asset and the tests become the institutional memory.  Linux ships kselftest and KUnit, and the Linux Test Project has decades of behavioural tests; they come in through the door like everything else, so every test is a reviewed claim we own.  A green suite is only trusted once it has been seen to fail for the right reason; mutation testing (break the code on purpose, see if the tests notice) is the cheap proof.

**Two specs, not one.**  The as-built spec says what the code actually does.  Derived from the code, it is true by construction even when the code is wrong; a spec that accurately describes a bug is an honest spec.  The as-intended spec says what the code should do, and it cannot come from the code at all: it comes from the purpose, the constitution, the customer contract, the original design.  Collapsing the two is how a bug gets promoted to a requirement.  Diffing them yields three outcomes:

| As-built vs as-intended | Meaning | Goes to |
| --- | --- | --- |
| Match | Fine | Nobody |
| As-built wrong | Defect | Mechanic |
| As-intended missing or self-contradictory | Constitutional gap | Humans who own the constitution |

A spec is only trusted while a test enforces it.  Code drifts and documentation never notices; that is how every wiki dies.  So the spec, its test and its petition are three views of the same fact.

**The catalogue is the menu.**  You cannot petition for something you cannot name.  Kconfig help text and the Documentation tree are a partial, stale, developer-facing version; nobody can hand you the list of components, what each does, what it depends on and what it exposes.  The quartermaster's first deliverable is that catalogue, derived from code rather than docs, because docs lie and code can't.

## Constitution and petitions

The constitution is a git repo: the machine's purposes, the invariants no vote can amend, the approved petitions, and the tests that make it executable.  It is always a live, accurate description of what the machine does and why, which no OS today can give you.

**The first petition is the purpose itself.**  Every later component traces back to it.  Nothing exists that cannot answer "who asked for you, and why?"

**A petition is structured data, not prose.**  It is a bill of materials: catalogue numbers that resolve to entries defined exactly once elsewhere.  Five catalogue tables:

- `CAP.*` capabilities: the finite vocabulary of what anything may touch (`CAP.FS.READ`, `CAP.TTY.RAW`, `CAP.NET.*`).  Small, stable, written once.
- `PART.*@v`, `MACHINE.*@v`, `TOOL.*@v` components: the catalogue.  Each entry says what it is, what it depends on, which capabilities it exposes.
- `SPEC.*` as-intended clauses: single testable sentences.
- `TST.*` tests: each points at the spec clause it can turn red.
- `SNS.*` sensors: question, threshold, consumer.

A petition carries: identity, the purpose catalogue number it traces to, the capability catalogue numbers it requests with a spec clause as the reason for each, the component catalogue numbers it draws, the test and sensor catalogue numbers that bind it, expiry, and the review record.  No prose; the prose lives in the spec and catalogue entries.  The referee enforces the graph, not the English: a capability with no justifying spec clause is rejected by rule; a spec clause with no test is a gap; a component the catalogue does not list cannot be requested; every reference must resolve; every petition expires.

Catalogue numbers are versioned, and a petition points at a catalogue number at a version.  Redefining `PART.editor` from a C program to a Rust one publishes `PART.editor@2`; petitions on `@1` keep building what they built, and the committee decides which move.  The petition is the stable contract; the code behind it is the implementation.

**The installer is a petition.**  Today's install is a one-way trust fall: the OS ships with everything, the app takes what it finds, nobody writes down why.  Here an app arrives with a manifest, the committee judges it, and the system grows only the parts approved.  Uninstall it and anything nothing else depends on is stripped back out.  Legacy apps won't ship petitions, so the system drafts them: run the app sandboxed, watch what it touches, propose the petition for review.  Drivers get the same treatment: observe, specify, regenerate.  This is the escape route from the ecosystem graveyard that took Plan 9 and BeOS.

**Rules for judging petitions.**

- The request must fit the machine's purpose.  A game asking to install on a database server is rejected at the door, however clean its manifest.
- Requests are judged against what the app claims to be.  A flashlight asking for contacts is a tell.
- The user states a need; the committee decides the means.  Ask for FTP and you may get SFTP.  Users are never the approval step: UAC and Android prompts proved people click yes to make the box go away.
- Use it or lose it.  A granted capability never exercised is revoked.  An undeclared action is an alarm, not a request.
- Amendments expire.  A port opened for a week-long migration closes itself unless renewed with a reason.

**Firewalls dissolve into the design.**  A firewall closes ports that should never have been built.  Here nothing listens unless it was petitioned for, and the connections that do exist are permissions in the constitution, enforced from inside by the referee: this component may talk to that address, for this reason, and nothing else.

**Purposes nest.**  "Browse the web" is itself an operating system running strangers' code; the constitution has to go a level deeper (which sites get the camera, which may download).  The deeper the purpose is written, the less the system has to guess.

**The pool and its door.**  The pool is the set of source that has passed entry review: read, catalogued, given an as-built spec, with tests and a reproducible build.  Being in a Git repo is not being in the pool; upstream Linux is outside, our fork is inside, and a component is in the pool only once it carries a signed admission commit.  Pool admission is its own petition type, done once per component version, judged against the invariants rather than any one machine's purpose.  Machine admission is the cheap, frequent door: a petition selects from the pool for one purpose.  Components nothing has petitioned for in a year get a retirement proposal.  The pool lives in our own bare Git repository; GitHub is a place the historian reads upstream, never a thing the system depends on.

## Architecture

Six layers, one door.  From the top: humans (signing keys, purposes, constitution) → committee (competing priorities, slow policy) → guardians (propose, never install) → the OS in the centre (mechanism only, live copy and shadow copy) → referee (no model, no judgment; every part traces to a live petition) → silicon root of trust (a few kilobytes that cannot change).

The clever parts have no authority and the part with authority has no cleverness: guardians and committee can only propose and rule, the referee alone changes the machine, and the referee is trusted only because silicon checks it at boot and your keys sit above that.

## The agents

Agents are components too.  Each enters by petition with a stated purpose and a bounded set of permissions, and the referee holds it to them exactly as it holds an app.  Build order is tester first, quartermaster second, mechanic third: nothing may change the system until something can prove what changed.

| Order | Agent | Purpose | May | May not |
| --- | --- | --- | --- | --- |
| 1 | Tester | Manufacture failures before the world does | Run anything in the shadow; fuzz; mutate; write tests | Touch the live copy |
| 2 | Quartermaster | Decide what enters the ecosystem | Read source; build the dependency map and catalogue; classify needed / not / tangled; draft petitions | Change source |
| 3 | Mechanic | Fix and refactor | Propose patches and untangling refactors, handed to the quartermaster as new code | Install anything; skip the quartermaster |
| 4 | Gateway | The only component that talks to the outside | Reach the repo and the model provider, signature-checked | Reach anything else; exist after seal |
| 5 | Cartographer | Map the hardware on arrival | Probe CPU, memory, devices | Trial-and-error on unknown silicon (bricks and cooks things; needs datasheets and a sacrificial board) |
| 6 | Builder | Turn approved petitions into the system | Run Nix, deterministically | Decide anything |
| 7 | Purpose agent | Weight the committee's priorities for this machine | Translate purposes into how much security, speed, portability and stability each get | Add a purpose on its own |
| 8 | Historian | Make machine 1,000 benefit from the first 999 | Record what worked where; push to the repo; pull upstream fixes through the door | Bypass review |
| 9 | Watchman | Survive the seal | Detect anomalies and trigger a molt | Diagnose or fix (it is not a model) |

**The committee** is the governing body: security, performance, portability, experience, stability and purpose-fit, each with its own model so they do not share blind spots, each biased on purpose so no single priority quietly wins.  The seats come from the BSD family, where each flavour built a whole operating system around one priority: OpenBSD security, FreeBSD performance, NetBSD portability, and Darwin, the BSD under macOS, the experience of the person using the machine.  Here those priorities share one table, joined by stability and purpose-fit, which have no BSD twin.  Each seat is a synthetic personality modelled on the culture of the people behind those systems, a tribute to them; the experience seat takes after Apple's founders.  A seat is never named after, never claims to be, and never speaks for any real person.  Negotiation is slow and happens at the policy level; fast decisions follow whatever was last agreed.  Quorum rules are fixed in advance so deadlock is not a new way to fail.

**The referee** is deliberately dumb.  No model, no judgment.  It checks that every component traces to an approved, unexpired petition; that nothing listens or connects without one; that the build is signed and reproducible; that the seed can regrow anything about to be stripped.  It fails loud.

**Humans** own the constitution.  Developers tune `architecture.md` and `CLAUDE.md` rather than fixing bugs; QA asks how a bug ever got through and closes the class; triage decides whether a problem fits the constitution, conflicts with it, or requires amending it.  The AI repairs the machine; it never redefines what a healthy machine looks like.

## Telemetry with a purpose

Telemetry is the tester running after the fact.  Tests prove a claim before shipping; telemetry proves the same claim in production, continuously, against real data.  Same rule: a sensor is only valuable if it can go red.

Telemetry enters by petition, and a telemetry petition is a question: what is measured, which purpose it serves, what "good enough" is as a number, who consumes it, and when it expires.  No consumer, no sensor.  The number must be in the petition.  Nothing acts on telemetry directly: a sensor going red is a finding; the guardians diagnose it; a repair petition goes to the committee.  Measure the seams between components, not everything.

## Lifecycle

The system is a metamorphosis: it grows while connected and full of agents, then strips itself down to a lean, sealed thing that does one job, and regrows only when something warrants it.

1. **Grow.**  Cartographer maps the hardware.  The purpose is the first petition.  Quartermaster pulls proven recipes for known hardware from the pool and involves the model only for what is genuinely new.  Builder produces a live copy and a shadow copy.
2. **Prove.**  Tester attacks the shadow.  Referee swaps live and shadow only if the shadow passes; swaps back on misbehaviour.  Rollback is free because the system state is declarative and versioned.
3. **Strip.**  Anything without a live petition is removed, including the gateway and the thinking agents.  The referee's job here is "remove it, and confirm we can still get it back."  Two things survive: a sealed seed the referee can never delete, and a molting rule decided while the system was still smart.
4. **Run sealed.**  Only the watchman and the purposeful sensors remain.  No model, no outbound connection, almost no attack surface.
5. **Molt.**  A signed update notice, a failure the sealed system cannot handle, a sensor gone red, or an explicit instruction wakes it.  It reconnects, regrows, proves, strips, and seals again.

## Security consequences

- **Nothing to sweet-talk.**  A sealed machine has no agent left to prompt-inject and no outbound connection to hijack.
- **Less to chew on.**  Purpose-built code is a fraction of a general-purpose install.
- **No monoculture.**  Every machine builds itself, so no two need be identical.  Cracking one machine cracks one machine.
- **The gap closes itself.**  A red-team tester attacking its own code, a mechanic patching, a shadow proving, and a historian pushing the fix to every other machine is herd immunity for software.
- **The soft spots, named.**  The molt is the attack window; signature checks are the defence.  The gateway is a single point of failure, hence the signed offline seed.  Sealed machines still process input; the seal shrinks the surface, it does not remove it.

## The same model applied to the dev process

| OS | Dev process |
| --- | --- |
| Anomaly / red sensor | Bug report |
| Watchman | Intake agent: do I have enough information?  If not, ask the source |
| Committee | Triage, now constitutional review: fits, conflicts, or needs an amendment |
| Mechanic | Fix agent |
| Shadow copy | Test container |
| Tester | QA agent |
| Historian | QA closing the class: how did this get through, what rule stops it recurring |
| Constitution | `architecture.md`, `CLAUDE.md`, the as-intended spec |

Two gates, kept separate.  "Do I have enough information?" is a question each agent answers for itself.  "Are we sure this should change?" is answered only by the committee.  Certainty is never self-reported.  No agent jumps to patching code: first, is it actually broken, or working as designed?

## Tool set and reduction plan

Every tool on the jig is bloat we are choosing on purpose, so every tool enters with a plan for how it leaves.  Nothing is exempt, including the jig itself.

| Tool | Why it is here | What we use of it | How it gets reduced |
| --- | --- | --- | --- |
| Git, bare repo on the Pi over SSH | The constitution, the pool and every petition live here; signed commits are the admission record | init, commit, tag, push, fetch, verify-commit; SSH-key signing, no GPG | Stays as plumbing only.  No forge, no web UI, no hooks beyond signature checks |
| SSH | Reach the Pi repo; sign commits; reach the jig from the Windows shell | client and signing on the Windows box; sshd on the jig, keys only | Server side on the Pi replaced with something smaller once in the pool; the jig's sshd and client go away with the jig |
| Nix | The builder | `nix build` and a small set of expressions; no channels, no Home Manager, no services | Shrink to the exact subset the translator emits.  Nix is the first implementation of the builder, not the definition |
| NixOS minimal (the jig) | Gen zero: turns the crank | one flake pinning nixpkgs and one configuration of roughly twenty lines, in jig/ | Absorbed: the quartermaster reads its configuration and drafts petitions for it; retired when gen two rebuilds itself bit-identical |
| Hyper-V (the Windows box) | The existing box runs Windows 11; the jig is a Gen 2 VM on it, Secure Boot off, nested virtualization on | New-VM and three Set-VM calls, in jig/hyperv.ps1 | Gone when the jig moves to bare metal or is retired; nothing inside the constitution knows it exists |
| KVM + QEMU | Boots the system under its own kernel so the no-NIC, nothing-listening checks are physically true | the microvm machine type with PIC, PIT, RTC and option ROMs off; one ISA serial on stdio; -kernel and -initrd; no network, disk, display or monitor.  Only inside jig/test: the serial register trace events and a paused monitor on a pipe, to observe the machine, never to change it | Admitted through the door as a recipe (TOOL.qemu, scaffold until then).  The VM definition is itself a petition: MACHINE.serial-only |
| C compiler + musl | The first pool components | a C compiler and one libc; nm from the same binutils as the linker, for the referee to list what a built kernel holds (rule 11) | The jig's compiler is scaffolding.  The pool's own chain bootstraps from a few hundred bytes upward; musl is the only libc in the pool |
| strace | Prove a component touches nothing it did not petition for | file, directory and socket syscalls | Replaced by the kernel's own tracing once in the pool; leaves with the jig |
| mkfs.minix, fsck.minix (util-linux) | Make the empty Minix disk the sort machine boots with, and check it afterwards with a tool nobody here wrote | `mkfs.minix -1 -n 14` once per fresh disk; `fsck.minix -fl` read-only, never repairing | Leave with the jig.  fsck stays the outside opinion until AiOS can check its own disk, and even then it is a second opinion worth keeping |
| Mutation tool | Prove the tests can fail | source mutation, count caught mutants | A shell script with a fixed list of mutations, until something smaller and source-only earns admission |
| cscope | The source reader's mechanical half: where a name is defined, who calls it, what it calls (findings 0015, plan 7.0) | `cscope -b -k` on a copy of the pinned tree, then the line questions `-L -1`, `-2` and `-3` | Leaves with the jig.  The reader moves into the pool as a C component and either takes cscope's job with it or admits cscope as a scaffold by its own petition |
| Source reader (jig/reader) | One way for every agent to read Linux, pinned to one commit, refusing anything that is not there: the quartermaster's eyes and the historian's | `git show`, `git ls-tree` and `git archive` against a clone of Linux, and cscope for names | A shell placeholder for a pool component with its own petition; rewritten in C, as the referee was, once the quartermaster uses it daily |
| Model call (jig/model) | One place that calls a model: costed, capped and recorded before it counts, the key kept in a file outside the repo; the first user of the gateway rule (only one component reaches the provider) | `curl` with its own AWS signing against the Bedrock Converse API; the reply's tokens and text picked out with sed | A shell placeholder for the gateway; replaced by a pool component with its own petition, and the second model (the 3090) goes behind the same interface |
| JSON tool (jq) | The model's replies and requests are JSON, and the exam runner has to read and write them without losing a quote or a newline in a lifted file | `jq` to build each request and pick the answer, tool calls and token counts out of each reply | Replaced when the gateway is written in C and reads the replies itself; until then the jig has no other way to read them safely |
| Model clients (Claude Code, Bedrock, Ollama) | The committee seats and the mechanic; different models per seat | API calls only | Collapse into the gateway component.  Stripped at seal.  Ollama is a local seat so one voice never depends on the network |
| Rust toolchain (rustup, cargo, rustc), in WSL, not on the jig | The layered agent (agent/, findings 0027) is a Rust program: JSON, HTTP and strings handling untrusted model output, where C bites and Rust does not | `cargo build` and `cargo test` on agent/ with `--locked`, every crate pinned by the committed lockfile (reqwest for HTTP to Ollama and Bedrock, serde for JSON); whether the crates' source is also vendored into the repo is Chris's call (findings 0027 item 10); Bedrock's signing written in agent/ to match what jig/model's curl does | Leaves with the gateway: when one pool component alone reaches a model, the agent's model calls move behind it.  The toolchain leaves WSL with `rustup self uninstall`, and the vendored crates go with agent/ |
| Shell (the jig's) | Scripts for the referee, tester and translator until they are real components | POSIX sh only | Each script is a placeholder for a pool component with its own petition.  The referee is rewritten first |
| Python | Only if a script outgrows sh | nothing yet | Not admitted.  Default answer is no |

Rule of thumb: nothing gets installed on the jig until it has a row here, and the row is not complete until the last column has something in it.

## Build plan

Start with the constitution, not the code; everything else derives from it.

**Petition 0000 is hello world.**  Purpose: write one line to the serial console and halt.  Two capabilities, console write and exit, plus the serial device the machine presents.  Three bricks, separate from the start: a loader that takes over from the machine, sets up the processor, runs one program and halts the machine when it exits; a serial part lifted from Linux, the lines that put a byte on a 16550 and nothing holding its hand; and hello, which makes a system call for each capability and never touches hardware.  No kernel, no libc, no disk, no filesystem, no init.  Tests: the bytes on the serial port are exactly hello's bytes with nothing before or after, and the machine has no device no capability names.  The failure cascade still admits the toolchain and bootstrap chain on the simplest possible program.  0000's bloat figure, lines in the image no petition justifies, should be near zero by construction; where it is not, the line is a finding.

**After 0000, a ladder.**  Each petition is a program every CS student wrote once, chosen because it needs exactly one thing from the machine the last one did not: Towers of Hanoi (nothing new, proving a second program changes nothing), Conway's Game of Life (memory), a reverse-Polish calculator (input), a stopwatch (time passing), save the output (a disk and a filesystem), two programs at once (the first thing that must be shared, and the kernel appears), then the Notepad-class console editor, written in C with Rust later as `PART.editor@2`.  Each rung adds one system call and never moves an old one; every earlier program keeps running under every later loader.

**Failure builds the distribution.**  The build loop is the admission process: try to build, fail on something absent, run it through the door (find, review, spec, test, admit), retry.  Failures name catalogue numbers, never raw tool output.  The quartermaster admits the smallest reviewable unit the failure asked for, not the package upstream ships it in.

**Environment.**  The thing being governed runs in a VM, never a container.  The hypervisor is KVM and QEMU, brought in as a proven recipe.  Each VM definition is itself a petition (one serial port, no NIC: absent, not firewalled).  Day one is two QEMU processes, live and shadow; the guardians are scripts and model calls on the host and need no isolation until they handle untrusted input.

The host is a jig, outside the constitution: NixOS minimal on the existing box.  Three generations: gen zero is the jig; gen one is hello world and then the editor, the first things living entirely by petition; gen two is a machine whose purpose is "build other instances of me."  When gen two rebuilds itself from source bit-identical to what gen zero made, the jig is retired or absorbed.  That self-rebuild is also the trusting-trust check.

Order of work: constitution and petition schema → tester → builder and referee → quartermaster → committee → the loop → historian and first bare metal.

## Open questions and honest risks

- **Defining "working as designed" is the hardest unsolved problem.**  Every "can't tell if this is broken or by design" is a finding that the constitution has a gap.
- **Review at scale is a claim about process, not safety.**  The honest bar is "reviewed against this purpose and these invariants," never "proven safe."
- **The oracle problem.**  A model that patches a bug can introduce a subtler one.  Everything rests on the tester being independent and adversarial.
- **Gridlock.**  Competing agents can deadlock.  Quorum rules are the mitigation; untested under load.
- **Hardware exploration.**  The cartographer needs datasheets and a sacrificial board, not curiosity.
- **Kernel maintainers will not help.**  We fork, the agents refactor, the historian keeps the fork in sync through the door.
- **The molt is the attack window.**  Signature checks and the bootstrappable toolchain must exist before the first real molt.
- **Almost nothing here is new.**  What is new is the organizing principle: purpose first, one door, a constitution everything traces to, guardians with competing priorities, and grow / strip / seal / molt.
