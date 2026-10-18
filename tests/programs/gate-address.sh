#!/bin/sh
# Probe for TST.gate.refuses-address@1: tests/programs/gate-address.c on the
# gate kernel.  Green when both calls that were handed a kernel address come
# back -14.
. "$(dirname "$0")/gate-lib.sh"
gate_boot tests/programs/gate-address.c 15
[ $rc -eq 124 ] && { echo "still running after 15s"; exit 1; }
printf '%s\n' "$out" | grep -q '^address refused$' || { echo "kernel address not refused: $(printf '%s' "$out" | tail -1)"; exit 1; }
echo "write and read handed a kernel address both came back -14"
