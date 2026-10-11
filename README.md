# AiOS

Pronounced AY-ohss.  Linux, re-admitted one justified line at a time.

## Press play

No commands needed.  Click **Code → Codespaces → Create codespace** on this
page (or open the folder in VS Code with a WSL or Linux terminal), wait for it
to set itself up, press **F5**, pick a flavor.  You will see it build, boot,
print, and shut itself off.  Everything you see is explained below.

An operating system that contains only what someone asked for.  Nothing enters
without a petition that names its purpose, a spec that says what it must do, a
test that can prove it wrong, and a signed commit that admitted it.  Anything
nobody asks for anymore comes back out.  Linux is the parts bin, read and lifted
from one segment at a time, never booted.

This repository is the constitution.  It is the only authority on what the
system contains and why.  The design is in [design.md](design.md); read it
first.  The plan is [plan/brief.md](plan/brief.md).  What went wrong and what
was decided is in [findings/](findings/).

## State

Petition 0000, hello world, boots: 170 lines across three segments, fourteen
bytes on the serial port and nothing else, the machine exits by itself.  57 of
those lines are lifted from Linux's serial driver; the rest are a loader that
runs one program and a program that only talks through the loader's door.
[findings/0002.md](findings/0002.md) lists every line no petition justifies.

Petition 0001, Towers of Hanoi, runs under the same loader binary with no
change to it.  The ladder of petitions after that is in the plan: each one a
program every CS student wrote once, chosen because it needs exactly one new
thing from the machine.

Built and run so far under software emulation.  The jig (the build host) is
declared in [jig/](jig/) and not yet installed.  Nothing has been formally
admitted; every entry is still `draft`.

## Layout

    constitution/   tenets and purposes; the part no vote can amend
    schema/         the petition format, entry formats, signing
    petitions/      one TOML file per petition
    index/          CAP and CMP entries: what can be named, what exists
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

## Running a flavor

    sh jig/aios list
    sh jig/aios run hello

`aios` is the referee and the builder as one shell script until they are real
components: `check` resolves every code in the flavor's petitions and refuses
by rule number and code; `build` collects the components and compiles them;
`run` boots the result on the flavor's machine.  Needs gcc with 32-bit support
and qemu-system-x86_64.  In VS Code, Terminal > Run Task > "AiOS: run flavor".

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

MIT, except lifted source, which keeps its own.  See [LICENSE](LICENSE).
