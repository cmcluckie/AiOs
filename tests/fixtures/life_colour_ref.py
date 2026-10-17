# Reference for TST.life.colour@1 and TST.life.screen@1: Conway's Game of Life,
# written from the rules in SPEC.life.2@1, not translated from life.c.  Runs on
# a normal computer, never on the jig.  Writes the bytes the program must send
# (life.colour@1.out), and, with --cells, the screen they must draw: 25 lines of
# 80 cells, each "A" + the VGA attribute in hex + the character, which is what
# tests/expected/life.screen@1.check compares the screendump's pixels against.
# usage: python3 life_colour_ref.py > life.colour@1.out
#        python3 life_colour_ref.py --cells > life.screen@1.cells
import sys
W, H, GENS, VW, VH = 256, 96, 200, 80, 24
live = set(); before = set()
cx, cy = W // 2, H // 2
for dx, dy in [(0, -1), (1, -1), (-1, 0), (0, 0), (0, 1)]:
    live.add((cx + dx, cy + dy))
for _ in range(GENS):
    counts = {}
    for (x, y) in live:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                if dx or dy:
                    p = (x + dx, y + dy)
                    if 0 <= p[0] < W and 0 <= p[1] < H:
                        counts[p] = counts.get(p, 0) + 1
    before, live = live, {p for p, n in counts.items() if n == 3 or (n == 2 and p in live)}
BORN, SURVIVED, EMPTY = "\033[1;33m", "\033[0;32m", "\033[0;34m"
# ANSI on a VGA screen: yellow is 3, bright adds 8; green 2; blue 1; on black
ATTR = {BORN: 0x0e, SURVIVED: 0x02, EMPTY: 0x01}
out = ["\033[0m\033[2J\033[H"]
head = f"generation {GENS}, population {len(live)}"
out.append(head + "\n")
cells = [[(0x07, c) for c in head.ljust(VW)]]
ox, oy = (W - VW) // 2, (H - VH) // 2
last = None
for y in range(oy, oy + VH):
    row = []
    for x in range(ox, ox + VW):
        p = (x, y)
        c = EMPTY if p not in live else (SURVIVED if p in before else BORN)
        if c != last:
            out.append(c)
        last = c
        ch = "#" if p in live else "."
        out.append(ch)
        row.append((ATTR[c], ch))
    out.append("\n")
    cells.append(row)
out.append("\033[0m")
if "--cells" in sys.argv:
    for row in cells:
        print(" ".join("A%02x%s" % (a, "_" if ch == " " else ch) for a, ch in row))
else:
    sys.stdout.write("".join(out))
