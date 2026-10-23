#!/bin/sh
# Probe for TST.gate.window-end@1: tests/programs/pastend.c, a plain program,
# under the cat flavor's kernel (PART.loader@10, PART.gate@2), on the cat
# flavor's machine given 62 MiB instead of its 64, so that the program's window,
# from 4 MiB to the end of RAM, does not end on a 4 MiB line; nothing else about
# the machine changes.  Green when the program reads the last byte it was given,
# then is stopped with a page fault at 03E00000, the first byte past it, and never
# says it got away.  Exit 2 without QEMU or mkfs.minix.
set -u
cd "$(dirname "$0")/../.."
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
L=pool/PART.loader@10
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
sh jig/aios build cat > "$t/build" 2>&1 || { echo "the cat flavor does not build: $(tail -1 "$t/build")"; exit 1; }
gcc $CF -I $L -c -o "$t/p.o" tests/programs/pastend.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T $L/program.ld -o "$t/p.bin" "$t/p.o" || { echo "the probe does not build"; exit 1; }
sed 's/-m 64M/-m 62M/' pool/MACHINE.serial-disk@2/machine.sh > "$t/machine.sh"
grep -q -- '-m 62M' "$t/machine.sh" || { echo "the machine's memory could not be set to 62 MiB"; exit 1; }
truncate -s 1M "$t/disk"; mkfs.minix -1 -n 14 "$t/disk" > /dev/null 2>&1 || { echo "no mkfs.minix here"; exit 2; }
out=$(AIOS_DISK="$t/disk" timeout 30 sh "$t/machine.sh" out/cat/loader.elf "$t/p.bin" < /dev/null 2> /dev/null | tr '\r' '\n')
printf '%s\n' "$out" | grep -qx inside || { echo "the program could not read the last byte it was given: $(printf '%s' "$out" | head -c 120 | tr '\n' '|')"; exit 1; }
printf '%s\n' "$out" | grep -q 'got away' && { echo "the program read past the end of its memory and carried on"; exit 1; }
printf '%s\n' "$out" | grep -q '#0000000E\.03E00000' || { echo "not stopped with a page fault at 03E00000: $(printf '%s\n' "$out" | grep Guru)"; exit 1; }
echo "read the last byte it was given, stopped at 03E00000 (page fault), the first byte past it"
