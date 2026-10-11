# AiOS project plan (brief)

One sentence per item.  Status in brackets: [done], [in progress], [coming up].
The detailed plan is plan/full.md; this file is the one to read.

1. Constitution: the signed repo everything traces to. [in progress]
   1.1. Repo rules, ten tenets and purpose 0000 committed. [done]
   1.2. Petition schema with the code tables and referee rules. [done]
   1.3. Petition 0000, hello world, written against the schema. [done]
   1.4. Findings 0000, the thirteen schema gaps, recorded. [done]
   1.5. Define the entry file formats for CAP, CMP, SPEC, TST and SNS. [done]
   1.6. Fix the review signing scheme so signatures live outside the file they sign. [done]
   1.7. Resolve the remaining gaps and publish the schema at version 2. [coming up]

2. The jig: the gen-zero host that turns the crank. [coming up]
   2.1. NixOS minimal on the existing box with one configuration of about twenty lines. [coming up]
   2.2. Bare git repo on the Pi over SSH that rejects unsigned commits. [coming up]
   2.3. Every installed tool has a complete row in the tool table, including how it leaves. [coming up]
   2.4. KVM and QEMU VM definition with one serial port and no NIC, as a petition. [coming up]

3. Referee: no model, no judgment, fails loud. [coming up]
   3.1. Resolver that checks every code reference resolves to an entry at a version. [coming up]
   3.2. Relation checker for rules 1 to 10 that reports a rule number and a code, nothing else. [coming up]
   3.3. Expiry, status and quorum enforcement. [coming up]
   3.4. Artifact check for rule 11: the built image and VM against the petition. [coming up]
   3.5. Commit and review signature verification against constitution/keys. [coming up]

4. Tester: manufacture failures before the world does. [coming up]
   4.1. Runner that boots the shadow VM and captures every byte on the serial port. [coming up]
   4.2. Executable form of TST.hello.exact-bytes bound to the runner. [coming up]
   4.3. Mutation script that breaks hello on purpose and proves the test goes red. [coming up]
   4.4. Sensor runner for SNS.hello.boot-to-output with the threshold as a number. [coming up]

5. Builder: turn an approved petition into the system, deterministically. [coming up]
   5.1. Translator from a petition file to a Nix expression. [coming up]
   5.2. Build that reproduces bit-identical on two hosts. [coming up]
   5.3. Live and shadow copies, swapped by the referee only on a passing shadow. [coming up]

6. Pool admission of what petition 0000 needs. [coming up]
   6.1. CMP.hello@1: raw-syscall hello in C with its as-built spec and index entry. [coming up]
   6.2. CMP.toolchain.c@1: a C compiler admitted as the smallest reviewable unit. [coming up]
   6.3. CMP.kernel.core@1: carved out with no conditionals inside, silent boot, halt when PID 1 exits. [coming up]
   6.4. CMP.kernel.serial.8250@1: the serial driver on its own. [coming up]
   6.5. CMP.kernel.initramfs@1: the initramfs support on its own. [coming up]
   6.6. Signed admission of each by the root key until committee seats exist. [coming up]
   6.7. First green build of 0000 with its bloat figure recorded. [coming up]

7. Quartermaster: decide what enters the ecosystem. [coming up]
   7.1. Index derived from source: components, dependencies, exposed capabilities. [coming up]
   7.2. Build failures mapped to index codes, never raw tool output. [coming up]
   7.3. Pool-admission petitions drafted automatically from a failure. [coming up]
   7.4. Kernel core classified as needed, not needed, or tangled. [coming up]

8. Committee: competing priorities, slow policy. [coming up]
   8.1. Five seats on separate models, each bias written down. [coming up]
   8.2. Quorum rules fixed in advance and tested under deadlock. [coming up]
   8.3. Seat keys in constitution/keys and review signing that works. [coming up]
   8.4. Petition 0000 approved by a real review. [coming up]

9. The loop: build, fail on a code, admit, retry. [coming up]
   9.1. Loop driven end to end by referee failures. [coming up]
   9.2. Petition 0001: a console text editor that fails only on what is new. [coming up]
   9.3. Mechanic that proposes patches and refactors and never installs. [coming up]

10. Lifecycle: grow, prove, strip, seal, molt. [coming up]
   10.1. Gateway as the only component that talks to the outside. [coming up]
   10.2. Strip everything without a live petition and prove the seed regrows it. [coming up]
   10.3. Watchman and the molting rule decided while the system is still smart. [coming up]
   10.4. First sealed run of hello with no model and no outbound connection. [coming up]

11. Historian, cartographer and bare metal. [coming up]
   11.1. Historian records what worked, pushes to the Pi, pulls upstream through the door. [coming up]
   11.2. Cartographer maps hardware on arrival from datasheets, never trial and error. [coming up]
   11.3. Purpose agent weights the committee's priorities per machine. [coming up]
   11.4. Gen two: a machine whose purpose is to build other instances of itself. [coming up]
   11.5. Gen two rebuilds itself bit-identical to what gen zero made; the jig retires. [coming up]
   11.6. First bare-metal machine built by petition. [coming up]
