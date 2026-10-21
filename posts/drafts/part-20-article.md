*The story behind AiOs, part 20.*

## Same C, different program

Here's something I've wondered about for years.  When you compile C for DOS, and when you compile C for Linux, and when you compile C for the inside of a kernel, what's actually different?  It's the same language.  Often it's the same compiler.  So why won't a program built for one run on the other?

This part is where AiOs has to answer that for real, because Berkeley's `cat` is about to run on it.  So I asked Claude to walk me through it, slowly.  And here's the answer I didn't see coming: the C library one small program needs to print a file turned out to be seven times bigger than the kernel it runs on.

## The menu and the kitchen

`printf` isn't part of C.  Neither is `fopen`.  C the language can add numbers and call functions, and that's about it.  Printing is something somebody else wrote.

That somebody is the C library.  It's a big box of ready-made functions every C program leans on: open a file, print some text, get some memory.  And `stdio.h` is its menu.  It lists what's available and how you call it, and it contains no working code at all.  The library is the kitchen.

Here's the part that clicked for me.  Every C library ships its own copy of the menu.  Borland wrote theirs, Microsoft wrote theirs, the Linux people wrote theirs.  What has to match is the outside: `printf` takes a format and gives back a number, `fopen` gives you a `FILE`.  What's inside is anybody's business.  `FILE` itself is a different structure in every library, and your program never looks inside it, so it doesn't matter.

That's why the same `cat.c` compiles on DOS and on Linux, and why the program you get from one won't run on the other.  Same menu, different kitchens.

## Three doors

The kitchen still can't print anything.  Only the operating system can touch the screen or the disk.  So in the end, the library's job is to knock on the operating system's door and ask, in the exact way that system understands.

I wanted to see that for real, not in a diagram.  So Claude compiled the same six lines twice:

```
#include <stdio.h>

int main(void)
{
	fputs("hi\n", stdout);
	return 0;
}
```

Once with Open Watcom for DOS, the old Watcom compiler that's open source now, and once with musl for Linux.  Then it pulled the programs apart to find the exact spot where each library knocks.

On DOS, `fputs` hands off to the library's flush, which hands off to a little routine called `__qwrite`, and there it is:

```
mov    $0x40,%ah
int    $0x21
```

Put 40h in a register, which means "write," and ring interrupt 21h.  That's DOS's door.  The whole program is 4,716 bytes.

On Linux, musl's `fputs` ends up in `__stdio_write`, which puts 146 in a register, Linux's number for "write these pieces," and calls one tiny function.  Here's that function, the whole thing, from musl's own source:

```
1:	int $128
	ret
```

Interrupt 128 is `int 0x80`.  That's Linux's door, and AiOs has answered it since part 18.  This one is 16,240 bytes.

Inside a kernel there's no door to knock on, because you are the operating system.  Kernel code is built with no C library at all.  This is how AiOs's serial driver, taken from Linux, prints one character:

```
while ((inb(base + UART_LSR) & UART_LSR_THRE) == 0)
	;
outb(c, base + UART_TX);
```

Wait until the chip says it's ready, then put the byte straight on its port.  No menu, no kitchen, no door.  That's how every part of AiOs has been built until now.

The Linux program and the kernel were built by the same compiler, gcc, from C.  What changes is the library, and the door it knocks on.

## A different cat

Now a confession, because some of you will spot it.  The `cat` in this part isn't the one from part 19.

Part 19's came from Unix Version 7, 1979.  Its licence, a 2002 letter from Caldera, the company that owned it by then, asks for credit in any advertising for a product built from it.  AiOs is GPL-2.0-only, the same licence as Linux, and that's the kind of extra condition the GPL doesn't allow.  Reading V7's `cat` was fine.  Building it in wasn't.

So I swapped it for Berkeley's, from 4.4BSD-Lite, their 1994 release.  Berkeley's code had the same kind of clause, and their licensing office dropped it in 1999.  It's 251 lines instead of 63, the model read it from scratch and wrote a new request, and the old request stays in the repo as the record of what it wrote for V7.

## Linux's kitchens

Linux itself never came with a C library.  In the early to mid 1990s, Linux developers kept their own fork of the GNU one, called Linux libc.  Around 1997, when glibc 2.0 came out, the big distributions went back to glibc, and most Linux machines still use it today.

musl is the other kitchen.  Rich Felker released it in February 2011, written to be small, clean and readable, under the MIT licence.  Alpine Linux, the tiny system a lot of Docker containers run on, uses it.  It's the kitchen AiOs is getting.

## Seven times the kernel

I ruled that we take only the pieces of musl that `cat` actually links, each one recorded.  The builder linked Berkeley's `cat` against all of musl on the jig and wrote down what the linker pulled in: 82 source files.  Easy, I thought.

Except source files don't compile without their headers.  With every header they include, the start-up files that run before `main`, and musl's licence, the lift is 178 files.  Counted the way this series counts, C only, no blank lines, no comments, that's 11,900 lines of code.

Now the kernel `cat` runs on, everything it boots with: the loader, the gate that answers Linux's calls, the serial port, the disk and the file system.  Counted the same way, 1,650 lines.  AiOs wrote 1,046 of those, and 604 came from Linux.

So the library is seven times the kernel underneath it.  To print a file.  I did not see that coming.

And it ties right back to the menu and the kitchen.  Only 2,735 of musl's lines are kitchen, the code that actually does the work.  The other 9,165 are menu: headers, describing everything so every program agrees on what a `FILE` is and how `printf` gets called.

## The C library nobody asked for

A petition is a list of everything a program needs: the pieces of the system, what it's allowed to do and why, and the tests.  The model wrote two of them, one for V7's `cat` and one for Berkeley's.  It asked for the serial port, the disk and the file system.  It never asked for a C library.  Not once, and `cat` can't even compile without one.

Nobody had to notice.  `cat`'s own catalogue entry says "I depend on musl."  The referee's rule 6 compared that against the petition's list, saw musl wasn't on it, and said so: `PART.libc.musl@1 is depended on but not listed`.  No AI in that check.  It just compared two lists.

So how did musl get in?  I put it in, by hand.  Petition 0014 is the model's request with six things fixed, musl among them.  Claude drafted it and I signed it.  Hold that thought.

## Teaching the kernel to run a Linux program

Every program in AiOs so far was built by AiOs, for AiOs.  This one is a normal Linux program file, built the way any Linux toolchain builds it, and I wanted the kernel to take it as it is.  So I ruled that it loads at Linux's own address, 0x08048000, gets its arguments from the kernel command line the way Linux's first program does, and reads files read-only from the disk AiOs already had.

Claude wrote the kernel side, as in every part so far.  No model wrote kernel code in this one.  The loader learned to read a Linux program file's header, put each piece where the file says, and build the starting stack musl expects.  The gate learned thirteen of Linux's system calls, each as small as `cat` needs, each refusing everything else with Linux's own error number.  The loader went from 303 lines of code to 430, the gate from 302 to 545.

One thing musl asks for at start-up is 16 random bytes.  AiOs has nothing random, so it hands over the processor's cycle counter, and the spec says plainly that it isn't random.  I'd rather write that down than pretend.

The model's petition couldn't be built as it was.  Petition 0014 keeps its purpose, its permissions and their reasons, and its tests, and fixes six things, musl the biggest.  The model's own petition stays in the repo exactly as it wrote it.

## cat with no file

The first thing that worked was `cat` with no file at all.  It copies whatever you type until you say you're done.  That meant musl had started up on AiOs, set itself up, and was reading and writing through Linux's door.

Two things came up on the way.  AiOs had no way to say "end of input" from the keyboard, so Ctrl+D does it now, like on a Unix terminal.  And nothing outside the machine could see whether a program said it succeeded, so when a Linux program ends, the kernel prints `exit` and the number.  That second one turned out to matter.

## Works on my machine

Everything was green in Claude's workspace, under software emulation.  Then I ran it on the jig, my real machine running the tests under KVM, and the probe for Linux's calls died with `Guru Meditation #00000006`.

Vector 6 is the processor saying "I don't know that instruction."  The jig's compiler had used SSE, the newer vector instructions, 26 times in one loop.  A processor only runs those once the kernel promises to save those registers, and AiOs never makes that promise.  Emulation let it slide.  Real hardware didn't.

So programs built against musl are now built for the 486, the way musl itself is.  Turning SSE on is real kernel work and isn't in this series.  I'll take "emulation is not the jig" as the lesson I keep having to learn.

## A bug from 1994

Here's my favourite part.

The model's part 19 petition asked for a test that `cat` treats a missing file as an error.  Good instinct.  So the test asks `cat` to print a file that isn't there, then one that is, and checks that it says so and exits with 1.

Berkeley's `cat` printed the warning, printed the next file, and said `exit 0`.  Success.  It declares a variable for its exit status and never sets it.  Later BSDs fixed it by setting it to 1.  It went out that way in 1994's Lite and again in 1995's Lite2.

I ruled red first.  The test went in red, on Berkeley's code exactly as lifted, and stayed red on the jig.  Then the fix went in as its own change: two lines, `rval = 1;` after each warning, the same fix the descendants made.  The record of where it came from now says the file was changed, where, why, and what the changed file's fingerprint is, and the check that compares it against Berkeley's original knows the difference.  Green, `exit 1`.

A thirty-year-old bug, found by a test a model asked for.  I didn't plan that, and I love it.

## Red for three commits

Then I ran everything on the jig, every test and every deliberate breakage, as the final record.  Two tests went red that had nothing to do with `cat`.

They were from part 19.  They hand the builder a petition that names three things that don't exist yet, and check that the builder lists exactly those.  This part gave two of them real entries.  The builder was right, the test was out of date, and it had been red for three commits.  Nobody saw it, because every jig run since then ran only the tests for what was changing.  The test now uses made-up names that can never exist, and goes red if one ever does.

I knew a partial run isn't the record.  Now it's written down in the findings, so I can't pretend I didn't.

## Nothing checked my petition

I was reading this article back, the draft, and I got to "I put it in, by hand," and stopped.  Hang on.  What made me decide that petition?

Nothing, it turns out.  Claude checked.  Tests, specs, the rules themselves, all of those need a written decision under my key before they can change.  Petitions didn't.  The line on a petition that says who submitted it is just text, and the model's two drafts carry my key on that line too.  And the builder never even looked at whether a petition was approved.  Petition 0014 was "pending" and it got built anyway.

The only thing that held was that nobody but me can commit to the repo.  That means I signed a commit.  It doesn't mean I decided a petition.

So that's fixed, in this part.  A petition now only gets built if there's a decision under my key that names it, and that decision says who drafted it: me and Claude for the first twelve, the model for its two, Claude for 0014.  The test for it went red first, like always.  Then Claude's first fix went red too, because the shell quietly turned a decision naming "every petition" into a decision for each one that exists.  Same trap we fell into back in part 15.  Third time green.

The referee caught the model's mistake.  I caught the system's.  That's the order I want it in.

## The number

I said at the end of part 19 that I'd have a number.  Here's what the repo says.

The model's calls, for both of `cat`'s petitions, came to 58 cents: 13 in part 19 and 45 in this part.  For that I got two petitions with the right shape, the test that caught Berkeley's bug, and a C library it forgot to ask for.

Everything else was Claude and me: 370 new lines of kernel code, a petition fixed six ways, and about 12,000 lines of other people's code taken in, musl and Berkeley's `cat`, each file checked against its source.  On the jig that's 112 tests passing, and 264 deliberate breakages: 261 caught, and the three that weren't are known and written down.

## 1969: one machine

One more thing before the verdict, because it changed how I read all of this.

The first Unix was written for one machine, a PDP-7 at Bell Labs, in assembly.  No C yet.  It survived as a badly printed dot-matrix printout with notes scribbled on it.  In 2016 a group of Unix history people scanned it, read it back in, fixed the errors, wrote an assembler for it, and booted it in a simulator.

And it already had `cat`.  It's 145 lines of PDP-7 assembly, and it asks the kernel for things the way programs still do: `sys open`, `sys write`, `sys close`.  The door was there from day one.  What wasn't there was a menu.

## 1975: the menu

Then something happened that I think gets badly underappreciated.

In 1975, Unix Version 6 shipped with something called the Portable C Library, by M. E. Lesk.  The first thing its paper says is that C "now exists on three operating systems": Unix on the PDP-11, Honeywell's GCOS, and IBM's 370.  The library was built so the same C program would run on all three.

Around early 1976, Steve Johnson and Dennis Ritchie wrote a memo to their boss asking for a second machine, one that wasn't a PDP-11, to prove a whole operating system could move.  They got an Interdata 8/32 in April 1977 and finished the move by the spring of 1978.  In 1979, Unix Version 7 shipped `stdio.h`, the menu every C library still copies.  In 1989 it went into the ANSI C standard, so every library's copy had to match.

Whether portability was the stated purpose back in 1969, I don't know.  I love to reverse-engineer systems and figure out the architect's intent, and from 1975 on, it's right there in the record.  To me it doesn't read like something written for one machine and thrown away.  It reads like a framework.

## North Star, not roadmap

This is the bit I care about as a product guy.

I've been accused of over-architecting.  I don't think that's fair, and here's the distinction I make.  A roadmap is a yearly commitment: here's what we'll deliver to customers and to the people who depend on us.  A North Star is where I want the system to be in five, ten, fifteen years.  Very little of the North Star ever makes it into a given roadmap.  But it decides which doors you leave open.

Bell Labs shipped a fixed menu, a hidden kitchen and a memo asking for a second machine.  Small choices.  Fifty years later it's why Linux, BSD and macOS all run the same `printf`.

I think we lost some of that with Agile, chasing the immediate need.  I don't want waterfall back.  But with AI doing more of the building, I think the North Star and the product's mission statement are going to matter a lot more than they have.

[TO COME: Chris, your verdict on viability, in your words.]

## What I'm taking away

[TO COME: the piece this rung adds, and the end of the series.]

## Read along

1. [lifted/PART.libc.musl@1/src/internal/i386/defsysinfo.s](https://github.com/cmcluckie/aios/blob/main/lifted/PART.libc.musl%401/src/internal/i386/defsysinfo.s).  Linux's door, two lines.
2. [lifted/PART.cat@1/cat.c](https://github.com/cmcluckie/aios/blob/main/lifted/PART.cat%401/cat.c).  Berkeley's `cat`.  Look for `rval`, and the two lines that were added in 2026.
3. [lifted/PART.cat@1/origin.toml](https://github.com/cmcluckie/aios/blob/main/lifted/PART.cat%401/origin.toml).  Where it came from, and what was changed and why.
4. [pool/PART.gate@2/gate.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.gate%402/gate.c).  Thirteen of Linux's system calls, each as small as `cat` needs.
5. [petitions/0014.toml](https://github.com/cmcluckie/aios/blob/main/petitions/0014.toml).  The petition that got built, on top of the model's 0013.
6. [findings/0023.md](https://github.com/cmcluckie/aios/blob/main/findings/0023.md).  The plan, what I ruled, and everything that went wrong.

## Try this

Open `petitions/0014.toml` and delete the four lines that list `PART.libc.musl@1`.  Then run `sh pool/AGT.referee@1/referee rules`.  Rule 6 fails on petition 0014 and names the library: the exact mistake the model made, caught with no AI in the check.  Run `git checkout -- petitions tests` to put it back.
