#!/bin/sh
# Run tests/programs/pitprobe.c on the stopwatch's machine.  From the repo root:
#   sh tests/programs/pitprobe.sh
# Findings 0012.  Not a test: it prints, it does not judge.
set -eu
cd "$(dirname "$0")/../.."
sh jig/aios build stopwatch > /dev/null
ldir=pool/PART.loader@4/
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I "$ldir" -c -o "$tmp/p.o" tests/programs/pitprobe.c
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T "$ldir/program.ld" -o "$tmp/p.bin" "$tmp/p.o"
[ -w /dev/kvm ] && echo "accel: kvm" || echo "accel: software emulation"
timeout 10 sh pool/MACHINE.serial-timer@1/machine.sh out/stopwatch/loader.elf "$tmp/p.bin" < /dev/null
