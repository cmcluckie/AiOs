*The story behind AiOs, part 19.*

## Going way back

Until now every program in this series has been a university or high school exercise.  Hanoi, a sieve, a calculator, a stopwatch.  They need almost nothing: a bit of C, and not much from the operating system.

This one is different.  It's the first program that isn't ours.  It comes off the internet, from somebody else, and whatever is in it is what goes into the system.  It's `cat`, from Unix Version 7, released in 1979.  If you've ever sat at a Unix prompt you've used its great-grandchild.  It prints a file.  That's the whole job.  The source is 63 lines of old C.

And here's the part I've been looking forward to.  I didn't write the request for it.  Nobody did.

## What a petition is, again

Every program in AiOs starts as a petition, a form that says what the program needs and why.  Parts, capabilities, a machine, tests.  It's structured, never prose, and a referee with no AI in it checks the form.  Every petition so far, Claude and I wrote together.

For `cat`, a model wrote it.  A small one, Sonnet 4.6, called through Amazon Bedrock.  It's handed the purpose in one sentence ("print the contents of the files named, or of standard input, to the console"), the layout of a petition, one finished petition as an example, and the catalogue, which is the list of everything AiOs has.  Then it's allowed to read the `cat` source, but only through the reader.  That's the tool that answers questions about one fixed copy of the old source and nothing else.  The reader already did this for Linux.  Now it can read Unix V7 as well.

## What I was hoping to see

Let me say this up front, like I have since the start of the series.  I have no delusions of grandeur.  I'm not building something that builds its own Linux distribution, and I still have real doubts about whether the cost and the time would ever make sense.  Linux has thirty million lines and a mountain of drivers.  I don't know if there's a business in pulling things out of open source, and I'm not going to guess in this article.  That isn't what this part is for.

For other kinds of software it could be very useful.  For something as big as Linux, I don't know yet.

What I do care about is something I struggle with every day.  I do a hell of a lot of AI development, and about three quarters of the time I don't know what it's working on.  I can never go back and see in detail what actually got built.  Maybe I'm doing something wrong.  But I want the same thing every time: I ask for the problem as a numbered list, one plain sentence per point, and I want to know where to find it and how to find it.  It doesn't matter if I'm tired, or fresh, or at the end of a long day.

A petition and its run record are that list.  They're the same shape every time.  The model's call is written down, a referee that has no AI in it checks the result, and I can go back later and say: this one I'll stand behind.  That's the sanity check I wanted from a program as old and as small as `cat`.

## The first thing it did was talk

The model's first answer was a good petition with a paragraph and a bullet list in front of it, explaining its thinking.  The prompt said, in so many words, no prose.  The model prefaced three times out of three on its first attempt.

And the referee accepted it.

That's the bug I like best in this part.  The referee's check for a petition only looked for a `[petition]` heading somewhere in the file.  I'd tested a pure essay (refused) and a clean petition (accepted), and never an essay in front of one.  Claude fixed it so every line has to be a comment, a heading or a `key = value` line, and the referee now says which line broke the rule.  Then we wrote the test that catches it, a seventh way of sabotaging the check to prove the test can go red, and a note in the decisions.  The real model found it for us, for about four cents.

I decided the draft step should not quietly trim the preface off.  A petition with prose in it is thrown away, the call is still written down because it was paid for, and on the next try the model is shown what was wrong.  Try two came back clean.  The whole thing is four calls and 13 cents.

## What it wrote

Eighty-two lines.  A purpose, four capabilities each with a reason, the program itself, and eight more components.  Here's what I think it got right.  It used the parts AiOs already has, a serial port, a disk and a Minix file system, to read a file.  Nobody told it which ones.  It added a test for a file that isn't there.

Here's what it got wrong or missed, and I'll say it plainly.

It named no C library.  Claude had measured this before the draft: `cat` needs three header files and eleven things from a C library, and AiOs has none of them.  The model's petition doesn't mention that at all.  It picked an old version of the loader when a newer one exists, and it listed two versions of the same serial driver.  Three of the referee's rules fail on it: a clause it invented that doesn't exist yet, a part that needs a capability it never asked for, and a part it depends on but didn't list.

So that's the number to hold on to.  It got the shape right and missed things that matter, for 13 cents.

## The builder says what's missing

Then I asked the builder to build it.  It stopped, on purpose, and wrote down why.  Eight things named in the petition don't exist in the catalogue yet: the `cat` part itself, four clauses, and three tests.  The list is a file, `missing.txt`, one line each.  The builder doesn't guess and it doesn't make anything up.  It says what it would need and exits.

That list is the work order for the next part.  The thing I wanted from this part was never a `cat` that runs.  Nothing runs yet.  It was a request nobody wrote, and a machine that reads it and says exactly what's missing.

## What nobody asked for

I ran the live calls from a second machine, the jig, and got a lesson in not trusting your own paste.  I told it to pull the new code, the pull stopped because of a leftover file from an earlier run, and the old code ran anyway.  Old code meant the old referee, the one with the hole, so it happily accepted another model reply with its reasoning stuck on the front.

That cost two and a half cents I hadn't planned on.  It also gave me a second, independent sample of what the model does, which is how I know the preface wasn't a one-off.  Claude's first move was to say what had gone wrong and to give me the two commands that fix it, and then we ran it again properly.  Every one of those calls, the accidental one included, is in the run record with its cost.  Nobody asked for that.  It's the reason I can tell you the real total is 13 cents and not "about ten".

## What I'm taking away

The piece this rung added: an AI can write the request, and a check that has no AI in it decides whether the request is any good, with a record I can open the next morning and read the same way every time.  The model prefaced; the referee, after we fixed it, refused.  The model missed things; the referee named three of them.

I'm not claiming this is how you'd build a distribution.  There are thirty million lines in Linux, and `cat` is 63 lines.  What I'll have after the next part is a number for how viable all this is, and I'll tell you what it is even if I don't like it.  The next part is the last one in this series: musl, the loader for Linux's program files, and `cat` printing a file.

## Read along

1. [draft/prompt.txt](https://github.com/cmcluckie/aios/blob/main/draft/prompt.txt).  Everything the model is told.  Fifteen lines.
2. [jig/draft](https://github.com/cmcluckie/aios/blob/main/jig/draft).  The draft step.  Look for where the reply is handed to the referee before anything is kept.
3. [petitions/0012.toml](https://github.com/cmcluckie/aios/blob/main/petitions/0012.toml).  The petition no person wrote.
4. [pool/AGT.referee@1/referee.c](https://github.com/cmcluckie/aios/blob/main/pool/AGT.referee%401/referee.c).  Look for `cmd_petition` and the line that says a petition is fields and never prose.
5. [findings/0022.md](https://github.com/cmcluckie/aios/blob/main/findings/0022.md).  The plan, the measured list, and what changed while building.

## Try this

Run `sh pool/AGT.referee@1/referee petition petitions/0012.toml`.  It says the petition is well formed and lists nine components still waiting to be admitted.  Now put one sentence of prose at the top of a copy of that file, something like "Here is the petition for cat, with my reasoning first.", and run it on the copy.  The referee refuses it and names line 1.  That's the exact mistake the model made, and the check that now catches it.
