# AGT.referee@1, as built

What it does: reads the constitution and the catalogue and says yes or no, naming a rule and a catalogue number.  No model, no judgment.  C11, one file, pool/AGT.referee@1/referee.c, 546 lines, built on the jig with the host compiler (libc for files, one popen for the compiler and one for git's short hash).  pool/AGT.referee@1/referee is a twelve-line shell wrapper that builds it when the source is newer than the binary and hands over.  First written in shell on 2026-10-15 and rewritten in C the same night: the referee is permanent code, and C is the language of everything permanent in AiOs.  The shell version was the spec by example; the C one passes the same four probes and writes the same referee-results file line for line.

Commands: `check FLAVOR` (rules 1, 9, 19), `hardware CODE SRC` (rule 12, the list in constitution/instructions.toml, on the program's own assembly from the same compiler, CC and CFLAGS from the builder), `rules` (rules 2 to 8 on every petition, written to tests/referee-results.txt), `lines FILE` (rule 13, verbs from constitution/verbs.toml, nouns from the catalogue).  The decision ledger is jig/ledger, a script, because it reads git history.

Reads: constitution/, index/, specs/, tests/*.toml, petitions/, flavors/.  Writes: tests/referee-results.txt only.  Touches no hardware, opens no socket, runs no model.

Called by: jig/aios build (check before, hardware after), the probes in tests/programs/.  jig/ledger is called by jig/test (every commit since the last run) and .githooks/pre-commit.

Not yet: running the tester itself (plan 3.7), rules 10, 11, 14 to 16 and 20 as checks, a signed verdict file, a schedule for expiry, rules 17 and 18 (still said by the builder).  Hand-written, as the definition of done allows until plan 7.1.
