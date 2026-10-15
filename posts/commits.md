# Commit ledger

Every commit Claude asks Chris to make, which post it belongs to, and the hash
once Chris reports it.  A post may own several commits; a commit owns one post.
Status: requested (hash unknown), confirmed (hash seen), pushed.

| Post | Commit subject | Hash | Status |
| --- | --- | --- | --- |
| 2026-10-12 (part 2, the rulebook) | Article One | 8167fbc | pushed |
| 2026-10-12 | Petition schema | d9c11f0 | pushed |
| 2026-10-12 | Petition 0000 | 3d691ac | pushed |
| 2026-10-12 | Findings 0000 | 007cc88 | pushed |
| 2026-10-12 | Project plan | d07d09a | pushed |
| 2026-10-12 | Entry formats | 84eb0fd | pushed |
| 2026-10-12 | Detached signatures | 4e4e1fb | pushed |
| 2026-10-12 | Schema version 2 | 6b41afe | pushed |
| 2026-10-11 (part 1, 170 lines) | The jig | 7537cfc | pushed |
| 2026-10-11 | The machine | 4b1b4a6 | pushed |
| 2026-10-11 | Findings 0001: segments, not a kernel | 6ecfafc | pushed |
| 2026-10-11 | Petition 0000 boots | 2fdd428 | pushed |
| 2026-10-11 | Line endings: LF everywhere, CRLF for PowerShell | 7c6a876 | pushed |
| 2026-10-13 (part 3, Hanoi) | The ladder, and the posts folder | 736d622 | pushed |
| 2026-10-13 | Petition 0001: Towers of Hanoi | 25d3699 | pushed |
| 2026-10-14 (part 4, ours and theirs) | Ours and theirs: lifted/, origin records, upstream watch; flavors and the referee placeholder | c246e58 | pushed |
| 2026-10-14 | Ledger: close c246e58 | fbebc14 | pushed |
| 2026-10-14 | VS Code tasks via WSL; machine falls back to emulation without KVM | fbebc14 | pushed |
| 2026-10-14 | Run and Debug lists the flavors | d3bfebb | pushed |
| 2026-10-14 | Press play from a browser: devcontainer and README | e8dcf79 | pushed |
| 2026-10-15 (part 5, memory) | Petition 0002: Conway's Game of Life; loader@2 with the alloc door | 6c21f6c | pushed |
| 2026-10-16 (part 6, the calculator) | Petition 0003: a reverse-Polish calculator; loader@3 with the read door | 00099c4 | pushed |
| 2026-10-17 (part 7, definition of done) | Definition of done: as-built specs, tests and the addenda | 6280a41 | pushed |
| 2026-10-17 | Ledger and post links: close 6c21f6c, 00099c4, 6280a41 | 0a49b8b | pushed |
| none | README: the parts of AiOs, in one picture | 5cdca86 | pushed |
| none | Posts become one HTML file each: post, title and article, with Copy buttons | 6bf9a21 | pushed |
| none | Post images drawn, kernel line counts, and two Linux-binary rungs in the plan | 7c0e7a9 | pushed |
| 2026-10-18 (part 8, the testers) | Tests run themselves: jig/test, jig/mutate, expected outputs and probes | bd08552 | pushed |
| none | Part 8 links: close bd08552 | ceddcf7 | pushed |
| none | Plan: a stopwatch, not Fibonacci, for the clock rung; the wall clock gets its own | ae00f8f | pushed |
| 2026-10-18 | Part 8 rewritten in the series voice | daa495a | pushed |
| 2026-10-18 | Part 8 back to the version scheduled on LinkedIn; the rewrite kept as a draft | d25c384 | pushed |
| none | Three clauses from the sabotage: the driver waits, sets 8N1, and reads never over-report | a61e96a | pushed |
| 2026-10-19 (part 9, the stopwatch) | Petition 0004: a stopwatch, and the whole repository is GPL-2.0-only | 2876c14 | pushed |
| 2026-10-19 | Part 9 links filled in; part 1's old article back so the LinkedIn link works | d9958c3 | pushed |
| none | Posts 1, 2, 4, 8 and 9: six claims a kernel developer would flag, corrected | 4d6c509 | pushed |
| none | Part 1 back as published: warts and all | 9f872c2 | pushed |
| none | The series' underlying message: safe, automated, agentic development | 9f872c2 | pushed |
| none | Rule: when options would each do the job, default to the one with the history | 1d14857 | pushed |
| 2026-10-20 (part 10, the disk) | Petition 0005: save the sorted output, on an IDE disk, in Minix | 8247a91 | pushed |
| 2026-10-20 | Part 10 links filled in | 9b980aa | pushed |
| 2026-10-21 (part 11, two at once) | Petition 0006: two programs at once; the KERNAL becomes a kernel | 9cbe6b7 | pushed |
| 2026-10-21 | Part 11 links filled in (one commit with the jig fixes below) | 5ea8f27 | pushed |
| 2026-10-22 (part 12, the jig) | The jig, before its first boot: gcc_multi for -m32, Hyper-V guest drivers, install from the public repo (in the part 11 links commit) | 5ea8f27 | pushed |
| 2026-10-22 (part 12, the jig) | install.sh: wait for udev before mounting by label, lock the flake before installing: the jig's first two bugs | 224de12 | pushed |
| 2026-10-22 (part 12, the jig) | jig: flake.lock pinned, tools.sh reads the added packages, /boot root-only, Claude's voice | 1f8a7a4 | pushed |
| 2026-10-22 (part 12, the jig) | jig's first run: CMP.timer.tsc-pit@2 (150 ms window), loaders 4-6 re-pointed, disk strace limit 120 s, pitprobe, findings 0012 | e160203 | pushed |
| 2026-10-22 (part 12, the jig) | clock: ask KVM first (CMP.timer.kvmclock@1), the 8254 as backup; nopit/nokvm runners; share.on-time@2 with a hog that never yields; findings 0012 #9-13 | 0296132 | pushed |
| 2026-10-22 (part 12, the jig) | findings 0012 #14-16: the strace slowness was the 8254, not the disk; loader.ms survivor recorded | 9c9304d | pushed |
| 2026-10-22 (part 12, the jig) | Part 12: the post, the image, the draft, results and mutations from the jig, loader@5 as-built | 823f7c3 | confirmed |
| none | Language matters: the nucleus, part, agent, system call (was the door), catalogue and catalogue number (were the index and code), the decision ledger; CMP split into PART, MACHINE, TOOL with directories moved; README series index and new picture; plan 3.6, 7.5, two parked; findings 0013, 0014, 0015 (shared source reader); retired-words check; upcoming stories | 3d01b84 | pushed |
| 2026-10-23 (part 13, language matters) | Part 13: the post, the image, Chris's notes and the draft | 040f92c | pushed |
| none | Language matters, follow-up: the components picture in the new words (system calls, catalogue, mechanic and committee apart); ledger rows closed | ced313d | pushed |
| 2026-10-23 (part 13, language matters) | Part 13: real commit links; the image in the new words | 27a9a27, e4a4843 | pushed |
| 2026-10-24 (part 14, the editor) | Petition 0007: the editor, Notepad-plain; PART.fs.minix@2 saves over a file (Linux's truncate); PART.loader@7; seven tests, fourteen mutations; the disk runner types a different input on each boot; the agent-part rhythm recorded | 414bd19 | pushed |
| 2026-10-24 (part 14, the editor) | Part 14: the jig's results (52 passed, 76 of 78 mutations caught) and the commit links | | requested |
