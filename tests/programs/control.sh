#!/bin/sh
# Probe for TST.exam.control@1: the race's control does not move.  jig/sit, the
# old bench's runner for exam parts, is frozen as it stood on 2026-10-29 (decision
# 0104), and jig/fix, the old bench's runner for fixes, as it stood at a8bfd5a
# (findings 0027 item 2d).  Each file's git fingerprint must be the one recorded
# here; a change to either is a change to the control, and is red.
set -u
cd "$(dirname "$0")/../.."
command -v git > /dev/null || { echo "no git here"; exit 2; }
bad=""
while read -r f want since; do
	have=$(git hash-object "$f" 2> /dev/null) || { bad="$bad $f is gone;"; continue; }
	[ "$have" = "$want" ] || bad="$bad $f is ${have%"${have#????????}"}, not ${want%"${want#????????}"} as frozen $since;"
done << 'FROZEN'
jig/sit cbb7bdd1f4ccdff0873f8ea0d71ac30da987e5c7 2026-10-29
jig/fix 7d2f7acd1be82ce0f99bcde9c6649d774ee2c635 a8bfd5a
FROZEN
[ -z "$bad" ] || { echo "the race's control moved:$bad"; exit 1; }
echo "the control as frozen: jig/sit at cbb7bdd1 (2026-10-29), jig/fix at 7d2f7acd (a8bfd5a)"
