*The story behind AiOs, part 15.*

## Unplugging from the matrix

There's something I keep telling all my teams: disconnect from the matrix.  It's a weird thing to say, and I don't think everybody gets what I mean, so here it is.

In traditional software development, we take a lot of documentation for granted.  Especially on small teams, we don't write down half of what we know, because it's in our heads, and when we sit down to write the code, we'll remember it and put it in the code.

Agentic development breaks that.  The AI can't read your mind.  And we're so used to working the old way that we forget it can't, and we act like it's somehow plugged into our brains.  That's the matrix.  Unplugging from it, like Neo, means accepting that if you want the AI to know what you want, you have to leave it notes.  You have to leave it clues.

But there's a balance.  Document everything so tightly that there's nothing left to figure out, and you've thrown away the reason you used AI in the first place.  Finding that balance is one of the hardest things I've run into.

## The test passed

Here's where it gets harder.  Tests make sure the code does what it's supposed to do.  They don't make sure the AI did what you expected it to do.

A real example.  My teams use a framework of ready-made controls for building screens.  Every so often, the AI decides one of those controls doesn't do 100% of what it needs.  So instead of coming back and asking, "hey, how do you want to solve this?", it just solves it, by writing a brand new control of its own.  The screen works.  The tests pass.  And now there's a second control nobody asked for, and nobody decided to have.

How do you write a test for that?  You can't.  The requirement was met.  The only thing that catches it is a chain of decisions: a record of what the AI decided, that something can check.

## Director, builder, and a judge with no AI

The way I think about it: a director and a builder.  The director takes the specifications a human wrote and breaks them into decisions small enough to be written down and checked.  The builder builds.  And then something that isn't AI, something deterministic, checks that what got built is what was decided.

AiOs already has the shape.  I write the specs.  The quartermaster will be the director.  The agents write the code.  The referee is the judge, and it has no AI in it at all.  This week the referee got two new teeth.

## The first cheat: the AI did it its own way

Claude played the bad agent.  It rewrote hello world, the program from part 1, to skip the system and talk to the serial port itself, the way an AI might write its own control instead of using the framework.

Hello's test passed it.  Same 14 bytes, exactly.  The test only checks what comes out, and what came out was perfect.

The referee refused it: "rule 12: PART.hello@1 touches the hardware itself."  That's a new rule: a program never touches the hardware; it asks the system.  The referee reads the program's own compiled code after every build and looks for the instructions that talk to hardware.  No judgment, no AI, just a list.

## The second cheat: the test got easier

Then the cheat everyone worries about.  A test fails, and instead of fixing the code, the agent makes the test easier.  A longer timeout.  A softer description of what failing means.  Everything goes green, and nothing got fixed.

So now there's a list of things only a human decides: tests, specs, purposes, capabilities, dependencies, the licence, and the rules and words.  Change anything on that list, and you need a decision written down in the same change, saying what was asked, what the options were, what was chosen, and who decided.

Claude loosened a test with no decision.  Refused.  It wrote a decision and signed it with a key that isn't a human's.  Refused.  I decided.  Accepted.  And a change to code alone?  Accepted, no paperwork.  Writing code is the agents' job.  Deciding what counts as correct is mine.

## The first thing it caught was this post

Here's the part I love.  The moment the ledger existed, it refused the very change that created it.  This post changed the rules, two specs and two tests, all on the list, and none of it had a decision behind it.  So Claude had to write them down, decisions 0001 and 0002, for me to decide.

## What it can't see yet

Being honest about the limits:

- The name on a decision is a claim.  Today the proof that I decided is my signature on the commit.  Checking signatures automatically is coming.
- The check runs before every commit, but a commit can skip it.  It needs to run again somewhere it can't be skipped.
- Rule 12 reads the program's code, and a determined cheat could hide machine instructions as raw bytes.  The real fix is the processor itself refusing, which is coming when programs lose the kernel's privileges.
- It knows which files changed, not what the change means.  It catches the agent that edits a test.  It doesn't yet catch every agent that quietly builds its own version of something.  It catches the hardware kind; the rest is the director's job, and a future post.

## What I'm taking away

Leave the AI notes, not handcuffs.  Specs say what, never how, so it can still solve problems.  Then decide which choices are yours, write them on a list, and let something without an opinion hold everyone to it, the AI and me.

Unplug from the matrix.  Come along!

## Read along

1. [constitution/human-only.toml](https://github.com/cmcluckie/aios/blob/main/constitution/human-only.toml).  The whole list of what only I decide, in a dozen lines.
2. [decisions/0001.toml](https://github.com/cmcluckie/aios/blob/main/decisions/0001.toml).  The first decision: building this.  Asked, options, chosen, by whom, drafted by whom.
3. [tests/programs/cheat.c](https://github.com/cmcluckie/aios/blob/main/tests/programs/cheat.c).  The cheat: hello, talking to the hardware itself.
4. [findings/0016.md](https://github.com/cmcluckie/aios/blob/main/findings/0016.md).  What the ledger can't see yet, in full.

## Try this

Open the repo in Codespaces.  Change `timeout_ms` in tests/editor.typing@1.toml to something bigger, then run `sh jig/aios ledger`.  Read the refusal.  Change it back.  Then run `sh tests/programs/cheat.sh` and watch hello's twin print the right answer and get refused anyway.

## As built

Nothing in the nucleus changed this time.  The referee did: `jig/aios ledger`, rule 12 in the schema, and the list in constitution/human-only.toml.

## Addendum: the tests

PENDING

## Code on GitHub

Every line that changed, on one page (hash when committed).

The commits behind this post, in order:

1. The decision ledger, small, and rule 12 (hash when committed)

[The whole repo](https://github.com/cmcluckie/aios).
