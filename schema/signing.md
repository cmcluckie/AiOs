# Signing

Three things get signed: commits, admissions, and reviews.  All three use the
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

## Reviews

A review is a signature over a petition at a commit.  It lives in its own
file so each seat signs the same bytes and nobody's signature changes what
the next seat sees.

    reviews/<petition id>/<seat>.toml

    petition   = "0000"
    commit     = "84eb0fd..."              # the commit whose petitions/0000.toml was reviewed
    seat       = "KEY.seat.security@1"
    verdict    = "approve"                 # approve | reject | counter
    reason     = "SPEC.hello.2@1"          # a catalogue number, or a findings file path for counter
    signature  = "-----BEGIN SSH SIGNATURE----- ..."

The signed bytes are exactly: the contents of `petitions/<id>.toml` at
`commit`, followed by one newline, followed by the ASCII verdict.  Nothing
else, so the same review file can be regenerated and verified by anyone with
the repo.  Signing and verifying use `ssh-keygen -Y sign` and `-Y verify`
with namespace `aios-review`.

The `[[review]]` blocks and `[quorum]` leave the petition file.  Quorum moves
to the schema: the referee counts review files for the petition's current
commit against the seats present in `constitution/keys/`.  Rule 10 becomes:

    status = "approved" requires, for the commit the petition was last
    changed in, a valid review file from every KEY entry with role = seat,
    none with verdict reject, and none with verdict counter.

A petition changed after a review is reviewed again; the old review files
point at an older commit and stop counting.  That is the point.

## Until seats exist

There are no seat keys yet.  Until there are, the root key reviews alone and
quorum is one.  The first admission of a seat key is itself an amendment the
root key signs, and from that commit forward quorum includes that seat.
