#!/bin/sh
# Probe for TST.life.screen@1 and TST.vga.plain@1: boot the screen flavor (or a
# probe program built against its kernel), wait until the program has stopped
# writing and is waiting for a key, take QEMU's screendump of the VGA screen,
# then press a key and let it exit.  The picture is checked cell by cell, 80 by
# 25 cells of 9 by 16 pixels: each cell's background colour, whether anything
# is drawn in it, and the colour it is drawn in, against a cells file, one line
# per row, "A" + the VGA attribute in hex + the character ("_" for a space).
# Pixels, not bytes: the font is the card's, so the check never asks which
# glyph, only where the ink is and what colour.  Lines 13 to 15 of each cell
# are skipped: that is where the card's blinking cursor is drawn.
# usage: screen.sh CELLS [PROBE.c]
set -u
cd "$(dirname "$0")/../.."
cells=$1; probe=${2:-}
t=$(mktemp -d); trap 'kill $qp 2>/dev/null; rm -rf "$t"' EXIT; qp=""
sh jig/aios build screen > "$t/b" 2>&1 || { echo "build failed: $(head -1 "$t/b")"; exit 1; }
bin=out/screen/$(cat out/screen/program).bin
if [ -n "$probe" ]; then
	CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
	gcc $CF -I pool/PART.loader@8 -c -o "$t/p.o" "$probe" &&
	gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T pool/PART.loader@8/program.ld -o "$t/p.bin" "$t/p.o" || { echo "probe does not build"; exit 1; }
	bin=$t/p.bin
fi
real=$(command -v qemu-system-x86_64) || { echo "no qemu-system-x86_64"; exit 2; }
mkdir "$t/bin"; mkfifo "$t/mon.in" "$t/mon.out" "$t/key"
printf '#!/bin/sh\nexec %s "$@" -chardev pipe,id=aiosmon,path=%s/mon -mon chardev=aiosmon,mode=readline\n' "$real" "$t" > "$t/bin/qemu-system-x86_64"
chmod +x "$t/bin/qemu-system-x86_64"
PATH="$t/bin:$PATH" timeout 60 sh pool/MACHINE.serial-vga@1/machine.sh out/screen/loader.elf "$bin" < "$t/key" > "$t/out" 2> /dev/null &
qp=$!
exec 7> "$t/key"			# the keyboard: held open, so the program waits
exec 8> "$t/mon.in"; exec 9< "$t/mon.out"
cat <&9 > "$t/mon.log" &
# written something, then quiet for two seconds: it is waiting for the key
last=-1; quiet=0; i=0
while [ $i -lt 100 ]; do
	sleep 0.5; i=$((i + 1)); n=$(wc -c < "$t/out")
	if [ "$n" -gt 0 ] && [ "$n" -eq "$last" ]; then quiet=$((quiet + 1)); [ $quiet -ge 4 ] && break; else quiet=0; fi
	last=$n
done
[ "$quiet" -ge 4 ] || { echo "the program never stopped writing ($n bytes)"; exit 1; }
printf 'screendump %s/s.ppm\n' "$t" >&8
i=0; while [ ! -s "$t/s.ppm" ] && [ $i -lt 20 ]; do sleep 0.5; i=$((i + 1)); done; sleep 1
printf 'q' >&7; exec 7>&-
i=0; while kill -0 $qp 2>/dev/null && [ $i -lt 20 ]; do sleep 0.5; i=$((i + 1)); done
kill -0 $qp 2>/dev/null && { echo "the program did not exit after the key"; exit 1; }
exec 8>&- 9<&-
[ -s "$t/s.ppm" ] || { echo "no screendump came back"; exit 1; }
[ -n "${AIOS_SHOT:-}" ] && cp "$t/s.ppm" "$AIOS_SHOT"	# keep the picture, for a person to look at
# the PPM: "P6", width height, 255, then RGB bytes
hdr=$(head -c 20 "$t/s.ppm" | tr '\n' ' ')
set -- $hdr
[ "$1" = P6 ] && [ "$2" = 720 ] && [ "$3" = 400 ] || { echo "not an 80 by 25 text screen: $2 x $3"; exit 1; }
skip=$(( ${#1} + ${#2} + ${#3} + ${#4} + 4 ))
tail -c +$((skip + 1)) "$t/s.ppm" | od -An -v -tu1 -w2160 | awk -v cells="$cells" '
BEGIN {
	# the 16 VGA colours, each channel in quarters of full (0, 1/3, 2/3, full:
	# the card'"'"'s 6-bit palette, 0, 21, 42 and 63, which QEMU scales to 0, 84,
	# 168 and 252 where the textbooks write 0, 85, 170 and 255)
	split("0 0 0|0 0 2|0 2 0|0 2 2|2 0 0|2 0 2|2 1 0|2 2 2|1 1 1|1 1 3|1 3 1|1 3 3|3 1 1|3 1 3|3 3 1|3 3 3", p, "|")
	for (i = 1; i <= 16; i++) { gsub(/ /, ",", p[i]); pal[p[i]] = i - 1 }
	r = 0; while ((getline line < cells) > 0) { n = split(line, f, " "); for (c = 1; c <= n; c++) want[r, c - 1] = f[c]; r++ }
	if (r != 25) { print "the cells file has " r " rows, not 25"; bad = 1; exit 1 }
}
{
	y = NR - 1; row = int(y / 16)
	# lines 13 to 15 of every cell are not looked at: the card'"'"'s blinking cursor
	# lives there, in whatever cell the firmware left it, because the kernel never
	# touches the card'"'"'s registers to move it (findings 0019 #4)
	if (y % 16 >= 13) next
	for (x = 0; x < 720; x++) {
		k = int(($(3 * x + 1) + 42) / 84) "," int(($(3 * x + 2) + 42) / 84) "," int(($(3 * x + 3) + 42) / 84)
		if (!(k in pal)) { print "pixel " x "," y " is " $(3 * x + 1) "," $(3 * x + 2) "," $(3 * x + 3) ", not a VGA colour"; bad = 1; exit 1 }
		col = int(x / 9); cnt[row, col, pal[k]]++
	}
}
END {
	if (bad) exit 1
	for (row = 0; row < 25; row++) for (col = 0; col < 80; col++) {
		w = want[row, col]; a = substr(w, 2, 2); ch = substr(w, 4)
		fg = index("0123456789abcdef", substr(a, 2, 1)) - 1; bg = index("0123456789abcdef", substr(a, 1, 1)) - 1
		best = -1; for (v = 0; v < 16; v++) if ((row, col, v) in cnt && (best < 0 || cnt[row, col, v] > cnt[row, col, best])) best = v
		ink = -1; for (v = 0; v < 16; v++) if (v != best && (row, col, v) in cnt) ink = v
		where = "row " row ", column " col " (" (ch == "_" ? "space" : "\"" ch "\"") ", attribute " a ")"
		if (best != bg) { print where ": background is colour " best ", not " bg; exit 1 }
		if (ch == "_" && ink >= 0) { print where ": something is drawn in colour " ink " where nothing should be"; exit 1 }
		if (ch != "_" && ink != fg) { print where ": drawn in colour " ink ", not " fg; exit 1 }
		if (ch != "_") drawn++
	}
	print "2000 cells, " drawn " drawn, every one where it should be, in its colours"
}' || exit 1
