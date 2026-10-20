*The story behind AiOs, part 19.*

## A program that isn't ours

Until now every program in this series has been a university or high school exercise.  Hanoi, a sieve, a calculator, a stopwatch.  They need almost nothing: a bit of C, and not much from the operating system.

This one is different.  It's the first program that comes from somebody else, off the internet, and whatever is in it is what goes into the system.  It's `cat`, from Unix Version 7, released in 1979.  If you've ever sat at a Unix prompt you've used its great-grandchild.  It prints a file.  The source is 63 lines of old C.

And here's the part I've been looking forward to.  I didn't write the request for it.  Nobody did.

## What a petition is, again

Every program in AiOs starts as a petition, a form that says what the program needs and why.  The pieces of the system it wants (I call them parts), the things it's allowed to do (capabilities), each with a reason, a machine to run on, and tests.  It's structured, never prose, and a referee with no AI in it checks the form.  Every petition so far, Claude and I wrote together.

For `cat`, a model wrote it.  A small one, Sonnet 4.6, called through Amazon Bedrock.  It's handed the purpose in one sentence ("print the contents of the files named, or of standard input, to the console"), the layout of a petition, one finished petition as an example, and the catalogue, which is the list of everything AiOs has.  It's allowed to read the `cat` source, but only through the reader, a tool that answers questions about one fixed copy of the old source and nothing else.  The reader already did this for Linux.  Now it can read Unix V7 as well.

## What I was hoping to see

I do a hell of a lot of AI development, and about three quarters of the time I don't know what it's working on.  I can never go back and see in detail what actually got built.  Maybe I'm doing something wrong.  But what I want is the same thing every time: the problem as a numbered list, one plain sentence per point, somewhere I know how to find it, whether I'm tired or fresh.

A petition and its run record are that list.  The model's call is written down, a check with no AI in it judges the result, and I can open it the next morning and say: this one I'll stand behind.  That was the sanity check I wanted, from a program as small and old as `cat`.

I'll say the other thing up front, like I have since the start of the series.  I have no delusions of grandeur.  Linux has thirty million lines and a mountain of drivers, and I have real doubts about whether the cost and the time would ever make sense.  I'm not going to guess in this article.  That isn't what this part is for.

## The first thing it did was talk

The model's first answer was a good petition with a paragraph and a bullet list in front of it, explaining its thinking.  The prompt said, in so many words, no prose.  It did that three times out of three on its first attempt.

And the referee accepted it.

Its check for a petition only looked for a `[petition]` heading somewhere in the file.  I'd tested a pure essay, which it refused, and a clean petition, which it accepted.  I'd never tested an essay stuck in front of a petition.  The real model found that for us, for about four cents.  Claude changed the check so every line has to be a comment, a heading or a `key = value` line, and the referee now says which line broke the rule.  Then we wrote the test that catches it, plus a seventh way of sabotaging the check to prove that test can go red.

I decided the draft step shouldn't quietly trim the preface off.  A petition with prose in it is thrown away, the call is still written down because it was paid for, and on the next try the model is shown what was wrong.  Try two came back clean.

## What it wrote

Eighty-two lines.  The program itself plus eight other components, four capabilities with a reason each, and three tests.  Here's what I think it got right.  It reused what AiOs already has, a serial port, a disk and a Minix file system (an old, simple way of storing files), to read a file.  Nobody told it which ones.  It also added a test for a file that isn't there.

Here's what it got wrong or missed.

It named no C library.  Claude had measured this before the draft: `cat` needs three header files and eleven things from a C library (the standard toolbox every C program leans on, like opening a file), and AiOs has none of them.  The petition doesn't mention it.  It picked an old version of the loader, the part that starts programs, when a newer one exists, and it listed two versions of the same serial driver.  Three of the referee's rules fail on it.  It cites a requirement that doesn't exist yet.  It includes a part that needs an ability the petition never asked for.  And it depends on a part it never listed.

So that's what 13 cents buys.  The shape is right and things that matter are missing.

## The builder says what's missing

Then I asked the builder to build it.  It stopped, on purpose, and wrote down why.  Eight things named in the petition don't exist in the catalogue yet: the `cat` part itself, four requirements, and three tests.  The list is a file, `missing.txt`, one line each.  The builder doesn't guess and it doesn't make anything up.

That list is the work order for the next part.  Nothing runs yet.  What I wanted from this part was a request nobody wrote, and a machine that reads it and says exactly what's missing.

## A lesson in trusting my own paste

I ran the live calls from a second machine, the jig, and I told it to pull the new code.  The pull stopped because of a leftover file from an earlier run, and the old code ran anyway.  Old code meant the old referee, the one with the hole, so it happily accepted another model reply with its reasoning stuck on the front.

That cost two and a half cents I hadn't planned on.  It also gave me a second, independent sample, which is how I know the preface wasn't a one-off.  Claude said what had gone wrong and gave me the two commands that fix it, and we ran it again properly.  Every call, the accidental one included, is in the run record with its cost.  The four calls came to 13 cents in total.

## The licence

One more thing came up before this went out, and it changed the next part.

The V7 source came with a letter.  In 2002 Caldera, the company that owned the old Unix code by then, let anyone use, change and share it, as long as they kept the copyright notice and, the odd part, credited Caldera in any advertising for a product built from it.  AiOs is GPL-2.0-only, the same licence as Linux, because every image mixes Linux's lines with ours.  A rule about advertising is the kind of extra condition the GPL doesn't allow.  I'm not a lawyer and I won't pretend to be one here.  This part only reads `cat` and copies nothing, so it's clean.  The next part would have built it in.

So I swapped.  The `cat` in the next part comes from 4.4BSD-Lite, Berkeley's 1994 release, the same line of Unix a generation on.  Berkeley's code carried the same kind of advertising clause, and in July 1999 their licensing office sent a letter dropping it.  What's left is a licence the GPL is fine with.  Same job, a longer program, and the model gets to read it from scratch.

## What I'm taking away

The piece this rung added: an AI can write the request, a check that has no AI in it decides whether the request is any good, and I get a record I can open the next morning and read the same way every time.

I'm not claiming this is how you'd build a distribution, and I'm not claiming `cat` proves anything about Linux.  The next part is the last one in this series: a small C library (musl), a loader for Linux's program files, and Berkeley's `cat` printing a file.  After that I'll have a number for how viable all this is, and I'll tell you what it is.

## Read along

1. [draft/prompt.txt](https://github.com/cmcluckie/aios/blob/main/draft/prompt.txt).  Everything the model is told.  Fifteen lines.
2. [jig/draft](https://github.com/cmcluckie/aios/blob/main/jig/draft).  The draft step.  Look for where the reply is handed to the referee before anything is kept.
3. [petitions/0012.toml](https://github.com/cmcluckie/aios/blob/main/petitions/0012.toml).  The petition no person wrote.
4. [pool/AGT.referee@1/referee.c](https://github.com/cmcluckie/aios/blob/main/pool/AGT.referee%401/referee.c).  Look for `cmd_petition` and the line that says a petition is fields and never prose.
5. [findings/0022.md](https://github.com/cmcluckie/aios/blob/main/findings/0022.md).  The plan, the measured list, and what changed while building.

## Try this

Run `sh pool/AGT.referee@1/referee petition petitions/0012.toml`.  It says the petition is well formed and lists nine components still waiting to be added to the catalogue.  Now put one sentence of prose at the top of a copy of that file, something like "Here is the petition for cat, with my reasoning first.", and run it on the copy.  The referee refuses it and names line 1.  That's the exact mistake the model made, and the check that now catches it.
