# Signing

Three things get signed: commits, admissions, and votes.  All three use the
same key format (SSH ed25519, `KEY.*` entries) and the same rule: a signature
never lives inside the bytes it signs.  Closes finding 4.

## Commits

Every commit is signed with a KEY entry whose `role` is `root` or `seat`.
Git carries the signature outside the tree, so this is already detached.  The
referee verifies with the public keys in `constitution/keys/`, never with the
host's `allowed_signers`; the repo is the only authority on who may sign.

## Admissions

An entry becomes admitted when a signed commit sets its `status` to
`"admitted"` and writes that commit's own hash into `admitted` on the next
commit (a commit cannot know its own hash).  So admission is two commits:

    1. status = "admitted", admitted = ""          signed by a root or seat key
    2. admitted = "<hash of commit 1>"             signed by the same key

The referee checks that commit 1 exists, is signed by a key allowed to admit,
and that the entry's bytes at commit 1 equal the entry's bytes now, minus the
`admitted` field.  No separate signature file.

## Votes (were reviews, until 2026-10-15)

A seat's vote is a decision, like every other choice in AiOs: one log,
decisions/, read by the referee.  There is no separate review file.  Chris:
"Decisions are a log.  Every decision the committee makes goes into the log
for the referee to review."

    decisions/<NNNN>.toml

    id      = "0042"
    date    = 2026-11-01
    kind    = "vote"
    touches = ["petitions/0000.toml"]
    asked   = "Approve petition 0000 at commit 84eb0fd?"
    options = ["approve", "reject", "counter"]
    default = "reject"
    chose   = "approve"
    why     = "SPEC.hello.2@1"                 # a catalogue number, or FND.NNNN@1 for counter
    by      = "KEY.seat.security@1"
    drafted = "KEY.seat.security@1"
    commit  = "84eb0fd..."                     # the commit whose petitions/0000.toml was voted on

The vote is signed the way every decision is: by the commit that adds it, which
the referee verifies against the seat's key in constitution/keys/ (plan 3.5).
One vote per commit, so each seat signs only its own.  Rule 10 reads:

    status = "approved" requires, for the commit the petition was last
    changed in, a decision of kind "vote" naming that petition and that
    commit from every KEY entry with role = seat, each added in a commit
    signed by that seat, none choosing reject, none choosing counter.

A petition changed after a vote is voted on again; the old votes name an
older commit and stop counting.  That is the point.

## Until seats exist

There are no seat keys yet.  Until there are, the root key reviews alone and
quorum is one.  The first admission of a seat key is itself an amendment the
root key signs, and from that commit forward quorum includes that seat.
