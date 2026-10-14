#!/bin/sh
# Probe for TST.tick.rate@1, TST.pic.setup@1 and TST.loader.plain-6@1: boot on
# the share machine with QEMU's 8259 trace on, then read what the controller
# saw.  The first argument says which test:
#   tick   the share flavor for 5 s: IRQ 0 must reach the processor 100 times a
#          second, give or take the boot
#   pic    the share flavor: the setup Linux's init_8259A does, then only IRQ 0
#          unmasked, and an EOI for every tick
#   plain  hello alone under loader@6: exactly hello's 14 bytes, and the 8259 is
#          never touched
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build share > /dev/null && sh jig/aios build hello > /dev/null || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
real=$(command -v qemu-system-x86_64) || { echo "no qemu-system-x86_64"; exit 2; }
mkdir "$t/bin"
printf '#!/bin/sh\nexec %s "$@" -trace pic_interrupt -trace pic_ioport_write -D %s/trace\n' "$real" "$t" > "$t/bin/qemu-system-x86_64"
chmod +x "$t/bin/qemu-system-x86_64"
m=pool/MACHINE.serial-irq@1/machine.sh
case "${1:-}" in
tick)
	PATH="$t/bin:$PATH" timeout 5 sh $m out/share/loader.elf out/share/programs.bin < /dev/null > /dev/null 2>&1
	n=$(grep -c 'pic_interrupt irq 0 ' "$t/trace")
	[ "$n" -ge 400 ] && [ "$n" -le 510 ] || { echo "$n ticks in 5 s, not about 500"; exit 1; }
	echo "$n ticks in 5 s: $((n / 5)) a second" ;;
pic)
	PATH="$t/bin:$PATH" timeout 3 sh $m out/share/loader.elf out/share/programs.bin < /dev/null > /dev/null 2>&1
	w=$(grep pic_ioport_write "$t/trace" | sed 's/.*master \([01]\) addr \(0x[01]\) val \(0x[0-9a-f]*\).*/\1:\2:\3/' | tr '\n' ' ')
	want="1:0x1:0xff 1:0x0:0x11 1:0x1:0x30 1:0x1:0x4 1:0x1:0x1 0:0x0:0x11 0:0x1:0x38 0:0x1:0x2 0:0x1:0x1 1:0x1:0xff 0:0x1:0xff 1:0x1:0xfe "
	case "$w" in "$want"*) ;; *) echo "setup was: $(echo "$w" | cut -c1-200)"; exit 1 ;; esac
	rest=$(echo "${w#"$want"}" | tr ' ' '\n' | grep . | sort -u | tr '\n' ' ')
	[ "$rest" = "1:0x0:0x60 " ] || { echo "after setup, more than EOIs for IRQ 0: $rest"; exit 1; }
	ticks=$(grep -c 'pic_interrupt irq 0 ' "$t/trace"); eois=$(echo "${w#"$want"}" | tr ' ' '\n' | grep -c .)
	[ $((ticks - eois)) -le 1 ] && [ $((ticks - eois)) -ge 0 ] || { echo "$ticks ticks but $eois EOIs"; exit 1; }
	echo "Linux's setup, only IRQ 0 unmasked, $eois EOIs for $ticks ticks" ;;
plain)
	PATH="$t/bin:$PATH" timeout 10 sh $m out/share/loader.elf out/hello/hello.bin < /dev/null > "$t/out" 2> /dev/null
	cmp -s "$t/out" tests/expected/hello.exact-bytes@1.out || { echo "hello under loader@6 changed: $(cmp "$t/out" tests/expected/hello.exact-bytes@1.out 2>&1)"; exit 1; }
	n=$(cat "$t/trace" 2> /dev/null | grep -c pic_)
	[ "$n" -eq 0 ] || { echo "one plain program, and the 8259 was touched $n times"; exit 1; }
	echo "hello unchanged, the 8259 untouched" ;;
*)	echo "usage: irqtrace.sh tick|pic|plain"; exit 1 ;;
esac
