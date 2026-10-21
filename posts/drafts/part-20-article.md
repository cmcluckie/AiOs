*The story behind AiOs, part 20.*

## The question I've always had

Here's something I've wondered about for years.  When you compile C for DOS, and when you compile C for Linux, and when you compile C for the inside of a kernel, what's actually different?  It's the same language.  Often it's the same compiler.  So why won't a program built for one run on the other?

This part is where AiOs has to answer that for real, because Berkeley's `cat` is about to run on it.  So I asked Claude to walk me through it, slowly, and this is what I got.

## A different cat

First, a confession, because some of you will spot it.  The `cat` in this part isn't the one from part 19.

Part 19's `cat` came from Unix Version 7, 1979.  The model read it, wrote its request, and the builder listed what was missing.  Then I looked at the licence.  The V7 code comes with a 2002 letter from Caldera, the company that owned it by then.  It lets anyone use it, as long as they credit Caldera in any advertising for a product built from it.  AiOs is GPL-2.0-only, the same licence as Linux, and an advertising rule is the kind of extra condition the GPL doesn't allow.  Reading V7's `cat` was fine.  Building it into AiOs wasn't.

So I swapped it for Berkeley's.  This `cat` comes from 4.4BSD-Lite, Berkeley's 1994 release, the same line of Unix a generation on.  Berkeley's code had the same kind of advertising clause, and in July 1999 their licensing office sent out a letter dropping it, which leaves a licence the GPL is fine with.  It's 251 lines instead of 63, and the model had to read it from scratch and write a new request.  The old request stays in the repo as the record of what it wrote for V7.

## The menu and the kitchen

`printf` isn't part of C.  Neither is `fopen`.  C the language can add numbers and call functions, and that's about it.  Printing is something somebody else wrote.

That somebody is the C library.  It's a big box of ready-made functions every C program leans on: open a file, print some text, get some memory.  And `stdio.h` is its menu.  It lists what's available and how you call it, and it contains no working code at all.  The library is the kitchen.

Here's the part that clicked for me.  Every C library ships its own copy of the menu.  Borland wrote theirs, Microsoft wrote theirs, the Linux people wrote theirs.  What has to match is the outside: `printf` takes a format and gives back a number, `fopen` gives you a `FILE`.  What's inside is anybody's business.  `FILE` itself is a different structure in every library, and your program never looks inside it, so it doesn't matter.

That's why the same `cat.c` compiles on DOS and on Linux, and why the program you get from one won't run on the other.  Same menu, different kitchens.

## Three doors

The kitchen still can't print anything.  Only the operating system can touch the screen or the disk.  So in the end, the library's job is to knock on the operating system's door and ask, in the exact way that system understands.

[TO COME: real compiler output, side by side, for one tiny program:]

- On DOS the library knocks with `int 21h` and puts 40h in a register.  That's DOS's door.
- On Linux the library knocks with `int 0x80` and puts 4 in a register.  That's Linux's door.  AiOs has answered this door since part 18.
- Inside a kernel there's no door to knock on, because you are the operating system.  Kernel code is built with no C library at all, and it talks to the hardware itself.  That's how every part of AiOs has been built so far.

The compiler is the same in all three.  The library is what changes.

## 1969: one machine

The first Unix was written for one machine, a PDP-7 at Bell Labs, in assembly.  No C yet.  It survived as a badly printed dot-matrix printout with notes scribbled on it.  In 2016 a group of Unix history people scanned it, read it back in, fixed the errors, wrote an assembler for it, and booted it in a simulator.

And it already had `cat`.  It's 145 lines of PDP-7 assembly, and it asks the kernel for things the way programs still do: `sys open`, `sys write`, `sys close`.  The door was there from day one.  What wasn't there was a menu.

## 1975 on: the North Star

Then something happened that I think gets badly underappreciated.

In 1975, Unix Version 6 shipped with something called the Portable C Library, by M. E. Lesk.  The first thing its paper says is that C "now exists on three operating systems": Unix on the PDP-11, Honeywell's GCOS, and IBM's 370.  The library was built so the same C program would run on all three.

Around early 1976, Steve Johnson and Dennis Ritchie wrote a memo to their boss asking for a second machine, one that wasn't a PDP-11, to prove a whole operating system could move.  They got an Interdata 8/32 in April 1977 and finished the move by the spring of 1978.  In 1979, Unix Version 7 shipped `stdio.h`, the menu every C library still copies.  In 1989 the ANSI C standard wrote it into law.

Whether portability was the stated purpose back in 1969, I don't know.  I love to reverse-engineer systems and figure out the architect's intent, and from 1975 on, it's right there in the record.  They weren't writing it for one machine and walking away.  They were building a framework.

## North Star, not roadmap

This is the bit I care about as a product guy.

I've been accused of over-architecting.  I don't think that's fair, and here's the distinction I make.  A roadmap is a yearly commitment: here's what we'll deliver to customers and to the people who depend on us.  A North Star is where I want the system to be in five, ten, fifteen years.  Very little of the North Star ever makes it into a given roadmap.  But it decides which doors you leave open.

Bell Labs didn't ship portability in 1975.  They shipped a library with a fixed menu and a hidden kitchen, and a memo asking for a second machine.  Small choices.  Fifty years later it's why Linux, BSD and macOS all run the same `printf`.

I think we lost some of that with Agile, chasing the immediate need.  I don't want waterfall back.  But with AI doing more of the building, I think the North Star and the product's mission statement are going to matter a lot more than they have.

## Linux's kitchens

Linux itself never came with a C library.  In the early to mid 1990s, Linux developers kept their own fork of the GNU one, called Linux libc.  Around 1997, when glibc 2.0 came out, the big distributions went back to glibc, and most Linux machines still use it today.

musl is the other kitchen.  Rich Felker released it in February 2011, written to be small, clean and readable, under the MIT licence.  Alpine Linux, the tiny system a lot of Docker containers run on, uses it.  It's the kitchen AiOs is getting.  Berkeley's `cat` uses 80 of its pieces [TO COME: how many lines, counted on the jig].

[TO COME: what it took to bring musl in, the kernel loading a Linux program file, the system calls, the halfway point, cat printing a file, the 1995 bug, and the number.]
