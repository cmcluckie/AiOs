# TOOL.agent@1 as built

Hand-derived from agent/ on 2026-10-29.  The layered mechanic (findings 0027), in
Rust (item 9): a runtime that knows nothing of AiOs, and the director and builder
program on top of it.  Not built yet: the prompt trial, the mentor and its log,
the shift log, and the track-record check.

## What it does
The runtime (agent/runtime), ported from CivicCanada as logic (agent/origin.toml):
- react: the ReAct loop.  System prompt for the role, first message for the job;
  calls capped; a call whose arguments cannot be read answered, not fatal; one
  reminder on a stop without a tool; a full context told from a reply cut at its
  limit; hooks before each call (room, money, time), for each tool, after each
  turn, and with the tokens after each reply.
- model: one call.  Gear 1 is Ollama's chat API; gear 2 is Bedrock's Converse
  API, the conversation handed over in Converse's shape and back in Ollama's.
- sigv4: AWS Signature Version 4 for Bedrock, checked against AWS's published
  examples.
- room: tokens as characters over four; a tool answer cut at a limit with
  CivicCanada's marker; the oldest tool answers dropped past a budget, the newest
  four kept.
- prompts: the prompt library, NAME@N.txt, with git's fingerprint for each.
- money: prices per million tokens from a price list, and the month's room asked
  of a command the caller names.
- transport: reqwest, or with the `fake` feature saved replies (tests only).

The program (agent/mechanic, the `agent` command):
- `agent note JOB TRY TOKENS SECONDS [FILE...]` (SPEC.agent.1): jig/test's output
  in, the result note out.
- `agent plan FILE` (SPEC.agent.2): a part plan checked, every field named when
  refused, written back with its defaults.
- `agent chat MODEL NUM_CTX MAX_TOKENS` (SPEC.agent.3, SPEC.agent.6): one call;
  AGENT_GEAR=2 for Bedrock.
- `agent build JOB` (SPEC.agent.4): one builder.  A copy of the repo at the job's
  commit (git archive, without runs/ and out/); the suite run on the jig first and
  the test must be red; then the loop with look (list, read, find; every look
  says why; answers cut at 8000 characters) and submit_fix (the paths checked
  before anything is written: existing files of one part under pool/, every part
  of the path a name).  The judge sends the fix to the jig as a tar into one ssh
  command and takes the suite's output back; a pass needs the red test to print
  PASS and no FAIL, and every test green before to print PASS and no FAIL.  A
  paid builder asks the job's share and the month's cap before each call; a job
  stops at its deadline.  Writes out/agent/JOB-tryN/ and records the run with
  jig/runs (bench, peak context, ended).
- `agent direct PLAN` (SPEC.agent.5): one director.  Tools job, search_log,
  search_prompts, ask, done.  The program keeps the tries, the minutes (passed to
  each builder as its deadline), the part's dollars and the stuck rule (the same
  failures on the same test, or no drop), takes done only from the notes, and
  folds the director's older turns into a standing written from its log.  Writes
  out/agent/PART/ (log.md, jobs/, director.json, report.md, question.md when it
  stops) and records the director's own run.

## Every syscall or hardware access
Files under its out folder and the repo it is given.  Child processes, each with
its arguments as separate words and no shell between: git and tar for the copy
(through `sh -c` with the values as arguments, never in the script), diff, tar and
ssh for the judge, jig/runs, and itself as a builder.  The one string that reaches
a shell is the command ssh runs on the jig, built only from values checked to be
letters, digits and a few marks.  The network only through transport.rs.

## Every symbol it imports
The Rust standard library; serde, serde_json and toml; reqwest (blocking, rustls
with ring) for HTTP; hmac, sha2, sha1 and hex for the signing and the
fingerprint.  111 crates in all on Linux, pinned by agent/Cargo.lock.

## Every byte it writes anywhere
Standard output: each command's result.  Standard error: one line a step unless
AGENT_QUIET.  Files: out/agent/, and one run in runs/ per builder and director.
On the jig, only the throwaway folder its ssh command makes and removes.

## Size
2985 lines of Rust: the runtime 1085 (react 294, model 271, sigv4 177, room 109,
money 84, transport 84, prompts 59), the program 1900 (build 507, direct 496,
plan 179, judge 160, main 149, look 148, record 113, note 83, stuck 58).  Built
with warnings as errors; `sh agent/make` in WSL builds out/agent/agent.
