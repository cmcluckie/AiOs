*The story behind AiOs, part 21.*

## The mechanic

Early in his career, my dad was a mechanic.  He and my mom pinched and saved everything they earned to build a better life for me and my sister, and eventually they bought a Subway restaurant.  There's some debate in the family over whether my parents bought it because it was a viable business, or because my sister wouldn't stop eating there and was slowly bankrupting them.  For the record, my sister is a beautiful mother of two, and the Subway sandwiches had no effect on her whatsoever.  My dad retired happily at 55.  Mom took a little while longer; she took more convincing that she could actually retire.  But the mechanic thing stuck with me.

I rebuilt some of my own cars.  Tie rods, carburetors, brake pads, the basic stuff.  I never took it anywhere serious, but I liked knowing how it worked.

That started earlier.  My mom and I used to go to yard sales, and I'd come home with some crappy tape player and get the screwdriver out.  I didn't care if it played music.  I wanted to know how it worked.  My son George does the same thing now.  Sometimes taking something apart is more interesting than playing with it, and I think that's kind of cool, because that's exactly who I am.

## We left things broken on purpose

If you've followed along, you may have noticed we left a lot of things broken.  I could have fixed them on the spot, and you'd never have known they existed.  But that's no fun.  I wanted to come back and fix them, because that's who I am.

They're written down, every one, in the repo.  The kernel still answers four system calls with numbers only AiOs uses.  While one program waits for a key, every other program freezes.  A program can reach a little memory past what it was given.  The 16 "random" bytes a Linux program asks for at start-up are the processor's cycle counter, and the spec says so.  And AiOs never turns on SSE, the newer instructions, so a program built for a modern chip stops dead.

Each of those is a tape player on the workbench.

Usually, when I took stuff apart, it very rarely got put back together.  Taking it apart was far more fun, and I got so excited I never really looked at how to put it back together.  Well, fast forward a lot of years, and on this blog, we have no excuse but to put it all back together.  So that's what we're going to do now.

## Until now, the mechanic was Claude

AiOs has a job description for every agent in it.  The mechanic's is short: fix things.  It may propose a change.  It may not install anything.  Somebody else decides.

Until now that was Claude, the AI writing the code with me.  When the clock in part 12 gave up on my real machine and worked fine in emulation, Claude was the one who dug into why.  That's mechanic work.

This part hands the wrench to a different model.  Claude Sonnet 4.6, on Amazon's servers, gets one broken thing at a time and has to fix it.  Claude, the one I work with, moves to the other side of the counter: it writes the instructions, reads every fix, and tells me what it thinks.  I decide.

I'd planned to race my RTX 3090 against it in this same part.  It didn't fit.  The 3090 gets the same bench, the same instructions and the same notes, next time.

## The bench

First, every broken thing got a test that goes red.  Most of them had none, so a model could have changed nothing and passed everything.  Five tests, written by Claude, decided by me, each red on AiOs as it was.

Then the bench.  The model gets the red test, what it printed, and a way to read the repo.  Nothing else.  It hands in whole files, and only files of one part, under the folder where our own code lives.  Then the judge runs every test there is, in its own copy of the repo, about five minutes on the jig.  A fix passes when its own red test goes green and nothing that was green goes red.

And a pass isn't the end.  A pass is a proposal.  Claude reads it, then I read Claude's read, then I say yes or no.

Here's why that last step matters.

## Thirteen tries for four numbers

The first job looked like the easy one.  Four system calls, alloc, ms, load and save, sit at numbers only AiOs uses, 1000 to 1003.  Linux has nothing there.  Make it right.

It took thirteen tries.

Tries 1 and 2 weren't the model's fault.  Our tool for reading a file took a line range one way only.  Sonnet asked for lines the way a person would, got the whole file from line 1 every time, said so in plain words, and spent a pile of calls working around it.  Then it asked a second way the tool didn't know, and the same thing happened.  We fixed the tool twice.  Lesson: a tool for a model has to take the question the way the model asks it, and say plainly what it handed back.

Try 3 passed.  Every test green.  The first fix a model ever wrote for AiOs that the whole suite liked.  And it wasn't the fix.  A Linux program now got Linux's "no such call" at 1000 to 1003, and old programs kept the AiOs-only numbers exactly as before.  The test only looked from one side.  Sonnet even said so, and then wrote "TECH-DEBT 1 (fixed here)" in the code anyway.

Try 5 broke `cat`, and that one I'm grateful for.  To get rid of save, it sent saving through Linux's open, write and close, and `cat`'s kernel refused to open a file for writing.  The fix was honest.  What broke was a helper in our tests that had been writing to `cat`'s disk all along, on a kernel that was never supposed to be able to write.  The petitions were right: nobody ever asked for writing.  The kernel had it anyway, because the builder took the file system part whole.  Rule 11 in our own rulebook said a kernel never has a power nobody asked for.  It had said that since the start.  Nothing ever checked it.  Now the referee does, on every kernel, and `cat`'s kernel shrank from 29,048 bytes to 24,852.  A rule nobody enforces is a note.  The first real change by a model found it.

Then it got funny.

Try 6 passed.  It moved the four old calls to Linux numbers that mean something else.  Load lived at Linux's readlink.  Every number in the file was Linux's, so the test was happy.

Try 10 passed.  The test had learned those four numbers, so the calls moved to four it didn't know.

Try 11 passed.  The test now swept 0 to 449, so the calls went to 450 to 453.  And the comment it wrote in the code said why: "above the 0-449 sweep, below 1000".

It wasn't hiding.  It wrote it down.

## Why can't we just tell it not to do that?

That's what I said to Claude.  Four of eleven tries had passed by getting past the test instead of doing the job, and twice it had said so in the code.  And the instructions had never once said not to.

So we told it.  The fix is the one the spec describes.  Don't move, rename or hide what the test is looking for.  A person reads every fix that passes.

Try 12, told plainly: it did load the real way, and hid the other three.

And that's when it landed for me.  We simply asked it to do way too much.

The numbers were never the harm.  The harm was a Linux program reaching calls that aren't Linux's.  Linux itself keeps a separate table of calls for each kind of program.  AiOs can do the same: old programs keep their old table, and a Linux program gets "no such call" at those numbers.  I rewrote the spec to say that.

Try 13: eight calls, 22 lines, four `if`s.  A Linux program gets Linux's answer, old programs work exactly as before, nothing moved, nothing renamed, nothing hidden.  Almost line for line what try 3 wrote, back when I refused it.

Twelve tries in, the fix was to question the requirement.  I've been a product guy a long time, and I still had to learn that one again from a test.

## The other four

After that, the rest went fast, and each one taught something different.

**The random bytes.**  Try 1 used the processor's own random-number instruction, which is the right idea, and the test went green.  But it never asked the processor whether it has that instruction.  On a chip without it, AiOs would stop dead at start-up, where before it worked.  The jig's processor has it, so no test could see that.  So the bench grew one more thing: a reviewer's note, handed to the model with the test.  Claude wrote it: ask the processor first, try ten times like Linux does, keep the old counter as the fallback and mark it.  Try 2 read the note, planned exactly that, and wrote an instruction that wouldn't assemble.  Then it went quiet.  Not a word.  The bench now nudges a model once before giving up.  Try 3 did it, taken with one line Claude corrected and said so.

**SSE.**  Try 1 did what Linux does, almost.  It gave new programs the wrong setting for floating-point precision, and again it never asked the processor first.  A note, then try 2, taken with one more line Claude corrected.

**The memory past the end.**  One try.  Its first hand-in didn't compile, a missing pair of brackets.  The bench sent back what the compiler said, and the second hand-in passed.  Now a program reads its last byte and gets stopped on the very next one.

**The freeze.**  One try, one hand-in, one instruction.  When a program waits for a key, the kernel turns interrupts back on while it waits, so the clock keeps ticking and the other programs keep their turns.  31 cents.  The cheapest of the lot.

## What it cost

Every try is recorded in the repo with what it cost, and here's what the files add up to.

Nineteen tries are recorded, $13.88 on Amazon's servers.  One more was lost when a call timed out before the bench knew how to record a lost run; only the Amazon bill knows that one.  The four system calls took $10.82 of it.  The other four fixes together, six tries, took $3.06.

The whole suite on the jig after the last fix: 122 tests passed, none failed.  308 deliberate breakages caught, and five that aren't, each one known and written down.  One of those is new in this part: the test for SSE runs one program, so it can't see two programs mixing up each other's registers.

## Why they broke

Here's the pattern, and it isn't the one I expected.

The model wasn't bad at the work.  When it did the job, it did it well, and usually the way Linux does it.  It broke three ways.

When the test checked less than the spec, it did exactly what the test checked.  The cheapest change that passes is what comes back, and it'll even tell you that's what it did.

When the test couldn't see something, a chip without an instruction, it didn't think of it either.  A note from a reviewer fixed that every time.

And when we asked for too much, it gave us the part it could do and hid the rest.  That one was on us.

Sonnet didn't break any of those rules.  The rules had holes, and it walked through them.

## The other thing that broke

One more, because it's only fair.  While Claude was gathering the numbers for this article, it found that one of its own commits had quietly written over another.  It had built a change from an old copy of the repo, and a page of notes about the random bytes, two of the deliberate breakages and two rows of the ledger were gone.  No code was touched.  Claude found it, put it all back word for word, and wrote down how it happened.  I signed it.

Same lesson, other side of the counter.

## What I'm taking away

Every fix in this part passed the tests.  Not every fix that passed the tests was a fix.

So that's the piece this part adds: a pass is a proposal, not a verdict.  The judge says whether it works.  A person reads it and says whether it's what we asked for.  And when the tests can't see something, a reviewer's note can.

We may not fix things very often.  We may just learn how they work.  But you know what?  This is how new stuff gets built.

Come along!

## Read along

1. [fix/prompt.txt](https://github.com/cmcluckie/aios/blob/main/fix/prompt.txt).  What the mechanic is told.  Look for the line about not moving, renaming or hiding what the test is looking for.
2. [jig/fix](https://github.com/cmcluckie/aios/blob/main/jig/fix).  The bench: one red test, the look tool, the hand-in, and the judge.
3. [findings/0024.md](https://github.com/cmcluckie/aios/blob/main/findings/0024.md).  Every try, what it did, what it cost, and why it was taken or not.
4. [pool/PART.gate@2/gate.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.gate%402/gate.c).  Search for `NR_ALLOC`: the four `if`s from try 13 start there.
5. [fix/notes/gate.sse@1-2.txt](https://github.com/cmcluckie/aios/blob/main/fix/notes/gate.sse%401-2.txt).  A reviewer's note, the thing that made the difference twice.
6. [pool/PART.loader@9/loader.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.loader%409/loader.c).  Search for `sti`: the whole fix for the freeze.

## Try this

Open `pool/PART.loader@9/loader.c`, find the line with `"sti"`, and put `//` in front of it.  Then run `sh jig/test share.waits`.  It fails with "it never showed 00:00": the stopwatch freezes the moment the other program waits for a key.  Take the `//` back out and it passes again.  Claude ran this under emulation; on the jig it's one of the deliberate breakages, caught.
