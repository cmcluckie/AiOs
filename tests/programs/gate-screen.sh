#!/bin/sh
# Probe for TST.gate.screen-refused@1: tests/programs/gate-screen.c on a plain
# kernel.  Green when the program is stopped with a Guru Meditation line for a
# page fault (vector 0000000E) at 000B8000, never says it got away, and the
# machine still halts by itself.
. "$(dirname "$0")/gate-lib.sh"
gate_boot tests/programs/gate-screen.c 15
[ $rc -eq 124 ] && { echo "still running after 15s"; exit 1; }
printf '%s\n' "$out" | grep -q 'got away' && { echo "the program stored into the screen's memory and carried on"; exit 1; }
g=$(printf '%s\n' "$out" | grep 'Guru Meditation')
[ -n "$g" ] || { echo "the program was never stopped"; exit 1; }
printf '%s\n' "$g" | grep -q '#0000000E\.000B8000' || { echo "wrong stop: $g"; exit 1; }
echo "stopped at 000B8000 (page fault)"
