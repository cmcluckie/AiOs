#!/bin/sh
# Probe for TST.loader.doors-wait@1: hello, which never asks for a file, booted
# under loader@5 on the disk machine with QEMU's IDE command trace on.  Green
# when it prints exactly what it always has and the drive saw no command.
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build sort > /dev/null && sh jig/aios build hello > /dev/null || { echo "build failed"; exit 1; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
real=$(command -v qemu-system-x86_64) || { echo "no qemu-system-x86_64"; exit 2; }
mkdir "$t/bin"
printf '#!/bin/sh\nexec %s "$@" -trace ide_exec_cmd -trace ide_ioport_write -D %s/trace\n' "$real" "$t" > "$t/bin/qemu-system-x86_64"
chmod +x "$t/bin/qemu-system-x86_64"
AIOS_DISK="$t/disk" PATH="$t/bin:$PATH" timeout 30 sh pool/CMP.machine.serial-disk@1/machine.sh out/sort/loader.elf out/hello/hello.bin < /dev/null > "$t/out" 2> /dev/null
cmp -s "$t/out" tests/expected/hello.exact-bytes@1.out || { echo "hello under loader@5 changed: $(cmp "$t/out" tests/expected/hello.exact-bytes@1.out 2>&1)"; exit 1; }
[ -f "$t/trace" ] || { echo "no trace came back"; exit 1; }
n=$(grep -c 'ide_' "$t/trace")
[ "$n" -eq 0 ] || { echo "the drive was touched $n times by a program that never asked"; exit 1; }
echo "hello unchanged, the drive untouched"
