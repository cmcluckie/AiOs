What each test's output must be.  jig/test compares the machine's serial
capture with `<test>.out` byte for byte, or, where `<test>.check` exists, runs
that short sh script on the capture (and the trace or strace, if the test
makes one) and goes green on exit 0.

The .out files are made once, on a normal computer, never on the jig:

    printf 'Hello, world.\n'                    > hello.exact-bytes@1.out   (also hello.silent-boot@1.out, loader.runs-program@1.out)
    python3 ../fixtures/hanoi_ref.py            > hanoi.exact-moves@1.out
    python3 ../fixtures/life_ref.py             > life.exact-output@1.out
    python3 ../fixtures/calc_ref.py ../fixtures/calc.in > calc.transcript@1.out
    python3 ../fixtures/sort_ref.py 1          > sort.first-boot@1.out
    python3 ../fixtures/sort_ref.py 2          > sort.second-boot@1.out
    serial.in-order@1.out      the bytes 0 to 255, then 255 down to 0
    serial.receive-order@1.out a copy of ../fixtures/receive.in: 4096 bytes,
                               every value but 3 and 4, repeating in order

The references are written from the spec clauses, not translated from the C.
If a reference changes, its .out file is regenerated in the same commit.

Part 17:
    reader.find@1.out          jig/reader find, callers and calls, run once on a normal computer
                               against the pinned commit and checked against a grep of the same
                               file: serial8250_early_out has 1 definition and 10 calls
    scorecard.table@1.out      worked out by hand from the spec, with made-up costs for two made-up
                               models, before jig/scorecard existed; the script has to match it
