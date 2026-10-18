#!/bin/sh
# Probe for TST.share.separate@1: the stopwatch and tests/programs/vandal.c,
# packed together and booted on the share machine for 6 seconds.  Green when
# the vandal is stopped with a Guru Meditation line for a page fault (vector
# 0000000E) at 00C00000, never says it got away with it, and the stopwatch
# goes on showing every second.
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build ${AIOS_SHARE:-share} > /dev/null && sh jig/aios build stopwatch > /dev/null || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I pool/PART.loader@6 -c -o "$t/vandal.o" tests/programs/vandal.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T pool/PART.loader@6/program.ld -o "$t/vandal.bin" "$t/vandal.o" || { echo "probe does not build"; exit 1; }
sh jig/aios pack "$t/both.bin" stopwatch out/stopwatch/stopwatch.bin vandal "$t/vandal.bin"
timeout 6 sh pool/MACHINE.serial-irq@1/machine.sh out/${AIOS_SHARE:-share}/loader.elf "$t/both.bin" < /dev/null 2> /dev/null | tr '\r' '\n' > "$t/out"
grep -q 'got away with it' "$t/out" && { echo "the vandal wrote past its own memory and carried on"; exit 1; }
guru=$(grep 'Guru Meditation' "$t/out")
[ -n "$guru" ] || { echo "the vandal was never stopped"; exit 1; }
echo "$guru" | grep -q '#0000000E\.00C00000  program 2 stopped' || { echo "wrong stop: $guru"; exit 1; }
last=$(grep -E '^[0-9][0-9]:[0-9][0-9]$' "$t/out" | tail -1)
case "$last" in 00:04|00:05) ;; *) echo "after the vandal, the stopwatch reached only ${last:-nothing} in 6 s"; exit 1 ;; esac
echo "vandal stopped at 00C00000 (page fault); the stopwatch reached $last"
