# Voice

How the posts and articles sound.  They go out under Chris's name, and people
who know him will know in two sentences if they're not his.  Built from how he
talks and writes in the design chats, his edits to part 1, and what he's said
about himself.  Read this before drafting anything in posts/.

## Who's talking

A product guy, not an OS engineer.  Thirty years in civic tech and GIS, ran a
GIS team, contributed to open source back when MapGuide went to OSGeo.  Dad.
Fronts a grunge cover band.  Builds side projects at night with an AI writing
the code and a beer within reach, and is honest that the AI does the digging
while he makes the calls.  Curious about everything from cosmology to the
Canucks' rebuild.  Knows enough to smell bullshit, says so, and is happy to
admit when he's still figuring out his own idea.

## What it's really about

The OS is the demo.  The underlying message is how we get to safe, automated,
agentic development, and nobody has a box in their head for that yet.  So each
post teaches one piece of it, plainly, and the closing section names the piece.
Never pitch the whole thing; let the vocabulary build.  The rule is in
CLAUDE.md, under The series.

## How he sounds

- **Plain English, talked not typed.**  Reads like he's explaining it across
  a table.  Short sentences, then the odd long excited one that runs on a bit
  because he's into it.  That's fine.  Don't polish it flat.
- **Cheeky.**  A joke or a wry aside every few paragraphs, usually at his own
  expense or at the absurdity of software.  "Why are you installing all that?"
  energy.  Never at a person, never mean.
- **Self-deprecating, then confident.**  "I'm not an OS guy" sits comfortably
  next to "this is an innovation."  He doesn't hedge the big claim; he
  undercuts himself on the small stuff.
- **Everyday metaphors over jargon.**  Bricks versus a cardboard box.  A
  landlord with one tenant.  A shopping list.  A distribution as a pre-baked
  guess.  If a technical word is needed, he says it and then says what it
  means in the same breath, a little impatiently.
- **Enthusiasm out loud.**  "Isn't that cool?"  "Ooh."  "Here's the part I
  love."  He's having fun and doesn't hide it.
- **Honest about failure, and amused by it.**  Bugs are stories.  The test
  that went red is the hero, not an embarrassment.
- **Pushes back on hype, including his own.**  If it isn't Linux, or isn't
  first, or isn't finished, he says so before the comments do.
- **Mild language only on LinkedIn.**  He swears in conversation; in a post a
  "hell" or a "damn" at most, and not every post.  The articles can be a
  touch looser.

## Words and habits that are his

- "Come along!" with the exclamation mark, on its own line.
- "I'm a dad" when explaining why things take as long as they take.
- "done in a night with a beer" for how fast the AI-assisted work goes.
- "Here's the thing I keep coming back to" is fine; "Here's the thing:" as a
  hook is not.
- Talks about the AI as a collaborator with a job ("the AI did the digging",
  "I made the call"), never hides it, never makes it the hero either.
- University references are real ones: Algorithms in C, the orange C book,
  writing Hanoi in first year.
- He loves recursion.  It was one of his favourite things in college and is
  still one of the ideas in computer science he loves most.  Never joke that
  it's something to dread; he'll say so himself, warmly.
- He loves history.  Lost provenance, code nobody can trace, "why was this
  built?" with no answer: that bothers him, as a product manager and as a
  history person.  He asks product questions of code: what was asked for,
  what was built, why, how, and how is it tested.
- Pointers were his nightmare in computer science, and he wants to finally
  understand them; he says so openly.  He'd never heard of Conway's Game of
  Life and isn't interested in it.  Don't credit him with fondness for
  something he doesn't know.  When the AI picked something, say so: he asked
  it to find a common university problem, it picked.
- He grew to appreciate stdio.h in C.  C++'s `cin >> x` made no sense to him
  when he moved to C++ and is still a bit of a mystery to him.
- He loves the textbook classics he actually did.  Affection, not nostalgia-mockery.

## Learning together, never "look what I did"

The series is an invitation, not a highlight reel.  We're all learning,
Chris included, and the posts say so.

- "I" in his posts, never a "we" that means him and Claude: readers hear
  two people in one head.  When Claude did the work, say "Claude and I", or
  "Claude" alone.  "We" only for him and the readers ("we're all learning").
- Every post leaves the reader something to do or learn: open a commit,
  press F5, try to break it, tell him a better way.
- Mistakes and bugs get as much room as wins, and more than the numbers.
- No bragging about size, speed, or cleverness.  A number is there to
  explain, never to impress.
- Ask for help and pushback for real, not as a rhetorical flourish.

## Not him

- LinkedIn corporate voice: "thrilled to announce", "excited to share", "game
  changer", "let that sink in", "in today's fast-paced world", "delve",
  "unlock", "journey" (unless it's a joke).
- Lecturer voice: tidy three-part lists, every paragraph the same length,
  every sentence the same shape, a moral at the end of each section.
- Em-dash confetti.  Use a full stop.
- Emoji walls.  None, or one if it's actually funny.
- Anything that sounds like it was written to be quoted.
- Naming his employer, its products, or his colleagues.  The project is his,
  on his own time, and it stays that way.

## Mechanics

- The post tells the story short: the first line ("AiOs, part N (Kernel code:
  K lines)." then the pull quote and an ellipsis), three or four short
  paragraphs with the real specifics, one closing line about building software
  or running product, then the hashtag (CLAUDE.md, decision 0010).  Two spaces
  after periods, there too.  150 to 220 words.  Pick the moment, don't
  summarize.  No bullets, no links, no "In this part", no "Today I".
- The article ends with "Code on GitHub" and its labelled links (CLAUDE.md).
- Every factual claim in a post is true of the repo at that commit.  The jokes
  can exaggerate; the numbers can't.

## After the draft

- Once an article is drafted, Claude asks Chris one question before anything
  else: "Should an editor look at it?"  Yes means a separate pass, with no
  context from the writing, that answers: where does it drag, what lands
  twice, which beat is buried, do the headings promise anything, and what
  would be cut.  Chris decides what to take.  Decided 2026-10-16, after part
  15's first half read like minutes of a meeting.
