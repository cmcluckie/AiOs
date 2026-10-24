#!/bin/sh
# Probe for the tests that make part 21's fixes visible (findings 0024 item 4):
# tests/programs/debt.c, a static Linux program file with no C library, booted
# under the cat flavor's kernel (PART.loader@10, PART.gate@2), as linuxcalls.sh
# does.  One mode per test:
#   numbers  TECH-DEBT 1: no AiOs-only number is left in PART.gate@2, in its
#            table of calls or in the stubs old programs go through, and at 1000
#            to 1003 a Linux program is answered what Linux answers, -38 (ENOSYS);
#            and an old program (tests/programs/oldnums.c) asking 85, 232, 265 and
#            45 is answered as Linux answers, not by load, save, ms or alloc, and
#            still loads, allocates, reads the clock and is refused a save
#            through its own table
#   random   TECH-DEBT 4: three boots give three AT_RANDOMs, each pair differing
#            in at least 12 of its 16 bytes; a counter read twice cannot do that
#   sse      findings 0023 item 13: a program built with SSE runs, its sums right
# Exit 0 green, 1 red, 2 no QEMU or no mkfs.minix.
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
sh jig/aios build cat > "$t/build" 2>&1 || { echo "the cat flavor does not build: $(tail -1 "$t/build")"; exit 1; }
cf="-m32 -O2 -static -nostdlib -no-pie -fno-pie -fno-stack-protector -ffreestanding -Wl,--build-id=none"
case "$mode" in sse) cf="$cf -msse2 -DSSE" ;; *) cf="$cf -march=i486" ;; esac
# shellcheck disable=SC2086
gcc $cf -o "$t/debt" tests/programs/debt.c || { echo "the probe does not build"; exit 1; }
truncate -s 1M "$t/disk"; mkfs.minix -1 -n 14 "$t/disk" > /dev/null 2>&1 || { echo "no mkfs.minix here"; exit 2; }
boot() { AIOS_DISK="$t/disk" AIOS_CMDLINE="debt $mode" timeout 60 sh pool/MACHINE.serial-disk@2/machine.sh out/cat/loader.elf "$t/debt" < /dev/null > "$1" 2> /dev/null; }
case "$mode" in
numbers)
	# no number of AiOs's own left in the gate: not in its table of calls, not in the
	# stubs old programs reach the kernel through (findings 0024 item 16)
	left=$(grep -nE '^#define[[:space:]]+NR_[A-Z_]+[[:space:]]+1[0-9][0-9][0-9]([^0-9]|$)' pool/PART.gate@2/gate.c; grep -nE 'movl[[:space:]]+\$1[0-9][0-9][0-9],[[:space:]]*%eax' pool/PART.gate@2/gate_user.S)
	[ -z "$left" ] || { echo "AiOs-only numbers are still in PART.gate@2: $(printf '%s\n' "$left" | head -4 | tr '\t' ' ' | tr -s ' ' | tr '\n' '|')"; exit 1; }
	boot "$t/got"
	printf 'nr-1000 -38\nnr-1001 -38\nnr-1002 -38\nnr-1003 -38\nexit 0\n' > "$t/want"
	cmp -s "$t/got" "$t/want" || { echo "an AiOs-only number is still answered: $(diff "$t/want" "$t/got" | grep '^>' | head -4 | tr '\n' ' ')"; exit 1; }
	# the old programs' side (findings 0024 item 19): a number means Linux's call
	# whoever asks, so an old program asking 85, 232, 265 or 45 with the old
	# calls' arguments gets Linux's answer or -38, never load, save, ms or alloc.
	# The disk holds a file named one, put there on the editor's kernel, which
	# may write (rule 22), so a load hidden at 85 would find it
	sh jig/aios build editor > "$t/build" 2>&1 || { echo "the editor flavor does not build: $(tail -1 "$t/build")"; exit 1; }
	pc="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
	pl="-static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack"
	gcc $pc -I pool/PART.loader@8 -c -o "$t/files.o" tests/programs/catfiles.c && gcc $pc $pl -T pool/PART.loader@8/program.ld -o "$t/files.bin" "$t/files.o" || { echo "the file writer does not build"; exit 1; }
	gcc $pc -I pool/PART.loader@10 -c -o "$t/old.o" tests/programs/oldnums.c && gcc $pc $pl -T pool/PART.loader@10/program.ld -o "$t/old.bin" "$t/old.o" || { echo "the old programs' probe does not build"; exit 1; }
	AIOS_DISK="$t/disk" timeout 60 sh pool/MACHINE.serial-disk@1/machine.sh out/editor/loader.elf "$t/files.bin" < /dev/null > "$t/files.out" 2> /dev/null
	grep -q "^files written" "$t/files.out" || { echo "the file one was not put on the disk: $(head -1 "$t/files.out")"; exit 1; }
	AIOS_DISK="$t/disk" timeout 60 sh pool/MACHINE.serial-disk@2/machine.sh out/cat/loader.elf "$t/old.bin" < /dev/null 2> /dev/null | tr -d '\r' > "$t/old"
	ans() { sed -n "s/^nr-$1 \(-*[0-9]*\).*/\1/p" "$t/old"; }
	bad=""
	case "$(ans 85)" in -38|-22|-2) grep -q '^nr-85 .* untouched$' "$t/old" || bad="$bad 85 wrote into the buffer;" ;; *) bad="$bad 85 answered '$(ans 85)', not readlink's answer;" ;; esac
	case "$(ans 232)" in -38|-2|-61|-95) ;; *) bad="$bad 232 answered '$(ans 232)', not listxattr's answer;" ;; esac
	case "$(ans 265)" in -38|-14) ;; *) bad="$bad 265 answered '$(ans 265)', not clock_gettime's answer;" ;; esac
	grep -q '^nr-45 0 same$' "$t/old" || bad="$bad 45 asked twice for 16 gave $(sed -n 's/^nr-45 0 //p' "$t/old") answers, not brk's;"
	grep -q '^sweep-load none$' "$t/old" || bad="$bad a load answered at $(sed -n 's/^sweep-load //p' "$t/old");"
	[ -z "$bad" ] || { echo "an old program is still answered by an AiOs call behind a Linux number:$bad"; exit 1; }
	# and through the table it was handed, an old program still works (try 8)
	grep -q '^old-load 33 exact$' "$t/old" || bad="$bad load gave $(sed -n 's/^old-load //p' "$t/old"), not the file's 33 bytes;"
	grep -q '^old-alloc 0 two blocks$' "$t/old" || bad="$bad alloc did not give two 16-aligned blocks that hold what is written;"
	grep -q '^old-ms 0$' "$t/old" || bad="$bad ms gave $(sed -n 's/^old-ms //p' "$t/old"), not the 0 a kernel with no clock gives;"
	grep -q '^old-save -1$' "$t/old" || bad="$bad save gave $(sed -n 's/^old-save //p' "$t/old"), not -1 on a kernel that may not write;"
	[ -z "$bad" ] || { echo "an old program no longer works through its table:$bad"; exit 1; }
	echo "no AiOs-only number left in the gate's table or its stubs; 1000 to 1003 each answered -38, as Linux answers them; an old program asking 85, 232, 265 and 45 gets Linux's answers, and no number from 0 to 999 loads a file; and through its table it still loads, allocates, reads the clock and is refused a save as before" ;;
random)
	for i in 1 2 3; do boot "$t/r$i"; grep -qx '[0-9a-f]\{32\}' "$t/r$i" || { echo "boot $i gave no AT_RANDOM: $(head -1 "$t/r$i")"; exit 1; }; done
	for pair in "1 2" "1 3" "2 3"; do
		set -- $pair
		d=$(paste -d' ' "$t/r$1" "$t/r$2" | head -1 | awk '{n = 0; for (i = 1; i <= 32; i += 2) if (substr($1, i, 2) != substr($2, i, 2)) n++; print n}')
		[ "$d" -ge 12 ] || { echo "boots $1 and $2 differ in only $d of 16 bytes: $(head -1 "$t/r$1") $(head -1 "$t/r$2")"; exit 1; }
	done
	echo "three boots, three AT_RANDOMs, each pair differing in at least 12 of 16 bytes" ;;
sse)
	boot "$t/got"
	printf 'sum-0 11\nsum-1 22\nsum-2 33\nsum-3 44\nexit 0\n' > "$t/want"
	cmp -s "$t/got" "$t/want" || { echo "a program built with SSE did not run: $(head -c 120 "$t/got" | tr '\n' '|')"; exit 1; }
	echo "a program built with SSE ran, its four sums right" ;;
*)	echo "tests/programs/debt.sh needs a mode: numbers, random or sse"; exit 1 ;;
esac
