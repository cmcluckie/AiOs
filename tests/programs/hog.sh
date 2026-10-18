#!/bin/sh
# Probe for TST.share.on-time@2: tests/programs/hog.c packed first and the
# stopwatch second, booted on the share machine and watched for 10 seconds by
# the host's clock, each line stamped as it arrives.  Green when the stopwatch
# still showed every second once, in order, through 00:09 or 00:10, though the
# program before it in the archive never gives the processor up.
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build ${AIOS_SHARE:-share} > /dev/null && sh jig/aios build stopwatch > /dev/null || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I pool/PART.loader@6 -c -o "$t/hog.o" tests/programs/hog.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T pool/PART.loader@6/program.ld -o "$t/hog.bin" "$t/hog.o" || { echo "probe does not build"; exit 1; }
sh jig/aios pack "$t/both.bin" hog "$t/hog.bin" stopwatch out/stopwatch/stopwatch.bin
timeout 10 sh pool/MACHINE.serial-irq@1/machine.sh out/${AIOS_SHARE:-share}/loader.elf "$t/both.bin" < /dev/null 2> /dev/null |
	stdbuf -o0 tr '\r' '\n' |
	while IFS= read -r l || [ -n "$l" ]; do echo "$(date +%s%3N) $l"; done > "$t/out"
sh tests/expected/stopwatch.ticks@1.check "$t/out" 10 || exit 1
echo "next to a program that never yields: $(sh tests/expected/stopwatch.ticks@1.check "$t/out" 10)"
