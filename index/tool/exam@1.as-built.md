# TOOL.exam@1 as built

Hand-derived from jig/sit, jig/judge, jig/model, jig/runs and jig/scorecard on
2026-10-17, pending the generator (plan 7.1).  Scaffolding: shell scripts that
stand in for the agent that will do this work.

## What it does
Sits a model for one exam item (exam/items.toml) and writes down what happened.

- sit ITEM: sends the model the question, the part's fixed header, and
  exam/prompt.txt as the system prompt.  The model may call one tool, the
  reader (TOOL.reader@1), and ends by writing the C file.  The file is judged;
  on a fail the model gets the failing test's line and its own file back and
  tries again, up to SIT_TRIES (3).  A try is also over when it passes
  SIT_TURNS (25) model calls, when a call is refused, or when the reply has no
  usable file (recorded as refused).  Each reply caps at SIT_MAXTOKENS (8000).
  Exit 0 if a try passed, 1 if all failed, 2 if it cannot run here.
- judge ITEM DIR: copies the repo to a temp folder, puts the model's .c file
  where the hand lift's is, runs the item's tests with jig/test, and prints
  pass or fail with the lift's code lines.  It passes only if every test is
  green.  Exit 2 if a test skipped.  The repo is not touched.
- model converse REQ OUT, model ping: one call to Bedrock's Converse API, signed
  by curl.  Refused first if the model has no row in jig/prices.toml or the
  month's cap has no room for MODEL_MAX_COST.
- runs record, count, spent, budget: writes runs/<id>/ (run.toml, input.txt,
  output.txt); counts code lines (non-blank, outside comments); sums the
  month's cost; refuses a call that could pass jig/budget.toml's cap.
- scorecard: reads runs/ and the hand lifts and prints, per model and item, one
  line per sitting: PASS or FAIL, lines kept against the hand lift's, tries,
  cost and the prompt's fingerprint; then items passed, sittings, total spent
  and cost per passing sitting.  Items never sat print "not sat".

## What a run records
id, date, item, model, version, sitting (one start of sit), prompt (first 8
characters of the git hash of exam/prompt.txt), try, input, output, cache read
and cache write tokens, cost_usd, result (pass, fail or refused), lines.  Cost
is tokens times the model's prices in jig/prices.toml, cached tokens at their own
prices.  A model with no price, or no cache price when it reports cached tokens,
is refused, never costed as free.  Prompt caching marks the system prompt, the
tool and the last message of every call.

## Everything it touches
- Reads: exam/items.toml, exam/prompt.txt, the part's header, the hand lift,
  jig/prices.toml, jig/budget.toml, runs/, ~/.aios/aws.
- Writes: runs/; out/sit/ (each try's messages, replies, final answer and lift);
  temp copies of the repo, deleted on exit.
- Network: model alone, to bedrock-runtime.<region>.amazonaws.com over HTTPS.
  The key file must be mode 600 and stays outside the repo.  MODEL_FAKE swaps in
  saved replies for tests and calls nothing.
- No ports, no registers.

## What it does not do
It does not tell the model what machine the part must run on; the pass is the
tests' word alone.  A try that fails because of that gap stays a fail.

## Size
sit 121 lines, judge 56, model 78, runs 123, scorecard 79: 457 lines of shell.
Needs jq, curl, awk, sed, tar and git.
