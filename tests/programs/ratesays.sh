#!/bin/sh
# Probe for TST.timer.says@1: what TST.timer.rate@1's check says about two
# captures made here, in its own format (the host's milliseconds, then the door):
# one empty, as from a machine that died before the program's first line; and the
# one line "0" that tests/programs/msprobe.c prints when the door is stuck at 0, for
# it prints only when the door has moved on.  Each must be red; the first must say
# the machine printed nothing, the second that the door said 0.  Exit 0 green, 1 red.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
c=tests/expected/timer.rate@1.check
: > "$t/empty"
echo "1000000 0" > "$t/zeros"
for f in empty zeros; do
	sh "$c" "$t/$f" 10 > "$t/$f.out" 2>&1 && { echo "a capture that is $f passed"; exit 1; }
done
grep -q 'printed nothing' "$t/empty.out" || { echo "an empty capture is not said to be one: $(cat "$t/empty.out")"; exit 1; }
grep -q 'said 0 throughout' "$t/zeros.out" || { echo "a door stuck at 0 is not said to be: $(cat "$t/zeros.out")"; exit 1; }
echo "nothing printed and a door stuck at 0 each said as what they are"
