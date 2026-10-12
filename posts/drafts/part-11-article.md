*The story behind AiOS, part 11.*

## The machine in the window

I remember drooling over an Amiga at Radio Shack.  That, and the Clodbuster the owner had sitting behind the counter.  I never got either one.  But the Amiga was the computer I wanted most, and I didn't fully understand why until this week, because this week AiOS learned the trick that made the Amiga special.  It runs two programs at once.

## KERNAL, with an A

First, a spelling lesson, courtesy of Commodore.

The Commodore 64's operating system was called the KERNAL.  With an A.  It started as a typo: one of Commodore's engineers, Robert Russell, misspelled "kernel" in his notebooks in 1980, the technical writers used his notes for the VIC-20 manual, and the misspelling stuck for good.

I never actually used a Commodore 64.  I'm a big fan of them now, and that's entirely down to David Murray, The 8-Bit Guy.  If you've never watched his videos, go and lose an evening to them.  You'll come out loving machines you never owned, like I did.

Here's what got me.  The KERNAL was a table of fixed addresses.  To print a character, a program jumped to address $FFD2, and Commodore kept those addresses where they were across their machines, so old programs never broke.  That is exactly what we've been building since part 1.  Our door is a table of entries a program calls, and the rule is that no entry ever moves.

So until this week, AiOS had a KERNAL.  A table of services and one program.  This week it got a kernel, with an E.  And I think the difference between the two spellings is the difference that matters: a kernel shares the machine.  The moment a second program shows up, something has to decide who gets the processor, and keep the two out of each other's memory.

## Polite, or the Amiga way

There are two ways to share a processor, and both have history.

The polite way is cooperative: each program runs until it decides to hand the processor over.  That's how Windows 3.1 and the classic Mac did it.  It works beautifully, right up until one program doesn't feel like handing it over, and then everything freezes.  If you used Windows 3.1, you remember.

The other way is preemptive: a timer interrupts whoever is running, every so often, and the system takes the processor back whether the program likes it or not.  The Amiga shipped with that in 1985.  Its kernel was called Exec, written by Carl Sassenrath, and it ran on a home computer with 256 KB of memory.  That's what I was drooling over, without knowing it.

Our new rule says pick the one with the history, and both had some.  I picked the Amiga.

## The rude program

To prove sharing works, you need a program that refuses to share.  The AI suggested one with the perfect pedigree: the sieve of Eratosthenes, exactly as BYTE magazine printed it in September 1981 as a benchmark.  Jim Gilbreath wrote it to compare programming languages, and it soon became the way people compared the home computers themselves.  8,191 flags, 1,899 primes.  Ours runs it 800 times flat out and never once calls the door while it works.  It never gives the processor back.

Then we ran it next to the stopwatch from part 9.

The stopwatch didn't miss a second.  The sieve hogged the processor for about five seconds, and the whole time the clock kept ticking: 00:01, 00:02, 00:03, every second on time.  Then the sieve reported its 1,899 primes, and the stopwatch kept going.  Two programs, one processor, neither one waiting for the other's permission.

## How it actually takes turns

A hundred times a second, the timer chip from part 9 interrupts the processor.  At that moment the kernel saves everything about the program that was running (where it was, every register) on that program's own stack, picks the next program, restores everything about that one, and lets it carry on as if nothing happened.  Neither program knows.  The stopwatch is the same binary we built in part 9 for loader 4, untouched.

The 100 comes from Linux too: it's the tick rate Linux reports to programs.

One bit of history fought back.  The interrupt controller is a chip design from the original IBM PC, and a modern processor ignores it until it's told to listen: you have to set up what Intel calls "virtual wire" mode, so the old chip's signals pass through the new processor's interrupt hardware.  For a while the timer was ticking away and the processor heard nothing.  Linux has the handful of lines that fix it, and now so do we.

## Breaking it on purpose, twice

Sabotage one: we switched the timer interrupt off.  Instantly AiOS became a cooperative machine.  The stopwatch, first in line, never handed the processor over, and the sieve never ran at all.  Windows 3.1, rebuilt in one deleted line.  The test caught it.

Sabotage two is the one I like best, because it's an attack from inside.  The Amiga had no memory protection.  Any program could write anywhere, which is why one buggy program could take down the whole machine.  Ours gives each program its own private memory: its own 8 MB, mapped so that it can't even see anyone else's.  So the AI wrote a vandal: a tiny program that runs next to the stopwatch and tries to write one byte just past the end of its own memory.

The processor refuses, the kernel stops the vandal, and the stopwatch carries on ticking.  And because I couldn't resist, the kernel announces it the way the Amiga did:

`Guru Meditation #0000000E.00C00000  program 2 stopped`

The first number is the kind of error (0E is a page fault: touching memory you don't own), and the second is the address it tried.  The name, by the way, came from an in-house joke at Amiga: a game where you sat cross-legged on a joystick board like a guru and tried not to move.  Move, and you got a "guru meditation" error.

## The hole I'm leaving open

Honesty time.  The programs still run with the same power as the kernel itself.  A sufficiently nasty program could still scribble on the kernel's own memory, or switch the memory protection off entirely.  Closing that properly means running programs with less power than the kernel, and a program with less power can't call our door the way it does now.  The door would have to become a gate.  That's a change to the one thing that's never changed since part 1, so it gets its own rung, and its own decision.

## What I'm taking away

The lines taken from Linux went from 638 to 844: the interrupt controller setup, the timer tick, and the code Linux uses to read a cpio archive, which is how we hand more than one program to the machine at boot.  The kernel itself, the bit that saves one program and resumes the next, is about 280 lines we wrote ourselves.

All up: ten new tests, thirteen new sabotages, all thirteen caught.  Every program on the ladder still runs byte for byte the same on the new loader.  Given one program, it doesn't even switch the timer on.

And the piece of the process this rung added: a test that attacks from inside.  The vandal is a hostile program sitting right next to the one it's trying to hurt, written by the same AI that wrote the kernel, trying its best to break the promise the kernel makes.  If the promise holds against that, I believe it.

## Read along

1. [pool/CMP.loader@6/loader.c](https://github.com/cmcluckie/aios/blob/main/pool/CMP.loader@6/loader.c).  The kernel.  Read the comment at the top, then find `kernel_trap`: the tick, the switch and the Guru Meditation are all in there, about forty lines.
2. [pool/CMP.loader@6/isr.S](https://github.com/cmcluckie/aios/blob/main/pool/CMP.loader@6/isr.S).  The moment of switching, in assembly: save every register, swap the stack and the memory map, restore every register.
3. [pool/CMP.sieve@1/sieve.c](https://github.com/cmcluckie/aios/blob/main/pool/CMP.sieve@1/sieve.c).  BYTE's 1981 sieve, the rude program.  Notice there's no door call inside the loop.
4. [lifted/CMP.irq.pic@1/pic.c](https://github.com/cmcluckie/aios/blob/main/lifted/CMP.irq.pic@1/pic.c).  The interrupt controller setup, from Linux, and the four lines of "virtual wire" at the bottom of `pic_init`.
5. [tests/programs/vandal.c](https://github.com/cmcluckie/aios/blob/main/tests/programs/vandal.c).  The vandal.  A dozen lines of bad intentions.
6. [findings/0011.md](https://github.com/cmcluckie/aios/blob/main/findings/0011.md).  What we found, including the hole that's left.

## Try this

Press F5 and pick AiOS: share.  Watch the stopwatch keep time while the sieve works, until the sieve reports its 1,899 primes.  Then build Windows 3.1: open pool/CMP.loader@6/loader.c, find the line `pic_unmask(PIT_IRQ);`, delete it, and run it again.  The stopwatch still counts, but the sieve never gets a turn.  Put the line back.

## As built

- [loader@6](https://github.com/cmcluckie/aios/blob/main/index/cmp/loader@6.as-built.md)
- [irq.pic@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/irq.pic@1.as-built.md)
- [timer.pit-tick@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/timer.pit-tick@1.as-built.md)
- [archive.cpio@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/archive.cpio@1.as-built.md)
- [sieve@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/sieve@1.as-built.md)
- [machine.serial-irq@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/machine.serial-irq@1.as-built.md)

## Addendum: the tests

Every test for the parts above, from tests/results.txt and tests/mutations-results.txt.  All of them ran under software emulation, not on the jig.

ADDENDUM

## Code on GitHub

Every line that changed, on one page (hash when committed).

The commits behind this post, in order:

1. Petition 0006: two programs at once; the KERNAL becomes a kernel (hash when committed)

[The whole repo](https://github.com/cmcluckie/aios).
