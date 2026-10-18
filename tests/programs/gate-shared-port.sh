#!/bin/sh
# Probe for TST.gate.shared-port@1: the stopwatch and tests/programs/gate-port.c
# (a program that writes to the serial port itself), packed together and booted
# on the kernel behind the gate for 6 seconds.  Green when the port-writer is
# stopped with a Guru Meditation line for vector 0000000D (general protection)
# as program 2, PORTCHEAT never reaches the port, it never says it got away, and
# the stopwatch goes on showing every second.
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build gateshare > /dev/null && sh jig/aios build stopwatch > /dev/null || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I pool/PART.loader@6 -c -o "$t/p.o" tests/programs/gate-port.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T pool/PART.loader@6/program.ld -o "$t/p.bin" "$t/p.o" || { echo "probe does not build"; exit 1; }
sh jig/aios pack "$t/both.bin" stopwatch out/stopwatch/stopwatch.bin port "$t/p.bin"
timeout 6 sh pool/MACHINE.serial-irq@1/machine.sh out/gateshare/loader.elf "$t/both.bin" < /dev/null 2> /dev/null | tr '\r' '\n' > "$t/out"
grep -q 'PORTCHEAT' "$t/out" && { echo "the program wrote to the serial port itself"; exit 1; }
grep -q 'got away' "$t/out" && { echo "the program carried on after its port write"; exit 1; }
guru=$(grep 'Guru Meditation' "$t/out")
[ -n "$guru" ] || { echo "the program was never stopped"; exit 1; }
echo "$guru" | grep -q '#0000000D\..*  program 2 stopped' || { echo "wrong stop: $guru"; exit 1; }
last=$(grep -E '^[0-9][0-9]:[0-9][0-9]$' "$t/out" | tail -1)
case "$last" in 00:04|00:05) ;; *) echo "after the stop, the stopwatch reached only ${last:-nothing} in 6 s"; exit 1 ;; esac
echo "port write stopped (general protection) as program 2; the stopwatch reached $last"
