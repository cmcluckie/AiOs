*The story behind AiOs, part 23.*

<!-- Title: Drag Race Series, Claude versus Local LLM, 2 of 3: One number -->

<!-- Draft by Claude, 2026-11-08.  Every number is from findings 0031 to 0036 and the run records and logs they name.  Questions for Chris are in comments like this one. -->

## Six reds

Last time, the 3090 sat its exam: three parts Claude and I had already lifted out of Linux by hand, the serial port, the timer and the screen, so we know what right looks like.  The director and builders beat the old bench 6 to 5.  The timer beat both of them.  Six runs, six reds.

The timer is the part that teaches AiOs how long a second is.  The processor has a counter that ticks billions of times a second, but nobody tells you exactly how fast.  So you time it against something you trust: the 8254 (an old Intel timer chip that every PC has carried since the 1980s, ticking a known 1,193,182 times a second).  Count both for a moment, divide, and now you know.  Every clock in AiOs hangs off that one number.  Get it wrong and every second is wrong.

The 3090 never got that far.  Run after run, the code it wrote didn't even turn into a program.  Six runs, hours each, and not one working timer.  Small model, hard part.  I was ready to write it off and move on to the sieve.

I'm going to be honest about the next bit, because it's the whole story.  I didn't figure out what was wrong.  I wouldn't have known where to look.  What I did was ask Claude the dumbest question in the room: why doesn't the code compile?

Nobody knew.  Not the 3090, not Claude, not me.

## The red pen

Claude went and looked.  It took the last file every builder had handed in, ten of them, and built each one itself.  Eight didn't build, and every time the compiler (the program that turns code into something the machine can run) said exactly why, down to the line.

Three tried to divide two really big numbers the ordinary way.  On a 32-bit processor that needs a little helper program that comes with a normal system, and AiOs doesn't have one.  Linux has carried a trick for that for decades, and our hand-written timer uses it.  Two spelled a piece of low-level code in a way our settings don't accept, and one put a number in the wrong place.  Two had warnings, and our build treats every warning as a failure, on purpose.

And our test threw all of that away.  All the 3090 ever got back was "build of stopwatch failed."  Not which line.  Not why.

It's like handing in an essay and getting back an F with no red pen marks.  You'd rewrite it, guess what was wrong, and probably make the same mistake again.  That's exactly what it did, over and over, until it ran out of tries.

So Claude gave it the red pen.  Now when a build fails, the 3090 gets the compiler's own words.

## Then the card died

Twice.  About an hour into a run, the graphics card crashed, and every question to the AI after that took twenty minutes instead of one.  The second time it took my screen with it, and I couldn't get in remotely.  Claude tried too, and Windows told it there was no screen at all.  Somebody had to walk over and hold the power button.

A 3090 working flat out for hours can spike its power draw.  Claude suggested capping it at 300 watts instead of 350.  It hasn't crashed since.

<!-- Chris: anything you want to add about that night?  Where you were when it went dark, what you thought it was. -->

## The test told the truth, then lied, then told the truth

With the red pen, the 3090 started building working programs.  Then it hit the next wall.  The test kept saying "the door said 0 throughout: the rate was never measured."  In plain English: the timer said no time had passed.  (The door is how a program asks AiOs for something.  Here, what time it is.)

Except most of those weren't zeros.  The 3090's code had crashed the machine in the first second, before it printed anything, and the test said the same thing it says when a timer runs fine for ten seconds and measures nothing.  So the 3090 went digging through Linux for a better way to read the clock, when the real problem was that its code was crashing.

Claude fixed the message.  Then Claude got it wrong.  Its new version told every timer that printed a single line that it had crashed.  But the test program only prints when the time moves, so a timer stuck at zero prints "0" once, and that's it.  Every honest zero was now being told it had died.  Claude had written the check without reading the program it was checking, and wrote that down in the findings with its own name on it.  Fixed again.

Part 22 was about a test that passed something it shouldn't have.  This time the test got pass and fail right, and the reason wrong.  For a person, a wrong reason is annoying.  For an AI, the reason is all it has to go on.

## What the builders were missing

Then Claude read what the builders were actually doing, every step, and three things jumped out.

They kept looking for the test.  They'd search Linux for the test's name, over and over, because Linux is the only thing they can read.  They wanted to know what they were being graded on, and nothing would show them.

Every builder started from scratch.  Builder 2 would get somewhere, then builder 3 would start over with a blank page and a paragraph of advice.  That was our rule from part 22, so no builder would be told about work that wasn't there.  It also threw away everything learned.

And the director, the AI that hands out the work, was making things up.  It told one builder a fact about the timer chip that isn't true.  The builder believed it.

So now the builder sees the test.  Each new builder starts from the last one's file, with what the tests said about it.  And the director got a new rule: don't state a fact about the hardware you can't back up.

Two out of three worked.  On the very next run the director told a builder not to assume the 8254 ticks at any particular speed.  It does.  That's the whole point of it.  You can't fix that with a sentence in the instructions.

## One number

The last run ended red, out of time.  The best file the 3090 wrote counted 34 seconds while the real clock counted 10.

Then Claude ran that same file on its own computer, in an emulator (software pretending to be a PC).  It passed.  Off by 1 millisecond in ten seconds.

Same code, two answers.  So what's different about my test machine, the jig?

This is the part I would never have found.  The jig is a Linux machine running inside another machine, inside my Windows box.  Every time the code asks the 8254 what time it is, the question goes through two layers of pretend hardware.  A pair of those reads costs about 66,000 to 129,000 of the processor's ticks on the jig, against about 850 in Claude's emulator.  Linux has a setting for how long the quick timing check is allowed to take: 50 milliseconds.  On a machine that slow to answer, 50 isn't enough.  When Claude and I lifted the timer by hand, we hit exactly this, and changed that one number to 150.

The 3090 copied Linux faithfully.  It kept the 50.  Nothing it could see told it the jig was slow.

So I ran its file on the jig both ways.  With 50: 34,000 milliseconds against 9,924.  With 150: 9,500 against 9,500.  Spot on.

<!-- Chris: your reaction when that came back green.  This is the moment. -->

<!-- The run where the 3090 is shown the jig's machine spec and asked to find the 150 itself goes here (findings 0037, out/find150.sh). -->

## What I'm taking away

The 3090 lifted the timer.  Right for a real PC, straight out of Linux, all of it.  What stood between it and green was one number that exists because of how my test machine is built, and nothing it could see told it so.

I want to be straight about who found what.  I didn't find the thrown-away error messages, the lying test, or the 150.  Claude did, by reading every file and every log.  My part was asking why, saying "stop" when a run was wasting the card, and deciding what we'd change.  That's the job I keep coming back to: not knowing the answer, but knowing which question to ask, and making sure the thing doing the work can see what it needs.

And that's the piece this part adds.  An AI can only get better at what it can see.  If the test throws away the reason, it guesses.  If the test gives it the wrong reason, it goes looking in the wrong place, very confidently.  If nobody tells it about the machine, it does the right thing for the wrong machine.  None of what moved it from six reds to one number was a smarter model.  It was telling it the truth.

That goes for people too.

Next: Dave's sieve.

Come along!

## Read along

1. [findings/0032.md](https://github.com/cmcluckie/aios/blob/main/findings/0032.md).  Why the timer never built: the ten last hand-ins, rebuilt, and what the compiler said about each.
2. [agent/mechanic/src/judge.rs](https://github.com/cmcluckie/aios/blob/main/agent/mechanic/src/judge.rs).  Search for `printed_before`: the whole red pen, about twenty lines.
3. [tests/expected/timer.rate@1.check](https://github.com/cmcluckie/aios/blob/main/tests/expected/timer.rate%401.check).  The timer's test, which now says "the machine printed nothing" when it means that.  [findings/0034.md](https://github.com/cmcluckie/aios/blob/main/findings/0034.md) is the version Claude got wrong.
4. [findings/0035.md](https://github.com/cmcluckie/aios/blob/main/findings/0035.md).  What the builders were missing, port 0x41 and all.
5. [runs/20261108T072726Z-timer-2-ollama-qwen3-8-27b/output.txt](https://github.com/cmcluckie/aios/blob/main/runs/20261108T072726Z-timer-2-ollama-qwen3-8-27b/output.txt).  The 3090's timer.  Search for `MAX_QUICK_PIT_MS`: the 50.
6. [lifted/PART.timer.tsc-pit@2/tsccal.c](https://github.com/cmcluckie/aios/blob/main/lifted/PART.timer.tsc-pit%402/tsccal.c).  Ours.  The comment at the top says why ours is 150.
7. [findings/0036.md](https://github.com/cmcluckie/aios/blob/main/findings/0036.md).  The jig, both ways.

## Try this

Open `lifted/PART.timer.tsc-pit@2/tsccal.c`, find `do_div(delta, i*256*1000);`, and change it to `delta /= i*256*1000;`.  That's dividing the normal way.  Then run `sh jig/test timer.rate`.  It fails with "build of stopwatch failed", and just before that line, the linker's own words: `undefined reference to '__udivdi3'`.  That's the helper a 32-bit processor needs for the big division, and the line the 3090 never got to see.  Put `do_div` back and it passes again.  Claude ran this under emulation.

## As built

- [TOOL.agent@1](https://github.com/cmcluckie/aios/blob/main/index/tool/agent%401.as-built.md): the director and builders.  A failed hand-in now carries the compiler's words; a lifter is shown the item's tests and starts from the last lifter's file.

## Addendum: the tests

<!-- Fill from tests/results.txt and tests/mutations-results.txt once the jig has run them at the post's last commit.  As of aaa9d05: TST.agent.lift@1 is skipped on the jig (no cargo) and passes in Claude's container, ten mutations, each checked there by hand; TST.timer.says@1 has not been run by the runner anywhere but its own probe, by hand, one mutation; TST.timer.rate@1 last passed on the jig at d515c84, before its message changed.  jig/mutate still counts a SKIP as KILLED (part 22's addendum), so the jig's KILLED for agent.lift means nothing yet. -->

## Code on GitHub

<!-- The compare from d8c30e9 to the post's last commit, then the commits from the ledger: d515c84, 883ea0f, b16b30b, b1e1f10, 5992997, aaa9d05, and the ones still to come. -->

