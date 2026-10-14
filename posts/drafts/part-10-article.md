*The story behind AiOS, part 10.*

## Well, that got out of hand

Part 1 reached about 8,000 people.  I was expecting about 50.  Honestly, I hope it calms down a bit, and if it doesn't, that'd be fun too.  Either way, a lot more people are reading this than I planned for, so I owe you the honest version of why it exists.

## It's not a new Linux

I'm not building a new Linux.  I'm not an OS engineer, I'm not out to replace anything, and I wouldn't want to.  One commenter caught me getting the licence wrong, and it's fixed.  Expect more of that.

What I'm actually working on is how to do agentic development safely.  AI writes code now, lots of it, fast.  The question I care about is how you let it without having to trust it.  And what better place to test that than the one piece of software that says yes or no to everything else?  If the process holds up at the bottom, where every byte matters and nothing hides, it'll hold up anywhere.

## Defense and offense

Here's the picture in my head.  Most of a development team's week goes on the mundane stuff: checking, rechecking, chasing what changed, proving the old things still work.  That's what holds good teams back from building great software.

In AiOS the machine does the mundane part, and it's split into defense and offense.  Defense is the referee and the tests: dumb, stubborn checkers that say no and give you a rule number.  Offense is the sabotage script from part 8, which attacks our own code on purpose to prove the defense would notice.  The AI proposes, and neither side takes its word for anything.

That frees up the people.  Developers, QA and support get to spend their time on the product and the business: what should this do, for whom, and is it right?  Their roles get clearer, and they sit closer to the customer.

I wrote this series as a slow walk on purpose.  Partly because it's hard to explain a whole vision at once.  Mostly because I don't fully understand it yet either.  This stuff is new enough that I need to be slow-walked to it as well, and every post teaches me one more piece.

Which brings me to the biggest piece of operating system so far.

## Saving a file

Everything on AiOS until now has forgotten everything the moment you switched it off.  This rung is a program that remembers.  It sorts twenty numbers, saves the sorted line to a file on a disk, and the next time the machine boots, it finds that file and checks it's still right.

The twenty numbers come from the rand() function printed in Kernighan and Ritchie, the C book from part 1, starting from its own seed of 1.  The AI picked that, and I love it: the same twenty numbers on every machine, forever.  The sort is insertion sort, the one every first course teaches.  The program is the easy part.  The disk is not.

## A new rule: pick the one with history

There were two choices to make, and both had a modern answer and an old one.

The disk.  Every cloud virtual machine today uses a disk called virtio.  Linux's code for it is about 7,000 lines, most of it general machinery shared by every virtio device.  The old answer is IDE, whose registers go back to the hard disk controller in the IBM PC/AT in 1984.  You talk to it the way we already talk to the serial chip: write a few numbers to a few ports, then read 512 bytes.

The filesystem.  The old answer here is Minix.  It's the filesystem from the teaching operating system in Andrew Tanenbaum's textbook, and it's what Linus Torvalds had on his own disk when he started writing Linux in 1991.  It was Linux's very first filesystem, and it's still in Linux today.

Both times the old one was also the smaller one.  So I made it a rule: when there's a choice between things that would each do the job, the default is the one with the history.  Minix on an IDE disk is roughly the machine Linux was born on.

## What we took, and what we cut

Linux's Minix code is about 2,400 lines, and it's tangled into Linux's shared file machinery more than 200 times: its general idea of a file, its cache, its locks.  So we took the parts that are Minix's own: where the maps of free space and the list of files live on the disk, how to find a free spot, how to add a name to a directory, which block holds which part of a file.  Then we rewrote the edges: every block gets read, changed and written straight back.

The top of the file we took from still says "Copyright (C) 1991, 1992 Linus Torvalds".  I find that genuinely moving.

The disk driver came from Linux too, from the code that still handles those old IDE disks.  All up, the lines we've taken from Linux went from 172 to 638.  About 150 of those are our own glue, holding Linux's pieces together.  Every range is recorded down to the commit and the line, the same as before, and the script that checks them against Linux says all 97 recorded ranges match.

## No clock, so no counting

Here's the bit that tied back to part 9.  Every time Linux waits on a disk, it times the wait: check the disk, wait 10 microseconds, check again, give up after 10 milliseconds.  This machine has no timer.  Adding one would break the "one new thing per rung" rule, and faking the wait by counting loops would be the exact turbo-button bug we spent the last post killing.

So our driver waits as long as it takes, watching the disk's own "busy" light.  If nobody's there at all, it notices and says so.  On a real dead disk it would hang, and that's written down for the day this runs on real hardware with a real clock.

Same reason, smaller detail: the file it saves is dated 1 January 1970.  The machine doesn't know what day it is.  That's its own rung, later.

## A second opinion nobody here wrote

This is my favourite part.  Linux comes with a tool called fsck.minix that checks a Minix disk for damage.  Nobody on this project wrote it, and nobody here can talk it into anything.  So after AiOS saves its file, the test hands the disk to fsck.minix.  It says the disk is clean and there's one file on it, called sorted.

That's defense from outside the building.  Then came the offense.  We broke the filesystem three ways: used a block without marking it as taken, forgot to grow the directory, made a file nothing points to.  fsck.minix caught every one, and two of the three our own program would never have noticed: it reads the file back just fine.

All up: eight new tests, fifteen new sabotages, fourteen caught.  The one that got away is honest and a bit scary.  When the program saves, the last thing it does is tell the disk "write everything you're holding, now".  Take that line out and every test still passes, because the emulator writes straight to the file anyway.  On a real disk with a cache, pull the plug at the wrong moment and the file is gone.  It's written down as a known survivor, the same as the three in part 8.

## The fine print

All of this ran under software emulation in a container, not on the jig.  Files can be up to 7 KiB, in one directory, and once saved they can't be overwritten or deleted, because nothing has asked for that yet.  Hello world, Hanoi, Life and the calculator all still run byte for byte the same on the new loader.

And the piece this rung added to the process: a checker from outside.  Every test before this one, the AI wrote.  This one, Linux wrote.

## Read along

1. [pool/PART.sort@1/sort.c](https://github.com/cmcluckie/aios/blob/main/pool/PART.sort@1/sort.c).  The program.  Find `rand`, copied from K&R, and the short insertion sort under it.
2. [lifted/PART.fs.minix@1/minix.c](https://github.com/cmcluckie/aios/blob/main/lifted/PART.fs.minix@1/minix.c).  Read the comment at the top for what came from where and what was cut.  Then `add_link`: that's Linux's code for adding a name to a directory, almost line for line.
3. [lifted/PART.disk.ata-pio@1/ata.c](https://github.com/cmcluckie/aios/blob/main/lifted/PART.disk.ata-pio@1/ata.c).  The disk driver.  Look at `command`: seven port writes and the disk knows what you want.
4. [pool/MACHINE.serial-disk@1/machine.sh](https://github.com/cmcluckie/aios/blob/main/pool/MACHINE.serial-disk@1/machine.sh).  The new machine: the old one, plus one IDE disk.  It formats its own disk the first time, like it came from the shop.
5. [tests/expected/fs.fsck-clean@1.check](https://github.com/cmcluckie/aios/blob/main/tests/expected/fs.fsck-clean@1.check).  The second opinion nobody here wrote, in a dozen lines.
6. [findings/0010.md](https://github.com/cmcluckie/aios/blob/main/findings/0010.md).  What we found, including the missing clock and the flush no test can see.

## Try this

Press F5 and pick AiOS: sort, twice.  The first time it saves the file; the second time it finds it.  Now open pool/PART.sort@1/sort.c, change the seed `next = 1` to the year you were born, and run it again.  It sorts twenty new numbers, finds last time's file still on the disk, and tells you it's different.  The disk remembered.  Delete out/disk.minix to start over.

## As built

- [loader@5](https://github.com/cmcluckie/aios/blob/main/index/part/loader@5.as-built.md)
- [fs.minix@1](https://github.com/cmcluckie/aios/blob/main/index/part/fs.minix@1.as-built.md)
- [disk.ata-pio@1](https://github.com/cmcluckie/aios/blob/main/index/part/disk.ata-pio@1.as-built.md)
- [sort@1](https://github.com/cmcluckie/aios/blob/main/index/part/sort@1.as-built.md)
- [machine.serial-disk@1](https://github.com/cmcluckie/aios/blob/main/index/machine/serial-disk@1.as-built.md)

## Addendum: the tests

Every test for the parts above, from tests/results.txt and tests/mutations-results.txt.  All of them ran under software emulation, not on the jig.

ADDENDUM

## Code on GitHub

[Every line that changed, on one page](https://github.com/cmcluckie/aios/compare/1d14857...8247a91).

The commits behind this post, in order:

1. [Petition 0005: save the sorted output, on an IDE disk, in Minix](https://github.com/cmcluckie/aios/commit/8247a91)

[The whole repo](https://github.com/cmcluckie/aios).
