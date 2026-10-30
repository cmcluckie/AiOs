#!/bin/sh
# Probe for TST.share.waits@2: tests/programs/waiter.c packed first and the
# stopwatch second, booted on the share machine and watched for 10 seconds by
# the host's clock, each line stamped as it arrives, and one byte typed 5 seconds
# in.  The stopwatch must still show every second once, in order, through 00:09
# or 00:10, and the waiter must say it got its byte: a fix that lets the others
# run by stopping the program that waits passes TST.share.waits@1, which never
# types anything, and fails this (findings 0029 item 6).
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build ${AIOS_SHARE:-share} > /dev/null 2>&1 && sh jig/aios build stopwatch > /dev/null 2>&1 || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I pool/PART.loader@6 -c -o "$t/waiter.o" tests/programs/waiter.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T pool/PART.loader@6/program.ld -o "$t/waiter.bin" "$t/waiter.o" || { echo "probe does not build"; exit 1; }
sh jig/aios pack "$t/both.bin" waiter "$t/waiter.bin" stopwatch out/stopwatch/stopwatch.bin
{ sleep 5; printf x; sleep 6; } | timeout 10 sh pool/MACHINE.serial-irq@1/machine.sh out/${AIOS_SHARE:-share}/loader.elf "$t/both.bin" 2> /dev/null |
	stdbuf -o0 tr '\r' '\n' |
	while IFS= read -r l || [ -n "$l" ]; do echo "$(date +%s%3N) $l"; done > "$t/out"
# the waiter's line may land in the middle of one of the stopwatch's, which ends
# with a carriage return, not a new line: the clock is read with it taken out
sed 's/got a byte$//' "$t/out" > "$t/clock"
clock=$(sh tests/expected/stopwatch.ticks@1.check "$t/clock" 10) || { echo "$clock"; exit 1; }
grep -q 'got a byte$' "$t/out" || { echo "the stopwatch kept time, but the waiter never got the byte typed at 5 s"; exit 1; }
echo "next to a program waiting for input: $clock, and the waiter got the byte typed at 5 s"
