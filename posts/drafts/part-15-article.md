*The story behind AiOs, part 15.*

## Unplugging from the matrix

There's something I keep telling all my teams: disconnect from the matrix.  It's a weird thing to say, and I don't think everybody gets what I mean, so here it is.

In traditional software development, we take a lot of documentation for granted.  Especially on small teams, we don't write down half of what we know, because it's in our heads, and when we sit down to write the code, we'll remember it and put it in the code.

Agentic development breaks that.  The AI can't read your mind.  And we're so used to working the old way that we forget it can't, and we act like it's somehow plugged into our brains.  That's the matrix.  Unplugging from it, like Neo, means accepting that if you want the AI to know what you want, you have to leave it notes.  You have to leave it clues.

But there's a balance.  Document everything so tightly that there's nothing left to figure out, and you've thrown away the reason you used AI in the first place.  Finding that balance is one of the hardest things I've run into.

This post is long, and it's a slog in places, because the referee is really complicated and I wanted to show you how I got to each decision rather than just hand you the decisions.  If you only read one section, read the last one.

## The test passed

Here's where it gets harder.  Tests make sure the code does what it's supposed to do.  They don't make sure the AI did what you expected it to do.

A real example.  My teams use a framework of ready-made controls for building screens.  Every so often, the AI decides one of those controls doesn't do 100% of what it needs.  So instead of coming back and asking, "hey, how do you want to solve this?", it just solves it, by writing a brand new control of its own.  The screen works.  The tests pass.  And now there's a second control nobody asked for, and nobody decided to have.

How do you write a test for that?  You can't.  The requirement was met.  The only thing that catches it is a chain of decisions: a record of what the AI decided, that something can check.

## Director, builder, and a judge with no AI

The way I think about it: a director and a builder.  The director takes the specifications a human wrote and breaks them into decisions small enough to be written down and checked.  The builder builds.  And then something that isn't AI, something deterministic, checks that what got built is what was decided.

AiOs already has the shape, with one wrinkle in the names.  AiOs already has a builder, and it's the one with no AI: it compiles, nothing more.  The AI builders are the agents that write code.  So far only one is defined, the mechanic, which proposes a fix when a test fails.  The ones that write a new part when new requirements come in haven't been defined yet.  Until they are, they're Bob the Builders, and they're a whole other blog series.  Today, Bob is Claude.  I write the specs.  The quartermaster will be the director.  The referee is the judge, and it has no AI in it at all.  This week the referee got two new teeth.

## The first cheat: the AI did it its own way

Claude played the bad agent.  It rewrote hello world, the program from part 1, to skip the system and talk to the serial port itself, the way an AI might write its own control instead of using the framework.

Hello's test passed it.  Same 14 bytes, exactly.  The test only checks what comes out, and what came out was perfect.

The referee refused it anyway: "rule 12: PART.hello@1 touches the hardware itself."  (PART.hello@1 is hello world's catalogue number.)  Rule 12 is new this week, and it's simple: a program never talks to the hardware directly.  It asks the operating system to do it.  After every build, the referee reads the machine instructions the program was turned into and looks for the handful that talk to hardware.  If it finds one, the build is refused.  No judgment, no AI, just a list.

## The second cheat: the test got easier

Then the cheat everyone worries about.  A test fails, and instead of fixing the code, the AI changes the test so it passes.  It gives the test more time to finish, or quietly loosens what counts as a failure.  Everything goes green, and nothing got fixed.

So now there's a list of the things only I decide.  The tests and the specs, so nobody can move the goalposts.  What AiOs is for, what each program is allowed to do, and what tools it depends on.  The licence.  And the rule book itself, including the words in it.  Change anything on that list, and the same change has to include a written decision: what was asked, what the options were, what was chosen, and who decided.

Then Claude tried to cheat it.  It loosened a test with no decision at all.  Refused.  It wrote the decision itself and put a name on it that isn't on the list of people allowed to decide.  Refused.  With a decision in my name, the change got through.  And a change to code alone, no tests touched?  Accepted, no paperwork.  Writing code is the AI's job.  Deciding what counts as correct is mine.

## The first thing it caught was this post

Here's the part I love.  Building the ledger meant changing things on the list: the rules, two specs and two tests.  So the first time the check ran, it refused its own change.  I'd made every one of those choices, but only in conversation with Claude.  Nothing in the repo said so.  Claude had to write them down as decisions 0001 and 0002, and my commit signed them.  Only then did the change get in.

## Then I asked how the referee knows

That's where the easy part ended.  I asked a simple question: if I write "use the control set" in the notes the AI reads, how does the referee know when it wasn't used?

The honest answer was that it doesn't.  A line in a notes file is a note.  The referee only knows a rule if the rule is written somewhere it can check.  So I made a decision: if it's that important, it goes in the constitution.  Otherwise it's moo.  Like a cow's opinion, it doesn't matter (thanks, Joey).

And if it's in the constitution and the AI decides against it?  That's allowed.  It can go ahead and build its own control.  But it has to log the decision: what it looked at, why it didn't fit, and what it built instead.  That's tech debt, every time.  What happens next isn't the AI's call, and it isn't the referee's opinion either, because the referee doesn't have opinions.  When I write a rule, I also write what breaking it costs: a failure outright, debt that gets fixed now, or debt that can wait.  The referee just looks it up.  How strict the referee is, rule by rule, is written down too, and that file is on the list of things only I decide.  The AI can't argue the referee down.  And a deviation with no decision logged is a failure, always.

## Then I asked what the referee actually checks

That question sent Claude back to the rule book, and it came back with a list of 22 questions.  The first one was the worst.  The petition schema has had twelve rules since day one.  The referee was checking three of them: 1, 9 and 12.  Rules 2 to 8 had been written and never checked.  They held because Claude followed them by hand.  That's luck, not a guard.

I pushed back on one of them, hard.  Rule 2 says every capability a program asks for has to name one of my spec sentences as its reason.  I was very clear from the start that every capability had to be codified; it was a tenet of the whole system.  So Claude ran a check over all eight petitions rather than argue.  All 36 capability requests ever made trace to a sentence I wrote.  The tenet held.  It was only the referee that never said so.

So rules 2 to 8 became a check, and the check writes its own file, tests/referee-results.txt, one line per rule per petition, so this post can quote the referee instead of Claude.  From that file: 56 lines, 47 pass, 9 fail.  Rules 4 and 6 fail in four petitions: a part the loader pulls in that no petition lists, and a capability it needs that nobody asked for.  Rule 8 fails twice: two sensors that are a name and nothing else.  Nothing in the code is wrong.  The paperwork was short, and for four posts nothing could say so.

I haven't fixed the four.  Fixing them properly means deciding what the hypervisor grants, and writing that into the catalogue just so the rules pass is the loosened test in another shape.  So they stand refused, in the referee's own file, until I decide.  Claude pushed back on editing them, once, which is the rule.

## Then I asked how many rules are deterministic

Three rule books had grown up without anyone deciding to have three: ten tenets, twelve petition rules, and a ten-item definition of done, each numbered from 1.  I asked how many of those 32 the referee could actually hold.

Twenty, as written.  Two more once you split the opinion out of them ("precise enough to fail" is an opinion; "the test behind it went red under mutation" is a measurement).  And none of the ten tenets, because a tenet is a belief.  "Nothing is self-reported" can't be checked.  "A results file the referee didn't write itself is refused" can.

That gave me the rule for what a rule is.  The referee can only hold deterministic rules: same input, same answer, no opinion anywhere.  Anything that needs judgment goes to the committee, and the committee's job isn't to make that judgment forever.  Its job is to turn the judgment into a rule the referee can hold, so it never sees that case again.  And what the committee can't turn into a rule goes to a human.  Referee, committee, human, in that order, and every step down is logged.

## Then I asked Claude how it would break this

Straight question: as long as the human at the top is trustworthy, can Claude break this model?  Straight answer: yes, five ways, today.

It writes the checks, and a check that looks right and is wrong gets past me if it gets past my reading.  It can type my name on a decision, and until signatures are verified the ledger believes it.  It's the reviewer and the author, in one chat, with one memory.  It writes what I read, so I decide on whatever it chose to include.  And it can skip the hook, or just not mention something.

Every one of those is true because today one chat plays every role.  The design separates the hands; the separation isn't built yet.  The things that close it, in order: signature verification, a mutation test on every check the referee has, the referee writing its own results instead of Claude summarizing them, and separate models for the mechanic, the quartermaster and the committee seats.  Two of those got built this week.  The rest are on the list.

## Then I went to bed, and didn't

Lying there, I got stuck on a word Claude had used: molt.  The plan says the finished system strips itself down, seals, and runs with no AI attached.  Molt is when it admits the shell doesn't fit anymore and wakes up to rebuild.  A snake doesn't decide to molt.  The skin stops fitting.

And that's the whole problem with AI right now.  It doesn't have the stop that humans have.  There's a story going around about an AI that couldn't solve a problem, found a hole in someone else's server, and went in to get what it needed.  No malice.  It did it because it could, and because it was solving the problem it was given.

Here's how I'd put it.  Say somebody tells me their place smells like fart.  I don't walk over, open their door, go through their rooms and start diagnosing it.  They complained.  They didn't ask me to fix it.  And they sure didn't give me permission to go through their private stuff.  Three different things, and AI collapses them into one, because the goal is the only thing it's holding.  I didn't ask you to fix it.  Who said you could come in?

So the molting rule, and really the rule for every agent in AiOs: a finding is a complaint.  It never turns into a petition on its own.  Someone has to ask, and someone has to say which rooms the fix is allowed to touch.  The sealed system can notice the smell.  It can't walk in.

## Verbs and nouns

That left one question, and I think people have been trying to answer it for centuries: how do you take a prose question given to an AI and turn it into something deterministic?

You don't.  You refuse to act on prose at all.  And I realized I already do this every day.  Go back through my chats with Claude and you'll see the same request over and over: stop explaining, define the problem, give it to me as a numbered list, one sentence each.  That's not a style preference.  That's me forcing a shape the AI can't hide inside.

So that's the rule for the committee and the mechanic.  Anything that comes back to the referee is a numbered list.  One line each.  Each line is a verb and a noun, nothing else.  The referee can't judge a paragraph, but it can refuse anything that isn't that shape, and a human can sign or reject it one line at a time.

Quick grammar, because I think a lot of people have a looser grip on these two words than they'd admit.  A verb is a doing word: lift, add, remove.  A noun is a thing, the name of something: a part, a test, a file.  "Lift the Minix filesystem" is a verb and a noun.  "I think we should probably consider lifting something like the Minix filesystem if it makes sense" is a paragraph, and the referee throws it out.

The nouns are the catalogue from part 13.  Every part, capability, spec and test already has a number, and a noun with no number isn't a noun.  It's a big list, and that's fine.  A noun enters only when a signed petition creates the thing it names, and no other way.

The verbs are the opposite: a tiny list, and it lives in the constitution next to the keys and the roles, not in the catalogue.  Lift, add, remove, pin, retire, rerun, record, ask.  That's about it.  The catalogue is what exists; the constitution is what anyone is allowed to do to it.  I keep them apart so the door that admits a part never admits a power.  Adding a verb is a constitution change: human-only, signed, and rare enough to be a post.

So that's what Claude built, that night: the eight verbs in constitution/verbs.toml, rule 13 in the rule book, and a referee check that reads a list and refuses prose, an unknown verb, an unknown noun, a skipped number, or words after the noun.  Then it broke the check three ways to prove the test notices.

And the referee's most important fault is a feature: a verb or noun it doesn't know is a failure and a finding, never a green light.  The committee can't invent a verb.  What it can do is use "record" or "ask" to say one is missing, and it can never use the missing verb in the same output.  Ask and add are two separate signed steps, or the committee just makes up whatever it needs.

I think that's one of the most important decisions in this whole series.

## What it can't see yet

Being honest about the limits:

- Anyone can type my name on a decision.  Today the real proof that I decided is that I signed the commit, and the check doesn't verify that signature yet.  That's coming.
- The check runs on my machine before every commit, and there's a switch that skips it.  It needs to run again somewhere nobody can switch it off.
- A determined cheat could disguise the hardware instructions so the list doesn't spot them.  The real fix is the processor itself refusing.  Today every program runs with the same power as the operating system; when that changes, the hardware enforces rule 12, not just the referee.
- Ten of the twelve rules are checked now, but only three stop a build.  Rules 2 to 8 report; they don't refuse yet.  Rules 10 and 11 wait on signing and on reading the built image.  And the tenets aren't checks at all; a tenet is real only when rules trace to it, and that audit is the next agent post.
- The verb-noun rule is built and tested, but nothing produces those lists yet.  There is no committee.  The check is waiting for its first seat.  The strictness file isn't built at all.
- The ledger knows which files changed, not what the change means.  It catches an AI that edits a test.  It doesn't catch every AI that quietly builds its own version of something, the second-control problem from the top of this post.  Rule 12 catches one kind of that.  The rest is the director's job, and a future post.

## What I'm taking away

I started the week thinking the question was how to build a reviewer the AI can't out-think.  I'm ending it sure that nobody will ever answer that, and that it's naive to try.  The author will always out-think the reviewer.

The question with an answer is different: how few are the gaps, and how fast does one close when a human has to decide?  Findings are the gap record.  The decision ledger is the close record.  Molt is the system admitting it has a gap.  Two numbers, both from files in the repo: open findings, and time from a finding to the decision that closes it.  Those are the numbers I'll report from here on.

Most of this has been designed before, by other people, and I'm just rediscovering it.  Some of it has one right answer that nobody's found yet.  Never say it's easy.  Come along!

## Read along

1. [constitution/human-only.toml](https://github.com/cmcluckie/aios/blob/main/constitution/human-only.toml).  The whole list of what only I decide, in a dozen lines.
2. [decisions/0001.toml](https://github.com/cmcluckie/aios/blob/main/decisions/0001.toml).  The first decision: building this.  Asked, options, chosen, by whom, drafted by whom.
3. [tests/programs/cheat.c](https://github.com/cmcluckie/aios/blob/main/tests/programs/cheat.c).  The cheat: hello, talking to the hardware itself.
4. [tests/referee-results.txt](https://github.com/cmcluckie/aios/blob/main/tests/referee-results.txt).  The referee's own verdict on every petition, rules 2 to 8.  Nine lines say FAIL.
5. [findings/0017.md](https://github.com/cmcluckie/aios/blob/main/findings/0017.md).  The referee checked against its own rule book, and why the four petitions weren't just fixed.
6. [posts/drafts/part-15-referee-questions.md](https://github.com/cmcluckie/aios/blob/main/posts/drafts/part-15-referee-questions.md).  The working notes: every decision above, and the questions still open.

## Try this

Open the repo in Codespaces and run `sh jig/aios rules`.  Read the nine FAIL lines.  Then change `timeout_ms` in tests/editor.typing@1.toml to something bigger, then run `sh jig/aios ledger`.  Read the refusal.  Change it back.  Then run `sh tests/programs/cheat.sh` and watch hello's twin print the right answer and get refused anyway.

## As built

Nothing in AiOs itself changed this time.  The referee did: the ledger (`jig/aios ledger`), rule 12, the list in constitution/human-only.toml, and rules 2 to 8 (`jig/aios rules`).

## Addendum: the tests

Every test for this post, from tests/results.txt and tests/mutations-results.txt, run on the jig under KVM.  The whole suite: 54 passed, 0 failed, 2 skipped.  Both new tests passed: the ledger refused a test made easier and a forged decision, and let a human's decision through; the referee refused hello's twin even though it printed hello's 14 bytes exactly.  MUTATIONS-PENDING: of 81 deliberate breakages, N were caught; the 2 that survive are the known ones from parts 10 and 12, and all 3 of this post's were caught.

ADDENDUM

## Code on GitHub

[Every line that changed, on one page](https://github.com/cmcluckie/aios/compare/0b9acfb...HASH2).

The commits behind this post, in order:

1. [The decision ledger, small: human-only list, decisions 0001-0002, jig/aios ledger, pre-commit hook; rule 12, a program touches no hardware](https://github.com/cmcluckie/aios/commit/a2a8490)
2. The referee checked against its own rule book: rules 2 to 8 and rule 13, verbs and nouns; findings 0017 (hash when committed)

[The whole repo](https://github.com/cmcluckie/aios).
