*The story behind AiOs, part 17.*

## This one scares me

I don't know why, but this part scares me more than any of the others.  So many questions go through my head while I work on it.  What will people think?  Will they think I'm stupid for trying to change Linux?  I've taken risks before.  Years ago I built a product people thought was a crazy idea, and it's still a company today, so I know this feeling.  But this is the post where the AI does something on its own, and honestly, this is where the blog either completely fails and I stop posting, or we go to the next level.

## The quartermaster

A quartermaster is the person who decides what goes in the pack, so that's what I called the agent that decides what goes into AiOs.

Here's the job.  For a while now, Claude and I have been lifting parts out of Linux by hand: the serial driver, the stopwatch, the screen.  Claude reads Linux, cuts it down to what the program needs, and I judge the result.  The quartermaster does that on its own.  Give it a petition (the plain list of what a program needs from the machine) and it has to find the right code, work out what that code calls, and cut it down to something small enough to use.  Ask two different models and you could get two completely different slices.

Do the models already know Linux?  As far as I can tell, yes, it's free source and every big model has read it, but I can't see inside a model to prove it.  So the exam doesn't ask what the model knows.  It checks what the model hands back.

## How it looks things up

Here are the steps, in plain English.

1. It starts from the petition.
2. The model says where it thinks the code lives, a file and a function.
3. A script checks that name against one fixed copy of Linux.  A name that isn't there is refused.
4. The script hands back just those lines, never more than 200 at a time.
5. The model reads them, sees what they call, asks for those next, and keeps going.
6. It trims the result down to what the part needs.
7. The jig builds it and runs the tests.

That script is the reader, and it has no AI in it.  The same question gets the same answer every time.  Here's what it looked like in the timer run.  One of the model's lookups asked where `quick_pit_calibrate` is defined.  The reader said line 557 of `arch/x86/kernel/tsc.c`.  The model asked to read that stretch and got Linux's own lines back.  Hold on to that, because it comes back at the end.

The reader keeps the model from inventing a lookup.  It can't check the file the model finally writes.  Only step 7 does that.

## The exam

Three items, the three parts I'd already lifted by hand: the serial driver, the timer, and the screen.  For each one the model gets the question, the part's header file, and the reader.  My hand lift stays hidden.

I should be straight about the header.  It's a list of names the code has to provide, because my tests build against those names.  It's an interface, not an implementation, but it is a hint, and you should know the model gets it.

The judge then swaps the model's file in for my hand lift, in a throwaway copy of the repo, and runs the same tests.  It passes only if every test goes green.  I score pass or fail, lines against mine, and tries.

## Why Sonnet 4.6

I started with Claude on Amazon Bedrock, one small pinned model.  Not the 3090 yet.  One unknown at a time: if a small local model failed, I wouldn't know whether the method or the model was at fault.

I wanted Sonnet 5.5.  AWS wouldn't let my account use it, or Sonnet 5.  The call came back "not available for this account," while Sonnet 4.6 worked fine on the same account.  I never found the cause, so I'm going to ask AWS.  The exam runs on Sonnet 4.6 and the newer row will wait.

## The first run failed, and that was my fault

The serial item failed to build.  The model had included a system header and called a function that doesn't exist in a stand-alone build.

The prompt said "stand-alone" and never said how the file is built: freestanding, no C library, no headers at all, every warning an error.  The model's function names matched the header exactly, and its lift was 68 lines against my 57.  The reading and writing were fine.  The missing fact was the build, and I hadn't told it.

So I changed the prompt after seeing a result.  That shouldn't happen quietly, so it's recorded: the first run stays on the scorecard as it was, and from now on every run carries a fingerprint of the prompt it was given.  The first three serial runs show a dash, because the fingerprint didn't exist yet.

## The bill

As a product manager, this is what I wanted before anything else.  Every try gets a record: tokens in and out, the result, and what it cost.  And there's a hard cap.  The script checks this month's recorded spending before every call, and refuses if the next call could pass 20 dollars.  I'm adding AWS's own budget alert as a second wall.

The second serial run passed, at 62 lines against 57.  Both runs together cost 98 cents.  I said a word I won't print here.

Nearly all of it was the conversation being sent again at every step, since the model is called over and over while it reads.  So the runner now marks what it has already sent to be remembered, and Bedrock charges a tenth of the price for that part.  The next serial pass came in at 54 lines, three under my hand lift, for 12 cents.  The two passes aren't the same run, so the drop isn't all caching, but it's most of it.

## The scorecard

Sonnet 4.6, all three items.

- Serial: pass, 54 lines against my 57, $1.10 with every try counted.
- Screen: pass, 79 lines against my 55, $0.07.
- Timer: fail, 95 to 100 lines against my 103, $1.96.

Total $3.13 across seven sittings.  Two parts passed, so each one cost about $1.56 once you count every failed try, the prompt mistake included.  That's a small model, one sitting each for two of them, and a header that gave it a hint, so I'm not drawing a big conclusion.  The screen is 24 lines longer than mine, and I haven't looked at why.

## The timer failed, and that's the best part

The timer item measures how fast the processor's clock ticks, by timing it against the PC's old 8254 timer chip.  The model's code built and ran, and the test said the rate was never measured.  It reported zero.

It tried three times in one sitting.  Two of the tries built and failed the same way, and the middle one ran out of steps without writing anything.  Claude and I went and looked.  The model's file is Linux's own code, almost line for line.  Mine differs in two places that matter, and both are things we found by running on this machine.

First, Linux gives the measurement 50 milliseconds.  Under this setup, reading that chip is so slow that Linux's own accuracy check can never pass in 50, so my version gives it 150.  Second, I leave out a write to a port this machine doesn't have.

Here's where the lookup from earlier comes back.  The 50 was right there in the lines the reader handed the model.  It did what the task asked, which was to keep what Linux actually does.  Nothing in the Linux source could tell it that Linux's number doesn't work here.

I changed that one number in the model's file, from 50 to 150, and ran it through the judge.  It passed, at 100 lines.  I did that by hand.  It's not a test, and it's not on the scorecard.  But it tells me the model's code wasn't the problem.  I'd even seen this one coming, sort of.  Back in part 12, one of my deliberate breakages puts Linux's 50 back and checks the timer test goes red.  On the jig, it does.

I'm leaving the fail on the card.  I could tell the model about the machine and it would pass, but the pass would prove nothing.  And I'm not going to pretend this is fine.  A part that comes back wrong on the machine it's for is a real failure, and it has to be fixed.  Fixing what goes red is the mechanic's job, the next agent on my list, so that's another post, when we build it.  Until then the timer stays red, in public.

## What I'm taking away

I was scared the exam would show the AI can't do this.  It showed something I can use: two parts passed, one failed for a reason I can point at, and every dollar is written down in a file you can open.  The timer still has to be fixed, and I'll fix it in the open.  I want to say it goes through.  Two out of three on one small model isn't proof, but it's a start.

Next, the same exam on the 3090.

## Read along

1. [jig/reader](https://github.com/cmcluckie/aios/blob/main/jig/reader).  The script that answers only from the one fixed copy of Linux.
2. [exam/prompt.txt](https://github.com/cmcluckie/aios/blob/main/exam/prompt.txt).  What the model is told.  This is the file I changed after the first run.
3. [jig/judge](https://github.com/cmcluckie/aios/blob/main/jig/judge).  Swaps the model's file in and runs the tests.
4. [jig/sit](https://github.com/cmcluckie/aios/blob/main/jig/sit).  The runner.
5. [runs/](https://github.com/cmcluckie/aios/tree/main/runs).  Every try, with the question, the answer and the cost.
6. [findings/0020.md](https://github.com/cmcluckie/aios/blob/main/findings/0020.md).  The account trouble, the prompt, the bill, and the timer.

## Try this

Open the repo in Codespaces and run `sh jig/scorecard`.  That's the table above, built from the files in `runs/`.  Then open the last timer run's `output.txt` next to `lifted/PART.timer.tsc-pit@2/tsccal.c` and find the 50.  Would you have caught it?  If you would have, tell me how.
