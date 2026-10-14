*The story behind AiOs, part 13.*

## The welder who couldn't turn on the computer

I was going to be a welder.  I was disinterested in education, and welding seemed like a good honest plan.  Then I found a computer science course that I figured would be easy marks.

It was not easy marks.  I couldn't turn the computer on.  Some of the smartest kids in the school were in that room, and they laughed at me, because I was the guy who couldn't find the power button.

Two weeks in, I was one of the best programmers in the class.  I've loved programming ever since.  I was sixteen.

## Why the hell are you a programmer?

I became a software developer, and I never quite fit the picture.  I started out as a bass player, and these days I front a band.  I sing.  I like standing in front of a crowd.  I'm not saying no programmer does that, I'm saying I was the odd one out, and people told me so.  More than once I got "You're not detail-oriented.  Why the hell are you a programmer?"  And the follow-up: "How are you even decent at it?"

They had a point.  I'm not a detail person.  I forget definitions.  I mix up terms.  In a job where one wrong character breaks everything, that's a strange trait to bring along.

But I was good at it.  What I didn't have in detail, I made up for in patterns.  I could see how a system fit together, spot the same problem showing up in three places, and tell what the code was really trying to do.  I made it work, and I did well.

I didn't leave because I was failing at it.  I left because I wanted to do other things.  Consultant, then product manager.  I prefer being a communicator, which is why I'm doing this silly blog.  But if I'm being honest, I'm probably a better developer, haha (did someone just say "existential crisis"?).  The job taught me one thing I didn't expect, and it's what this post is about.

## Language matters

When you're not a detail person, you can't rely on remembering what everyone meant.  You have to get the words right, out loud, up front, and then hold everyone to them, starting with yourself.  A product manager who lets a word mean two things in a room ends up with two products.

Yesterday Claude wrote the post.  Today I went back over everything Claude and I have said since the start of this project, word by word.  One thing I've learned as an agentic developer: things move really fast.  The words had drifted while Claude and I built, a little at a time, and you'd remember every one of them.  As Ferris Bueller once said, "Life moves pretty fast.  If you don't stop and look around once in a while, you could miss it."

So this is a good place to recap every term I'm using in this project.  Where a word changed, I've said what I used to call it.

## The problem

First, the problem, because every word below exists to serve it.  AiOs isn't a general-purpose operating system.  It's purpose-driven: every machine starts from what it's for, and gets exactly the software that purpose needs and nothing else.  Nothing gets in without a written reason.  And the work is done by AI agents, so the hard part isn't writing the code, it's making sure nothing an agent does goes unseen or unchecked.

### The nucleus: the operating system itself

- **Nucleus.**  The operating system: everything that runs on the machine.  Linux's own code, only the lines it needs, plus the kernel written around them.  I don't know who the hell this guy is, but Claude tells me the word is Per Brinch Hansen's, from his RC 4000 system in 1969, for the small centre everything else is built around.  *Was "the OS".  Not "core": CoreOS is taken, and every processor has cores.*
- **Part.**  What the nucleus is made of: a driver, the kernel, a program.  Either lifted out of Linux with a record of exactly where it came from, or written from scratch.  *Was "segment", which already means x86 memory segmentation.  I wanted "quark".  Claude said no.*
- **Lifted.**  Taken out of Linux: only the lines a part needs, cut away from the rest of Linux, with a record of exactly which commit, file and lines they came from.  Lifted code lives in its own folder, apart from anything written from scratch.
- **Kernel.**  The part that shares the machine between programs and stops one that misbehaves.  *Was "the loader", and still is in the code.  I still prefer KERNAL, with the AL, like the Commodore 64.*
- **System call.**  How a program asks the nucleus for anything: write, exit, memory, read, time, load, save.  The list of them is the system call table.  New ones get added; old ones never move.  *Was "the door", which also meant a petition.*
- **Program.**  An application the nucleus runs: hello world, Hanoi, the calculator, the stopwatch, the sieve.

### Asking for things

- **Purpose.**  One sentence saying what a machine is for.  Everything traces back to it.  *I'm still not sure what this does beyond telling a human what the machine is for.  It's helpful, but I haven't decided yet how a machine will read it.*
- **Petition.**  The written request for anything that goes in: the purpose, what the software does, what it needs, and how it's tested.  Structured, never prose.  No petition, no part.
- **Flavor.**  The collection of parts defined by each application's petition.  It's the manifest a build starts from.
- **Catalogue.**  Everything a petition can ask for, filed in one place.  Like a library's card catalogue, it holds a card for each part, saying what it is, what it needs and which shelf its code is on, not the code itself.  *Was "the index", and the folder still is (formerly the index, so old links keep working).*
- **Catalogue number.**  The name of one thing in the catalogue.  A petition asks for things by catalogue number.  *Was "code", which already means the programs themselves.*
- **Capability.**  One thing a part is allowed to touch, like the serial port or the disk.  A part asks for capabilities, and gets nothing else.

### The agents

- **Agent.**  Anything around the nucleus that acts on it, on someone's behalf.  Some will use AI and some never will.  *Was "roles" or "components".  Even I got confused.*
- **Referee.**  Runs before every build and again after it.  Before, it checks that every petition asks only for what's in the catalogue, and that every capability it asks for has a test.  After, it runs those tests itself and refuses anything that doesn't pass, against the rule book I wrote.  No AI, no judgment, and nothing gets cached until it says yes.
- **Builder.**  Turns the petitions into the nucleus, with the referee checking before and after.  It works from a queue: it builds everything it can, sends the full list of anything missing to the quartermaster, and stops.  When the missing parts are added, the flavor goes back in its queue and the whole thing runs again.  Today I press play.  Proving it makes the same bytes every time is still on the list.
- **Tester.**  Boots it, proves what it does, then breaks the code on purpose to prove the tests notice.  The referee will run it after every build.  Today I run it by hand.
- **Historian.**  Run on demand by the quartermaster, the referee or me.  Looks up the Linux commit a part was lifted from, checks whether anything has touched those lines since, and if so, reads the change and says whether the part needs to change.  Which branches AiOs pulls from, like the long-term support ones, will be part of how AiOs is configured, and the historian will weigh a fix by the branch it's on.  Reports, never merges.
- **Quartermaster.**  When the builder sends it something missing, it will read the Linux source, find the smallest piece that provides it, and draft the petition to add that part.  It reads the mechanic's fixes the same way, as new code.  *Editor's note: an open question.  When a fix touches code that came from Linux, should the quartermaster decide whether to send it back upstream?  A patch to Linux goes out under a person's name, so a fully agentic system has to answer that somehow.*  It will also draft each application's petition, because no human could.  Not built yet.  Claude and I do it by hand.
- **Mechanic.**  When a test fails, it will propose a fix to the code, never to the test.  It reads what the test checks, the spec behind it and what the code actually does; the gap between them is the bug.  It never installs a fix.  A fix is new code, so it goes to the quartermaster like anything else, and then to the committee.  Not built yet.
- **Committee.**  AI seats that will vote on every petition the quartermaster drafts, including the mechanic's fixes.  Each seat is one priority, borrowed from the BSD family, where each flavour built a whole operating system around one: security (OpenBSD), performance (FreeBSD), portability (NetBSD) and experience (Apple's Darwin), plus stability and fit with the purpose.  Each is a synthetic personality, a tribute to the people behind those systems, on its own AI model.  It never writes a petition.  Until it exists, the committee is me.
- **Decision ledger.**  Not an agent, a record: every choice an agent makes, with its options and who picked it, checked by the referee against what actually changed, before anything is committed.  Not built yet.  It comes first.

### Proving it

- **Spec.**  One sentence, written by a human, saying what something must do, precise enough to fail.  This is where the plain-English explanation lives.  A petition never explains itself; it points to specs.
- **Test.**  Bound to one spec, and it says in plain words exactly what makes it fail, so an AI can understand what went wrong.  A test that can't fail is decoration.  Only a human changes a test.
- **Findings.**  The project's corrections log: what went wrong, what was decided, and why.  Dated and public.
- **The jig.**  The temporary workbench everything is built and tested on: a small Linux machine on my PC.  It gets thrown away once AiOs can rebuild itself and get the same bytes.

## What going back found

Here's the part that proves the point.  While renaming everything, Claude found a line in the rulebook that still said my own code was under the MIT licence.  That's the licence a commenter caught me on back in part 1, the one I fixed in an hour.  I fixed the licence.  I missed one line saying the old one.

Nobody had read that line in days.  The rename read every line, because it had to.

## What it cost

Renaming means moving folders, and some of the earlier articles link to those folders.  Parts 5, 6, 9, 10 and 11 have links on LinkedIn that will now go nowhere.  I don't edit posts once they're up, so those links stay broken there.  The copies on GitHub are fixed.

I chose that.  A handful of dead links now, against confusing words forever.  Easy call.

## Locking it

Fixing the words once is only worth it if they stay fixed.  So two things changed.

Changing the vocabulary is now a decision only a human can make.  An agent can propose a new word.  It can't quietly start using one.

And the check I run over every post before it's committed now fails if a new post uses a word I've retired.  I tested it by sneaking "segment" into the last article, and it went red.  The machine reads the words the way it reads the code.

## What I'm taking away

An agent can only check what's been defined.  "Does this part touch only what it asked for?" is a question a machine can answer, but only if "part" and "asked for" mean one thing, written down.  Fuzzy words don't just confuse people, they make things impossible to check.  So before you let a machine hold anyone to anything, the words come first.

For a guy who was told he wasn't detail-oriented enough to be a programmer, that's a nice place to end up.  Language matters.  Come along!

## Read along

1. [CLAUDE.md](https://github.com/cmcluckie/aios/blob/main/CLAUDE.md).  Scroll to "Terms".  Every word in this project, what it means, and for the ones that changed, what they used to be.
2. [findings/0014.md](https://github.com/cmcluckie/aios/blob/main/findings/0014.md).  The rename, written up: the table, the reasons, and what broke.
3. [schema/petition.md](https://github.com/cmcluckie/aios/blob/main/schema/petition.md).  The tables a petition can name.  Nine now.
4. [jig/postcheck](https://github.com/cmcluckie/aios/blob/main/jig/postcheck).  Find "Retired words": the few lines that keep the old words from coming back.
5. [docs/aios-components.png](https://github.com/cmcluckie/aios/blob/main/docs/aios-components.png).  The whole thing in one picture, in the new words.

## Try this

Open the repo in Codespaces and run `sh jig/postcheck`.  It passes.  Now open this article's file, posts/2026-10-23.html, change any word to "segment", and run it again.  It names the file and the retired word.  Change it back.

## As built

Nothing new was built in this part.  Every part, machine and tool was renamed and moved, and their as-built specs moved with them, under index/part/, index/machine/ and index/tool/.

## Addendum: the tests

No new tests.  After the rename, every existing test was run again under software emulation: 45 passed, 0 failed, 2 skipped (the KVM clock test, which needs the jig, and the retired sieve test).  The new retired-words check in jig/postcheck was proven able to fail before it was trusted.

## Code on GitHub

Every line that changed, on one page (hash when committed).

The commits behind this post, in order:

1. Language matters (hash when committed)

[The whole repo](https://github.com/cmcluckie/aios).
