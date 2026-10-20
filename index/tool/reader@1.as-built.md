# TOOL.reader@1 as built

Hand-derived from jig/reader on 2026-10-17, brought up to date 2026-10-20 (the sources), pending the generator (plan 7.1).
A scaffold: a shell script standing in for a part.

## What it does
Answers questions about one source, Linux unless told otherwise, from one
commit and nothing else.  The commit is the one in the source's pin file
(READER_PIN overrides it for tests).  It reads the source out of a clone with git, so the working tree of the clone never
matters.  The same question gets the same answer; no model is in it.  It decides
nothing and changes nothing in the clone or the repo.

## Sources
`--source NAME` comes before the question; without it the source is Linux.
- linux: the commit in jig/linux.pin, out of the clone at LINUX (default ../linux).
- v7: Unix Version 7 (1979), the commit in jig/v7.pin, out of a clone of the
  Unix history repo at V7 (default ../unix-history-repo).  Added in part 19.
- bsd: 4.4BSD-Lite (Berkeley, 1994), the commit in jig/bsd.pin, out of the
  clone at BSD (default the V7 clone, which holds both).  Added in part 20.
Every rule below holds for all three.  find, callers and calls need cscope's
index of Linux, so for v7 and bsd they are refused with exit 1, saying they are
only Linux's.  An unknown source is refused with exit 1.

## The questions
- pin: prints the commit.
- list DIR: the entries of a directory at the pin, folders ending in /.
- read FILE FIRST LAST: those lines, each as FILE:LINE: text.
- check FILE FIRST LAST: prints ok and the range if the file and lines exist.
- ports FILE FIRST LAST: the lines in that range that call inb, inw, inl, outb,
  outw, outl, the _p forms, or the ins/outs string forms.
- find NAME, callers NAME, calls NAME: cscope's definition, callers and callees,
  each as FILE:LINE: FUNCTION: text, sorted by file and line.

## Refusals
Exit 1 with the reason on stderr for: a file that is not a file at the pin; a
directory that is not a directory at the pin; lines past the end of the file;
LAST before FIRST; FIRST under 1; a number that is not whole; a name that is not
letters, digits and underscore; more than READER_MAX lines (default 200) asked
for at once; a name cscope finds nothing for; an unknown question.  Answers from
list, find, callers and calls are cut to READER_MAX lines, with a note on stderr
of how many were left out.  Exit 2 when it cannot run here: no clone, no pin,
the clone lacks the pinned commit, or cscope is not installed.

## Everything it touches
- Reads: jig/linux.pin, jig/v7.pin or jig/bsd.pin; the clone at LINUX, V7 or
  BSD (see Sources), through git cat-file, show, ls-tree and archive.
- Writes: only a cache folder for cscope, $TMPDIR/aios-reader-<first 12 of the
  pin>-<checksum of READER_DIRS> (default /tmp), holding a copy of the pinned
  tree's .c, .h and .S files, cscope.files and cscope.out.  Built once, reused
  after.  READER_CACHE moves it.
- No network, no ports, no registers.

## Size
124 lines in jig/reader (shell).  Needs git, awk, sed, tar, cksum and cscope.
