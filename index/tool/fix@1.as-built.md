# TOOL.fix@1 as built

Hand-derived from jig/fix and fix/prompt.txt on 2026-10-22.
A scaffold: a shell script on the jig standing in for the mechanic (design.md's
agents table: fix and refactor; may propose a patch; may not install anything or
skip the quartermaster).

## What it does
`sh jig/fix TEST` hands one red test to one model and judges what comes back.
1. Copies the repo (without .git, out/ and runs/) to out/fix/TEST-tryN/base and
   runs the suite there (every test, or the prefixes in FIX_SUITE), to know which
   tests are red before any fix.  A test that is not red is refused before any call
   ("nothing to fix"); one that cannot run on this host is exit 2.
2. Sends the model the prompt (fix/prompt.txt, or FIX_PROMPT), then the test's
   file, the spec clause it checks, and the FAIL line it printed.
3. Answers the model's look calls from the copy only: list a folder; read a file,
   from a first line to a last, given as two numbers, as FIRST-LAST, or as the
   fields start_line and end_line, at most 400 lines at a time, headed with the
   file, the lines shown and how many it has, then each line numbered; find a word in a folder or file it names, or in pool,
   specs, tests, index, findings and jig.  A path that starts with /, holds .., or reaches .git
   is refused.
4. Stops at submit_fix (files: one or more, each a path and its whole content; a
   single path and content is taken too), at FIX_TURNS calls (40), or when
   the model ends its turn without one.
5. Refuses a fix for anything but existing files under pool/, all of one part
   (pool/PART.x@n/), unjudged.  Otherwise copies the repo again, puts every file
   of the fix in that copy, and runs jig/test
   there, the same tests.  A pass needs the red test PASS and no test FAIL that
   was not already FAIL before (decision 0060): tests already red are other jobs.
6. Records the run with jig/runs: item fix-TEST, the model, the try (FIX_TRY), the
   prompt's fingerprint, every call's input and output tokens added up, the result
   (pass, fail or refused), and the lines changed: lines removed plus lines added,
   by diff, over every file of the fix.  The conversation is input.txt; the diff is output.txt.

The suite in each copy is told where the upstream clones are (LINUX, MUSL, V7, BSD,
beside the repo unless set), so the tests that read them run there as they do in the
repo, instead of skipping.

## What it never does
Changes the repo: the model's look and the judge each work in their own copy, and
a green fix is a proposal Chris decides on, like any other change.

## Everything it touches
- Reads: the repo, fix/prompt.txt, tests/, specs/.
- Writes: out/fix/TEST-tryN/ (the copies, the conversation, the suite's output),
  runs/ (one run).
- Calls: jig/test in its copies; jig/model for every call (the price check and the
  cap are jig/model's); jig/runs to record.

## Size
jig/fix 183 lines (shell); fix/prompt.txt 7 lines.
