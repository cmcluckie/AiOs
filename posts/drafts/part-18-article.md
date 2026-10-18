*The story behind AiOs, part 18.*

## The long rectangle

Around the time I got into programming, the mother of the person I was dating gave me an old IBM 5150.  I wrote a video game on it, a spaceship shooter.  About three weeks in, I pulled the machine apart to see how it all connected, and I regretted it fast, because then I couldn't test my game anymore.  I went looking for the processor.  I think it was an 8088, and I remember finding it odd that it was a long rectangle, nothing like the square Pentiums I'd seen.

Later I read books on something called protected mode, and nobody explained it well.  Here's the part I'm leaving in on purpose.  From what I read back then, the 386 added it.  But in writing this part I learned the 286 did, and it sucked so much that nobody used it.  Maybe the 386 was just the first one where it was usable.  I haven't looked into it, and I'd rather leave the doubt in than pretend I know.  [Apparently OS/2 used it, but who the hell used OS/2?]

Here's what the 5150 has to do with this.  The 8088 inside it has no protected mode at all, so every program on that machine ran with every privilege there was.  That's exactly where AiOs has been until this part.

[The first candidate for this project was getting Linux running on a 6502.  I dropped that idea quickly, once I realized how impossible and useless it was.]

## What was wrong

Part 16 gave every program its own memory, so two programs can't see each other.  That part is real.  But I never asked the harder question: what can a program do to the machine itself?

Answer: anything.  A program can write straight into the screen's memory.  It can poke the serial port without asking.  It can read the nucleus.  The nucleus is what I call the centre of AiOs, the thing everything else is built around, and the rest of the world would say the kernel.  The hole was even written down in the findings, and I'd read it, and it hadn't really sunk in.

Here's an AI miss worth owning.  When I asked Claude what the share part did and didn't protect, it told me the first megabyte of memory, the screen's included, was mapped into every program.  That turned out to be wrong.  Share doesn't map the screen at all.  Claude found out when it read the code for the real answer, and said so.  The real hole is simpler and bigger: programs run at the highest privilege the processor has.

## The idea, in plain English

The nucleus starts a program and tells the processor it runs with fewer privileges.  It can't touch the nucleus's memory, the screen or the ports.  Instead of reaching for those directly, the program puts a message on a bus.  The nucleus picks it up, does the job, and hands the answer back.

[For the exact version: the program runs one instruction, `int 0x80`, which is a trap.  The processor jumps to a fixed place in the nucleus with the arguments sitting in registers.  The nucleus does the job and returns.  The processor has four levels of privilege, called rings.  The nucleus runs in ring 0 and programs run in ring 3.]

I call the part that does this the gate.

## I had no idea what either meant

Claude stopped and asked me two questions.  First: should the processor fence off memory the old way, or with something called paging?  Second: what numbers should the system calls get that Linux has no number for?  I had no idea what either question meant, and I told Claude so, in words I won't print.  Then I asked what I should have asked first: what is this protecting against?  Tell me that, then ask me to choose.

Once it was in plain words, the choice was easy.  The processor has two ways to fence memory.  The old way cuts memory into slices, and it can't hide the nucleus, which sits under the programs.  The other way is paging: the nucleus keeps a map of which pieces of memory a program may touch, and the processor checks the map on every single access.  I said this is a modern operating system, so it gets done the way it's meant to be done.

## Linux's numbers

A program asks for something by number.  Linux already has numbers for the three calls that matter most: exit is 1, read is 3, write is 4.  So AiOs now uses Linux's numbers.  That's a little step toward running real Linux programs one day.

Four of AiOs's own calls, alloc, ms, load and save, have no Linux number yet.  They get numbers from 1000 up, as a temporary measure.  I said they must be marked as tech debt that has to be removed, so every one of them has a `TECH-DEBT 1` comment in the code and an entry in the findings saying what replaces it and when it goes.  There's a test that checks the two lists match.  Forget a mark, or forget the entry, and the test goes red.

## Tests that cheat

Claude and I wrote the tests first, and they cheat on purpose.  One program writes into the screen's memory.  One pokes the serial port.  One reads the nucleus's own memory.  One never uses the table at all and asks with the raw trap.  Each one was shown failing on the old nucleus before the gate existed.

Then Claude built the gate.  The screen cheat stops with a page fault at `000B8000`.  The nucleus read stops at `00100000`.  The port write stops with a general protection fault.  Each prints a Guru Meditation line, the Amiga's name for a program that's been stopped.

Here's the thing I loved: when more than one program is running, the one that cheats is stopped, and the stopwatch next to it never skips a second.

## What nobody asked for

A program doesn't only reach for memory directly.  It can hand the nucleus an address and say "write these bytes to the console from here."  If that address belongs to the nucleus, the nucleus would happily read its own private memory out to the screen.

Nobody asked for that check.  Claude put it in, and then told me it had no spec and no test, which is exactly the kind of thing that rots.  I said go.  The spec is one sentence: a call handed an address outside the program's own memory is refused with the same error Linux uses, and the nucleus doesn't touch the address.  Then, to prove the test can fail, Claude sabotaged the check so it accepted everything.  The nucleus printed its own bytes on the console, a line of garbage, and the test went red.  Isn't that cool?  Tests are written to fail, and a test that can't go red is decoration.  Then Claude put the check back.

## A bug that said nothing

My favourite failure.  The machine just reset.  No message, no error, nothing, right at the first system call.  A reset with no message is the worst kind of bug.

Claude asked the emulator to log every interrupt, and the log told the story.  The processor had complained about a table, couldn't handle its own complaint, and given up and rebooted.  The cause was one line.  The address of a table the processor needs was being cut short because a number was too small to hold it, so the processor was looking in the wrong place.  The fix was a cast to a bigger number.

## Sharing again

Sharing was the big move.  Part 16's share part had its own page tables, its own interrupt table, and its own code for saving a program's registers.  All of that now lives in the gate, and share keeps only the turn taking: the timer, who's next, and what to do when a program exits or is stopped.  Share's code went from 353 lines (share.c and isr.S) to 177 (share.c).

The old nucleus and the old share are untouched.  A new flavor, called gateshare, runs the same stopwatch and sieve on the new one.  It passes the old checks and new ones, including the stopwatch beside a vandal and beside a program that never gives up the processor.

One honest wrinkle.  In a full run of everything, one older test failed once: the stopwatch reached 00:06 in a 6 second window where the test accepts 00:04 or 00:05.  That nucleus wasn't touched, it passed three reruns, and I read it as timing at the edge of the window.  It stays on the list until I've seen it again.

## Two holes I left open on purpose

The gate has two known holes, and I'm leaving both open.

The first: a system call runs with the timer switched off.  So if one program waits for a key, nothing else gets a turn, and the stopwatch freezes.  The second: a program's memory is handed out in 4 MiB chunks, so if its end doesn't land on a chunk boundary, it can reach the rest of that chunk, memory it was never given.  No test catches either, because none of them ask for it.

Claude found both in a cold read of the code, with no knowledge of how it was built.  Claude checked each one in the code and had already written the fix for the first.  I said leave them.  Both are marked `TECH-DEBT 2` and `TECH-DEBT 3` in the code and listed in the findings, so the test that matches marks to entries still passes.

Here's why.  This series is the climb up to a working system, and I'm saving a mechanic series for after it: go back, fix what's rough, and write each fix up as its own post.  If I fix everything now, that series has nothing to write about.  These two are the start of its list.  A hole you know about, marked and written down, is a different thing from a hole you didn't find.

## What I'm taking away

The piece this rung added: a limit is only a limit if the machine enforces it, and you only know it holds if you've watched something try to break it.  A program can ask.  It can't reach.

That's the shape I want for the AI agents later.  An agent can ask for what it needs, and a check, not the agent's good intentions, decides what it gets.  I'm not pitching that yet.  I'm just noting the shape came out of a processor from 1982.

## Read along

1. [pool/PART.gate@1/gate_user.S](https://github.com/cmcluckie/aios/blob/main/pool/PART.gate@1/gate_user.S).  The table of stubs a program is handed.  Each one is a single trap.
2. [pool/PART.gate@1/gate.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.gate@1/gate.c).  The whole gate.  Look for `user_ok`, and for the `TECH-DEBT` marks.
3. [tests/programs/gate-screen.c](https://github.com/cmcluckie/aios/blob/main/tests/programs/gate-screen.c).  The program that reaches for the screen.
4. [pool/PART.share@2/share.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.share@2/share.c).  What's left of sharing.
5. [findings/0021.md](https://github.com/cmcluckie/aios/blob/main/findings/0021.md).  The plan, my rulings, the tech debt, and the correction.

## Try this

Open the repo in Codespaces and run `sh tests/programs/gate-screen.sh`.  You'll see the program stopped at `000B8000`.  Then open `pool/PART.gate@1/gate.c`, find the line in `base_init` that gives each kernel page the flag `0x3`, change it to `0x7`, and run it again.  Now the screen is open to the program, and the test goes red.  `sh jig/mutate gate.screen` does that for you and reports KILLED.  Both need QEMU, and the script says SKIP if it isn't there.
