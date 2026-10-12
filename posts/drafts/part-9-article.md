*The story behind AiOS, part 9.*

## The turbo button

I'll be honest: I don't like modern video games much.  I love the old ones.  And there's one thing about the old ones that drove me up the wall as a kid.  Take a game that played perfectly on one computer, put it on a faster one, and it ran at double speed.  Everything flew across the screen and the game was unplayable.  PCs even came with a button for it, labelled Turbo.  Turbo on, fast machine.  Turbo off, slow it back down so your games behaved.

I could never understand how programmers made that mistake.  This part is where I finally found out, because we nearly made it ourselves.

Here's what was going on.  A lot of those games didn't have a clock.  To wait a second, they ran a loop a certain number of times, tuned on the machine they were written on.  On the original IBM PC at 4.77 MHz, some particular number of trips round the loop came out at about a second.  On a faster machine the same loop finished sooner, so the "second" shrank, and the whole game sped up with it.  The turbo button existed because of that mistake: switch it off and the PC slowed itself down, closer to the speed the old games expected.

I could never play my favourite games on my friend's faster computer.  This week we made sure AiOS never has that problem.

## Why a stopwatch

The plan said the next rung was "Fibonacci, timed".  Work out some Fibonacci numbers the slow, recursive way and report how long it took.  I'd already done recursion in part 3, and a number printed at the end of a calculation is a dull way to show off a clock.  So we swapped it.  The next rung is a stopwatch.  It shows 00:00, then 00:01, then 00:02, redrawn on the same line, once a second, until you switch the machine off.  That's it.

It needs exactly one new thing from the machine: time passing.  Not the time of day.  A stopwatch doesn't care whether it's 10 o'clock; it only cares how long it's been.  A real clock on the screen needs to know what time it actually is, which is a different problem, and it gets its own rung later.

## A clock with no markings

Every modern processor has a counter built in that ticks at a fixed rate, billions of times a second, and doesn't slow down when the processor does.  It's exactly what you want for a stopwatch.  With one catch.

It counts ticks, not seconds.  It's a stopwatch with no markings on the dial.  To turn ticks into seconds you have to know how fast it ticks, and if you just assume a number, congratulations: you've rebuilt the turbo bug.

So the AI went and asked the processor.  Modern processors can tell you their counter's speed, and the hypervisor that runs a virtual machine can usually tell you too.  Ours said nothing.  Under the emulator we run on for now, the answer is blank, and the emulator's own little information block literally signs itself "TCGTCGTCGTCG" and leaves the speed empty.  The counter ticks.  Nobody will say how fast.

## Letting a 1981 chip back in

When nobody will tell you how fast your clock ticks, you time it against one you trust.  On a PC, the one everybody trusts is the timer chip, the 8254 (an 8253 in the very first machines), which has ticked about 1,193,182 times a second since the original IBM PC in 1981.  Count the processor's ticks while the old chip counts a known stretch, divide, and you know how fast the processor's counter runs.

That's how Linux does it when it has to.  To be fair to Linux, modern Linux asks the processor first, then the hypervisor, and only falls back to the old chip when nobody answers.  We went straight to the old chip on purpose.  It's what our machine needs, and honestly, I just love the story: a 1981 chip brought back to stop a 1985 bug.

There was a catch here too.  At the start of this project we switched every timer chip off, because nothing had asked for one.  So the machine got its first new device since day one.  It's a new machine definition with exactly one change: the 8254 is back.  It's wired to nothing, so it can never interrupt anything.  At boot, the loader spends about 50 milliseconds measuring the processor's counter against it, and then never touches it again.  The code that does the measuring is Linux's, lifted line by line: it watches the old chip count down and notes the processor's counter at each step until the error is under 0.05%.  That brings the lines we've taken from Linux to 172.

## The bit that didn't work first time

The first stopwatch I watched worked beautifully.  The next one sat at 00:00 forever.

Linux's measuring code is fussy, on purpose.  If the readings aren't steady enough to trust, it refuses to answer rather than guess.  Under the emulator, about one try in three isn't steady enough: the host computer glances away for a moment, or the emulator is still warming up on the first try, and the code says "not sure".  So the loader got a measurement of zero, the clock never moved, and neither did the stopwatch.

I actually love that it failed that way.  It never gave a wrong answer, just no answer.  Every time it did answer, it said the same thing, 2.8 billion ticks a second, to within 0.03%.  Real Linux would fall back to a slower method here, but that one needs a bit of the old PC hardware our machine doesn't have.  So the loader simply tries up to five times.  The stopwatch hasn't hung since.

## Three rules against the turbo button

So how do we make sure time is real, and not just "however fast this machine happens to be"?  Three rules, and the code follows all of them.

Never time anything by counting your own work.  That was the old games' mistake.  The stopwatch never counts loops; it asks the loader what time it is.

Work out time by subtraction, never by adding up.  The wrong way is "every time round the loop, add a second".  Any time a trip runs late, the error stays in the total and the clock drifts.  The right way is to note the counter at the start, and always show now minus start.  A late redraw just shows the right time a moment later.  Each tick of the stopwatch is aimed at "start plus n seconds", never at "a second after the last one".

And programs never see ticks.  The new door entry gives programs milliseconds since the loader started the clock.  The conversion from ticks happens in one place, the loader, once.  A program can't assume how fast a tick is, because it never gets a tick.

## Breaking it the 1985 way

You already know about the sabotage script from part 8.  This time we used it to put the turbo bug back in, on purpose, every way we could think of.  We made the stopwatch time itself by counting its own loops, exactly like the old games.  We had the loader assume the counter ticks at 1 GHz instead of measuring it, which is the turbo button in a single line.  We told it the old chip ticks at a round 1 MHz.  We took the chip out of the machine.

The loop-counting one is my favourite.  Left to run for ten real seconds on the same machine, it showed 04:59.  Thirty times too fast.  Turbo button, meet AiOS.

Every one of them got caught.  The test watches the stopwatch for ten seconds by the computer's own clock and checks it showed every second once, in order, and kept up.  Another test checks the loader's clock against the host's for ten seconds.  Over those ten seconds, the honest version drifts by somewhere between 0 and 4 milliseconds.

## The fine print

This all ran under software emulation, not on the jig and not on real virtualisation.  On real hardware the processor will probably just tell us its speed, and this whole dance becomes the fallback, like it is in Linux.

The stopwatch waits by asking the clock "is it time yet?" over and over, flat out, as fast as the processor can ask.  That keeps the processor 100% busy doing nothing.  On a one-program machine that's fine, because nothing else wants the processor.  The day two programs share it, it won't be, and that's the first honest argument for interrupts.  That's a rung for another day.

And the stopwatch never stops.  It's a stopwatch; you switch it off.  So its tests can't wait for it to finish.  They watch it for exactly ten seconds instead.

## Read along

1. [pool/CMP.stopwatch@1/stopwatch.c](https://github.com/cmcluckie/aios/blob/main/pool/CMP.stopwatch@1/stopwatch.c).  The whole stopwatch, about twenty lines.  Find the `while` that aims at `n * 1000`: that's rule two.
2. [pool/CMP.loader@4/loader.c](https://github.com/cmcluckie/aios/blob/main/pool/CMP.loader@4/loader.c).  Look for `door_ms` and the five tries at boot.  One subtraction, one division.
3. [lifted/CMP.timer.tsc-pit@1/tsccal.c](https://github.com/cmcluckie/aios/blob/main/lifted/CMP.timer.tsc-pit@1/tsccal.c).  Linux's measuring code.  Read the long comment above `pit_verify_msb`: that's Linux explaining why it refuses to guess.
4. [pool/CMP.machine.serial-timer@1/machine.sh](https://github.com/cmcluckie/aios/blob/main/pool/CMP.machine.serial-timer@1/machine.sh).  The new machine.  Put it next to the old one; the only real difference is `pit=on`.
5. [findings/0008.md](https://github.com/cmcluckie/aios/blob/main/findings/0008.md).  What we found, including the missing speaker port and the one-in-three failures.

## Try this

Press F5 and pick AiOS: stopwatch, and watch it count.  Then build yourself a turbo button: open pool/CMP.stopwatch@1/stopwatch.c, change `n * 1000` to `n * 500`, and run it again.  It now counts two "seconds" for every real one, which is exactly the bug from 1985.  Run `sh jig/test stopwatch` and watch the test catch you.

## As built

- [loader@4](https://github.com/cmcluckie/aios/blob/main/index/cmp/loader@4.as-built.md)
- [timer.tsc-pit@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/timer.tsc-pit@1.as-built.md)
- [stopwatch@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/stopwatch@1.as-built.md)
- [machine.serial-timer@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/machine.serial-timer@1.as-built.md)

## Addendum: the tests

Every test for the parts above, from tests/results.txt and tests/mutations-results.txt.  All of them ran under software emulation, not on the jig.

ADDENDUM

## Code on GitHub

[Every line that changed, on one page](https://github.com/cmcluckie/aios/compare/a61e96a...2876c14).

The commits behind this post, in order:

1. [Petition 0004: a stopwatch, and the whole repository is GPL-2.0-only](https://github.com/cmcluckie/aios/commit/2876c14)

[The whole repo](https://github.com/cmcluckie/aios).
