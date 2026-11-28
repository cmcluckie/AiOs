#!/bin/sh
# Probe for TST.gate.elf-shares@1 and TST.gate.elf-heap-shares@1: car 3's ladder
# (findings 0051).  tests/programs/dragcalls.c, built as a static Linux program
# file with no C library, packed after the stopwatch into one archive, booted on
# the dragshare flavor's kernel (the sharing kernel with PART.gate@3) on its
# machine, and watched by the host's clock.  One mode per test:
#   hello   the Linux program prints "elf hello" and exits 0
#   heap    the Linux program takes 64 KiB and 4 KiB and gives them back in that
#           order, 20,000 times, and prints "order rounds 20000"
# Either way the stopwatch must tick at least two seconds, no program may be
# stopped, and the Linux program's line must be seen.  Exit 0 green, 1 red,
# 2 no QEMU.
#
# A red run says more than what was missing (findings 0055): the same archive is
# booted once more for two seconds under software emulation with QEMU's own
# record of every interrupt (-d int), and the last line printed says where the
# timer's ticks landed, which calls (int 0x80, by number) came from where, and
# the first faults, each by the top of its address (001xxxxx is AiOs's own
# programs' window), beside the address the Linux file asks to start at.
set -u
cd "$(dirname "$0")/../.."
mode=${1:-}
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
# seen: what QEMU saw the machine do, in one line, for a red run
seen() {
	real=$(command -v qemu-system-x86_64); mkdir -p "$t/bin"; mkfifo "$t/ints"
	{ echo '#!/bin/sh'; echo 'a=; skip=; for x in "$@"; do'
	  echo '	[ -n "$skip" ] && { skip=; continue; }'
	  echo '	case "$x" in -enable-kvm) continue ;; -cpu|-accel) skip=1; continue ;; esac'
	  echo '	a="$a $x"; done'
	  echo "exec $real \$a -accel tcg -d int -D $t/ints"; } > "$t/bin/qemu-system-x86_64"
	chmod +x "$t/bin/qemu-system-x86_64"
	awk '
	function hex(h,   n, i) { n = 0; h = tolower(h); for (i = 1; i <= length(h); i++) n = n * 16 + index("0123456789abcdef", substr(h, i, 1)) - 1; return n }
	/^ *[0-9]+: v=/ {
		v = substr($2, 3); pc = ""; cr2 = ""; eax = ""
		for (i = 3; i <= NF; i++) { if ($i ~ /^pc=/) pc = substr($i, length($i) - 7); if ($i ~ /^CR2=/) cr2 = substr($i, length($i) - 7); if ($i ~ /R_EAX\]=/) eax = substr($i, length($i) - 7) }
		at = substr(pc, 1, 3) "xxxxx"
		if (v == "80") { calls[at " " hex(eax)]++; where[at] = 1 }
		else if (v ~ /^[2-9a-f]/) { ticks[at]++; where[at] = 1 }
		else if (nf < 3) { faults = faults " fault " v " at " pc (cr2 != "" ? " touching " cr2 : "") ";"; nf++ }
	}
	END {
		for (w in where) {
			out = out " at " w ": " (ticks[w] + 0) " ticks"
			for (c in calls) { split(c, k, " "); if (k[1] == w) out = out ", call " k[2] " x" calls[c] }
			out = out ";"
		}
		print (out == "" && faults == "" ? " nothing: no tick, call or fault" : out faults)
	}' < "$t/ints" > "$t/seen" &
	PATH="$t/bin:$PATH" timeout 2 sh "pool/MACHINE.$machine/machine.sh" out/dragshare/loader.elf "$t/arc.bin" < /dev/null > /dev/null 2>&1
	wait
	entry=$(od -An -tx4 -j24 -N4 "$t/probe" | tr -d ' ')
	echo "seen under emulation in 2 s, by address:$(cat "$t/seen")  the Linux file asks to start at $entry"
}
red() { echo "$1"; seen; exit 1; }
case "$mode" in
hello) def=HELLO; want="elf hello" ;; heap) def=ORDER; want="order rounds 20000" ;;
*) fail "no mode '$mode': hello or heap" ;;
esac
sh jig/aios build dragshare > "$t/build" 2>&1 || fail "the dragshare flavor does not build: $(tail -1 "$t/build")"
gcc -m32 -march=i486 -O2 -static -nostdlib -no-pie -fno-pie -fno-stack-protector -ffreestanding -Wl,--build-id=none -D$def -o "$t/probe" tests/programs/dragcalls.c || fail "the probe does not build"
sh jig/aios pack "$t/arc.bin" stopwatch out/dragshare/stopwatch.bin probe "$t/probe" || fail "the archive was not made"
machine=$(sed -n 's/^machine *= *"MACHINE\.\(.*\)".*/\1/p' flavors/dragshare.toml)
timeout 12 sh "pool/MACHINE.$machine/machine.sh" out/dragshare/loader.elf "$t/arc.bin" < /dev/null 2> /dev/null | tr '\r' '\n' > "$t/got"
grep -q "Guru Meditation" "$t/got" && red "a program was stopped: $(grep -m1 "Guru Meditation" "$t/got")"
# the line may follow the stopwatch's on the shared console: it redraws its own in place and never ends it
grep -q "$want\$" "$t/got" || red "the Linux program never printed \"$want\": $(tr '\n' ' ' < "$t/got" | cut -c1-120)"
[ "$(grep -c '^00:0[1-9]$' "$t/got")" -ge 2 ] || red "the stopwatch did not tick two seconds beside it: $(tr '\n' ' ' < "$t/got" | cut -c1-120)"
echo "$want, beside the stopwatch"
