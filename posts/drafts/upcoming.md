# Upcoming stories, and the decisions behind them

Kept so the stories and the reasons for the order are written down before they
are built.  Recorded 2026-10-13.

## Decided, in this order

1. Decision ledger first (plan 3.6), small: a decision table in each petition,
   detectors on the diff, a human-only list.  The committee waits for it,
   because a committee can only be graded against a record that already exists.
2. Quartermaster next, read-only (plan 7.1, 7.5): it drafts capability labels
   for the parts we already have, and its answers are graded against the
   labels written by hand.  An exam before a job.
3. Committee after that (plan 8): seats graded on planted petitions with known
   answers, against the ledger.

## Stories, in order

- **The trap (the ledger).**  Re-enact the MIT licence: Claude slips a licence
  change into a commit without declaring it, and the referee fails the build:
  licence changed, no decision recorded, human-only kind.  The committee has
  one seat today, Chris.  Hook for the next one.
- **The exam (the quartermaster).**  The agent reads the source and draws each
  part's labels on its own; we grade it against the ones written by hand.
- **The jury (the committee).**  Seats on different models vote; they are
  graded against decisions Chris already made, with planted petitions.
- **Language matters (findings 0014).**  The words we use for the nucleus,
  its parts and the agents around it, why each one changed once, and the
  table mapping old words to new for readers who remember everything.  The
  post check now fails a post that uses a retired word.
- **Labels (findings 0013).**  The best security idea in Linux, given up
  because it was too much work for people, made practical by agents, with
  people still holding the decisions that matter.
