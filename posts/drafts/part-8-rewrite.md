*The story behind AiOS, part 8.*

## Who's been marking the homework

Confession time, again.  Every "passed" in this series so far has come from the AI.  It built the program, booted it, looked at what came out, compared it with what it expected, and told me it was fine.  It was right every time.  It's also exactly the setup I'd never accept at work: the person who wrote the code marking their own homework, by eye, whenever they remember to.

So this part was supposed to be simple.  Write the testers.  One command that boots every program, checks every byte, and writes the result down.  Something that doesn't get tired, doesn't get optimistic, and doesn't care who wrote the code.

It turned into the part where I finally understood what an operating system actually is.  Because to decide what to test, I had to know what the pieces were.  And I couldn't get past one of them.

## Where's the kernel?

I kept asking the AI where the kernel was.  The honest answer is there isn't one yet.

I must have asked it a few different ways.  Where's the kernel?  So the loader is the kernel?  So it'll turn into Linux?  Each time it kept saying, more or less politely, that I was asking about something that didn't exist yet.  And slowly the penny dropped.  I'd been assuming an operating system starts with a kernel and everything else hangs off it.  This one doesn't.  It starts with a program that needs something, and the operating system is whatever has to exist to give it that thing.

What we actually have is the loader.  The firmware wakes the virtual machine up and hands control to it, and from then on the loader is the only thing in charge.  It sets up the processor, copies the program into memory, hands it a little table of things it's allowed to ask for (write, exit, memory and read) and lets it run.  When the program says it's done, the loader switches the machine off.

If you know a bit about how processors work (I know just enough to be dangerous), here's the detail I got wrong at first.  The processor is already in protected mode, the 32-bit mode every modern PC runs in.  The loader switches that on in its first handful of instructions.  What it doesn't do is the part that makes a kernel a kernel: run the program with less power than itself.  Right now the program and the loader hold exactly the same keys to the building.  No scheduler, no interrupts, no protection.  With one program on the machine, none of that has a job yet.

So it's not a kernel.  It's a traffic cop.  It's much closer to the first operating systems on the old micro PCs than to anything Linux does, and the old word for it is a monitor.  The kernel shows up when a second program has to share the machine (plan 9.6), and I'm looking forward to pointing at the exact commit where it happens.

And why does the traffic cop do any of this?  Because we told it to.  Every program on AiOS has a petition.  Every flavor of the system is built from the petitions for the programs it runs.  No program, no petition.  No petition, nothing gets built.  That's the whole idea in two sentences, and it took me eight posts to say it that simply.

## Where a byte actually goes

Here's the bit that really got me.  The write itself, the code that actually puts a byte on the wire, lives in the serial driver: the lines we took from Linux.  Hello world never touches it.  Hello world hands its 14 bytes to the loader through the door.  The loader hands them to the driver.  The driver pokes them into the serial chip, one at a time, waiting for the chip to say it's ready before each one.

So the loader is the handoff point.  Today every write goes to exactly one place.  But that's the bit that's going to get interesting.  A write could go to a screen, a file, a network, and the program wouldn't know or care, because all it ever does is knock on the door.  The day the loader starts deciding where a write goes is the day it starts turning into a kernel.  I love that you can see it coming from here.

## Two kinds of test, and a workaround

Once I could see the pieces, the testing made sense.  It's two things.

The driver on its own: does every byte handed to it come out on the wire, in order, untouched?  Does every byte that arrives get handed over once, none lost, none invented?  That's the unit test.

Then everything together: boot the real machine with the real loader, the real driver and the real program, type into it if it needs typing, capture every byte that comes out, and compare.  That's the integration test, and it's most of what we have.

There was one snag.  The normal way to unit test a driver is to swap the real chip for a fake one that writes down everything it's asked to do.  We can't do that without editing the Linux lines, and the lifted code doesn't get touched.  That's a rule.  So the AI found a way around it that I really like: it asks the virtual machine itself to log every single read and write to the chip's registers while the real thing runs.  Same evidence, real code, nobody touches the driver.

## The second opinion

This is where I got suspicious.  The AI said it needed Python for the tests.  Python isn't allowed on the jig, so I asked why.

The answer turned out to be completely reasonable.  For hello world you know the right answer by looking.  For Conway's Game of Life after 200 generations you can't: it's 1,992 bytes of grid, and nobody can eyeball whether one cell in the middle should be alive.  You need a second opinion.

Think of two people adding up the same restaurant bill, separately.  If they get the same total, you trust it.  If they don't, someone made a mistake and you go find it.  So for Hanoi, Life and the calculator there's a second version of each program, written in Python from the same rules, run once on a normal computer, with its answer saved as a file.  The runner boots the real thing and compares every byte against that file.  That's exactly how the calculator bug got caught in part 6: the second opinion said "too big", ours said 1215752191.

The catch, and I'd rather say it before someone else does: both people adding up the bill were the AI.  If it misread a rule, both versions make the same mistake and happily agree with each other.  So it's evidence, not proof.  The saved answers and the programs that made them are in the repo, so anyone can check them against someone else's.  And the jig still doesn't need Python, because all it does is compare bytes.

## Breaking it on purpose

There's a rule in this project that a test which has never failed proves nothing.  So the other half of this part is a script that breaks the code on purpose, one small change at a time, and checks the right test goes red.

Change "Hello" to "Jello".  Lose the newline.  Make hello never exit.  Delete the overflow check from the calculator again.  Copy the program 16 bytes off from where it should run.  Give the machine a device nobody asked for.  Make it open a network socket.

24 sabotages.  21 caught.  And the three that got away are honestly my favourite part of the whole thing.

The serial driver normally waits for the chip to say "ready" before every byte.  Take that wait out and every single test still passes, because the virtual chip is always ready.  On a real chip you'd quietly lose bytes.  Nobody ever wrote down that the driver has to wait, so nothing checks it.  The setup code that tells the chip to use 8-bit bytes is the same story.  The third is a test that's honest but blind: the calculator only ever asks for one keystroke at a time, so a bug that only shows up when you ask for more than one sails right past it.  A different test catches that one instead.

None of them are fixed in this part.  They're written down (findings 0007), and the first two are waiting on me to write the rule they break.  Which is exactly how it's supposed to work.  The system just told me where my safety net has holes, by name.

## What I'm taking away

Every test we've written now runs with one command, and every one passes.  All but one has been seen failing on purpose.  The "checked by hand" notes in the earlier addenda were true when they were written; from here on, the addendum says whatever the runner says.

The fine print: all of this ran under software emulation in a container, not on the jig and not on real virtualisation.  Installing the jig is still on my list.  I'm a dad.  It'll happen.

If this is where things start to come together for you too, that makes two of us.

