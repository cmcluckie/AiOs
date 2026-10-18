#!/bin/sh
# Probe for TST.gate.kernel-refused@1: tests/programs/gate-kernel.c on a plain
# kernel.  Green when the program is stopped with a Guru Meditation line for a
# page fault (vector 0000000E) at 00100000 and never says it got away.
. "$(dirname "$0")/gate-lib.sh"
gate_boot tests/programs/gate-kernel.c 15
[ $rc -eq 124 ] && { echo "still running after 15s"; exit 1; }
printf '%s\n' "$out" | grep -q 'got away' && { echo "the program reached the kernel's memory and carried on"; exit 1; }
g=$(printf '%s\n' "$out" | grep 'Guru Meditation')
[ -n "$g" ] || { echo "the program was never stopped"; exit 1; }
printf '%s\n' "$g" | grep -q '#0000000E\.00100000' || { echo "wrong stop: $g"; exit 1; }
echo "stopped at 00100000 (page fault)"
