#!/bin/sh
# Probe for TST.fs.overwrite-clean@1: build tests/programs/overwrite.c against
# the editor's loader (PART.loader@7, with PART.fs.minix@2), boot it once on a
# fresh disk, and check that every save worked, the last one loads back the
# same, and Linux's own fsck.minix finds nothing wrong: every zone the file
# gave back is free again, and notes.txt is the only file.
set -u
cd "$(dirname "$0")/../.."
command -v fsck.minix > /dev/null || { echo "no fsck.minix on this host"; exit 2; }
sh jig/aios build editor > /dev/null || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
ldir=pool/PART.loader@7
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I $ldir -c -o "$t/p.o" tests/programs/overwrite.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T $ldir/program.ld -o "$t/p.bin" "$t/p.o" || { echo "probe does not build"; exit 1; }
AIOS_DISK="$t/disk" timeout 50 sh pool/MACHINE.serial-disk@1/machine.sh out/editor/loader.elf "$t/p.bin" < /dev/null > "$t/out" 2> /dev/null
want=$(printf 'save 2500: ok\nsave 10: ok\nsave 7168: ok\nsave 0: ok\nsave 1500: ok\nload: 1500 bytes, the same')
[ "$(cat "$t/out")" = "$want" ] || { echo "the probe said: $(tr '\n' '|' < "$t/out")"; exit 1; }
fsck.minix -fl "$t/disk" > "$t/fsck" 2>&1 || { echo "fsck.minix found errors: $(grep -v -e '^Forcing' -e '^/' "$t/fsck" | head -2 | tr '\n' ' ')"; exit 1; }
[ "$(grep '^/' "$t/fsck")" = "/notes.txt" ] || { echo "fsck.minix lists: $(grep '^/' "$t/fsck" | tr '\n' ' ')"; exit 1; }
echo "five saves over one file, the last loads back the same, fsck.minix clean"
