#!/bin/sh
# Probe for the tests that make part 21's fixes visible (findings 0024 item 4):
# tests/programs/debt.c, a static Linux program file with no C library, booted
# under the cat flavor's kernel (PART.loader@10, PART.gate@2), as linuxcalls.sh
# does.  One mode per test:
#   numbers  TECH-DEBT 1: no AiOs-only number is left in PART.gate@2, in its
#            table of calls or in the stubs old programs go through, and at 1000
#            to 1003 a Linux program is answered what Linux answers, -38 (ENOSYS)
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
	echo "no AiOs-only number left in the gate's table or its stubs, and 1000 to 1003 each answered -38, as Linux answers them" ;;
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
