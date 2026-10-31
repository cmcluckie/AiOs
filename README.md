# AiOs

Pronounced AY-ohss.  Linux, re-admitted one justified line at a time.

## The series

The story of building it, told on LinkedIn, one part every Tuesday and Thursday.  The date is the day
each part goes up; the files are here before then, so read ahead if you like.
Each file holds the short post, the title and the full article.

| Part | Date | Title |
| --- | --- | --- |
| 1 | 2026-10-11 | [170 lines](posts/2026-10-11.html) |
| 2 | 2026-10-12 | [The rulebook](posts/2026-10-12.html) |
| 3 | 2026-10-13 | [Zero lines](posts/2026-10-13.html) |
| 4 | 2026-10-14 | [Ours and theirs](posts/2026-10-14.html) |
| 5 | 2026-10-15 | [The landlord learns what it owns](posts/2026-10-15.html) |
| 6 | 2026-10-16 | [The machine listens](posts/2026-10-16.html) |
| 7 | 2026-10-18 | [Definition of done](posts/2026-10-18.html) |
| 8 | 2026-10-20 | [Where's the kernel?](posts/2026-10-20.html) |
| 9 | 2026-10-22 | [The turbo button](posts/2026-10-22.html) |
| 10 | 2026-10-24 | [Saved like it's 1991](posts/2026-10-24.html) |
| 11 | 2026-10-26 | [From KERNAL to kernel](posts/2026-10-26.html) |
| 12 | 2026-10-28 | [Hi, I'm the other one](posts/2026-10-28.html) |
| 13 | 2026-10-30 | [Language matters](posts/2026-10-30.html) |
| 14 | 2026-10-31 | [Give me Notepad or die](posts/2026-10-31.html) |
| 15 | 2026-11-03 | [Unplug from the matrix](posts/2026-11-03.html) |
| 16 | 2026-11-05 | [I never looked](posts/2026-11-05.html) |
| 17 | 2026-11-10 | [The exam](posts/2026-11-10.html) |
| 18 | 2026-11-12 | [The gate](posts/2026-11-12.html) |
| 19 | 2026-11-17 | [The petition nobody wrote](posts/2026-11-17.html) |
| 20 | 2026-11-19 | [Found a 30 year old bug hidden in a basic UNIX command](posts/2026-11-19.html) |
| 21 | 2026-11-24 | [Why can't we just tell it not to do that?](posts/2026-11-24.html) |
| 22 | 2026-11-26 | [Disqualified](posts/2026-11-26.html) |

![AiOs: the nucleus at the centre, the agents that build and guard it around it, and the jig underneath](docs/aios-components.png)

## Press play

No commands needed.  Click **Code → Codespaces → Create codespace** on this
page (or open the folder in VS Code with a WSL or Linux terminal), wait for it
to set itself up, press **F5**, pick a flavor.  You will see it build, boot,
print, and shut itself off.  Everything you see is explained below.

An operating system that contains only what someone asked for.  Nothing enters
without a petition that names its purpose, a spec that says what it must do, a
test that can prove it wrong, and a signed commit that admitted it.  Anything
nobody asks for anymore comes back out.  Linux is the parts bin, read and lifted
from one part at a time, never booted.

This repository is the constitution.  It is the only authority on what the
system contains and why.  The design is in [design.md](design.md); read it
first.  The plan is [plan/brief.md](plan/brief.md).  What went wrong and what
was decided is in [findings/](findings/).

## State

Fourteen petitions build, and every one of the ladder's programs still runs
under the latest kernel it was built for.  Petition 0000, hello world, is 170
lines across three parts, fourteen bytes on the serial port and nothing else.
The ladder after it: Towers of Hanoi (nothing new), Life (memory), a
reverse-Polish calculator (input), a stopwatch (time), sort saved to a Minix
disk (a disk and a filesystem lifted from Linux), two programs at once (the
loader becomes a kernel), a console editor (saving over a file), the referee
as the first agent in the pool, Life in colour on a VGA screen, programs in
ring 3 behind a gate (`int 0x80`, Linux's numbers), and Berkeley's cat from
4.4BSD-Lite, linked against musl and run as a normal Linux program file, its
petition drafted by a model and decided by a human.  Each kernel holds only
the parts its flavor's petitions name; what older kernels expose that nobody
asked for is listed in [constitution/extras.toml](constitution/extras.toml)
and only shrinks.

The jig is a NixOS VM on Hyper-V and runs every test under KVM;
[tests/results.txt](tests/results.txt) and
[tests/mutations-results.txt](tests/mutations-results.txt) are its last run,
written by the runner, never by hand.  The referee (`pool/AGT.referee@1`, C)
checks rules 1 to 9, 11, 12, 13, 19, 21 and 22 of
[schema/petition.md](schema/petition.md); the rest are written and not yet
enforced ([findings/0025.md](findings/0025.md)).  Nothing has been formally
admitted; every entry is still `draft`, and commit signatures are checked by
Chris's shell, not yet by the referee.

## Layout

    constitution/   tenets and purposes; the part no vote can amend
    schema/         the petition format, entry formats, signing
    petitions/      one TOML file per petition
    index/          CAP, PART, MACHINE and TOOL entries: what can be named, what exists
    specs/          SPEC clauses: as-intended behaviour, one testable sentence each
    tests/          TST entries: each bound to the clause it can turn red
    sensors/        SNS entries: question, threshold, consumer
    pool/           admitted source we wrote, one directory per component at a version
    lifted/         admitted source from upstream, each part with an origin record saying exactly where it came from
    flavors/        one TOML file per flavor: the set of petitions one machine runs
    findings/       what the schema or the plan could not express, and what was decided
    plan/           the project plan, brief and full
    jig/            the gen-zero build host, declared before it existed
    pi/             the bare repository that refuses unsigned pushes
    posts/          the series: each day a LinkedIn post and article in one HTML file, and its image

## Running a flavor

    sh jig/aios list
    sh jig/aios run hello

`jig/aios` is the builder: `build` runs the referee on the flavor's petitions,
compiles only the parts they name, links the kernel, and runs the referee on
the image; `run` boots it on the flavor's machine.  The referee itself is
`pool/AGT.referee@1/referee`.  Needs gcc with 32-bit support and
qemu-system-x86_64.  In VS Code, Terminal > Run Task > "AiOS: run flavor".

## How this is made

Chris McLuckie decides and signs.  An AI (Claude) reads source, drafts entries,
and stages files; it never commits.  The rules that bind both are in
[CLAUDE.md](CLAUDE.md).  This GitHub repository is a mirror; the authority is a
bare repository that verifies every signature on push.

## Upstream

Nothing is merged from Linux.  Parts are lifted, with line-numbered provenance
in `origin.toml`, and watched:

    LINUX=/path/to/linux sh jig/upstream verify    # the records match upstream
    LINUX=/path/to/linux sh jig/upstream watch     # what upstream touched since

A change that matters is a new version of the part through the door.

## Licence

GPL-2.0-only, the same as Linux, which AiOs is built from.  Lifted source also
keeps its upstream notices.  See [LICENSE](LICENSE).
