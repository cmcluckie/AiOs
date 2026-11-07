#!/bin/sh
# Probe for TST.timer.says@1: what TST.timer.rate@1's check says about three
# captures made here, in its own format (the host's milliseconds, then the door):
# one empty, as from a machine that died before the program's first line; one that
# stops after 2 of 10 seconds; one of 10 seconds of zeros.  Each must be red, and
# only the last may say the door said 0.  Exit 0 green, 1 red.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
c=tests/expected/timer.rate@1.check
: > "$t/empty"
for i in 0 1 2; do echo "$((1000000 + i * 1000)) 0"; done > "$t/short"
for i in 0 1 2 3 4 5 6 7 8 9 10; do echo "$((1000000 + i * 1000)) 0"; done > "$t/zeros"
for f in empty short zeros; do
	sh "$c" "$t/$f" 10 > "$t/$f.out" 2>&1 && { echo "a capture that is $f passed"; exit 1; }
done
grep -q 'printed nothing' "$t/empty.out" || { echo "an empty capture is not said to be one: $(cat "$t/empty.out")"; exit 1; }
grep -q 'stopped printing after 2 of its 10 seconds' "$t/short.out" || { echo "a capture that stops early is not said to: $(cat "$t/short.out")"; exit 1; }
grep -q 'said 0 throughout' "$t/zeros.out" || { echo "ten seconds of zeros is not said to be: $(cat "$t/zeros.out")"; exit 1; }
echo "nothing printed, stopped early and zeros throughout each said as what they are"
