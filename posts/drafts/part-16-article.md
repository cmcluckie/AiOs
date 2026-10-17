*The story behind AiOs, part 16.*

## Play authentic

Remember Gibson's "play authentic" video?  2019, the one they pulled after the outcry.  If you play guitar, you just cringed.  If you don't, [Guitar World has the story](https://www.guitarworld.com/news/gibson-pulls-controversial-youtube-video-following-outcry).  Hold that cringe, I'm coming back to it.

## Why is this weirdo doing this?

That's the question, and it's fair.  Linux has been around forever, everybody trusts it, and nobody needs a better one.  That's not why I'm doing this.

One, I needed a creative outlet.  Two, I've wanted to do a creative blog series like this for a long time.  Three, I want to show how far you can actually go with AI, so I picked the most complicated project I could think of.

And I gave myself a rule.  The posts go up as fast as possible, they're written with AI, and they have to be me.  The first six went out a day apart, and most of the series was written in about two days and scheduled, so the rest is already on GitHub if you want to read ahead.  I sign off that I contributed 90% of the articles.  The articles, not the code.  Claude writes the code.  The articles are mostly me dictating and Claude editing.

## Written by AI, big surprise, still me

So the secret's out: I'm using AI to write these.  Are they still me?  Yes.  My voice, my tone, my stupid sense of humour.  Not fake.

Here's where the cringe comes back.  Every trade has people saying "we don't need it, I'm against it," and they're half right, because there are two ways to use AI.  The inauthentic way is "AI, write this," and people pick that out in about two sentences.  The other way is to change how you think about it.  You give it as much information and as much of your personality as you possibly can, and you stay in the room the whole time.  As Gibson says, play authentic.  Code authentic.  It goes for writing code and writing articles exactly the same.

At some point I'll prove it.  One post in this series will be written the inauthentic way: a separate Claude, a synopsis, "write a blog," slipped in without saying which one.  Yes, that one breaks my own rule.  That's the point.  If anybody's reading, I'd love to see who finds it.

## Who's ever unit tested hello world?

Here's the bigger trust thing.  Don't write the code, write the spec.  On this project I'm telling myself not to look at the code, and so far I haven't looked at a single program besides hello world.  The only screens I've looked at are the passes and fails.

You're thinking: Chris, you're crazy, you need to look at the application.  Fine.  Tell me who's ever written a unit test for a hello world program, and then come back and ask me that.  The one program I did look at is the one nobody bothers testing.

I haven't looked on purpose.  That's the whole point: to get the real speed out of these tools, we have to trust them with some of this.  Should you go as far as I have?  Absolutely not.  Can you trust it a little more than you do, with good governance and good QA around it?  Absolutely.  People make mistakes too, me included, plenty of them.  That's why we invented reviews and tests in the first place.

## I recorded my band without listening

A lot of musicians are scared of this.  I'm not.  I think AI could completely change how music gets written, refined, edited and marketed.

My band just started advertising, and we needed to record a practice.  Instead of a sound guy, I gave Claude a spec of what I wanted it to sound like, running on my studio MacBook.  During the whole practice, without listening to the music, only the levels, it kept making adjustments to get everything where the spec said.  I couldn't add anything once we started.  That was a huge trust thing for me.  It came out sounding really freaking good.  I'm not sharing it here, it's not a business thing, but if you want to find it, it's out there.

## Then I realised I'd never seen it

And that brings me to this part.  I've been purposely not looking at the output, because I trusted the tests.  And everything goes to a serial port anyway.  I can watch it scroll by in a terminal, but nothing is really being shown to a person.

Claude pointed out I could have run every program myself any time, and I never had, because the tests were green.  Claude's line, not mine: looking is the test you write when you can't write a test.  Anything that bugs me when I actually use it is a rule nobody wrote down.

So this part brings in the monitor.  But before Claude got to the screen, it found something that made me grumpy.

## Why would you even think that's close?

The plan said each kernel gets built from exactly the parts its programs' petitions (their written lists of what they need) ask for.  Claude came back with a question: what about the sort, the sieve and the editor, which the referee (the checker with no AI in it that reads every petition) had been failing since part 15?  So I asked why.  Since part 10, every new kernel had been built by copying the one before and adding one more part.  The editor's kernel had the stopwatch's clocks in it.  The sieve's had the disk code.  Nobody asked for any of that.  It just piled up.

My reaction, more or less word for word: if a kernel gets built for a set of applications, it has the code their petitions asked for.  Delete an application and its parts go with it.  Period.  Why would you even think that's close to what we should have?

Claude's answer was honest.  It had read "petitions add, never swap" as "each kernel stacks on the last one."  The rule was about petitions.  Nobody decided the bundle.  It grew, one reasonable step at a time, which is how every bundle I've ever seen in a real product got there.

So that got fixed first.  One kernel source now, every part behind a switch, and the builder turns on only what the petitions name.  The editor's kernel went from 19,704 bytes to 11,372 (measured on Claude's machine, not the jig, the machine everything really runs on).  The referee went from four failures to none.  And a new test opens every kernel and checks that every function in it belongs to a part somebody asked for.  Put the bundle back on purpose and it goes red.

## The monitor

Now the fun part: Conway's Game of Life, on a screen, in colour.

The screen is VGA text mode, 80 columns by 25 rows, which every PC has had since IBM's PS/2 in 1987.  Each spot on the screen is two bytes in memory: the letter and its colour.  Write to that memory and the letter appears.

Here's the part I love.  The code that does it is Linux's, from the same file the serial driver came from in part 1.  Linux uses it to print before anything else in the kernel is ready, and it prints everything light grey on black, forever.  Claude changed exactly one thing: the colour comes from the program now.  The table that turns a terminal's colour numbers into the screen's is Linux's too, from a file that still says "Copyright (C) 1991, 1992 Linus Torvalds" at the top.  ANSI calls red 1.  VGA calls it 4.  Somebody had to write that table down, and he did.

## A screen needs a BIOS

The first try showed nothing.  QEMU (the program that pretends to be a PC) just said "Guest has not initialized the display (yet)."  A VGA card comes up blank.  On a real PC, the card's own little BIOS runs at power-on and puts it in text mode.  Linux never has to, because the screen's already set up by the time it runs.  Our machines boot through a tiny firmware that skips all that.  So the screen machine boots through SeaBIOS, the open source PC BIOS that comes with QEMU, and the card's BIOS does exactly what it did in 1987.  The kernel just writes letters.

## The first picture was double-spaced

The first time Life actually drew, every row had a blank row under it, and the top line had scrolled off the screen.

Linux's early screen code jumps to the next line the instant the 80th character lands.  Then the newline at the end of the row jumps again.  Two lines for one.  Real terminals have waited politely since DEC's VT100 in 1978.  The serial terminal got it right.  The screen didn't.

The fix is in the kernel's console, not in Linux's lines: if the line already ended at the edge, the screen doesn't get told about the newline.  And that bug is the point.  No test caught it, because the test didn't exist yet.  Looking caught it.

## The tester gets eyes

Then Claude wrote the test.

Every test until now read bytes off the serial port.  This one takes QEMU's picture of the screen and checks it pixel by pixel, cell by cell, all 2,000 of them: the background colour, whether anything is drawn, and what colour it's drawn in.

Here's why that matters.  Swap Linux's colour table for one that uses the terminal numbers as VGA numbers, and every byte on the serial port is still exactly right.  The serial test passes.  The blue dots come out red.  Only the test with eyes goes red.

## Two things the referee missed

Rule 12 is the referee's check that a program never touches the hardware directly.  Turns out it had been passing programs it couldn't even read.  It looked at nothing and called that clean.  Now it refuses what it can't read.

And the screen opened a hole rule 12 can't see.  To the processor, the screen is just memory, so a program can write straight to it without a single forbidden instruction.  There's a file in the repo that does exactly that, the screen cheat, and the referee passes it.  The real fix is running programs with less power than the kernel.  That's a later rung, and it's written down.

## What I'm taking away

The first half of this post says don't look.  The second half is a bug only looking caught.  Both are true.  The tests caught everything they were written for.  The double-spaced screen was caught by somebody looking at it, and then it became a test so nobody has to look again.  That's the order it should go in.

And the bundle: when something grows one reasonable step at a time, nobody decided it, and nobody notices until something asks "why is that there?"  Here, the referee asked.

Code authentic still means staying in the room.  Sometimes that means looking.

## Read along

1. [pool/PART.loader@8/loader.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.loader@8/loader.c).  The kernel built from the petition.  Find `HAVE_`: every switch.
2. [lifted/PART.screen.vga@1/vga.c](https://github.com/cmcluckie/aios/blob/main/lifted/PART.screen.vga@1/vga.c).  Linux's screen code, and the one line Claude changed.
3. [pool/PART.life@2/life.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.life@2/life.c).  Life in colour.  Find `BORN`.
4. [tests/programs/screen.sh](https://github.com/cmcluckie/aios/blob/main/tests/programs/screen.sh).  The test with eyes.
5. [tests/programs/screencheat.c](https://github.com/cmcluckie/aios/blob/main/tests/programs/screencheat.c).  The screen cheat: the one rule 12 can't see.
6. [findings/0018.md](https://github.com/cmcluckie/aios/blob/main/findings/0018.md).  The bundle, and how it grew.
7. [findings/0019.md](https://github.com/cmcluckie/aios/blob/main/findings/0019.md).  The screen: the BIOS, the double spacing, the cursor, the cheat.

## Try this

Open the repo in Codespaces and run `sh tests/programs/partition.sh`.  It builds every kernel and names the parts in each.  Then run `AIOS_SHOT=life.ppm sh tests/programs/screen.sh tests/expected/life.screen@1.cells` and open life.ppm: that's what the test saw.

## As built

- [loader@8](https://github.com/cmcluckie/aios/blob/main/index/part/loader@8.as-built.md)
- [share@1](https://github.com/cmcluckie/aios/blob/main/index/part/share@1.as-built.md)
- [screen.vga@1](https://github.com/cmcluckie/aios/blob/main/index/part/screen.vga@1.as-built.md)
- [life@2](https://github.com/cmcluckie/aios/blob/main/index/part/life@2.as-built.md)
- [serial-vga@1](https://github.com/cmcluckie/aios/blob/main/index/machine/serial-vga@1.as-built.md)

## Addendum: the tests

Every test for this post, from tests/results.txt and tests/mutations-results.txt, run on the jig under KVM at commit cf303c1.  The whole suite: 63 passed, 0 failed, 5 skipped.  Of 104 deliberate breakages, 102 were caught; the 2 that survive are the known ones from parts 10 and 12, and all 13 of this post's were caught.  The referee's own file, tests/referee-results.txt: 80 lines, 0 FAIL.

- [loader.partition@1](https://github.com/cmcluckie/aios/blob/main/tests/loader.partition@1.toml).  Checks: every kernel built from the new source holds exactly the parts its petitions name.  Passed: 4 flavors on PART.loader@8, each kernel exactly its petitions' parts.  Mutations caught: 2.
- [loader.ladder-8-editor@1](https://github.com/cmcluckie/aios/blob/main/tests/loader.ladder-8-editor@1.toml), [-sort@1](https://github.com/cmcluckie/aios/blob/main/tests/loader.ladder-8-sort@1.toml), [-share@1](https://github.com/cmcluckie/aios/blob/main/tests/loader.ladder-8-share@1.toml), [-screen@1](https://github.com/cmcluckie/aios/blob/main/tests/loader.ladder-8-screen@1.toml).  Checks: hello, Hanoi, Life and the calculator, built for their own kernels, run unchanged on each new kernel.  Passed: 4 programs byte-identical under loader@8, all four times.  Mutations caught: 4.
- [life.colour@1](https://github.com/cmcluckie/aios/blob/main/tests/life.colour@1.toml).  Checks: every byte Life in colour writes, against a reference written separately.  Passed: 3075 bytes, exact.  Mutations caught: 2.
- [life.screen@1](https://github.com/cmcluckie/aios/blob/main/tests/life.screen@1.toml).  Checks: QEMU's picture of the screen, all 2,000 cells.  Passed: 2000 cells, 1947 drawn, every one where it should be, in its colours.  Mutations caught: 2.
- [vga.plain@1](https://github.com/cmcluckie/aios/blob/main/tests/vga.plain@1.toml).  Checks: thirty plain lines scroll the way Linux's early console scrolls them.  Passed: 2000 cells, 146 drawn, every one where it should be, in its colours.  Mutations caught: 1.
- [machine.vga-only@1](https://github.com/cmcluckie/aios/blob/main/tests/machine.vga-only@1.toml).  Checks: the screen machine has a serial port, a VGA card, and nothing nobody asked for.  Passed: 15 devices: the serial port, the VGA card, and what microvm forces.  Mutations caught: 1.
- [machine.vga-no-sockets@1](https://github.com/cmcluckie/aios/blob/main/tests/machine.vga-no-sockets@1.toml).  Checks: the screen machine never touches the network.  Passed: 38281 syscalls traced, none on the network.  No mutation written for it.
- [referee.no-hardware@1](https://github.com/cmcluckie/aios/blob/main/tests/referee.no-hardware@1.toml).  Checks: a program that touches the hardware is refused, and so is one the referee can't read.  Passed: the cheat printed hello's 14 bytes exactly, so hello's test would pass it; the referee refused it under rule 12, and refused a program it could not read.  Mutations caught: 2, one of them new.

## Code on GitHub

[Every line that changed, on one page](https://github.com/cmcluckie/aios/compare/a1dd78f...cf303c1).

The commits behind this post, in order:

1. [The kernel built from the petition; the screen; rule 12 fails closed](https://github.com/cmcluckie/aios/commit/cf303c1)

[The whole repo](https://github.com/cmcluckie/aios).
