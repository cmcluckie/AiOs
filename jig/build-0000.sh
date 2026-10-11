#!/bin/sh
# Placeholder for the builder (plan 5.1).  Builds petition 0000's three segments with
# the jig's compiler (CMP.toolchain.c@1, scaffold) and runs the machine.
# usage: sh jig/build-0000.sh [run]        output in out/0000/
# The flags are the as-built contract: 32-bit, freestanding, no libc, no PIC, no
# stack protector, no SSE, no unwind tables.  Anything the compiler adds beyond the
# source is scaffolding the as-built spec must list.
set -eu
repo=$(cd "$(dirname "$0")/.." && pwd)
pool=$repo/pool
out=$repo/out/0000
mkdir -p "$out"
CC=${CC:-gcc}
CFLAGS="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin \
 -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only \
 -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"

$CC $CFLAGS -c -o "$out/serial8250.o" "$pool/CMP.serial.8250@1/serial8250.c"
$CC $CFLAGS -c -o "$out/start.o"      "$pool/CMP.loader@1/start.S"
$CC $CFLAGS -c -o "$out/loader.o"     "$pool/CMP.loader@1/loader.c"
$CC $CFLAGS -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T "$pool/CMP.loader@1/loader.ld" \
    -o "$out/loader.elf" "$out/start.o" "$out/loader.o" "$out/serial8250.o"

$CC $CFLAGS -c -o "$out/hello.o" "$pool/CMP.hello@1/hello.c"
$CC $CFLAGS -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T "$pool/CMP.hello@1/hello.ld" \
    -o "$out/hello.bin" "$out/hello.o"

printf 'loader.elf %6d bytes\nhello.bin  %6d bytes\n' "$(wc -c < "$out/loader.elf")" "$(wc -c < "$out/hello.bin")"

if [ "${1:-}" = run ]; then
	exec sh "$pool/CMP.machine.serial-only@1/machine.sh" "$out/loader.elf" "$out/hello.bin"
fi
