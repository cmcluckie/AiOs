#!/bin/sh
# Probe for TST.gate.int80@1: tests/programs/gate-int80.c on a plain kernel.
# Green when its message arrives on the serial port and the machine halts by
# itself (exit 1 reached the kernel through int 0x80).
. "$(dirname "$0")/gate-lib.sh"
gate_boot tests/programs/gate-int80.c 15
[ $rc -eq 124 ] && { echo "still running after 15s"; exit 1; }
printf '%s\n' "$out" | grep -q '^int80 ok$' || { echo "no message through int 0x80: $(printf '%s' "$out" | tail -1)"; exit 1; }
echo "write and exit through int 0x80 alone"
