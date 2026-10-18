#!/bin/sh
# Probe for TST.gate.debt-listed@1: the numbers after TECH-DEBT in pool/ and the
# numbers of the entries in findings/0021.md ("   TECH-DEBT n.") are the same
# set.  Green when they are; names the first one that has no partner.
set -u
cd "$(dirname "$0")/../.."
marked=$(grep -rhoE 'TECH-DEBT [0-9]+' pool 2> /dev/null | sed 's/TECH-DEBT //' | sort -un)
listed=$(grep -oE '^ +TECH-DEBT [0-9]+\.' findings/0021.md | grep -oE '[0-9]+' | sort -un)
[ -n "$listed" ] || { echo "findings/0021.md lists no TECH-DEBT entry"; exit 1; }
for n in $marked; do
	printf '%s\n' "$listed" | grep -qx "$n" || { echo "TECH-DEBT $n is marked in the source and not listed in findings/0021.md"; exit 1; }
done
for n in $listed; do
	printf '%s\n' "$marked" | grep -qx "$n" || { echo "TECH-DEBT $n is listed in findings/0021.md and marked nowhere in pool/"; exit 1; }
done
echo "TECH-DEBT marks and findings/0021.md agree: $(echo $marked)"
