# Shared by the TST.gate.* probe scripts (sourced, not run).  Builds the gate
# flavor's kernel, builds a probe program against the newest loader's system
# call table and program link script, boots it alone on the serial-only machine,
# and leaves what came out on the serial port in $out and the machine's exit
# status in $rc.  Exit 2 (cannot run here) if QEMU is not on this host.
set -u
cd "$(dirname "$0")/../.."
command -v qemu-system-x86_64 > /dev/null || { echo "SKIP: qemu is not on this host"; exit 2; }
ver=$(ls -d pool/PART.loader@* | sed 's/.*@//' | sort -n | tail -1)
L=pool/PART.loader@$ver
CF="-m32 -std=c11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-red-zone -mgeneral-regs-only -nostdlib -nostdinc -O2 -Wall -Wextra -Werror"
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT

# gate_boot PROBE.c SECONDS
gate_boot() {
	sh jig/aios build gate > /dev/null || { echo "build of the gate flavor failed"; exit 1; }
	gcc $CF -I "$L" -c -o "$t/p.o" "$1" &&
	gcc $CF -static -no-pie -nostartfiles -Wl,--build-id=none -Wl,-z,noexecstack -T "$L/program.ld" -o "$t/p.bin" "$t/p.o" || { echo "probe $1 does not build"; exit 1; }
	out=$(timeout "$2" sh pool/MACHINE.serial-only@1/machine.sh out/gate/loader.elf "$t/p.bin" < /dev/null 2> /dev/null | tr '\r' '\n')
	rc=$?
}
