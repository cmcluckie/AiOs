#!/bin/sh
# Probe for TST.share.all-exit@1: hello and Hanoi, each built for loader@1,
# packed into one archive and run together under the share flavor's kernel on the share
# machine.  Green when both say everything they always say, Hanoi's 15 moves
# in order, nothing else, and the machine halts by itself once both have exited.
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build share > /dev/null && sh jig/aios build hello > /dev/null && sh jig/aios build hanoi > /dev/null || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
sh jig/aios pack "$t/two.bin" hello out/hello/hello.bin hanoi out/hanoi/hanoi.bin
timeout 20 sh pool/MACHINE.serial-irq@1/machine.sh out/share/loader.elf "$t/two.bin" < /dev/null > "$t/out" 2> /dev/null
[ $? -eq 124 ] && { echo "still running 20 s after both should have exited"; exit 1; }
grep -v '^Move disk' "$t/out" > "$t/hello"
grep '^Move disk' "$t/out" > "$t/hanoi"
cmp -s "$t/hello" tests/expected/hello.exact-bytes@1.out || { echo "hello's part differs: $(head -c 80 "$t/hello" | tr '\n' '|')"; exit 1; }
cmp -s "$t/hanoi" tests/expected/hanoi.exact-moves@1.out || { echo "Hanoi's moves differ: $(cmp "$t/hanoi" tests/expected/hanoi.exact-moves@1.out 2>&1)"; exit 1; }
echo "both programs whole, $(wc -l < "$t/out") lines, and the machine halted"
