*The story behind AiOs, part 16.*

## Play authentic

For the musicians: remember Gibson's "play authentic" video?  2019, the one they pulled after the outcry ([Guitar World has the story](https://www.guitarworld.com/news/gibson-pulls-controversial-youtube-video-following-outcry)).  I'm only bringing it up to make you cringe.  Hold that cringe, I'm coming back to it.

## The pace

The first six of these went out a day apart, which is crazy in itself.  Crazier: most of the series was written in about two days and scheduled.  The articles are already on GitHub, ready to go, so read ahead if you like.

## Why is this weirdo doing this?

That's the question, and it's fair.  Linux has been around a while, everybody trusts it, and nobody needs a better Linux.  You're right.  That's not why.

One, I needed a creative outlet.  Two, I've wanted to do a creative blog series like this for a long time.  Three, I want to show how far you can actually go with AI, so I picked the most complicated project I could think of.

And I gave myself a rule.  The posts go up as fast as possible, they're written with AI, and they have to be me.  I sign off that I contributed 90% of every article.  The article, not the code.  Claude writes the code.  The articles are mostly me dictating and Claude editing.

## Written by AI, big surprise, still me

So the secret's out: I'm using AI to write these.  Are they still me?  Yes.  My voice, my tone, my stupid sense of humour.  Not fake.

Here's where the cringe comes back.  Every trade has people saying "we don't need it, I'm against it," and they're half right, because there are two ways to use AI.  The inauthentic way is "AI, write this," and people pick that out in about two sentences.  The other way is to change how you think about it.  You give it as much information and as much of your personality as you possibly can, and you stay in the room the whole time.  As Gibson says, play authentic.  Code authentic.  It goes for writing code and writing articles exactly the same.

At some point I'll prove it.  One post in this series will be written the inauthentic way: a separate Claude, a synopsis, "write a blog," slipped in without saying which one.  If anybody's reading, I'd love to see who finds it.

## Who's ever unit tested hello world?

Here's the bigger trust thing.  Don't write the code, write the spec.  On this project I'm telling myself not to look at the code, and so far I haven't looked at a single program besides hello world.  The only screens I've looked at are the passes and fails.

You're thinking: Chris, you're crazy, you need to look at the application.  Fine.  Tell me who's ever written a unit test for a hello world program, and then come back and ask me that.

I haven't looked on purpose.  That's the whole point: to get the real speed out of these tools, we have to trust them with some of this.  Should you go as far as I have?  Absolutely not.  Can you trust it a little more than you do, with good governance and good QA around it?  Absolutely.  People make mistakes too, me included, plenty of them.  That's why we invented reviews and tests in the first place.

## I recorded my band without listening

A lot of musicians are scared of this.  I'm not.  I think AI could completely change how music gets written, refined, edited and marketed.

My band just started advertising, and we needed to record a practice.  Instead of a sound guy, I gave Claude, on my studio MacBook, a spec of what I wanted it to sound like.  During the whole practice, without listening to the music, only the levels, it kept making adjustments to get everything where the spec said.  I couldn't add anything once we started.  That was a huge trust thing for me.  It came out sounding really freaking good.  I'm not sharing it here, it's not a business thing, but if you want to find it, it's out there.

## Then I realised I'd never seen it

And that brings me to this part.  I've been purposely not looking at the output, because I trusted the tests.  And everything goes to a serial port anyway.  I can watch it scroll by in a terminal, but nothing is really being shown to a person.

Claude pointed out I could have run every program myself on the jig any time, `sh jig/aios run calc`, and I never had, because the tests were green.  Looking is the test you write when you can't write a test.  Anything that bugs me when I actually use it is a rule nobody wrote down.

So this part brings in the monitor.

<!-- TO COME AFTER THE BUILD AND THE JIG RUN.  Nothing below is written until it happens.

## Petition 0009, the screen
VGA text mode, 80 by 25, a character and a colour per cell at a fixed address.  Life in colour, the first thing you can see.  A new machine with a display, a new capability.

## The kernel is built from the petition (plan 5.8)
Only the display flavour wants the VGA part, so the kernel stops dragging in everything it has ever gained.  The referee's four FAIL lines from part 15 should go to zero: say what the file says.

## The tester gets eyes
QEMU's screendump: pixels, not bytes.  What turns it red.

## The cheat rule 12 can't see
Writing to the screen's memory isn't a forbidden instruction.  Plan 9.13.

## What I'm taking away

## Read along / Try this / As built / Addendum: the tests / Code on GitHub
-->
