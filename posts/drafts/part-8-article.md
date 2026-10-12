*The story behind AiOS, part 8.*

## Who's been checking the homework

Up until now, the testing on this project has been done by the AI.  It runs the program, looks at what comes out, compares it with what it should be, and tells me it passed.  That's fine for getting started.  It is not where we want to be.  The whole point of writing tests down is that something other than the author runs them, every time, and doesn't get tired or optimistic.

So this part is the testers.  A runner that boots every program and checks every byte, and a script that breaks the code on purpose to prove the tests would notice.  But to decide what to test, I first had to understand what the parts actually are.  And that's where the light bulb went on for me about what actually makes an operating system.

## Where's the kernel?

I kept asking the AI where the kernel was.  The honest answer is there isn't one yet.

It took me a few goes to get there, because I kept asking the question as if there had to be one.  What we have is a loader.  The firmware wakes the virtual machine up and hands control to the loader, and from then on the loader is the only thing in charge.  It sets the processor up, copies the program into memory, gives it a little table of things it's allowed to ask for (write, exit, memory, read), and runs it.  When the program says it's done, the loader shuts the machine off.

If you know a bit about how processors work (I know just enough to be dangerous), the processor is already in protected mode, the 32-bit mode every modern PC runs in.  The loader turns that on in its first few instructions.  What it doesn't do is the part that makes a kernel a kernel: run the program with less power than itself.  Right now the program and the loader have exactly the same keys to the building.  There's no scheduler, no interrupts, no protection.  With one program on the machine, none of that has a job yet.

It's a lot closer to the first operating systems on micro PCs than to Linux: a resident program directing traffic.  The old word for it is a monitor.  When a second program has to share the machine, that's when the kernel shows up (plan 9.6), and you'll be able to point at the exact commit.

And why does the loader do any of this?  Because we told it to.  Every program on AiOS has a petition, and every flavor of the system is built from the petitions for the programs it runs.  No program, no petition.  No petition, nothing gets built.

## How a byte gets out

Here's the bit I find really interesting.  The actual write, the code that puts a byte on the wire, lives in the serial driver, the lines we took from Linux.  The program never touches it.  Hello world hands its 14 bytes to the loader through the door.  The loader hands them to the driver.  The driver pokes them into the serial chip one at a time.

So the loader is the handoff point.  Today every write goes to one place, the serial port.  As this grows, that routing is where it gets interesting: a write could go to a screen, a file, a network, and the program wouldn't know or care.  That's what the door is for.  The day the loader starts deciding where a write goes, it's starting to become a kernel.

## Two kinds of tests

So what do you test?  Two things, really.

The driver on its own: does it put every byte on the wire, in order, untranslated, and does it take every byte off the wire it's handed?  That's the unit test.

Then everything together: boot the machine with the real loader, the real driver and the real program, type into it if it needs typing, capture every byte that comes out, and compare.  That's the integration test, and most of our tests are that shape.

One honest wrinkle.  The normal way to unit test a driver is to swap the real hardware for a fake one that records everything.  We can't do that without editing the Linux lines, and the lifted code doesn't get edited.  So instead, the runner asks the virtual machine to log every single read and write to the serial chip's registers while the real thing runs.  Same evidence, nobody touches the driver.

## The second opinion

For hello world you know the right answer by looking.  For Conway's Game of Life after 200 generations, you can't: it's 1,992 bytes of grid.  When I asked why the testing needed Python, this was the answer.  We need a second opinion.

Think of two people adding up the same restaurant bill, separately.  If they both get the same total, you trust it.  If they don't, somebody made a mistake and you go find it.  So for Life, Hanoi and the calculator there's a second version of each program, written in Python from the same rules, run on a normal computer.  Its answer is saved as a file.  The runner boots the real program and compares every byte against that file.  That's exactly how the calculator bug in part 6 got caught: the second opinion said "too big", ours said 1215752191.

The catch is that both people were the AI.  If it misread a rule, both versions carry the same mistake and happily agree.  That's evidence, not proof.  The saved answers are in the repo, along with the programs that made them, so anyone can check them against someone else's.

And the jig, the machine that builds AiOS, still doesn't need Python.  It only compares bytes.

## Breaking it on purpose

A test that has never failed proves nothing.  So the other half of this part is a script that breaks the code on purpose, one small change at a time, and checks the right test goes red.  Change "Hello" to "Jello".  Lose the newline.  Make hello never exit.  Delete the overflow check from the calculator again.  Copy the program 16 bytes off from where it should run.  Give the machine a device nobody asked for.  Make it open a network socket.

24 sabotages.  21 caught.  The three that weren't are the best part.

The serial driver normally waits for the chip to say "ready" before each byte.  Take that wait out and every single test still passes, because the virtual chip is always ready.  On a real chip, you'd lose bytes.  Nobody ever wrote down that the driver must wait, so nothing checks it.  Same story for the setup code that tells the chip to use 8-bit bytes.  The third one is a test that's honest but blind: the calculator only ever asks for one keystroke at a time, so a bug that only shows up when you ask for more than one byte sails past its test.  It's caught by a different test instead.

None of those are fixed in this part.  They're written down (findings 0007), and the first two need someone, me, to write the rule they break.  That's how it's supposed to work: the system tells you exactly where your safety net has holes.

## Where that leaves us

Every test we've written now runs with one command, and every one passes.  Most of them have been shown failing on purpose.  The "checked by hand" notes in the earlier parts' addenda were true when they were written; from here on, the addendum says what the runner says.

The fine print: all of this ran under software emulation in a container, not on the jig and not on real virtualisation.  The jig is still a thing I need to sit down and install.

If this is where things start to come together for you too, that makes two of us.

## Read along

1. [jig/test](https://github.com/cmcluckie/aios/blob/main/jig/test).  The runner.  The comment at the top explains every kind of test it knows.  Look for `trace`: that's the register log standing in for a fake chip.
2. [tests/results.txt](https://github.com/cmcluckie/aios/blob/main/tests/results.txt).  What the runner said, test by test, and what it ran on.
3. [tests/mutations.txt](https://github.com/cmcluckie/aios/blob/main/tests/mutations.txt).  Every sabotage, one per line, with what it breaks.  The "survives" lines are the holes.
4. [tests/fixtures/life_ref.py](https://github.com/cmcluckie/aios/blob/main/tests/fixtures/life_ref.py).  The second opinion for Life, in about 20 lines of Python.  Put it next to pool/CMP.life@1/life.c and compare.
5. [findings/0007.md](https://github.com/cmcluckie/aios/blob/main/findings/0007.md).  What the runner and the sabotage found, including the three survivors.

## Try this

Open pool/CMP.hello@1/hello.c and change "Hello" to "Jello".  In a terminal (the Codespaces one works), run `sh jig/test hello`.  You'll get FAIL, and it tells you the very first byte that's wrong.  Change it back and run it again.  Then try `sh jig/mutate hello` to watch the sabotage script do the same thing five ways.

## As built

No component changed in this part; it adds the tests around them.  These are the descriptions the tests check against:

- [loader@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/loader@1.as-built.md), [loader@2](https://github.com/cmcluckie/aios/blob/main/index/cmp/loader@2.as-built.md), [loader@3](https://github.com/cmcluckie/aios/blob/main/index/cmp/loader@3.as-built.md)
- [serial.8250@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/serial.8250@1.as-built.md), [serial.8250@2](https://github.com/cmcluckie/aios/blob/main/index/cmp/serial.8250@2.as-built.md)
- [machine.serial-only@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/machine.serial-only@1.as-built.md)
- [hello@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/hello@1.as-built.md), [hanoi@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/hanoi@1.as-built.md), [life@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/life@1.as-built.md), [calc@1](https://github.com/cmcluckie/aios/blob/main/index/cmp/calc@1.as-built.md)

## Addendum: the tests

Every test in the repo, from tests/results.txt and tests/mutations-results.txt.  All of them ran under software emulation, not on the jig.

ADDENDUM

## Code on GitHub

Every line that changed, on one page (hash when committed).

The commits behind this post, in order:

1. Tests run themselves: jig/test, jig/mutate, expected outputs and probes (hash when committed)

[The whole repo](https://github.com/cmcluckie/aios).
