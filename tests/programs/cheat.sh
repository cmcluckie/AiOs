#!/bin/sh
# Probe for TST.referee.no-hardware@1.  In a throwaway copy, hello is replaced by
# tests/programs/cheat.c, which writes its 14 bytes to the serial port itself.
# First the cheat is built by hand, around the referee, and booted: its output
# must be exactly hello's, so hello's own test would pass it.  Then the copy is
# built the normal way, and the referee must refuse it under rule 12.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
tar --exclude=./out --exclude=./.git -cf - . | tar -xf - -C "$t"
cp tests/programs/cheat.c "$t/pool/PART.hello@1/hello.c"
cd "$t"
sh jig/aios build hello > "$t/build" 2>&1 && { echo "the referee built the cheat"; exit 1; }
grep -q 'rule 12: PART.hello@1 touches the hardware itself' "$t/build" || { echo "refused, but not by rule 12: $(cat "$t/build")"; exit 1; }
# around the referee: the cheat built by hand, against the same loader
ld=pool/PART.loader@1
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
mkdir -p out/cheat
gcc $CF -c -o out/cheat/c.o pool/PART.hello@1/hello.c && gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T $ld/program.ld -o out/cheat/hello.bin out/cheat/c.o || { echo "the cheat does not build"; exit 1; }
cd "$OLDPWD" && sh jig/aios build hello > /dev/null || { echo "the real hello does not build"; exit 1; }
timeout 20 sh pool/MACHINE.serial-only@1/machine.sh out/hello/loader.elf "$t/out/cheat/hello.bin" < /dev/null > "$t/cap" 2> /dev/null
cmp -s "$t/cap" tests/expected/hello.exact-bytes@1.out || { echo "the cheat's output differs from hello's, so the demonstration proves nothing: $(cmp "$t/cap" tests/expected/hello.exact-bytes@1.out 2>&1)"; exit 1; }
echo "the cheat printed hello's 14 bytes exactly, so hello's test would pass it; the referee refused it under rule 12"
