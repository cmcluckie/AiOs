# Reference for TST.hanoi.exact-moves@1: the standard four-disk solution, pegs A
# to C by way of B.  Runs on a normal computer, never on the jig; its output is
# saved once as tests/expected/hanoi.exact-moves@1.out.
import sys
def moves(n, a, b, c):
    if n:
        yield from moves(n - 1, a, c, b)
        yield f"Move disk {n} from {a} to {c}\n"
        yield from moves(n - 1, b, a, c)
sys.stdout.write(''.join(moves(4, 'A', 'B', 'C')))
