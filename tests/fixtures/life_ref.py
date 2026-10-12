# Reference for TST.life.exact-output@1: Conway's Game of Life, written from the
# rules in SPEC.life.1@1, not translated from life.c.  Runs on a normal computer,
# never on the jig; its output is saved once as tests/expected/life.exact-output@1.out.
# usage: python3 life_ref.py > life.exact-output@1.out
import sys
W, H, GENS, VW, VH = 256, 96, 200, 80, 24
live = set()
cx, cy = W // 2, H // 2
# the R-pentomino, centred:  .##  /  ##.  /  .#.
for dx, dy in [(0, -1), (1, -1), (-1, 0), (0, 0), (0, 1)]:
    live.add((cx + dx, cy + dy))
for _ in range(GENS):
    counts = {}
    for (x, y) in live:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                if dx or dy:
                    p = (x + dx, y + dy)
                    if 0 <= p[0] < W and 0 <= p[1] < H:   # dead beyond the edge, no wrap
                        counts[p] = counts.get(p, 0) + 1
    live = {p for p, n in counts.items() if n == 3 or (n == 2 and p in live)}
out = [f"generation {GENS}, population {len(live)}\n"]
ox, oy = (W - VW) // 2, (H - VH) // 2
for y in range(oy, oy + VH):
    out.append(''.join('#' if (x, y) in live else '.' for x in range(ox, ox + VW)) + "\n")
out.append("128 MiB: refused\n")   # 128 MiB asked of a 64 MiB machine
sys.stdout.write(''.join(out))
