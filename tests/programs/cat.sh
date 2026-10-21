#!/bin/sh
# Probe for TST.cat.*@1: Berkeley's cat on AiOs, on the cat flavor's machine.  A
# fresh empty Minix disk is made the way the machine makes one, the files are put
# on it by tests/programs/catfiles.c (run first, under the same kernel, through
# the old table's save), then cat is booted with its arguments on the kernel
# command line.  One mode per test:
#   stdin    cat, typed two lines and Ctrl+D: the two lines, then "exit 0"
#   one      cat /one: the file, then "exit 0"
#   multi    cat /one /two: one, then two, then "exit 0"
#   big      cat /big: all 5,000 bytes in order, then "exit 0"
#   missing  cat /none /one: /none reported, one printed, then "exit 1"
# Exit 0 green, 1 red, 2 this host cannot run it (no QEMU, no mkfs.minix).
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
command -v qemu-system-x86_64 > /dev/null && command -v mkfs.minix > /dev/null || { echo "no QEMU or mkfs.minix here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
sh jig/aios build cat > "$t/build" 2>&1 || fail "the cat flavor does not build: $(tail -1 "$t/build")"
machine=pool/MACHINE.serial-disk@2/machine.sh
L=pool/PART.loader@10
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I $L -c -o "$t/files.o" tests/programs/catfiles.c && gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T $L/program.ld -o "$t/files.bin" "$t/files.o" || fail "the file writer does not build"
truncate -s 1M "$t/disk"; mkfs.minix -1 -n 14 "$t/disk" > /dev/null
export AIOS_DISK="$t/disk"
timeout 60 sh $machine out/cat/loader.elf "$t/files.bin" < /dev/null > "$t/files.out" 2> /dev/null
grep -q "^files written" "$t/files.out" || fail "the files were not put on the disk: $(head -1 "$t/files.out")"
fsck.minix -f "$t/disk" > /dev/null 2>&1 || fail "the disk is not clean after the files were written"

printf 'the first file\nsecond line of it\n' > "$t/one"
printf 'the second file\n' > "$t/two"
awk 'BEGIN { for (i = 1; i <= 500; i++) printf "line %04d\n", i }' > "$t/big"
run() {	# ARGS INPUT: boot cat with ARGS on the command line, INPUT on the console
	AIOS_CMDLINE="$1" timeout 90 sh $machine out/cat/loader.elf out/cat/cat.bin < "$2" > "$t/got" 2> /dev/null
	[ $? -ne 124 ] || fail "cat $1: still running after 90 s"
}
case "$mode" in
stdin)
	printf 'hello\nworld\n\004' > "$t/in"; run cat "$t/in"
	printf 'hello\nworld\nexit 0\n' > "$t/want" ;;
one)
	run "cat /one" /dev/null; { cat "$t/one"; echo "exit 0"; } > "$t/want" ;;
multi)
	run "cat /one /two" /dev/null; { cat "$t/one" "$t/two"; echo "exit 0"; } > "$t/want" ;;
big)
	run "cat /big" /dev/null; { cat "$t/big"; echo "exit 0"; } > "$t/want" ;;
missing)
	run "cat /none /one" /dev/null; { echo "cat: /none: No such file or directory"; cat "$t/one"; echo "exit 1"; } > "$t/want" ;;
*)	fail "unknown mode '$mode'" ;;
esac
cmp -s "$t/got" "$t/want" || fail "cat ${mode}: got $(wc -c < "$t/got") bytes, not the $(wc -c < "$t/want") expected: $(cmp "$t/got" "$t/want" 2>&1 | head -1); first lines: $(head -c 120 "$t/got" | tr '\n' '|')"
echo "cat ($mode): $(wc -c < "$t/want" | tr -d ' ') bytes on the console, exact, ending $(tail -1 "$t/want")"
