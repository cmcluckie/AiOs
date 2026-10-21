#!/bin/sh
# Probe for TST.divide64.exact@1: the divide helper in pool/PART.divide64@1, built
# for 32-bit x86 beside a checker that holds forty divisions worked out beforehand,
# and run on the jig itself (a host program, not AiOs).  Exit 0 green, 1 red, 2 this
# host cannot build or run a 32-bit program.
set -u
cd "$(dirname "$0")/../.."
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
${CC:-gcc} -m32 -O2 -I tests/fixtures/divide64 -o "$t/check" tests/fixtures/divide64/check.c pool/PART.divide64@1/divide64.c 2> "$t/err" || { echo "cannot build a 32-bit program here: $(head -1 "$t/err")"; exit 2; }
"$t/check" > "$t/out" 2>&1; rc=$?
[ $rc -le 1 ] || { echo "cannot run a 32-bit program here (exit $rc)"; exit 2; }
[ $rc -eq 0 ] || { cat "$t/out"; exit 1; }
nm "$t/check" | grep -q " T __udivmoddi4$" || { echo "the checker did not use the pool's helper"; exit 1; }
cat "$t/out"
