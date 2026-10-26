#!/bin/sh
# Probe for TST.gate.debt-listed@1: every number after TECH-DEBT in pool/ is the
# number of an entry in findings/0021.md ("   TECH-DEBT n.").  Green when it is;
# names the first mark with no entry.  An entry with no mark left is paid, and
# is said, not refused (decision 0076).
set -u
cd "$(dirname "$0")/../.."
marked=$(grep -rhoE 'TECH-DEBT [0-9]+' pool 2> /dev/null | sed 's/TECH-DEBT //' | sort -un)
listed=$(grep -oE '^ +TECH-DEBT [0-9]+\.' findings/0021.md | grep -oE '[0-9]+' | sort -un)
[ -n "$listed" ] || { echo "findings/0021.md lists no TECH-DEBT entry"; exit 1; }
for n in $marked; do
	printf '%s\n' "$listed" | grep -qx "$n" || { echo "TECH-DEBT $n is marked in the source and not listed in findings/0021.md"; exit 1; }
done
# a listed debt with no mark left is paid, waiting to be closed in the findings:
# its own red test, not this one, says whether it really is (decision 0076)
paid=""
for n in $listed; do
	printf '%s\n' "$marked" | grep -qx "$n" || paid="$paid $n"
done
echo "every TECH-DEBT mark is listed in findings/0021.md: $(echo $marked)${paid:+; paid, to be closed there:$paid}"
