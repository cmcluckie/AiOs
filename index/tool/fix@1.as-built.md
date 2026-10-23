# TOOL.fix@1 as built

Hand-derived from jig/fix and fix/prompt.txt on 2026-10-22.
A scaffold: a shell script on the jig standing in for the mechanic (design.md's
agents table: fix and refactor; may propose a patch; may not install anything or
skip the quartermaster).

## What it does
`sh jig/fix TEST` hands one red test to one model and judges what comes back.
1. Copies the repo (without .git, out/ and runs/) to out/fix/TEST-tryN/base and
   runs the test there.  A test that is not red is refused before any call
   ("nothing to fix"); one that cannot run on this host is exit 2.
2. Sends the model the prompt (fix/prompt.txt, or FIX_PROMPT), then the test's
   file, the spec clause it checks, and the FAIL line it printed.
3. Answers the model's look calls from the copy only: list a folder, read a file
   (numbered, at most 400 lines at a time), find a word in pool, specs, tests,
   index, findings and jig.  A path that starts with /, holds .., or reaches .git
   is refused.
4. Stops at submit_fix (a path and a whole file), at FIX_TURNS calls (40), or when
   the model ends its turn without one.
5. Refuses a fix for anything but an existing file under pool/, unjudged.
   Otherwise copies the repo again, puts the fix in that copy, and runs jig/test
   there: every test, or the prefixes in FIX_SUITE.  A pass needs the red test
   PASS, no FAIL anywhere, and a summary of 0 failed.
6. Records the run with jig/runs: item fix-TEST, the model, the try (FIX_TRY), the
   prompt's fingerprint, every call's input and output tokens added up, the result
   (pass, fail or refused), and the lines changed: lines removed plus lines added,
   by diff.  The conversation is input.txt; the diff is output.txt.

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
jig/fix 136 lines (shell); fix/prompt.txt 7 lines.
