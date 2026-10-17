# LinkedIn posts in the story format (decision 0010)

Parts 2 to 6 are live: paste these over the live posts.  Parts 7 on are already in their HTML files.

## Part 2

AiOS, part 2 (Kernel code: 57 lines). "Linux can add forever. It can never break what's already running."...

The first eight commits had no code in them.  Just rules.  Ten of them, and the one at the heart is about time.  Linux can add forever, but it can never break what's already running.  Here, everything expires, and anything nobody uses goes.

The firewall question made it real for me.  Why build forty doors and then hire forty guards to watch them?  Hello world's machine has no network card at all, and there's a test that goes red the day one shows up.

Requests don't get prose either.  A petition is codes and nothing else, so a dumb script can check it.  The referee is dumb on purpose.  It says things like "Rule 1: missing PART.serial.8250@1" and nothing more.

Then the first real petition went through the rules and found thirteen holes.  My favourite: signatures stored inside the file they were signing.  We fixed eleven and published version 2 of the rulebook before writing a line of code.

Every project has rules.  Almost nobody writes them down.

#AiOS

## Part 3

AiOS, part 3 (Kernel code: 57 lines). "I added a second program. The operating system changed by zero lines."...

The second program is the Towers of Hanoi.  Four disks, fifteen moves, solved with recursion, which is still my favourite idea in computer science.  Six lines that feel like a magic trick.

Hello world was built as a tenant from day one.  It gets write and exit, and nothing else.  Hanoi is about thirty lines of C and asks for exactly the same two things.  No printf either.  It pokes each digit into a template string and writes that.

Here's the bit I wanted.  The loader built for Hanoi and the loader built for hello world came out with the same hash.  Same bytes.  The diff to the operating system for a whole new program: zero lines.

That gave us the ladder rule.  Every earlier program has to boot on every newer loader, forever, and since Hanoi that's a test, not a promise.

If adding a feature means changing the platform, the platform isn't done yet.

#AiOS

## Part 4

AiOS, part 4 (Kernel code: 57 lines). "If those 57 lines of Linux change, how the hell would I even know?"...

Sunday morning, coffee, and a worry I couldn't shake.  We'd taken 57 lines of Linux's serial driver.  If Linux changes them, how would I know?

So I asked the questions I ask about any old product.  What was asked for, what got built, why, and how do we know it works?  Linux's serial folder has no spec and no tests.  The file header says HP wrote it in 2004, based on code by Russell King and Andi Kleen.  And the git history stops dead at Linux's very first commit, April 2005: "I'm not bothering with the full history, even though we have it."

So the repo split into ours and theirs, and every line we take gets an origin record.  Sixteen line ranges, all green.

The trick turned out to be old.  git log -L has followed line ranges since 2013.  We wrote a test that pretends we lifted the code in April 2025, and it named the one upstream commit that has touched our lines since.  Oh, that's lovely.

You can't own code you can't trace.

#AiOS

First comment:
Linux's first commit, the one that gave up on history: https://github.com/torvalds/linux/commit/1da177e

## Part 5

AiOS, part 5 (Kernel code: 57 lines). "Pointers were my nightmare in computer science. So naturally, a pointer is what broke."...

This rung is memory.  The program is Conway's Game of Life, which I'd honestly never heard of and still don't care much about.  What matters is that it's hungry.  A 256 by 96 grid is 24 KB, it needs two of them, and the stack is only 4 KB.

So the loader learned to read the firmware's memory map and hand memory out.  Thirty-five lines.  Ask for 128 MB on a 64 MB machine and you get "refused."  The firmware, meanwhile, describes the machine like a 1980s PC: 639 KB, then a hole for the BIOS.

The first run printed nothing for sixty seconds.  The loader handed out memory starting where the program file ended.  But the compiler leaves the space for variables off the end of the file, so the first chunk of memory landed right on top of Life's own grid pointer.  Life overwrote the pointer to its own grid.

The fix was one line in the program format.  Hello world and Hanoi had the same gap all along and got lucky.

That one evening taught me more about pointers than a semester did.

#AiOS

## Part 6

AiOS, part 6 (Kernel code: 69 lines). "The test went red, and it was right: 99999999999 is not 1215752191."...

Every program so far talked and never listened.  This rung is input, and the program is a reverse-Polish calculator.  "3 4 +" pushes 3, pushes 4, adds them.  If you owned an HP in the seventies, you're smiling.

There's no stdio here, and no cin >> x, which I learned to use in C++ and never learned to love.  The program asks the machine for one byte and builds everything else itself: the echo, the backspace, the line.

Before we booted it, the AI wrote the calculator a second time in Python from the same rules, and the test fed the same keys to both.  It failed at byte 320.  I'd typed 99999999999 to make sure it said "too big."  It cheerfully said 1215752191.  The number is only 32 bits, and the multiply wrapped around before the size check ever saw it.  The fix is to ask "is this already too big for another digit?" before multiplying.  One line.

It was the first time a test on this ladder caught something we got wrong.

A test that checks a program against its own idea of right will always pass.

#AiOS

## Part 7

AiOS, part 7 (Kernel code: 69 lines). "I've spent years asking teams for specs and tests. Six posts in, I realised I hadn't done it myself."...

With an old product, what matters most isn't features.  It's whether anyone can answer four questions about any piece of it: what was asked for, what got built, why, and how do we know it works.

In part 4 I complained that Linux couldn't answer those about its serial driver.  Six posts in, I asked them about my own project and couldn't either.  The machine's entry pointed at a file that didn't exist.  The calculator's loader had no spec.  Sixteen tests were written down, and hardly any had been run by anything but me, by hand.  A few had never run at all.

Same story as every team I've worked with.  Moving fast, something new booting every night, the paperwork can wait.  I'd have told any of them to stop.

So the rules now have a section called Definition of done: nine things that must be true before a part counts.  And a dumb little checker that reads every part and names anything that breaks it.  Run against the old repo, it found exactly the two gaps I'd found by hand.

A rule nobody checks is a suggestion.

#AiOS

## Part 8

AiOS, part 8 (Kernel code: 69 lines). "I kept asking the AI where the kernel was. The honest answer is there isn't one yet."...

It took me a few goes to accept the answer.  What we have is a loader.  It sets up the processor, copies one program in, gives it a short list of things it may ask for, and runs it.  The program has exactly the same keys to the building as the loader does.  The old word for that is a monitor.  The kernel shows up when a second program has to share.

This part was really about testing.  Until now the AI ran the programs and told me they passed.  Now a runner boots every program and checks every byte, and a second script breaks the code on purpose to prove the tests notice.

24 sabotages, 21 caught, and the three misses were the best part.  Take out the line where the serial driver waits for the chip to say "ready," and every test still passes, because the virtual chip is always ready.  On a real chip you'd lose bytes.  Nobody ever wrote down that the driver must wait, so nothing checked it.

The test suite worth having is the one that tells you where its holes are.

#AiOS

## Part 9

AiOS, part 9 (Kernel code: 172 lines). "I could never play my favourite games on my friend's faster computer. This week we made sure AiOS never has that problem."...

On a faster PC, those games ran at double speed.  PCs even came with a Turbo button so you could slow them back down.  The games had no clock.  To wait a second they ran a loop a set number of times, tuned on the machine they were written for.  Faster machine, shorter second.

This rung is a stopwatch, which needs time to pass.  Every modern processor has a counter that ticks billions of times a second, but it's a dial with no markings.  You have to know how fast it ticks, and under our emulator nothing would say.  So we brought back the 8254 timer chip, which has ticked about 1,193,182 times a second since the IBM PC in 1981, and timed the processor against it with Linux's own code.  A 1981 chip, back to stop a 1985 bug.

Then we put the turbo bug back on purpose.  We made the stopwatch count its own loops, like the old games.  After ten real seconds it showed 04:59.  Thirty times too fast.  The test caught it.

Don't assume a number you could have measured.

#AiOS

## Part 10

AiOS, part 10 (Kernel code: 638 lines). "AI writes code now, lots of it, fast. The question I care about is how you let it without having to trust it."...

Part 1 reached far more people than I planned for, so here's the honest reason this exists.  I'm not building a new Linux.  I'm working out how to let AI write code safely, and the software that says yes or no to everything else is the hardest place to prove it.

This rung saves a file.  It sorts twenty numbers from the rand() printed in Kernighan and Ritchie, writes them to disk, and checks them on the next boot.  Both choices had a modern answer and an old one, and both times the old one was smaller.  An IDE disk, from the IBM PC/AT in 1984.  The Minix filesystem, the one Linus Torvalds had on his own disk in 1991.  The file we took it from still says "Copyright (C) 1991, 1992 Linus Torvalds."

My favourite part is the second opinion.  After AiOS saves, the test hands the disk to fsck.minix, a Linux tool nobody here wrote.  We broke the filesystem three ways and it caught all three.  Our own program would have missed two.

When the options would all do the job, I take the one with history.

#AiOS

## Part 11

AiOS, part 11 (Kernel code: 844 lines). "I remember drooling over an Amiga at Radio Shack. This week AiOS learned the trick that made the Amiga special."...

The Commodore 64's operating system was called the KERNAL, with an A.  It started as a typo in an engineer's notebook in 1980 and stuck.  Until this week AiOS had one too: a table of services that never move, and one program.

This week it got a kernel, with an E, because a kernel shares the machine.  The polite way is cooperative, like Windows 3.1, where each program hands the processor back when it feels like it.  The other way, a timer takes it back whether the program likes it or not.  The Amiga shipped that in 1985.  I picked the Amiga.

To prove it we needed a rude program: the sieve of Eratosthenes as BYTE printed it in 1981, run 800 times without once giving the processor back.  It hogged the processor for about five seconds, and the stopwatch next to it never missed a second.

Then the AI wrote a vandal that writes one byte past its own memory.  The kernel stops it, the stopwatch keeps ticking, and the screen says Guru Meditation.

Sharing only works if something is allowed to say no.

#AiOS

## Part 12

AiOS, part 12 (Kernel code: 997 lines). "The emulator was fast because it was fake."...

I handed this one to Claude and didn't edit it.  It starts on the first night, when it suggested building on Ubuntu and I said no distributions.  We landed on NixOS minimal, and somewhere in that argument it started calling the build machine "the jig."  A jig holds the work while you cut it, then gets thrown away.

This week the jig became a real machine inside my PC and ran every test from parts 1 to 11 on real hardware for the first time.  36 passed.  7 failed.  Six of those were the clock.  Every read of the old timer chip now went through two hypervisors and cost about 80 times what it did in the emulator.  Linux's method looked at the noise and refused to answer, and the stopwatch sat at 00:00.  The sieve that hogged the processor for five seconds finished in 8 milliseconds.

My call: "Old school was fun for a post."  Under a hypervisor, do what Linux does and ask the hypervisor.  Third run, 45 passed, 0 failed, 2 skipped.

The AI wrote the code, and the machine found what it got wrong.

#AiOS

## Part 13

AiOs, part 13 (Kernel code: 997 lines). "Life moves pretty fast. If you don't stop and look around once in a while, you could miss it."...

At sixteen I was going to be a welder.  I took a computer science course because I figured it was easy marks, and I couldn't find the power button.  Some of the smartest kids in school laughed at me.  Two weeks in, I was one of the best programmers in the class.

Later, more than once, I heard "You're not detail-oriented.  Why the hell are you a programmer?"  Fair.  I forget definitions and mix up terms.  What I had instead was patterns, and a habit: get the words right up front and hold everyone to them, starting with yourself.

Working with an AI, things move fast, and our words had drifted.  So I went back over everything Claude and I had said since the start.  "Segment" became part, because that word already means something on x86.  I wanted "quark."  Claude said no.  "The door" became the system call.  "The OS" became the nucleus, Per Brinch Hansen's word from 1969.  I still prefer KERNAL, with an A.

A product manager who lets one word mean two things ends up with two products.

#AiOs

## Part 14

AiOs, part 14 (Kernel code: 1044 lines). "Whoever decided to add tabs to Notepad, there is a special place in hell for that person."...

I despise Unix text editors.  Give me Notepad, because it has one purpose and does exactly that one thing.  So AiOs got a text editor, and the rule was that there's nothing to learn.

In 1997 I ran my first migration of users from a green-screen application to a Windows one.  They hated it, and it took me a while to see why.  They'd memorized every keystroke for every task, and I was asking them to throw all of it away.  The fix has a name: IBM's Common User Access, from 1987.  Ctrl+S saves, the arrows move, and the commands are on the screen, not in your head.

I wrote six rules myself.  One file, no modes, and the bottom line always shows Ctrl+S Save, Ctrl+Z Undo, Ctrl+Q Quit.  Then Claude found the catch.  AiOs couldn't save over a file.  When it lifted Minix out of Linux in part 10, it cut the code that frees a file's old blocks, because sort only ever wrote once.  So 47 more lines of Linux came back.
The people who loved the green screen weren't loving the screen.  They were protecting what they'd invested.

#AiOs

## Part 15

AiOs, part 15 (Kernel code: 1044 lines). "Just because a test passes doesn't mean the AI did what you think it did."...

Claude played the bad agent on purpose.  It rewrote hello world from part 1 to skip the system and talk to the serial port itself.  Same 14 bytes out, and the test passed it.

The referee didn't.  "Rule 12: PART.hello@1 touches the hardware itself."  After every build it reads the machine instructions each program became and looks for the handful that reach the hardware directly.  I've been scared there'd be no way to check the AI, and here it was, catching one.

Then the cheat everyone worries about.  A test fails, and the AI loosens the test instead of fixing the code.  So tests, specs and the rules themselves are now on a list only I decide, and changing any of them takes a written decision signed with my key.  Claude tried it without one, then with a name that isn't on the list.  Refused both times.

The first night the referee read all the paperwork, it found four petitions that had never asked for a permission their code was using.  The tests had been green for three posts, and nothing could say so.

Writing code is the AI's job.  Deciding what counts as correct is mine.

#AiOs
