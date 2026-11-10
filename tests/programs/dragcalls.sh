#!/bin/sh
# Probe for TST.gate.clock@1, TST.gate.munmap@1 and TST.gate.avx@1:
# tests/programs/dragcalls.c, built as a static Linux program file with no C
# library, one mode a test, booted under the drag flavor's kernel (flavors/
# drag.toml: Dave Plummer's sieve, petition 0015, findings 0039).  One mode per
# test:
#   clock   ten seconds of clock_gettime64(CLOCK_PROCESS_CPUTIME_ID): the
#           milliseconds must keep pace with the host's clock to within 1%, as
#           tests/expected/loader.ms@1.check holds the ms door to it
#   munmap  20,000 rounds of 64 KiB taken and given back, on a 64 MiB machine
#   keep    a block given back while a newer one is in use: the one in use is
#           never handed out again, and keeps what was written to it
#   avx     one SSE and one AVX instruction, each with the right answer
# Under KVM the machine's processor is the host's (-cpu host), which has AVX.
# Exit 0 green, 1 red, 2 no QEMU.
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
sh jig/aios build drag > "$t/build" 2>&1 || fail "the drag flavor does not build: $(tail -1 "$t/build")"
case "$mode" in
clock) def=CLOCK; secs=10; x= ;; munmap) def=MUNMAP; secs=60; x= ;; keep) def=KEEP; secs=20; x= ;; avx) def=AVX; secs=20; x="-msse2 -mavx" ;;
*) fail "no mode '$mode': clock, munmap, keep or avx" ;;
esac
gcc -m32 -march=i486 -O2 -static -nostdlib -no-pie -fno-pie -fno-stack-protector -ffreestanding -Wl,--build-id=none -D$def $x -o "$t/probe" tests/programs/dragcalls.c || fail "the probe does not build"
machine=$(sed -n 's/^machine *= *"MACHINE\.\(.*\)".*/\1/p' flavors/drag.toml)
[ -n "$machine" ] || fail "flavors/drag.toml names no machine"
# every line stamped with the host's milliseconds as it arrives, as jig/test's timed runner does
timeout "$secs" sh "pool/MACHINE.$machine/machine.sh" out/drag/loader.elf "$t/probe" < /dev/null 2> /dev/null |
	stdbuf -o0 tr '\r' '\n' | while IFS= read -r l || [ -n "$l" ]; do echo "$(date +%s%3N) $l"; done > "$t/got"
grep -q "Guru Meditation" "$t/got" && fail "the program was stopped: $(grep -m1 "Guru Meditation" "$t/got" | cut -d' ' -f2-)"
case "$mode" in
clock)
	grep -q ' clock_gettime64 ' "$t/got" && fail "the call was refused: $(grep -m1 ' clock_gettime64 ' "$t/got" | cut -d' ' -f2-)"
	[ -s "$t/got" ] || fail "the machine printed nothing in its $secs seconds"
	sh tests/expected/loader.ms@1.check "$t/got" "$secs" || exit 1 ;;
munmap)
	grep -q ' munmap rounds 20000$' "$t/got" || fail "$(cut -d' ' -f2- "$t/got" | tr '\n' ' ' | cut -c1-200)" ;;
keep)
	grep -q ' keep ok$' "$t/got" || fail "$(cut -d' ' -f2- "$t/got" | tr '\n' ' ' | cut -c1-200)" ;;
avx)
	grep -q ' sse ok$' "$t/got" && grep -q ' avx ok$' "$t/got" || fail "$(cut -d' ' -f2- "$t/got" | tr '\n' ' ' | cut -c1-200)"
	echo "sse ok, avx ok" ;;
esac
