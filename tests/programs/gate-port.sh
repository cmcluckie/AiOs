#!/bin/sh
# Probe for TST.gate.port-refused@1: tests/programs/gate-port.c on a plain
# kernel.  Green when PORTCHEAT never reaches the serial port, the program is
# stopped with a Guru Meditation line for vector 0000000D (general protection),
# and it never says it got away.
. "$(dirname "$0")/gate-lib.sh"
gate_boot tests/programs/gate-port.c 15
[ $rc -eq 124 ] && { echo "still running after 15s"; exit 1; }
printf '%s\n' "$out" | grep -q 'PORTCHEAT' && { echo "the program wrote to the serial port itself"; exit 1; }
printf '%s\n' "$out" | grep -q 'got away' && { echo "the program carried on after its port write"; exit 1; }
g=$(printf '%s\n' "$out" | grep 'Guru Meditation')
[ -n "$g" ] || { echo "the program was never stopped"; exit 1; }
printf '%s\n' "$g" | grep -q '#0000000D\.' || { echo "wrong stop: $g"; exit 1; }
echo "stopped on its first port write (general protection)"
