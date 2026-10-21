#!/bin/sh
# Probe for TST.gate.linux-refuses@1: tests/programs/linuxcalls.c, built as a
# static Linux program file with no C library, booted under the cat flavor's
# kernel (PART.loader@10, PART.gate@2).  Every call must be refused with Linux's
# error number (EROFS 30, ENOENT 2, EBADF 9, EFAULT 14, EINVAL 22, ENOSYS 38),
# fresh memory must be zeroed and inside the window, and the heap must not move
# past the window.  Exit 0 green, 1 red, 2 no QEMU.
set -u
cd "$(dirname "$0")/../.."
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
sh jig/aios build cat > "$t/build" 2>&1 || { echo "the cat flavor does not build: $(tail -1 "$t/build")"; exit 1; }
gcc -m32 -O2 -static -nostdlib -no-pie -fno-pie -fno-stack-protector -ffreestanding -Wl,--build-id=none -o "$t/calls" tests/programs/linuxcalls.c || { echo "the probe does not build"; exit 1; }
truncate -s 1M "$t/disk"; mkfs.minix -1 -n 14 "$t/disk" > /dev/null 2>&1 || { echo "no mkfs.minix here"; exit 2; }
AIOS_DISK="$t/disk" AIOS_CMDLINE="calls" timeout 60 sh pool/MACHINE.serial-disk@2/machine.sh out/cat/loader.elf "$t/calls" < /dev/null > "$t/got" 2> /dev/null
cat > "$t/want" <<'END'
open-for-writing -30
open-below-root -2
open-missing -2
read-not-open -9
writev-not-open -9
write-kernel-buffer -14
mmap2-shared -22
mmap2-fresh-zero 0
set_thread_area-entry-3 -22
statx-by-name -38
brk-past-window 0
fork -38
exit 0
END
cmp -s "$t/got" "$t/want" || { echo "not refused as Linux would: $(diff "$t/want" "$t/got" | grep '^[<>]' | head -4 | tr '\n' ' ')"; exit 1; }
echo "12 calls refused with Linux's error numbers, fresh memory zeroed inside the window, the heap held inside it"
