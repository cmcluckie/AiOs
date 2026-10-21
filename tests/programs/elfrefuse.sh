#!/bin/sh
# Probe for TST.loader.elf-refuses@1: Berkeley's cat as the cat flavor builds it,
# booted under PART.loader@10 as it is (it must run: Ctrl+D, "exit 0") and with
# one field of its file changed each time, which must be refused with the reason
# on the console and nothing run:
#   e_type 3       a shared or position-independent file, not one that runs as it is
#   e_machine 62   a 64-bit x86 file
#   p_type 3       its first program header asks for a dynamic linker (PT_INTERP)
# Exit 0 green, 1 red, 2 no QEMU.
set -u
cd "$(dirname "$0")/../.."
command -v qemu-system-x86_64 > /dev/null || { echo "no QEMU here"; exit 2; }
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
fail() { echo "$*"; exit 1; }
sh jig/aios build cat > "$t/build" 2>&1 || fail "the cat flavor does not build: $(tail -1 "$t/build")"
truncate -s 1M "$t/disk"; mkfs.minix -1 -n 14 "$t/disk" > /dev/null 2>&1 || { echo "no mkfs.minix here"; exit 2; }
boot() { AIOS_DISK="$t/disk" AIOS_CMDLINE="cat" timeout 60 sh pool/MACHINE.serial-disk@2/machine.sh out/cat/loader.elf "$1" < "$t/in" > "$t/got" 2> /dev/null; }
printf '\004' > "$t/in"
boot out/cat/cat.bin
[ "$(cat "$t/got")" = "exit 0" ] || fail "cat unchanged did not run: $(head -c 80 "$t/got")"
# patch FILE OFFSET BYTE: one byte of a copy changed
patch() { cp "$1" "$t/p"; printf "$(printf '\\%03o' "$3")" | dd of="$t/p" bs=1 seek="$2" conv=notrunc 2> /dev/null; }
phoff=$(od -An -tu4 -j28 -N4 out/cat/cat.bin | tr -d ' ')
for c in "16 3 not a 32-bit x86 program file" "18 62 not a 32-bit x86 program file" "$phoff 3 asks for a dynamic linker"; do
	set -- $c; off=$1; byte=$2; shift 2
	patch out/cat/cat.bin "$off" "$byte"; boot "$t/p"
	grep -q "^loader: .*$*" "$t/got" || fail "byte $off set to $byte was not refused for '$*': $(head -c 80 "$t/got" | tr '\n' '|')"
	grep -q "^exit" "$t/got" && fail "byte $off set to $byte: refused, but the program ran"
done
echo "cat ran unchanged; a shared file, a 64-bit file and one asking for a dynamic linker each refused with the reason, nothing run"
