#!/bin/sh
# Probe for TST.share.waits@1: tests/programs/waiter.c packed first and the
# stopwatch second, booted on the share machine and watched for 10 seconds by
# the host's clock, each line stamped as it arrives, with nothing ever typed.
# still showed every second once, in order, through 00:09 or 00:10, though the
# program before it in the archive is waiting for a byte that never comes
# (TECH-DEBT 2: a system call runs with interrupts off).
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build ${AIOS_SHARE:-share} > /dev/null 2>&1 && sh jig/aios build stopwatch > /dev/null 2>&1 || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I pool/PART.loader@6 -c -o "$t/waiter.o" tests/programs/waiter.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T pool/PART.loader@6/program.ld -o "$t/waiter.bin" "$t/waiter.o" || { echo "probe does not build"; exit 1; }
sh jig/aios pack "$t/both.bin" waiter "$t/waiter.bin" stopwatch out/stopwatch/stopwatch.bin
timeout 10 sh pool/MACHINE.serial-irq@1/machine.sh out/${AIOS_SHARE:-share}/loader.elf "$t/both.bin" < /dev/null 2> /dev/null |
	stdbuf -o0 tr '\r' '\n' |
	while IFS= read -r l || [ -n "$l" ]; do echo "$(date +%s%3N) $l"; done > "$t/out"
sh tests/expected/stopwatch.ticks@1.check "$t/out" 10 || exit 1
echo "next to a program waiting for input: $(sh tests/expected/stopwatch.ticks@1.check "$t/out" 10)"
