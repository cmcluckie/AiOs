#!/bin/sh
# Probe for TST.disk.no-disk@1: sort, built as usual, booted on the serial-only
# machine, which has no disk.  Green when it halts and says it could not save.
set -u
cd "$(dirname "$0")/../.."
sh jig/aios build sort > /dev/null || { echo "build of sort failed"; exit 1; }
out=$(timeout 30 sh pool/CMP.machine.serial-only@1/machine.sh out/sort/loader.elf out/sort/sort.bin < /dev/null 2> /dev/null)
rc=$?
[ $rc -eq 124 ] && { echo "still running after 30s with no disk"; exit 1; }
printf '%s\n' "$out" | grep -q '^could not save "sorted"$' || { echo "did not say it could not save: $(printf '%s' "$out" | tail -1)"; exit 1; }
echo "no disk: halted, and said it could not save"
