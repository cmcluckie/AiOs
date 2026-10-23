*The story behind AiOs, part 21.*

## The mechanic

Early in his career, my dad was a mechanic.  He and my mom pinched and saved everything they earned to build a better life for me and my sister, and eventually they bought a Subway restaurant.  There's some debate in the family over whether my parents bought it because it was a viable business, or because my sister wouldn't stop eating there and was slowly bankrupting them.  For the record, my sister is a beautiful mother of two, and the Subway sandwiches had no effect on her whatsoever.  My dad retired happily at 55.  Mom took a little while longer; she took more convincing that she could actually retire.  But the mechanic thing stuck with me.

I rebuilt some of my own cars.  Tie rods, carburetors, brake pads, the basic stuff.  I never took it anywhere serious, but I liked knowing how it worked.

That started earlier.  My mom and I used to go to yard sales, and I'd come home with some crappy tape player and get the screwdriver out.  I didn't care if it played music.  I wanted to know how it worked.  My son George does the same thing now.  Sometimes taking something apart is more interesting than playing with it, and I think that's kind of cool, because that's exactly who I am.

## We left things broken on purpose

If you've followed along, you may have noticed we left a lot of things broken.  I could have fixed them on the spot, and you'd never have known they existed.  But that's no fun.  I wanted to come back and fix them, because that's who I am.

They're written down, every one, in the repo.  The kernel still answers four system calls with numbers only AiOs uses.  While one program waits for a key, every other program freezes.  A program can reach a little memory past what it was given.  The 16 "random" bytes a Linux program asks for at start-up are the processor's cycle counter, and the spec says so.  And AiOs never turns on SSE, the newer instructions, so a program built for a modern chip stops dead.

Each of those is a tape player on the workbench.

Usually, when I took stuff apart, it very rarely got put back together.  Taking it apart was far more fun, and I got so excited I never really looked at how to put it back together.  Well, fast forward a lot of years, and on this blog, we have no excuse but to put it all back together.  So that's what we're going to do now.

## Until now, the mechanic was Claude

[TO COME: the mechanic in the design, may propose and never install; when the clock gave up on the real chip in part 12, Claude was the one who figured out why.]

## Number one and number two

[TO COME: Claude on Amazon's servers against my RTX 3090 on the same fixes.  My prediction, written down before the run: Claude knocks it out of the park and the 3090 sucks.  The setup story: Ollama too old, upgraded, the first run at about 7 tokens a second, the second at about 105.]

## How we keep score

[TO COME: tokens per working line, a line counts only when every test passes, a failure means a better prompt, and if you don't know how to talk to an AI, ask the AI.  Why: the 3090 may be cheaper without looking it.  Dave Plummer's drag races.]

## Five fixes

[TO COME: each fix, skimmed: the test that went red first, what number one did, what number two did.]

## Why they broke

[TO COME: the pattern.]

## What I'm taking away

[TO COME: the rest of the closing.]

We may not fix things very often.  We may just learn how they work.  But you know what?  This is how new stuff gets built.

Come along!
