# Entry formats

What it means to "define a code exactly once."  One TOML file per code at a
version, in the directory its table owns.  File name is the code path with the
version: `index/cap/tty.write@1.toml`.  Every entry carries the same header;
the body is per table.  A field not listed here is a schema violation, not an
extension.  Closes finding 5; touches 2, 6, 7, 9, 11, 12.

## Header, every table

    code        = "CAP.tty.write@1"      # must equal the file name
    status      = "draft"                # draft | admitted | retired
    admitted    = ""                     # commit hash of the signed admission, or empty
    retired_by  = ""                     # code of the entry that replaces this one, or empty

An entry is immutable once `status = "admitted"`.  Changing it means
publishing the next version and setting `retired_by` on this one.

## PUR: a purpose  (constitution/purposes/)

    sentence    = "Write one line to the serial console and halt."
    holder      = "KEY.chris"            # who may amend or retire it

## KEY: a signing key  (constitution/keys/)

    public      = "ssh-ed25519 AAAA..."  # the key, SSH public format
    holder      = "Chris McLuckie"
    role        = "root"                 # root | seat | agent
    seat        = ""                     # for role = seat: security | performance | portability | stability | purpose-fit

Resolves finding 12: submitter, consumer and reviewer all become KEY codes, so
rule 1 covers them.  Seat names are not an enum in prose; they are whichever
KEY entries carry `role = "seat"`.

## CAP: a capability  (index/cap/)

    touches     = "the serial console, output direction only"
    mechanism   = "write(2) on the console file descriptor"
    scope       = "device"               # none | device | path | address | port
    scope_rule  = "a console device name: console[0-9]"   # regex the referee applies to a petition's scope value

Resolves finding 6: each CAP declares what its scope looks like, and `none`
means a petition's scope for it must be empty.

## CMP: a component  (index/cmp/)

    kind          = "program"            # program | kernel | driver | library | toolchain | hardware
    source        = "pool/CMP.hello@1/"  # directory under pool/ (ours) or lifted/ (upstream), nothing outside
    licence       = "MIT"
    depends       = []                   # CMP codes needed at run time; empty is a statement, not an omission
    build_depends = ["CMP.toolchain.c@1"]  # CMP codes needed to build; never in the image
    exposes       = []                   # CAP codes this component provides to others
    requires      = ["CAP.tty.write@1", "CAP.proc.exit@1"]   # CAP codes it needs
    scaffold      = false                # true: provided by the jig for now; rebuilt when its pool version is admitted
    as_built      = "index/cmp/hello@1.as-built.md"   # derived from source, regenerated on every admission
    reproducible  = ""                   # hash of the artifact two hosts both produced, or empty

Resolves finding 2 (`depends = []` says "nothing"), finding 11
(`build_depends` and `scaffold`), and lets rule 6 mean run-time dependencies
only.  `kind = "hardware"` is how a VM definition enters the index: a CMP
whose `exposes` are the device CAPs and whose source is the machine file.
Closes finding 10 without a new table.

## SPEC: an as-intended clause  (specs/)

    component   = "CMP.kernel.serial.8250@1"   # any CMP, not only the petition's primary
    clause      = "From power-on to halt, no byte reaches the serial port that a component did not write."
    author      = "KEY.chris"            # specs are human claims; never an agent key

Resolves finding 7: a clause may constrain a dependency.  Petition rule 2
becomes "the reason's SPEC belongs to a CMP in this petition's component
list."

## TST: a test  (tests/)

    binds       = "SPEC.hello.1@1"       # exactly one clause
    red_when    = "stdout differs from the 14 bytes 'Hello, world.\n'"
    timeout_ms  = 5000                   # red when exceeded; required
    isolates    = "CMP.hello@1"          # the one CMP whose fault a red run indicates
    runner      = ""                     # path to the executable form, or empty before one exists
    mutations   = 0                      # mutants this test has been seen to catch; 0 means never seen red

Resolves finding 9 (`timeout_ms`) and finding 8 (`isolates`: a test that
cannot name one component is two tests).  `mutations = 0` makes tenet 8
visible: an admitted component with any test at 0 is a finding.

## SNS: a sensor  (sensors/)

    proves      = "SPEC.hello.1@1"
    question    = "How long from power-on to the last byte of output?"
    unit        = "ms"
    red_above   = 1000                   # or red_below; exactly one
    consumer    = "KEY.seat.stability"
    exercises   = ["CAP.tty.write@1"]    # CAPs whose use this sensor evidences; empty allowed

`exercises` is the first answer to finding 13: a CAP no sensor and no test
ever evidences is a candidate for revocation under tenet 10.

## Origin  (lifted/<code>/origin.toml)

Every part under lifted/ carries one.  It records exactly enough to lift the
part again from upstream, and to prove the record against upstream.

    [upstream]
    name       = "linux"
    repo       = "https://github.com/torvalds/linux.git"   # where it was read
    canonical  = "git://git.kernel.org/..."                # the authority, if different
    commit     = "a5ebb76233b7..."                         # full hash
    committed  = 2026-10-10T16:57:30-07:00                 # that commit's date
    subject    = "..."                                     # that commit's subject
    licence    = "GPL-2.0-only"

    [lift]
    by         = "KEY.chris@1"
    on         = 2026-10-10T23:30:00-07:00
    into       = "serial8250.c"                            # the file in this directory

    [[lift.from]]                                          # one per upstream range
    file   = "drivers/tty/serial/8250/8250_early.c"
    blob   = "44ec209f..."                                 # object id of that file at the commit
    lines  = [[86, 98], [131, 152]]                        # inclusive, at that commit
    what   = "serial_putc, init_port"

    [cut]
    removed = ["..."]                                      # what the lifted lines held onto

`jig/upstream verify` checks each blob id against the commit; `jig/upstream
watch` lists upstream commits that touched each range since.

## As-built spec  (index/cmp/<code>.as-built.md)

Markdown, generated from source, regenerated on every admission.  Sections,
in order: what it does, every syscall or hardware access it makes, every
symbol it imports, every byte it writes anywhere, size of the artifact.  A
human never edits it; a human edits the SPEC it is diffed against.

## Still open after this file

Findings 1 (PUR is now a table, so closed), 3 (pending-admission state: the
header's `status` is the start, rule 3 in petition.md still needs rewording),
4 (detached signatures: plan item 1.6), and the bloat figure (needs a SPEC
clause to prove; likely a SPEC on the machine CMP).
