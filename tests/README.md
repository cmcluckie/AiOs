Tests are written to fail.  One TOML file per TST catalogue number: the SPEC clause it binds, the condition under which it goes red, and the location of its executable form once one exists.  A test earns trust only after it has been seen to fail for the right reason; mutation runs are how that is proven.  The tester owns this directory and shares no model or prompt with the mechanic.

Running them: `sh jig/test` runs every test and writes results.txt; `sh jig/mutate`
breaks the code on purpose, one line at a time from mutations.txt, and checks the
named test goes red, writing mutations-results.txt.  Expected outputs and checks
are in expected/ (its README says how they were made); probe programs that exist
only to be tested against are in programs/; inputs and reference programs are in
fixtures/.
