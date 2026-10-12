# Reference for TST.sort.first-boot@1 and TST.sort.second-boot@1, written from
# SPEC.sort.1@1 and SPEC.sort.2@1: the rand() printed in Kernighan and Ritchie,
# section 2.7, from seed 1, twenty numbers mod 1000, sorted.  Runs on a normal
# computer, never on the jig.  Argument: how many boots on one fresh disk.
import sys
nxt = 1
def rand():
    global nxt
    nxt = (nxt * 1103515245 + 12345) & 0xffffffff    # unsigned long is 32 bits there
    return (nxt // 65536) % 32768
line = ' '.join(str(n) for n in sorted(rand() % 1000 for _ in range(20))) + '\n'
out = ''
for boot in range(int(sys.argv[1])):
    out += 'sorted 20 numbers: ' + line
    if boot == 0:
        out += f'nothing saved yet; saved "sorted", {len(line)} bytes\n'
    else:
        out += f'found "sorted" from last time, {len(line)} bytes: the same\n'
sys.stdout.write(out)
