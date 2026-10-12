#!/bin/sh
# Probe for TST.loader.door-layout@1: compile, for every version of the door,
# assertions that each entry sits at its fixed offset.  Exit 0 when every one holds.
set -eu
cd "$(dirname "$0")/../.."
n=0
for h in pool/CMP.loader@*/door.h; do
	v=${h#pool/CMP.loader@}; v=${v%/door.h}
	{
		echo "#include \"$PWD/$h\""
		echo "#define AT(f, o) _Static_assert(__builtin_offsetof(struct door, f) == o, \"$h: \" #f \" not at \" #o);"
		echo "AT(write, 0) AT(exit, 4)"
		[ "$v" -ge 2 ] && echo "AT(alloc, 8)"
		[ "$v" -ge 3 ] && echo "AT(read, 12)"
		[ "$v" -ge 4 ] && echo "AT(ms, 16)"
		[ "$v" -ge 5 ] && echo "AT(load, 20) AT(save, 24)"
		echo "_Static_assert(sizeof(void *) == 4, \"32-bit\");"
	} > /tmp/door_layout.$$.c
	gcc -m32 -ffreestanding -nostdinc -fsyntax-only /tmp/door_layout.$$.c
	n=$((n + 1))
done
rm -f /tmp/door_layout.$$.c
echo "door layout holds in $n versions"
