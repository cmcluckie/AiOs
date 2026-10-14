*The story behind AiOs, part 14.*

## Give me Notepad or die

I despise Unix text editors.  vi, Emacs, all of them.  Give me Notepad or give me death.

I love Notepad.  I think it's one of the best things ever written, because it has one purpose and it does exactly that one thing: it writes text.  Whoever decided to add tabs to Notepad, there is a special place in hell for that person.  And whoever decided it should remember your documents after you close them, damn you, damn you all to hell.

I'm a big fan of simplicity, and in a text editor, Notepad is the very definition of it.  So after yesterday's post, which was heavy on definitions, AiOs gets a text editor today, and I'm keeping it simple, pure and innocent.

## The green screen

There's a reason this matters to me beyond taste.

In 1997 I ran my first migration of users from a green-screen application to a Windows one.  They hated it.  Absolutely hated it.  And it took me a while to understand why.  It wasn't that the new one was worse.  It was how much they'd invested in the old one.  They had memorized every keystroke it took to do every single thing.  Which F key did what.  Which combination got you to which screen.  It was way beyond hitting Tab and having the cursor land in the right field.  All of that knowledge lived in their heads, and I was asking them to throw it away.

The early text editors were the same, and so were the early versions of Word.  vi still is.  You invest in learning it, and it pays you back in speed.  Plenty of people love that deal, and if you're one of them, no judgment.  Well, a little judgment.

## Keystrokes are a language too

Yesterday was about words.  It turns out keystrokes are words too: a language you have to learn before you can say anything.

The thing that ended my 1997 fight has a name.  In 1987, IBM published Common User Access, a rulebook for how programs should behave: Ctrl+S saves, the arrows move, F1 is help, and the commands should be on the screen, not in your head.  Windows followed it, and Notepad is its purest form.  You don't learn Notepad.  You just use it.

So that's the rule for this editor: there is nothing to learn.

## What I asked for

Six rules, and I wrote them myself:

1. Typing types: printable keys insert at the cursor, and the arrows, Home, End, Backspace, Delete and Enter do what they say.  No modes.
2. Ctrl+S writes exactly the text on screen to the file, replacing what was there.
3. Ctrl+Z undoes the last run of typing or deleting, and a second Ctrl+Z puts it back.  There's one level only.
4. Ctrl+Q quits.  If there are unsaved changes, it asks once: "Unsaved.  Ctrl+Q again to quit."
5. The bottom line always shows every command there is.
6. Saving over a file leaves the disk clean, with the old blocks freed and nothing lost.

One file, called notes.txt.  No tabs.  No remembering your last session.  The whole bottom line of the screen says ` Ctrl+S Save   Ctrl+Z Undo   Ctrl+Q Quit`, all the time, so you never have to remember anything.  The screen is 80 columns by 24 rows, the size of DEC's VT100 terminal from 1978, because that's the language the serial line speaks.

Fun fact I learned along the way: Ctrl+S and Ctrl+Q used to mean "stop sending" and "start again" on old terminals.  That's why some Unix editors still avoid them today.  Notepad's keys win here (and whoever thought Ctrl+C should stop a command, you can go hang out with the Notepad tab people).

## The one thing AiOs couldn't do

Every rung on this ladder asks the system for exactly one new thing.  I assumed the editor would ask for nothing: typing came in part 6, the disk in part 10.

Claude found the catch.  AiOs couldn't save over a file.  When Claude lifted Minix's filesystem out of Linux for part 10, it cut the code that frees a file's old blocks, because sort only ever wrote its file once.  An editor saves the same file over and over.

So this rung brings back a small, exact piece of Linux: the code that runs when a program opens a file to overwrite it.  It frees every block the old file held, then writes the new one.  That's 47 lines, nearly all of them Linux's own, each traced to the exact line it came from.  The kernel itself didn't change at all, apart from being built with the new filesystem.

## How the tests type

You can't test an editor by reading its output.  You have to type at it.

So the tests do.  Each one feeds in the exact bytes a terminal sends when you press keys, saves, quits, boots the machine again on the same disk, and reads what the editor shows when it opens the file.  "Helo world", back seven, insert an "l", and so on, until the second boot has to show exactly two lines, 20 bytes.  Then Claude broke the editor on purpose 14 different ways (the up arrow that stays put, the Ctrl+Q that doesn't ask, a freed block that never gets freed) to make sure every test notices.

## What I'm taking away

Every product I've ever managed has had someone in it who loved the old green screen.  I get it now in a way I didn't in 1997.  They weren't loving the screen.  They were protecting what they'd invested.

The best thing an interface can do is ask for no investment at all.  Put every command on the screen, and let typing be typing.  It took 305 lines.  Come along!

## Read along

1. [pool/PART.editor@1/editor.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.editor@1/editor.c).  The whole editor.  Find `case 0x13`: that's Ctrl+S.
2. [specs/editor.1@1.toml](https://github.com/cmcluckie/aios/blob/main/specs/editor.1@1.toml).  The first of my six rules, as the system reads it.
3. [lifted/PART.fs.minix@2/minix.c](https://github.com/cmcluckie/aios/blob/main/lifted/PART.fs.minix@2/minix.c).  Find `free_data`: the piece of Linux that came back.
4. [tests/fixtures/editor_keys.sh](https://github.com/cmcluckie/aios/blob/main/tests/fixtures/editor_keys.sh).  Every key the tests press, readable.

## Try this

Open the repo in Codespaces and run `sh jig/aios run editor`.  Type something, press Ctrl+S, then Ctrl+Q.  Run it again: your text is still there.  Now type something and press Ctrl+Q without saving, and watch it ask.

## As built

- [editor@1](https://github.com/cmcluckie/aios/blob/main/index/part/editor@1.as-built.md)
- [fs.minix@2](https://github.com/cmcluckie/aios/blob/main/index/part/fs.minix@2.as-built.md)
- [loader@7](https://github.com/cmcluckie/aios/blob/main/index/part/loader@7.as-built.md)

## Addendum: the tests

Every test for the parts above, from tests/results.txt and tests/mutations-results.txt.

ADDENDUM

## Code on GitHub

Every line that changed, on one page (hash when committed).

The commits behind this post, in order:

1. Petition 0007: the editor (hash when committed)

[The whole repo](https://github.com/cmcluckie/aios).
