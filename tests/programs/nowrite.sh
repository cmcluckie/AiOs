#!/bin/sh
# Probe for TST.fs.no-write@1: the cat flavor's petition (0014) asks for
# CAP.fs.read and not CAP.fs.write, so its kernel must have no way to write a
# file (decision 0065).  Three looks: nm finds no minix_save in the image; a
# program that asks to save is told -1; and the disk it ran on is byte for byte
# the empty disk it started as.  Exit 0 green, 1 red, 2 this host cannot run it.
set -u
cd "$(dirname "$0")/../.."
command -v qemu-system-x86_64 > /dev/null && command -v mkfs.minix > /dev/null || { echo "no QEMU or mkfs.minix here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
sh jig/aios build cat > "$t/build" 2>&1 || fail "the cat flavor does not build: $(tail -1 "$t/build")"
nm out/cat/loader.elf > "$t/nm" || fail "nm cannot read out/cat/loader.elf"
grep -q ' minix_save$' "$t/nm" && fail "minix_save is in the cat kernel, which no petition asked to write"
L=pool/PART.loader@10
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I $L -c -o "$t/p.o" tests/programs/nowrite.c && gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T $L/program.ld -o "$t/p.bin" "$t/p.o" || fail "the probe does not build"
truncate -s 1M "$t/disk"; mkfs.minix -1 -n 14 "$t/disk" > /dev/null; cp "$t/disk" "$t/before"
AIOS_DISK="$t/disk" timeout 50 sh pool/MACHINE.serial-disk@2/machine.sh out/cat/loader.elf "$t/p.bin" < /dev/null > "$t/out" 2> /dev/null
[ "$(cat "$t/out")" = "save refused" ] || fail "the probe said: $(tr '\n' '|' < "$t/out")"
cmp -s "$t/disk" "$t/before" || fail "the disk changed: $(cmp "$t/disk" "$t/before" 2>&1 | head -1)"
echo "no minix_save in the cat kernel; save answered -1; the disk is the empty disk it started as"
