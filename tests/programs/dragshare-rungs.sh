#!/bin/sh
# Probe for TST.gate.elf-shares@1 and TST.gate.elf-heap-shares@1: car 3's ladder
# (findings 0051).  tests/programs/dragcalls.c, built as a static Linux program
# file with no C library, packed after the stopwatch into one archive, booted on
# the dragshare flavor's kernel (the sharing kernel with PART.gate@3) on its
# machine, and watched by the host's clock.  One mode per test:
#   hello   the Linux program prints "elf hello" and exits 0
#   heap    the Linux program takes 64 KiB and 4 KiB and gives them back in that
#           order, 20,000 times, and prints "order rounds 20000"
# Either way the stopwatch must tick at least two seconds, no program may be
# stopped, and the Linux program's line must be seen.  Exit 0 green, 1 red,
# 2 no QEMU.
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
case "$mode" in
hello) def=HELLO; want="elf hello" ;; heap) def=ORDER; want="order rounds 20000" ;;
*) fail "no mode '$mode': hello or heap" ;;
esac
sh jig/aios build dragshare > "$t/build" 2>&1 || fail "the dragshare flavor does not build: $(tail -1 "$t/build")"
gcc -m32 -march=i486 -O2 -static -nostdlib -no-pie -fno-pie -fno-stack-protector -ffreestanding -Wl,--build-id=none -D$def -o "$t/probe" tests/programs/dragcalls.c || fail "the probe does not build"
sh jig/aios pack "$t/arc.bin" stopwatch out/dragshare/stopwatch.bin probe "$t/probe" || fail "the archive was not made"
machine=$(sed -n 's/^machine *= *"MACHINE\.\(.*\)".*/\1/p' flavors/dragshare.toml)
timeout 12 sh "pool/MACHINE.$machine/machine.sh" out/dragshare/loader.elf "$t/arc.bin" < /dev/null 2> /dev/null | tr '\r' '\n' > "$t/got"
grep -q "Guru Meditation" "$t/got" && fail "a program was stopped: $(grep -m1 "Guru Meditation" "$t/got")"
grep -q "^$want\$" "$t/got" || fail "the Linux program never printed \"$want\": $(tr '\n' ' ' < "$t/got" | cut -c1-200)"
[ "$(grep -c '^00:0[1-9]$' "$t/got")" -ge 2 ] || fail "the stopwatch did not tick two seconds beside it: $(tr '\n' ' ' < "$t/got" | cut -c1-200)"
echo "$want, beside the stopwatch"
