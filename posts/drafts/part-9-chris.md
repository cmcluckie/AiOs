# Part 9, Chris's own words (2026-10-11).  Raw; the article is drafted from this.

Well, first off, that should be the thesis behind this article, is I hate modern video games. I love the old ones. And I could never understand how programmers could make the mistake of when the turbo switch was turned on, the video game either ran faster or slower. And always found that annoying. That I couldn't play games on certain computers. So that needs to be in there somewhere, but written a lot better.

---
Notes for the draft (Claude):
- The thesis: the turbo-button bug is exactly the stopwatch problem.  Old games
  timed themselves by counting loops on the CPU, i.e. ticks with no idea how
  fast a tick is.  Faster CPU, faster game.  The turbo button existed because
  of it: switching turbo OFF slowed a PC back down to original-IBM-PC speed so
  those games were playable.
- Our stopwatch has the same trap: the processor's counter is ticks, not
  seconds.  "You need a clock to read a clock."  The rung is about not making
  the 1985 mistake: learn how fast the counter ticks, and prove it with the
  sensor (10 s by the host's clock reads 10 s).
- Check before publishing: the turbo-button facts (which direction, 4.77 MHz).
