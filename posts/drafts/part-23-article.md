*The story behind AiOs, part 23.*

<!-- Draft by Claude, 2026-11-08.  Every number is from findings 0031 to 0036 and the run records and logs they name.  Questions for Chris are in comments like this one. -->

## One number

Last time, the race ended 6 to 5.  The director and builders beat the old bench, and nobody finished the timer.  Nine runs, nine reds.

The timer is the piece of AiOs that works out how fast the processor counts, by timing it against the PC's 8254, a timer chip whose design goes back to the original IBM PC.  Every clock in AiOs hangs off that one number.  If it's wrong, every second is wrong.

I was ready to write it off.  Small model, hard part, fair enough.  Then I asked Claude a simple question: why doesn't the code compile?

Nobody knew.  Not the 3090, not Claude, not me.

## The red pen

It took ten minutes to find out.  Claude rebuilt the last file each builder handed in, ten of them, and eight didn't build.  The compiler said exactly why every time.  Three divided two big numbers the normal way, which a 32-bit processor can't do in one step without a helper from a library AiOs doesn't have.  Linux has had a little trick for that for decades, and our hand-written timer uses it.  Two used a word for assembly our compiler settings don't take, and one put a value in the wrong register.  Two had warnings, and our build treats every warning as a failure, on purpose.

And our test threw all of that away.  All the 3090 ever got back was "build of stopwatch failed."

It's handing in an essay and getting back an F with no red pen marks.  You'd rewrite it, guess what was wrong, and probably make the same mistake again.  That's exactly what it did, until it ran out of tries.

So Claude gave it the red pen.  When a build fails, the 3090 now gets the compiler's own words.

## Then the card died

Twice.  About an hour into a run, Ollama said "CUDA error: unknown error," and every call after that took twenty minutes instead of one.  The second time it took the display with it, and I couldn't get in over Remote Desktop.  Claude couldn't either; it tried, and Windows reported there was no screen at all.  Somebody had to walk over and hold the power button.

A 3090 under a long, heavy load can spike its power.  We capped it at 300 watts instead of 350.  It hasn't crashed since.

<!-- Chris: anything you want to add about that night?  Where you were when it went dark, what you thought it was. -->

## The test told the truth, then lied, then told the truth

With the red pen, the 3090 started building.  Then it hit the next wall: "the door said 0 throughout: the rate was never measured."  (The door is how a program asks AiOs for something.  Here, what time it is.)

Except most of those weren't zeros.  The machine had crashed in the first second and printed nothing, and the test said the same thing it says when a timer runs for ten seconds and measures nothing.  So the 3090 went hunting through Linux for another way to read the clock, when its code was crashing the machine.

Claude fixed the message.  Then Claude got it wrong: its new check said a machine that printed one line had "stopped printing: died or hung."  But the test program only prints when the time moves.  A timer stuck at zero prints "0" once, and that's it.  So every honest zero was now being told it had crashed.  Claude had written its own test without reading the program it was testing, and said so in the findings.  Fixed again.

A test checks what you wrote down, not what you meant.  That was part 22.  This time the test was right about pass and fail, and wrong about why.  For a person that's annoying.  For an AI, the why is all it has.

## What the builders were missing

Then I read what the builders were actually doing, and three things jumped out.

They kept looking for the test.  "list .", "find stopwatch", "find timer.rate", all through the Linux reader, which is the only thing they can read.  They wanted to know what they were being graded on, and nothing would show them.

Every builder started from nothing.  Builder 2 would get somewhere, and builder 3 would start over with a blank file and a paragraph of advice.  That was our rule from part 22, so no builder would be told about changes that weren't there.  It also threw away everything learned.

And the director was making things up.  It told one builder that port 0x41 is "channel select."  It isn't.  The builder did what it was told.

So the builder now sees the test, and each new builder starts from the last one's file, with what the tests said about it.  And the director got a new rule: never state a fact about the hardware you can't quote.

Two out of three worked.  On the next run the director told a builder not to assume the 8254 runs at a particular frequency.  It does: 1,193,182 times a second, and it's written in the builder's own file.  You can't fix that with a sentence in a prompt.

## One number

The last run ended red, out of time.  The best file counted 34 seconds while the host counted 10.

Then Claude ran that same file in its own sandbox, under emulation.  It passed.  Drift of 1 millisecond over ten seconds.

So what's different about the jig?  The jig is a Linux machine inside Hyper-V, inside my Windows box.  Reading the 8254 there costs about 67,000 processor cycles instead of 850, because every read crosses two hypervisors.  When Claude and I lifted this timer by hand, we had to change exactly one of Linux's numbers because of that: how long the quick calibration is allowed to take, from 50 milliseconds to 150.

The 3090 copied Linux faithfully.  It kept the 50.

I ran its file on the jig both ways.  With 50: 34,000 milliseconds against 9,924.  With 150: 9,500 against 9,500.  Drift, zero.

<!-- Chris: your reaction when that came back green.  This is the moment. -->

## What it means

The 3090 lifted the timer.  Right for a real PC, out of Linux, its own 64-bit division in assembly, all of it.  What stood between it and green was one number that exists because of how my test machine is set up, and nothing it could see told it that.

It didn't pass by itself, and I'm not going to pretend it did.  I changed the number.  But look at what moved it from nine reds to one number: none of it was a smarter model.  It was the tests telling it the truth, about what failed and why.

That's the piece this part adds.  An agent can only get better at what it can see.  If the test throws away the reason, the agent guesses.  If the test lies about the reason, the agent goes looking in the wrong place, very confidently.  A test is how you talk to the thing doing the work, and that goes for people too.

Next time: what the 3090 needs to know about the machine it's building for, and whether it can find that 150 by itself.  Then Dave's sieve.

<!-- Sections to fill once the commits are final, as part 22: Read along (findings/0031.md to 0036.md), Try this (put a hand-in under runs/ in the hand lift's place and run TST.timer.rate@1), As built (index/tool/agent@1.as-built.md, decision 0114), Addendum: the tests (agent.lift, timer.says, timer.rate, with their status from tests/results.txt), Code on GitHub (883ea0f to the post's last commit). -->
