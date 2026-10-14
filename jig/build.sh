#!/bin/sh
# Placeholder for the builder (plan 5.1).  Builds the loader once and one program
# from the pool with the jig's compiler (TOOL.c@1, scaffold).
# usage: sh jig/build.sh <program> [run]      e.g. sh jig/build.sh hanoi run
# Output in out/<program>/.  The flags are the as-built contract: 32-bit,
# freestanding, no libc, no PIC, no stack protector, no SSE, no unwind tables.
set -eu
prog=${1:?program name, e.g. hello or hanoi}
repo=$(cd "$(dirname "$0")/.." && pwd)
pool=$repo/pool
out=$repo/out/$prog
mkdir -p "$out"
CC=${CC:-gcc}
CFLAGS="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin \
 -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only \
 -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
LDFLAGS="-static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack"

$CC $CFLAGS -c -o "$out/serial8250.o" "$pool/PART.serial.8250@1/serial8250.c"
$CC $CFLAGS -c -o "$out/start.o"      "$pool/PART.loader@1/start.S"
$CC $CFLAGS -c -o "$out/loader.o"     "$pool/PART.loader@1/loader.c"
$CC $CFLAGS $LDFLAGS -T "$pool/PART.loader@1/loader.ld" \
    -o "$out/loader.elf" "$out/start.o" "$out/loader.o" "$out/serial8250.o"

$CC $CFLAGS -c -o "$out/$prog.o" "$pool/PART.$prog@1/$prog.c"
$CC $CFLAGS $LDFLAGS -T "$pool/PART.loader@1/program.ld" \
    -o "$out/$prog.bin" "$out/$prog.o"

printf 'loader.elf %6d bytes  %s\n' "$(wc -c < "$out/loader.elf")" "$(sha256sum < "$out/loader.elf" | cut -c1-16)"
printf '%-10s %6d bytes\n' "$prog.bin" "$(wc -c < "$out/$prog.bin")"

if [ "${2:-}" = run ]; then
	exec sh "$pool/MACHINE.serial-only@1/machine.sh" "$out/loader.elf" "$out/$prog.bin"
fi
