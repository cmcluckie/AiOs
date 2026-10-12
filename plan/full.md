# AiOS project plan (full)

The detailed plan behind plan/brief.md.  Same numbering.  Each item carries:
why (the tenet or design.md section it serves), what is produced, done-when
(the check that can go red), depends-on, and open questions.  Status words
match the brief: [done], [in progress], [coming up].  Update both files
together; the brief is the one the owner reads.

Order of work from design.md: constitution and petition schema, then tester,
then builder and referee, then quartermaster, then committee, then the loop,
then historian and first bare metal.  Items 2 (jig) and 6 (pool admission)
are interleaved as the build fails on them.

Standing rules for every item: no code until a petition names it; no tool on
the jig without a tool-table row; every commit signed; every test seen red
before it is trusted; failures name codes, never raw output.

---

## 1. Constitution [in progress]

Why: tenets 1, 2, 7.  The repo is the thing everything traces to; nothing
else can start until petitions have a schema to be written against.

### 1.1 Repo rules, tenets, purpose 0000 [done]
- Produced: CLAUDE.md, constitution/tenets.md (verbatim from design.md),
  constitution/purposes/0000.md, design.md.  Commit 8167fbc "Article One".
- Done-when: tenets diff clean against design.md (checked); commit signed
  and verifies (checked).

### 1.2 Petition schema [done]
- Produced: schema/petition.md, READMEs for index, specs, tests, sensors and
  pool, constitution/keys/README.md and chris.pub.  Commit d9c11f0.
- Note: schema has six tables, not five; PUR was added (findings 0000 #1).

### 1.3 Petition 0000 [done]
- Produced: petitions/0000.toml, index/cap/tty.write.toml and
  proc.exit.toml, specs/hello.1.toml, tests/hello.exact-bytes.toml,
  sensors/hello.boot-to-output.toml.  Commit 3d691ac.
- Deviations chosen: one test and one clause (brief and design say one);
  no comments in the petition; numeric sensor threshold.

### 1.4 Findings 0000 [done]
- Produced: findings/0000.md, thirteen gaps.  Commit 007cc88.

### 1.5 Entry file formats [coming up]
- Why: findings #5.  The schema defines the petition but not the entries it
  references, so "defined exactly once" is undefined.
- Produce: one section per table in schema/petition.md (or schema/entries.md)
  giving required and optional fields, types, and which fields are codes.
  Fields already in use, to be ratified or replaced: CAP touches, mechanism;
  SPEC component, clause; TST binds, red_when, runner; SNS proves, question,
  threshold, unit, red, consumer.  CMP has no entry yet and needs: source
  (pool path), depends (CMP list), exposes (CAP list), requires (CAP list),
  as_built (spec text or path), admission (signature, signer, date,
  reproducibility record).
- Done-when: a validator script (sh, per tool table) rejects each existing
  entry file when one required field is removed.
- Depends on: nothing.  Open: whether the as-built spec is a field or a file.

### 1.6 Review signing scheme [coming up]
- Why: findings #4.  Signatures inside the signed file are circular.
- Options: (a) sign a canonical form of the petition with the review block
  stripped; (b) detached signatures in petitions/0000.sig.seat; (c) each
  review is its own signed commit touching only the review block.
  Lean: (c), because git already verifies commit signatures and the referee
  gets rule 10 for free from git verify-commit.
- Done-when: referee rule 10 passes on a petition with five signed reviews
  and fails when any one review commit is unsigned or by a key not in
  constitution/keys.

### 1.7 Schema version 2 [coming up]
- Resolve findings #2, #3, #6, #7, #9, #10, #12, #13.  Candidates:
  - #2 negative requirements: a forbids list of CAP codes, or a CMP field
    depends = [] meaning "nothing", enforced by rule 6.
  - #3 unadmitted components: a petition status pending-admission that
    rule 1 permits for codes listed with admitted = false, and that the
    builder refuses.
  - #6 scope grammar: per-CAP scope_syntax in the CAP entry.
  - #7 clauses on dependencies: allow SPEC.component to be any CMP in the
    petition; rule 2 becomes "belongs to a component in this petition".
  - #9 test budget: TST field budget_ms.
  - #10 machine: petition field machine = "MCH.id@v", a seventh table or a
    folder under constitution/.
  - #12 non-code references: a KEY table for signers and seats.
  - #13 exercised grants: a per-CAP usage sensor the referee emits
    automatically, exempt from the one-sensor rule because it is the
    referee's, not the petition's.
- Done-when: petitions/0000.toml rewritten against v2 with zero findings
  left unaddressed or each one explicitly deferred in findings/0001.md.
- Note: #8 (one test, two causes) and #11 (scaffolding) may be accepted as
  design facts rather than fixed; record the decision.

## 2. The jig [in progress]

Why: design.md "Environment" and "Tool set and reduction plan".  The jig is
outside the constitution and is bloat on purpose; every tool enters with its
exit written down.

### 2.1 NixOS minimal [in progress]
- Produce: one configuration.nix of about twenty lines on the existing box.
  The dev machine is Windows 11.  Decided: the jig is a Gen 2 Hyper-V VM on
  it, Secure Boot off, nested virtualization on, not WSL2.  Written in jig/
  before the VM existed, with disk labels so the hardware file is true as
  written.
- Done-when: the configuration is committed under jig/ and a rebuild from it
  is bit-identical to the running system.

### 2.2 Bare repo on the Pi [in progress]
- Produce: bare git repo over SSH on the Pi; a pre-receive hook that runs
  git verify-commit with the allowed signers from constitution/keys and
  rejects anything unsigned.  No forge, no web UI.
- Done-when: an unsigned push is rejected; a signed push lands.

### 2.3 Tool table complete [in progress]
- For each of: git, ssh, nix, nixos, kvm and qemu, c compiler and musl,
  strace, mutation script, model clients, sh.  The row is not complete until
  "how it gets reduced" is filled.  Python: default no.
- Done-when: a script lists installed packages on the jig and every one maps
  to a row.

### 2.4 VM definition as a petition [in progress]
- Produce: a QEMU invocation with -kernel, one serial, -nic none, no
  display, -no-reboot; committed as a petitioned component once the machine
  table exists (1.7 #10).  Done as CMP.machine.serial-only@1 on the microvm
  machine type, with CMP.hypervisor.qemu@1 as scaffold.
- Done-when: strace of the QEMU process shows no socket syscalls; the VM has
  no NIC device in its device tree.

## 3. Referee [in progress]

Why: tenet 7.  Deliberately dumb.  Rewritten first among the sh scripts.

### 3.1 Resolver [in progress]
- Input: a petition file.  Output: for each code, the file it resolves to at
  that version, or the code that failed.
- 2026-10-11: `jig/aios check <flavor>` does this for every petition in a
  flavor, plus rule 9 (expiry) and the one-program rule.  Shell; replaced
  when the referee is a component.
- Done-when: deleting any one entry file turns the resolver red naming
  exactly that code.

### 3.2 Relation checker [coming up]
- Rules 1 to 10 from schema/petition.md.  Report format: rule number and
  code per failure, nothing else.
- Done-when: for each rule, a fixture petition exists that violates only
  that rule and the checker names it; disabling any single rule check makes
  its fixture pass (the test goes red).

### 3.3 Expiry, status, quorum [coming up]
- Done-when: a petition with expires in the past is refused; a petition with
  status approved and one missing seat is refused.

### 3.4 Artifact check (rule 11) [coming up]
- Compare the built image's component list and the VM device list to the
  petition.  Needs the builder (5) to emit a manifest.
- Done-when: adding one unpetitioned file to the initramfs is detected.

### 3.5 Signature verification [coming up]
- git verify-commit over the whole history against constitution/keys,
  generated into an allowed_signers file.
- Done-when: the current commits pass; a commit signed by a key not in the
  directory fails.

## 4. Tester [in progress]

Why: tenet 8; build order "tester first".  Shares no model or prompt with
the mechanic.

### 4.1 Serial-capture runner [done]
- Boot the shadow VM, capture the serial port to a file from power-on to
  halt, with a budget (findings #9) after which the run is red.
- Done-when: the runner goes red on a VM that never halts and on a VM that
  prints a kernel banner.

### 4.2 TST.hello.exact-bytes executable [done]
- Byte-compare the capture to the 14 bytes.  Fill the runner field.
- Done-when: 4.3 proves it can fail.

### 4.3 Mutation script [done]
- Fixed list of mutations to hello: wrong byte, missing newline, extra
  write, no exit, exit before write.  Count caught mutants; all must be
  caught.
- Done-when: the script reports 5 of 5 caught, and removing the comparison
  from the test reports 0 of 5.
- As built: jig/test and jig/mutate run every test and every mutation in
  tests/mutations.txt, not just hello's.  5 of 5 caught; without the
  comparison 1 of 5 is still caught, by the time limit, which is right
  (findings 0007 #11).

### 4.4 Sensor runner [coming up]
- Time from QEMU start to last serial byte; compare to threshold 1000 ms;
  deliver red to the stability seat (a file or a message until the
  committee exists).
- Done-when: an artificial 2 s delay in boot turns it red.

## 5. Builder [in progress]

Why: tenet 3; design "Nix is the builder".  Decides nothing.

### 5.1 Translator [in progress]
- 2026-10-11: `jig/aios build <flavor>` walks primary -> loader -> serial
  from the index entries and compiles from the pool.  Shell now; Nix later.
- petition.toml to a Nix expression listing exactly the CMP entries at their
  versions from pool/.  No channels, no Home Manager.
- Done-when: the expression for 0000 builds; adding a component to the
  petition that is not in the pool fails naming the CMP code.

### 5.2 Reproducibility [coming up]
- Done-when: two hosts (jig and Pi, or jig and a VM) produce the same hash.

### 5.3 Live and shadow [coming up]
- Two QEMU definitions; the referee swaps only on a passing shadow; swaps
  back on a red sensor.
- Done-when: a deliberately broken shadow is never promoted.

## 6. Pool admission for 0000: segments, not a kernel [in progress]

Why: tenets 1 to 5; design "Linux is read, never booted"; findings 0001.
Petition 0000 names no kernel.  Linux is the pool we read; each segment is
lifted into its own pool directory with the hands it held cut, and arrives
with a SPEC and a TST written here because upstream has neither.  The order
below is the expected failure order.

### 6.1 CMP.hello@1 [in progress]
- C, no libc, static, _start entry.  Calls the loader's two doors (6.5) and
  nothing else; never touches hardware.  As-built spec derived from the
  source.  Index entry with depends = [CMP.loader@1] and
  requires = [CAP.tty.write, CAP.proc.exit].
- Done-when: index entry validates (1.5); test 4.2 green against the loader.

### 6.2 CMP.toolchain.c@1 [coming up]
- The jig's compiler is scaffolding; the pool's toolchain is a separate
  admission.  Decide the first pool compiler (tcc, or gcc at a pinned
  version, or the Guix bootstrap chain head).  Findings 0000 #11 applies.
- Done-when: hello, the serial segment and the loader build with the pool
  compiler alone.

### 6.3 CMP.serial.8250@1 [in progress]
- Read drivers/tty/serial/8250/ and find the lines that put a byte on the
  wire: wait for the transmitter, write the data register.  Lift those and
  nothing holding their hand (no tty layer, no device model, no locking).
  Record in the as-built spec what was cut and where it came from.  Write
  the SPEC (bytes appear on the port, in order, complete) and the TST.
- Done-when: the segment is a few dozen lines; its test goes red when one
  line is mutated; it exposes CAP.tty.write and requires CAP.dev.serial.

### 6.4 CMP.loader@1 [in progress]
- Takes over from the machine (the microvm firmware hands off via the
  Linux boot protocol or a PVH entry; read arch/x86/platform/pvh/ and
  arch/x86/boot/ for what is worth lifting), sets up the processor far
  enough to run one flat program, jumps to it, and when it exits asks the
  machine to reboot, which -no-reboot turns into a halt.  No scheduler, no
  memory manager, no interrupts beyond what the handoff forces.  Every line
  the handoff forces that no petition names is a finding.
- Done-when: SPEC.hello.2 (silence) green; the loader's own spec and test
  written; its as-built spec lists every port and register it touches.

### 6.5 The doors [in progress]
- How hello asks the loader to write bytes and to exit: a call table, a
  software interrupt, or a syscall instruction.  Decide once, as SPEC
  clauses on the loader, so that a later petition adds a door
  without moving these two.
- Done-when: a second toy program using only the doors runs under the same
  loader with no change to the loader.

### 6.6 Signed admission [coming up]
- Root key signs admission commits until committee seat keys exist (8.3).
- Done-when: each index entry carries a signed admission the referee
  accepts (3.5).

### 6.7 First green build and bloat figure [in progress]
- Record: lines in the image vs lines a petition justifies.  Expect near
  zero by construction; each unjustified line is a finding naming the
  segment that carries it.  Where it is recorded is findings 0000 #11.
- 2026-10-10: first green boot under software emulation in a container,
  14 bytes exact, machine exits by itself.  The figure is findings/0002.md.
  Remaining for done: the same on the jig under KVM, with the runner (4)
  recording it rather than a hand.

## 7. Quartermaster [coming up]

Why: design "The index is the menu"; agent 2.  Reads source, never changes
it.

### 7.1 Index derivation [coming up]
- From the forked Linux tree: components, dependencies, exposed CAPs,
  derived from code not docs.
- Done-when: the derived entry for serial.8250 matches the hand-written one
  from 6.3, or the diff is a finding.

### 7.2 Failure-to-code mapping [coming up]
- Build failure text to index code.  "undefined reference to write" means a
  missing CMP or CAP.
- Done-when: every failure in the 0000 build loop has a code.

### 7.3 Petition drafting [coming up]
- From a failure code, draft a pool-admission petition for review.

### 7.4 Segment classification [coming up]
- liftable, tangled, or not needed, for the Linux directories petition
  the ladder will reach into (block, a filesystem, tty input).  Tangled means the
  hands to cut outnumber the lines wanted.

## 8. Committee [coming up]

Why: tenet 9; design "The committee".  Five seats, five models, biases
written down, quorum fixed in advance.

### 8.1 Seats [coming up]
- security, performance, portability, stability, purpose-fit.  Different
  model per seat; at least one local (Ollama) so one voice never depends on
  the network.  Model clients collapse into the gateway later (10.1).

### 8.2 Quorum [coming up]
- rule = all for now.  Add: timeouts, what a counter does, what happens when
  a seat is unavailable.  Test deadlock deliberately.

### 8.3 Seat keys [coming up]
- One key per seat in constitution/keys; review signing per 1.6.

### 8.4 First review [coming up]
- Petition 0000 goes from draft to approved by five signed reviews.

## 9. The ladder [in progress]

Why: design "Failure builds the distribution".  Each petition is a program
every CS student wrote once, chosen because it needs exactly one thing from
the machine that the last one did not.  Each rung adds one door and never
moves an old one, so every earlier program keeps running under every later
loader, and that is a test run every time (9.8).  Each rung is also a post.

### 9.1 Petition 0001: Towers of Hanoi [done]
- Recursion and output.  Needs nothing new.  Proves additivity: a second
  program under CMP.loader@1 with a zero-line diff to the loader.  Closes
  6.5's done-when.
- Done-when: hello and hanoi both boot under the same loader binary; the
  move list is exact (2^n - 1 lines for n disks).
- 2026-10-11: done under software emulation.  Same loader binary, same hash,
  for hello and hanoi; 15 moves, 360 bytes, exact against a reference solver.
  Diff to the loader: zero lines.

### 9.2 Petition 0002: Conway's Game of Life [done]
- The memory chapter.  Two grids far bigger than the stack.  The program asks the door for memory.  Forces the
  loader to read the memory map the firmware hands over (hvm_start_info
  memmap) and to hand out pieces: a bump allocator first, free when a
  program needs it.  CAP.mem.alloc; door entry 3.  One tenant, no
  protection yet.
- Done-when: a program builds and walks a list larger than its own stack;
  a program asking for more than the machine has gets refused, not
  corrupted.
- 2026-10-11: done under software emulation.  CMP.loader@2 adds the alloc
  door (35 lines); Life runs 200 generations byte-exact against a reference;
  128 MiB refused; hello and hanoi unchanged under @2.  Findings 0005.

### 9.3 Petition 0003: a reverse-Polish calculator [done]
- The program reads.  Guess-the-number needs randomness, which needs a clock
  (9.4), so the input chapter became the stack chapter instead.  CMP.serial.8250@2 gains receive (LSR data-ready,
  RX); door entry 4, read.  Polling, no interrupts.
- Done-when: bytes typed on the serial port reach the program in order; a
  program that never reads leaves the port untouched.
- 2026-10-11: done under software emulation.  CMP.serial.8250@2 (receive
  side, lifted), CMP.loader@3 (read door, 13 lines), CMP.calc@1.  Transcript
  exact against an independent Python reference after the reference caught a
  32-bit overflow bug.  Hello, Hanoi and Life unchanged under @3.  Findings
  0006.

### 9.4 Petition 0004: a stopwatch [done]
- Ticks once a second, counting up from 00:00, redrawn in place on one line,
  until the machine is switched off (as built: no key; the tests watch it for
  a fixed time instead).  Each
  tick is aimed at start + n seconds, never at "a second after the last one",
  so a late redraw does not drift.  Waiting is polling the time door: no
  interrupts yet, and with one program nothing else wants the processor.  The one new thing:
  time passing.  CAP.time.read, elapsed time only, never the time of day;
  the processor's own counter or the LAPIC timer, whichever the machine
  petition can honestly present, and how fast it ticks has to be worked out,
  not assumed.  The clock debt from findings 0002 becomes a named capability.
  First real sensor: elapsed time with a threshold.  As built (findings 0008):
  the 8254 measures the processor's counter at boot, Linux's fallback method,
  chosen on purpose; drift 0 to 4 ms in 10 s under emulation.  Was Fibonacci timed;
  Chris swapped it on 2026-10-11: recursion is done, and a number at the end
  of a calculation is a dull way to show a clock.
- Done-when: a run of n seconds by the host's clock shows n seconds, within
  the sensor's threshold; the sensor goes red when the threshold is tightened
  below the error.

### 9.5 Petition 0005: save the sorted output [done]
- Persistent storage.  A block device the machine petition adds, a driver
  lifted from Linux, a filesystem lifted or written.  First expensive lift;
  the count of hands cut is the finding.  Chris chose on 2026-10-12, by the
  history rule: an IDE disk (registers from the PC/AT, 1984) over
  virtio-blk, and the original Minix filesystem (v1, 0x137F), Linux's first,
  over FAT and ext2.
- Done-when: a file written by one boot is read back by the next.
- 2026-10-12: done under software emulation.  CMP.disk.ata-pio@1 (122 lines,
  lifted from libata), CMP.fs.minix@1 (344 lines, lifted from fs/minix, about
  120 of them glue), CMP.loader@5 (load and save doors), CMP.sort@1,
  CMP.machine.serial-disk@1.  The second boot finds the first boot's file;
  fsck.minix finds the disk clean.  Lines from Linux: 638.  Findings 0010.

### 9.6 Petition 0006: two programs at once [coming up]
- The first purpose that needs sharing.  The kernel appears: a scheduler
  and whatever separation the purpose demands, every line named and
  justified by this petition.  The next CMP.loader growing into it, or a
  separate CMP.kernel@1 the loader hands off to; decided then, in a finding.
- Done-when: both programs produce their output; neither can corrupt the
  other's memory, and a test proves it by trying.

### 9.7 Petition 0007: the editor [coming up]
- Open a text file, type, save.  Needs 9.3 and 9.5.  C now; Rust later as
  CMP.editor@2.  libc (musl) enters here if anything fails on it, not before.

### 9.8 The ladder holds [coming up]
- Every program from 0000 up boots under the current loader with no change
  to the program.  Run on every commit that touches the loader or a door.

### 9.9 The loop [coming up]
- try build, referee names a code, quartermaster drafts, committee reviews,
  admit, retry.  Human in the loop at review until 8 exists.  Mechanic
  proposes patches and untangling refactors; never installs; separate model
  and prompt from the tester.

### 9.10 The first Linux binary [coming up]
- A statically linked Linux hello world, unmodified, runs on AiOs.  It makes
  two system calls, write and exit_group, both already behind the door: the
  loader answers Linux's system-call entry for those two numbers and loads
  the program the way Linux lays it out.  Nothing else of Linux's interface
  exists.
- From here each Linux system call is a capability of its own, petitioned by
  the first program that needs it, found by tracing that program on Linux,
  never by guessing.  The list of system calls AiOs answers is the record of
  which programs asked for each one.
- Done-when: the binary's output is byte-identical to the same binary on
  Linux; a system call nobody petitioned fails loudly with its number.

### 9.11 BusyBox vi, unmodified [coming up]
- The capstone of the Linux-binary rungs: BusyBox's vi, built static, runs
  on AiOs unchanged.  Expected to ask for files (on 9.5's filesystem),
  terminal control (raw keys, screen size), signals enough for Ctrl-C and
  resize, time and memory growth.  No fork, no exec.  The trace decides.
- Done-when: open a file, edit it, save it, quit; the saved bytes match what
  the same keystrokes produce in the same vi on Linux.

### 9.12 The clock on the screen [coming up]
- A wall clock: shows the time of day and keeps it.  The one new thing over
  the stopwatch: knowing what time it actually is.  The petition has to argue
  for where that comes from, a battery-backed clock chip on the machine or
  asking the host, and either is a hole in "nothing nobody asked for".  Its
  place on the ladder is decided when it is petitioned; numbered last so no
  earlier number moves.

## 10. Lifecycle [coming up]

Why: design "Lifecycle" and "Security consequences".

### 10.1 Gateway [coming up]
- Only outbound component; reaches the repo and the model provider,
  signature-checked.  Does not exist after seal.

### 10.2 Strip [coming up]
- Remove everything without a live petition.  Referee's job: remove it and
  confirm the seed can regrow it.
- Done-when: strip, then regrow from the seed, produces a bit-identical
  system.

### 10.3 Watchman and molting rule [coming up]
- Not a model.  Detects anomalies, triggers a molt.  Molting rule written
  while the system is still smart.

### 10.4 First sealed run [coming up]
- Hello runs sealed: no model, no outbound connection, only the watchman and
  the one sensor.
- Done-when: strace shows no socket syscalls across the sealed run.

## 11. Historian, cartographer, bare metal [coming up]

### 11.1 Historian [coming up]
- Records what worked where; pushes to the Pi; pulls upstream fixes through
  the door.  Never bypasses review.

### 11.2 Cartographer [coming up]
- Probes CPU, memory, devices from datasheets.  Needs a sacrificial board.

### 11.3 Purpose agent [coming up]
- Translates purposes into committee weights.  Never adds a purpose.

### 11.4 Gen two [coming up]
- Purpose: build other instances of me.

### 11.5 Self-rebuild and jig retirement [coming up]
- Gen two rebuilds itself bit-identical to what gen zero made.  This is also
  the trusting-trust check.  The jig is retired or absorbed.

### 11.6 First bare metal [coming up]

---

## Decisions log
- 2026-10-10: TOML for petitions.  Six tables (PUR added).  SSH-key signing
  with ed25519, no passphrase, Windows ssh-agent on the dev box.
- 2026-10-10: one test for 0000, not two; recorded as findings #8.

## Open questions not yet assigned to an item
- Where does the jig live: a second machine, a VM on the Windows box, or
  WSL?  design.md says NixOS minimal on the existing box.
- Which Pi, and is it already on the network?
- Licence policy for admission (index/README mentions "a licence the
  constitution accepts" but none is named).
