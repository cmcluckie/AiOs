#!/bin/sh
# Probe for TST.loader.quiet-exit@1: the waiter and the stopwatch on the gate's
# sharing kernel, one byte typed after a second so the waiter's read returns.
# QEMU, in software emulation so its log can see the processor, writes the
# flags each time the processor starts on the gate's way out: from the
# instruction after the call to gate_trap through the iret.  The interrupt flag
# (0x200) must be off at every one of them, so no tick can land between a
# system call's answer and the program.  Red when the flag is on at any of them,
# or when the log never saw the way out at all.
set -u
cd "$(dirname "$0")/../.."
s=${AIOS_SHARE:-gateshare}
sh jig/aios build $s > /dev/null 2>&1 && sh jig/aios build stopwatch > /dev/null 2>&1 || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
gcc $CF -I pool/PART.loader@6 -c -o "$t/waiter.o" tests/programs/waiter.c &&
gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T pool/PART.loader@6/program.ld -o "$t/waiter.bin" "$t/waiter.o" || { echo "probe does not build"; exit 1; }
sh jig/aios pack "$t/both.bin" waiter "$t/waiter.bin" stopwatch out/stopwatch/stopwatch.bin
# the way out: the address after "call gate_trap" in trap_common, to its iret
way=$(objdump -d out/$s/loader.elf | awk '/<trap_common>:/,/iret/' | grep -E 'call.*<gate_trap>|iret' | awk '{print $1}' | tr -d ':' | tr '\n' ' ')
set -- $way
[ $# -eq 2 ] || { echo "the gate's way out not found in out/$s/loader.elf"; exit 1; }
from=$(printf '0x%x' $((0x$1 + 5))); to=$(printf '0x%x' $((0x$2 + 1)))
real=$(command -v qemu-system-x86_64) || { echo "no qemu-system-x86_64"; exit 2; }
mkdir "$t/bin"
# the same machine, but software emulation: KVM's processor cannot be logged
cat > "$t/bin/qemu-system-x86_64" << EOF
#!/bin/sh
n=\$#; i=0; skip=0
for a in "\$@"; do
	i=\$((i + 1))
	if [ \$skip -eq 1 ]; then skip=0; [ "\$a" = host ] && continue; set -- "\$@" -cpu; fi
	case "\$a" in -enable-kvm) set -- "\$@" -accel tcg ;; -cpu) skip=1 ;; *) set -- "\$@" "\$a" ;; esac
done
shift \$n
exec $real "\$@" -d cpu -dfilter $from..$to -D $t/log
EOF
chmod +x "$t/bin/qemu-system-x86_64"
# the log is read as it is written, never kept: hundreds of thousands of passes
mkfifo "$t/log"
# EFL is eight hex digits; 0x200, the interrupt flag, is the 2 in the sixth
awk '/^EIP=/ { n++; if (substr($2, 10, 1) ~ /[2367abefABEF]/) { on++; at[$1]++ } }
	END { printf "%d %d", n, on; for (a in at) printf " %s:%d", substr(a, 5), at[a]; print "" }' "$t/log" > "$t/count" &
{ sleep 2; printf x; sleep 3; } | PATH="$t/bin:$PATH" timeout 6 sh pool/MACHINE.serial-irq@1/machine.sh out/$s/loader.elf "$t/both.bin" 2> /dev/null | tr '\r' '\n' > "$t/out"
grep -q 'got a byte' "$t/out" || { echo "the waiter never got its byte, so its read never came back through the gate"; exit 1; }
wait
read -r seen on at < "$t/count" || { echo "the log was not read"; exit 1; }
[ "$seen" -gt 0 ] || { echo "the log never saw the gate's way out ($from..$to)"; exit 1; }
[ "$on" -eq 0 ] || { echo "interrupts on on the gate's way out, so a tick can land there: $on of $seen times through, at $at"; exit 1; }
echo "interrupts off every time through the gate's way out: $seen times, the waiter's read among them"
