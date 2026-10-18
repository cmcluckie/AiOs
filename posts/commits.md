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
| 2026-10-22 (part 12, the jig) | Part 12: the post, the image, the draft, results and mutations from the jig, loader@5 as-built | 823f7c3 | pushed |
| none | Language matters: the nucleus, part, agent, system call (was the door), catalogue and catalogue number (were the index and code), the decision ledger; CMP split into PART, MACHINE, TOOL with directories moved; README series index and new picture; plan 3.6, 7.5, two parked; findings 0013, 0014, 0015 (shared source reader); retired-words check; upcoming stories | 3d01b84 | pushed |
| 2026-10-23 (part 13, language matters) | Part 13: the post, the image, Chris's notes and the draft | 040f92c | pushed |
| none | Language matters, follow-up: the components picture in the new words (system calls, catalogue, mechanic and committee apart); ledger rows closed | ced313d | pushed |
| 2026-10-23 (part 13, language matters) | Part 13: real commit links; the image in the new words | 27a9a27, e4a4843 | pushed |
| 2026-10-24 (part 14, the editor) | Petition 0007: the editor, Notepad-plain; PART.fs.minix@2 saves over a file (Linux's truncate); PART.loader@7; seven tests, fourteen mutations; the disk runner types a different input on each boot; the agent-part rhythm recorded | 414bd19 | pushed |
| 2026-10-24 (part 14, the editor) | Part 14: the jig's results (52 passed, 76 of 78 mutations caught) and the commit links | 0b9acfb | pushed |
| 2026-10-25 (part 15, unplug from the matrix) | The decision ledger, small: constitution/human-only.toml, decisions/0001-0002, jig/aios ledger, the pre-commit hook; rule 12, a program touches no hardware; two tests, three mutations; findings 0016; part 15 draft | a2a8490 | pushed |
| 2026-10-25 (part 15, unplug from the matrix) | The referee checked against its own rule book: `jig/aios rules` (rules 2 to 8, tests/referee-results.txt), SPEC.referee.2-8@1, TST.referee.2-8@1, seven mutations; rule 13 and constitution/verbs.toml, `jig/aios lines`, SPEC.referee.13@1, TST.referee.verb-noun@1, three mutations; decisions 0003-0004; findings 0017; plan 3.2, 3.4, 3.5, 3.8-3.10, 7.7; part 15 draft, image, notes | ee2f53c | pushed |
| 2026-10-25 (part 15, unplug from the matrix) | Votes are decisions (rule 10, schema/signing.md); the machines expose the hypervisor's clock and the four petitions ask for it, two unfinished sensors out (decision 0005); two rule books, two readers, tenets out of constitution/, rules 17 to 19, the ledger's globbing hole fixed with a probe case and a third mutation (decision 0006); plan 3.11, 3.12, 5.8; findings 0017 #5-7; the jig's results for ee2f53c; part 15 article and notes | 40d7e3d | pushed |
| 2026-10-25 (part 15, unplug from the matrix) | The fixed ledger refused ee2f53c on the jig (the keys README, no decision naming it); rule 20, a later decision may amend an earlier commit; decision 0007 covers ee2f53c; a probe case and a fourth mutation; findings 0017 #8 | 795d6ba | pushed |
| 2026-10-25 (part 15, unplug from the matrix) | Part 15 closed: the jig's results for 795d6ba (56 passed, 91 of 93 mutations caught, the referee's file at 4 FAIL), the article folded up to date, posts/2026-10-25.html; the referee leaves the jig and is rewritten in C: pool/AGT.referee@1/referee.c (546 lines, the same four probes, fifteen mutations rewritten), AGT.referee@1 (a new table), PUR.0008 and petition 0008, TOOL.sh@1, jig/aios the builder only, jig/ledger the ledger, rule 12's list in constitution/instructions.toml; the last twelve questions decided (plan 3.13); decisions 0007 (amended), 0008; findings 0017 #9 | 99fa5e6 | pushed |
| 2026-10-25 (part 15, unplug from the matrix) | Part 15 closed: the jig's results for 99fa5e6 with the referee in C (56 passed, 92 of 94 mutations caught), the addendum and links, posts/2026-10-25.html rebuilt | 80da74b | pushed |
| series (schedule and post format) | The post becomes the story told short (decision 0010): CLAUDE.md, posts/voice.md, jig/postcheck by part number; parts 7 to 15 renamed to every second day from 2026-10-18 (part 15 now 2026-11-03), their posts rewritten, README.md's table; posts/drafts/linkedin-posts.md with parts 2 to 6 for the live posts; "yesterday" out of parts 13 and 14 | 0f3ed3d | pushed |
| 2026-11-03 (part 15, unplug from the matrix) | Fact fix: the hypervisor's clock arrived in part 12 for the stopwatch from part 9, and the four petitions were the stopwatch's, the sort's, the sieve's and the editor's, three posts of wrong paperwork not four; post and article; the series row's hash | 2d85ad1 | pushed |
| 2026-11-03 (part 15, unplug from the matrix) | The article's Copy button puts a marker naming each image's file where it goes, since images don't paste into LinkedIn; parts 7 on and the template | a1dd78f | pushed |
| 2026-11-05 (part 16, code authentic) | The kernel built from the petition (plan 5.8, decision 0011, findings 0018): PART.loader@8 and PART.share@1, the builder turns on only the parts a flavor's petitions name, sort, the sieve and the editor moved onto it and name only what they use, the referee walks the boot kernel (0 FAIL), TST.loader.partition@1, ladder-8 per flavor; the screen (plan 9.15, decision 0012, findings 0019): PART.screen.vga@1 lifted, MACHINE.serial-vga@1, PART.life@2 in colour, pixel tests, rule 12 fails closed; part 16's first half drafted | cf303c1 | pushed |
| 2026-11-05 (part 16, I never looked) | Part 16 closed: the jig's results for cf303c1 (63 passed, 102 of 104 mutations caught, the referee's file at 0 FAIL); the article, edited, posts/2026-11-05.html and its image; README's table | 7365385 | pushed |
| 2026-11-07 (part 17, the exam) | The shared source reader (plan 7.0, decision 0013, findings 0020): jig/reader reads Linux at one pinned commit (jig/linux.pin) and refuses what is not there, cscope in the tool table and jig/configuration.nix, TOOL.reader@1, TST.reader.* (22 mutations); the exam's run record, spending cap, line counter and scorecard (decision 0014): jig/runs, jig/scorecard, exam/items.toml, jig/prices.toml, jig/budget.toml, TOOL.exam@1, TST.exam.* (18 mutations killed, 1 known survivor), and jig/judge and the first model row and call (decisions 0015 and 0016, jig/model, TST.exam.ping@1), and jig/sit, the runner that sits an item with up to three tries (decision 0017, TST.exam.sit@1, jq added to the jig); the first real run of the serial item and the prompt sentence it showed was missing (decision 0018, findings 0020 15), and prompt caching in the runner and cached tokens in the run record (decision 0019, TST.exam.cache@1), which grades a lift by standing it in for the hand lift and running the item's tests (SPEC.exam.5@1, TST.exam.judge@1), which also fixed the serial item to name the 57-line first serial part; plan 7.0 and 7.8; the five-part plan in posts/upcoming.md; Chris's notes for the article | 8a823df | pushed |
| 2026-11-07 (part 17, the exam) | The scorecard keeps sittings apart (decision 0020, findings 0020 #17): jig/runs records a sitting and the prompt's fingerprint, jig/sit sets them, jig/scorecard prints a line per sitting and the cost per passing sitting; SPEC.exam.2@1 and 4@1, five tests touched, five mutations added | 810d191 | pushed |
| 2026-11-07 (part 17, the exam) | The first scorecard: the timer (five runs, three sittings, all failed on the 50 ms window) and screen (passed) runs on Sonnet 4.6, findings 0020 #18 | c550ddc | pushed |
| 2026-11-07 (part 17, the exam) | Part 17: the article (posts/2026-11-07.html, its image, posts/drafts/part-17-article.md), the jig's results for c550ddc (76 passed, 172 of 175 breakages caught, the referee at 0 FAIL), findings 0020 #19; the ledger rows for 810d191 and c550ddc and the compare link wait for this commit's hash | c654be9 | pushed |
| 2026-11-07 (part 17, the exam) | The as-built specs for TOOL.exam@1 and TOOL.reader@1, written by hand from the scripts; the catalogue entries point at them; the page and findings 0020 #19 say so; decision 0021 | 27e4427 | pushed |
| 2026-11-09 (part 18, the gate) | Chris's notes for part 18 in posts/upcoming.md: the 5150 story, what he learned about protected mode, the facts checked, and the open question about the 386 | | requested |
| 2026-11-09 (part 18, the gate) | The gate planned: findings 0021, decision 0022, SPEC.gate.1 to 4, six TST.gate.* tests (all red until built), plan 9.13 in progress; no code yet | | requested |
| 2026-11-09 (part 18, the gate) | The probes behind the six gate tests (decision 0023): four probe programs, their scripts, a shared helper and the debt check; each shown red on loader@8 in software emulation, each green path checked against a faked stop line | | requested |
